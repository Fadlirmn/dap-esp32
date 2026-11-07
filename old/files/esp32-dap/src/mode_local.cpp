// Mode 1: Local Player (SD -> PCM5102A -> IEM), pakai ESP32-audioI2S
#include <Arduino.h>
#include <SD.h>
#include <Audio.h>
#include <Preferences.h>
#include "modes.h"
#include "common.h"
#include "ui.h"

static Audio *audio = nullptr;
static SemaphoreHandle_t mux;
static std::vector<String> names;
static int sel = 0, top = 0, cur = -1;
static bool ready = false, inPlayer = false, paused = false;
static uint8_t vol = 12;                       // 0..21
static uint32_t startedAt = 0, drawAt = 0;

static void audioTask(void *) {
  for (;;) {
    xSemaphoreTake(mux, portMAX_DELAY);
    audio->loop();
    xSemaphoreGive(mux);
    vTaskDelay(1);
  }
}

static String fmt(uint32_t s) {
  char b[12];
  snprintf(b, sizeof(b), "%u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
  return String(b);
}

static void showList() {
  uiHeader("Local Player");
  uiList(names, sel, top);
  String f = String(sel + 1) + " / " + String(names.size()) + "   Klik: putar   MENU";
  uiFooter(f.c_str());
}

static void drawPlayer(bool full) {
  if (full) uiPlayerFull("Local Player");
  PlayerView v;
  v.title = cur >= 0 ? names[cur] : String("-");
  v.sub = String(cur + 1) + " / " + String(names.size());
  uint32_t t = 0, d = 0;
  xSemaphoreTake(mux, portMAX_DELAY);
  t = audio->getAudioCurrentTime();
  d = audio->getAudioFileDuration();
  xSemaphoreGive(mux);
  v.progress = d ? min(1.0f, (float)t / d) : 0;
  v.tLeft = fmt(t);
  v.tRight = d ? fmt(d) : String("--:--");
  v.volume = vol / 21.0f;
  v.playing = !paused;
  uiPlayer(v);
}

static void playIndex(int i) {
  int n = tracks.size();
  if (!n) return;
  cur = (i % n + n) % n;
  xSemaphoreTake(mux, portMAX_DELAY);
  audio->stopSong();
  audio->connecttoFS(SD, tracks[cur].c_str());
  xSemaphoreGive(mux);
  paused = false;
  startedAt = millis();
}

static void toggle() {
  xSemaphoreTake(mux, portMAX_DELAY);
  audio->pauseResume();
  xSemaphoreGive(mux);
  paused = !paused;
}

static uint32_t footerUntil = 0;
static void volFooter() {
  uiFooter((String("Volume ") + String(vol) + " / 21").c_str());
  footerUntil = millis() + 1500;
}

static void openMenu() {
  Preferences p; p.begin("dap", false);
  p.putUChar("vol", vol);
  p.end();
  int r = uiMenu(true, nullptr);               // audio tetap jalan di task sendiri
  if (r == 100) inPlayer = false;
  if (inPlayer) drawPlayer(true); else if (ready) showList();
  else uiMessage("Tidak ada lagu", "Pakai mode Transfer File");
}

void localSetup() {
  uiHeader("Local Player");
  if (!storageBegin()) {
    uiMessage("SD card tidak terbaca", "Cek kartu dan kabel", C_BAD);
    return;
  }
  scanTracks(false);
  if (tracks.empty()) {
    uiMessage("Tidak ada lagu", "Taruh di /Music atau pakai Transfer", C_WARN);
    return;
  }
  for (size_t i = 0; i < tracks.size(); i++) names.push_back(trackTitle(i));

  Preferences p; p.begin("dap", true);
  vol = p.getUChar("vol", 12);
  p.end();

  audio = new Audio();
  audio->setPinout(PIN_I2S_BCK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio->setVolume(vol);
  mux = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(audioTask, "audio", 8192, nullptr, 3, nullptr, 0);

  ready = true;
  showList();
}

void localLoop() {
  uint32_t now = millis();
  int d = encoderTake();
  BtnEvent b = buttonPoll();
  NavEvent nv = navPoll(!inPlayer);              // di daftar: tahan PREV/NEXT = scroll cepat
  int16_t tx = 0, ty = 0;
  bool tap = false;
  static uint32_t touchAt = 0;
  if (now - touchAt > 40) { touchAt = now; tap = uiTouch(tx, ty); }

  if (!ready) {
    if (nv == NAV_MENU || b == BTN_CLICK || b == BTN_LONG) openMenu();
    delay(10);
    return;
  }

  // Putar encoder = volume saja
  if (d) {
    vol = constrain((int)vol + d, 0, 21);
    xSemaphoreTake(mux, portMAX_DELAY);
    audio->setVolume(vol);
    xSemaphoreGive(mux);
    if (inPlayer) drawPlayer(false); else volFooter();
  }
  if (nv == NAV_MENU || b == BTN_LONG) { openMenu(); return; }

  if (!inPlayer) {
    // ----- Daftar lagu: PREV/NEXT = naik/turun, klik encoder = putar -----
    int n = names.size();
    int step = (nv == NAV_NEXT) ? 1 : (nv == NAV_PREV ? -1 : 0);
    if (step) {
      sel = constrain(sel + step, 0, n - 1);
      if (sel < top) top = sel;
      if (sel >= top + UI_ROWS) top = sel - UI_ROWS + 1;
      footerUntil = 0;
      showList();
    }
    if (tap && ty >= UI_LIST_Y && ty < UI_LIST_Y + UI_ROWS * UI_ROW_H) {
      int i = top + (ty - UI_LIST_Y) / UI_ROW_H;
      if (i < n) { sel = i; b = BTN_CLICK; }
    }
    if (b == BTN_CLICK) { playIndex(sel); inPlayer = true; drawPlayer(true); }
    if (footerUntil && now > footerUntil) { footerUntil = 0; showList(); }
  } else {
    // ----- Sedang memutar: PREV/NEXT = lagu sebelum/berikut, klik = play/pause -----
    int act = -1;                                // 0 prev, 1 toggle, 2 next
    if (tap) act = uiControlHit(tx, ty);
    if (nv == NAV_PREV) act = 0; else if (nv == NAV_NEXT) act = 2;
    if (b == BTN_CLICK) act = 1; else if (b == BTN_DOUBLE) act = 2;

    switch (act) {
      case 0: playIndex(cur - 1); break;
      case 1: toggle(); break;
      case 2: playIndex(cur + 1); break;
    }
    if (act >= 0) drawPlayer(false);
  }

  // lagu habis -> otomatis lanjut
  if (cur >= 0 && !paused && now - startedAt > 2000 && !audio->isRunning()) {
    playIndex(cur + 1);
    sel = cur;
    if (inPlayer) drawPlayer(false);
  }
  if (inPlayer && now - drawAt > 500) { drawAt = now; drawPlayer(false); }
  delay(2);
}
