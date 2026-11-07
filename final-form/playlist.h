// SD playlist, likes, and resume state. Ported verbatim from
// old/Stream32-ESP-Player.ino lines 319-453 (no display-library dependency).
#pragma once
#include <Arduino.h>

constexpr int MAX_TRACKS = 50;
extern String playlist[MAX_TRACKS];
extern bool isLiked[MAX_TRACKS];
extern int total_tracks;
extern int current_track_index;
extern volatile bool need_next_track;

String trackDisplayName(const String &path);
void loadPlaylist();   // scans /tracks, sorts by display name
void loadLikes();      // reads /tracks/likes.txt into isLiked[]
void saveLikes();
void toggleLike();
void saveResumeState();      // persists current track + byte offset (Preferences)
bool openCurrentTrack();     // opens playlist[current_track_index] into audioFile
void switchTrack(int direction);  // direction: -1 prev, 0 reload current, +1 next
