#ifndef MountStepper_h
#define MountStepper_h

#include "Arduino.h"

class MountStepper {
    public:
        MountStepper(int stepPin, int dirPin, float gearRatio, signed long zeroSpeed, signed long maxSpeed, float maxPos);
        void run();
        void setPos(float pos);
        void setSpeed(signed long controlSpeed);
        void setSpeedRamp(signed long controlSpeed);
        void setDir(byte dir);
        void GoTo(float goPos);
        void stop();
        volatile float degrees;
        signed long speed;
        byte _dir;
    private:
        float _maxPos;
        byte _ramp;
        int _rampSpeed;
        signed long _targetSpeed;
        signed int _lastRampAdjust;
        int _stepPin;
        int _dirPin;
        float _gearRatio;
        volatile float _goToPos;
        volatile byte _goToMove;
        volatile byte _halfStep;
        volatile signed long _timeDiff;
        volatile signed long _lastStep;
        volatile signed long _lastRamp;
        volatile signed long _currTimeStep;
        volatile signed long _zeroSpeed;
        volatile byte _motion;
        signed long _maxSpeed;
};

#endif
