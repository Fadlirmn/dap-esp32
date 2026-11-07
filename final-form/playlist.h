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

// .m3u playlists, read lazily from /Playlists (not cached beyond the name
// index -- these are small, infrequent reads, not a hot path).
constexpr int MAX_PLAYLISTS = 20;
extern String playlistNames[MAX_PLAYLISTS];  // display name (filename minus .m3u), no path
extern int totalPlaylists;

void loadPlaylistIndex();                          // scan /Playlists for *.m3u, populate playlistNames/totalPlaylists
int playlistTrackCount(int plIndex);                // # entries in that .m3u, or 0 if invalid/missing
String playlistTrackDisplayName(int plIndex, int trackIdx);
bool playPlaylistTrack(int plIndex, int trackIdx);  // opens+plays it (matches against the /tracks library when the path is found there); caller still does enterState(STATE_PLAYER)
