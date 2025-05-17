#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <U8g2lib.h>
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include "Alarm.h"
#include "PorteAutomatique.h"

// --- Pins ---
#define TRIG 8
#define ECHO 9
#define BUZZER 6
#define LED_ROUGE 4
#define LED_BLEUE 5
#define LED_VERTE 3

#define IN1 31
#define IN2 33
#define IN3 35
#define IN4 37
#define LUM_PIN A0 

// --- WiFi & MQTT ---
#define HAS_SECRETS 1
#if HAS_SECRETS
#include "arduino_secrets.h"
const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;
#else
const char ssid[] = "TechniquesInformatique-Etudiant";
const char pass[] = "shawi123";
#endif

#define AT_BAUD_RATE 115200
#define DEVICE_NAME "ArduinoMegaClient"

const char* mqttServer = "216.128.180.194";
#define MQTT_PORT 1883
#define MQTT_USER "etdshawi"
#define MQTT_PASS "shawi123"

WiFiClient wifiClient;
PubSubClient client(wifiClient);


LiquidCrystal_I2C lcd(0x27, 16, 2);
U8G2_MAX7219_8X8_F_4W_SW_SPI u8g2(U8G2_R0, 30, 34, 32, U8X8_PIN_NONE, U8X8_PIN_NONE);

float distance = 100.0;
int luminosity = 0;
unsigned long currentTime = 0;


Alarm alarm(LED_ROUGE, LED_VERTE, LED_BLEUE, BUZZER, &distance);
PorteAutomatique porte(IN1, IN2, IN3, IN4, distance);

// --- Lecture de distance ---
float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  float dist = duration * 0.034 / 2;
  if (dist < 2 || dist > 400) dist = 100.0;
  return dist;
}

// --- Lecture de la luminosité ---
int getLuminosity() {
  return map(analogRead(LUM_PIN), 0, 1023, 0, 100);
}

// --- Configuration de l'alarme ---
void configureAlarm() {
  alarm.setDistance(15);         
  alarm.setTimeout(5000);        
  alarm.setVariationTiming(500); 
  alarm.setColourA(255, 0, 0);   
  alarm.setColourB(0, 0, 255);   
  alarm.turnOn();                
}

// --- Affichage LCD ---
void lcdTask() {
  lcd.setCursor(0, 0);
  lcd.print("Dist : ");
  lcd.print((int)distance);
  lcd.print(" cm   ");

  lcd.setCursor(0, 1);
  lcd.print("Porte: ");
  const char* etat = porte.getEtatTexte();
  if (strcmp(etat, "Ouverte") == 0 || strcmp(etat, "Fermee") == 0) {
    lcd.print(etat);
  } else {
    lcd.print((int)porte.getAngle());
    lcd.print((char)223);
    lcd.print(" deg");
  }
}

// --- Configuration MQTT ---
void setupMQTT() {
  client.setServer(mqttServer, MQTT_PORT);
  client.setCallback(mqttEvent);
  reconnect();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connexion MQTT...");
    if (client.connect(DEVICE_NAME, MQTT_USER, MQTT_PASS)) {
      Serial.println("Connecté au serveur MQTT.");
      client.subscribe("etd/33/command");
    } else {
      delay(2000);
    }
  }
}

// --- Gestion des messages MQTT ---
void mqttEvent(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// --- Envoi périodique des données MQTT ---
void periodicTask() {
  static unsigned long lastTime = 0;
  if (millis() - lastTime < 2500) return;
  lastTime = millis();
  unsigned long uptime = millis() / 1000;

  distance = getDistance();
  luminosity = getLuminosity();
  float angle = porte.getAngle();
  const char* etat = porte.getEtatTexte();

  String line1 = "Dist : " + String((int)distance) + " cm";
  String line2 = "Porte: ";
  if (strcmp(etat, "Ouverte") == 0 || strcmp(etat, "Fermee") == 0) {
    line2 += etat;
  } else {
    line2 += String((int)angle) + " deg";
  }

  String message = "{";
  message += "\"name\":\"Falana\",";
  message += "\"number\":\"2412384\",";
  message += "\"uptime\":" + String(uptime) + ",";
  message += "\"dist\":" + String(distance, 2) + ",";
  message += "\"angle\":" + String(angle, 2) + ",";
  message += "\"motor\":" + String((etat == "Ouverte") ? 1 : 0) + ",";
  message += "\"lum\":" + String(luminosity) + ",";
  message += "\"line1\":\"" + line1 + "\",";
  message += "\"line2\":\"" + line2 + "\",";
  message += "\"color\":\"#2200ff\"";
  message += "}";

  client.publish("etd/33/data", message.c_str());
  Serial.println("Message MQTT envoyé : " + message);
}


void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LUM_PIN, INPUT);

  lcd.init();
  lcd.backlight();
  u8g2.begin();
  u8g2.setFont(u8g2_font_4x6_tr);

  // Configuration de l'alarme
  alarm.setDistance(15);          
  alarm.setTimeout(3000);        
  alarm.setVariationTiming(300);  
  alarm.setColourA(255, 0, 0);    
  alarm.setColourB(0, 0, 255);    
  alarm.turnOn();                 

  // Configuration de la porte automatique
  porte.setAngleOuvert(170);      
  porte.setAngleFerme(10);        
  porte.setPasParTour(2048);      
  porte.setDistanceOuverture(30); 
  porte.setDistanceFermeture(60); 

  wifiInit();
  setupMQTT();
}


// --- Initialisation WiFi ---
void wifiInit() {
  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(&Serial1);

  WiFi.disconnect();
  WiFi.setPersistent();
  WiFi.endAP();
  Serial.println("Connexion au WiFi...");
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connecté au WiFi.");
}

// --- Boucle principale ---
void loop() {
  currentTime = millis();
  distance = getDistance();
  luminosity = getLuminosity();
  
  alarm.update();
  porte.update();
  lcdTask();
  periodicTask();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
