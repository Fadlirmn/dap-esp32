// No-op stand-in for ESP32-A2DP's BluetoothA2DPSource: no real Bluetooth radio
// on desktop. sim_main.cpp drives bt_connected/bt_state_changed directly via
// keyboard instead of real connection callbacks.
#pragma once
#include "Arduino.h"
#include "esp_gap_bt_api.h"
#include <cstdint>

struct Frame {
    int16_t channel1;
    int16_t channel2;
};

typedef enum {
    ESP_A2D_CONNECTION_STATE_DISCONNECTED = 0,
    ESP_A2D_CONNECTION_STATE_CONNECTING,
    ESP_A2D_CONNECTION_STATE_CONNECTED,
    ESP_A2D_CONNECTION_STATE_DISCONNECTING,
} esp_a2d_connection_state_t;

typedef enum {
    ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND = 0,
    ESP_A2D_AUDIO_STATE_STOPPED,
    ESP_A2D_AUDIO_STATE_STARTED,
} esp_a2d_audio_state_t;

class BluetoothA2DPSource {
public:
    void set_auto_reconnect(esp_bd_addr_t, int = 0) {}
    void set_auto_reconnect(bool) {}
    void set_on_connection_state_changed(void (*)(esp_a2d_connection_state_t, void *), void * = nullptr) {}
    void set_on_audio_state_changed(void (*)(esp_a2d_audio_state_t, void *), void * = nullptr) {}
    void start(const char *, int32_t (*)(Frame *, int32_t)) {}
    void set_volume(uint8_t) {}
    void disconnect() {}
    void reconnect() {}
    bool is_connected() { return false; }
    void set_ssid_callback(bool (*)(const char *, esp_bd_addr_t, int32_t)) {}
    void cancel_discovery() {}
    void end(bool = false) {}
    void set_valid_cod_service(uint16_t) {}
};
