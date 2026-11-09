#pragma once

#include <Preferences.h>
#include "meter.h"
#include "pulseSensor.h"

void initStorage();

// Write methods
void savePulseCount(unsigned long pulseCount);
void savePulseConstant(uint16_t pulseConstant);
void saveUtilityRate(float utilityRate);
void saveBaseline(double baselineKwh, unsigned long baselinePulses);

// Read methods
double readBaselineKwh();
unsigned long readBaselinePulse();
unsigned long readSavedPulseCount();
uint16_t readSavedPulseConstant();
float readSavedUtilityRate();