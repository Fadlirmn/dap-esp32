// Color theme, ported verbatim from old/Stream32-ESP-Player.ino. tft.color565() works
// unchanged: Adafruit_ILI9341 inherits it from Adafruit_SPITFT, same call signature
// TFT_eSPI used.
#pragma once
#include <stdint.h>

extern uint16_t COLOR_BG, COLOR_PANEL, COLOR_PANEL_BORDER, COLOR_ACCENT, COLOR_ACCENT_DIM,
    COLOR_TEXT_PRIMARY, COLOR_TEXT_SECONDARY, COLOR_HEART_ACTIVE, COLOR_HEART_INACTIVE,
    COLOR_TRACK_BG, COLOR_BT_CONNECTED, COLOR_BT_DISCONNECTED;

extern uint8_t themeIndex;

void initColorTheme();
void applyTheme(uint8_t theme);
