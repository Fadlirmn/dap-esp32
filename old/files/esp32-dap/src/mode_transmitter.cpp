// Mode 3: BT Transmitter (SD MP3 -> A2DP Source -> headset BT), pakai AudioTools + libhelix
// EKSPERIMENTAL: hanya MP3 44.1 kHz. Include path AudioTools berbeda antar versi, jadi ada fallback.
#include <Arduino.h>
#include <SD.h>
#include "AudioTools.h"
#if __has_include("AudioTools/AudioCodecs/CodecMP3Helix.h")
  #include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#else
  #include "AudioCodecs/CodecMP3Helix.h"
#endif
#if __has_include("AudioTools/Communication/A2DPStream.h")
  #include "AudioTools/Communication/A2DPStream.h"
#elif __has_include("AudioTools/AudioLibs/A2DPStream.h")
  #include "AudioTools/AudioLibs/A2DPStream.h"
#else
  #include "AudioLibs/A2DPStream.h"
#endif
#include "modes.h"
#include "common.h"
#include "ui.h"

static A2DPStream *a2dp = nullptr;
static EncodedAudioStream *dec = nullptr;
static StreamCopy *cp = nullptr;
static File f;
static int cur = 0;
static bool playing = true, ready = false, firstStart = true;
static float vol = 0.5f;

static void startTrack(int i) {
  int n = tracks.size();
  if (!n) return;
  cur = (i % n + n) % n;
  if (f) f.close();
  f = SD.open(tracks[cur].c_str());
  if (!firstStart) dec->end();
  firstStart = false;
  dec->begin();
  delete cp;
  cp = new StreamCopy(*dec, f);
}

void transmitterSetup() {
  uiPlayerFull("BT Transmitter");
  if (!storageBegin()) { uiMessage("SD card tidak terbaca", nullptr, C_BAD); return; }
  scanTracks(true);
  if (tracks.empty()) { uiMessage("Tidak ada file MP3", "Taruh di /Music", C_WARN); return; }

  a2dp = new A2DPStream();
  auto cfg = a2dp->defaultConfig(TX_MODE);
  cfg.name = BT_TARGET_NAME;
  cfg.auto_reconnect = true;
  a2dp->begin(cfg);
  a2dp->setVolume(vol);

  dec = new EncodedAudioStream(a2dp, new MP3DecoderHelix());
  startTrack(0);
  ready = true;
}

void transmitterLoop() {
  int d = encoderTake();
  BtnEvent b = buttonPoll();
  NavEvent nv = navPoll(false);
  int16_t tx = 0, ty = 0;
  bool tap = false;
  static uint32_t touchAt = 0, drawAt = 0;
  uint32_t now = millis();
  if (now - touchAt > 40) { touchAt = now; tap = uiTouch(tx, ty); }

  if (!ready) {
    if (b == BTN_LONG || b == BTN_CLICK) { uiMenu(false, nullptr); uiPlayerFull("BT Transmitter"); }
    delay(10);
    return;
  }

  if (d) { vol = constrain(vol + d * 0.04f, 0.0f, 1.0f); a2dp->setVolume(vol); }

  int act = -1;
  if (tap) act = uiControlHit(tx, ty);
  if (b == BTN_CLICK) act = 1;
  else if (b == BTN_DOUBLE) act = 2;
  else if (b == BTN_LONG) act = 3;
  if (nv == NAV_PREV) act = 0; else if (nv == NAV_NEXT) act = 2; else if (nv == NAV_MENU) act = 3;
  switch (act) {
    case 0: startTrack(cur - 1); break;
    case 1: playing = !playing; break;
    case 2: startTrack(cur + 1); break;
    case 3: uiMenu(false, nullptr); uiPlayerFull("BT Transmitter"); break;
  }

  if (playing && cp) {
    if (cp->copy() == 0 && !f.available()) startTrack(cur + 1);   // lagu habis -> berikutnya
  }

  if (now - drawAt > 300) {
    drawAt = now;
    PlayerView v;
    v.title = trackTitle(cur);
    v.sub = String("Ke: ") + BT_TARGET_NAME;
    v.progress = (f && f.size()) ? (float)f.position() / f.size() : 0;
    v.tLeft = String(cur + 1) + " / " + String(tracks.size());
    v.tRight = "";
    v.volume = vol;
    v.playing = playing;
    uiPlayer(v);
  }
}
