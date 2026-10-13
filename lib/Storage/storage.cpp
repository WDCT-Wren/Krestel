#include "storage.h"

Preferences preferences;

void initStorage()
{
    if (!preferences.begin("sensorData", false))
    {
        Serial.println("Storage initialization failed");
        return;
    }

    currPulseCounted = readSavedPulseCount();
    pulseConstant = readSavedPulseConstant();
    utilityRate = readSavedUtilityRate();

    baselinePulses = readBaselinePulse();
    baselineKwh = readBaselineKwh();

    Serial.print("Loaded pulses: ");
    Serial.println(currPulseCounted);
    Serial.print("Pulse Constant: ");
    Serial.println(pulseConstant);
    Serial.print("Utility Rate: ");
    Serial.println(utilityRate);
    Serial.print("Baseline KWh: ");
    Serial.println(baselineKwh);
    Serial.print("Baseline pulses: ");
    Serial.println(baselinePulses);
}

void saveBaseline(double baselineKwh, unsigned long baselinePulses) {
    preferences.putDouble("baselineKwh", baselineKwh);
    preferences.putULong("baselinePulses", baselinePulses);
}
double readBaselineKwh() { return preferences.getDouble("baselineKwh", 0);}
unsigned long readBaselinePulse() { return preferences.getULong("baselinePulses", 0);}

void savePulseCount(unsigned long pulseCount) { preferences.putULong("totalPulses", pulseCount);}
unsigned long readSavedPulseCount() { return preferences.getULong("totalPulses", 0);}

void savePulseConstant(uint16_t pulseConstant) { preferences.putUShort("pulseConstant", pulseConstant);}
uint16_t readSavedPulseConstant(){ return preferences.getUShort("pulseConstant", 1000);}

void saveUtilityRate(float utilityRate) { preferences.putFloat("utilityRate", utilityRate);}
float readSavedUtilityRate() { return preferences.getFloat("utilityRate", 10);}