#include "../src/modules/others/battery_information.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#define BTN_UP     38
#define BTN_DOWN   39
#define BTN_LEFT   4
#define BTN_RIGHT  5
#define BTN_A      15
#define BTN_B      16
#define BTN_START  48
#define BTN_SELECT 47

void _setup_gpio() {
    Serial.println(">>> BUILD-T11: setup_gpio <<<");
    
    // Initialize ADC (Bruce's core requires this to prevent crashes)
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    // TFT_BL is defined in pins_arduino.h as 2
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 1;
}

void _post_setup_gpio() {
    Serial.printf(">>> BUILD-T11: post_setup_gpio, PSRAM=%u free heap=%u <<<\n",
                  (unsigned)ESP.getPsramSize(), (unsigned)ESP.getFreeHeap());
}

int getBattery() { return 100; }
void _setBrightness(uint8_t brightval) { analogWrite(TFT_BL, brightval); }

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
    digitalWrite(TFT_BL, LOW);
    while (digitalRead(BTN_START) == LOW) delay(10);
    while (digitalRead(BTN_A) == LOW) delay(10);
    rtc_gpio_pullup_en((gpio_num_t)BTN_A);
    rtc_gpio_pulldown_dis((gpio_num_t)BTN_A);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_A, 0);
    esp_deep_sleep_start();
}

void checkReboot() {}
