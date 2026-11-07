// Mode 2: BT Receiver (HP -> A2DP Sink -> PCM5102A -> IEM)
#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include "modes.h"
#include "common.h"
#include "ui.h"

static BluetoothA2DPSink *a2dp = nullptr;
static char bt_title[96] = "", bt_artist[96] = "";
static uint8_t vol = 64;                        // 0..127

static void onMeta(uint8_t id, const uint8_t *text) {
  if (id == ESP_AVRC_MD_ATTR_TITLE)       strlcpy(bt_title,  (const char *)text, sizeof(bt_title));
  else if (id == ESP_AVRC_MD_ATTR_ARTIST) strlcpy(bt_artist, (const char *)text, sizeof(bt_artist));
}

void receiverSetup() {
  uiPlayerFull("BT Receiver");
  i2s_pin_config_t pins = {
    .mck_io_num   = I2S_PIN_NO_CHANGE,
    .bck_io_num   = PIN_I2S_BCK,
    .ws_io_num    = PIN_I2S_LRC,
    .data_out_num = PIN_I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };
  a2dp = new BluetoothA2DPSink();
  a2dp->set_pin_config(pins);
  a2dp->set_avrc_metadata_callback(onMeta);
  a2dp->set_auto_reconnect(true);
  a2dp->start(BT_RECEIVER_NAME);
  a2dp->set_volume(vol);
}

void receiverLoop() {
  int d = encoderTake();
  BtnEvent b = buttonPoll();
  NavEvent nv = navPoll(false);
  int16_t tx = 0, ty = 0;
  bool tap = false;
  static uint32_t touchAt = 0, drawAt = 0;
  uint32_t now = millis();
  if (now - touchAt > 40) { touchAt = now; tap = uiTouch(tx, ty); }

  if (d) { vol = constrain((int)vol + d * 4, 0, 127); a2dp->set_volume(vol); }

  bool connected = a2dp->is_connected();
  bool playing = connected && a2dp->get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED;

  int act = -1;                                  // 0 prev, 1 toggle, 2 next, 3 menu
  if (tap) act = uiControlHit(tx, ty);
  if (b == BTN_CLICK) act = 1;
  else if (b == BTN_DOUBLE) act = 2;
  else if (b == BTN_LONG) act = 3;
  if (nv == NAV_PREV) act = 0; else if (nv == NAV_NEXT) act = 2; else if (nv == NAV_MENU) act = 3;

  switch (act) {
    case 0: a2dp->previous(); break;
    case 1: if (playing) a2dp->pause(); else a2dp->play(); break;
    case 2: a2dp->next(); break;
    case 3:
      uiMenu(false, nullptr);
      uiPlayerFull("BT Receiver");
      break;
  }

  if (now - drawAt > 250) {
    drawAt = now;
    PlayerView v;
    if (connected) {
      v.title = bt_title[0] ? String(bt_title) : String("Terhubung");
      v.sub   = bt_artist[0] ? String(bt_artist) : String("Putar musik dari HP");
    } else {
      v.title = "Menunggu perangkat...";
      v.sub   = String("Pairing: ") + BT_RECEIVER_NAME;
    }
    v.progress = -1;
    v.volume = vol / 127.0f;
    v.playing = playing;
    uiPlayer(v);
  }
  delay(2);
}
