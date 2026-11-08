#include "theme.h"
#include <Adafruit_ILI9341.h>

extern Adafruit_ILI9341 tft;

uint16_t COLOR_BG, COLOR_PANEL, COLOR_PANEL_BORDER, COLOR_ACCENT, COLOR_ACCENT_DIM,
    COLOR_TEXT_PRIMARY, COLOR_TEXT_SECONDARY, COLOR_HEART_ACTIVE, COLOR_HEART_INACTIVE,
    COLOR_TRACK_BG, COLOR_BT_CONNECTED, COLOR_BT_DISCONNECTED;
uint16_t COLOR_BT_PANEL, COLOR_BT_PANEL_TEXT, COLOR_WARNING, COLOR_SUCCESS,
    COLOR_CHIP_BG, COLOR_CHIP_TEXT;
uint16_t COLOR_BUTTON_BG, COLOR_ACCENT_TEXT, COLOR_OVERLAY_TRACK, COLOR_ERROR;
uint16_t COLOR_ON_ACCENT_DIM;
uint8_t themeIndex = 1;  // default Blue, closest existing theme to dap-ui-design's accent

// Base/neutral colors updated to dap-ui-design's Material dark palette
// (bg #101418, panel #1C2026, border #33383F, text #E2E2E9/#A3ACB8).
void initColorTheme() {
    COLOR_BG              = tft.color565(16, 20, 24);
    COLOR_PANEL           = tft.color565(28, 32, 38);
    COLOR_PANEL_BORDER    = tft.color565(51, 56, 63);
    COLOR_ACCENT          = tft.color565(0, 224, 193);
    COLOR_ACCENT_DIM      = tft.color565(0, 120, 105);
    COLOR_TEXT_PRIMARY    = tft.color565(226, 226, 233);
    COLOR_TEXT_SECONDARY  = tft.color565(163, 172, 184);
    COLOR_HEART_ACTIVE    = tft.color565(255, 82, 110);
    COLOR_HEART_INACTIVE  = tft.color565(78, 83, 92);
    COLOR_TRACK_BG        = tft.color565(51, 56, 63);
    COLOR_BT_CONNECTED    = tft.color565(52, 145, 255);
    COLOR_BT_DISCONNECTED = tft.color565(255, 180, 171);  // #FFB4AB, matches COLOR_ERROR below

    COLOR_BT_PANEL      = tft.color565(59, 42, 95);
    COLOR_BT_PANEL_TEXT = tft.color565(233, 221, 255);
    COLOR_WARNING       = tft.color565(255, 217, 125);
    COLOR_SUCCESS       = tft.color565(125, 219, 154);
    COLOR_CHIP_BG       = tft.color565(58, 70, 88);
    COLOR_CHIP_TEXT     = tft.color565(216, 227, 248);

    COLOR_BUTTON_BG     = tft.color565(39, 42, 48);
    COLOR_ACCENT_TEXT   = tft.color565(0, 48, 98);
    COLOR_OVERLAY_TRACK = tft.color565(74, 80, 89);
    COLOR_ERROR         = tft.color565(255, 180, 171);
    COLOR_ON_ACCENT_DIM = tft.color565(214, 227, 255);
}

void applyTheme(uint8_t theme) {
    themeIndex = theme % 5;
    if (themeIndex == 0) {
        COLOR_ACCENT = tft.color565(112, 226, 125);
        COLOR_ACCENT_DIM = tft.color565(55, 130, 70);
    } else if (themeIndex == 1) {
        COLOR_ACCENT = tft.color565(168, 200, 255);
        COLOR_ACCENT_DIM = tft.color565(0, 70, 140);
    } else if (themeIndex == 2) {
        COLOR_ACCENT = tft.color565(190, 105, 255);
        COLOR_ACCENT_DIM = tft.color565(105, 55, 155);
    } else if (themeIndex == 3) {
        COLOR_ACCENT = tft.color565(255, 170, 70);
        COLOR_ACCENT_DIM = tft.color565(155, 90, 35);
    } else {
        COLOR_ACCENT = tft.color565(255, 90, 145);
        COLOR_ACCENT_DIM = tft.color565(155, 45, 85);
    }
}
