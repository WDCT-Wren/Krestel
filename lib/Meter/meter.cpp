#include "meter.h"
#include "pulseSensor.h"

unsigned long baselinePulses;
double baselineKwh;

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