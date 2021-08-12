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
  @brief  Base class to derive your pattern from. Offers a unified set of
          functions to store all relevant paramteres. These function can be
          overridenid necessary. Pattern should be self-containted and not 
          rely on any stepper/servo related properties. Internal book keeping
          is done in steps. The translation from real word units to steps is
          provided by the StrokeEngine. Also the sanity check whether motion
          parameters are physically possible are done by the StrokeEngine. 
          Imposible motion commands are clipped, cropped or adjusted while 
          still having a smooth appearance.  
*/
/**************************************************************************/
class Pattern {

    public:
        //! Constructor
        /*!
          @param str Sring containing the name of a pattern 
        */
        Pattern(char *str) { strcpy(_name, str); }

        //! Set the time a normal stroke should take to complete
        /*! 
          @param speed time of a full stroke in [sec] 
        */
        virtual void setTimeOfStroke(float speed) { _timeOfStroke = speed; }

        //! Set the depth of a stroke. Depth is the furthest position a stroke ever reaches
        /*! 
          @param depth depth in Steps 
        */
        virtual void setDepth(int depth) { _depth = depth; }

        //! Set the maximum stroke a pattern may have
        /*! 
          @param stroke stroke distance in Steps 
        */
        virtual void setStroke(int stroke) { _stroke = stroke; }

        //! Sensation is a additional parametere a pattern can take to alter its behaviour
        /*! 
          @param sesation Arbitrary value from -100 to 100, with 0 beeing neutral 
        */
        virtual void setSensation(float sensation) { _sensation = sensation; } 

        //! Retrives the name of a pattern
        /*! 
          @return c_string containing the name of a pattern 
        */
        char *getName() { return _name; }

        //! Calculate the position of the next stroke based on the various parameters
        /*! 
          @param index index of a stroke. Increments with every new stroke. 
          @return Set of motion parameteres like speed, acceleration & position
        */
        virtual motionParameter nextTarget(unsigned int index) {
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
  @brief  Simple Stroke Pattern. It creates a trapezoidal stroke profile
  with 1/3 acceleration, 1/3 coasting, 1/3 deceleration. Sensation has 
  no effect.
*/
/**************************************************************************/
class SimpleStroke : public Pattern {
    public:
        SimpleStroke(char *str) : Pattern(str) {}

        void setTimeOfStroke(float speed = 0) { 
             // In & Out have same time, so we need to devide by 2
            _timeOfStroke = 0.5 * speed; 
        }   

        motionParameter nextTarget(unsigned int index) {
            // maximum speed of the trapezoidal motion 
            _nextMove.speed = int(1.5 * _stroke/_timeOfStroke);  

            // acceleration to meet the profile
            _nextMove.acceleration = int(3.0 * _nextMove.speed/_timeOfStroke);

            // odd stroke is moving out    
            if (index % 2) {
                _nextMove.position = _depth - _stroke;
            
            // even stroke is moving in
            } else {
                _nextMove.position = _depth;
            }

            _index = index;
            return _nextMove;
        }
};

/**************************************************************************/
/*!
  @brief  Simple pattern where the sensation value can change the speed 
  ratio between in and out. Sensation > 0 make the in move faster (up to 5x)
  giving a hard pounding sensation. Values < 0 make the out move going 
  faster. This gives a more pleasing sensation. The time for the overall 
  stroke remains the same. 
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
            // odd stroke is moving out
            if (index % 2) {
                // maximum speed of the trapezoidal motion
                _nextMove.speed = int(1.5 * _stroke/_timeOfOutStroke);  
                // acceleration to meet the profile                  
                _nextMove.acceleration = int(3.0 * float(_nextMove.speed)/_timeOfOutStroke);    
                _nextMove.position = _depth - _stroke;
            // even stroke is moving in
            } else {
                // maximum speed of the trapezoidal motion
                _nextMove.speed = int(1.5 * _stroke/_timeOfInStroke);        
                // acceleration to meet the profile            
                _nextMove.acceleration = int(3.0 * float(_nextMove.speed)/_timeOfInStroke);    
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
            // calculate the time it takes to complete the faster stroke
            // Division by 2 because reference is a half stroke
            _timeOfFastStroke = (0.5 * _timeOfStroke) / fscale(0.0, 100.0, 1.0, 5.0, abs(_sensation), 0.0);
            // positive sensation, in is faster
            if (_sensation > 0.0) {
                _timeOfInStroke = _timeOfFastStroke;
                _timeOfOutStroke = _timeOfStroke - _timeOfFastStroke;
            // negative sensation, out is faster
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
/*
  Array holding all different patterns. Please include any custom pattern here.
*/
/**************************************************************************/
static SimpleStroke p00 = SimpleStroke("Simple Stroke");
static TeasingPounding p01 = TeasingPounding("Teasing or Pounding");

static Pattern *patternTable[] = { &p00, &p01 };
static const unsigned int patternTableSize = sizeof(patternTable) / sizeof(patternTable[0]);
