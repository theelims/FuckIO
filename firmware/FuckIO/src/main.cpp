/**
 *   FuckIO Firmware for OSSM Ref Board V2
 *   Use MQTT messages to control all built-in motion patterns
 *   https://github.com/theelims/FuckIO
 *
 * Copyright (C) 2023 theelims <elims@gmx.net>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#define VIRTUAL

#include <Arduino.h>
#include "esp_log.h"
#include <config.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
// #include <ArduinoJson.h>
// #include <ArduinoWebsockets.h>
#include <NeoPixelBus.h>
#include <StrokeEngine.h>
#ifdef VIRTUAL
#include <motor/virtualMotor.h>
#else
#include <motor/genericStepper.h>
#endif
#include <LittleFS.h>

#define SPIFFS LITTLEFS

/*#################################################################################################
##
##    G L O B A L    D E F I N I T I O N S   &   D E C L A R A T I O N S
##
##################################################################################################*/

// StrokeEngine ###################################################################################
// Calculation Aid:
#define STEP_PER_REV 2000 // How many steps per revolution of the motor (S1 off, S2 on, S3 on, S4 off)
#define PULLEY_TEETH 20   // How many teeth has the pulley
#define BELT_PITCH 2      // What is the timing belt pitch in mm
#define MAX_RPM 3000.0    // Maximum RPM of motor
#define STEP_PER_MM STEP_PER_REV / (PULLEY_TEETH * BELT_PITCH)
#define MAX_SPEED (MAX_RPM / 60.0) * PULLEY_TEETH *BELT_PITCH

#ifndef VIRTUAL
static motorProperties servoMotor{
    .stepsPerMillimeter = STEP_PER_MM,
    .invertDirection = true,
    .enableActiveLow = true,
    .stepPin = OSSM[0].pin.servoPul,
    .directionPin = OSSM[0].pin.servoDir,
    .enablePin = OSSM[0].pin.servoEna};
#endif

#ifdef VIRTUAL
VirtualMotor motor;
#else
GenericStepperMotor motor;
#endif

StrokeEngine Stroker;

// NeoPixel #######################################################################################

#define colorSaturation 128

NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod> status(1, OSSM[0].pin.rgb); ////WS2813

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor pink(colorSaturation, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// WebSocket Server ###############################################################################

AsyncWebServer server(80);
AsyncWebSocket ws("/ws/api");

/*#################################################################################################
##
##    C A L L B A C K S
##
##################################################################################################*/

void printSpeedPositionOnSerial(unsigned int time, float position, float speed)
{
    String json = "{\"time\": ";
    json += String(time);
    json += ", \"position\": ";
    json += String(position, 2);
    json += ", \"speed\": ";
    json += String(speed, 2);
    json += "}";
    ws.textAll(json);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        // client connected
        Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
        client->printf("Hello Client %u :)", client->id());
        client->ping();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        // client disconnected
        Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    }
    else if (type == WS_EVT_ERROR)
    {
        // error was received from the other end
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    }
    else if (type == WS_EVT_PONG)
    {
        // pong message was received (in response to a ping request maybe)
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    }
    else if (type == WS_EVT_DATA)
    {
        // data packet
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len)
        {
            // the whole message is in a single frame and we got all of it's data
            Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
            if (info->opcode == WS_TEXT)
            {
                data[len] = 0;
                Serial.printf("%s\n", (char *)data);
            }
            else
            {
                for (size_t i = 0; i < info->len; i++)
                {
                    Serial.printf("%02x ", data[i]);
                }
                Serial.printf("\n");
            }
            if (info->opcode == WS_TEXT)
                client->text("I got your text message");
            else
                client->binary("I got your binary message");
        }
        else
        {
            // message is comprised of multiple frames or the frame is split into multiple packets
            if (info->index == 0)
            {
                if (info->num == 0)
                    Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
            }

            Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
            if (info->message_opcode == WS_TEXT)
            {
                data[len] = 0;
                Serial.printf("%s\n", (char *)data);
            }
            else
            {
                for (size_t i = 0; i < len; i++)
                {
                    Serial.printf("%02x ", data[i]);
                }
                Serial.printf("\n");
            }

            if ((info->index + len) == info->len)
            {
                Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
                if (info->final)
                {
                    Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                    if (info->message_opcode == WS_TEXT)
                        client->text("I got your text message");
                    else
                        client->binary("I got your binary message");
                }
            }
        }
    }
}

// void homingNotification(bool isHomed) {
//   if (isHomed) {
//     mqttNotify("Found home - Ready to rumble!");
//   } else {
//     mqttNotify("Homing failed!");
//   }
// }

// void controlSpeed(String payload) {
//   Serial.println("Speed: " + String(payload));
//   Stroker.setParameter(StrokeParameter::RATE, payload.toFloat(), true);
// }

// void controlDepth(String payload) {
//   Serial.println("Depth: " + String(payload));
//   Stroker.setParameter(StrokeParameter::DEPTH, payload.toFloat(), true);
// }

// void controlStroke(String payload) {
//   Serial.println("Stroke: " + String(payload));
//   Stroker.setParameter(StrokeParameter::STROKE, payload.toFloat(), true);
// }

// void controlSensation(String payload) {
//   Serial.println("Sensation: " + payload);
//   Stroker.setParameter(StrokeParameter::SENSATION, payload.toFloat(), true);
// }

// void receiveCommand(String payload) {
//   Serial.println("Command: " + payload);
//   if (payload.equals("start")) {
//     Stroker.startPattern();
//   }
//   if (payload.equals("stop")) {
//     Stroker.stopMotion();
//   }
//   if (payload.equals("home")) {
//     //motor.home(homingNotification);
//     motor.home();
//   }
//   if (payload.equals("patternlist")) {
//     mqttPublish("/config", getPatternJSON());
//   }
// }

// void receivePattern(String payload) {
//   Serial.println("Pattern Index: " + String(payload));
//   Stroker.setPattern(payload.toInt(), true);
// }

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

// None

/*#################################################################################################
##
##    F U N C T I O N S
##
##################################################################################################*/

/**
 * Wait for WiFi connection, and, if not connected, reboot
 */
void waitForWiFiConnectOrReboot(bool printOnSerial = true)
{
    uint32_t notConnectedCounter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        if (printOnSerial)
        {
            Serial.println("Wifi connecting...");
        }
        notConnectedCounter++;
        if (notConnectedCounter > 50)
        { // Reset board if not connected after 5s
            if (printOnSerial)
            {
                Serial.println("Resetting due to Wifi not connecting...");
            }
            ESP.restart();
        }
    }
    if (printOnSerial)
    {
        // Print wifi IP addess
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

String getPatternJSON()
{
    String JSON = "[{\"";
    for (size_t i = 0; i < Stroker.getNumberOfPattern(); i++)
    {
        JSON += String(Stroker.getPatternName(i));
        JSON += "\": ";
        JSON += String(i, DEC);
        if (i < Stroker.getNumberOfPattern() - 1)
        {
            JSON += "},{\"";
        }
        else
        {
            JSON += "}]";
        }
    }
    Serial.println(JSON);
    return JSON;
}

/*#################################################################################################
##
##    M A I N   P R O G R A M
##
##################################################################################################*/

void setup()
{

    // Configure recovery pin as input
    pinMode(OSSM[0].pin.sw, INPUT_PULLDOWN);

    // start Neopixel light show
    status.Begin();

    // start serial output
    Serial.begin(115200);
    while (!Serial)
        ; // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    status.SetPixelColor(0, red);
    status.Show();

    // Initialize LittleFS and halt if not flashed
    if (!LittleFS.begin(false))
    {
        ESP_LOGE("FuckIO", "Failed to mount file system! Please upload filesystem image.");
        while (true)
            ;
    }

    // Handle WiFi, Provisioning by IoTWebConf, MQTT connection & power in the background
    ESP_LOGI("FuckIO", "Start WiFi");
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname("fuckio");
    WiFi.begin("Kellerkind", "Putzplan");
    waitForWiFiConnectOrReboot();
    status.SetPixelColor(0, green);
    status.Show();

    Serial.println();
    Serial.println("Running...");

    // Register callbacks for all MQTT subscriptions
    // ESP_LOGI("FuckIO", "Register MQTT subscriptions");
    // mqttSubscribe("/speed", controlSpeed);
    // mqttSubscribe("/depth", controlDepth);
    // mqttSubscribe("/stroke", controlStroke);
    // mqttSubscribe("/sensation", controlSensation);
    // mqttSubscribe("/command", receiveCommand);
    // mqttSubscribe("/pattern", receivePattern);

    ESP_LOGI("FuckIO", "WebSocket Server available on port 80");
    // Start webserver
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.begin();

    // Wait a little bit for topic subscriptions to complete
    delay(1000);

    ESP_LOGI("FuckIO", "Configuring Motor");

#ifdef VIRTUAL
    motor.begin(printSpeedPositionOnSerial, 50);
#else
    motor.begin(&servoMotor);
    // motor.attachPositionFeedback(printSpeedPositionOnSerial, 50);
    motor.setSensoredHoming(OSSM[0].pin.lmt1, INPUT_PULLUP, true);
#endif
    motor.setMaxSpeed(MAX_SPEED);     // 2 m/s
    motor.setMaxAcceleration(100000); // 100 m/s^2
    motor.setMachineGeometry(160.0, 5.0);

    Stroker.attachMotor(&motor);
    motor.enable();
    motor.home();
    // motor.home(homingNotification);

    // Send available patterns as JSON
    // mqttPublish("/config", getPatternJSON());

    // Wait a little bit
    delay(1000);

    Stroker.setParameter(StrokeParameter::SENSATION, 30.0, true);
    Stroker.startPattern();
}

void loop()
{
    ws.cleanupClients(); // <-- add this line
    delay(100);
}
