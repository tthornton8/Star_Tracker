#include "Arduino.h"
#include "MountStepper.h"

MountStepper::MountStepper(int stepPin, int dirPin, float gearRatio, signed long zeroSpeed, signed long maxSpeed, float maxPos)
{
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);

    _stepPin = stepPin;
    _dirPin = dirPin;
    _gearRatio = gearRatio;
    _zeroSpeed = zeroSpeed;
    _maxSpeed = maxSpeed;
    _dir = 1;
    _halfStep = 1;
    _lastStep = 0;
    _lastRampAdjust = 0;
    speed = 100;
    degrees = 0;
    _ramp = 0;
    _lastRamp = 0;
    _rampSpeed = 100;
    _maxPos = maxPos;
}

void MountStepper::run() {
  if (degrees > _maxPos) {
    stop();
  }
  
  if (speed < _maxSpeed) {
    speed = _maxSpeed;
  }
  _currTimeStep = micros();
  _timeDiff = abs(_currTimeStep-_lastStep);

  if (_ramp && (abs(_currTimeStep-_lastRamp) > 1e5) ) {
    if (speed > _targetSpeed) {
      if (_lastRampAdjust == 1) {
        speed = _targetSpeed;
        _ramp = 0;
      } else {
        _lastRampAdjust = -1;
        speed = speed - _rampSpeed; 
      }
    } else {
      if (_lastRampAdjust == -1) {
        speed = _targetSpeed;
        _ramp = 0;
      } else {
        _lastRampAdjust = 1;
        speed = speed + _rampSpeed; 
      }
    }
  }

  if ((_timeDiff > speed/2) && (_timeDiff < speed) && _halfStep && _motion) {
    digitalWrite(_stepPin, HIGH);
    _halfStep = 0;
  } else if ((_timeDiff > speed) && _motion) {
    // Set direction
    digitalWrite(_dirPin, _dir);
    
    // Step Once
    digitalWrite(_stepPin, LOW);

    if (_dir == HIGH) {
      degrees = degrees + 1.8/(16*_gearRatio);
    } else {
      degrees = degrees - 1.8/(16*_gearRatio);
    }

    degrees = degrees > 360 ? 0 : degrees;
    degrees = degrees < 0 ? 360 : degrees;

    _lastStep = _currTimeStep;
    _halfStep = 1;
  }

  if (_goToMove && _motion) {
    speed = 1e2*log(1e6/abs(degrees-_goToPos));
    //Serial.println(speed);
    
    if (abs(_goToPos-degrees) < 3.6/(16*_gearRatio)) {
      _goToMove = 0;
      stop();
    }
    if (degrees < _goToPos) {
      _dir = HIGH;
    } else {
      _dir = LOW;
    }
  }
}

void MountStepper::stop() {
  speed = _zeroSpeed;
  _goToMove = 0;
  if (!(_zeroSpeed)) {
    _motion = 0;
  }
}

void MountStepper::GoTo(float goPos) {
  _goToMove = 1;
  _goToPos = goPos;
  speed = 1e2*log(1e6/abs(degrees-_goToPos));
  _motion = 1;
}

void MountStepper::setPos(float pos) {
  degrees = pos;
}

void MountStepper::setSpeed(signed long controlSpeed) {
  speed = controlSpeed;
  _motion = 1;
}

void MountStepper::setSpeedRamp(signed long controlSpeed) {
  _targetSpeed = controlSpeed;
  _motion = 1;
  _ramp = 1;
}

void MountStepper::setDir(byte dir) {
  _dir = dir;
  _motion = 1;
}
