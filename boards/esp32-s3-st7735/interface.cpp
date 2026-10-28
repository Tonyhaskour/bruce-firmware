#include "../src/modules/others/battery_information.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>

// --- 8 Physical Buttons from your Target Definition ---
#define BTN_UP     38  // Navigation Up / Previous
#define BTN_DOWN   39  // Navigation Down / Next
#define BTN_LEFT   4   // Dedicated Back / Escape
#define BTN_RIGHT  5   // Right / Tab switch (mapped as NextPress for now)
#define BTN_A      15  // Primary Select / OK
#define BTN_B      16  // Secondary Back / Escape
#define BTN_START  48  // Home / Quick Access (Long press for Deep Sleep)
#define BTN_SELECT 47  // Secondary Select / Menu

// --- Screen Backlight ---
#define TFT_BL     2 

const unsigned long debounceDelay = 50;
const unsigned long longPressTime = 600; // 600ms for Deep Sleep

// State tracking for all 8 buttons
struct ButtonState {
    bool lastReading = HIGH;
    bool stableState = HIGH;
    unsigned long lastDebounceTime = 0;
    unsigned long pressTime = 0;
    bool longPressed = false;
};

ButtonState up, down, left, right, a, b, start, select;

void _setup_gpio() {
    // All buttons are configured with internal pull-ups (connected to GND)
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 1;
}

void _post_setup_gpio() {}

// Hardcoded to 100% to prevent random shutdowns if the battery pin is floating
int getBattery() { return 100; } 

void _setBrightness(uint8_t brightval) { analogWrite(TFT_BL, brightval); }

// Helper function to handle debouncing for any button
bool checkButton(ButtonState &btn, int pin) {
    bool reading = digitalRead(pin);
    if (reading != btn.lastReading) {
        btn.lastDebounceTime = millis();
    }
    if ((millis() - btn.lastDebounceTime) > debounceDelay) {
        if (reading != btn.stableState) {
            btn.stableState = reading;
            if (btn.stableState == LOW) { // Button pressed
                btn.pressTime = millis();
                btn.longPressed = false;
                return true; // Short press registered
            }
        }
    }
    btn.lastReading = reading;
    return false;
}

void InputHandler(void) {
    unsigned long now = millis();
    PrevPress = false; NextPress = false; SelPress = false; EscPress = false;

    // 1. UP Button -> Previous Item
    if (checkButton(up, BTN_UP)) {
        if (!wakeUpScreen()) { PrevPress = true; AnyKeyPress = true; }
    }

    // 2. DOWN Button -> Next Item
    if (checkButton(down, BTN_DOWN)) {
        if (!wakeUpScreen()) { NextPress = true; AnyKeyPress = true; }
    }

    // 3. LEFT Button -> Dedicated Back / Escape
    if (checkButton(left, BTN_LEFT)) {
        if (!wakeUpScreen()) { EscPress = true; AnyKeyPress = true; }
    }

    // 4. RIGHT Button -> Secondary Next (or map to tab switch if needed)
    if (checkButton(right, BTN_RIGHT)) {
        if (!wakeUpScreen()) { NextPress = true; AnyKeyPress = true; }
    }

    // 5. A Button -> Primary Select / OK
    if (checkButton(a, BTN_A)) {
        if (!wakeUpScreen()) { SelPress = true; AnyKeyPress = true; }
    }

    // 6. B Button -> Secondary Back / Escape
    if (checkButton(b, BTN_B)) {
        if (!wakeUpScreen()) { EscPress = true; AnyKeyPress = true; }
    }

    // 7. SELECT Button -> Secondary Select / Menu
    if (checkButton(select, BTN_SELECT)) {
        if (!wakeUpScreen()) { SelPress = true; AnyKeyPress = true; }
    }

    // 8. START Button -> Home / Quick Access + Long Press for Deep Sleep
    if (checkButton(start, BTN_START)) {
        start.pressTime = now;
        start.longPressed = false;
        if (!wakeUpScreen()) { 
            // You can map START to a specific action, e.g., Quick Access
            // For now, we'll treat it as a standard select or ignore it
            // SelPress = true; AnyKeyPress = true;
        }
    }

    // Check for Long Press on START button to power off
    if (digitalRead(BTN_START) == LOW && !start.longPressed && (now - start.pressTime > longPressTime)) {
        start.longPressed = true;
        powerOff();
    }
}

void powerOff() {
    // Put device into deep sleep
    analogWrite(TFT_BL, 0);
    esp_deep_sleep_start();
}

void checkReboot() {}
