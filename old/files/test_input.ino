// Tes ringkas: encoder (volume + klik) dan tombol PREV/NEXT/MENU, hasil tampil di LCD
// Library: Adafruit GFX Library, Adafruit ILI9341
// Layar: CS17 DC27 RST33 MOSI23 SCK18 (sama seperti tes sebelumnya)
// Encoder: CLK->13, DT->14, SW->16, +->3V3, GND. Tombol (ke GND): PREV->32, NEXT->36, MENU->39

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

Adafruit_ILI9341 tft(17, 27, 33);
const int PIN_A = 13, PIN_B = 14, PIN_SW = 16, BTN[3] = {32, 36, 39};
const char *NAMA[3] = {"PREV", "NEXT", "MENU"};

volatile int vol = 10, putar = 0;
int klik = 0, terakhir = -1;
bool lastSw = true, lastBtn[3] = {true, true, true};

void IRAM_ATTR encISR() {                    // arah putar dari kondisi pin B saat A berubah
  static uint32_t t = 0;
  if (millis() - t < 2) return;
  t = millis();
  if (digitalRead(PIN_A) == digitalRead(PIN_B)) { if (vol > 0)  vol--; }
  else                                          { if (vol < 21) vol++; }
  putar++;
}

void tampil() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_GREEN);  tft.setCursor(10, 20);  tft.printf("VOL: %d", vol);
  tft.setTextColor(ILI9341_YELLOW); tft.setCursor(10, 80);  tft.printf("KLIK: %d", klik);
  tft.setTextColor(ILI9341_CYAN);   tft.setCursor(10, 140);
  tft.printf("TOMBOL: %s", terakhir < 0 ? "-" : NAMA[terakhir]);
  Serial.printf("vol=%d klik=%d tombol=%s\n", vol, klik, terakhir < 0 ? "-" : NAMA[terakhir]);
}

void setup() {
  Serial.begin(115200);
  tft.begin(); tft.setRotation(1);
  pinMode(PIN_A, INPUT_PULLUP); pinMode(PIN_B, INPUT_PULLUP); pinMode(PIN_SW, INPUT_PULLUP);
  for (int i = 0; i < 3; i++) pinMode(BTN[i], INPUT_PULLUP);   // GPIO36/39 di hardware: tambah pull-up 10k ke 3V3
  attachInterrupt(digitalPinToInterrupt(PIN_A), encISR, FALLING);
  tampil();
}

void loop() {
  static int lastVol = -1;
  bool sw = digitalRead(PIN_SW);
  bool ubah = false;
  if (lastSw && !sw) { klik++; ubah = true; }
  lastSw = sw;
  for (int i = 0; i < 3; i++) {
    bool b = digitalRead(BTN[i]);
    if (lastBtn[i] && !b) { terakhir = i; ubah = true; }
    lastBtn[i] = b;
  }
  if (vol != lastVol) { lastVol = vol; ubah = true; }
  if (ubah) tampil();
  delay(5);
}
