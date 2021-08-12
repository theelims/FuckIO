#pragma once

#include <Arduino.h>
#include "pattern.h"

// Enum to store the servo state
typedef enum {
  SERVO_DISABLED,          // No power to the servo. We don't know its position
  SERVO_READY,             // Servo is energized and knows it position. Not running.
  SERVO_ERROR,             // Servo is on error state. Needs to be cleared by removing power
  SERVO_RUNNING,           // Stroke Engine is running and servo is moving according to defined pattern
  SERVO_STREAMING          // Stroke Engine is running and is receaiving a stream of position data
} ServoState;

static String verboseState[] = {
  "[0] Servo disabled",
  "[1] Servo ready",
  "[2] Servo error",
  "[3] Servo running",
  "[4] Servo streaming"
};

class StrokeEngine {
    public:
        void begin();
        void setSpeed(float speed);
        void setDepth(float depth);
        void setStroke(float stroke);
        void setSensation(float sensation);
        bool setPattern(int patternIndex);
        bool applyNewSettingsNow();
        bool startMotion();
        void stopMotion();
        void enableAndHome();
        void enableAndHome(void(*callBackHoming)(bool));
        void thisIsHome();
        bool moveToMax();
        bool moveToMin();
        ServoState getState();
        void disable();
        void safeState();
        String getPatternJSON();

    protected:
        ServoState _state = SERVO_DISABLED;
        int _patternIndex = 0;
        bool _isHomed = false;
        int _index = 0;
        int _depth;
        int _stroke;
        float _timeOfStroke;
        float _sensation;
        static void _homingProcedureImpl(void* _this) { static_cast<StrokeEngine*>(_this)->_homingProcedure(); }
        void _homingProcedure();
        static void _strokingImpl(void* _this) { static_cast<StrokeEngine*>(_this)->_stroking(); }
        void _stroking();
        void _applyMotionProfile(motionParameter* motion);
        TaskHandle_t _taskStrokingHandle = NULL;
        TaskHandle_t _taskHomingHandle = NULL;
        void(*_callBackHomeing)(bool);
};

