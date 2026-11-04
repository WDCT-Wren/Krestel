#include "header.h"

void setup() {
  Serial.begin(115200);
  initStorage();
  initSensor();
  initLED();
  initWebServer();
}

void loop() {
  pulseLED();

  updateSafeKwh();
  updateSafeRate();
  savePulseCount(totalPulses);
}