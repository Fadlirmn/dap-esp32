// Screen rendering, ported from old/Stream32-ESP-Player.ino lines 721-1162.
#pragma once
#include <Arduino.h>

void drawCoverArt(int x, int y, int w, int h);

void drawMainMenu();
void drawSettingsScreen();
void drawPlaybackSettingsScreen();
void drawScreenSettingsScreen();
void drawThemeSettingsScreen();
void drawScreenLocked();
void drawInfoScreen();
void drawResetConfirmScreen();
void drawPlayerScreen();
void drawTrackListScreen(bool onlyLikes);
void drawBTManagerScreen();
void drawBluetoothScanScreen();

void drawCurrentStateUI();  // dispatches on nav.h's currentState

// Partial Player-screen redraws (avoid a full repaint on every tick).
void updateLikeButton();
void updatePlayPauseButton();
void updateProgressBar(bool forceRedraw);
void updateVolumeUI();
