// Ported from final-form/i2s_output.cpp, minus its Local Player drain task
// (i2sOutputDrainTask, gated on nav.h's operatingMode == MODE_LOCAL_PLAYER):
// that mode doesn't exist here. The only caller here is BT Receiver's own
// A2DP sink callback (bt_backend.cpp's on_rx_data_stream), which calls
// i2sWriteStereo16() directly -- no task of ours needed.
#include <Arduino.h>
#include <driver/i2s.h>
#include "i2s_output.h"
#include "display_pins.h"

static volatile uint8_t gVolume = 127;

void i2sOutputSetup() {
    i2s_config_t config = {};
    config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = 44100;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = 0;
    config.dma_desc_num = 8;
    config.dma_frame_num = 64;
    config.use_apll = false;
    config.tx_desc_auto_clear = true;

    i2s_driver_install(I2S_NUM_0, &config, 0, NULL);

    i2s_pin_config_t pinConfig = {};
    pinConfig.mck_io_num = I2S_PIN_NO_CHANGE;
    pinConfig.bck_io_num = PIN_I2S_BCLK;
    pinConfig.ws_io_num = PIN_I2S_LRC;
    pinConfig.data_out_num = PIN_I2S_DOUT;
    pinConfig.data_in_num = I2S_PIN_NO_CHANGE;
    i2s_set_pin(I2S_NUM_0, &pinConfig);
}

void i2sOutputSetVolume(uint8_t volume0to127) {
    gVolume = volume0to127;
}

void i2sWriteStereo16(const int16_t *interleavedLR, size_t frameCount) {
    uint8_t vol = gVolume;
    const size_t CHUNK_FRAMES = 256;  // 256 frames * 2ch * 2B = 1KB, well under stack budget
    int16_t buf[CHUNK_FRAMES * 2];

    size_t done = 0;
    while (done < frameCount) {
        size_t chunk = frameCount - done;
        if (chunk > CHUNK_FRAMES) chunk = CHUNK_FRAMES;

        for (size_t i = 0; i < chunk * 2; i++) {
            int32_t scaled = (int32_t)interleavedLR[done * 2 + i] * vol / 127;
            if (scaled > INT16_MAX) scaled = INT16_MAX;
            if (scaled < INT16_MIN) scaled = INT16_MIN;
            buf[i] = (int16_t)scaled;
        }

        size_t bytes_written = 0;
        i2s_write(I2S_NUM_0, buf, chunk * 4, &bytes_written, portMAX_DELAY);
        done += chunk;
    }
}
