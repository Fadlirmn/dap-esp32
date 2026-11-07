// No-op stand-in for ESP-IDF's legacy I2S driver: no real I2S peripheral on
// desktop. Just enough shape for i2s_output.cpp to compile and link unmodified.
#pragma once
#include <cstdint>
#include <cstddef>

typedef int i2s_port_t;
#define I2S_NUM_0 0

typedef int i2s_mode_t;
#define I2S_MODE_MASTER 1
#define I2S_MODE_TX 2

typedef int i2s_bits_per_sample_t;
#define I2S_BITS_PER_SAMPLE_16BIT 16

typedef int i2s_channel_fmt_t;
#define I2S_CHANNEL_FMT_RIGHT_LEFT 0

typedef int i2s_comm_format_t;
#define I2S_COMM_FORMAT_STAND_I2S 0

#define I2S_PIN_NO_CHANGE (-1)

struct i2s_config_t {
    i2s_mode_t mode = 0;
    int sample_rate = 0;
    i2s_bits_per_sample_t bits_per_sample = 0;
    i2s_channel_fmt_t channel_format = 0;
    i2s_comm_format_t communication_format = 0;
    int intr_alloc_flags = 0;
    int dma_desc_num = 0;
    int dma_frame_num = 0;
    bool use_apll = false;
    bool tx_desc_auto_clear = false;
};

struct i2s_pin_config_t {
    int mck_io_num = I2S_PIN_NO_CHANGE;
    int bck_io_num = I2S_PIN_NO_CHANGE;
    int ws_io_num = I2S_PIN_NO_CHANGE;
    int data_out_num = I2S_PIN_NO_CHANGE;
    int data_in_num = I2S_PIN_NO_CHANGE;
};

inline int i2s_driver_install(i2s_port_t, const i2s_config_t *, int, void *) { return 0; }
inline int i2s_set_pin(i2s_port_t, const i2s_pin_config_t *) { return 0; }
inline int i2s_write(i2s_port_t, const void *, size_t bytesToWrite, size_t *bytesWritten, uint32_t) {
    if (bytesWritten) *bytesWritten = bytesToWrite;
    return 0;
}
