#include "../src/modules/others/battery_information.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

// --- Retro-Go Wiring Buttons ---
#define BTN_UP     38  
#define BTN_DOWN   39  
#define BTN_LEFT   4   // Back
#define BTN_RIGHT  5   // Secondary Next
#define BTN_A      15  // Select
#define BTN_B      16  // Secondary Back
#define BTN_START  48  // Power off
#define BTN_SELECT 47  // Secondary Select

#define SDCARD_CS_PIN 1
#define TFT_BL_PIN 2

void powerOff();

void _setup_gpio() {
    Serial.println(">>> BUILD-FINAL: setup_gpio <<<");
    
    // 1. Initialize ADC (Required by Bruce's core)
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    // 2. Deselect SD Card
    pinMode(SDCARD_CS_PIN, OUTPUT);
    digitalWrite(SDCARD_CS_PIN, HIGH);

    // 3. Setup Buttons
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    // 4. Setup Backlight
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);

    // 5. Apply Bruce Configuration (Required by core)
    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 1;
}

void _post_setup_gpio() {
    Serial.printf(">>> BUILD-FINAL: post_setup_gpio, PSRAM=%u free heap=%u <<<\n",
                  (unsigned)ESP.getPsramSize(), (unsigned)ESP.getFreeHeap());
}

int getBattery() { return 100; }
void _setBrightness(uint8_t brightval) { analogWrite(TFT_BL_PIN, brightval); }

bool lastUp = HIGH, stableUp = HIGH; unsigned long lastUpTime = 0;
bool lastDown = HIGH, stableDown = HIGH; unsigned long lastDownTime = 0;
bool lastLeft = HIGH, stableLeft = HIGH; unsigned long lastLeftTime = 0;
bool lastRight = HIGH, stableRight = HIGH; unsigned long lastRightTime = 0;
bool lastA = HIGH, stableA = HIGH; unsigned long lastATime = 0;
bool lastB = HIGH, stableB = HIGH; unsigned long lastBTime = 0;
bool lastStart = HIGH, stableStart = HIGH; unsigned long lastStartTime = 0;
bool lastSelect = HIGH, stableSelect = HIGH; unsigned long lastSelectTime = 0;

bool checkButton(bool &last, bool &stable, unsigned long &lastTime, int pin) {
    bool reading = digitalRead(pin);
    if (reading != last) { lastTime = millis(); }
    last = reading;
    if ((millis() - lastTime) > 50) {
        if (reading != stable) {
            stable = reading;
            if (stable == LOW) return true;
        }
    }
    return false;
}

void InputHandler(void) {
    static unsigned long lastBeat = 0;
    if (millis() - lastBeat > 2000) {
        lastBeat = millis();
        Serial.printf("alive %lu\n", millis());
    }

    unsigned long now = millis();
    PrevPress = false; NextPress = false; SelPress = false; EscPress = false;

    if (checkButton(lastUp, stableUp, lastUpTime, BTN_UP)) { if (!wakeUpScreen()) { PrevPress = true; AnyKeyPress = true; } }
    if (checkButton(lastDown, stableDown, lastDownTime, BTN_DOWN)) { if (!wakeUpScreen()) { NextPress = true; AnyKeyPress = true; } }
    if (checkButton(lastLeft, stableLeft, lastLeftTime, BTN_LEFT)) { if (!wakeUpScreen()) { EscPress = true; AnyKeyPress = true; } }
    if (checkButton(lastRight, stableRight, lastRightTime, BTN_RIGHT)) { if (!wakeUpScreen()) { NextPress = true; AnyKeyPress = true; } }
    if (checkButton(lastA, stableA, lastATime, BTN_A)) { if (!wakeUpScreen()) { SelPress = true; AnyKeyPress = true; } }
    if (checkButton(lastB, stableB, lastBTime, BTN_B)) { if (!wakeUpScreen()) { EscPress = true; AnyKeyPress = true; } }
    if (checkButton(lastSelect, stableSelect, lastSelectTime, BTN_SELECT)) { if (!wakeUpScreen()) { SelPress = true; AnyKeyPress = true; } }
    
    checkButton(lastStart, stableStart, lastStartTime, BTN_START);

    if (digitalRead(BTN_START) == LOW && (millis() - lastStartTime > 600)) {
        powerOff();
    }
}

void powerOff() {
    digitalWrite(TFT_BL_PIN, LOW);
    while (digitalRead(BTN_START) == LOW) delay(10);
    while (digitalRead(BTN_A) == LOW) delay(10);
    rtc_gpio_pullup_en((gpio_num_t)BTN_A);
    rtc_gpio_pulldown_dis((gpio_num_t)BTN_A);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_A, 0);
    esp_deep_sleep_start();
}

void checkReboot() {}
