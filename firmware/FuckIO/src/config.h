#pragma once

// Pin Definitions
#define SERVO_PULSE       4
#define SERVO_DIR         16
#define SERVO_ENABLE      17
#define SERVO_ENDSTOP     25
#define SERVO_ALARM       26
#define PWM               21
#define STATUS_PIN        22        // IoTWebConf WiFi Status Pin
#define BUTTON_PIN        23        // IoTWebConf Button (for restoring default WiFi AP)

// Housekeeping Settings:
#define CONFIG_VERSION      "fio00.01"      // Version of the configuration
#define THINGNAME           "FuckIO"        // Inital Thing Name, also used as hostname, MQTT topic root and AP name
#define INITIAL_AP_PASSWORD "smrtTHNG32"    // Default password of the AP, must be changed on first start, 
                                            // but can be restored by the WiFi button
#define SERIAL_BAUDS         115200         // Default baud rate for serial communication over USB
#define RSSI_UPDATES         500            // Intervall in ms at which the RSSI value should be send as MQTT message
#define STRING_LEN           64             // Bytes used to initalize char array. No path, topic, name, etc. should exceed this value

