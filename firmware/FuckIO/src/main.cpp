/**
 *   FuckIO Firmware for ESP32
 *   Use MQTT messages to control all built-in motion patterns
 *   https://github.com/theelims/FuckIO 
 *
 * Copyright (C) 2021 theelims <elims@gmx.net>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <Arduino.h>
#include "esp_log.h"
#include <config.h>
#include <housekeeping.h>
<<<<<<< Updated upstream
#include <FastAccelStepper.h>
=======
#include "motor/genericStepper.h"
>>>>>>> Stashed changes
#include <StrokeEngine.h>

/*#################################################################################################
##
##    G L O B A L    D E F I N I T I O N S   &   D E C L A R A T I O N S
##
##################################################################################################*/

// Step per mm calculation aid:
#define STEP_PER_REV      3200      // How many steps per revolution of the motor (S1 on)
#define PULLEY_TEETH      20        // How many teeth has the pulley
#define BELT_PITCH        2         // What is the timing belt pitch in mm
#define STEP_PER_MM       STEP_PER_REV / (PULLEY_TEETH * BELT_PITCH)

<<<<<<< Updated upstream
static motorProperties servoMotor {
  .stepsPerRevolution = STEP_PER_REV,     
  .maxRPM = 2900,                
  .maxAcceleration = 300000,      
=======
static motorProperties servoMotor {      
>>>>>>> Stashed changes
  .stepsPerMillimeter = STEP_PER_MM,   
  .invertDirection = true,      
  .enableActiveLow = true,      
  .stepPin = SERVO_PULSE,              
  .directionPin = SERVO_DIR,          
  .enablePin = SERVO_ENABLE              
}; 

<<<<<<< Updated upstream
static machineGeometry strokingMachine = {
  .physicalTravel = 160.0,       
  .keepoutBoundary = 5.0      
};

=======

GenericStepperMotor motor;
>>>>>>> Stashed changes
StrokeEngine Stroker;

String getPatternJSON();

/*#################################################################################################
##
##    C A L L B A C K S
##
##################################################################################################*/

void homingNotification(bool isHomed) {
  if (isHomed) {
    mqttNotify("Found home - Ready to rumble!");
  } else {
    mqttNotify("Homing failed!");
  }
}

void controlSpeed(String payload) {
  Serial.println("Speed: " + String(payload));
<<<<<<< Updated upstream
  Stroker.setSpeed(payload.toFloat());
  Stroker.applyNewSettingsNow();
=======
  Stroker.setParameter(StrokeParameter::RATE, payload.toFloat(), true);
>>>>>>> Stashed changes
}

void controlDepth(String payload) {
  Serial.println("Depth: " + String(payload));
<<<<<<< Updated upstream
  Stroker.setDepth(payload.toFloat());
  Stroker.applyNewSettingsNow();
=======
  Stroker.setParameter(StrokeParameter::DEPTH, payload.toFloat(), true);
>>>>>>> Stashed changes
}

void controlStroke(String payload) {
  Serial.println("Stroke: " + String(payload));
<<<<<<< Updated upstream
  Stroker.setStroke(payload.toFloat());
  Stroker.applyNewSettingsNow();
=======
  Stroker.setParameter(StrokeParameter::STROKE, payload.toFloat(), true);
>>>>>>> Stashed changes
}

void controlSensation(String payload) {
  Serial.println("Sensation: " + payload);
<<<<<<< Updated upstream
  Stroker.setSensation(payload.toFloat());
  Stroker.applyNewSettingsNow();
=======
  Stroker.setParameter(StrokeParameter::SENSATION, payload.toFloat(), true);
>>>>>>> Stashed changes
}

void receiveCommand(String payload) {
  Serial.println("Command: " + payload);
  if (payload.equals("start")) {
    Stroker.startMotion();
  }
  if (payload.equals("stop")) {
    Stroker.stopMotion();
  }
<<<<<<< Updated upstream
  if (payload.equals("retract")) {
    Stroker.moveToMin();
  }
  if (payload.equals("extend")) {
    Stroker.moveToMax();
  }
  if (payload.equals("disable")) {
    Stroker.disable();
  }
  if (payload.equals("home")) {
    Stroker.enableAndHome(SERVO_ENDSTOP, true, homingNotification);
=======
  // if (payload.equals("retract")) {
  //   Stroker.moveToMin();
  // }
  // if (payload.equals("extend")) {
  //   Stroker.moveToMax();
  // }
  // if (payload.equals("setup")) {
  //   Stroker.setupDepth(10.0, true);
  // }
  // if (payload.equals("disable")) {
  //   Stroker.disable();
  // }
  if (payload.equals("home")) {
    motor.home(homingNotification);
  }
  if (payload.equals("patternlist")) {
    mqttPublish("/config", getPatternJSON());
>>>>>>> Stashed changes
  }
}

void receivePWM(String payload) {
  Serial.println("PWM: " + payload);

  // Convert payload to int
  int pulseWidth = payload.toInt();

  // constrain to 8-bit value range
  pulseWidth = constrain(pulseWidth, 0, 255);

  // set PWM
  ledcWrite(0, 0);
}

void receivePattern(String payload) {
  Serial.println("Pattern Index: " + String(payload));
  Stroker.setPattern(payload.toInt());
  Stroker.applyNewSettingsNow();
}

/*#################################################################################################
##
##    T A S K S
##
##################################################################################################*/



/*#################################################################################################
##
##    I S R ' S
##
##################################################################################################*/

// ISR: Handel alarm input from servo
void IRAM_ATTR alarmISR() {
  Stroker.motorFault();
}

/*#################################################################################################
##
##    M A I N   P R O G R A M
##
##################################################################################################*/

void setup() 
{
  // Handle WiFi, Provisioning by IoTWebConf, MQTT connection & power in the background
  beginHousework();

  // Register callbacks for all MQTT subscriptions
  mqttSubscribe("/speed", controlSpeed);
  mqttSubscribe("/depth", controlDepth);
  mqttSubscribe("/stroke", controlStroke);
  mqttSubscribe("/sensation", controlSensation);
  mqttSubscribe("/command", receiveCommand);
  mqttSubscribe("/pattern", receivePattern);
  mqttSubscribe("/pwm", receivePWM);

  // Set GPIOs
  pinMode(SERVO_ALARM, INPUT);

  // Set PWM output with 8bit resolution and 5kHz
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PWM, 0);
  ledcWrite(0, 0);

  // Setup interrupt
  attachInterrupt(SERVO_ALARM, alarmISR, FALLING);

  // Wait for MQTT to be available
  while (!mqttConnected()) {
    delay(1000);
  }

  // Wait a little bit for topic subscriptions to complete
  delay(1000);

<<<<<<< Updated upstream
  // Setup Stroke Engine
  Stroker.begin(&strokingMachine, &servoMotor);
  Stroker.enableAndHome(SERVO_ENDSTOP, true, homingNotification);
=======
  ESP_LOGI("main", "Configuring Motor");
  motor.begin(&servoMotor);
  motor.setMaxSpeed(MAX_SPEED); // 2 m/s
  motor.setMaxAcceleration(100000); // 100 m/s^2
  motor.setMachineGeometry(160.0, 5.0);
  motor.setSensoredHoming(SERVO_ENDSTOP, INPUT_PULLUP, false);

  // Setup Stroke Stroker
  Stroker.attachMotor(&motor);
  motor.home(homingNotification);
>>>>>>> Stashed changes

  // Send available patterns as JSON
  mqttPublish("/config", getPatternJSON());
}

void loop() 
{
  // Nothing to do here, it's all FreeRTOS Tasks
  vTaskDelete(NULL);
}

/*#################################################################################################
##
##    O T H E R   F U N C T I O N S
##
##################################################################################################*/

String getPatternJSON() {
    String JSON = "[{\"";
    for (size_t i = 0; i < Stroker.getNumberOfPattern(); i++) {
        JSON += String(Stroker.getPatternName(i));
        JSON += "\": ";
        JSON += String(i, DEC);
        if (i < Stroker.getNumberOfPattern() - 1) {
            JSON += "},{\"";
        } else {
            JSON += "}]";
        }
    }
    Serial.println(JSON);
    return JSON;
}
