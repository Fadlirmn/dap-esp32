#pragma once
#include <TFT_eSPI.h>
#include <vector>
#include "common.h"

// Layar 320x240 (landscape). Header 0-26, body 26-214, footer 214-240.
#define RGB565(r,g,b) ((uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))
#define C_BG        RGB565(11,15,20)
#define C_PANEL     RGB565(24,31,40)
#define C_TEXT      RGB565(230,234,240)
#define C_DIM       RGB565(130,140,152)
#define C_ACCENT    RGB565(56,189,248)
#define C_ACCENT_DK RGB565(12,74,110)
#define C_GOOD      RGB565(74,222,128)
#define C_WARN      RGB565(250,204,21)
#define C_BAD       RGB565(248,113,113)

#define UI_LIST_Y 30
#define UI_ROW_H  26
#define UI_ROWS   7

extern TFT_eSPI tft;

void uiBegin();                              // init layar + kalibrasi touch (tahan tombol saat boot = kalibrasi ulang)
void uiSplash(const char *mode);
void uiHeader(const char *title);
void uiBatteryIcon();
void uiFooter(const char *text);
void uiMessage(const char *l1, const char *l2 = nullptr, uint16_t color = C_TEXT);
bool uiTouch(int16_t &x, int16_t &y);        // true sekali per ketukan

void uiList(const std::vector<String> &items, int sel, int top);

struct PlayerView {
  String title, sub;
  float  progress = -1;                      // 0..1, negatif = sembunyikan
  String tLeft, tRight;
  float  volume = 0;                         // 0..1
  bool   playing = false;
};
void uiPlayerFull(const char *header);       // gambar ulang semua + reset cache
void uiPlayer(const PlayerView &v);          // hanya menggambar bagian yang berubah
int  uiControlHit(int16_t x, int16_t y);     // 0 prev, 1 play/pause, 2 next, -1 di luar

// Menu mode. Return: -1 batal, 100 = "Daftar Lagu". Memilih mode -> simpan & restart.
int  uiMenu(bool withLibrary, void (*idle)());
