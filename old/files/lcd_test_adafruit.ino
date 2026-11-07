// Tes layar ILI9341 untuk simulator / Arduino: pakai Adafruit_ILI9341 (pin langsung di kode,
// tanpa User_Setup.h). Library: "Adafruit GFX Library", "Adafruit ILI9341" (+ Adafruit BusIO).
//
// Wiring: VCC->3V3, GND->GND, LED->3V3, CS->GPIO17 (TX2), RESET->GPIO33, DC->GPIO27,
//         MOSI->GPIO23, SCK->GPIO18. MISO tidak perlu disambung.

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS  17
#define TFT_DC  27
#define TFT_RST 33

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);   // SPI hardware: SCK 18, MOSI 23

void setup() {
  Serial.begin(115200);
  Serial.println("Mulai tes LCD");
  tft.begin();
  tft.setRotation(1);                            // landscape 320x240

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(5);
  tft.setCursor(80, 90);
  tft.print("TEST");
  Serial.println("Tulisan TEST digambar");
  delay(2000);
}

void loop() {
  uint16_t warna[] = {ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE, ILI9341_YELLOW, ILI9341_CYAN, ILI9341_MAGENTA};
  for (int i = 0; i < 6; i++) {
    tft.fillScreen(warna[i]);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(3);
    tft.setCursor(60, 105);
    tft.print("TEST LCD");
    delay(700);
  }
}
