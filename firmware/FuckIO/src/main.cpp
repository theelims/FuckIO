/**
 *   FuckIO Firmware for ESP32
 *   Use MQTT messages to control all built-in motion patterns
 *   https://github.com/theelims/FuckIO 
 *
 * Copyright (C) 2022 theelims <elims@gmx.net>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <Arduino.h>
#include "esp_log.h"
#include <config.h>
#include <housekeeping.h>
#include "motor/genericStepper.h"
#include <StrokeEngine.h>

/*#################################################################################################
##
##    G L O B A L    D E F I N I T I O N S   &   D E C L A R A T I O N S
##
##################################################################################################*/

// Calculation Aid:
#define STEP_PER_REV      2000      // How many steps per revolution of the motor (S1 off, S2 on, S3 on, S4 off)
#define PULLEY_TEETH      20        // How many teeth has the pulley
#define BELT_PITCH        2         // What is the timing belt pitch in mm
#define MAX_RPM           3000.0    // Maximum RPM of motor
#define STEP_PER_MM       STEP_PER_REV / (PULLEY_TEETH * BELT_PITCH)
#define MAX_SPEED         (MAX_RPM / 60.0) * PULLEY_TEETH * BELT_PITCH

static motorProperties servoMotor {      
  .stepsPerMillimeter = STEP_PER_MM,   
  .invertDirection = true,      
  .enableActiveLow = true,      
  .stepPin = SERVO_PULSE,              
  .directionPin = SERVO_DIR,          
  .enablePin = SERVO_ENABLE              
}; 


GenericStepperMotor motor;
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
  Stroker.setParameter(StrokeParameter::RATE, payload.toFloat(), true);
}

void controlDepth(String payload) {
  Serial.println("Depth: " + String(payload));
  Stroker.setParameter(StrokeParameter::DEPTH, payload.toFloat(), true);
}

void controlStroke(String payload) {
  Serial.println("Stroke: " + String(payload));
  Stroker.setParameter(StrokeParameter::STROKE, payload.toFloat(), true);
}

void controlSensation(String payload) {
  Serial.println("Sensation: " + payload);
  Stroker.setParameter(StrokeParameter::SENSATION, payload.toFloat(), true);
}

void receiveCommand(String payload) {
  Serial.println("Command: " + payload);
  if (payload.equals("start")) {
    Stroker.startPattern();
  }
  if (payload.equals("stop")) {
    Stroker.stopMotion();
  }
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
  Stroker.setPattern(payload.toInt(), true);
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

// None

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

  // Set PWM output with 8bit resolution and 5kHz
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PWM, 0);
  ledcWrite(0, 0);

  // Wait for MQTT to be available
  while (!mqttConnected()) {
    delay(1000);
  }

  // Wait a little bit for topic subscriptions to complete
  delay(1000);

  ESP_LOGI("main", "Configuring Motor");
  motor.begin(&servoMotor);
  motor.setMaxSpeed(MAX_SPEED); // 2 m/s
  motor.setMaxAcceleration(100000); // 100 m/s^2
  motor.setMachineGeometry(160.0, 5.0);
  motor.setSensoredHoming(SERVO_ENDSTOP, INPUT_PULLUP, false);

  // Setup Stroke Stroker
  Stroker.attachMotor(&motor);
  motor.home(homingNotification);

  // Send available patterns as JSON
  mqttPublish("/config", getPatternJSON());
}

void loop() 
{
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
