#include "server.h"

static AsyncWebServer server(WEB_SERVER_PORT);


// Method that connects the esp32 to the wifi connection to serve a website for devices connected to the same wifi.
static void connectWiFi() {
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
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
    WiFi.softAP("Wattrack-Setup", "wattrack123");
    Serial.printf("[WiFi] AP Mode — IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
}

// JSON helpers

/**
 * JSON helper for capturing sensor data such as:
 *  - pulse count
 *  - Kwh consumed
 *  - server uptime
 */
static void handleMetrics(AsyncWebServerRequest *request) {
  unsigned long pulses;
  double currentKwh;

  noInterrupts();
  pulses = totalPulses;
  currentKwh = safeKwhRead;
  interrupts();

  String json = "{";
  json += "\"pulses\":" + String(pulses) + ",";
  json += "\"kwh\":" + String(currentKwh, 2) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  request->send(200, "application/json", json);
}

/**
 * Sends the esp32's local IP address for the dashboard
 */ 
static void handleIPAdress(AsyncWebServerRequest * request) {
  request->send(200, "text/plain", WiFi.localIP().toString().c_str());
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

  // List files in LittleFS for debugging
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("[LittleFS] %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }

  // Static file serving from LittleFS
  // Serves /dashboard/index.html at GET /
  server.serveStatic("/", LittleFS, "/dashboard/").setDefaultFile("index.html");

  // API endpoints
  server.on("/api/metrics", HTTP_GET, handleMetrics);
  server.on("/api/ip_address", HTTP_GET, handleIPAdress);

  // Daily usage chart
  // TODO: implement daily usage chart response

  // Fallback 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404: Not found");
  });

  server.begin();
  Serial.println("[Web] Server started");
}