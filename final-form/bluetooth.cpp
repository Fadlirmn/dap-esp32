// Ported from old/Stream32-ESP-Player.ino lines 169-270 (state + callbacks) and
// 477-576 (scan/select), plus the BT portion of setup()/loop() folded into
// btSetup()/checkBluetoothScanTimeout(). BT_DEVICES[]/startBluetoothDevice(index)
// was dead code there (nothing ever called it) and is dropped here.
#include "bluetooth.h"
#include <Preferences.h>
#include "audio_decoder.h"
#include "nav.h"
#include "screens.h"

extern Preferences preferences;

BluetoothA2DPSource a2dp_source;

volatile bool bt_connected = false;
volatile bool bt_audio_started = false;
volatile bool play_requested = false;
volatile bool bt_state_changed = false;
volatile bool bt_connecting = false;
unsigned long play_request_started_at = 0;

uint8_t bt_volume = 70;
bool autoPlayEnabled = false;

String btScanNames[MAX_BT_SCAN_RESULTS];
esp_bd_addr_t btScanAddresses[MAX_BT_SCAN_RESULTS];
volatile uint8_t btScanCount = 0;
volatile bool btScanActive = false;
unsigned long btScanStartedAt = 0;
bool hasSavedBTAddress = false;
esp_bd_addr_t savedBTAddress = {0, 0, 0, 0, 0, 0};
String savedBTDevice = "Bluetooth Device";

void onBluetoothConnectionState(esp_a2d_connection_state_t state, void *obj) {
    bt_connected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
    if (bt_connected) bt_connecting = false;
    if (!bt_connected) bt_audio_started = false;
    bt_state_changed = true;
}

void onBluetoothAudioState(esp_a2d_audio_state_t state, void *obj) {
    bt_audio_started = (state == ESP_A2D_AUDIO_STATE_STARTED);
    bt_state_changed = true;
}

// All queue and decoder changes happen in the main loop, never in a Bluetooth callback.
void updatePlaybackFromBluetooth() {
    if (!bt_connected) {
        if (is_playing) {
            play_requested = true;
            is_playing = false;
            decoder_reset_requested = true;
            if (pcm_frame_queue) xQueueReset(pcm_frame_queue);
        }
        return;
    }

    if (autoPlayEnabled && !play_requested) {
        play_requested = true;
        play_request_started_at = millis();
    }
    // Some headsets report A2DP media state only after the source begins supplying PCM.
    // Prime the decoder shortly after a valid Play request instead of waiting forever for that callback.
    const bool mayPrimeStream = play_requested &&
                                (bt_audio_started ||
                                 (millis() - play_request_started_at >= A2DP_PLAY_PRIME_DELAY_MS));
    if (mayPrimeStream && !is_playing && audioFile && g_file_size > 0) {
        decoder_reset_requested = true;
        if (pcm_frame_queue) xQueueReset(pcm_frame_queue);
        is_playing = true;
        Serial.printf("[Audio] Starting decoder (media=%d)\n", bt_audio_started ? 1 : 0);
    }
}

void btSetup() {
    preferences.begin("bt_pref", true);
    String storedBTName = preferences.getString("bt_name", "Bluetooth Device");
    if (storedBTName.length() > 0) savedBTDevice = storedBTName;
    if (preferences.getBytesLength("bt_addr") == ESP_BD_ADDR_LEN) {
        preferences.getBytes("bt_addr", savedBTAddress, ESP_BD_ADDR_LEN);
        hasSavedBTAddress = true;
    }
    preferences.end();

    preferences.begin("vol_pref", true);
    bt_volume = preferences.getUChar("volume", 70);
    preferences.end();

    preferences.begin("play_pref", true);
    autoPlayEnabled = preferences.getBool("autoplay", false);
    preferences.end();

    if (hasSavedBTAddress) {
        a2dp_source.set_auto_reconnect(savedBTAddress, 1);
        bt_connecting = true;
    }
    a2dp_source.set_on_connection_state_changed(onBluetoothConnectionState);
    a2dp_source.set_on_audio_state_changed(onBluetoothAudioState);
    a2dp_source.start(savedBTDevice.c_str(), get_sound_data);
    a2dp_source.set_volume(bt_volume);
}

// CONNECT/DISCONNECT button -- ported from the STATE_BT_MANAGER touch branch
// (lines 1553-1569); caller (nav.cpp) redraws afterward.
void toggleBluetoothConnection() {
    if (bt_connected) {
        Serial.println("[BT] Disconnect requested");
        bt_connecting = false;
        a2dp_source.disconnect();
    } else {
        Serial.println("[BT] Reconnect requested");
        bt_connecting = true;
        a2dp_source.reconnect();
    }
    delay(100);
    bt_connected = a2dp_source.is_connected();
    if (bt_connected) bt_connecting = false;
    bt_state_changed = true;
}

bool bluetoothScanCallback(const char *name, esp_bd_addr_t address, int rssi) {
    if (!btScanActive || name == nullptr || name[0] == '\0') return false;
    for (uint8_t i = 0; i < btScanCount; i++) {
        if (memcmp(btScanAddresses[i], address, ESP_BD_ADDR_LEN) == 0) return false;
    }
    if (btScanCount < MAX_BT_SCAN_RESULTS) {
        btScanNames[btScanCount] = String(name);
        memcpy(btScanAddresses[btScanCount], address, ESP_BD_ADDR_LEN);
        Serial.print("[BT] Found: ");
        Serial.print(name);
        Serial.print("  RSSI: ");
        Serial.println(rssi);
        btScanCount++;
    }
    return false;
}

void startBluetoothScan() {
    btScanCount = 0;
    btScanActive = true;
    bt_connecting = false;
    btScanStartedAt = millis();
    if (a2dp_source.is_connected()) {
        a2dp_source.disconnect();
        delay(300);
    }
    a2dp_source.set_auto_reconnect(false);
    a2dp_source.set_valid_cod_service(ESP_BT_COD_SRVC_RENDERING |
                                      ESP_BT_COD_SRVC_AUDIO |
                                      ESP_BT_COD_SRVC_TELEPHONY);
    a2dp_source.set_ssid_callback(bluetoothScanCallback);
    esp_err_t scanResult = esp_bt_gap_start_discovery(
        ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    Serial.printf("[BT] SCAN START result=%d\n", (int)scanResult);
    currentState = STATE_BT_SCAN;
    drawCurrentStateUI();
}

void selectBluetoothScanResult(uint8_t index) {
    if (index >= btScanCount) return;
    memcpy(savedBTAddress, btScanAddresses[index], ESP_BD_ADDR_LEN);
    hasSavedBTAddress = true;
    savedBTDevice = btScanNames[index];
    preferences.begin("bt_pref", false);
    preferences.putString("bt_name", savedBTDevice);
    preferences.putBytes("bt_addr", savedBTAddress, ESP_BD_ADDR_LEN);
    preferences.end();
    btScanActive = false;
    bt_connected = false;
    bt_audio_started = false;
    bt_connecting = true;
    bt_state_changed = true;
    a2dp_source.set_ssid_callback(nullptr);
    a2dp_source.end(false);
    delay(250);
    a2dp_source.set_auto_reconnect(savedBTAddress, 1);
    a2dp_source.set_on_connection_state_changed(onBluetoothConnectionState);
    a2dp_source.set_on_audio_state_changed(onBluetoothAudioState);
    a2dp_source.start(savedBTDevice.c_str(), get_sound_data);
    a2dp_source.set_volume(bt_volume);
    xQueueReset(pcm_frame_queue);
    is_playing = false;
    currentState = STATE_BT_MANAGER;
    drawCurrentStateUI();
}

// Ported from the scan-timeout block in loop() (lines 1744-1753).
void checkBluetoothScanTimeout() {
    if (!btScanActive || millis() - btScanStartedAt <= 10000) return;
    btScanActive = false;
    a2dp_source.cancel_discovery();
    a2dp_source.set_ssid_callback(nullptr);
    if (!screenLocked) drawCurrentStateUI();
}
