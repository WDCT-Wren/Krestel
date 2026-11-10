#include "meter.h"
#include "pulseSensor.h"

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