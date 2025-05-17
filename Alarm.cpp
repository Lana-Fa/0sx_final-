#include "Alarm.h"
#include <Arduino.h>

Alarm::Alarm(int rPin, int gPin, int bPin, int buzzerPin, float* distancePtr)
  : _rPin(rPin), _gPin(gPin), _bPin(bPin), _buzzerPin(buzzerPin), _distance(distancePtr) {
  pinMode(_rPin, OUTPUT);
  pinMode(_gPin, OUTPUT);
  pinMode(_bPin, OUTPUT);
  pinMode(_buzzerPin, OUTPUT);
  _turnOff(); 
}

void Alarm::update() {
  _currentTime = millis();

  // Gestion des transitions externes
  if (_turnOnFlag) {
    _turnOnFlag = false;
    _state = WATCHING;
  }

  if (_turnOffFlag) {
    _turnOffFlag = false;
    _state = OFF;
  }

  switch (_state) {
    case OFF:       _offState(); break;
    case WATCHING:  _watchState(); break;
    case ON:        _onState(); break;
    case TESTING:   _testingState(); break;
  }
}

void Alarm::turnOn() {
  _turnOnFlag = true;
}

void Alarm::turnOff() {
  _turnOffFlag = true;
}

void Alarm::test() {
  _state = TESTING;
  _testStartTime = _currentTime;
  digitalWrite(_buzzerPin, HIGH);
  _setRGB(255, 255, 255); 
}

AlarmState Alarm::getState() const {
  return _state;
}

void Alarm::setColourA(int r, int g, int b) {
  _colA[0] = r; _colA[1] = g; _colA[2] = b;
}

void Alarm::setColourB(int r, int g, int b) {
  _colB[0] = r; _colB[1] = g; _colB[2] = b;
}

void Alarm::setVariationTiming(unsigned long ms) {
  _variationRate = ms;
}

void Alarm::setDistance(float d) {
  _distanceTrigger = d;
}

void Alarm::setTimeout(unsigned long ms) {
  _timeoutDelay = ms;
}

void Alarm::_setRGB(int r, int g, int b) {
  digitalWrite(_rPin, r);
  digitalWrite(_gPin, g);
  digitalWrite(_bPin, b);
}

void Alarm::_turnOff() {
  digitalWrite(_buzzerPin, LOW);
  _setRGB(0, 0, 0);
}

// ----- États FSM -----

void Alarm::_offState() {
  _turnOff();
}

void Alarm::_watchState() {
  if (*_distance < _distanceTrigger) {
    _lastDetectedTime = _currentTime;
    _state = ON;
    digitalWrite(_buzzerPin, HIGH);
  } else {
    digitalWrite(_buzzerPin, LOW);
    _setRGB(0, 0, 0); // 
  }
}

void Alarm::_onState() {
  if (_currentTime - _lastUpdate >= _variationRate) {
    _lastUpdate = _currentTime;
    _currentColor = !_currentColor;
    if (_currentColor)
      _setRGB(_colA[0], _colA[1], _colA[2]);
    else
      _setRGB(_colB[0], _colB[1], _colB[2]);
  }

  if (*_distance >= _distanceTrigger && (_currentTime - _lastDetectedTime >= _timeoutDelay)) {
    _state = WATCHING;
    digitalWrite(_buzzerPin, LOW);
    _setRGB(0, 0, 0); // 
  }
}

void Alarm::_testingState() {
  if (_currentTime - _testStartTime >= 3000) {
    _turnOff();
    _state = OFF;
  }
}
