// Color theme, ported verbatim from old/Stream32-ESP-Player.ino. tft.color565() works
// unchanged: Adafruit_ILI9341 inherits it from Adafruit_SPITFT, same call signature
// TFT_eSPI used.
#pragma once
#include <stdint.h>

extern uint16_t COLOR_BG, COLOR_PANEL, COLOR_PANEL_BORDER, COLOR_ACCENT, COLOR_ACCENT_DIM,
    COLOR_TEXT_PRIMARY, COLOR_TEXT_SECONDARY, COLOR_HEART_ACTIVE, COLOR_HEART_INACTIVE,
    COLOR_TRACK_BG, COLOR_BT_CONNECTED, COLOR_BT_DISCONNECTED;

// Status/semantic colors, fixed regardless of the user's accent theme choice
// (dap-ui-design's Material dark palette) -- used by the new mode-switch,
// BT Receiver and Transfer File screens.
extern uint16_t COLOR_BT_PANEL, COLOR_BT_PANEL_TEXT, COLOR_WARNING, COLOR_SUCCESS,
    COLOR_CHIP_BG, COLOR_CHIP_TEXT;

extern uint8_t themeIndex;

void initColorTheme();
void applyTheme(uint8_t theme);
