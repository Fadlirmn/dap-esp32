// Tes layar ILI9341: tulisan "TEST" + pola warna (tanpa touch)
// Konfigurasi pin TFT_eSPI sama dengan lcd_test.ino (CS 17, DC 27, RST 33, MOSI 23, SCLK 18)

#include <TFT_eSPI.h>

TFT_eSPI tft;

void layarWarna(uint16_t warna, const char *nama) {
  tft.fillScreen(warna);
  tft.setTextColor(TFT_WHITE, warna);
  tft.drawCentreString(nama, 160, 110, 4);
  delay(800);
}

void setup() {
  tft.init();
  tft.setRotation(1);                       // landscape 320x240
}

void loop() {
  // 1. Tulisan TEST besar
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawCentreString("TEST", 160, 80, 4);
  tft.setTextSize(1);
  tft.drawCentreString("LCD 320 x 240", 160, 170, 2);
  delay(2000);

  // 2. Isi layar satu warna (cek piksel mati dan urutan warna)
  layarWarna(TFT_RED,   "MERAH");
  layarWarna(TFT_GREEN, "HIJAU");
  layarWarna(TFT_BLUE,  "BIRU");
  layarWarna(TFT_WHITE, "PUTIH");
  layarWarna(TFT_BLACK, "HITAM");

  // 3. Batang warna dan bingkai tepi (cek seluruh area layar)
  uint16_t warna[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_WHITE, TFT_ORANGE};
  for (int i = 0; i < 8; i++) tft.fillRect(i * 40, 0, 40, 240, warna[i]);
  tft.drawRect(0, 0, 320, 240, TFT_BLACK);
  tft.drawRect(1, 1, 318, 238, TFT_BLACK);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString(" TEST LCD OK ", 160, 110, 4);
  delay(3000);
}
