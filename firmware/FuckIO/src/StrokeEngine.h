#pragma once

#include <Arduino.h>
#include <pattern.h>
#include <config.h>

// Debug Levels
#define DEBUG_VERBOSE               // Show debug messages from the StrokeEngine on Serial
//#define DEBUG_STROKE                // Show debug messaged for each individual stroke on Serial
#define DEBUG_PATTERN               // Show debug messages from inside pattern generator on Serial

// Servo Settings:
#ifndef STEP_PER_REV
  #define STEP_PER_REV      3200      // How many steps per revolution of the motor (S1 on)
#endif

#ifndef PULLEY_TEETH
  #define PULLEY_TEETH      20        // How many teeth has the pulley
#endif

#ifndef BELT_PITCH
  #define BELT_PITCH        2         // What is the timing belt pitch in mm
#endif

#ifndef MAX_TRAVEL
  #define MAX_TRAVEL        160       // What is the maximum physical travel in mm
#endif

#ifndef KEEPOUT_BOUNDARY
  #define KEEPOUT_BOUNDARY  5         // Soft endstop preventing hard crashes in mm. Will be 
#endif                                // subtracted twice from MAX_TRAVEL
                                      // Should be sufficiently to completley drive clear from 
                                      // homing switch
#ifndef MAX_RPM                                     
  #define MAX_RPM           2900      // What is the maximum RPM of the servo
#endif

#ifndef MAX_ACCEL 
  #define MAX_ACCEL         300000    // Maximum acceleration in mm/s^2
#endif

#ifndef INVERT_DIRECTION
  #define INVERT_DIRECTION  false     // Set to true to invert the direction signal
#endif                                // The firmware expects the home switch to be located at the 
                                      // end of an retraction move. That way the machine homes 
                                      // itself away from the body. Home position is -KEEPOUTBOUNDARY

#ifndef SERVO_ACTIVE_LOW                         
  #define SERVO_ACTIVE_LOW  true      // Polarity of the enable signal. True for active low.
#endif

// Motion Settings:
#ifndef HOMING_SPEED
  #define HOMING_SPEED      5 * STEP_PER_MM       // Home with 5 mm/s
#endif

#ifndef HOMING_ACCEL
  #define HOMING_ACCEL      MAX_STEP_ACCEL / 10     // Acceleation is 10% of max allowed acceleration
#endif

#ifndef SAFE_SPEED
  #define SAFE_SPEED        10 * STEP_PER_MM      // Safe Speed is 10 mm/s
#endif

#ifndef SAFE_ACCEL
  #define SAFE_ACCEL        MAX_STEP_ACCEL / 10     // Acceleation is 10% of max allowed acceleration
#endif

// Default Settings:
#ifndef STROKE
  #define STROKE            60        // Stroke defaults to 60 mm
#endif

#ifndef DEPTH
  #define DEPTH             TRAVEL    // Stroke is carried out at the front of the machine
#endif

#ifndef SPEED
  #define SPEED             60        // 60 Strokes per Minute
#endif

// Derived Servo Settings:
#define STEP_PER_MM       STEP_PER_REV / (PULLEY_TEETH * BELT_PITCH)
#define TRAVEL            (MAX_TRAVEL - (2 * KEEPOUT_BOUNDARY))
#define MIN_STEP          0
#define MAX_STEP          int(0.5 + TRAVEL * STEP_PER_MM)
#define MAX_STEP_PER_SEC  int(0.5 + (MAX_RPM * STEP_PER_REV) / 60)
#define MAX_STEP_ACCEL    int(0.5 + MAX_ACCEL * STEP_PER_MM)

#ifndef STRING_LEN
  #define STRING_LEN           64             // Bytes used to initalize char array. No path, topic, name, etc. should exceed this value
#endif

/**************************************************************************/
/*!
  @brief  Enum containing the states of the state machine
*/
/**************************************************************************/
typedef enum {
  SERVO_DISABLED,          //!< No power to the servo. We don't know its position
  SERVO_READY,             //!< Servo is energized and knows it position. Not running.
  SERVO_ERROR,             //!< Servo is on error state. Needs to be cleared by removing power
  SERVO_RUNNING,           //!< Stroke Engine is running and servo is moving according to defined pattern
  SERVO_STREAMING          //!< Stroke Engine is running and is receaiving a stream of position data
} ServoState;

// Verbose strings of states for debugging purposes
static String verboseState[] = {
  "[0] Servo disabled",
  "[1] Servo ready",
  "[2] Servo error",
  "[3] Servo running",
  "[4] Servo streaming"
};

/**************************************************************************/
/*!
  @brief  Stroke Engine provides a conveniant package for stroking motions
  created by stepper or servo motors. It's internal states are handled by a 
  finite state machine. A pattern generator allows to creat a variaty of 
  motion profiles. Under the hood FastAccelStepper is used for interfacing
  a stepper or servo motor vie a STEP/DIR interface.  
*/
/**************************************************************************/
class StrokeEngine {
    public:

        /**************************************************************************/
        /*!
          @brief  Initializes FastAccelStepper and configures all pins and outputs
          accordingly. StrokeEngine is in state SERVO_DISABLED
        */
        /**************************************************************************/
        void begin();

        /**************************************************************************/
        /*!
          @brief  Set the speed of a stroke. Speed is given in Strokes per Minute
          and internally calculated to the time a full stroke needs to complete. 
          Settings tale effect with next stroke, or after calling 
          applyNewSettingsNow().
          @param speed Strokes per Minute. Is constrained from 0.5 to 6000 
        */
        /**************************************************************************/
        void setSpeed(float speed);

        /**************************************************************************/
        /*!
          @brief  Set the depth of a stroke. Settings tale effect with next stroke, 
          or after calling applyNewSettingsNow().
          @param speed Depth in [mm]. Is constrained from 0 to TRAVEL 
        */
        /**************************************************************************/
        void setDepth(float depth);

        /**************************************************************************/
        /*!
          @brief  Set the stroke length of a stroke. Settings tale effect with next 
          stroke, or after calling applyNewSettingsNow().
          @param speed Stroke length in [mm]. Is constrained from 0 to TRAVEL 
        */
        /**************************************************************************/
        void setStroke(float stroke);

        /**************************************************************************/
        /*!
          @brief  Set the depthsensation of a pattern. Settings tale effect with next 
          stroke, or after calling applyNewSettingsNow().
          @param speed  Sesation in [mm]. Is constrained from -100 to 100 with 0 
                        beeing assumed as neutral 
        */
        /**************************************************************************/
        void setSensation(float sensation);

        /**************************************************************************/
        /*!
          @brief  Choose a pattern for the StrokeEngine. Settings take effect with 
          next stroke, or after calling applyNewSettingsNow(). 
          @param patternIndex  Index of a pattern
          @return TRUE on success, FALSE, if patternIndex is invalid. Previous 
                        pattern will be retained.
        */
        /**************************************************************************/
        bool setPattern(int patternIndex);

        /**************************************************************************/
        /*!
          @brief  Normally parameter changes take affect only with next stroke. If
          immideate parameter changes are desired call applyNewSettingsNow() to 
          change even mid-stroke.
        */
        /**************************************************************************/
        bool applyNewSettingsNow();

        /**************************************************************************/
        /*!
          @brief  Creates a FreeRTOS task to run a stroking pattern. Only valid in
          state SERVO_READY. Pattern is initilized with the values from the set 
          functions. If the task is running, state is SERVO_RUNNING.
          @return TRUE when task was created and motion starts, FALSE on failure.
        */
        /**************************************************************************/
        bool startMotion();

        /**************************************************************************/
        /*!
          @brief  Stops the motion with MAX_ACCEL and deletes the stroking task. Is
          in state SERVO_READY faterwards.
        */
        /**************************************************************************/
        void stopMotion();

        /**************************************************************************/
        /*!
          @brief  Enable the servo/stepper and do the homing procedure. Drives towards
          the endstop with HOMING_SPEED. Function is non-blocking and backed by a task.
          Optionally a callback can be given to receive feedback if homing succeded 
          going in state SERVO_READY. If homing switch is not found after traveling 
          MAX_TRAVEL it times out, disables the servo and goes into SERVO_DISABLED. 
          @param callBackHoming Callback function is called after homing is done. 
                        Function parametere holds a bool containing the success (TRUE)
                        or failure (FALSE) of homing.
        */
        /**************************************************************************/
        void enableAndHome();
        void enableAndHome(void(*callBackHoming)(bool));

        /**************************************************************************/
        /*!
          @brief  If no homing switch is present homing can be done manually. Push 
          the endeffector all the way in and call thisIsHome(). This enables the
          the servo and sets the position to -KEEPOUT_BOUNDARY
        */
        /**************************************************************************/
        void thisIsHome();

        /**************************************************************************/
        /*!
          @brief  In state SERVO_RUNNING and SERVO_READY this moves the endeffector
          to TRAVEL with SAFE_SPEED. Can be used for adjustments. Stops any running
          pattern and ends in state SERVO_READY.
          @return TRUE on success, FALSE if state does not allow this.
        */
        /**************************************************************************/
        bool moveToMax();

        /**************************************************************************/
        /*!
          @brief  In state SERVO_RUNNING and SERVO_READY this moves the endeffector
          to 0 with SAFE_SPEED. Can be used for adjustments. Stops any running
          pattern and ends in state SERVO_READY.
          @return TRUE on success, FALSE if state does not allow this.
        */
        /**************************************************************************/
        bool moveToMin();

        /**************************************************************************/
        /*!
          @brief  Retrieves the current servo state from the internal state machine.
          @return Current state of the state machine
        */
        /**************************************************************************/
        ServoState getState();

        /**************************************************************************/
        /*!
          @brief  Disables the servo motor instantly and deletes any motion task. 
          Sets state machine to SERVO_DISABLED. Must be followed by homing to enable
          servo again. 
        */
        /**************************************************************************/
        void disable();

        /**************************************************************************/
        /*!
          @brief  This function internally calls disable() and sets the state machine
          to SERVO_ERROR. Intended to be called from an interrupt if a servo/stepper
          fault signal should be monitored. Propably not interrupt safe, but this 
          maybe even doesn't matter.
        */
        /**************************************************************************/
        void safeState();

        /**************************************************************************/
        /*!
          @brief  Creates a JSON describing all available pattern from pattern.h
          @return JSON string
        */
        /**************************************************************************/
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

