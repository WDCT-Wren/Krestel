#include "pulseSensor.h"
#include "storage.h"

volatile unsigned long lastPulseTime = 0;
volatile unsigned long rawFires = 0;
volatile unsigned long totalPulses = 0;

double safeKwhRead = 0;
double safeEstimateReading = 0;

uint16_t pulseConstant = DEFAULT_PULSE_CONSTANT; //default
float utilityRate = DEFAULT_RATE;

/**
 * Initializes the sensor as an input as well as attatching it into an interrupt.
 */
void initSensor() {
  pinMode(SENSOR_PIN, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(SENSOR_PIN),
    readPulse,
    FALLING
  );
}

void updateSafeKwh() {
  noInterrupts();
  safeKwhRead = static_cast<double>(totalPulses) / pulseConstant;
  interrupts();
}

void updateSafeRate() {
  noInterrupts();
  safeEstimateReading = static_cast<double>(utilityRate) * safeKwhRead;
  interrupts();
}

/*
  reads and detects pulses of light from the photodiode
*/
void readPulse() {
  unsigned long now = millis();
  rawFires++;

  //Debounce logic 
  if (now - lastPulseTime < DEBOUNCE_MILLIS) return;

  lastPulseTime = now; 

  totalPulses++;
}