#include <Arduino.h>
#include <ESP32Time.h>           // fbiego/ESP32Time@^2.0.6
#include <PubSubClient.h>        // knolleary/PubSubClient@^2.8
#include <SimpleKalmanFilter.h>  //  denyssene/SimpleKalmanFilter@^0.1.0
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <time.h>
#include <vector>

#include "setup_ui.h"
#include "config.h"

#include "NTP.h"  // sstaub/NTP@^1.6

#define PUMP_CONTROL 14
#define PUMP_CONTROL_2 15

#define TANK_PRESSURE 33
#define LED_RED 16
#define LED_GREEN 17
#define LED_BLUE 18

#define SETUP_MODE_PIN 23  // Pull low on startup to enter setup.

// Successful MQTT is our watchdog keepalive signal. This also means
// if our MQTT broker is down we'll reboot loop, so with that in mind the
// watchdog timeout should be large enough that we'd still do something
// reasonable in this case (1*plant watering at least).
#define WATCHDOG_TIMEOUT_S (11 * 3600 + 3117)
#define MQTT_KEEPALIVE_SEC 47
#define MQTT_UPDATE_USEC 10000LL
#define MQTT_DO_PUBLISH 1

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
  int64_t rtc_offset_post_init = 0;

  // Increment when data send is requested (some changes do not trigger).
  int64_t version = 0;
};

void WatchdogStart(int reset_timeout_s) {
  // Configure to exevute panic = restart on timeout.
  esp_task_wdt_init(reset_timeout_s, /*panic=*/true);
  esp_task_wdt_add(nullptr);  // nullpr = this task.
}

void WatchdogImAlive() { esp_task_wdt_reset(); }

int64_t ConnectWifi(const Config::Wifi &wifi_config, bool &wifi_ok) {
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
  Serial.printf("Try connecting to %s MAC %s state %d\n", wifi_config.ssid,
                WiFi.macAddress().c_str(), WiFi.status());

  WiFi.disconnect();
  WiFi.begin(wifi_config.ssid, wifi_config.password);

  return 3000;  // Retry time
}

// All times in usec since Epoch.
struct SysTime {
  int64_t best_time = 0;  // Our best estimate
  uint32_t micros_at_best_time =
      0;  // Be careful with unisgned and rollover (every ~70min)!
  int64_t ntp_time = 0;
  int64_t rtc_at_ntp_time = 0;
};

int64_t RtcMicros(ESP32Time *rtc) {
  return rtc->getEpoch() * 1000000LL + rtc->getMicros();
}

int64_t BestMicros(const SysTime &sys_time) {
  // Unsigned dragons here: first take the uint32_t difference, which will
  // work even when micros wrapped, then the addition casts up to int64_t
  return (micros() - sys_time.micros_at_best_time) + sys_time.best_time;
}

int64_t ConnectNtp(const Config::Ntp &ntp_config, const StateFlags &state_flags,
                   bool &ntp_ok, SysTime &sys_time, ESP32Time *rtc) {
  static WiFiUDP wifiUdp;
  static NTP ntp(wifiUdp);
  if (state_flags.wifi_ok && !ntp_ok) {
    Serial.println("NTP connect attempt ");
    ntp.begin(ntp_config.server);
    ntp_ok = true;
    return 1000;
  } else if (!state_flags.wifi_ok) {
    Serial.println("NTP no wifi ");
    ntp.stop();
    ntp_ok = false;
    return 1000;
  } else {
    ntp.update();
    const int64_t new_ntp_time = ntp.epoch() * 1000000LL;
    if (new_ntp_time != sys_time.ntp_time) {
      sys_time.ntp_time = new_ntp_time;
      sys_time.rtc_at_ntp_time = RtcMicros(rtc);
      Serial.print("NTP: @  ");
      Serial.println(ntp.formattedTime("%A %T"));
    } else {
      Serial.print("NTP: Strange, did not change  - ignoring! @ ");
      Serial.println(ntp.formattedTime("%A %T"));
    }
    return 20000;
  }
}

int64_t UpdateMqtt(const Config::Mqtt &mqtt_config,
                   const StateFlags &state_flags, bool &mqtt_ok,
                   const MqttPacket &packet) {
  static bool connect_announce = true;
  static int is_connected_polls_left = 0;
  static int publish_failure_count = 0;
  static int64_t sent_version = 0;

  static WiFiClient wifiClient;
  static PubSubClient client(mqtt_config.broker, mqtt_config.port, wifiClient);
  static char message[2048];  // TODO: send buffer in client is 256

  if (packet.version <= sent_version) {
    // No new data yet.
    return 100;
  }

  if (!state_flags.wifi_ok) {
    client.disconnect();
    mqtt_ok = false;
    is_connected_polls_left = 0;
    Serial.println("MQTT no wifi");
    return 1000;
  } else if (state_flags.wifi_ok && !mqtt_ok && is_connected_polls_left < 1) {
    Serial.printf("MQTT connect attempt for packet %lld -> %lld\n",
                  sent_version, packet.version);
    client.disconnect();  // Just to be sure
    client.setKeepAlive(MQTT_KEEPALIVE_SEC);
    client.connect(mqtt_config.device_id, mqtt_config.user,
                   mqtt_config.password);
    client.setBufferSize(512);
    is_connected_polls_left = 100;
    return 500;
  } else if (state_flags.wifi_ok && !mqtt_ok && is_connected_polls_left > 0) {
    bool old_mqtt_ok = mqtt_ok;
    mqtt_ok = client.connected();
    if (is_connected_polls_left < 20) {
      Serial.printf("MQTT connected polls left %ld old_ok=%d ok=%d\n",
                    is_connected_polls_left, old_mqtt_ok, mqtt_ok);
    }
    is_connected_polls_left = mqtt_ok ? 0 : is_connected_polls_left - 1;
    return 500;
  } else if (state_flags.wifi_ok && mqtt_ok) {
    bool old_mqtt_ok = mqtt_ok;
    mqtt_ok = client.connected();
    if (!mqtt_ok) {
      is_connected_polls_left = 0;
      Serial.printf("MQTT disconnected %d -> %d\n", old_mqtt_ok, mqtt_ok);
      return 5000;  // Retry in 5s
    }
    bool success = false;
    if (MQTT_DO_PUBLISH) {
      Serial.printf("MQTT sending %lld\n", packet.version);
      snprintf(message, sizeof(message),
               R"({ 
  "data": { 
    "ping-count": %lld, 
    "tank-pressure": %ld,
    "sec-to-next-pump": %ld,
    "rtc-offset-post-init": %lld
  } 
  })",
               packet.version, packet.tank_pressure, packet.sec_to_next_pump,
               packet.rtc_offset_post_init);
      success = client.publish(mqtt_config.topic, message);
    } else {
      Serial.printf("MQTT FAKE sending %lld\n", packet.version);
      success = true;
    }
    sent_version = packet.version;
    publish_failure_count = success ? 0 : publish_failure_count + 1;
    if (publish_failure_count > 10) {
      Serial.printf("MQTT subsequent publish failures\n");
      publish_failure_count = 0;
      is_connected_polls_left = 0;
      mqtt_ok = false;
      return 5000;
    }
    WatchdogImAlive();
    return MQTT_UPDATE_USEC;
  }

  Serial.printf("MQTT bad state %d,%d,%d\n", state_flags.wifi_ok, mqtt_ok,
                is_connected_polls_left);
  return 1000;
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
  static int debug_print_count = 0;
  static SimpleKalmanFilter pressureKalmanFilter(100, 100, 0.1);
  int tank_pressure_raw = analogRead(TANK_PRESSURE);
  int estimated_pressure = static_cast<int>(
      pressureKalmanFilter.updateEstimate(tank_pressure_raw) + 0.5);
  packet.tank_pressure = estimated_pressure;
  packet.version++;
  if ((debug_print_count++) % 4 == 0) {
    Serial.printf("READ pressure %ld (raw %ld) @ %lld\n", packet.tank_pressure,
                  tank_pressure_raw, packet.version);
  }
  return 517LL;  // few ms extra to try to catch cancelling 50hz noise
}

int64_t TimeKeeper(const StateFlags &state_flags, SysTime &sys_time,
                   MqttPacket &mqtt, ESP32Time *rtc) {
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

  mqtt.rtc_offset_post_init = rtc_offset - initial_rtc_offset;
  return 5000;  // ZZZ more in final.
}

int64_t PumpControl(const Config::Schedule &schedule, bool &state_pumping,
                    const SysTime &sys_time, MqttPacket &mqtt_packet) {
  time_t best_time_s = BestMicros(sys_time) / 1000000LL;
  tm *dt = gmtime(&best_time_s);

  int time_of_day_s =
      Config::SafeHMSToSecondOfUtcDay(dt->tm_hour, dt->tm_min, dt->tm_sec);

  int min_next_s = 100000;
  bool active = false;
  for (int i_index = 0; i_index < schedule.interval_count; i_index++) {
    const Config::Schedule::Interval &i = schedule.intervals[i_index];
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

void setup() {
  pinMode(PUMP_CONTROL, OUTPUT);
  pinMode(PUMP_CONTROL_2, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(SETUP_MODE_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  WatchdogStart(WATCHDOG_TIMEOUT_S);
}

void loop() {
  if (digitalRead(SETUP_MODE_PIN) == LOW) {
    Serial.println("Entering setup mode...");
    SetupUI setup_ui;
    setup_ui.run();
    return;  // Exit loop to prevent further execution in setup mode
  }

  static std::unique_ptr<Config> config = Config::CreateFromJsonFile("/config.json");
  if (!config) {
    Serial.println("Configuration not loaded. Exiting loop.");
    return;
  } else {
    config->PrintConfigOnSerial();
  }

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
  dispatch(next_calls_ms.wifi, std::bind(ConnectWifi, std::cref(config->wifi),
                                         std::ref(state_flags.wifi_ok)));
  dispatch(next_calls_ms.ntp,
           std::bind(ConnectNtp, std::cref(config->ntp), std::cref(state_flags),
                     std::ref(state_flags.ntp_ok), std::ref(sys_time), &rtc));
  dispatch(next_calls_ms.timekeeper,
           std::bind(TimeKeeper, std::cref(state_flags), std::ref(sys_time),
                     std::ref(mqtt_packet), &rtc));
  dispatch(next_calls_ms.mqtt,
           std::bind(UpdateMqtt, std::cref(config->mqtt), std::cref(state_flags),
                     std::ref(state_flags.mqtt_ok), std::cref(mqtt_packet)));
  dispatch(next_calls_ms.read_pressure,
           std::bind(ReadTankPressure, std::ref(mqtt_packet)));
  dispatch(next_calls_ms.pump_control,
           std::bind(PumpControl, std::cref(config->schedule),
                     std::ref(state_flags.pumping), std::cref(sys_time),
                     std::ref(mqtt_packet)));

  dispatch(next_calls_ms.serial,
           std::bind(UpdateSerial, std::cref(sys_time), &rtc));

  epoch_ms = rtc.getEpoch() * 1000L + rtc.getMillis();
  delay(
      std::max(MIN_SLEEP_MS, std::min(next_epoch_ms - epoch_ms, MAX_SLEEP_MS)));
}
