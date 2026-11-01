#include "../src/modules/others/battery_information.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

// --- 8 buttons, identical to the Retro-Go RG_GAMEPAD_GPIO_MAP ---
#define BTN_UP     38  // Prev / Up
#define BTN_DOWN   39  // Next / Down
#define BTN_LEFT   4   // Back
#define BTN_RIGHT  5   // Next (secondary)
#define BTN_A      15  // Select / OK (also the wake-up button)
#define BTN_B      16  // Back (secondary)
#define BTN_START  48  // Hold = power off (deep sleep)
#define BTN_SELECT 47  // Select (secondary)

const unsigned long debounceDelay = 50;
const unsigned long longPressTime = 600;

struct ButtonState {
    bool lastReading = HIGH;
    bool stableState = HIGH;
    unsigned long lastDebounceTime = 0;
    unsigned long pressTime = 0;
    bool longPressed = false;
};

ButtonState btnUp, btnDown, btnLeft, btnRight, btnA, btnB, btnStart, btnSel;

void powerOff();

void _setup_gpio() {
    // Keep the SD card deselected (CS high) so it stays quiet on the shared SPI bus
    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);

    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    // TFT_BL (pin 2) comes from platformio.ini - do not redefine it here
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 1;
}

void _post_setup_gpio() {}

int getBattery() { return 100; }

void _setBrightness(uint8_t brightval) { analogWrite(TFT_BL, brightval); }

bool checkButton(ButtonState &btn, int pin) {
    bool reading = digitalRead(pin);
    if (reading != btn.lastReading) {
        btn.lastDebounceTime = millis();
    }
    btn.lastReading = reading;

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
    return false;
}

void InputHandler(void) {
    // --- Heartbeat Test ---
    static unsigned long lastBeat = 0;
    if (millis() - lastBeat > 2000) {
        lastBeat = millis();
        Serial.printf("alive %lu\n", millis());
    }

    unsigned long now = millis();
    PrevPress = false;
    NextPress = false;
    SelPress = false;
    EscPress = false;

    if (checkButton(btnUp, BTN_UP)) {
        if (!wakeUpScreen()) { PrevPress = true; AnyKeyPress = true; }
    }
    if (checkButton(btnDown, BTN_DOWN)) {
        if (!wakeUpScreen()) { NextPress = true; AnyKeyPress = true; }
    }
    if (checkButton(btnLeft, BTN_LEFT)) {
        if (!wakeUpScreen()) { EscPress = true; AnyKeyPress = true; }
    }
    if (checkButton(btnRight, BTN_RIGHT)) {
        if (!wakeUpScreen()) { NextPress = true; AnyKeyPress = true; }
    }
    if (checkButton(btnA, BTN_A)) {
        if (!wakeUpScreen()) { SelPress = true; AnyKeyPress = true; }
    }
    if (checkButton(btnB, BTN_B)) {
        if (!wakeUpScreen()) { EscPress = true; AnyKeyPress = true; }
    }
    if (checkButton(btnSel, BTN_SELECT)) {
        if (!wakeUpScreen()) { SelPress = true; AnyKeyPress = true; }
    }
    if (checkButton(btnStart, BTN_START)) {
        wakeUpScreen();
    }

    if (digitalRead(BTN_START) == LOW && !btnStart.longPressed &&
        (now - btnStart.pressTime > longPressTime)) {
        btnStart.longPressed = true;
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
