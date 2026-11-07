#include "input.h"
#include "pins.h"
#include "nav.h"

// Encoder direction logic ported from old/files/test_input.ino's encISR (same
// 2ms interrupt debounce, same A/B-phase comparison) -- only the destination
// changed: instead of clamping a local `vol`, it accumulates signed ticks for
// inputPoll() to hand to nav.h's handleEncoderRotate() outside interrupt context
// (Preferences/BT calls there aren't IRAM-safe).
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
constexpr unsigned long LONG_PRESS_MS = 600;
constexpr unsigned long BTN_REPEAT_DELAY_MS = 400;
constexpr unsigned long BTN_REPEAT_MS = 150;
constexpr unsigned long DEBOUNCE_MS = 30;

void inputSetup() {
    pinMode(PIN_ENC_CLK, INPUT_PULLUP);
    pinMode(PIN_ENC_DT, INPUT_PULLUP);
    pinMode(PIN_ENC_SW, INPUT_PULLUP);
    pinMode(PIN_BTN_PREV, INPUT_PULLUP);
    pinMode(PIN_BTN_NEXT, INPUT_PULLUP);
    pinMode(PIN_BTN_MENU, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderISR, FALLING);
}

// ---- encoder switch: click / double-click / long-press ----
static void pollEncoderSwitch() {
    static bool lastSw = HIGH;
    static unsigned long downAt = 0, lastEdgeAt = 0, pendingClickAt = 0;
    static bool longPressFired = false, pendingClick = false;
    unsigned long now = millis();

    bool sw = digitalRead(PIN_ENC_SW);
    if (sw != lastSw && now - lastEdgeAt >= DEBOUNCE_MS) {
        lastEdgeAt = now;
        if (sw == LOW) {
            downAt = now;
            longPressFired = false;
        } else if (!longPressFired) {
            if (pendingClick && now - pendingClickAt <= DOUBLE_CLICK_MS) {
                handleEncoderDoubleClick();
                pendingClick = false;
            } else {
                pendingClick = true;
                pendingClickAt = now;
            }
        }
        lastSw = sw;
    }

    if (sw == LOW && !longPressFired && now - downAt >= LONG_PRESS_MS) {
        handleEncoderLongPress();
        longPressFired = true;
        pendingClick = false;
    }

    if (pendingClick && now - pendingClickAt > DOUBLE_CLICK_MS) {
        handleEncoderClick();
        pendingClick = false;
    }
}

// ---- MENU: short press (back/menu) vs. long press (open mode switcher) ----
static void pollMenuButton() {
    static bool last = HIGH;
    static unsigned long downAt = 0, lastEdgeAt = 0;
    static bool longFired = false;
    unsigned long now = millis();

    bool b = digitalRead(PIN_BTN_MENU);
    if (b != last && now - lastEdgeAt >= DEBOUNCE_MS) {
        lastEdgeAt = now;
        last = b;
        if (b == LOW) {
            downAt = now;
            longFired = false;
        } else if (!longFired) {
            handleMenuButton();
        }
    }
    if (b == LOW && !longFired && now - downAt >= LONG_PRESS_MS) {
        handleMenuLongPress();
        longFired = true;
    }
}

// ---- PREV/NEXT/MENU: debounced press + hold-to-repeat (PREV/NEXT only) ----
static void pollButton(uint8_t pin, void (*onFire)(), bool repeats) {
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
            onFire();
        }
        return;
    }
    if (repeats && b == LOW && now - s.downAt > BTN_REPEAT_DELAY_MS && now - s.lastRepeatAt > BTN_REPEAT_MS) {
        s.lastRepeatAt = now;
        onFire();
    }
}

void inputPoll() {
    pollEncoderSwitch();
    pollButton(PIN_BTN_PREV, handlePrevButton, true);
    pollButton(PIN_BTN_NEXT, handleNextButton, true);
    pollMenuButton();

    noInterrupts();
    int ticks = encoderTicks;
    encoderTicks = 0;
    interrupts();
    if (ticks != 0) handleEncoderRotate(ticks);
}
