#include "config.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <bits/unique_ptr.h>

namespace {

// Clock arithmetic mod (negative values are wrapped around)
// e.g. -1 % 24 = 23
constexpr int SafeMod(int value, int mod) { return (value % mod + mod) % mod; }

}  // namespace

int Config::SafeHMSToSecondOfUtcDay(int h, int m, int s) {
  return SafeMod(h, 24) * 3600 + SafeMod(m, 60) * 60 + SafeMod(s, 60);
}

std::unique_ptr<Config> Config::CreateFromJsonFile(const char file[]) {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return nullptr;
  }

  File configFile = SPIFFS.open(file, "r");
  if (!configFile) {
    Serial.printf("Failed to open config file: %s\n", file);
    return nullptr;
  }

  size_t size = configFile.size() + 1;
  if (size == 0) {
    Serial.println("Config file is empty");
    return nullptr;
  }

  std::unique_ptr<char[]> buffer(new char[size]);
  configFile.readBytes(buffer.get(), size);
  buffer[size - 1] = '\0';  // Null-terminate the string
  configFile.close();

  auto config = std::make_unique<Config>();
  DeserializationError error = deserializeJson(config->jsonDoc, buffer.get());
  if (error) {
    Serial.printf("Failed to parse config file: %s\n", error.c_str());
    return nullptr;
  }

  auto &jsonDoc = config->jsonDoc;

  // Parse WiFi configuration
  JsonObject wifi = jsonDoc["wifi"];
  if (!wifi.isNull()) {
    config->wifi.ssid = wifi["ssid"] | "";
    config->wifi.password = wifi["password"] | "";
  }

  // Parse NTP configuration
  JsonObject ntp = jsonDoc["ntp"];
  if (!ntp.isNull()) {
    config->ntp.server = ntp["server"] | "pool.ntp.org";
  }

  // Parse MQTT configuration
  JsonObject mqtt = jsonDoc["mqtt"];
  if (!mqtt.isNull()) {
    config->mqtt.broker = mqtt["broker"] | "";
    config->mqtt.port = mqtt["port"] | 1883;
    config->mqtt.user = mqtt["user"] | "";
    config->mqtt.password = mqtt["password"] | "";
    config->mqtt.device_id = mqtt["device_id"] | "";
    config->mqtt.topic = mqtt["topic"] | "";
  }

  // Parse pump schedule
  JsonObject pumpSchedule = jsonDoc["pumpSchedule"];
  if (!pumpSchedule.isNull()) {
    config->schedule.interval_count = 0;

    const int utcOffset =
        pumpSchedule["utcOffset"] | 0;  // Default to 0 if missing
    JsonArray pump = pumpSchedule["pump"];
    for (JsonObject interval : pump) {
      if (config->schedule.interval_count >= Config::kMaxIntervals) {
        Serial.println("Too many pump intervals, truncating");
        break;
      }
      Config::Schedule::Interval &currentInterval =
          config->schedule.intervals[config->schedule.interval_count++];
      const JsonObject start = interval["start"];
      const JsonObject end = interval["end"];
      if (!start.isNull() && !end.isNull()) {
        currentInterval.start_sec = Config::SafeHMSToSecondOfUtcDay(
            (start["hour"] | 0) - utcOffset, start["minute"] | 0,
            start["second"] | 0);
        currentInterval.end_sec = Config::SafeHMSToSecondOfUtcDay(
            (end["hour"] | 0) - utcOffset, end["minute"] | 0,
            end["second"] | 0);
      }
    }
  }

  return config;
}

void Config::PrintConfigOnSerial() const {
  Serial.println("** Configuration **");
  Serial.println("wifi");
  Serial.printf("  ssid = %s\n", wifi.ssid ? wifi.ssid : "(not set)");
  Serial.printf("  password = %s\n", wifi.password ? wifi.password : "(not set)");

  Serial.println("ntp");
  Serial.printf("  server = %s\n", ntp.server ? ntp.server : "(not set)");

  Serial.println("mqtt");
  Serial.printf("  broker = %s\n", mqtt.broker ? mqtt.broker : "(not set)");
  Serial.printf("  port = %d\n", mqtt.port);
  Serial.printf("  user = %s\n", mqtt.user ? mqtt.user : "(not set)");
  Serial.printf("  password = %s\n", mqtt.password ? mqtt.password : "(not set)");
  Serial.printf("  device_id = %s\n", mqtt.device_id ? mqtt.device_id : "(not set)");
  Serial.printf("  topic = %s\n", mqtt.topic ? mqtt.topic : "(not set)");

  Serial.println("schedule");
  Serial.printf("  interval_count = %d\n", schedule.interval_count);
  for (int i = 0; i < schedule.interval_count; ++i) {
    const auto &interval = schedule.intervals[i];
    Serial.printf("  - interval %d\n", i + 1);
    Serial.printf("      start_sec = %d\n", interval.start_sec);
    Serial.printf("      end_sec = %d\n", interval.end_sec);
  }
  Serial.println("***");
}
