// Tes layar ILI9341 2.8" + touch di Lolin D32 (library: TFT_eSPI)
//
// Konfigurasi TFT_eSPI (Arduino IDE: edit User_Setup.h; PlatformIO: jadikan build_flags -D...)
//   #define ILI9341_DRIVER
//   #define TFT_MISO 19
//   #define TFT_MOSI 23
//   #define TFT_SCLK 18
//   #define TFT_CS   17
//   #define TFT_DC   27
//   #define TFT_RST  33
//   #define TOUCH_CS 21
//   #define LOAD_GLCD
//   #define LOAD_FONT2
//   #define LOAD_FONT4
//   #define SPI_FREQUENCY       40000000
//   #define SPI_TOUCH_FREQUENCY  2500000
//
// Board: ESP32 Dev Module

#include <TFT_eSPI.h>

TFT_eSPI tft;
uint16_t cal[5];

void drawUI() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Layar OK", 160, 40, 4);
  tft.fillRect(20, 90, 80, 40, TFT_RED);
  tft.fillRect(120, 90, 80, 40, TFT_GREEN);
  tft.fillRect(220, 90, 80, 40, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Sentuh layar: titik kuning muncul", 160, 160, 2);
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);                       // landscape 320x240

  drawUI();                                 // tes tampilan dulu (sebelum kalibrasi)
  delay(2000);

  // Kalibrasi touch (tekan 4 sudut). calibrateTouch menghapus layar, jadi UI digambar ulang sesudahnya.
  // Hapus bagian ini bila tidak memakai touch.
  tft.calibrateTouch(cal, TFT_YELLOW, TFT_BLACK, 15);
  tft.setTouch(cal);
  drawUI();
}

void loop() {
  static uint32_t n = 0;
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    tft.fillCircle(x, y, 3, TFT_YELLOW);    // titik di tempat disentuh
    Serial.printf("touch %u,%u\n", x, y);
  }
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Detik: " + String(millis() / 1000) + "   ", 20, 200, 2);
  delay(10);
}
