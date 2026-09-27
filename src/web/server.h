#pragma once

#include "header.h"
#include <LittleFS.h>

// Real credentials live in credentials.h (gitignored)
// Copy credentials.h.example to credentials.h and fill in your WiFi details
#include "credentials.h"

// Web server port
#define WEB_SERVER_PORT 80

// Initialize WiFi and start the async web server
void initWebServer();
