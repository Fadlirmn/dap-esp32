// Adafruit_GFX has no setTextDatum/drawCentreString/drawRightString/textWidth/fillEllipse
// (those are TFT_eSPI-only). This is the minimal shim the ported Stream32 drawing code
// needs so the port stays a near-mechanical find/replace instead of a rewrite.
#pragma once
#include <Adafruit_GFX.h>
#include <Arduino.h>

enum GfxDatum : uint8_t { GFX_TL, GFX_MC, GFX_TR };  // y is always "top of text", like TFT_eSPI's drawString/drawCentreString/drawRightString

// Maps a TFT_eSPI GFX font id (1, 2, 4 — the only ones Stream32 used) to the closest
// Adafruit_GFX built-in-font textSize multiplier. Approximate: Adafruit only has one
// 6x8 font scaled by integer size, TFT_eSPI's font 2/4 are taller purpose-built glyphs.
// Upgrade path: swap in a custom GFXfont via setFont() if pixel-exact sizing matters.
inline uint8_t fontToSize(uint8_t tftFont) {
    if (tftFont >= 4) return 3;
    if (tftFont >= 2) return 2;
    return 1;
}

void gfxDrawString(Adafruit_GFX &gfx, const String &text, int x, int y, uint8_t textSize, GfxDatum datum = GFX_TL);
int gfxTextWidth(Adafruit_GFX &gfx, const String &text, uint8_t textSize);
void gfxFillEllipse(Adafruit_GFX &gfx, int cx, int cy, int rx, int ry, uint16_t color);

// Ported from Stream32's truncateToWidth(text, maxWidth, font) — same signature
// (tftFont, not textSize) so screens.cpp call sites stay a 1:1 port.
String truncateToWidth(Adafruit_GFX &gfx, const String &text, int maxWidth, uint8_t tftFont);
