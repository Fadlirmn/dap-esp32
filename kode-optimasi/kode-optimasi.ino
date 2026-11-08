// LVGL 8.x port of the 12-board dap-ui-landscape mockup, for ESP32 WROOM-32
// (no PSRAM) + ILI9341/ST7789 320x240 landscape via TFT_eSPI.
//
// This is a UI shell with demo/static data only (see README.md) — no real
// SD/audio/BT/WiFi backend. A later integration pass swaps the demo timers
// below for real input/audio/BT events via each screen's public setters.
//
// Setup needed before this compiles: see README.md for lv_conf.h placement
// and the TFT_eSPI_User_Setup.h pin/driver copy step.
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

#include "display_pins.h"
#include "screen_manager.h"
#include "demo_cycle.h"
#include "bt_backend.h"

TFT_eSPI tft = TFT_eSPI();

// Partial draw buffer: 15 lines only (not a full-screen buffer) to keep RAM
// usage low on a no-PSRAM ESP32. 320 * 15 * 2 bytes = 9600 bytes (~9.4 KB).
#define DRAW_BUF_LINES 15
static lv_color_t s_draw_buf1[SCREEN_W * DRAW_BUF_LINES];
static_assert(sizeof(s_draw_buf1) < 16 * 1024,
              "partial draw buffer exceeds the no-PSRAM RAM budget");
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(drv);
}

// Owns LVGL: all lv_obj_* / lv_timer_handler calls happen only on this task,
// so no mutex is needed anywhere in this codebase.
static void guiTask(void *pv) {
    LV_UNUSED(pv);
    uint32_t last_ms = millis();
    for (;;) {
        uint32_t now_ms = millis();
        lv_tick_inc(now_ms - last_ms);
        last_ms = now_ms;
        lv_timer_handler();
        bt_backend_tick(); // cheap: just checks the BT scan timeout, see bt_backend.cpp
        vTaskDelay(pdMS_TO_TICKS(66)); // 15 FPS: drives redraw + the cassette reel anim
    }
}

// Empty skeleton per the brief's FreeRTOS split: real I2S audio decode/output
// goes here later, pinned to core 0, independent of the GUI task above.
static void audioTaskStub(void *pv) {
    LV_UNUSED(pv);
    for (;;) {
        // TODO: I2S decode/output pipeline.
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(1); // landscape 320x240, same orientation as final-form

    lv_init();
    lv_disp_draw_buf_init(&s_draw_buf, s_draw_buf1, NULL, SCREEN_W * DRAW_BUF_LINES);
    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = SCREEN_W;
    s_disp_drv.ver_res = SCREEN_H;
    s_disp_drv.flush_cb = disp_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    screen_manager_init(SCREEN_NOW_PLAYING);
    demo_cycle_init();

    xTaskCreatePinnedToCore(guiTask, "GUI", 10240, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(audioTaskStub, "AudioStub", 4096, NULL, 3, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000)); // all real work happens in the two pinned tasks above
}
