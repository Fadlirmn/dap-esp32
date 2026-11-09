// Optional secondary input: XPT2046 resistive touch, shared SPI bus.
// Ported from final-form/touch.cpp/.h, retargeted at a plain LVGL
// LV_INDEV_TYPE_POINTER (same shape as sim/sim_main.cpp's mouse_read_cb)
// instead of final-form's nav.h processGlobalTouch -- every screen here
// already handles pointer taps correctly, so this needs no per-screen code.
#pragma once
#include "display_pins.h"

#if ENABLE_TOUCH
#include <lvgl.h>
void touchSetup();
void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
#endif
