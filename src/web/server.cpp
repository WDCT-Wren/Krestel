#include "server.h"

static AsyncWebServer server(WEB_SERVER_PORT);


// Method that connects the esp32 to the wifi connection to serve a website for devices connected to the same wifi.
static void connectWiFi() {
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);

  WiFi.persistent(false);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  

  int attempts = 0;
  while (WiFiClass::status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFiClass::status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected — IP Address: %s\n", WiFi.localIP().toString().c_str());
  } else {
    // Create own WiFi hotspot for devices to connect to (Access Point)

    Serial.println("\n[WiFi] Connection failed. Starting AP mode...");
    WiFiClass::mode(WIFI_AP);
    WiFi.softAP("Krestel-Setup", "Krestel123");
    Serial.printf("[WiFi] AP Mode — IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
}

// Server setup

void initWebServer() {
  connectWiFi();

  if (!LittleFS.begin(true)) {
    Serial.println("[LittleFS] Mount failed — formatting...");
    LittleFS.format();
    LittleFS.begin(true); 
  }
  Serial.println("[LittleFS] Mounted successfully");


  // Static file serving from LittleFS
  // Serves /dashboard/index.html at GET /
  server.serveStatic("/", LittleFS, "/dashboard/").setDefaultFile("index.html");

  // API endpoints
  server.on("/api/metrics", HTTP_GET, sendMetrics);
  server.on("/api/config", HTTP_GET, sendConfig);

  server.on("/api/pulse-config", HTTP_POST, receivePulseConfig);
  server.on("/api/rate-config", HTTP_POST, receiveUtilityRateConfig);

  // Daily usage chart
  // TODO: implement daily usage chart response

  // Fallback 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404: Not found");
  });

  server.begin();
  Serial.println("[Web] Server started");
}
