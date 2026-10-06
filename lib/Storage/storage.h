#pragma once

#include <Preferences.h>
#include "pulseSensor.h"

void initStorage();

// Write methods
void savePulseCount(unsigned long pulseCount);
void savePulseConstant(uint16_t pulseConstant);
void saveUtilityRate(float utilityRate);

// Read methods
unsigned long readSavedPulseCount();
uint16_t readSavedPulseConstant();
float readSavedUtilityRate();