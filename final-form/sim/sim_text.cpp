#include "sim_text.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cstdio>

static FT_Library ftLib = nullptr;
static FT_Face ftFace = nullptr;

bool simTextInit(SDL_Renderer *) {
    if (FT_Init_FreeType(&ftLib)) {
        fprintf(stderr, "simText: FT_Init_FreeType failed\n");
        return false;
    }
    static const char *candidates[] = {
        "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        nullptr,
    };
    for (int i = 0; candidates[i]; i++) {
        if (!FT_New_Face(ftLib, candidates[i], 0, &ftFace)) return true;
    }
    fprintf(stderr, "simText: no monospace font found\n");
    return false;
}

static void setSize(int pixelHeight) {
    FT_Set_Pixel_Sizes(ftFace, 0, pixelHeight);
}

void simTextMeasure(const char *s, int pixelHeight, int *outW, int *outH) {
    if (outW) *outW = 0;
    if (outH) *outH = pixelHeight;
    if (!ftFace || !s) return;
    setSize(pixelHeight);
    int w = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (FT_Load_Char(ftFace, *p, FT_LOAD_DEFAULT)) continue;
        w += ftFace->glyph->advance.x >> 6;
    }
    if (outW) *outW = w;
}

void simTextDraw(SDL_Renderer *ren, int x, int y, const char *s, int pixelHeight,
                  uint8_t r, uint8_t g, uint8_t b) {
    if (!ftFace || !s || !ren) return;
    setSize(pixelHeight);
    int baseline = y + (int)(ftFace->size->metrics.ascender >> 6);
    int penX = x;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (FT_Load_Char(ftFace, *p, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g_ = ftFace->glyph;
        FT_Bitmap *bmp = &g_->bitmap;
        int gx = penX + g_->bitmap_left;
        int gy = baseline - g_->bitmap_top;
        for (unsigned int row = 0; row < bmp->rows; row++) {
            for (unsigned int col = 0; col < bmp->width; col++) {
                uint8_t alpha = bmp->buffer[row * bmp->pitch + col];
                if (!alpha) continue;
                SDL_SetRenderDrawColor(ren, r, g, b, alpha);
                SDL_RenderDrawPoint(ren, gx + col, gy + row);
            }
        }
        penX += g_->advance.x >> 6;
    }
}
