// Ported from old/Stream32-ESP-Player.ino lines 580-718. All shapes used
// (fillTriangle/fillRoundRect/fillCircle/drawLine/drawPixel) exist natively
// in Adafruit_GFX; only text alignment and ellipse fill go through gfx_helpers.
#include "icons.h"
#include <Adafruit_ILI9341.h>
#include <math.h>
#include "theme.h"
#include "layout.h"
#include "gfx_helpers.h"
#include "bluetooth.h"

extern Adafruit_ILI9341 tft;

void drawPlayIcon(int cx, int cy, int size, uint16_t color) {
    tft.fillTriangle(cx - size / 2, cy - size, cx - size / 2, cy + size, cx + size, cy, color);
}

void drawPauseIcon(int cx, int cy, int size, uint16_t color) {
    int barW = size / 2;
    tft.fillRoundRect(cx - size, cy - size, barW, size * 2, 2, color);
    tft.fillRoundRect(cx + size - barW, cy - size, barW, size * 2, 2, color);
}

void drawPrevIcon(int cx, int cy, int size, uint16_t color) {
    tft.fillRoundRect(cx - size - 2, cy - size, 3, size * 2, 1, color);
    tft.fillTriangle(cx + size, cy - size, cx + size, cy + size, cx - size + 2, cy, color);
}

void drawNextIcon(int cx, int cy, int size, uint16_t color) {
    tft.fillRoundRect(cx + size - 1, cy - size, 3, size * 2, 1, color);
    tft.fillTriangle(cx - size, cy - size, cx - size, cy + size, cx + size - 2, cy, color);
}

void drawHeartIcon(int cx, int cy, int r, uint16_t color) {
    tft.fillCircle(cx - r / 2, cy - r / 3, r / 2, color);
    tft.fillCircle(cx + r / 2, cy - r / 3, r / 2, color);
    tft.fillTriangle(cx - r, cy - r / 4, cx + r, cy - r / 4, cx, cy + r, color);
}

void drawVectorMusicNote(int x, int y, int w, int h, uint16_t color) {
    gfxFillEllipse(tft, x + w / 4, y + h - h / 6, w / 4, h / 6, color);
    gfxFillEllipse(tft, x + w - w / 4, y + h - h / 4, w / 4, h / 6, color);
    tft.fillRect(x + w / 2, y, w / 10 + 2, h, color);
    tft.fillRect(x + w - w / 4 - w / 10, y, w / 10 + 2, h - h / 8, color);
    tft.fillRect(x + w / 2, y, w / 3, h / 6, color);
}

void drawBluetoothStatusIcon() {
    int cx = SCREEN_W - 20;
    int cy = HEADER_H / 2;
    uint16_t color = bt_connected ? COLOR_BT_CONNECTED : COLOR_BT_DISCONNECTED;
    drawBluetoothLogo(cx, cy, 8, color);
}

void drawBluetoothLogo(int cx, int cy, int size, uint16_t color) {
    int top = cy - size, bot = cy + size;
    tft.drawLine(cx, top, cx, bot, color);
    tft.drawLine(cx, top, cx + size, cy - size / 2, color);
    tft.drawLine(cx + size, cy - size / 2, cx - size, cy + size / 2, color);
    tft.drawLine(cx - size, cy + size / 2, cx, cy, color);
    tft.drawLine(cx, cy, cx - size, cy - size / 2, color);
    tft.drawLine(cx - size, cy - size / 2, cx + size, cy + size / 2, color);
    tft.drawLine(cx + size, cy + size / 2, cx, bot, color);
}

void drawBackIcon(int cx, int cy, uint16_t color) {
    tft.drawLine(cx + 4, cy - 6, cx - 4, cy, color);
    tft.drawLine(cx - 4, cy, cx + 4, cy + 6, color);
}

// "Tracks" glyph: 3 note-stems + dots, hoisted out of drawMainMenu's inline
// code so the main-menu grid and any future screen can both use it.
void drawLibraryIcon(int cx, int cy, int size, uint16_t color) {
    for (int r = -1; r <= 1; r++) {
        tft.drawFastHLine(cx - 2, cy + r * (size / 2), size + 3, color);
        tft.fillCircle(cx - size, cy + r * (size / 2), 2, color);
    }
}

// "Settings" glyph: gear (ring + 8 ticks), hoisted out of drawMainMenu's
// inline code for the same reason as drawLibraryIcon above.
void drawGearIcon(int cx, int cy, int size, uint16_t color) {
    tft.drawCircle(cx, cy, size / 2, color);
    for (int tick = 0; tick < 8; tick++) {
        float angle = tick * 0.7854f;
        tft.drawLine(cx + (int)(size * cos(angle)), cy + (int)(size * sin(angle)),
                     cx + (int)((size + 3) * cos(angle)), cy + (int)((size + 3) * sin(angle)), color);
    }
}

// "Playlists" glyph: stacked horizontal bars (a list of rows) with a small
// play-triangle badge at bottom-right, distinguishing it from drawLibraryIcon's
// note-stems-and-dots.
void drawPlaylistIcon(int cx, int cy, int size, uint16_t color) {
    int barLen = size * 2 - 4;
    tft.drawFastHLine(cx - size, cy - size + 2, barLen, color);
    tft.drawFastHLine(cx - size, cy - size / 4, barLen - 3, color);
    tft.drawFastHLine(cx - size, cy + size / 2, barLen - 6, color);
    tft.fillTriangle(cx + size / 2 - 2, cy + size - 6, cx + size / 2 - 2, cy + size + 2, cx + size + 2, cy + size - 2, color);
}

// "Mode-switch" glyph: two opposing arrows joined into a loop (the usual
// curved-arrow swap icon, approximated with straight lines + triangle heads --
// Adafruit_GFX has no arc/bezier primitive).
void drawSwapIcon(int cx, int cy, int size, uint16_t color) {
    int topY = cy - size / 2, botY = cy + size / 2;
    int left = cx - size, right = cx + size;
    tft.drawLine(left, topY, right - 4, topY, color);
    tft.fillTriangle(right - 6, topY - 4, right - 6, topY + 4, right, topY, color);
    tft.drawLine(right, botY, left + 4, botY, color);
    tft.fillTriangle(left + 6, botY - 4, left + 6, botY + 4, left, botY, color);
    tft.drawLine(left, topY, left, botY, color);
    tft.drawLine(right, topY, right, botY, color);
}

// Header dummy-battery glyph: outline + tip, filled proportional to pct.
// Decorative only -- no ADC pin wired, see layout.h's DUMMY_BATTERY_PCT.
void drawBatteryIcon(int x, int y, int w, int h, uint8_t pct, uint16_t color) {
    uint8_t p = pct > 100 ? 100 : pct;
    tft.drawRoundRect(x, y, w, h, 2, color);
    int tipH = h / 2;
    tft.fillRect(x + w, y + (h - tipH) / 2, 2, tipH, color);
    int innerW = w - 4;
    int fillW = (innerW * p) / 100;
    if (fillW > 0) tft.fillRect(x + 2, y + 2, fillW, h - 4, color);
}

// Volume-overlay speaker glyph: body + cone, sound-wave rings when unmuted,
// a cross when `muted` (caller passes bt_volume==0, with a warning color).
void drawSpeakerIcon(int cx, int cy, int size, uint16_t color, bool muted) {
    int bodyW = size / 2;
    tft.fillRect(cx - size, cy - bodyW / 2, bodyW, bodyW, color);
    tft.fillTriangle(cx - size + bodyW, cy - size, cx - size + bodyW, cy + size, cx, cy, color);
    if (muted) {
        tft.drawLine(cx + 2, cy - size, cx + size, cy + size, color);
        tft.drawLine(cx + 2, cy + size, cx + size, cy - size, color);
    } else {
        tft.drawCircle(cx + size / 2, cy, size / 2, color);
        tft.drawCircle(cx + size, cy, size, color);
    }
}

void maskCoverCorners(int x, int y, int w, int h, int radius) {
    for (int dy = 0; dy < radius; dy++) {
        for (int dx = 0; dx < radius; dx++) {
            if ((dx - radius) * (dx - radius) + (dy - radius) * (dy - radius) > radius * radius) {
                tft.drawPixel(x + dx, y + dy, COLOR_BG);
                tft.drawPixel(x + w - 1 - dx, y + dy, COLOR_BG);
                tft.drawPixel(x + dx, y + h - 1 - dy, COLOR_BG);
                tft.drawPixel(x + w - 1 - dx, y + h - 1 - dy, COLOR_BG);
            }
        }
    }
}

void drawHeader(const char *title, bool hasBackButton) {
    (void)title;  // original never renders this param either -- quirk preserved, not a port bug
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, COLOR_PANEL);
    tft.drawFastHLine(0, HEADER_H, SCREEN_W, COLOR_PANEL_BORDER);
    if (hasBackButton) {
        tft.fillCircle(20, HEADER_H / 2, 12, COLOR_TRACK_BG);
        drawBackIcon(20, HEADER_H / 2, COLOR_TEXT_PRIMARY);
    }
    // Dummy battery glyph + %, past the back button (no ADC wired, see layout.h).
    int battX = 40, battY = (HEADER_H - 10) / 2, battW = 18, battH = 10;
    drawBatteryIcon(battX, battY, battW, battH, DUMMY_BATTERY_PCT, COLOR_TEXT_PRIMARY);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_PANEL);
    gfxDrawString(tft, String(DUMMY_BATTERY_PCT) + "%", battX + battW + 6, battY - 1, 1, GFX_TL);
    drawBluetoothStatusIcon();
}

void drawFocusRing(int x, int y, int w, int h) {
    tft.drawRoundRect(x, y, w, h, 4, COLOR_ACCENT);
    tft.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 5, COLOR_ACCENT);
}

void drawFocusRingCircle(int cx, int cy, int r) {
    tft.drawCircle(cx, cy, r, COLOR_ACCENT);
    tft.drawCircle(cx, cy, r + 1, COLOR_ACCENT);
}
