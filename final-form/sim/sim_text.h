// FreeType glue so the Adafruit_GFX stub can render real glyphs instead of
// guessed bitmap font data.
#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

bool simTextInit(SDL_Renderer *ren);
void simTextMeasure(const char *s, int pixelHeight, int *outW, int *outH);
void simTextDraw(SDL_Renderer *ren, int x, int y, const char *s, int pixelHeight,
                  uint8_t r, uint8_t g, uint8_t b);
