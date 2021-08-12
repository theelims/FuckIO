#pragma once

#include <Arduino.h>
#include "config.h"


using MQTTSubscriptionCallback = void (*)(String); //payload

class MQTTSubscriptionResponse {
  public:
    char topic[STRING_LEN];                    
    MQTTSubscriptionCallback MQTTCallback;
};

void beginHousework();

bool mqttConnected();

void mqttPublish(char *, char *);
void mqttPublish(char *, String);

void mqttSubscribe(char *, MQTTSubscriptionCallback);
void mqttNotify(String);

