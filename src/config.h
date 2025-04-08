#include <memory>
#include <ArduinoJson.h>

// Holds the current configuration for the device.
class Config {
 private:
  static constexpr size_t kMaxJsonSize = 64 * 1024;  // 64k constant
  static constexpr int kMaxIntervals = 20;          // Maximum number of pump intervals

  Config() = default;
  Config(const Config&) = delete;
  Config(Config&&) = delete;
  
  friend std::unique_ptr<Config> std::make_unique<Config>();
 public:
  // Converts hour, minute, and second to seconds since the start of the UTC day.
  // Handles negative values and wraps overflows correctly (e.g. minute 62 becomes minute 2).
  static int SafeHMSToSecondOfUtcDay(int h, int m, int s);

  // Loads or nullptr if failed.
  static std::unique_ptr<Config> CreateFromJsonFile(const char file[]);

  // Prints the configuration in a YAML-like format to Serial.
  void PrintConfigOnSerial() const;

  struct Wifi {
    const char *ssid;
    const char *password;
  };

  struct Ntp {
    const char *server;
  };

  struct Mqtt {
    const char *broker;
    int port;
    const char *user;
    const char *password;
    const char *device_id;
    const char *topic;
  };

  struct Schedule {
    // Note: seconds start from 0 on day starting at 0:00 UTC.
    struct Interval {
      int start_sec;
      int end_sec;
    };
    Interval intervals[kMaxIntervals];  // Use kMaxIntervals for flexibility
    int interval_count;
  };

  Wifi wifi;
  Ntp ntp;
  Mqtt mqtt;
  Schedule schedule;
 private:
  String string_table;  // String table for storing interned strings of the config.
};
