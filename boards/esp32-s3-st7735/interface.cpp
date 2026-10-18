//   Developed by ViniciusHNF
//   GitHub Repository: https://github.com/viniciushnf/ESP32-S3-Bruce-ST7735-1.8

#include "../src/modules/others/battery_information.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>

// ======================================================
//        SETTINGS
// ======================================================
const unsigned long debounceDelay = 50; // Debounce button filter

// ======================================================
//        PINS (Configured for Digital Push Buttons)
// ======================================================
#ifndef BTN_PREV
#define BTN_PREV 12 // Old JOY_X pin -> Connected to Button 1 (Prev / Up)
#endif
#ifndef BTN_NEXT
#define BTN_NEXT 13 // Old JOY_Y pin -> Connected to Button 2 (Next / Down)
#endif
#ifndef SEL_BTN
#define SEL_BTN 14  // Joystick SW pin -> Connected to Button 3 (Select / OK)
#endif
#ifndef TFT_BL
#define TFT_BL 4
#endif

// ======================================================
// BUTTON STATES & DEBOUNCE TIMERS
// ======================================================
bool lastPrevReading = HIGH;
bool stablePrevState = HIGH;
unsigned long lastPrevDebounce = 0;

bool lastNextReading = HIGH;
bool stableNextState = HIGH;
unsigned long lastNextDebounce = 0;

bool lastSelReading = HIGH;
bool stableSelState = HIGH;
unsigned long lastSelDebounce = 0;

// ======================================================
// SETUP GPIO
// ======================================================
void _setup_gpio() {
    // Configure push buttons as inputs with internal pull-ups
    pinMode(BTN_PREV, INPUT_PULLUP);
    pinMode(BTN_NEXT, INPUT_PULLUP);
    pinMode(SEL_BTN, INPUT_PULLUP);

    pinMode(TFT_BL, OUTPUT);    // Backlight OUTPUT
    digitalWrite(TFT_BL, HIGH); // Backlight HIGH

    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 1;
}

void _post_setup_gpio() {
    // nothing
}

// ======================================================
//        BATTERY
// ======================================================
int getBattery() { return Battery_information::getBatteryPercentage(); }

// ======================================================
//        SCREEN BRIGHTNESS
// ======================================================
void _setBrightness(uint8_t brightval) { analogWrite(TFT_BL, brightval); }

// ======================================================
//        INPUT HANDLER (Digital Push Buttons)
// ======================================================
void InputHandler(void) {
    unsigned long now = millis();

    // Reset temporary flags at the start of the cycle
    PrevPress = false;
    NextPress = false;
    SelPress = false;

    // ==================================================
    // 1. PREV / UP BUTTON (GPIO 12)
    // ==================================================
    bool prevReading = digitalRead(BTN_PREV);
    if (prevReading != lastPrevReading) { lastPrevDebounce = now; }
    if ((now - lastPrevDebounce) > debounceDelay) {
        if (prevReading != stablePrevState) {
            stablePrevState = prevReading;
            if (stablePrevState == LOW) { // Button pressed (Active LOW)
                PrevPress = true;
                AnyKeyPress = true;
            }
        }
    }
    lastPrevReading = prevReading;

    // ==================================================
    // 2. NEXT / DOWN BUTTON (GPIO 13)
    // ==================================================
    bool nextReading = digitalRead(BTN_NEXT);
    if (nextReading != lastNextReading) { lastNextDebounce = now; }
    if ((now - lastNextDebounce) > debounceDelay) {
        if (nextReading != stableNextState) {
            stableNextState = nextReading;
            if (stableNextState == LOW) { // Button pressed (Active LOW)
                NextPress = true;
                AnyKeyPress = true;
            }
        }
    }
    lastNextReading = nextReading;

    // ==================================================
    // 3. SELECT / OK BUTTON (GPIO 14)
    // ==================================================
    bool selReading = digitalRead(SEL_BTN);
    if (selReading != lastSelReading) { lastSelDebounce = now; }
    if ((now - lastSelDebounce) > debounceDelay) {
        if (selReading != stableSelState) {
            stableSelState = selReading;
            if (stableSelState == LOW) { // Button pressed (Active LOW)
                SelPress = true;
                AnyKeyPress = true;
            }
        }
    }
    lastSelReading = selReading;

    // Wake up screen automatically on any button press if asleep
    if (AnyKeyPress) {
        wakeUpScreen();
    }
}

// ======================================================
// POWER
// ======================================================
void powerOff() {}
void checkReboot() {}