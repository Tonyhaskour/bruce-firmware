#include "../src/modules/others/battery_information.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>

// --- 8 Physical Buttons from your Target Definition ---
#define BTN_UP     38  
#define BTN_DOWN   39  
#define BTN_LEFT   4   // Dedicated Back / Escape
#define BTN_RIGHT  5   // Secondary Next
#define BTN_A      15  // Primary Select / OK
#define BTN_B      16  // Secondary Back / Escape
#define BTN_START  48  // Home / Long press for Deep Sleep
#define BTN_SELECT 47  // Secondary Select

const unsigned long debounceDelay = 50;
const unsigned long longPressTime = 600;

struct ButtonState {
    bool lastReading = HIGH;
    bool stableState = HIGH;
    unsigned long lastDebounceTime = 0;
    unsigned long pressTime = 0;
    bool longPressed = false;
};

ButtonState up, down, left, right, a, b, start, btnSelect;

void _setup_gpio() {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    // DO NOT redefine TFT_BL here. The platformio.ini handles it (Pin 2).
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 1;
}

void _post_setup_gpio() {}

// Hardcoded to 100% to prevent random shutdowns if the battery pin is floating
int getBattery() { return 100; } 
void _setBrightness(uint8_t brightval) { analogWrite(TFT_BL, brightval); }

bool checkButton(ButtonState &btn, int pin) {
    bool reading = digitalRead(pin);
    if (reading != btn.lastReading) {
        btn.lastDebounceTime = millis();
    }
    if ((millis() - btn.lastDebounceTime) > debounceDelay) {
        if (reading != btn.stableState) {
            btn.stableState = reading;
            if (btn.stableState == LOW) {
                btn.pressTime = millis();
                btn.longPressed = false;
                return true;
            }
        }
    }
    btn.lastReading = reading;
    return false;
}

void InputHandler(void) {
    unsigned long now = millis();
    PrevPress = false; NextPress = false; SelPress = false; EscPress = false;

    if (checkButton(up, BTN_UP)) {
        if (!wakeUpScreen()) { PrevPress = true; AnyKeyPress = true; }
    }
    if (checkButton(down, BTN_DOWN)) {
        if (!wakeUpScreen()) { NextPress = true; AnyKeyPress = true; }
    }
    if (checkButton(left, BTN_LEFT)) {
        if (!wakeUpScreen()) { EscPress = true; AnyKeyPress = true; }
    }
    if (checkButton(right, BTN_RIGHT)) {
        if (!wakeUpScreen()) { NextPress = true; AnyKeyPress = true; }
    }
    if (checkButton(a, BTN_A)) {
        if (!wakeUpScreen()) { SelPress = true; AnyKeyPress = true; }
    }
    if (checkButton(b, BTN_B)) {
        if (!wakeUpScreen()) { EscPress = true; AnyKeyPress = true; }
    }
    if (checkButton(btnSelect, BTN_SELECT)) {
        if (!wakeUpScreen()) { SelPress = true; AnyKeyPress = true; }
    }
    if (checkButton(start, BTN_START)) {
        start.pressTime = now;
        start.longPressed = false;
    }

    // Long press START for Deep Sleep
    if (digitalRead(BTN_START) == LOW && !start.longPressed && (now - start.pressTime > longPressTime)) {
        start.longPressed = true;
        powerOff();
    }
}

void powerOff() {
    analogWrite(TFT_BL, 0);
    esp_deep_sleep_start();
}

void checkReboot() {}
