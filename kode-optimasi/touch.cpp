#include "touch.h"

#if ENABLE_TOUCH
#include <XPT2046_Touchscreen.h>

static XPT2046_Touchscreen touchscreen(PIN_TOUCH_CS, PIN_TOUCH_IRQ);
static int16_t s_last_x = 0, s_last_y = 0;

void touchSetup() {
    touchscreen.begin();  // default SPI bus -- shared with TFT/SD, see display_pins.h
}

void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    LV_UNUSED(drv);
    if (touchscreen.touched()) {
        TS_Point p = touchscreen.getPoint();
        if (p.x != 8191 && p.y != 8191 && p.z >= 250) {
            // Calibration carried over from final-form/touch.cpp (original
            // hardware's touch panel).
            // ponytail: recalibrate these map() ranges once real touch hardware is wired.
            s_last_x = constrain(map(p.x, 200, 3700, 0, SCREEN_W - 1), 0, SCREEN_W - 1);
            s_last_y = constrain(map(p.y, 240, 3800, 0, SCREEN_H - 1), 0, SCREEN_H - 1);
        }
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->point.x = s_last_x;
    data->point.y = s_last_y;
}
#endif
