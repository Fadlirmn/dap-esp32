#include "gfx_helpers.h"
#include <math.h>

int gfxTextWidth(Adafruit_GFX &gfx, const String &text, uint8_t textSize) {
    gfx.setTextSize(textSize);
    int16_t x1, y1;
    uint16_t w, h;
    gfx.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return (int)w;
}

void gfxDrawString(Adafruit_GFX &gfx, const String &text, int x, int y, uint8_t textSize, GfxDatum datum) {
    gfx.setTextSize(textSize);
    int drawX = x;
    if (datum != GFX_TL) {
        int16_t x1, y1;
        uint16_t w, h;
        gfx.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        drawX = (datum == GFX_MC) ? x - (int)w / 2 : x - (int)w;
    }
    gfx.setCursor(drawX, y);
    gfx.print(text);
}

// No ellipse primitive in Adafruit_GFX: fill one horizontal scanline per row.
void gfxFillEllipse(Adafruit_GFX &gfx, int cx, int cy, int rx, int ry, uint16_t color) {
    if (rx <= 0 || ry <= 0) return;
    for (int dy = -ry; dy <= ry; dy++) {
        int dx = (int)((double)rx * sqrt(1.0 - ((double)dy * dy) / ((double)ry * ry)));
        gfx.drawFastHLine(cx - dx, cy + dy, 2 * dx + 1, color);
    }
}

String truncateToWidth(Adafruit_GFX &gfx, const String &text, int maxWidth, uint8_t tftFont) {
    uint8_t size = fontToSize(tftFont);
    if (gfxTextWidth(gfx, text, size) <= maxWidth) return text;
    String result = text;
    while (result.length() > 1 && gfxTextWidth(gfx, result + "...", size) > maxWidth) {
        result.remove(result.length() - 1);
    }
    return result + "...";
}
