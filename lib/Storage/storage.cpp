#include "storage.h"

void initStorage() {
    if (!EEPROM.begin(EEPROM_PULSE_ADDR + sizeof(unsigned long))) {
        Serial.println("EEPROM initialization failed");
        return;
    }

    uint8_t flag;
    EEPROM.get(EEPROM_INIT_ADDR, flag);

    Serial.print("Expected magic: 0x");
    Serial.println(EEPROM_INIT_MAGIC, HEX);

    Serial.print("Stored flag: 0x");
    Serial.println(flag, HEX);

    if (flag != EEPROM_INIT_MAGIC) {
        Serial.println("New storage");

        totalPulses = 0;
        pulseConstant = DEFAULT_PULSE_CONSTANT; // default pulse constant if none was read
        const unsigned long initialPulseCount = totalPulses;
        EEPROM.put(EEPROM_PULSE_ADDR, initialPulseCount);
        EEPROM.put(EEPROM_CONSTANT_ADDR, pulseConstant);
        EEPROM.put(EEPROM_INIT_ADDR, EEPROM_INIT_MAGIC);
        EEPROM.commit();
    } else {
        totalPulses = readSavedPulseCount();

        Serial.print("Loaded pulses: ");
        Serial.println(totalPulses);
        Serial.print("Pulse Constant: ");
        Serial.println(pulseConstant);
    }
}

void savePulseCount(unsigned long pulseCount) {
    EEPROM.put(EEPROM_PULSE_ADDR, pulseCount);
    EEPROM.commit();
}

unsigned long readSavedPulseCount() {
    unsigned long savedTotalPulse = 0;
    EEPROM.get(EEPROM_PULSE_ADDR, savedTotalPulse);
    return savedTotalPulse;
}

void savePulseConstant(uint16_t pulseConstant) {
    EEPROM.put(EEPROM_CONSTANT_ADDR, pulseConstant);
    EEPROM.commit();
}

uint16_t readSavedPulseConstant() {
    uint16_t pulseconstant = 1000;
    EEPROM.get(EEPROM_CONSTANT_ADDR, pulseconstant);
    return pulseconstant;
}