// Ported from final-form/audio_decoder.cpp, with the playlist.h/screens.h
// couplings it wasn't supposed to have (per its own header's "no display
// dependency" contract) dropped: need_next_track is now a local flag (no
// playlist module exists here), and seekToRatio() no longer redraws a
// progress bar directly.
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "audio_decoder.h"

#define READ_BUF_SIZE 2048
#define PCM_QUEUE_SIZE 2048

static mp3dec_t mp3d;
static mp3dec_frame_info_t info;
static TaskHandle_t mp3DecoderTaskHandle = NULL;

QueueHandle_t pcm_frame_queue = NULL;
File audioFile;
SemaphoreHandle_t audioFileMutex = NULL;

volatile bool is_playing = false;
volatile bool decoder_reset_requested = false;
volatile size_t g_file_size = 0;
volatile size_t g_file_bytes_read = 0;
volatile bool g_need_next_track = false;

void audioDecoderSetup() {
    pcm_frame_queue = xQueueCreate(PCM_QUEUE_SIZE, sizeof(Frame));
    audioFileMutex = xSemaphoreCreateMutex();
    if (!pcm_frame_queue || !audioFileMutex) {
        Serial.println("[Audio] memory error: could not allocate queue/mutex");
        while (true) delay(1000);
    }
}

void startDecoderTask() {
    BaseType_t decoderResult = xTaskCreatePinnedToCore(
        mp3DecoderTask, "MP3Dec", 20480, NULL, 3, &mp3DecoderTaskHandle, 1);
    Serial.printf("[Audio] decoder task: %s\n", decoderResult == pdPASS ? "started" : "FAILED");
}

int32_t get_sound_data(Frame *data, int32_t frame_count) {
    if (pcm_frame_queue == NULL) return 0;
    if (!is_playing) {
        memset(data, 0, frame_count * sizeof(Frame));
        return frame_count;
    }
    int32_t fetched = 0;
    while (fetched < frame_count) {
        if (xQueueReceive(pcm_frame_queue, &data[fetched], 0) == pdTRUE) {
            fetched++;
        } else break;
    }
    // Fill a temporarily empty callback frame with silence so A2DP never sees a short frame.
    if (fetched < frame_count) {
        memset(&data[fetched], 0, (frame_count - fetched) * sizeof(Frame));
    }
    return frame_count;
}

void mp3DecoderTask(void *pvParameters) {
    // On the task's own 20KB stack, not static .bss -- kode-optimasi's combined
    // WiFi+BT+LVGL+TFT_eSPI static footprint left only ~2.5KB of DRAM headroom,
    // not enough for these ~11KB buffers (final-form had more headroom to spare).
    uint8_t mp3_read_buf[READ_BUF_SIZE];
    int16_t pcm_output_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];
    mp3dec_init(&mp3d);
    size_t read_offset = 0;
    size_t read_len = 0;

    uint32_t last_hz = 0;
    uint32_t step = 65536; // Fixed-point resampling step.

    while (true) {
        if (decoder_reset_requested) {
            read_offset = 0;
            read_len = 0;
            mp3dec_init(&mp3d);
            decoder_reset_requested = false;
        }
        if (!is_playing) { vTaskDelay(100 / portTICK_PERIOD_MS); continue; }

        if (uxQueueMessagesWaiting(pcm_frame_queue) > (PCM_QUEUE_SIZE - 512)) {
            vTaskDelay(pdMS_TO_TICKS(15));
            continue;
        }

        if (read_len - read_offset < 1024) {
            size_t bytesRead = 0;
            if (audioFileMutex && xSemaphoreTake(audioFileMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
                if (audioFile && audioFile.available()) {
                    if (read_offset > 0) {
                        memmove(mp3_read_buf, mp3_read_buf + read_offset, read_len - read_offset);
                        read_len -= read_offset;
                        read_offset = 0;
                    }
                    bytesRead = audioFile.read(mp3_read_buf + read_len, READ_BUF_SIZE - read_len);
                    read_len += bytesRead;
                    g_file_bytes_read += bytesRead;
                }
                xSemaphoreGive(audioFileMutex);
            }
        }

        int samples = mp3dec_decode_frame(&mp3d, mp3_read_buf + read_offset, read_len - read_offset, pcm_output_buffer, &info);

        if (samples > 0) {
            read_offset += info.frame_bytes;

            if (info.hz != last_hz && info.hz > 0) {
                last_hz = info.hz;
                step = ((uint32_t)info.hz << 16) / 44100;
            }

            if (last_hz != 44100) {
                uint32_t acc = 0;
                while ((acc >> 16) < (uint32_t)(samples - 1)) {
                    uint32_t idx = acc >> 16;
                    uint32_t frac = acc & 0xFFFF; // Fractional interpolation position.

                    Frame f;
                    if (info.channels == 2) {
                        int32_t s1_l = pcm_output_buffer[idx * 2];
                        int32_t s2_l = pcm_output_buffer[(idx + 1) * 2];
                        f.channel1 = s1_l + (((s2_l - s1_l) * (int32_t)frac) >> 16);

                        int32_t s1_r = pcm_output_buffer[idx * 2 + 1];
                        int32_t s2_r = pcm_output_buffer[(idx + 1) * 2 + 1];
                        f.channel2 = s1_r + (((s2_r - s1_r) * (int32_t)frac) >> 16);
                    } else {
                        int32_t s1 = pcm_output_buffer[idx];
                        int32_t s2 = pcm_output_buffer[idx + 1];
                        f.channel1 = f.channel2 = s1 + (((s2 - s1) * (int32_t)frac) >> 16);
                    }
                    xQueueSend(pcm_frame_queue, &f, portMAX_DELAY);
                    acc += step;
                }
            } else {
                for (int i = 0; i < samples; i++) {
                    Frame f;
                    if (info.channels == 2) {
                        f.channel1 = pcm_output_buffer[i * 2];
                        f.channel2 = pcm_output_buffer[i * 2 + 1];
                    } else {
                        f.channel1 = f.channel2 = pcm_output_buffer[i];
                    }
                    xQueueSend(pcm_frame_queue, &f, portMAX_DELAY);
                }
            }
        } else {
            if (info.frame_bytes > 0) {
                read_offset += info.frame_bytes;
            } else {
                bool fileEnded = true;
                if (audioFileMutex && xSemaphoreTake(audioFileMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
                    fileEnded = !audioFile || !audioFile.available();
                    xSemaphoreGive(audioFileMutex);
                }
                if (fileEnded) {
                    g_need_next_track = true;
                    vTaskDelay(pdMS_TO_TICKS(100));
                } else {
                    if (read_offset + 1 < read_len) {
                        read_offset++;
                    } else {
                        read_offset = 0;
                        read_len = 0;
                    }
                }
            }
        }
    }
}

bool audioDecoderOpenTrack(const char *path) {
    if (audioFileMutex) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
    if (audioFile) audioFile.close();
    audioFile = SD.open(path, FILE_READ);
    bool opened = audioFile && !audioFile.isDirectory() && audioFile.size() >= 512;
    if (opened) {
        g_file_size = audioFile.size();
        g_file_bytes_read = 0;
    } else {
        if (audioFile) audioFile.close();
        g_file_size = 0;
    }
    if (audioFileMutex) xSemaphoreGive(audioFileMutex);
    decoder_reset_requested = true;
    if (pcm_frame_queue) xQueueReset(pcm_frame_queue);
    return opened;
}

void seekToRatio(float ratio) {
    if (g_file_size == 0 || !audioFile) return;

    bool was_playing = is_playing;
    is_playing = false;
    vTaskDelay(pdMS_TO_TICKS(40));

    size_t target_pos = (size_t)(ratio * g_file_size);
    if (audioFileMutex) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
    bool seekOk = audioFile.seek(target_pos);
    if (audioFileMutex) xSemaphoreGive(audioFileMutex);
    if (!seekOk) {
        Serial.println("[SD] Seek failed");
        is_playing = was_playing;
        return;
    }
    g_file_bytes_read = target_pos;

    xQueueReset(pcm_frame_queue);

    is_playing = was_playing;
}
