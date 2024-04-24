#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "/home/ctl/untracked/wifi-cred.h"
#include "/home/ctl/untracked/mqtt-cred.h"

#define PUMP_CONTROL 14
#define TANK_PRESSURE 33

void setupWifi() {
  static const char ssid[] = SSID;
  static const char password[] = SSID_CRED;
  //IPAddress dns(8, 8, 8, 8);  //Google dns 
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nConnected :) ");
  delay(200);  // Maybe helps.  
}

PubSubClient * getMqttClient() {
  static const char brokerAddress[] = "146.148.110.247"; // "broker.losant.com";
  static const uint16_t brokerPort = 1883;
  static const char devId[] = MQTT_DEV_ID;
  static const char devUser[] = MQTT_DEV_USER;
  static const char devPass[] = MQTT_DEV_PASS;

  static WiFiClient wifiClient;
  static PubSubClient client(brokerAddress, 1883, wifiClient);

  if (!client.connected()) {
    Serial.print("Connecting MQTT ");
    while (!client.connected()) {
      Serial.print(",");
      client.connect(devId, devUser, devPass);
      delay(200);
    }        
    Serial.println("\nMQTT Connected");
  }
  return &client;
}

void setup() {
  pinMode(PUMP_CONTROL, OUTPUT);

  Serial.begin(115200);
  setupWifi();
}


void loop() {
  static int counter = 4200;
  char message[1024];
  PubSubClient * mqtt = getMqttClient();
  int tankPressure = analogRead(TANK_PRESSURE);
  snprintf(message, sizeof(message), 
  R"({ 
      "data": { 
        "ping-count": %d,
        "tank-pressure": %d 
      } 
    })", counter++, tankPressure);
  mqtt->publish("losant/" MQTT_DEV_ID "/state", message);
  //digitalWrite(PUMP_CONTROL, counter%2); 
  Serial.printf("State: pump %d pressure %d\n", counter%2, tankPressure);
  delay(2000);
}