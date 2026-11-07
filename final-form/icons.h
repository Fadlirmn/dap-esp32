// Icon/chrome drawing, ported from old/Stream32-ESP-Player.ino lines 580-718.
// Uses the global `extern Adafruit_ILI9341 tft` (matches the original's own
// global-tft convention) instead of passing a GFX reference around.
#pragma once
#include <stdint.h>

void drawPlayIcon(int cx, int cy, int size, uint16_t color);
void drawPauseIcon(int cx, int cy, int size, uint16_t color);
void drawPrevIcon(int cx, int cy, int size, uint16_t color);
void drawNextIcon(int cx, int cy, int size, uint16_t color);
void drawHeartIcon(int cx, int cy, int r, uint16_t color);
void drawVectorMusicNote(int x, int y, int w, int h, uint16_t color);
void drawBluetoothStatusIcon();
void drawBluetoothLogo(int cx, int cy, int size, uint16_t color);
void drawBackIcon(int cx, int cy, uint16_t color);
void drawNavIcon(uint8_t index, int cx, int cy, uint16_t color);
void maskCoverCorners(int x, int y, int w, int h, int radius);

void drawHeader(const char *title, bool hasBackButton = true);
void drawBottomNavigation(uint8_t activeTab);

// NEW: accent-colored outline marking the cursor-focused item (encoder/button nav
// has no finger to tap, so every selectable item needs a visible "this one" marker).
void drawFocusRing(int x, int y, int w, int h);
