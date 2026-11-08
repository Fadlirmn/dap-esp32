// Screen rendering, ported from old/Stream32-ESP-Player.ino lines 721-1162.
#pragma once
#include <Arduino.h>

void drawCoverArt(int x, int y, int w, int h);

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

// New per the dap-ui-design redesign (nav.h's new UIState values).
void drawPlaylistsScreen();
void drawPlaylistDetailScreen();
void drawModeSelectScreen();
void drawModeSwitchingScreen();
void drawBTReceiverScreen();
void drawTransferFileScreen();

void drawCurrentStateUI();  // dispatches on nav.h's currentState

// Partial Player-screen redraws (avoid a full repaint on every tick).
void updateLikeButton();
void updatePlayPauseButton();
void updateProgressBar(bool forceRedraw);
void updateVolumeUI();

// Floating auto-hiding volume overlay (nav.h's lastVolumeChangeAt/
// checkVolumeOverlayTimeout): draw it over whatever screen is current.
void drawVolumeOverlay();

// Low-battery dialog + SD-error toast (nav.h's lowBatteryOverlayActive/
// sdErrorOverlayActive): drawCurrentStateUI() paints whichever is active
// over the current screen, every redraw, until dismissed/resolved.
void drawLowBatteryOverlay();
void drawSDErrorToast();
