#pragma once
#include "Adafruit_GFX.h"
#include "layout.h"

#define ILI9341_BLACK 0x0000
#define ILI9341_WHITE 0xFFFF
#define ILI9341_RED 0xF800
#define ILI9341_LIGHTGREY 0xC618

class Adafruit_ILI9341 : public Adafruit_GFX {
public:
    Adafruit_ILI9341(int8_t, int8_t, int8_t) : Adafruit_GFX(SCREEN_W, SCREEN_H) {}
    void begin() {}
    void setRotation(uint8_t) {}
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }
};
