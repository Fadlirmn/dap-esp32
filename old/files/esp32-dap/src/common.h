#pragma once
#include <Arduino.h>
#include <vector>
#include "config.h"

// ---- Baterai ----
void    batteryBegin();
void    batteryUpdate();          // panggil ~1x per detik
float   batteryVolts();
uint8_t batteryPercent();

// ---- Input: rotary encoder + tombol ----
enum BtnEvent : uint8_t { BTN_NONE, BTN_CLICK, BTN_DOUBLE, BTN_LONG };
// Tombol atas: PREV / NEXT / MENU
enum NavEvent : uint8_t { NAV_NONE, NAV_PREV, NAV_NEXT, NAV_MENU };
NavEvent navPoll(bool repeat);    // repeat=true: tahan PREV/NEXT mengulang (untuk scroll daftar)
void     inputBegin();
int      encoderTake();           // selisih detent sejak panggilan terakhir
BtnEvent buttonPoll();

// ---- Storage ----
extern std::vector<String> tracks;
bool   storageBegin();
void   scanTracks(bool mp3Only = false);
String trackTitle(int i);

// ---- Mode (disimpan di NVS, ganti mode = restart) ----
uint8_t     loadMode();
void        saveModeAndRestart(uint8_t m);
const char* modeName(uint8_t m);
