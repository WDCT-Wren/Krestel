#pragma once

#include <EEPROM.h>
#include "pulseSensor.h"

#define EEPROM_INIT_ADDR 0
#define EEPROM_INIT_MAGIC 0xA5
#define EEPROM_CONSTANT_ADDR 1 // cosntant takes address 1-2
#define EEPROM_PULSE_ADDR 3 // takes address 3-6

void initStorage();
void savePulseCount(unsigned long pulseCount);
void savePulseConstant(uint16_t pulseConstant);
unsigned long readSavedPulseCount();
uint16_t readSavedPulseConstant();