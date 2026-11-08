// A2DP sink (phone -> this device -> wired IEM), for MODE_BT_RECEIVER.
// Mirrors bluetooth.cpp's callback-registration pattern but uses
// BluetoothA2DPSink instead of BluetoothA2DPSource. The sink's own internal
// I2S output is disabled (set_stream_reader(..., false)) since i2s_output.cpp
// already owns the I2S driver -- we just forward PCM bytes to it.
#include "bt_receiver.h"
#include <BluetoothA2DPSink.h>
#include "i2s_output.h"

BluetoothA2DPSink a2dp_sink;

volatile bool btrx_connected = false;
volatile bool btrx_playing = false;
String btrx_deviceName = "No device";

static void onBtReceiverConnectionState(esp_a2d_connection_state_t state, void *obj) {
    btrx_connected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
    if (btrx_connected) {
        const char *name = a2dp_sink.get_peer_name();
        btrx_deviceName = (name && name[0]) ? String(name) : String("Unknown Device");
    } else {
        btrx_playing = false;
        btrx_deviceName = "No device";
    }
}

static void onBtReceiverAudioState(esp_a2d_audio_state_t state, void *obj) {
    btrx_playing = (state == ESP_A2D_AUDIO_STATE_STARTED);
}

// Informational only: i2s_output.cpp is configured for a fixed rate (44100Hz)
// by i2sOutputSetup(), which we don't control here. Just log if the phone
// is actually sending something else, instead of silently playing wrong speed.
static void onBtReceiverSampleRate(uint16_t rate) {
    if (rate != 44100) {
        Serial.printf("[BTRX] Warning: source sample rate=%u, expected 44100\n", rate);
    }
}

// Raw PCM from the phone: 16-bit signed, interleaved stereo. Forward straight
// to the shared I2S writer; never touch I2S registers directly here.
static void onBtReceiverDataStream(const uint8_t *data, uint32_t length) {
    size_t frameCount = length / 4;  // 2 bytes/sample * 2 channels
    i2sWriteStereo16(reinterpret_cast<const int16_t *>(data), frameCount);
}

void btReceiverSetup() {
    btrx_connected = false;
    btrx_playing = false;
    btrx_deviceName = "No device";

    a2dp_sink.set_on_connection_state_changed(onBtReceiverConnectionState);
    a2dp_sink.set_on_audio_state_changed(onBtReceiverAudioState);
    a2dp_sink.set_sample_rate_callback(onBtReceiverSampleRate);
    // false = don't let the library drive its own I2S output; we supply PCM
    // to i2s_output.cpp ourselves via the callback instead.
    a2dp_sink.set_stream_reader(onBtReceiverDataStream, false);
    a2dp_sink.start("DAP Receiver");
}

// Sink-side absolute volume (AVRCP) isn't essential and support varies by
// library version -- the encoder must work regardless, so drive it through
// the shared I2S software gain instead.
void btReceiverSetVolume(uint8_t volume0to127) {
    i2sOutputSetVolume(volume0to127);
}

// AVRCP passthrough: tell the connected phone to play/pause/skip, the same
// way a BT headset's buttons control the phone. BluetoothA2DPSink::play()/
// pause()/next()/previous() already send the PRESSED+RELEASED passthrough
// pair internally (see execute_avrc_command() in the library).
void btReceiverPlayPause() {
    if (!btrx_connected) return;
    if (btrx_playing) {
        a2dp_sink.pause();
    } else {
        a2dp_sink.play();
    }
}

void btReceiverNext() {
    if (!btrx_connected) return;
    a2dp_sink.next();
}

void btReceiverPrev() {
    if (!btrx_connected) return;
    a2dp_sink.previous();
}
