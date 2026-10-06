#pragma once

#include <Preferences.h>
#include "pulseSensor.h"

void initStorage();
void savePulseCount(unsigned long pulseCount);
void savePulseConstant(uint16_t pulseConstant);
unsigned long readSavedPulseCount();
uint16_t readSavedPulseConstant();