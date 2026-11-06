#include "../src/modules/others/battery_information.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "core/display.h"
#include <globals.h>
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

#ifndef TFT_BL
#define TFT_BL 2
#endif
#ifndef SDCARD_CS
#define SDCARD_CS 1
#endif

static const unsigned long RG_READ_DELAY   = 20;
static const unsigned long RG_DEBOUNCE     = 30;
static const unsigned long RG_FIRST_REPEAT = 400;
static const unsigned long RG_REPEAT       = 200;
static const unsigned long RG_POWEROFF_MS  = 1200;

struct RgKey {
    uint8_t pin;
    bool raw;
    bool stable;
    bool repeating;
    unsigned long changedAt;
    unsigned long pressedAt;
    unsigned long firedAt;
};

enum { RG_UP, RG_DOWN, RG_LEFT, RG_RIGHT, RG_A, RG_B, RG_SELECT, RG_START, RG_KEY_COUNT };

static RgKey rgKeys[RG_KEY_COUNT] = {
    {BTN_UP,     true, true, false, 0, 0, 0},
    {BTN_DOWN,   true, true, false, 0, 0, 0},
    {BTN_LEFT,   true, true, false, 0, 0, 0},
    {BTN_RIGHT,  true, true, false, 0, 0, 0},
    {BTN_A,      true, true, false, 0, 0, 0},
    {BTN_B,      true, true, false, 0, 0, 0},
    {BTN_SELECT, true, true, false, 0, 0, 0},
    {BTN_START,  true, true, false, 0, 0, 0},
};

void powerOff();

void _setup_gpio() {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
}

void _post_setup_gpio() {
    // FORCE the screen settings AFTER Bruce loads the SD card config!
    bruceConfigPins.rotation = 3; // If upside down, change this to 1
    tft.setRotation(3);           // If upside down, change this to 1

    bruceConfig.colorInverted = 1;
    tft.invertDisplay(1);

    tftWidth = tft.width();
    tftHeight = tft.height();

    Serial.printf("DISPLAY FIX: rot=%u inv=%u W=%d H=%d\n",
        (unsigned)bruceConfigPins.rotation,
        (unsigned)bruceConfig.colorInverted,
        (int)tftWidth, (int)tftHeight);
}

int getBattery() { return 100; }

void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
    } else if (brightval > 99) {
        analogWrite(TFT_BL, 254);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

static bool rgKeyEvent(RgKey &k, unsigned long now, bool repeatable) {
    bool reading = digitalRead(k.pin);
    if (reading != k.raw) {
        k.raw = reading;
        k.changedAt = now;
    }

    if (reading != k.stable && (now - k.changedAt) >= RG_DEBOUNCE) {
        k.stable = reading;
        if (reading == LOW) {
            k.pressedAt = now;
            k.firedAt = now;
            k.repeating = false;
            return true;
        }
        return false;
    }

    if (repeatable && k.stable == LOW) {
        unsigned long wait = k.repeating ? RG_REPEAT : RG_FIRST_REPEAT;
        if (now - k.firedAt >= wait) {
            k.firedAt = now;
            k.repeating = true;
            return true;
        }
    }
    return false;
}

void InputHandler(void) {
    static unsigned long lastRead = 0;
    unsigned long now = millis();
    if (now - lastRead < RG_READ_DELAY) return;
    lastRead = now;

    bool up    = rgKeyEvent(rgKeys[RG_UP], now, true);
    bool down  = rgKeyEvent(rgKeys[RG_DOWN], now, true);
    bool left  = rgKeyEvent(rgKeys[RG_LEFT], now, true);
    bool right = rgKeyEvent(rgKeys[RG_RIGHT], now, true);
    bool a     = rgKeyEvent(rgKeys[RG_A], now, false);
    bool b     = rgKeyEvent(rgKeys[RG_B], now, false);
    bool sel   = rgKeyEvent(rgKeys[RG_SELECT], now, false);
    bool start = rgKeyEvent(rgKeys[RG_START], now, false);

    if (rgKeys[RG_START].stable == LOW && (now - rgKeys[RG_START].pressedAt) >= RG_POWEROFF_MS) powerOff();
    if (start) wakeUpScreen();

    if (!(up || down || left || right || a || b || sel)) return;

    // Do NOT block the button press if the screen was asleep. 
    // Wake it up, but let the keypress register.
    wakeUpScreen(); 

    AnyKeyPress = true;

    // The correct 4-way mapping for Bruce's keyboard and menus
    if (up) UpPress = true;
    if (down) DownPress = true;
    if (left) PrevPress = true;
    if (right) NextPress = true;

    if (a || sel) SelPress = true;
    if (b) EscPress = true;
}

void powerOff() {
    analogWrite(TFT_BL, 0);
    while (digitalRead(BTN_START) == LOW || digitalRead(BTN_A) == LOW) delay(10);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_A, 0);
    rtc_gpio_pullup_en((gpio_num_t)BTN_A);
    rtc_gpio_pulldown_dis((gpio_num_t)BTN_A);
    esp_deep_sleep_start();
}

void checkReboot() {}
