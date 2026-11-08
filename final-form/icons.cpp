// Ported from old/Stream32-ESP-Player.ino lines 580-718. All shapes used
// (fillTriangle/fillRoundRect/fillCircle/drawLine/drawPixel) exist natively
// in Adafruit_GFX; only text alignment and ellipse fill go through gfx_helpers.
//
// Visual-fidelity pass vs. dap-ui-design/source/*.dc.html: the new icon
// functions below (drawHamburgerIcon onward) are transcribed from that
// mockup's 24x24-viewBox SVG `path` data, scaled to whatever pixel size the
// caller asks for, and drawn with Adafruit_GFX's line/circle primitives --
// not freehand shapes.
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

// Plain "<" chevron, dap-ui-design's `M15 5l-7 7 7 7`. `size` ~ half-height.
void drawBackIcon(int cx, int cy, int size, uint16_t color) {
    int dx = (size * 3) / 5;
    if (dx < 2) dx = 2;
    tft.drawLine(cx + dx, cy - size, cx - dx, cy, color);
    tft.drawLine(cx - dx, cy, cx + dx, cy + size, color);
}

// "Settings" glyph: gear (ring + 8 ticks).
void drawGearIcon(int cx, int cy, int size, uint16_t color) {
    tft.drawCircle(cx, cy, size / 2, color);
    for (int tick = 0; tick < 8; tick++) {
        float angle = tick * 0.7854f;
        tft.drawLine(cx + (int)(size * cos(angle)), cy + (int)(size * sin(angle)),
                     cx + (int)((size + 3) * cos(angle)), cy + (int)((size + 3) * sin(angle)), color);
    }
}

// "Playlists" glyph: stacked horizontal bars (a list of rows) with a small
// play-triangle badge at bottom-right, distinguishing it from drawListIcon's
// plain bars (no badge).
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

// Header battery glyph: outline + fill + terminal nub, fixed theme colors per
// dap-ui-design (outline/nub COLOR_TEXT_SECONDARY, fill COLOR_SUCCESS -- the
// previous port used COLOR_TEXT_PRIMARY for both and had no nub).
// Decorative only -- no ADC pin wired, see layout.h's DUMMY_BATTERY_PCT.
void drawBatteryIcon(int x, int y, int w, int h, uint8_t pct) {
    uint8_t p = pct > 100 ? 100 : pct;
    tft.drawRoundRect(x, y, w, h, 2, COLOR_TEXT_SECONDARY);
    int nubH = h / 2;
    tft.fillRoundRect(x + w, y + (h - nubH) / 2, 2, nubH, 1, COLOR_TEXT_SECONDARY);
    int innerW = w - 4;
    int fillW = (innerW * p) / 100;
    if (fillW > 0) tft.fillRect(x + 2, y + 2, fillW, h - 4, COLOR_SUCCESS);
}

// Low-battery dialog glyph (L_Overlays.dc.html): outline body + a single thin
// bar at the far left (not a pct fill like drawBatteryIcon -- this one always
// reads as "nearly empty", matching the mockup's literal SVG: `rect x=.5 y=.5
// w=18 h=11 rx=2` outline, `rect x=2 y=2 w=2 h=8 rx=1` bar, `rect x=19.5 y=3.5
// w=2 h=5 rx=1` nub, all stroked/filled in the caller's color (COLOR_ERROR)).
// x/y/w/h is the outline body's box; the nub extends past x+w like the mockup.
void drawLowBatteryIcon(int x, int y, int w, int h, uint16_t color) {
    tft.drawRoundRect(x, y, w, h, 2, color);
    tft.fillRoundRect(x + 2, y + 2, 2, h - 4, 1, color);
    int nubH = h / 2 + 1;
    tft.fillRoundRect(x + w, y + (h - nubH) / 2, 2, nubH, 1, color);
}

// Volume-overlay speaker glyph: body+horn fixed COLOR_TEXT_SECONDARY; a single
// outward sound-wave arc (approximated with 2 line segments -- Adafruit_GFX
// has no arc primitive) when unmuted, a COLOR_ERROR cross when muted.
void drawSpeakerIcon(int cx, int cy, int size, bool muted) {
    int bodyW = size / 2;
    tft.fillRect(cx - size, cy - bodyW / 2, bodyW, bodyW, COLOR_TEXT_SECONDARY);
    tft.fillTriangle(cx - size + bodyW, cy - size, cx - size + bodyW, cy + size, cx, cy, COLOR_TEXT_SECONDARY);
    if (muted) {
        tft.drawLine(cx + 3, cy - size, cx + size + 3, cy + size, COLOR_ERROR);
        tft.drawLine(cx + 3, cy + size, cx + size + 3, cy - size, COLOR_ERROR);
    } else {
        tft.drawLine(cx + size / 2 + 2, cy - size, cx + size + 4, cy, COLOR_TEXT_SECONDARY);
        tft.drawLine(cx + size + 4, cy, cx + size / 2 + 2, cy + size, COLOR_TEXT_SECONDARY);
    }
}

// MENU chip's hamburger: 3 equal bars (`M4 6h16 / M4 12h16 / M4 18h16`).
void drawHamburgerIcon(int cx, int cy, int size, uint16_t color) {
    for (int r = -1; r <= 1; r++) {
        tft.drawFastHLine(cx - size, cy + r * size, size * 2 + 1, color);
    }
}

// Header "list" identity icon: 3 bars, bottom one shorter (`M4 6h16 / M4 12h16 / M4 18h10`).
void drawListIcon(int cx, int cy, int size, uint16_t color) {
    int full = size * 2 + 1;
    int shortLen = (full * 6) / 10;
    tft.drawFastHLine(cx - size, cy - size, full, color);
    tft.drawFastHLine(cx - size, cy, full, color);
    tft.drawFastHLine(cx - size, cy + size, shortLen, color);
}

// Bluetooth identity glyph used by the NEW screens (mode-select rows, BT
// Receiver/Transmitter header + panel icons): dap-ui-design's zigzag path
// `M7 7l10 10-5 5V2l5 5L7 17`, NOT the same shape as drawBluetoothLogo's
// classic rune (that one stays as-is for screens that already used it).
// 24x24 viewBox, centered on (12,12); `size` ~ half the icon's height.
void drawBluetoothZigzag(int cx, int cy, int size, uint16_t color) {
    float s = size / 10.0f;
    auto X = [&](float vx) { return cx + (int)((vx - 12) * s); };
    auto Y = [&](float vy) { return cy + (int)((vy - 12) * s); };
    tft.drawLine(X(7), Y(7), X(17), Y(17), color);
    tft.drawLine(X(17), Y(17), X(12), Y(22), color);
    tft.drawLine(X(12), Y(22), X(12), Y(2), color);
    tft.drawLine(X(12), Y(2), X(17), Y(7), color);
    tft.drawLine(X(17), Y(7), X(7), Y(17), color);
}

// "SD source" glyph for the BT Transmitter panel + mode-select "Local Player"
// row (`M9 18V6l10-2v12` stem+beam, `M9 18a3 3 0 11-6 0...`/`M19 16a3 3...`
// note heads) -- two connected eighth-notes.
void drawMusicNoteGlyph(int cx, int cy, int size, uint16_t color) {
    float s = size / 10.0f;
    auto X = [&](float vx) { return cx + (int)((vx - 12) * s); };
    auto Y = [&](float vy) { return cy + (int)((vy - 12) * s); };
    tft.drawLine(X(9), Y(18), X(9), Y(6), color);
    tft.drawLine(X(9), Y(6), X(19), Y(4), color);
    tft.drawLine(X(19), Y(4), X(19), Y(16), color);
    int r = (int)(3 * s);
    if (r < 2) r = 2;
    tft.drawCircle(X(6), Y(18), r, color);
    tft.drawCircle(X(16), Y(16), r, color);
}

// Shuffle glyph (`M16 3h5v5 M4 20L21 3 M21 16v5h-5 M15 15l6 6 M4 4l5 5`):
// two crossing diagonal arrows, arrowhead corners approximated with straight
// lines (Adafruit_GFX has no arc primitive). Decorative only -- no
// shuffle-state exists anywhere in this codebase.
void drawShuffleIcon(int cx, int cy, int size, uint16_t color) {
    float s = size / 10.0f;
    auto X = [&](float vx) { return cx + (int)((vx - 12) * s); };
    auto Y = [&](float vy) { return cy + (int)((vy - 12) * s); };
    tft.drawLine(X(4), Y(4), X(9), Y(9), color);
    tft.drawLine(X(4), Y(20), X(21), Y(3), color);
    tft.drawLine(X(16), Y(3), X(21), Y(3), color);
    tft.drawLine(X(21), Y(3), X(21), Y(8), color);
    tft.drawLine(X(15), Y(15), X(21), Y(21), color);
    tft.drawLine(X(21), Y(16), X(21), Y(21), color);
    tft.drawLine(X(21), Y(21), X(16), Y(21), color);
}

// Repeat glyph (`M17 1l4 4-4 4 M3 11V9a4 4 0 014-4h14 M7 23l-4-4 4-4
// M21 13v2a4 4 0 01-4 4H3`): a loop top+bottom, each with an arrowhead;
// the two quarter-circle arcs are approximated with a single diagonal line
// (same convention drawUploadIcon's chevron uses). Decorative only -- no
// repeat-state exists anywhere in this codebase.
void drawRepeatIcon(int cx, int cy, int size, uint16_t color) {
    float s = size / 10.0f;
    auto X = [&](float vx) { return cx + (int)((vx - 12) * s); };
    auto Y = [&](float vy) { return cy + (int)((vy - 12) * s); };
    tft.drawLine(X(17), Y(1), X(21), Y(5), color);
    tft.drawLine(X(21), Y(5), X(17), Y(9), color);
    tft.drawLine(X(3), Y(11), X(3), Y(9), color);
    tft.drawLine(X(3), Y(9), X(7), Y(5), color);
    tft.drawLine(X(7), Y(5), X(21), Y(5), color);
    tft.drawLine(X(7), Y(23), X(3), Y(19), color);
    tft.drawLine(X(3), Y(19), X(7), Y(15), color);
    tft.drawLine(X(21), Y(13), X(21), Y(15), color);
    tft.drawLine(X(21), Y(15), X(17), Y(19), color);
    tft.drawLine(X(17), Y(19), X(3), Y(19), color);
}

// Upload/"transfer" glyph (`M12 16V4` + `M7 9l5-5 5 5` chevron + `M5 20h14`
// baseline). Used for Transfer File's header icon + its mode-select row icon.
void drawUploadIcon(int cx, int cy, int size, uint16_t color) {
    int top = cy - size, bottom = cy + size / 3;
    tft.drawLine(cx, bottom, cx, top, color);
    int headW = (size * 3) / 4;
    tft.drawLine(cx - headW, top + headW, cx, top, color);
    tft.drawLine(cx, top, cx + headW, top + headW, color);
    tft.drawFastHLine(cx - size, cy + size, size * 2 + 1, color);
}

// "Now playing" row glyph: 3 filled bars (short/tall/medium), replacing the
// row-index number for whichever list row is the currently-playing track.
// Static heights -- no animation possible usefully on this hardware.
void drawEqualizerBars(int cx, int cy, int w, int h, uint16_t color) {
    int barW = w / 5;
    if (barW < 2) barW = 2;
    int gap = barW;
    int heights[3] = {(h * 40) / 100, h, (h * 70) / 100};
    int totalW = barW * 3 + gap * 2;
    int x = cx - totalW / 2;
    for (int i = 0; i < 3; i++) {
        int bh = heights[i];
        if (bh < 2) bh = 2;
        tft.fillRoundRect(x, cy + h / 2 - bh, barW, bh, 1, color);
        x += barW + gap;
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

// Header: [optional small identity icon] title -- MENU chip -- battery.
// Replaces the old back-circle + bluetooth-status-icon layout: no screen in
// the dap-ui-design reference has a back circle in its header (going back is
// MENU-button-driven, see nav.cpp's handleMenuButton), and there is no
// bluetooth indicator in the header at all (BT screens show their own
// connection-status chip under the main panel instead, see screens.cpp).
void drawHeader(const char *title, uint8_t headerIcon, bool showMenuChip) {
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, COLOR_PANEL);
    tft.drawFastHLine(0, HEADER_H, SCREEN_W, COLOR_PANEL_BORDER);
    int midY = HEADER_H / 2;

    int textX = 10;
    if (headerIcon != HICON_NONE) {
        int iconCx = 16;
        switch (headerIcon) {
            case HICON_LIST: drawListIcon(iconCx, midY, 7, COLOR_TEXT_PRIMARY); break;
            case HICON_BACK: drawBackIcon(iconCx, midY, 7, COLOR_TEXT_PRIMARY); break;
            case HICON_BT: drawBluetoothZigzag(iconCx, midY, 7, COLOR_TEXT_PRIMARY); break;
            case HICON_TRANSFER: drawUploadIcon(iconCx, midY, 7, COLOR_TEXT_PRIMARY); break;
            case HICON_PLAYLIST: drawPlaylistIcon(iconCx, midY, 7, COLOR_TEXT_PRIMARY); break;
        }
        textX = iconCx + 12;
    }
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_PANEL);
    gfxDrawString(tft, title, textX, midY - 4, fontToSize(2), GFX_TL);

    // Battery, far right (L_*.dc.html's battery svg: 18x11 body + nub).
    int battW = 18, battH = 11;
    int battX = SCREEN_W - 10 - battW - 2;
    int battY = midY - battH / 2;
    String pctStr = String(DUMMY_BATTERY_PCT) + "%";
    int pctW = gfxTextWidth(tft, pctStr, 1);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_PANEL);
    gfxDrawString(tft, pctStr, battX - 6, midY - 3, 1, GFX_TR);
    drawBatteryIcon(battX, battY, battW, battH, DUMMY_BATTERY_PCT);

    // MENU chip, just left of the battery/% block -- skipped on L_Menu.dc.html's
    // own "Menu" screen (STATE_MODE_SELECT), which has no chip at all.
    if (!showMenuChip) return;
    const char *chipLabel = "MENU";
    int chipTextW = gfxTextWidth(tft, chipLabel, 1);
    int chipIconW = 12, chipH = 20;
    int chipW = 10 + chipIconW + 4 + chipTextW + 8;
    int chipX = battX - 6 - pctW - 10 - chipW;
    int chipY = midY - chipH / 2;
    tft.fillRoundRect(chipX, chipY, chipW, chipH, chipH / 2, COLOR_CHIP_BG);
    drawHamburgerIcon(chipX + 10, midY, 4, COLOR_CHIP_TEXT);
    tft.setTextColor(COLOR_CHIP_TEXT, COLOR_CHIP_BG);
    gfxDrawString(tft, chipLabel, chipX + 10 + chipIconW + 2, midY - 3, 1, GFX_TL);
}

// Thin bottom hint bar, per dap-ui-design's footer (layout.h's FOOTER_H).
void drawFooter(const String &hint) {
    int y = SCREEN_H - FOOTER_H;
    tft.fillRect(0, y, SCREEN_W, FOOTER_H, COLOR_PANEL);
    tft.drawFastHLine(0, y, SCREEN_W, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_PANEL);
    gfxDrawString(tft, hint, SCREEN_W / 2, y + (FOOTER_H - 8) / 2, 1, GFX_MC);
}

void drawFocusRing(int x, int y, int w, int h) {
    tft.drawRoundRect(x, y, w, h, 4, COLOR_ACCENT);
    tft.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 5, COLOR_ACCENT);
}
