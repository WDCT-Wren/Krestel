#include "storage.h"

Preferences preferences;

void initStorage()
{
    if (!preferences.begin("sensorData", false))
    {
        Serial.println("Storage initialization failed");
        return;
    }

    totalPulses = readSavedPulseCount();
    pulseConstant = readSavedPulseConstant();
    utilityRate = readSavedUtilityRate();

    Serial.print("Loaded pulses: ");
    Serial.println(totalPulses);
    Serial.print("Pulse Constant: ");
    Serial.println(pulseConstant);
    Serial.print("Utility Rate: ");
    Serial.println(utilityRate);
}

void savePulseCount(unsigned long pulseCount) { preferences.putULong("totalPulses", pulseCount);}
unsigned long readSavedPulseCount() { return preferences.getULong("totalPulses", 0);}

void savePulseConstant(uint16_t pulseConstant) { preferences.putUShort("pulseConstant", pulseConstant);}
uint16_t readSavedPulseConstant(){ return preferences.getUShort("pulseConstant", 1000);}

void saveUtilityRate(float utilityRate) { preferences.putFloat("utilityRate", utilityRate);}
float readSavedUtilityRate() { return preferences.getFloat("utilityRate", 10);}