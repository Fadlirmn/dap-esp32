// A2DP source (SD -> phone/headset over Bluetooth), ported from
// old/Stream32-ESP-Player.ino lines 169-270, 477-576, and the BT block of setup()/loop().
// No display dependency. (The original's BT_DEVICES[]/startBluetoothDevice(index)
// was dead code -- a 1-entry placeholder list nothing ever called -- dropped here.)
#pragma once
#include <Arduino.h>
#include "BluetoothA2DPSource.h"
#include "esp_gap_bt_api.h"

extern BluetoothA2DPSource a2dp_source;

extern volatile bool bt_connected;
extern volatile bool bt_audio_started;
extern volatile bool play_requested;
extern volatile bool bt_state_changed;
extern volatile bool bt_connecting;
extern unsigned long play_request_started_at;
constexpr unsigned long A2DP_PLAY_PRIME_DELAY_MS = 350;

extern uint8_t bt_volume;
extern bool autoPlayEnabled;

constexpr uint8_t MAX_BT_SCAN_RESULTS = 6;
extern String btScanNames[MAX_BT_SCAN_RESULTS];
extern esp_bd_addr_t btScanAddresses[MAX_BT_SCAN_RESULTS];
extern volatile uint8_t btScanCount;
extern volatile bool btScanActive;
extern unsigned long btScanStartedAt;
extern bool hasSavedBTAddress;
extern esp_bd_addr_t savedBTAddress;
extern String savedBTDevice;

// One-time init: load bt/volume/autoplay prefs and start the A2DP source. Call once
// from setup(), after get_sound_data's queue/mutex exist.
void btSetup();

void onBluetoothConnectionState(esp_a2d_connection_state_t state, void *obj);
void onBluetoothAudioState(esp_a2d_audio_state_t state, void *obj);
void updatePlaybackFromBluetooth();
void toggleBluetoothConnection();  // CONNECT/DISCONNECT button and tap share this
bool bluetoothScanCallback(const char *name, esp_bd_addr_t address, int rssi);
void startBluetoothScan();
void selectBluetoothScanResult(uint8_t index);
void checkBluetoothScanTimeout();  // called from loop(): cancels scan after 10s
