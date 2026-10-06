#include "storage.h"

Preferences preferences;

void initStorage() {
    if (!preferences.begin("sensorData", false)) {
        Serial.println("Storage initialization failed");
        return;
    }

    totalPulses = readSavedPulseCount();
    pulseConstant = readSavedPulseConstant();

    Serial.print("Loaded pulses: ");
    Serial.println(totalPulses);
    Serial.print("Pulse Constant: ");
    Serial.println(pulseConstant);
}

void savePulseCount(unsigned long pulseCount) {
    preferences.putULong("totalPulses", pulseCount);
}

unsigned long readSavedPulseCount() {
    return preferences.getULong("totalPulses", 0);
}

void savePulseConstant(uint16_t pulseConstant) {
    preferences.putUShort("pulseConstant", pulseConstant);
}

uint16_t readSavedPulseConstant() {
    // return 1000 if there are no stored data
    return preferences.getULong("pulseConstant", 1000);
}