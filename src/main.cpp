#include <Arduino.h>
#include <ESP32Time.h>     // fbiego/ESP32Time@^2.0.6
#include <PubSubClient.h>  // knolleary/PubSubClient@^2.8
#include <WiFi.h>
#include <time.h>

#include <vector>

#include "/home/ctl/untracked/mqtt-cred.h"
#include "/home/ctl/untracked/wifi-cred.h"
#include "NTP.h"  // sstaub/NTP@^1.6

#define PUMP_CONTROL 14
#define PUMP_CONTROL_2 15

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
  bool pumping = false;
};

struct MqttPacket {
  int32_t tank_pressure = 0;
  int32_t sec_to_next_pump = 0;

  // Increment when data send is requested (some changes do not trigger).
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

// All times in usec since Epoch.
struct SysTime {
  int64_t best_time = 0;  // Our best estimate
  int64_t micros_at_best_time = 0;
  int64_t ntp_time = 0;
  int64_t rtc_at_ntp_time = 0;
};

int64_t RtcMicros(ESP32Time *rtc) {
  return rtc->getEpoch() * 1000000LL + rtc->getMicros();
}

int64_t ConnectNtp(const StateFlags &state_flags, bool &ntp_ok,
                   SysTime &sys_time, ESP32Time *rtc) {
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
    sys_time.ntp_time = ntp.epoch() * 1000000LL;
    sys_time.rtc_at_ntp_time = RtcMicros(rtc);
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
  "tank-pressure": %ld,
  "sec-to-next-pump": %ld 
} 
})",
               packet.version, packet.tank_pressure, packet.sec_to_next_pump);
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
  pinMode(PUMP_CONTROL_2, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  Serial.begin(115200);
}

int64_t UpdateLeds(const StateFlags &state_flags) {
  static bool is_blink = false;
  bool blink = false;
  int led = -1;
  if (state_flags.pumping) {
    led = 2;
  } else if (!state_flags.wifi_ok) {
    led = 0;
    blink = true;
  } else if (!state_flags.ntp_ok || !state_flags.mqtt_ok) {
    led = 0;
  } else {
    led = 1;
  }

  if (blink && !is_blink) {
    led = -1;
  }
  digitalWrite(LED_RED, led == 0);
  digitalWrite(LED_GREEN, led == 1);
  digitalWrite(LED_BLUE, led == 2);

  is_blink = !is_blink;
  return 250L;
}

int64_t ReadTankPressure(MqttPacket &packet) {
  packet.tank_pressure = analogRead(TANK_PRESSURE);
  packet.version++;
  Serial.printf("READ pressure %ld @ %lld\n", packet.tank_pressure,
                packet.version);

  return 2000;
}

int64_t TimeKeeper(const StateFlags &state_flags, SysTime &sys_time,
                   ESP32Time *rtc) {
  constexpr int64_t kMaxAdjustRateUsPerS = 100000LL;  // 10 % = 100000
  static int initial_loops = 4;
  static int64_t initial_rtc_offset = 0;
  static int64_t rtc_offset = 0;
  static int64_t previous_offset;
  static int64_t previous_offset_calc_time_rtc;

  int64_t rtc_time = RtcMicros(rtc);
  sys_time.best_time = rtc_time + rtc_offset;
  sys_time.micros_at_best_time = micros();

  if (!state_flags.ntp_ok) {
    return 1000;  // No ntp, no can adjust.
  }

  // Update rtc offset using NTP
  int64_t new_offset = sys_time.ntp_time - sys_time.rtc_at_ntp_time;
  const int64_t rtc_now = RtcMicros(rtc);
  if (initial_rtc_offset == 0LL || initial_loops-- > 0) {
    rtc_offset = previous_offset = initial_rtc_offset = new_offset;
    previous_offset_calc_time_rtc = rtc_now;
    Serial.printf("TIME: Initial offset is %.6f (%lld)\n",
                  initial_rtc_offset / 1e6L, initial_rtc_offset);
  } else {
    // int64 (1e18) has enough range here for 1e3s * 1e6 us/s * 1e6 adjust
    int64_t max_adjust =
        ((rtc_now - previous_offset_calc_time_rtc) * kMaxAdjustRateUsPerS) /
        1000000LL;
    new_offset = std::min(std::max(new_offset, rtc_offset - max_adjust),
                          rtc_offset + max_adjust);
    previous_offset = rtc_offset;
    previous_offset_calc_time_rtc = rtc_now;
    rtc_offset = new_offset;
    Serial.printf("TIME: Adjusted offset initial+ %.6f (%lld)\n",
                  (rtc_offset - initial_rtc_offset) / 1e6L, rtc_offset);
  }

  return 10000;  // ZZZ more in final.
}

constexpr int HMS(int h, int m, int s) {
  return ((h + 24) % 24) * 3600 + ((m + 60) % 60) * 60 + ((s + 60) % 60);
};

int64_t BestMicros(const SysTime &sys_time) {
  return micros() - sys_time.micros_at_best_time + sys_time.best_time;
}

int64_t PumpControl(bool &state_pumping, const SysTime &sys_time,
                    MqttPacket &mqtt_packet) {
  struct ActiveInterval {
    int start_sec;
    int end_sec;
  };

  // ZZZZZ
  const int h = 9 - 3;
  const int m = 40;
  static const std::vector<ActiveInterval> schedule{
      {HMS(21 - 3, 0, 0), HMS(21 - 3, 1, 0)},
      {HMS(8 - 3, 0, 0), HMS(8 - 3, 1, 0)},

      /// ZZZZZ
      {HMS(h, m + 1, 0), HMS(h, m + 1, 10)},
      {HMS(h, m + 2, 0), HMS(h, m + 2, 10)},
      {HMS(h, m + 3, 0), HMS(h, m + 3, 10)},
      {HMS(h, m + 4, 0), HMS(h, m + 4, 10)},
      {HMS(h, m + 5, 0), HMS(h, m + 5, 10)},
      // {HMS(h, m + 6, 0), HMS(h, m + 6, 10)},
      // {HMS(h, m + 7, 0), HMS(h, m + 7, 10)},
      // {HMS(h, m + 8, 0), HMS(h, m + 8, 10)},
      // {HMS(h, m + 9, 0), HMS(h, m + 9, 10)},

  };

  time_t best_time_s = BestMicros(sys_time) / 1000000LL;
  tm *dt = gmtime(&best_time_s);

  int time_of_day_s = HMS(dt->tm_hour, dt->tm_min, dt->tm_sec);

  int min_next_s = 100000;
  bool active = false;
  for (const ActiveInterval &i : schedule) {
    // Are we in the interval with ascending start to end ?
    bool in_asc_interval = std::min(i.start_sec, i.end_sec) <= time_of_day_s &&
                           time_of_day_s < std::max(i.start_sec, i.end_sec);
    // Active if in interval, or complement if start < end (interval crosses
    // 24h).
    active |= (i.start_sec <= i.end_sec ? in_asc_interval : !in_asc_interval);
    // Serial.printf("PUMP: Check %d < %d < %d in int %d act = %d ?\n",
    //               i.start_sec, time_of_day_s, i.end_sec,
    //               (int)in_asc_interval, (int)active);
    int start_dist_s = (i.start_sec - time_of_day_s + 86400) % 86400;
    min_next_s = std::min(min_next_s, start_dist_s);
  }

  digitalWrite(PUMP_CONTROL, active);
  digitalWrite(PUMP_CONTROL_2, active);
  state_pumping = active;

  mqtt_packet.sec_to_next_pump = active ? 0 : min_next_s;

  return 500;
}

int64_t UpdateSerial(const SysTime &sys_time, ESP32Time *rtc) {
  // Serial.print(rtc->getTime("SERIAL: RTC=%Y-%m-%d %H:%M:%S"));
  // Serial.printf(".%03d\n", rtc->getMillis());

  Serial.printf("SERIAL: SysTime{best=%lld,ntp=%lld,rtc@ntp=%lld}\n",
                sys_time.best_time, sys_time.ntp_time,
                sys_time.rtc_at_ntp_time);

  time_t best_epoch = sys_time.best_time / 1000000LL;
  tm *dt = gmtime(&best_epoch);
  Serial.printf("SERIAL: BestTime %04ld-%02ld-%02ld %02ld:%02ld:%02ld.%06lld\n",
                dt->tm_year + 1900L, dt->tm_mon + 1L, dt->tm_mday, dt->tm_hour,
                dt->tm_min, dt->tm_sec, sys_time.best_time % 1000000LL);

  return 5000L;
}

void loop() {
  static StateFlags state_flags;
  static SysTime sys_time;
  static struct {
    int64_t wifi = 0;
    int64_t leds = 0;
    int64_t ntp = 0;
    int64_t timekeeper = 0;
    int64_t serial = 0;
    int64_t mqtt = 0;
    int64_t read_pressure = 0;
    int64_t pump_control = 0;
  } next_calls_ms;

  static ESP32Time rtc;
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

  dispatch(next_calls_ms.leds, std::bind(UpdateLeds, std::cref(state_flags)));
  dispatch(next_calls_ms.wifi,
           std::bind(ConnectWifi, std::ref(state_flags.wifi_ok)));
  dispatch(next_calls_ms.ntp,
           std::bind(ConnectNtp, std::cref(state_flags),
                     std::ref(state_flags.ntp_ok), std::ref(sys_time), &rtc));
  dispatch(
      next_calls_ms.timekeeper,
      std::bind(TimeKeeper, std::cref(state_flags), std::ref(sys_time), &rtc));
  dispatch(next_calls_ms.mqtt,
           std::bind(UpdateMqtt, std::cref(state_flags),
                     std::ref(state_flags.mqtt_ok), std::cref(mqtt_packet)));
  dispatch(next_calls_ms.read_pressure,
           std::bind(ReadTankPressure, std::ref(mqtt_packet)));
  dispatch(next_calls_ms.pump_control,
           std::bind(PumpControl, std::ref(state_flags.pumping),
                     std::cref(sys_time), std::ref(mqtt_packet)));

  dispatch(next_calls_ms.serial,
           std::bind(UpdateSerial, std::cref(sys_time), &rtc));

  epoch_ms = rtc.getEpoch() * 1000L + rtc.getMillis();
  delay(
      std::max(MIN_SLEEP_MS, std::min(next_epoch_ms - epoch_ms, MAX_SLEEP_MS)));
}
