#pragma once

#include <ESPAsyncWebServer.h>

// HTTP handlers for the dashboard API. They are registered on the server in
// server.cpp.

// GET /api/metrics  — pulse count, kWh values, server uptime
void sendMetrics(AsyncWebServerRequest *request);

// GET /api/config   — pulse constant, utility rate, baseline, IP address
void sendConfig(AsyncWebServerRequest *request);

// POST /api/pulse-config — update the configurable pulse constant
void receivePulseConfig(AsyncWebServerRequest *request);

// POST /api/rate-config  — update the configurable utility rate
void receiveUtilityRateConfig(AsyncWebServerRequest *request);