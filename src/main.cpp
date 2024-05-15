#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>  // knolleary/PubSubClient@^2.8
#include <ESP32Time.h> // fbiego/ESP32Time@^2.0.6
#include "/home/ctl/untracked/wifi-cred.h"
#include "/home/ctl/untracked/mqtt-cred.h"

#define PUMP_CONTROL 14
#define TANK_PRESSURE 33
#define LED_RED 16
#define LED_GREEN 17
#define LED_BLUE 18

constexpr int64_t MAX_SLEEP_MS=10000;
constexpr int64_t MIN_SLEEP_MS=50;
// When calculating the next event, we allow the current call to be at most this much
// in the past. This cutoff is necessary to prevent situations where some events play
// catchup with realtime forever (next event always going into the past desite max
// update rate).
constexpr int64_t MAX_BACKLOG_MS=100;

void setupWifi() {
  static const char ssid[] = SSID;
  static const char password[] = SSID_CRED;
  //IPAddress dns(8, 8, 8, 8);  //Google dns 
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  Serial.print("MAC address ");
  Serial.println(WiFi.macAddress());
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
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  Serial.begin(115200);
  setupWifi();
}

int64_t UpdateLeds() {
  static int counter = 0;

  digitalWrite(LED_RED, counter%3 == 0);
  digitalWrite(LED_GREEN, counter%3 == 1);
  digitalWrite(LED_BLUE, counter%3 == 2);

  ++counter;
  return 250L;
}

int64_t UpdateMisc() {
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
  digitalWrite(PUMP_CONTROL, counter < 4300 && counter%4==1); 
  Serial.printf("State: pump %d pressure %d\n", counter%2, tankPressure);

  return 2000L;
}

int64_t UpdateSerial(ESP32Time *rtc) {
  Serial.print(rtc->getTime("%Y-%m-%d %H:%M:%S"));
  Serial.printf(".%03d\n", rtc->getMillis());
  return 1000L;
}

void loop() {
  static struct {
     int64_t leds = 0;
     int64_t serial = 0;
     int64_t misc = 0; 
  } next_calls_ms;

  static ESP32Time rtc(2 * 3600); // UTC+2

  int64_t epoch_ms = rtc.getEpoch() * 1000L + rtc.getMillis();
  int64_t next_epoch_ms = epoch_ms + MAX_SLEEP_MS;

  auto dispatch = [&] (int64_t &next_ms, std::function<int64_t()> fn) {
    next_ms = std::max(next_ms, epoch_ms - MAX_BACKLOG_MS);
    if (epoch_ms > next_ms) {      
      next_ms += fn();
    }
    next_epoch_ms = std::min(next_ms, next_epoch_ms);
  };

  dispatch(next_calls_ms.leds, UpdateLeds);
  dispatch(next_calls_ms.misc, UpdateMisc);
  dispatch(next_calls_ms.serial, std::bind(UpdateSerial, &rtc));

  epoch_ms = rtc.getEpoch() * 1000L + rtc.getMillis();  
  delay(std::max(MIN_SLEEP_MS, std::min(next_epoch_ms - epoch_ms, MAX_SLEEP_MS)));
}
