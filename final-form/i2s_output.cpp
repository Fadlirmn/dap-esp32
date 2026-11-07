// Wired I2S DAC output. Shared by Local Player (drains audio_decoder.cpp's
// pcm_frame_queue itself, via i2sOutputDrainTask) and BT Receiver (bt_receiver.cpp
// calls i2sWriteStereo16() directly from its own A2DP sink callback -- no task
// of ours runs in that mode). See i2s_output.h for the contract.
#include <Arduino.h>
#include <driver/i2s.h>
#include "i2s_output.h"
#include "pins.h"
#include "audio_decoder.h"  // pcm_frame_queue, is_playing, Frame
#include "nav.h"            // operatingMode

static volatile uint8_t gVolume = 127;
static TaskHandle_t i2sDrainTaskHandle = NULL;

static void i2sOutputDrainTask(void *pvParameters) {
    Frame f;
    int16_t pair[2];
    while (true) {
        if (!is_playing) { vTaskDelay(100 / portTICK_PERIOD_MS); continue; }
        if (xQueueReceive(pcm_frame_queue, &f, portMAX_DELAY) == pdTRUE) {
            pair[0] = f.channel1;
            pair[1] = f.channel2;
            i2sWriteStereo16(pair, 1);
        }
    }
}

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

    if (operatingMode == MODE_LOCAL_PLAYER && i2sDrainTaskHandle == NULL) {
        xTaskCreatePinnedToCore(i2sOutputDrainTask, "I2SOut", 4096, NULL, 3, &i2sDrainTaskHandle, 1);
    }
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
