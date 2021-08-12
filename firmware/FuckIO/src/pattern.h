#pragma once

#include <Arduino.h>
#include <config.h>
#include <math.h>
#include "PatternMath.h"

/**************************************************************************/
/*!
  @brief  struct to return all parameters FastAccelStepper needs to calculate
  the trapezoidal profile.
*/
/**************************************************************************/
typedef struct {
    int position;       //!< Absolute target position of a move in steps 
    int speed;          //!< Speed of a move in Hz 
    int acceleration;   //!< Acceleration to get to speed or halt 
} motionParameter;

/**************************************************************************/
/*!
  @class Pattern 
  @brief  Load the stepper configuration and checker whether a valid pulse 
  was used. Selects the correct timer peripherial and configures all stepper
  related pins as inputs & outputs. Enable the clocks for timer TCC0 to TC3
  with GCLKDiv = 1.
*/
/**************************************************************************/
class Pattern {

    public:
        Pattern(char *str) { strcpy(_name, str); }
        virtual void setTimeOfStroke(float speed) { _timeOfStroke = speed; }
        virtual void setDepth(int depth) { _depth = depth; }
        virtual void setStroke(int stroke) { _stroke = stroke; }
        virtual void setSensation(float sensation) { _sensation = sensation; } 
        char *getName() { return _name; }
        virtual motionParameter nextTarget(unsigned int index) {
           // #error "nextTarget() must be overridden."
            _index = index;
            return _nextMove;
        } 

    protected:
        int _depth;
        int _stroke;
        float _timeOfStroke;
        float _sensation = 0.0;
        unsigned int _index;
        char _name[STRING_LEN]; 
        motionParameter _nextMove = {0, 0, 0};

};

/**************************************************************************/
/*!
  @brief  Load the stepper configuration and checker whether a valid pulse 
  was used. Selects the correct timer peripherial and configures all stepper
  related pins as inputs & outputs. Enable the clocks for timer TCC0 to TC3
  with GCLKDiv = 1.
*/
/**************************************************************************/
class SimpleStroke : public Pattern {
    public:
        SimpleStroke(char *str) : Pattern(str) {}
        void setTimeOfStroke(float speed = 0) { _timeOfStroke = 0.5 * speed; }
        motionParameter nextTarget(unsigned int index) {
            _nextMove.speed = int(1.5 * _stroke/_timeOfStroke);                   // maximum speed of the trapezoidal motion 
            _nextMove.acceleration = int(3.0 * _nextMove.speed/_timeOfStroke);    // acceleration to meet the profile
            if (index % 2) {
                _nextMove.position = _depth - _stroke;
            } else {
                _nextMove.position = _depth;
            }
            _index = index;
            return _nextMove;
        }
};

/**************************************************************************/
/*!
  @brief  Load the stepper configuration and checker whether a valid pulse 
  was used. Selects the correct timer peripherial and configures all stepper
  related pins as inputs & outputs. Enable the clocks for timer TCC0 to TC3
  with GCLKDiv = 1.
*/
/**************************************************************************/
class TeasingPounding : public Pattern {
    public:
        TeasingPounding(char *str) : Pattern(str) {}
        void setSensation(float sensation) { 
            _sensation = sensation;
            _updateStrokeTiming();
        }
        void setTimeOfStroke(float speed = 0) {
            _timeOfStroke = speed;
            _updateStrokeTiming();
        }
        motionParameter nextTarget(unsigned int index) {
            if (index % 2) {
                _nextMove.speed = int(1.5 * _stroke/_timeOfOutStroke);                   // maximum speed of the trapezoidal motion 
                _nextMove.acceleration = int(3.0 * float(_nextMove.speed)/_timeOfOutStroke);    // acceleration to meet the profile
                _nextMove.position = _depth - _stroke;
            } else {
                _nextMove.speed = int(1.5 * _stroke/_timeOfInStroke);                   // maximum speed of the trapezoidal motion 
                _nextMove.acceleration = int(3.0 * float(_nextMove.speed)/_timeOfInStroke);    // acceleration to meet the profile
                _nextMove.position = _depth;
            }
            _index = index;
            return _nextMove;
        }
    protected:
        float _timeOfFastStroke = 1.0;
        float _timeOfInStroke = 1.0;
        float _timeOfOutStroke = 1.0;
        void _updateStrokeTiming() {
            _timeOfFastStroke = (0.5 * _timeOfStroke) / fscale(0.0, 100.0, 1.0, 5.0, abs(_sensation), 0.0);
            if (_sensation > 0.0) {
                _timeOfInStroke = _timeOfFastStroke;
                _timeOfOutStroke = _timeOfStroke - _timeOfFastStroke;
            } else {
                _timeOfOutStroke = _timeOfFastStroke;
                _timeOfInStroke = _timeOfStroke - _timeOfFastStroke;
            }
#ifdef DEBUG_PATTERN
            Serial.println("TimeOfInStroke: " + String(_timeOfInStroke));
            Serial.println("TimeOfOutStroke: " + String(_timeOfOutStroke));
#endif
        }
};

/**************************************************************************/
/*!
  @brief  Load the stepper configuration and checker whether a valid pulse 
  was used. Selects the correct timer peripherial and configures all stepper
  related pins as inputs & outputs. Enable the clocks for timer TCC0 to TC3
  with GCLKDiv = 1.
*/
/**************************************************************************/
static SimpleStroke p00 = SimpleStroke("Simple Stroke");
static TeasingPounding p01 = TeasingPounding("Teasing or Pounding");

static Pattern *patternTable[] = { &p00, &p01 };
static const unsigned int patternTableSize = sizeof(patternTable) / sizeof(patternTable[0]);
