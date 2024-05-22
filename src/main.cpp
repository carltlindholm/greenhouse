#include <Arduino.h>
#include <ESP32Time.h>     // fbiego/ESP32Time@^2.0.6
#include <PubSubClient.h>  // knolleary/PubSubClient@^2.8
#include <WiFi.h>

#include "/home/ctl/untracked/mqtt-cred.h"
#include "/home/ctl/untracked/wifi-cred.h"
#include "NTP.h"  // sstaub/NTP@^1.6

#define PUMP_CONTROL 14
#define TANK_PRESSURE 33
#define LED_RED 16
#define LED_GREEN 17
#define LED_BLUE 18

constexpr int64_t MAX_SLEEP_MS = 10000;
constexpr int64_t MIN_SLEEP_MS = 50;
// When calculating the next event, we allow the current call to be at most this
// much in the past. This cutoff is necessary to prevent situations where some
// events play catchup with realtime forever (next event always going into the
// past despite max update rate).
constexpr int64_t MAX_BACKLOG_MS = 100;

struct StateFlags {
  bool wifi_ok = false;
  bool ntp_ok = false;
  bool mqtt_ok = false;
};

struct MqttPacket {
  int32_t tank_pressure = 0;

  // Increment when data updated.
  int64_t version = 0;
};

int64_t ConnectWifi(bool &wifi_ok) {
  static const char ssid[] = SSID;
  static const char password[] = SSID_CRED;
  static bool connect_announce = true;
  wifi_ok = WiFi.status() == WL_CONNECTED;
  if (wifi_ok) {
    if (connect_announce) {
      Serial.println("WIFI Connected :)");
    } else {
      Serial.println("WIFI ok");
    }
    connect_announce = false;
    return 10000;  // Poll every 10s
  }

  connect_announce = true;
  Serial.printf("Try connecting to %s MAC %s state %d\n", ssid,
                WiFi.macAddress().c_str(), WiFi.status());

  WiFi.disconnect();
  WiFi.begin(ssid, password);

  return 3000;  // Retry time
}

int64_t ConnectNtp(const StateFlags &state_flags, bool &ntp_ok) {
  static WiFiUDP wifiUdp;
  static NTP ntp(wifiUdp);
  if (state_flags.wifi_ok && !ntp_ok) {
    Serial.println("NTP connect attempt ");
    ntp.begin();
    ntp_ok = true;
    return 1000;
  } else if (!state_flags.wifi_ok) {
    Serial.println("NTP no wifi ");
    ntp.stop();
    ntp_ok = false;
    return 1000;
  } else {
    ntp.update();
    Serial.print("NTP: ");
    Serial.println(ntp.formattedTime("%A %T"));
    return 20000;
  }
}

int64_t UpdateMqtt(const StateFlags &state_flags, bool &mqtt_ok,
                   const MqttPacket &packet) {
  static const char brokerAddress[] =
      "broker.losant.com";  // "146.148.110.247";
  static const uint16_t brokerPort = 1883;
  static const char devId[] = MQTT_DEV_ID_DEV;
  static const char devUser[] = MQTT_DEV_USER_2;
  static const char devPass[] = MQTT_DEV_PASS_2;
  static bool connect_announce = true;
  static bool connecting = false;
  static int64_t sent_version = 0;

  static WiFiClient wifiClient;
  static PubSubClient client(brokerAddress, 1883, wifiClient);
  static char message[1024];  // TODO: send buffer in client is 256

  if (packet.version <= sent_version) {
    // No new data yet.
    return 100;
  }

  if (state_flags.wifi_ok && !mqtt_ok && !connecting) {
    Serial.printf("MQTT connect attempt for packet %lld -> %lld\n",
                  sent_version, packet.version);
    client.connect(devId, devUser, devPass);
    connecting = true;
    return 100;
  } else if (state_flags.wifi_ok && !mqtt_ok && connecting) {
    bool old_mqtt_ok = mqtt_ok;
    mqtt_ok = client.connected();
    connecting = !mqtt_ok;
    if (!old_mqtt_ok && mqtt_ok) {
      Serial.printf("MQTT connected\n", packet.version);
    }
    return 500;
  } else if (state_flags.wifi_ok && mqtt_ok) {
    bool old_mqtt_ok = mqtt_ok;
    mqtt_ok = client.connected();
    if (mqtt_ok) {
      Serial.printf("MQTT sending %lld\n", packet.version);
      snprintf(message, sizeof(message),
               R"({ 
            "data": { 
              "ping-count": %lld,
              "tank-pressure": %ld 
            } 
          })",
               packet.version, packet.tank_pressure);
      client.publish("losant/" MQTT_DEV_ID_DEV "/state", message);
      sent_version = packet.version;
    }
    if (!old_mqtt_ok && mqtt_ok) {
      Serial.println("MQTT disconnected");
    }
    return 1000;
  } else if (!state_flags.wifi_ok) {
    client.disconnect();
    mqtt_ok = false;
    connecting = false;
    Serial.println("MQTT no wifi");
    return 1000;
  }

  Serial.printf("MQTT bad state %b,%b,%b\n", state_flags.wifi_ok, mqtt_ok,
                connecting);
  return 1000;
}

void setup() {
  pinMode(PUMP_CONTROL, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  Serial.begin(115200);
}

int64_t UpdateLeds() {
  static int counter = 0;

  digitalWrite(LED_RED, counter % 3 == 0);
  digitalWrite(LED_GREEN, counter % 3 == 1);
  digitalWrite(LED_BLUE, counter % 3 == 2);

  ++counter;
  return 250L;
}

int64_t ReadTankPressure(MqttPacket &packet) {
  packet.tank_pressure = analogRead(TANK_PRESSURE);
  packet.version++;
  Serial.printf("READ pressure %ld @ %lld\n", packet.tank_pressure,
                packet.version);

  return 2000;
}

int64_t UpdateSerial(ESP32Time *rtc) {
  Serial.print(rtc->getTime("%Y-%m-%d %H:%M:%S"));
  Serial.printf(".%03d\n", rtc->getMillis());
  return 1000L;
}

void loop() {
  static StateFlags state_flags;
  static struct {
    int64_t wifi = 0;
    int64_t leds = 0;
    int64_t ntp = 0;
    int64_t serial = 0;
    int64_t mqtt = 0;
    int64_t read_pressure = 0;
  } next_calls_ms;

  static ESP32Time rtc(2 * 3600);  // UTC+2
  static MqttPacket mqtt_packet;

  int64_t epoch_ms = rtc.getEpoch() * 1000L + rtc.getMillis();
  int64_t next_epoch_ms = epoch_ms + MAX_SLEEP_MS;

  auto dispatch = [&](int64_t &next_ms, std::function<int64_t()> fn) {
    next_ms = std::max(next_ms, epoch_ms - MAX_BACKLOG_MS);
    if (epoch_ms > next_ms) {
      next_ms += fn();
    }
    next_epoch_ms = std::min(next_ms, next_epoch_ms);
  };

  dispatch(next_calls_ms.leds, UpdateLeds);
  dispatch(next_calls_ms.wifi,
           std::bind(ConnectWifi, std::ref(state_flags.wifi_ok)));
  dispatch(next_calls_ms.ntp, std::bind(ConnectNtp, std::cref(state_flags),
                                        std::ref(state_flags.ntp_ok)));
  dispatch(next_calls_ms.mqtt,
           std::bind(UpdateMqtt, std::cref(state_flags),
                     std::ref(state_flags.mqtt_ok), std::cref(mqtt_packet)));
  dispatch(next_calls_ms.read_pressure,
           std::bind(ReadTankPressure, std::ref(mqtt_packet)));
  dispatch(next_calls_ms.serial, std::bind(UpdateSerial, &rtc));

  epoch_ms = rtc.getEpoch() * 1000L + rtc.getMillis();
  delay(
      std::max(MIN_SLEEP_MS, std::min(next_epoch_ms - epoch_ms, MAX_SLEEP_MS)));
}
