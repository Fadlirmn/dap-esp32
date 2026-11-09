#include "input.h"
#include <Arduino.h>
#include <lvgl.h>
#include "display_pins.h"
#include "screen_manager.h"
#include "overlay_volume.h"
#include "screen_now_playing.h"
#include "screen_receiver.h"
#include "screen_transmitter.h"

// Encoder direction logic ported verbatim from final-form/input.cpp (itself
// from old/files/test_input.ino's encISR): same 2ms interrupt debounce, same
// A/B-phase comparison. Destination changed: ticks drive the volume HUD
// directly (see inputPoll), not a local `vol` or nav.h handler.
static volatile int encoderTicks = 0;

static void IRAM_ATTR encoderISR() {
    static uint32_t t = 0;
    uint32_t now = millis();
    if (now - t < 2) return;
    t = now;
    if (digitalRead(PIN_ENC_CLK) == digitalRead(PIN_ENC_DT)) encoderTicks--;
    else encoderTicks++;
}

constexpr unsigned long DOUBLE_CLICK_MS = 400;
constexpr unsigned long BTN_REPEAT_DELAY_MS = 400;
constexpr unsigned long BTN_REPEAT_MS = 150;
constexpr unsigned long DEBOUNCE_MS = 30;

// Fed to the LVGL encoder indev (see encoder_read_cb) -- ENC_SW's raw
// debounced level IS the indev's PRESSED/RELEASED state, so a single
// click reaches the focused widget via LVGL's own encoder handling with no
// extra code here; only the double-click (track skip) needs our own timer.
static volatile bool s_enc_sw_down = false;
static volatile int16_t s_enc_diff_pending = 0;

// ---- ENC_SW: raw level for the indev + double-click detection for track skip ----
static void on_encoder_double_click() {
    switch (screen_manager_current()) {
        case SCREEN_NOW_PLAYING: now_playing_next_track(); break;
        case SCREEN_RECEIVER:    receiver_next_track(); break;
        case SCREEN_TRANSMITTER: transmitter_next_track(); break;
        default: break; // no track concept on other screens
    }
}

static void pollEncoderSwitch() {
    static bool lastSw = HIGH;
    static unsigned long lastEdgeAt = 0, pendingClickAt = 0;
    static bool pendingClick = false;
    unsigned long now = millis();

    bool sw = digitalRead(PIN_ENC_SW);
    if (sw != lastSw && now - lastEdgeAt >= DEBOUNCE_MS) {
        lastEdgeAt = now;
        lastSw = sw;
        s_enc_sw_down = (sw == LOW);
        if (sw == HIGH) { // released -- resolve single vs. double click
            if (pendingClick && now - pendingClickAt <= DOUBLE_CLICK_MS) {
                on_encoder_double_click();
                pendingClick = false;
            } else {
                pendingClick = true;
                pendingClickAt = now;
            }
        }
    }
    if (pendingClick && now - pendingClickAt > DOUBLE_CLICK_MS) {
        pendingClick = false; // window closed with no second click -- the single click already reached LVGL via raw state above
    }
}

// ---- PREV/NEXT: debounced press + hold-to-repeat, feeds the encoder indev's enc_diff ----
static void pollButton(uint8_t pin, int8_t diff, bool repeats) {
    struct State { bool last; unsigned long downAt, lastEdgeAt, lastRepeatAt; };
    static State states[40] = {};  // indexed by pin; GPIOs here are all < 40
    State &s = states[pin];
    unsigned long now = millis();

    bool b = digitalRead(pin);
    if (b != s.last && now - s.lastEdgeAt >= DEBOUNCE_MS) {
        s.lastEdgeAt = now;
        s.last = b;
        if (b == LOW) {
            s.downAt = now;
            s.lastRepeatAt = now;
            s_enc_diff_pending += diff;
        }
        return;
    }
    if (repeats && b == LOW && now - s.downAt > BTN_REPEAT_DELAY_MS && now - s.lastRepeatAt > BTN_REPEAT_MS) {
        s.lastRepeatAt = now;
        s_enc_diff_pending += diff;
    }
}

static void pollMenuButton() {
    static bool last = HIGH;
    static unsigned long lastEdgeAt = 0;
    unsigned long now = millis();

    bool b = digitalRead(PIN_BTN_MENU);
    if (b != last && now - lastEdgeAt >= DEBOUNCE_MS) {
        lastEdgeAt = now;
        last = b;
        if (b == LOW) screen_manager_show(SCREEN_MENU);
    }
}

static void encoder_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    LV_UNUSED(drv);
    data->enc_diff = (int16_t)s_enc_diff_pending;
    s_enc_diff_pending = 0;
    data->state = s_enc_sw_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void inputSetup() {
    pinMode(PIN_ENC_CLK, INPUT_PULLUP);
    pinMode(PIN_ENC_DT, INPUT_PULLUP);
    pinMode(PIN_ENC_SW, INPUT_PULLUP);
    pinMode(PIN_BTN_PREV, INPUT_PULLUP);
    pinMode(PIN_BTN_NEXT, INPUT_PULLUP);
    pinMode(PIN_BTN_MENU, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderISR, FALLING);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_read_cb;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    lv_indev_set_group(indev, screen_manager_group());
}

void inputPoll() {
    pollEncoderSwitch();
    pollButton(PIN_BTN_PREV, -1, true);
    pollButton(PIN_BTN_NEXT, +1, true);
    pollMenuButton();

    noInterrupts();
    int ticks = encoderTicks;
    encoderTicks = 0;
    interrupts();
    if (ticks != 0) overlay_volume_adjust(ticks * 5); // same +-5 step as sim/sim_main.cpp's mouse wheel
}
