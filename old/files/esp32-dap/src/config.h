#pragma once
#include <Arduino.h>

// ---- Pin untuk Lolin D32 (sesuai plan.md). Pin TFT/touch diatur di platformio.ini ----
// TFT CS = GPIO 17 (bukan 5, karena GPIO 5 = LED onboard + strapping pada D32)
#define PIN_I2S_BCK   26
#define PIN_I2S_LRC   25
#define PIN_I2S_DOUT  22
#define PIN_SD_CS     4
#define PIN_ENC_A     13
#define PIN_ENC_B     14
#define PIN_ENC_SW    16
#define PIN_BAT_ADC   35
// Tombol atas (aktif LOW ke GND). GPIO 36 dan 39 input-only: pasang resistor pull-up 10k ke 3V3.
#define PIN_BTN_PREV  32
#define PIN_BTN_NEXT  36
#define PIN_BTN_MENU  39
#define ENABLE_TOUCH  1        // 0 = matikan touch (tanpa kalibrasi)

#define ENC_REVERSE   0        // ganti 1 bila arah putar terbalik
#define BAT_DIV_RATIO 2.0f     // D32: pembagi 100k/100k sudah ada di board (GPIO 35, tidak di header)

#define MUSIC_DIR     "/Music"
#define MAX_TRACKS    400

// ---- Bluetooth ----
#define BT_RECEIVER_NAME "DAP-ESP32"            // nama yang muncul di HP
#define BT_TARGET_NAME   "ISI-NAMA-HEADSET-BT"  // nama headset tujuan (mode Transmitter)

// ---- WiFi (mode Transfer) ----
// Kosongkan SSID agar langsung memakai hotspot ESP32 (AP)
#define WIFI_STA_SSID ""
#define WIFI_STA_PASS ""
#define WIFI_AP_SSID  "DAP-ESP32"
#define WIFI_AP_PASS  "12345678"

enum Mode : uint8_t { MODE_LOCAL = 0, MODE_RECEIVER, MODE_TRANSMITTER, MODE_TRANSFER, MODE_COUNT };
