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

// .m3u playlists -------------------------------------------------------

String playlistNames[MAX_PLAYLISTS];
int totalPlaylists = 0;

void loadPlaylistIndex() {
    File dir = SD.open("/Playlists");
    if (!dir) {
        Serial.println("[SD] /Playlists is missing; creating it");
        SD.mkdir("/Playlists");
        return;
    }

    totalPlaylists = 0;
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory()) {
            String fileName = entry.name();
            int slash = fileName.lastIndexOf('/');
            if (slash >= 0) fileName = fileName.substring(slash + 1);
            if (fileName.endsWith(".m3u") || fileName.endsWith(".M3U")) {
                if (totalPlaylists < MAX_PLAYLISTS) {
                    fileName.remove(fileName.length() - 4);
                    playlistNames[totalPlaylists] = fileName;
                    totalPlaylists++;
                } else {
                    Serial.printf("[SD] Skipping playlist, MAX_PLAYLISTS reached: %s\n", fileName.c_str());
                }
            }
        }
        entry.close();
    }
    dir.close();
    Serial.printf("[SD] Playlist index loaded: %d playlist(s)\n", totalPlaylists);
}

// Shared line-walker over playlistNames[plIndex]'s .m3u file, skipping
// blank/#-comment lines per the M3U convention. A bare filename (no leading
// '/') is resolved against /tracks.
//   wantIdx < 0 : returns the total entry count (outPath ignored).
//   wantIdx >= 0: fills *outPath with that entry's resolved path and returns
//                 1, or returns 0 if out of range / file missing (outPath
//                 left untouched).
static int walkM3U(int plIndex, int wantIdx, String *outPath) {
    if (plIndex < 0 || plIndex >= totalPlaylists) return 0;

    String m3uPath = "/Playlists/" + playlistNames[plIndex] + ".m3u";
    File f = SD.open(m3uPath.c_str(), FILE_READ);
    if (!f) return 0;

    int count = 0;
    int result = 0;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        if (wantIdx >= 0 && count == wantIdx) {
            if (outPath) *outPath = line.startsWith("/") ? line : ("/tracks/" + line);
            result = 1;
            break;
        }
        count++;
    }
    f.close();
    return (wantIdx < 0) ? count : result;
}

int playlistTrackCount(int plIndex) {
    return walkM3U(plIndex, -1, nullptr);
}

String playlistTrackDisplayName(int plIndex, int trackIdx) {
    if (trackIdx < 0) return "";
    String path;
    if (!walkM3U(plIndex, trackIdx, &path)) return "";
    return trackDisplayName(path);
}

// Fallback for a playlist entry not present in playlist[]: opens an explicit
// path directly into audioFile. Same validation/mutex pattern as
// openCurrentTrack(), but takes the path as a parameter instead of reading
// it off playlist[current_track_index].
static bool openTrackPath(const String &path) {
    if (audioFileMutex) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
    if (audioFile) audioFile.close();

    audioFile = SD.open(path.c_str(), FILE_READ);
    bool opened = audioFile && !audioFile.isDirectory() && audioFile.size() >= 512;
    if (opened) {
        g_file_size = audioFile.size();
        g_file_bytes_read = 0;
        Serial.printf("[SD] Opened playlist track: %s (%u bytes)\n",
                      path.c_str(), (unsigned int)g_file_size);
    } else {
        if (audioFile) audioFile.close();
        g_file_size = 0;
        g_file_bytes_read = 0;
        Serial.printf("[SD] Could not open a valid MP3: %s\n", path.c_str());
    }
    if (audioFileMutex) xSemaphoreGive(audioFileMutex);
    return opened;
}

// Shared tail end of playPlaylistTrack(): same decoder-reset sequencing as
// switchTrack(), then opens either the matched library index or the
// fallback path directly.
static bool playResolvedTrack(int libraryIndex, const String &fallbackPath) {
    saveResumeState();
    is_playing = false;
    decoder_reset_requested = true;
    vTaskDelay(pdMS_TO_TICKS(50));
    xQueueReset(pcm_frame_queue);

    bool opened;
    if (libraryIndex >= 0) {
        current_track_index = libraryIndex;
        opened = openCurrentTrack();
    } else {
        opened = openTrackPath(fallbackPath);
    }
    is_playing = opened;

    if (currentState == STATE_PLAYER && !screenLocked) {
        drawCurrentStateUI();
    }
    return opened;
}

bool playPlaylistTrack(int plIndex, int trackIdx) {
    if (trackIdx < 0) return false;
    String path;
    if (!walkM3U(plIndex, trackIdx, &path)) return false;

    for (int i = 0; i < total_tracks; i++) {
        if (playlist[i] == path) return playResolvedTrack(i, "");
    }
    return playResolvedTrack(-1, path);
}
