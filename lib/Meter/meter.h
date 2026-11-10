#pragma once
#include <Arduino.h>

extern double baselineKwh; 
extern double currentKwh;

//Kwh reading derived from totalPulses
extern double safeKwhRead;

//Estimated utility derived from the totalKwh and the utility rate
extern double safeEstimateReading;

extern uint16_t pulseConstant;
extern float utilityRate;

void updateSafeKwh();
void updateSafeRate();