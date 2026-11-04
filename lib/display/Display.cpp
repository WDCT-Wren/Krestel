#include <Wire.h>
#include "display.h"
#include "pulseSensor.h"

LiquidCrystal_I2C lcd(0x27, 24, 4);

void initDisplay() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void displayData() {
    lcd.setCursor(0, 0);
    lcd.print("Pulses: ");
    lcd.print(totalPulses);

    lcd.setCursor(0, 1);
    lcd.print("kwh: ");
    lcd.print(safeKwhRead);

    lcd.setCursor(0, 3);
    lcd.print(PULSE_CONSTANT);
    lcd.print(" imp/kwh");
}