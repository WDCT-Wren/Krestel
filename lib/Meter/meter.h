#pragma once
#include <Arduino.h>

extern unsigned long baselinePulses;
extern double baselineKwh; 

extern double safeKwhRead;
extern double safeEstimateReading;
extern uint16_t pulseConstant;
extern float utilityRate;

void updateSafeKwh();
void updateSafeRate();