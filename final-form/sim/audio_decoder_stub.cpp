// Sim replacement for ../audio_decoder.cpp: the real one needs minimp3.h,
// which isn't vendored in this repo. Display-only MVP doesn't decode audio,
// it just needs these globals/functions to exist for screens.cpp/nav.cpp to link.
#include "audio_decoder.h"

QueueHandle_t pcm_frame_queue = nullptr;
File audioFile;
SemaphoreHandle_t audioFileMutex = nullptr;
volatile bool is_playing = false;
volatile bool decoder_reset_requested = false;
volatile size_t g_file_size = 0;
volatile size_t g_file_bytes_read = 0;

void audioDecoderSetup() {
    pcm_frame_queue = xQueueCreate(1, sizeof(Frame));
    audioFileMutex = xSemaphoreCreateMutex();
}
void startDecoderTask() {}

int32_t get_sound_data(Frame *frame, int32_t frame_count) {
    for (int32_t i = 0; i < frame_count; i++) frame[i] = {0, 0};
    return frame_count;
}

void mp3DecoderTask(void *) {}
void seekToRatio(float) {}
