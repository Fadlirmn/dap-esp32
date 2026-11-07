// No-op stand-in for ESP32-A2DP's BluetoothA2DPSink: no real Bluetooth radio
// on desktop. Reuses BluetoothA2DPSource.h's connection/audio-state enums and
// Frame type, same as the real library shares them between source and sink.
#pragma once
#include "Arduino.h"
#include "BluetoothA2DPSource.h"
#include <cstdint>

class BluetoothA2DPSink {
public:
    void set_on_connection_state_changed(void (*)(esp_a2d_connection_state_t, void *), void * = nullptr) {}
    void set_on_audio_state_changed(void (*)(esp_a2d_audio_state_t, void *), void * = nullptr) {}
    void set_sample_rate_callback(void (*)(uint16_t)) {}
    void set_stream_reader(void (*)(const uint8_t *, uint32_t), bool = true) {}
    void start(const char *) {}
    const char *get_peer_name() { return "Sim Phone"; }
};
