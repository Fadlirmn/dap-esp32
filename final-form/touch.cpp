#include "touch.h"

#if ENABLE_TOUCH
#include <XPT2046_Touchscreen.h>
#include "layout.h"
#include "nav.h"

XPT2046_Touchscreen touchscreen(PIN_TOUCH_CS, PIN_TOUCH_IRQ);

void touchSetup() {
    touchscreen.begin();  // default SPI bus -- shared with TFT/SD, see pins.h
}

void touchPoll() {
    static unsigned long last_touch_time = 0;
    if (!touchscreen.touched()) return;

    TS_Point p = touchscreen.getPoint();
    if (p.x == 8191 || p.y == 8191 || p.z < 250) return;

    // Calibration carried over from the original hardware's touch panel.
    // ponytail: recalibrate these map() ranges once real touch hardware is wired.
    int x = constrain(map(p.x, 200, 3700, 0, SCREEN_W - 1), 0, SCREEN_W - 1);
    int y = constrain(map(p.y, 240, 3800, 0, SCREEN_H - 1), 0, SCREEN_H - 1);

    if (millis() - last_touch_time > 300) {
        processGlobalTouch(x, y);
        last_touch_time = millis();
    }
}
#endif
