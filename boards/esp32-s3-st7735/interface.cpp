#include "../src/modules/others/battery_information.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>

#define BTN_PREV 12
#define BTN_NEXT 13
#define SEL_BTN  14
#define TFT_BL   4

const unsigned long debounceDelay = 50;
const unsigned long longPressTime = 500; 

bool lastPrevReading = HIGH, stablePrevState = HIGH; unsigned long lastPrevDebounce = 0;
bool lastNextReading = HIGH, stableNextState = HIGH; unsigned long lastNextDebounce = 0;
bool lastSelReading = HIGH,  stableSelState = HIGH;  unsigned long lastSelDebounce = 0;

unsigned long selPressTime = 0;
bool selLongPressed = false;

void _setup_gpio() {
    pinMode(BTN_PREV, INPUT_PULLUP);
    pinMode(BTN_NEXT, INPUT_PULLUP);
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 1;
}

void _post_setup_gpio() {}

// Hardcoded to 100% to prevent the board from shutting down if the battery pin is floating
int getBattery() { return 100; } 

void _setBrightness(uint8_t brightval) { analogWrite(TFT_BL, brightval); }

void InputHandler(void) {
    unsigned long now = millis();
    PrevPress = false; NextPress = false; SelPress = false; EscPress = false;

    // Read Prev / Up
    bool prevReading = digitalRead(BTN_PREV);
    if (prevReading != lastPrevReading) { lastPrevDebounce = now; }
    if ((now - lastPrevDebounce) > debounceDelay) {
        if (prevReading != stablePrevState) {
            stablePrevState = prevReading;
            if (stablePrevState == LOW) { 
                if (!wakeUpScreen()) { // Wakes screen if asleep, otherwise triggers button
                    PrevPress = true; 
                    AnyKeyPress = true; 
                }
            }
        }
    }
    lastPrevReading = prevReading;

    // Read Next / Down
    bool nextReading = digitalRead(BTN_NEXT);
    if (nextReading != lastNextReading) { lastNextDebounce = now; }
    if ((now - lastNextDebounce) > debounceDelay) {
        if (nextReading != stableNextState) {
            stableNextState = nextReading;
            if (stableNextState == LOW) { 
                if (!wakeUpScreen()) {
                    NextPress = true; 
                    AnyKeyPress = true; 
                }
            }
        }
    }
    lastNextReading = nextReading;

    // Read Select (Short press = Select, Long press = Exit/Back)
    bool selReading = digitalRead(SEL_BTN);
    if (selReading != lastSelReading) { lastSelDebounce = now; }
    if ((now - lastSelDebounce) > debounceDelay) {
        if (selReading != stableSelState) {
            stableSelState = selReading;
            if (stableSelState == LOW) { 
                selPressTime = now; 
                selLongPressed = false;
            } else {
                if (!selLongPressed) { 
                    if (!wakeUpScreen()) {
                        SelPress = true; 
                        AnyKeyPress = true; 
                    }
                }
            }
        }
    }
    lastSelReading = selReading;

    // Trigger Back/Exit if held down
    if (stableSelState == LOW && !selLongPressed && (now - selPressTime > longPressTime)) {
        if (!wakeUpScreen()) {
            EscPress = true; 
            AnyKeyPress = true;
        }
        selLongPressed = true;
    }
}

void powerOff() {}
void checkReboot() {}