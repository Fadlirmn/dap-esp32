// Real ESP32 classic-BT backend: A2DP source (Transmitter, L5) + sink
// (Receiver, L4). Ported from final-form/bluetooth.* (scan/connect logic) and
// final-form/bt_receiver.* (sink connection + AVRCP passthrough), stripped of
// their UI coupling (currentState/drawCurrentStateUI/nav.h) since
// screen_manager.cpp owns navigation here instead.
//
// Not linked into sim/ (desktop SDL has no real BT radio) -- every call site
// in screen_*.cpp is already #ifndef KODE_OPTIMASI_SIM-gated, same pattern as
// the dummy-data gating. Only one role is ever active at a time (source XOR
// sink, same constraint final-form has) -- start_* tears down the other role
// first if needed.
//
// No SD/audio decode exists in this folder (out of scope, see README.md), so
// the source role streams silence until a later pass wires a real decoder
// into bt_tx_fill_audio() -- this still lets scanning/pairing work for real.
#pragma once
#include <stdint.h>

constexpr uint8_t BT_MAX_SCAN_RESULTS = 6;

void bt_backend_start_transmitter(); // idempotent: no-op if already the source
void bt_backend_start_receiver();    // idempotent: no-op if already the sink
void bt_backend_stop();              // tear down whichever role is active

// Call once per guiTask loop iteration (cheap: just checks a scan timeout).
void bt_backend_tick();

// --- Transmitter (source): ESP32 -> BT speaker ---
void bt_tx_start_scan();  // 10s classic-BT inquiry, manual trigger (not continuous)
bool bt_tx_scan_active();
uint8_t bt_tx_scan_count();
const char *bt_tx_scan_name(uint8_t index);
void bt_tx_connect_scan_result(uint8_t index);
void bt_tx_disconnect();
bool bt_tx_connected();
const char *bt_tx_device_name(); // connected/saved speaker name, else "Tidak ada speaker"

// --- Receiver (sink): phone -> ESP32 ---
// No scan here by design -- a sink is passive (the phone initiates the
// connection), so there's nothing to pick from a list.
bool bt_rx_connected();
const char *bt_rx_device_name(); // connected phone's name, else "Menunggu koneksi..."
void bt_rx_forget_device();      // drop the current connection so another phone can pair
void bt_rx_play_pause();         // AVRCP passthrough to the connected phone
void bt_rx_next();
void bt_rx_prev();
bool bt_rx_is_playing();
