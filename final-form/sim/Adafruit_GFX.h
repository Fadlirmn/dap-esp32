// SDL-backed stand-in for Adafruit_GFX: just the drawing primitives and text
// API that theme.cpp/gfx_helpers.cpp/icons.cpp/screens.cpp actually call.
#pragma once
#include "Arduino.h"
#include "sim_text.h"
#include <SDL2/SDL.h>
#include <cstdint>
#include <cmath>

class Adafruit_GFX {
public:
    inline static SDL_Renderer *renderer = nullptr;

    Adafruit_GFX(int16_t w, int16_t h) : w_(w), h_(h) {}

    int16_t width() const { return w_; }
    int16_t height() const { return h_; }

    static void unpack565(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
        r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
        g = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
        b = (uint8_t)((c & 0x1F) * 255 / 31);
    }
    static void setColor(uint16_t c) {
        uint8_t r, g, b;
        unpack565(c, r, g, b);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if (!renderer) return;
        setColor(color);
        SDL_RenderDrawPoint(renderer, x, y);
    }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (!renderer) return;
        setColor(color);
        SDL_Rect r{x, y, w, h};
        SDL_RenderFillRect(renderer, &r);
    }
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (!renderer) return;
        setColor(color);
        SDL_Rect r{x, y, w, h};
        SDL_RenderDrawRect(renderer, &r);
    }
    void fillScreen(uint16_t color) { fillRect(0, 0, w_, h_, color); }
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        if (!renderer) return;
        setColor(color);
        SDL_RenderDrawLine(renderer, x, y, x + w - 1, y);
    }
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        if (!renderer) return;
        setColor(color);
        SDL_RenderDrawLine(renderer, x, y, x, y + h - 1);
    }
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        if (!renderer) return;
        setColor(color);
        SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
    }
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
        for (double a = 0; a < 2 * M_PI; a += 0.02)
            drawPixel((int16_t)(x0 + r * cos(a)), (int16_t)(y0 + r * sin(a)), color);
    }
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
        for (int16_t dy = -r; dy <= r; dy++) {
            int16_t dx = (int16_t)sqrt((double)r * r - (double)dy * dy);
            drawFastHLine((int16_t)(x0 - dx), (int16_t)(y0 + dy), (int16_t)(2 * dx + 1), color);
        }
    }
    // ponytail: true corner arcs skipped, drawn as plain rects — add arc
    // quadrants (classic Adafruit_GFX algorithm) if pixel-exact corners matter.
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t, uint16_t color) {
        drawRect(x, y, w, h, color);
    }
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t, uint16_t color) {
        fillRect(x, y, w, h, color);
    }
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t x2, int16_t y2, uint16_t color) {
        int16_t a, b, y, last;
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y1 > y2) { std::swap(y2, y1); std::swap(x2, x1); }
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y0 == y2) {
            a = b = x0;
            if (x1 < a) a = x1; else if (x1 > b) b = x1;
            if (x2 < a) a = x2; else if (x2 > b) b = x2;
            drawFastHLine(a, y0, b - a + 1, color);
            return;
        }
        int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
                dx12 = x2 - x1, dy12 = y2 - y1;
        int32_t sa = 0, sb = 0;
        last = (y1 == y2) ? y1 : y1 - 1;
        for (y = y0; y <= last; y++) {
            a = x0 + sa / dy01;
            b = x0 + sb / dy02;
            sa += dx01; sb += dx02;
            if (a > b) std::swap(a, b);
            drawFastHLine(a, y, b - a + 1, color);
        }
        sa = (int32_t)dx12 * (y - y1);
        sb = (int32_t)dx02 * (y - y0);
        for (; y <= y2; y++) {
            a = x1 + sa / dy12;
            b = x0 + sb / dy02;
            sa += dx12; sb += dx02;
            if (a > b) std::swap(a, b);
            drawFastHLine(a, y, b - a + 1, color);
        }
    }
    void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
        for (int16_t j = 0; j < h; j++)
            for (int16_t i = 0; i < w; i++)
                drawPixel(x + i, y + j, bitmap[j * w + i]);
    }

    void setTextSize(uint8_t s) { textSize_ = s ? s : 1; }
    void setCursor(int16_t x, int16_t y) { cursorX_ = x; cursorY_ = y; }
    void setTextColor(uint16_t fg) { textColor_ = fg; }
    void setTextColor(uint16_t fg, uint16_t) { textColor_ = fg; }

    void getTextBounds(const String &s, int16_t x, int16_t y, int16_t *x1, int16_t *y1,
                        uint16_t *w, uint16_t *h) {
        int pw = 0, ph = 0;
        simTextMeasure(s.c_str(), glyphHeight(), &pw, &ph);
        if (x1) *x1 = x;
        if (y1) *y1 = y;
        if (w) *w = (uint16_t)pw;
        if (h) *h = (uint16_t)cellHeight();
    }
    void print(const String &s) {
        uint8_t r, g, b;
        unpack565(textColor_, r, g, b);
        simTextDraw(renderer, cursorX_, cursorY_, s.c_str(), glyphHeight(), r, g, b);
    }

protected:
    int16_t w_, h_;

private:
    int glyphHeight() const { return 7 * textSize_; }
    int cellHeight() const { return 8 * textSize_; }

    uint8_t textSize_ = 1;
    int16_t cursorX_ = 0, cursorY_ = 0;
    uint16_t textColor_ = 0xFFFF;
};
