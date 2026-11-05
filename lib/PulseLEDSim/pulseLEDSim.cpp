#include <Arduino.h>
#include <stdint.h>
#include "pulseLEDSim.h"

void initLED() {
  pinMode(LED_PIN, OUTPUT);
}

void pulseLED() {
    digitalWrite(LED_PIN, HIGH);
    delay(5);
    digitalWrite(LED_PIN, LOW);
    delay(5);
  digitalWrite(LED_PIN, HIGH);   // second rapid transition, well inside 50ms window
    delay(5);
    digitalWrite(LED_PIN, LOW);
    delay(200);
}
