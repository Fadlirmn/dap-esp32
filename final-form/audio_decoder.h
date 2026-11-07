// MP3 decode pipeline: SD file -> minimp3 -> resampled PCM queue -> A2DP callback.
// Ported verbatim from old/Stream32-ESP-Player.ino lines 162-181 and 1154-1303.
// No display dependency.
#pragma once
#include <Arduino.h>
#include <SD.h>
#include "BluetoothA2DPSource.h"  // for the `Frame` type used by get_sound_data

extern QueueHandle_t pcm_frame_queue;
extern File audioFile;
extern SemaphoreHandle_t audioFileMutex;

extern volatile bool is_playing;
extern volatile bool decoder_reset_requested;
extern volatile size_t g_file_size;
extern volatile size_t g_file_bytes_read;

// Creates the PCM queue + file mutex. Must run before btSetup() / startDecoderTask().
void audioDecoderSetup();

// Starts the pinned decoder FreeRTOS task. Call once after a track is opened.
void startDecoderTask();

int32_t get_sound_data(Frame *data, int32_t frame_count);
void mp3DecoderTask(void *pvParameters);

// Seeks the current track to `ratio` (0..1) of its file size.
void seekToRatio(float ratio);
