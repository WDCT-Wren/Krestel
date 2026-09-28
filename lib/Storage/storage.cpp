#include "storage.h"

Preferences preferences;

void initStorage() {
    if (!preferences.begin("sensorData", false)) {
        Serial.println("Storage initialization failed");
        return;
    }

    uint32_t flag = preferences.getUInt("magicFlag", 0);

    // Detects new flash storage
    if (flag != PREF_INIT_MAGIC) {
        Serial.println("New storage");

        totalPulses = 0;
        pulseConstant = DEFAULT_PULSE_CONSTANT; // default pulse constant if none was read
        const unsigned long initialPulseCount = totalPulses;
        
        preferences.putULong("totalPulses", totalPulses);
        preferences.putUShort("pulseConstant", pulseConstant);

        preferences.putUInt("magicFlag", PREF_INIT_MAGIC);
    } else {
        totalPulses = readSavedPulseCount();
        pulseConstant = readSavedPulseConstant();

        Serial.print("Loaded pulses: ");
        Serial.println(totalPulses);
        Serial.print("Pulse Constant: ");
        Serial.println(pulseConstant);
    }
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
    return preferences.getULong("pulseConstant", 1000);
}