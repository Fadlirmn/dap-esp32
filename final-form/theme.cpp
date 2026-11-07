#include "theme.h"
#include <Adafruit_ILI9341.h>

extern Adafruit_ILI9341 tft;

uint16_t COLOR_BG, COLOR_PANEL, COLOR_PANEL_BORDER, COLOR_ACCENT, COLOR_ACCENT_DIM,
    COLOR_TEXT_PRIMARY, COLOR_TEXT_SECONDARY, COLOR_HEART_ACTIVE, COLOR_HEART_INACTIVE,
    COLOR_TRACK_BG, COLOR_BT_CONNECTED, COLOR_BT_DISCONNECTED;
uint8_t themeIndex = 0;

void initColorTheme() {
    COLOR_BG              = tft.color565(16, 18, 22);
    COLOR_PANEL           = tft.color565(28, 31, 37);
    COLOR_PANEL_BORDER    = tft.color565(45, 50, 58);
    COLOR_ACCENT          = tft.color565(0, 224, 193);
    COLOR_ACCENT_DIM      = tft.color565(0, 120, 105);
    COLOR_TEXT_PRIMARY    = tft.color565(235, 238, 240);
    COLOR_TEXT_SECONDARY  = tft.color565(140, 146, 156);
    COLOR_HEART_ACTIVE    = tft.color565(255, 82, 110);
    COLOR_HEART_INACTIVE  = tft.color565(78, 83, 92);
    COLOR_TRACK_BG        = tft.color565(40, 44, 51);
    COLOR_BT_CONNECTED    = tft.color565(52, 145, 255);
    COLOR_BT_DISCONNECTED = tft.color565(235, 78, 78);
}

void applyTheme(uint8_t theme) {
    themeIndex = theme % 5;
    if (themeIndex == 0) {
        COLOR_ACCENT = tft.color565(112, 226, 125);
        COLOR_ACCENT_DIM = tft.color565(55, 130, 70);
    } else if (themeIndex == 1) {
        COLOR_ACCENT = tft.color565(78, 150, 255);
        COLOR_ACCENT_DIM = tft.color565(45, 88, 160);
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
