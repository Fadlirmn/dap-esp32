// Ported verbatim from old/Stream32-ESP-Player.ino lines 319-475.
#include "playlist.h"
#include <SD.h>
#include <Preferences.h>
#include "audio_decoder.h"
#include "nav.h"
#include "screens.h"

extern Preferences preferences;

constexpr const char *LIKES_FILE_PATH = "/tracks/likes.txt";

String playlist[MAX_TRACKS];
bool isLiked[MAX_TRACKS];
int total_tracks = 0;
int current_track_index = 0;
volatile bool need_next_track = false;

String trackDisplayName(const String &path) {
    String name = path;
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    if (name.endsWith(".mp3") || name.endsWith(".MP3")) name.remove(name.length() - 4);
    return name;
}

static void sortPlaylist() {
    for (int i = 0; i < total_tracks - 1; i++) {
        for (int j = i + 1; j < total_tracks; j++) {
            String a = trackDisplayName(playlist[i]);
            String b = trackDisplayName(playlist[j]);
            a.toLowerCase();
            b.toLowerCase();
            if (a > b) {
                String path = playlist[i];
                playlist[i] = playlist[j];
                playlist[j] = path;
            }
        }
    }
}

void loadPlaylist() {
    File dir = SD.open("/tracks");
    if (!dir) {
        Serial.println("[SD] /tracks is missing; creating it");
        SD.mkdir("/tracks");
        return;
    }

    total_tracks = 0;
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory()) {
            String fileName = entry.name();
            if (fileName.endsWith(".mp3") || fileName.endsWith(".MP3")) {
                if (entry.size() >= 512 && total_tracks < MAX_TRACKS) {
                    playlist[total_tracks] = "/tracks/" + fileName;
                    isLiked[total_tracks] = false;
                    total_tracks++;
                } else {
                    Serial.printf("[SD] Skipping invalid or too small MP3: %s\n", fileName.c_str());
                }
            }
        }
        entry.close();
    }
    dir.close();
    sortPlaylist();
    Serial.printf("[SD] Playlist loaded: %d track(s)\n", total_tracks);
}

void loadLikes() {
    if (!SD.exists(LIKES_FILE_PATH)) return;
    File f = SD.open(LIKES_FILE_PATH, FILE_READ);
    if (!f) {
        Serial.println("[SD] Could not open likes file");
        return;
    }

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        for (int i = 0; i < total_tracks; i++) {
            if (playlist[i] == line) {
                isLiked[i] = true;
                break;
            }
        }
    }
    f.close();
}

void saveLikes() {
    const char *tmpPath = "/tracks/likes.tmp";
    File f = SD.open(tmpPath, FILE_WRITE);
    if (!f) {
        Serial.println("[SD] Could not save likes");
        return;
    }

    for (int i = 0; i < total_tracks; i++) {
        if (isLiked[i]) f.println(playlist[i]);
    }
    f.close();
    SD.remove(LIKES_FILE_PATH);
    if (!SD.rename(tmpPath, LIKES_FILE_PATH)) {
        Serial.println("[SD] Could not replace likes file");
    }
}

void toggleLike() {
    if (total_tracks == 0) return;
    isLiked[current_track_index] = !isLiked[current_track_index];
    saveLikes();
    updateLikeButton();
}

void saveResumeState() {
    if (total_tracks == 0 || g_file_size == 0) return;
    preferences.begin("resume_pref", false);
    preferences.putString("track", playlist[current_track_index]);
    preferences.putUInt("bytes", (uint32_t)g_file_bytes_read);
    preferences.end();
}

bool openCurrentTrack() {
    if (current_track_index < 0 || current_track_index >= total_tracks) return false;

    if (audioFileMutex) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
    if (audioFile) audioFile.close();

    audioFile = SD.open(playlist[current_track_index].c_str(), FILE_READ);
    bool opened = audioFile && !audioFile.isDirectory() && audioFile.size() >= 512;
    if (opened) {
        g_file_size = audioFile.size();
        g_file_bytes_read = 0;
        Serial.printf("[SD] Opened track: %s (%u bytes)\n",
                      playlist[current_track_index].c_str(), (unsigned int)g_file_size);
    } else {
        if (audioFile) audioFile.close();
        g_file_size = 0;
        g_file_bytes_read = 0;
        Serial.printf("[SD] Could not open a valid MP3: %s\n",
                      playlist[current_track_index].c_str());
    }
    if (audioFileMutex) xSemaphoreGive(audioFileMutex);
    return opened;
}

void switchTrack(int direction) {
    if (total_tracks == 0) return;
    saveResumeState();

    bool wasPlaying = is_playing;
    is_playing = false;
    decoder_reset_requested = true;
    vTaskDelay(pdMS_TO_TICKS(50));

    xQueueReset(pcm_frame_queue);

    current_track_index += direction;
    if (current_track_index >= total_tracks) current_track_index = 0;
    if (current_track_index < 0) current_track_index = total_tracks - 1;

    is_playing = wasPlaying && openCurrentTrack();

    if (currentState == STATE_PLAYER && !screenLocked) {
        drawCurrentStateUI();
    }
}
