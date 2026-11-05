#include "handlers.h"

#include "header.h"

/**
 * JSON helper for capturing sensor data such as:
 *  - pulse count
 *  - Kwh consumed
 *  - server uptime
 */
void sendMetrics(AsyncWebServerRequest *request) {
  unsigned long pulses;
  double currentKwh;
  double sinceBaselineKwh;
  double meterKwh;
  bool hasBaseline;

  noInterrupts();
  pulses = totalPulses;
  currentKwh = safeKwhRead;
  sinceBaselineKwh = safeSinceBaselineKwh;
  meterKwh = safeMeterKwhRead;
  hasBaseline = baselineValid;
  interrupts();

  // 'kwh' stays device-relative (totalPulses / pulseConstant); 'meter_kwh' is
  // the absolute register and is only meaningful when a baseline exists.
  String json = "{";
  json += "\"pulses\":" + String(pulses) + ",";
  json += "\"kwh\":" + String(currentKwh, 2) + ",";
  json += "\"since_baseline_kwh\":" + String(sinceBaselineKwh, 2) + ",";
  json += "\"meter_kwh\":" + String(meterKwh, 2) + ",";
  json += "\"has_baseline\":" + String(hasBaseline ? "true" : "false") + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  request->send(200, "application/json", json);
}

/**
 * Sends the API current configuration of the imp to the dashboard such as:
 *  - Pulse constant (i.e. 1000imp)
 *  - Utility rate (i.e. 12.5)
 *  - Server IP Address (i.e. 255.255.255.255)
 */
void sendConfig(AsyncWebServerRequest * request) {
  String configJson = "{";
  configJson += "\"constant\":" + String(pulseConstant) + ",";
  configJson += "\"rate\":" + String(utilityRate) + ",";
  configJson += "\"has_baseline\":" + String(baselineValid ? "true" : "false") + ",";
  configJson += "\"baseline_kwh\":" + String(baselineKwh, 2) + ",";
  configJson += "\"baseline_pulses\":" + String(baselinePulses) + ",";
  configJson += "\"ip_address\":\"" + WiFi.localIP().toString() + "\"";
  configJson += "}";

  request->send(200, "application/json", configJson);
}

/**
 * Recieves pulse config data for configurable pulse constant
 */
void receivePulseConfig(AsyncWebServerRequest * request) {
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
void receiveUtilityRateConfig(AsyncWebServerRequest * request) {
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


