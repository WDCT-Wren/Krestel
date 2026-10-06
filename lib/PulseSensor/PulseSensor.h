#pragma once

#include <Arduino.h>

#define SENSOR_PIN 4

const unsigned long DEBOUNCE_MILLIS = 50; // 50 milliseconds
constexpr uint16_t DEFAULT_PULSE_CONSTANT = 1600; // default

extern volatile unsigned long lastPulseTime;
extern volatile unsigned long rawFires;
extern volatile unsigned long totalPulses;

extern double safeKwhRead;
extern uint16_t pulseConstant;
extern uint8_t utilityRate;

void readPulse();
void initSensor();
void updateSafeKwh();
void updateSafeRate();