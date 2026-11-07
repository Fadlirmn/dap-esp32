// Tes touch XPT2046 + layar ILI9341
// Library: Adafruit GFX Library, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen)
// Wiring sama seperti tes layar; touch: T_CS->GPIO21, T_CLK->GPIO18, T_DIN->GPIO23, T_DO->GPIO19.
// T_IRQ tidak dipakai (touch di-polling).

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

#define TFT_CS    17
#define TFT_DC    27
#define TFT_RST   33
#define TOUCH_CS  21

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);            // tanpa pin IRQ -> polling

// Kalibrasi kasar (nilai mentah XPT2046). Sentuh tiap sudut, lihat angka mentah di layar/serial, lalu ubah.
int RAW_X_MIN = 200, RAW_X_MAX = 3800;
int RAW_Y_MIN = 200, RAW_Y_MAX = 3800;
bool SWAP_XY = false;                        // ganti true bila gerak horizontal jadi vertikal
bool FLIP_X  = false;                        // ganti true bila arah kiri-kanan terbalik
bool FLIP_Y  = false;                        // ganti true bila arah atas-bawah terbalik

void gambarAwal() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 8);
  tft.print("Sentuh layar");
  tft.fillRect(240, 0, 80, 30, ILI9341_RED);          // tombol HAPUS
  tft.setCursor(250, 8);
  tft.print("HAPUS");
  tft.drawRect(0, 0, 320, 240, ILI9341_GREEN);        // bingkai tepi
}

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);                        // landscape 320x240
  ts.begin();
  gambarAwal();
  Serial.println("Tes touch siap");
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int rx = p.x, ry = p.y;
    if (SWAP_XY) { int t = rx; rx = ry; ry = t; }
    int x = map(rx, RAW_X_MIN, RAW_X_MAX, 0, 319);
    int y = map(ry, RAW_Y_MIN, RAW_Y_MAX, 0, 239);
    if (FLIP_X) x = 319 - x;
    if (FLIP_Y) y = 239 - y;
    x = constrain(x, 0, 319);
    y = constrain(y, 0, 239);

    Serial.printf("mentah %d,%d z=%d -> layar %d,%d\n", p.x, p.y, p.z, x, y);

    if (x > 240 && y < 30) {                 // sentuh tombol HAPUS
      gambarAwal();
    } else {
      tft.fillCircle(x, y, 4, ILI9341_YELLOW);
      tft.fillRect(0, 216, 320, 22, ILI9341_BLACK);
      tft.setTextColor(ILI9341_CYAN);
      tft.setTextSize(2);
      tft.setCursor(8, 218);
      tft.printf("%d,%d  mentah %d,%d", x, y, p.x, p.y);
    }
  }
  delay(10);
}
