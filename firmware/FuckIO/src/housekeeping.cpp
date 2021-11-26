#include <Arduino.h>
#include <housekeeping.h>
#include <LinkedList.h>
#include "config.h"
#include <MQTT.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.
#include <IotWebConfESP32HTTPUpdateServer.h>

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
String ChipId = String((uint32_t)ESP.getEfuseMac(), HEX);
// String thingName = String(THINGNAME) + "-" + ChipId;
String thingName = String(THINGNAME);

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = INITIAL_AP_PASSWORD;

// -- Method declarations.
void handleRoot();
bool connectMqtt();
bool connectMqttOptions();
void doHousework(void * parameter);
void reportRSSI(void * parameter);

// -- Callback methods.
void wifiConnected();
void configSaved();
bool formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper);
void mqttMessageReceived(String &topic, String &payload);

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
WiFiClient net;
MQTTClient mqttClient(128);

char mqttServerValue[STRING_LEN];
char mqttUserNameValue[STRING_LEN];
char mqttUserPasswordValue[STRING_LEN];

IotWebConf iotWebConf(thingName.c_str(), &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
IotWebConfParameterGroup mqttGroup = IotWebConfParameterGroup("mqtt", "MQTT configuration");
IotWebConfTextParameter mqttServerParam = IotWebConfTextParameter("MQTT server", "mqttServer", mqttServerValue, STRING_LEN);
IotWebConfTextParameter mqttUserNameParam = IotWebConfTextParameter("MQTT user", "mqttUser", mqttUserNameValue, STRING_LEN);
IotWebConfPasswordParameter mqttUserPasswordParam = IotWebConfPasswordParameter("MQTT password", "mqttPass", mqttUserPasswordValue, STRING_LEN);

bool needMqttConnect = false;
bool needReset = false;

unsigned long lastMqttConnectionAttempt = 0;

LinkedList<MQTTSubscriptionResponse*> MQTTResponseList = LinkedList<MQTTSubscriptionResponse*>();

char* generateMqttTopic(char *topicSuffix)
{
    static char connectTopic[STRING_LEN];
    strcpy(connectTopic, iotWebConf.getThingName());
    strcat(connectTopic, topicSuffix);
    return connectTopic;
}

void beginHousework()
{

    pinMode(BUTTON_PIN, INPUT_PULLUP); // Shady workaraound for IoTWebConf for ESP32 Issue 124

#if defined(POWER_HOLD)
    pinMode(POWER_HOLD, OUTPUT);
    digitalWrite(POWER_HOLD, LOW);
#endif

    Serial.begin(115200); 
    Serial.println();
    Serial.println("Starting up...");

    mqttGroup.addItem(&mqttServerParam);
    mqttGroup.addItem(&mqttUserNameParam);
    mqttGroup.addItem(&mqttUserPasswordParam);

    iotWebConf.setStatusPin(STATUS_PIN);
    iotWebConf.setConfigPin(BUTTON_PIN); //FIXME Bug in IoTWebConf for ESP32: (Github Issue 124) 
    iotWebConf.skipApStartup();
    iotWebConf.addParameterGroup(&mqttGroup);
    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setFormValidator(&formValidator);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);
    iotWebConf.setupUpdateServer(
        [](const char* updatePath) { httpUpdater.setup(&server, updatePath); },
        [](const char* userName, char* password) { httpUpdater.updateCredentials(userName, password); });

    // -- Initializing the configuration.
    bool validConfig = iotWebConf.init();
    if (!validConfig)
    {
        mqttServerValue[0] = '\0';
        mqttUserNameValue[0] = '\0';
        mqttUserPasswordValue[0] = '\0';
    }

    // -- Set up required URL handlers on the web server.
    server.on("/", handleRoot);
    server.onNotFound([](){ iotWebConf.handleNotFound(); });

    mqttClient.begin(mqttServerValue, net);
    mqttClient.onMessage(mqttMessageReceived);
    
    Serial.println("Ready.");

#if defined(POWER_HOLD)
    //Subscribe to a switch off topic and add to link list 
    mqttSubscribe("/goodbye", goodbye);
#endif

    xTaskCreate(
        doHousework, // Function that should be called
        "Housekeeping task",    // Name of the task (for debugging)
        4096,            // Stack size (bytes)
        NULL,            // Parameter to pass
        10,              // Run task with modest priority
        NULL             // Task handle
    );

      // Create task to report RSSI periodically as MQTT message
    xTaskCreate(
        reportRSSI,      // Function that should be called
        "Report RSSI",   // Name of the task (for debugging)
        2048,            // Stack size (bytes)
        NULL,            // Parameter to pass
        2,               // Task priority
        NULL             // Task handle
    );

}

// Housework Task
void doHousework(void * parameter) {
    while(1) { // infinite loop
        // -- doLoop should be called as frequently as possible.
        iotWebConf.doLoop();
        mqttClient.loop();
        
        if (needMqttConnect)
        {
            if (connectMqtt())
            {
            needMqttConnect = false;
            }
        }
        else if ((iotWebConf.getState() == iotwebconf::OnLine) && (!mqttClient.connected()))
        {
            Serial.println("MQTT reconnect");
            connectMqtt();
        }

        if (needReset)
        {
            Serial.println("Rebooting after 1 second.");
            iotWebConf.delay(1000);
            ESP.restart();
        }   

        // Delay 100ms 
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

bool mqttConnected() {
    // Check wether WiFi is online and MQTT client is connected
    return (iotWebConf.getState() == iotwebconf::OnLine) && (mqttClient.connected());
}

void reportRSSI(void * parameter) {
  while(1) { // infinite loop
    mqttPublish("/rssi", String(WiFi.RSSI()));

    // delay to next RSSI message
    vTaskDelay(RSSI_UPDATES / portTICK_PERIOD_MS);
  }
}

bool connectMqtt() {
    unsigned long now = millis();
    if (1000 > now - lastMqttConnectionAttempt)
    {
        // Do not repeat within 1 sec.
        return false;
    }
    Serial.println("Connecting to MQTT server...");
    if (!connectMqttOptions()) {
        lastMqttConnectionAttempt = now;
        return false;
    }
    Serial.println("Connected!");
   
    // Should we connect subscribe to all topics stored in the linked list
    Serial.println("\nSubscribe to all registered topics");

    // Iterate through complete list
    for (int i = 0; i < MQTTResponseList.size(); i++)
    {
        // Subscribe to its topic
        Serial.print("\nSubscribe to: " + String(MQTTResponseList.get(i)->topic) + " > ");
        if (mqttClient.subscribe(MQTTResponseList.get(i)->topic))
        {
            Serial.println("Success");
        }
        else
        {
            Serial.println("Couldn't subscribe");
        }
    }

    return true;
}

void mqttMessageReceived(String &topic, String &payload)
{
    // Iterate linked list for match and evoke callback
    Serial.println("Message recevied: " + topic + " - " + payload);
    for (int i = 0; i < MQTTResponseList.size(); i++)
    {
        //on match evoke callack
        if (String(MQTTResponseList.get(i)->topic) == topic)
        {
            MQTTResponseList.get(i)->MQTTCallback(payload);
            break;
        }
    }
}

void mqttSubscribe(char* topicSufix, MQTTSubscriptionCallback callback)
{
    //Create new item for link list 
    MQTTSubscriptionResponse *subscribeNew = new MQTTSubscriptionResponse();
    
    //Concatenate topic root & subscription and copy into linked list item
    strcpy(subscribeNew->topic, generateMqttTopic(topicSufix)); 

    //Register callback
    subscribeNew->MQTTCallback = callback;

    //Add topic and callback to linked list
    MQTTResponseList.add(subscribeNew);
}


void mqttPublish(char *topicSufix, char *payload)
{
    mqttClient.publish(generateMqttTopic(topicSufix), payload);
}

void mqttPublish(char *topicSufix, String payload)
{
    mqttClient.publish(generateMqttTopic(topicSufix), payload);
}


// Handle web requests to "/" path.
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  iotWebConf.handleConfig();
}

void wifiConnected()
{
  needMqttConnect = true;
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  needReset = true;
}

bool formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper)
{
  Serial.println("Validating form.");
  bool valid = true;

  int l = webRequestWrapper->arg(mqttServerParam.getId()).length();
  if (l < 3)
  {
    mqttServerParam.errorMessage = "Please provide at least 3 characters!";
    valid = false;
  }

  return valid;
}

bool connectMqttOptions()
{
  bool result;
  if (mqttUserPasswordValue[0] != '\0')
  {
    result = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue, mqttUserPasswordValue);
  }
  else if (mqttUserNameValue[0] != '\0')
  {
    result = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue);
  }
  else
  {
    result = mqttClient.connect(iotWebConf.getThingName());
  }
  return result;
}

void mqttNotify(String note) {
  mqttPublish("/notify", note);
  Serial.println(note);
}
