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
#include <config.h>
#include <housekeeping.h>
#include "FastAccelStepper.h"
#include "StrokeEngine.h"

/*#################################################################################################
##
##    G L O B A L    D E F I N I T I O N S   &   D E C L A R A T I O N S
##
##################################################################################################*/

StrokeEngine Stroker;

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
  Stroker.setSpeed(payload.toFloat());
  Stroker.applyNewSettingsNow();
}

void controlDepth(String payload) {
  Serial.println("Depth: " + String(payload));
  Stroker.setDepth(payload.toFloat());
  Stroker.applyNewSettingsNow();
}

void controlStroke(String payload) {
  Serial.println("Stroke: " + String(payload));
  Stroker.setStroke(payload.toFloat());
  Stroker.applyNewSettingsNow();
}

void controlSensation(String payload) {
  Serial.println("Sensation: " + payload);
  Stroker.setSensation(payload.toFloat());
  Stroker.applyNewSettingsNow();
}

void receiveCommand(String payload) {
  Serial.println("Command: " + payload);
  if (payload.equals("start")) {
    Stroker.startMotion();
  }
  if (payload.equals("stop")) {
    Stroker.stopMotion();
  }
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
    Stroker.enableAndHome(homingNotification);
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

// None

/*#################################################################################################
##
##    I S R ' S
##
##################################################################################################*/

// ISR: Handel alarm input from servo
void IRAM_ATTR alarmISR() {
  Stroker.safeState();
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

  // Setup Stroke Engine
  Stroker.begin();
  Stroker.enableAndHome(homingNotification);
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

