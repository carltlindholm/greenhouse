#include "setup_ui.h"

#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
const char* kSetupHtmlPath = "/setup.html.gz";
const char* kConfigJsonPath = "/config.json";
const char* kPostBodyArgName = "plain";
const char* ssid = "Greenhouse";
const char* password = "Tomatoes";
const IPAddress local_ip(192, 168, 42, 1);
const IPAddress gateway(192, 168, 42, 1);
const IPAddress subnet(255, 255, 255, 0);
const int kPort = 80;

void RunWebServer() {
  WebServer server(kPort);

  server.on("/", HTTP_GET, [&]() {
    if (SPIFFS.exists(kSetupHtmlPath)) {
      File file = SPIFFS.open(kSetupHtmlPath, "r");
      // streamFile adds server.sendHeader("Content-Encoding", "gzip", true);
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(404, "text/plain", "File not found");
    }
  });

  server.on("/api/settings", HTTP_GET, [&]() {
    if (SPIFFS.exists(kConfigJsonPath)) {
      File file = SPIFFS.open(kConfigJsonPath, "r");
      String settings = file.readString();
      file.close();
      server.send(200, "application/json", settings);
      Serial.println("Responded with configs loaded from file.");
    } else {
      server.send(200, "application/json", R"({
        "wifi": {
          "ssid": "",
          "password": ""
        },
        "ntp": {
          "server": "pool.ntp.org"
        },
        "mqtt": {
          "broker": "",
          "port": 1883,
          "user": "",
          "password": "",
          "deviceId": "",
          "topic": ""
        },
        "pumpSchedule": {
          "pump": [],
          "utcOffset": 3
        }
      })");
      Serial.println("Responded with empty config (no file?).");
    }
  });

  server.on("/api/save-settings", HTTP_POST, [&]() {
    if (server.hasArg(kPostBodyArgName)) {
      String body = server.arg(kPostBodyArgName);
      File file = SPIFFS.open("/config.json", "w");
      if (file) {
        Serial.println("Saving settings...");
        file.print(body);
        file.close();
        server.send(200, "text/plain", "Settings saved");
        Serial.println("Settings saved successfully");
      } else {
        server.send(500, "text/plain", "Failed to save settings");
      }
    } else {
      server.send(400, "text/plain", "Bad Request");
      Serial.println("FAILED to save settings");
    }
  });

  server.on("/api/reboot", HTTP_POST, [&]() {
    Serial.println("Reboot requested!");
    server.send(200, "text/plain", "Rebooting...");
    delay(1000);
    ESP.restart();
  });

  server.on("/api/factory", HTTP_GET, [&]() {
    if (!server.hasArg("reset")) {
      server.send(400, "text/plain", "Missing parameter");
      return;
    }

    String body = server.arg("doit");
    if (body != "42") {
      server.send(400, "text/plain", "Invalid parameter value");
      return;
    }

    if (!SPIFFS.exists(kConfigJsonPath)) {
      server.send(404, "text/plain", "Config file not found");
      return;
    }

    if (SPIFFS.remove(kConfigJsonPath)) {
      server.send(200, "text/plain", "Config file deleted");
    } else {
      server.send(500, "text/plain", "Failed to delete config file");
    }
  });

  server.on("/ping", [&]() {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Pong @ %d.", millis());
    server.send(200, "text/plain", buffer); 
  });

  server.begin();
  Serial.printf("Web server started at: %s:%d\n",
                WiFi.softAPIP().toString().c_str(), kPort);

  while (true) {
    server.handleClient();
  }
}
void WifiAccessPoint() {
  // Start WiFi in Access Point mode
  WiFi.softAP(ssid, password);

  // Configure the Access Point IP address
  WiFi.softAPConfig(local_ip, gateway, subnet);

  // Log the IP address
  if (SPIFFS.begin(true)) {
    Serial.println("SPIFFS mounted successfully");
  } else {
    Serial.println("SPIFFS mount failed");
  }

  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

}  // namespace

void SetupUI::run() {
  WifiAccessPoint();
  RunWebServer();
}
