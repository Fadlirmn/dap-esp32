#include "ui.h"
#include <Preferences.h>

TFT_eSPI tft;
static uint16_t calData[5];

static String clip(const String &s, int n) {
  return (int)s.length() <= n ? s : s.substring(0, n - 2) + "..";
}

void uiBegin() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(C_BG);

  Preferences p; p.begin("dap", false);
  bool have = p.getBytes("touch", calData, sizeof(calData)) == sizeof(calData);
  if (ENABLE_TOUCH && (!have || digitalRead(PIN_ENC_SW) == LOW)) {
    tft.setTextColor(C_TEXT, C_BG);
    tft.drawCentreString("Kalibrasi layar", 160, 96, 4);
    tft.drawCentreString("Tekan tiap penanda di sudut", 160, 134, 2);
    tft.calibrateTouch(calData, C_ACCENT, C_BG, 15);
    p.putBytes("touch", calData, sizeof(calData));
  }
  p.end();
  if (ENABLE_TOUCH) tft.setTouch(calData);
}

void uiSplash(const char *mode) {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_ACCENT, C_BG);
  tft.drawCentreString("DAP ESP32", 160, 90, 4);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawCentreString(mode, 160, 128, 2);
}

void uiBatteryIcon() {
  tft.fillRect(224, 0, 96, 26, C_PANEL);
  if (batteryVolts() < 2.5f) {                 // tidak ada baterai (daya dari USB)
    tft.setTextColor(C_DIM, C_PANEL);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("USB", 310, 5, 2);
    tft.setTextDatum(TL_DATUM);
    return;
  }
  uint8_t pct = batteryPercent();
  uint16_t col = pct > 40 ? C_GOOD : (pct > 15 ? C_WARN : C_BAD);
  int x = 284, y = 7;
  tft.drawRect(x, y, 24, 12, C_TEXT);
  tft.fillRect(x + 24, y + 3, 2, 6, C_TEXT);
  tft.fillRect(x + 2, y + 2, (20 * pct) / 100, 8, col);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(String(pct) + "%", 280, 5, 2);
  tft.setTextDatum(TL_DATUM);
}

void uiHeader(const char *title) {
  tft.fillRect(0, 0, 320, 26, C_PANEL);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(title, 10, 5, 2);
  uiBatteryIcon();
}

void uiFooter(const char *text) {
  tft.fillRect(0, 214, 320, 26, C_PANEL);
  tft.setTextColor(C_DIM, C_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, 160, 227, 2);
  tft.setTextDatum(TL_DATUM);
}

void uiMessage(const char *l1, const char *l2, uint16_t color) {
  tft.fillRect(0, 26, 320, 188, C_BG);
  tft.setTextColor(color, C_BG);
  tft.drawCentreString(l1, 160, l2 ? 94 : 108, 4);
  if (l2) {
    tft.setTextColor(C_DIM, C_BG);
    tft.drawCentreString(l2, 160, 132, 2);
  }
}

bool uiTouch(int16_t &x, int16_t &y) {
  if (!ENABLE_TOUCH) return false;
  static bool was = false;
  static uint32_t last = 0;
  uint16_t tx, ty;
  bool t = tft.getTouch(&tx, &ty);
  if (t && !was && millis() - last > 150) {
    was = true; last = millis();
    x = tx; y = ty;
    return true;
  }
  if (!t) was = false;
  return false;
}

// ---------- Daftar ----------
void uiList(const std::vector<String> &items, int sel, int top) {
  tft.setTextDatum(ML_DATUM);
  for (int r = 0; r < UI_ROWS; r++) {
    int i = top + r, y = UI_LIST_Y + r * UI_ROW_H;
    bool s = (i == sel);
    uint16_t bg = s ? C_ACCENT_DK : C_BG;
    tft.fillRect(0, y, 320, UI_ROW_H, bg);
    if (i < (int)items.size()) {
      tft.setTextColor(s ? C_TEXT : C_DIM, bg);
      tft.drawString(clip(items[i], 32), 12, y + UI_ROW_H / 2, 2);
    }
  }
  tft.setTextDatum(TL_DATUM);
}

// ---------- Player ----------
static PlayerView prev;
static bool prevValid = false;

static void drawControls(bool playing) {
  const int y = 176, h = 44, cy = y + h / 2;
  tft.fillRoundRect(16, y, 80, h, 8, C_PANEL);
  tft.fillRoundRect(112, y, 96, h, 8, C_ACCENT_DK);
  tft.fillRoundRect(224, y, 80, h, 8, C_PANEL);
  // prev
  tft.fillRect(44, cy - 9, 3, 18, C_TEXT);
  tft.fillTriangle(70, cy - 9, 70, cy + 9, 50, cy, C_TEXT);
  // next
  tft.fillTriangle(251, cy - 9, 251, cy + 9, 271, cy, C_TEXT);
  tft.fillRect(274, cy - 9, 3, 18, C_TEXT);
  // play / pause
  if (playing) {
    tft.fillRect(148, cy - 10, 8, 20, C_TEXT);
    tft.fillRect(164, cy - 10, 8, 20, C_TEXT);
  } else {
    tft.fillTriangle(152, cy - 11, 152, cy + 11, 174, cy, C_TEXT);
  }
}

int uiControlHit(int16_t x, int16_t y) {
  if (y < 176 || y > 220) return -1;
  if (x >= 16 && x <= 96) return 0;
  if (x >= 112 && x <= 208) return 1;
  if (x >= 224 && x <= 304) return 2;
  return -1;
}

void uiPlayerFull(const char *header) {
  uiHeader(header);
  tft.fillRect(0, 26, 320, 188, C_BG);
  prevValid = false;
}

void uiPlayer(const PlayerView &v) {
  bool full = !prevValid;

  if (full || v.title != prev.title) {
    tft.fillRect(0, 36, 320, 34, C_BG);
    tft.setTextColor(C_TEXT, C_BG);
    tft.drawString(clip(v.title, 20), 16, 40, 4);
  }
  if (full || v.sub != prev.sub) {
    tft.fillRect(0, 74, 320, 18, C_BG);
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString(clip(v.sub, 34), 16, 74, 2);
  }
  if (full || fabsf(v.progress - prev.progress) > 0.002f ||
      v.tLeft != prev.tLeft || v.tRight != prev.tRight) {
    tft.fillRect(0, 100, 320, 34, C_BG);
    if (v.progress >= 0) {
      tft.fillRect(16, 104, 288, 6, C_PANEL);
      tft.fillRect(16, 104, (int)(288 * constrain(v.progress, 0.0f, 1.0f)), 6, C_ACCENT);
    }
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString(v.tLeft, 16, 116, 2);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(v.tRight, 304, 116, 2);
    tft.setTextDatum(TL_DATUM);
  }
  if (full || fabsf(v.volume - prev.volume) > 0.001f) {
    tft.fillRect(0, 142, 320, 22, C_BG);
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("VOL", 16, 146, 2);
    tft.fillRect(60, 150, 244, 8, C_PANEL);
    tft.fillRect(60, 150, (int)(244 * constrain(v.volume, 0.0f, 1.0f)), 8, C_GOOD);
  }
  if (full || v.playing != prev.playing) drawControls(v.playing);

  prev = v;
  prevValid = true;
}

// ---------- Menu ----------
int uiMenu(bool withLibrary, void (*idle)()) {
  std::vector<String> items;
  std::vector<int> ids;
  if (withLibrary) { items.push_back("Daftar Lagu"); ids.push_back(100); }
  for (int m = 0; m < MODE_COUNT; m++) { items.push_back(modeName(m)); ids.push_back(m); }
  items.push_back("Kembali"); ids.push_back(-1);

  int n = items.size(), sel = 0, top = 0;
  uiHeader("Menu");
  uiList(items, sel, top);
  uiFooter("Klik: OK    MENU: kembali");

  uint32_t touchAt = 0;
  for (;;) {
    if (idle) idle();
    encoderTake();                               // putar = volume saja, diabaikan di menu
    NavEvent nv = navPoll(true);
    int d = (nv == NAV_NEXT) ? 1 : (nv == NAV_PREV ? -1 : 0);
    if (d) {
      sel = constrain(sel + d, 0, n - 1);
      if (sel < top) top = sel;
      if (sel >= top + UI_ROWS) top = sel - UI_ROWS + 1;
      uiList(items, sel, top);
    }
    BtnEvent b = buttonPoll();
    if (millis() - touchAt > 40) {
      touchAt = millis();
      int16_t tx, ty;
      if (uiTouch(tx, ty) && ty >= UI_LIST_Y && ty < UI_LIST_Y + UI_ROWS * UI_ROW_H) {
        int i = top + (ty - UI_LIST_Y) / UI_ROW_H;
        if (i < n) { sel = i; b = BTN_CLICK; }
      }
    }
    if (b == BTN_CLICK) {
      int id = ids[sel];
      if (id >= 0 && id < MODE_COUNT) saveModeAndRestart(id);   // tidak kembali
      return id;
    }
    if (b == BTN_LONG || nv == NAV_MENU) return -1;
    delay(5);
  }
}
