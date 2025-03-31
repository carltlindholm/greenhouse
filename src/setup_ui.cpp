#include "setup_ui.h"

#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
const char* ssid = "Greenhouse";
const char* password = "Tomatoes";
const IPAddress local_ip(192, 168, 42, 1);
const IPAddress gateway(192, 168, 42, 1);
const IPAddress subnet(255, 255, 255, 0);
const int kPort = 80;

void webserver() {
  WebServer server(kPort);  // Create a web server on port 80

  server.on("/ping", [&]() {
    server.send(200, "text/plain", "Welcome to Greenhouse!");
  });

  server.on("/", HTTP_GET, [&]() {
    if (SPIFFS.exists("/index.html")) {
      File file = SPIFFS.open("/setup.html", "r");
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(404, "text/plain", "File not found");
    }
  });

  server.begin();
  Serial.printf("Web server started at: %s:%d\n", WiFi.softAPIP().toString().c_str(), kPort);

  while (true) {
    server.handleClient();
  }
}
}  // namespace

void SetupUI::run() {
  // Start WiFi in Access Point mode
  WiFi.softAP(ssid, password);

  // Configure the Access Point IP address
  WiFi.softAPConfig(local_ip, gateway, subnet);

  // Log the IP address
  Serial.begin(115200);
  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  webserver();
}
