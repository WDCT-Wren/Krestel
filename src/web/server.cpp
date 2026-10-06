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
    WiFi.softAP("Wattrack-Setup", "wattrack123");
    Serial.printf("[WiFi] AP Mode — IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
}

// Helper function to give data from server to web dashbaord

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

/**
 * Recieves pulse config data for configurable pulse constant 
 */
static void handlePulseConfig(AsyncWebServerRequest * request) {
  const AsyncWebParameter *param = request->getParam("pulse-config", true);

  // Never dereference it without checking, or the ESP32 panics.
  if (param == nullptr) {
    request->send(400, "text/plain", "Missing 'pulse-config' parameter");
    return;
  }

  uint16_t newPulseConfig = (uint16_t)param->value().toInt();

  if (newPulseConfig == 0) {
    request->send(400, "text/plain", "Invalid 'pulse-config' value");
    return;
  }

  pulseConstant = newPulseConfig;
  savePulseConstant(newPulseConfig);

  Serial.printf("[Web] New pulse config: %u\n", newPulseConfig);

  request->send(200, "text/plain", "OK");
}

/**
 * Recieves utility rate from configurable utility rate
 */
static void handleUtilityRateConfig(AsyncWebServerRequest * request) {
  const AsyncWebParameter *param = request->getParam("rate-config", true);

  // Never dereference it without checking, or the ESP32 panics.
  if (param == nullptr) {
    request->send(400, "text/plain", "Missing 'rate-config' parameter");
    return;
  }

  float newUtilityRate = param->value().toFloat();

  if (newUtilityRate == 0) {
    request->send(400, "text/plain", "Invalid 'rate-config' value");
    return;
  }

  utilityRate = newUtilityRate;
  saveUtilityRate(newUtilityRate);

  Serial.printf("[Web] New Utility Rate: %.2f\n", newUtilityRate);

  request->send(200, "text/plain", "OK");
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
  server.on("/api/pulse-config", HTTP_POST, handlePulseConfig);
  server.on("/api/rate-config", HTTP_POST, handleUtilityRateConfig);

  // Daily usage chart
  // TODO: implement daily usage chart response

  // Fallback 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404: Not found");
  });

  server.begin();
  Serial.println("[Web] Server started");
}