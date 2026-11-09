// See bt_backend.h. Scan/connect logic ported near-verbatim from
// final-form/bluetooth.cpp; sink connection+AVRCP ported from
// final-form/bt_receiver.cpp. Both now live behind one role switch instead
// of final-form's boot-time mode pick, since here the user picks BT
// Receiver/Transmitter at runtime from the Menu.
#include "bt_backend.h"
#include <Arduino.h>
#include <Preferences.h>
#include <BluetoothA2DPSource.h>
#include <BluetoothA2DPSink.h>
#include <esp_gap_bt_api.h>
#include <string.h>
#include "audio_decoder.h"
#include "i2s_output.h"

enum class BtRole { NONE, SOURCE, SINK };
static BtRole s_role = BtRole::NONE;

static BluetoothA2DPSource s_source;
static BluetoothA2DPSink s_sink;

// --- Transmitter (source) state ---
static String s_scanNames[BT_MAX_SCAN_RESULTS];
static esp_bd_addr_t s_scanAddrs[BT_MAX_SCAN_RESULTS];
static volatile uint8_t s_scanCount = 0;
static volatile bool s_scanActive = false;
static unsigned long s_scanStartedAt = 0;
static bool s_hasSavedAddr = false;
static esp_bd_addr_t s_savedAddr = {0, 0, 0, 0, 0, 0};
static String s_savedName = "Tidak ada speaker";
static volatile bool s_txConnected = false;

static void on_tx_connection_state(esp_a2d_connection_state_t state, void *obj) {
    (void)obj;
    s_txConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
    Serial.printf("[BT-TX] connection state=%d connected=%d\n", (int)state, s_txConnected);
}

static bool tx_scan_callback(const char *name, esp_bd_addr_t address, int rssi) {
    if (!s_scanActive || name == nullptr || name[0] == '\0') return false;
    for (uint8_t i = 0; i < s_scanCount; i++) {
        if (memcmp(s_scanAddrs[i], address, ESP_BD_ADDR_LEN) == 0) return false;
    }
    if (s_scanCount < BT_MAX_SCAN_RESULTS) {
        s_scanNames[s_scanCount] = String(name);
        memcpy(s_scanAddrs[s_scanCount], address, ESP_BD_ADDR_LEN);
        s_scanCount++;
        Serial.printf("[BT-TX] found: %s  RSSI: %d\n", name, rssi);
    }
    return false; // we only want discovery results, never auto-connect from here
}

// --- Receiver (sink) state ---
static volatile bool s_rxConnected = false;
static volatile bool s_rxPlaying = false;
static String s_rxDeviceName = "Menunggu koneksi...";

static void on_rx_connection_state(esp_a2d_connection_state_t state, void *obj) {
    (void)obj;
    s_rxConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
    if (s_rxConnected) {
        const char *name = s_sink.get_peer_name();
        s_rxDeviceName = (name && name[0]) ? String(name) : String("Perangkat tidak dikenal");
    } else {
        s_rxPlaying = false;
        s_rxDeviceName = "Menunggu koneksi...";
    }
    Serial.printf("[BT-RX] connected=%d device=%s\n", s_rxConnected, s_rxDeviceName.c_str());
}

static void on_rx_audio_state(esp_a2d_audio_state_t state, void *obj) {
    (void)obj;
    s_rxPlaying = (state == ESP_A2D_AUDIO_STATE_STARTED);
}

// Raw PCM from the phone: 16-bit signed, interleaved stereo. Forward straight
// to the shared I2S writer (ported from final-form/bt_receiver.cpp's
// onBtReceiverDataStream).
static void on_rx_data_stream(const uint8_t *data, uint32_t length) {
    size_t frameCount = length / 4;  // 2 bytes/sample * 2 channels
    i2sWriteStereo16(reinterpret_cast<const int16_t *>(data), frameCount);
}

// --- Role lifecycle ---
void bt_backend_start_transmitter() {
    if (s_role == BtRole::SOURCE) return;
    bt_backend_stop();

    Preferences prefs;
    prefs.begin("bt_tx_pref", true);
    String storedName = prefs.getString("name", "Tidak ada speaker");
    if (storedName.length() > 0) s_savedName = storedName;
    if (prefs.getBytesLength("addr") == ESP_BD_ADDR_LEN) {
        prefs.getBytes("addr", s_savedAddr, ESP_BD_ADDR_LEN);
        s_hasSavedAddr = true;
    }
    prefs.end();

    if (s_hasSavedAddr) s_source.set_auto_reconnect(s_savedAddr, 1);
    s_source.set_on_connection_state_changed(on_tx_connection_state);
    s_source.start("DAP Transmitter", get_sound_data);
    s_role = BtRole::SOURCE;
}

void bt_backend_start_receiver() {
    if (s_role == BtRole::SINK) return;
    bt_backend_stop();

    s_sink.set_on_connection_state_changed(on_rx_connection_state);
    s_sink.set_on_audio_state_changed(on_rx_audio_state);
    // false = don't let the library drive its own I2S output; we forward
    // PCM to i2s_output.cpp ourselves via on_rx_data_stream instead.
    s_sink.set_stream_reader(on_rx_data_stream, false);
    s_sink.start("DAP Receiver");
    s_role = BtRole::SINK;
}

void bt_backend_stop() {
    if (s_role == BtRole::SOURCE) {
        s_scanActive = false;
        s_source.set_ssid_callback(nullptr);
        s_source.end(false);
        s_txConnected = false;
    } else if (s_role == BtRole::SINK) {
        s_sink.end(true);
        s_rxConnected = false;
        s_rxPlaying = false;
        s_rxDeviceName = "Menunggu koneksi...";
    }
    s_role = BtRole::NONE;
}

void bt_backend_tick() {
    if (!s_scanActive) return;
    if (millis() - s_scanStartedAt <= 10000) return;
    s_scanActive = false;
    s_source.cancel_discovery();
    s_source.set_ssid_callback(nullptr);
}

// --- Transmitter API ---
void bt_tx_start_scan() {
    if (s_role != BtRole::SOURCE) bt_backend_start_transmitter();
    s_scanCount = 0;
    s_scanActive = true;
    s_scanStartedAt = millis();
    if (s_source.is_connected()) {
        s_source.disconnect();
        delay(300);
    }
    s_source.set_auto_reconnect(false);
    s_source.set_valid_cod_service(ESP_BT_COD_SRVC_RENDERING | ESP_BT_COD_SRVC_AUDIO);
    s_source.set_ssid_callback(tx_scan_callback);
    esp_err_t r = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    Serial.printf("[BT-TX] scan start result=%d\n", (int)r);
}

bool bt_tx_scan_active() { return s_scanActive; }
uint8_t bt_tx_scan_count() { return s_scanCount; }

const char *bt_tx_scan_name(uint8_t index) {
    if (index >= s_scanCount) return "";
    return s_scanNames[index].c_str();
}

void bt_tx_connect_scan_result(uint8_t index) {
    if (index >= s_scanCount) return;
    memcpy(s_savedAddr, s_scanAddrs[index], ESP_BD_ADDR_LEN);
    s_hasSavedAddr = true;
    s_savedName = s_scanNames[index];

    Preferences prefs;
    prefs.begin("bt_tx_pref", false);
    prefs.putString("name", s_savedName);
    prefs.putBytes("addr", s_savedAddr, ESP_BD_ADDR_LEN);
    prefs.end();

    s_scanActive = false;
    s_source.set_ssid_callback(nullptr);
    s_source.set_auto_reconnect(s_savedAddr, 1);
    s_source.connect_to(s_savedAddr);
}

void bt_tx_disconnect() {
    s_source.disconnect();
    s_txConnected = false;
}

bool bt_tx_connected() { return s_txConnected; }
const char *bt_tx_device_name() { return s_savedName.c_str(); }

// --- Receiver API ---
bool bt_rx_connected() { return s_rxConnected; }
const char *bt_rx_device_name() { return s_rxDeviceName.c_str(); }

void bt_rx_forget_device() {
    s_sink.disconnect();
    s_rxConnected = false;
    s_rxPlaying = false;
    s_rxDeviceName = "Menunggu koneksi...";
}

void bt_rx_play_pause() {
    if (!s_rxConnected) return;
    if (s_rxPlaying) s_sink.pause();
    else s_sink.play();
}

void bt_rx_next() { if (s_rxConnected) s_sink.next(); }
void bt_rx_prev() { if (s_rxConnected) s_sink.previous(); }
bool bt_rx_is_playing() { return s_rxPlaying; }
