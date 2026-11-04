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

#define SDCARD_CS_PIN 1
#define TFT_BL_PIN 2

void powerOff();

void _setup_gpio() {
    Serial.println(">>> BUILD-POST-SETUP-FIX: setup_gpio <<<");
    
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    pinMode(SDCARD_CS_PIN, OUTPUT);
    digitalWrite(SDCARD_CS_PIN, HIGH);

    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);
}

void _post_setup_gpio() {
    // Force the orientation used by your Retro-Go target
    bruceConfigPins.rotation = 1;
    tft.setRotation(1);

    // Force display inversion ON to change the white background
    // to the intended dark background
    bruceConfig.colorInverted = 1;
    tft.invertDisplay(1);

    // Update Bruce's display dimensions after rotation
    tftWidth = tft.width();
    tftHeight = tft.height();

    Serial.printf(
        "DISPLAY FIX: rotation=%u inversion=%u width=%d height=%d\n",
        (unsigned)bruceConfigPins.rotation,
        (unsigned)bruceConfig.colorInverted,
        (int)tftWidth,
        (int)tftHeight
    );

    Serial.printf(
        "PSRAM=%u FREE_HEAP=%u\n",
        (unsigned)ESP.getPsramSize(),
        (unsigned)ESP.getFreeHeap()
    );
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
            if (stable == LOW) {
                return true;
            }
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

    // Reset navigation flags
    PrevPress = false;
    NextPress = false;
    UpPress = false;
    DownPress = false;
    SelPress = false;
    EscPress = false;

    // Read the physical buttons
    bool up = checkButton(lastUp, stableUp, lastUpTime, BTN_UP);
    bool down = checkButton(lastDown, stableDown, lastDownTime, BTN_DOWN);
    bool left = checkButton(lastLeft, stableLeft, lastLeftTime, BTN_LEFT);
    bool right = checkButton(lastRight, stableRight, lastRightTime, BTN_RIGHT);
    bool a = checkButton(lastA, stableA, lastATime, BTN_A);
    bool b = checkButton(lastB, stableB, lastBTime, BTN_B);
    bool sel = checkButton(lastSelect, stableSelect, lastSelectTime, BTN_SELECT);

    // Handle a press only after the display wake-up check
    if (up || down || left || right || a || b || sel) {
        if (!wakeUpScreen()) {
            AnyKeyPress = true;

            if (up) UpPress = true;
            if (down) DownPress = true;
            if (left) PrevPress = true;
            if (right) NextPress = true;

            if (a || sel) SelPress = true;
            if (b) EscPress = true;
        }
    }

    // START: hold for 600 ms to power off
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
