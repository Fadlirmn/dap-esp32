#include "common.h"
#include <SD.h>
#include <SPI.h>
#include <Preferences.h>
#include <algorithm>

// ================= Baterai =================
struct BatPt { float v; uint8_t p; };
static const BatPt CURVE[] = {
  {4.20f, 100}, {4.00f, 80}, {3.80f, 55}, {3.70f, 35}, {3.50f, 10}, {3.30f, 0}
};
static float batV = 0;

void batteryUpdate() {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(PIN_BAT_ADC);
  float v = (sum / 16.0f) / 1000.0f * BAT_DIV_RATIO;
  batV = (batV <= 0.1f) ? v : batV * 0.8f + v * 0.2f;
}

void batteryBegin() {
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
  for (int i = 0; i < 5; i++) batteryUpdate();
}

float batteryVolts() { return batV; }

uint8_t batteryPercent() {
  if (batV >= CURVE[0].v) return 100;
  for (int i = 0; i < 5; i++) {
    const BatPt &a = CURVE[i], &b = CURVE[i + 1];
    if (batV >= b.v) {
      float t = (batV - b.v) / (a.v - b.v);
      return (uint8_t)(b.p + t * (a.p - b.p));
    }
  }
  return 0;
}

// ================= Encoder =================
static volatile int8_t encDelta = 0, encAcc = 0;
static volatile uint8_t encState = 0;
static portMUX_TYPE encMux = portMUX_INITIALIZER_UNLOCKED;
static const DRAM_ATTR int8_t ENC_TBL[16] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};

static void IRAM_ATTR encISR() {
  encState = ((encState << 2) |
              (gpio_get_level((gpio_num_t)PIN_ENC_A) << 1) |
               gpio_get_level((gpio_num_t)PIN_ENC_B)) & 0x0F;
  int8_t a = encAcc + ENC_TBL[encState];
  if (a >= 4)       { encDelta = encDelta + 1; a = 0; }
  else if (a <= -4) { encDelta = encDelta - 1; a = 0; }
  encAcc = a;
}

struct NavBtn { uint8_t pin; NavEvent ev; bool stable; uint32_t changeAt, repeatAt; };
static NavBtn nav[3] = {
  {PIN_BTN_PREV, NAV_PREV, false, 0, 0},
  {PIN_BTN_NEXT, NAV_NEXT, false, 0, 0},
  {PIN_BTN_MENU, NAV_MENU, false, 0, 0},
};

NavEvent navPoll(bool repeat) {
  uint32_t now = millis();
  for (auto &b : nav) {
    bool raw = (digitalRead(b.pin) == LOW);
    if (raw != b.stable && now - b.changeAt > 20) {
      b.stable = raw; b.changeAt = now;
      if (raw) { b.repeatAt = now + 450; return b.ev; }
    } else if (raw && b.stable && repeat && b.ev != NAV_MENU && now >= b.repeatAt) {
      b.repeatAt = now + 110;
      return b.ev;
    }
  }
  return NAV_NONE;
}

void inputBegin() {
  for (auto &b : nav) pinMode(b.pin, INPUT_PULLUP);   // GPIO 36/39 tidak punya pull-up internal: pakai resistor eksternal
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), encISR, CHANGE);
}

int encoderTake() {
  portENTER_CRITICAL(&encMux);
  int d = encDelta;
  encDelta = 0;
  portEXIT_CRITICAL(&encMux);
  return ENC_REVERSE ? -d : d;
}

// ================= Tombol: klik / klik 2x / tahan =================
#define DEBOUNCE_MS 20
#define DOUBLE_MS   300
#define LONG_MS     700

BtnEvent buttonPoll() {
  static bool stable = false, longFired = false;
  static uint32_t lastChange = 0, tDown = 0, tUp = 0;
  static uint8_t clicks = 0;

  uint32_t now = millis();
  bool raw = (digitalRead(PIN_ENC_SW) == LOW);
  bool pressed = false, released = false;

  if (raw != stable && now - lastChange > DEBOUNCE_MS) {
    stable = raw; lastChange = now;
    if (stable) pressed = true; else released = true;
  }
  if (pressed) { tDown = now; longFired = false; }
  if (stable && !longFired && now - tDown >= LONG_MS) {
    longFired = true; clicks = 0;
    return BTN_LONG;
  }
  if (released && !longFired) { clicks++; tUp = now; }
  if (!stable && clicks > 0 && now - tUp >= DOUBLE_MS) {
    BtnEvent e = (clicks >= 2) ? BTN_DOUBLE : BTN_CLICK;
    clicks = 0;
    return e;
  }
  return BTN_NONE;
}

// ================= Storage =================
std::vector<String> tracks;

bool storageBegin() {
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);
  return SD.begin(PIN_SD_CS, SPI, 20000000);
}

static bool isAudioFile(const String &path, bool mp3Only) {
  String n = path.substring(path.lastIndexOf('/') + 1);
  if (n.startsWith(".")) return false;           // file sampah macOS (._xxx)
  String l = n; l.toLowerCase();
  if (l.endsWith(".mp3")) return true;
  if (mp3Only) return false;
  return l.endsWith(".wav") || l.endsWith(".flac") || l.endsWith(".aac") || l.endsWith(".m4a");
}

void scanTracks(bool mp3Only) {
  tracks.clear();
  const char *dirs[] = {MUSIC_DIR, "/"};
  for (const char *d : dirs) {
    File dir = SD.open(d);
    if (!dir || !dir.isDirectory()) { if (dir) dir.close(); continue; }
    File f = dir.openNextFile();
    while (f) {
      if (!f.isDirectory()) {
        String p = f.path();
        if (isAudioFile(p, mp3Only)) tracks.push_back(p);
      }
      f.close();
      if ((int)tracks.size() >= MAX_TRACKS) break;
      f = dir.openNextFile();
    }
    dir.close();
    if (!tracks.empty()) break;                  // pakai /Music dulu, kalau kosong baru root
  }
  std::sort(tracks.begin(), tracks.end(), [](const String &a, const String &b) {
    return strcasecmp(a.c_str(), b.c_str()) < 0;
  });
}

String trackTitle(int i) {
  if (i < 0 || i >= (int)tracks.size()) return String("-");
  String n = tracks[i].substring(tracks[i].lastIndexOf('/') + 1);
  int dot = n.lastIndexOf('.');
  return dot > 0 ? n.substring(0, dot) : n;
}

// ================= Mode =================
static const char *MODE_NAMES[MODE_COUNT] = {
  "Local Player", "BT Receiver", "BT Transmitter", "Transfer File"
};
const char *modeName(uint8_t m) { return m < MODE_COUNT ? MODE_NAMES[m] : "?"; }

uint8_t loadMode() {
  Preferences p; p.begin("dap", true);
  uint8_t m = p.getUChar("mode", MODE_LOCAL);
  p.end();
  return m < MODE_COUNT ? m : MODE_LOCAL;
}

void saveModeAndRestart(uint8_t m) {
  Preferences p; p.begin("dap", false);
  p.putUChar("mode", m);
  p.end();
  delay(100);
  ESP.restart();
}
