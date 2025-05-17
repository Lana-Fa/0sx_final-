#include "PorteAutomatique.h"

PorteAutomatique::PorteAutomatique(int p1, int p2, int p3, int p4, float& distancePtr)
  : _stepper(AccelStepper::HALF4WIRE, p1, p3, p2, p4),
    _distance(distancePtr) {
  _stepper.setMaxSpeed(1000);
  _stepper.setAcceleration(300);
  _stepper.setSpeed(300);
  _stepper.setCurrentPosition(_angleEnSteps(_angleFerme));
}

void PorteAutomatique::update() {
  _currentTime = millis();
  _mettreAJourEtat();

  switch (_etat) {
    case FERMEE:
      _fermeState();
      break;
    case OUVERTE:
      _ouvertState();
      break;
    case EN_OUVERTURE:
      _ouvertureState();
      break;
    case EN_FERMETURE:
      _fermetureState();
      break;
  }

  _stepper.run(); 
}

void PorteAutomatique::_mettreAJourEtat() {
  // Distance actuelle est trop proche -> ouvrir
  if (_etat == FERMEE && _distance < _distanceOuverture) {
    _etat = EN_OUVERTURE;
    _ouvrir();
  }

  // Distance actuelle est trop loin -> fermer
  if (_etat == OUVERTE && _distance > _distanceFermeture) {
    _etat = EN_FERMETURE;
    _fermer();
  }
}

void PorteAutomatique::_fermeState() {
  
}

void PorteAutomatique::_ouvertState() {
  
}

void PorteAutomatique::_ouvertureState() {
  if (!_stepper.isRunning()) {
    _etat = OUVERTE;
  }
}

void PorteAutomatique::_fermetureState() {
  if (!_stepper.isRunning()) {
    _etat = FERMEE;
  }
}

void PorteAutomatique::_ouvrir() {
  long target = _angleEnSteps(_angleOuvert);
  _stepper.moveTo(target);
}

void PorteAutomatique::_fermer() {
  long target = _angleEnSteps(_angleFerme);
  _stepper.moveTo(target);
}

void PorteAutomatique::setAngleOuvert(float angle) {
  _angleOuvert = angle;
}

void PorteAutomatique::setAngleFerme(float angle) {
  _angleFerme = angle;
}

void PorteAutomatique::setPasParTour(int steps) {
  _stepsPerRev = steps;
}

void PorteAutomatique::setDistanceOuverture(float distance) {
  _distanceOuverture = distance;
}

void PorteAutomatique::setDistanceFermeture(float distance) {
  _distanceFermeture = distance;
}

const char* PorteAutomatique::getEtatTexte() const {
  switch (_etat) {
    case FERMEE: return "Fermee";
    case OUVERTE: return "Ouverte";
    case EN_OUVERTURE: return "En ouverture";
    case EN_FERMETURE: return "En fermeture";
    default: return "Inconnu";
  }
}

float PorteAutomatique::getAngle() const {
  long position = _stepper.currentPosition();
  return (float)position * 360.0 / _stepsPerRev;
}

long PorteAutomatique::_angleEnSteps(float angle) const {
  return angle * _stepsPerRev / 360.0;
}
