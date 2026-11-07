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

void drawNavIcon(uint8_t index, int cx, int cy, uint16_t color) {
    switch (index) {
        case 0:  // Home
            tft.drawLine(cx - 6, cy, cx, cy - 6, color);
            tft.drawLine(cx, cy - 6, cx + 6, cy, color);
            tft.drawRect(cx - 4, cy, 8, 6, color);
            break;
        case 1:  // Library
            tft.fillRect(cx - 6, cy - 6, 3, 12, color);
            tft.fillRect(cx - 1, cy - 6, 3, 12, color);
            tft.fillRect(cx + 4, cy - 6, 3, 12, color);
            break;
        case 2:  // Likes
            drawHeartIcon(cx, cy, 6, color);
            break;
        case 3:  // Bluetooth
            drawBluetoothLogo(cx, cy, 6, color);
            break;
        case 4:  // Settings
            for (int a = 0; a < 360; a += 45) {
                float rad = a * M_PI / 180.0;
                tft.drawLine(cx, cy, cx + cos(rad) * 8, cy + sin(rad) * 8, color);
            }
            tft.fillCircle(cx, cy, 3, color);
            break;
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
    drawBluetoothStatusIcon();
}

void drawBottomNavigation(uint8_t activeTab) {
    static const char *labels[5] = {"Home", "Library", "Likes", "Bluetooth", "Settings"};
    tft.fillRect(0, NAV_Y, SCREEN_W, NAV_H, COLOR_PANEL);
    tft.drawFastHLine(0, NAV_Y, SCREEN_W, COLOR_PANEL_BORDER);
    int centers[5];
    for (int i = 0; i < 5; i++) centers[i] = (SCREEN_W / 5) * i + (SCREEN_W / 10);
    for (int i = 0; i < 5; i++) {
        uint16_t color = (i == activeTab) ? COLOR_ACCENT : COLOR_TEXT_SECONDARY;
        drawNavIcon(i, centers[i], NAV_Y + 12, color);
        tft.setTextColor(color, COLOR_PANEL);
        gfxDrawString(tft, labels[i], centers[i], NAV_Y + 22, 1, GFX_MC);
    }
}

void drawFocusRing(int x, int y, int w, int h) {
    tft.drawRoundRect(x, y, w, h, 4, COLOR_ACCENT);
    tft.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 5, COLOR_ACCENT);
}
