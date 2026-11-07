// Ported from old/Stream32-ESP-Player.ino lines 721-1134 (drawXScreen functions +
// drawCoverArt + the Player-screen partial-redraw helpers). TFT_eSPI text-datum
// calls become gfxDrawString/truncateToWidth; everything else (fillRoundRect,
// drawRoundRect, fillCircle, drawLine...) is unchanged Adafruit_GFX API.
//
// Added vs. the original: drawFocusRing() calls marking nav.h's `focusIndex` on
// every cursor-navigable screen, since encoder+button input has no finger to tap.
#include "screens.h"
#include <Adafruit_ILI9341.h>
#include <SD.h>
#include <math.h>
#include "theme.h"
#include "layout.h"
#include "gfx_helpers.h"
#include "icons.h"
#include "bluetooth.h"
#include "playlist.h"
#include "audio_decoder.h"
#include "nav.h"

extern Adafruit_ILI9341 tft;

static constexpr const char *PLAYER_VERSION = "0.0.10-beta";

void drawCoverArt(int x, int y, int w, int h) {
    tft.fillRect(x, y, w, h, COLOR_BG);
    if (total_tracks == 0) {
        drawVectorMusicNote(x, y, w, h, COLOR_ACCENT);
        return;
    }

    String rawPath = playlist[current_track_index];
    rawPath.replace(".mp3", ".raw");
    rawPath.replace(".MP3", ".raw");

    if (!SD.exists(rawPath)) {
        drawVectorMusicNote(x, y, w, h, COLOR_ACCENT);
        return;
    }

    File imgFile = SD.open(rawPath, FILE_READ);
    if (!imgFile) {
        drawVectorMusicNote(x, y, w, h, COLOR_ACCENT);
        return;
    }

    uint16_t lineBuffer[110];
    for (int row = 0; row < h; row++) {
        imgFile.read((uint8_t *)lineBuffer, w * 2);
        tft.drawRGBBitmap(x, y + row, lineBuffer, w, 1);
    }
    imgFile.close();
    maskCoverCorners(x, y, w, h, 12);
}

void drawMainMenu() {
    tft.fillScreen(COLOR_BG);
    drawHeader("STREAM 32 PLAYER", false);
    const char *menuLabelsNew[] = {"PLAYER", "TRACKS", "LIKES", "SETTINGS"};
    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = col == 0 ? MENU_BTN_X_LEFT : MENU_BTN_X_RIGHT;
        int y = MENU_BTN_Y[row];

        tft.fillRoundRect(x, y, MENU_BTN_W, MENU_BTN_H, 10, COLOR_PANEL);
        tft.drawRoundRect(x, y, MENU_BTN_W, MENU_BTN_H, 10, COLOR_PANEL_BORDER);
        uint16_t iconColor = COLOR_ACCENT;
        if (i == 0) drawPlayIcon(x + MENU_BTN_W / 2, y + 25, 22, iconColor);
        else if (i == 1) {
            int cx = x + MENU_BTN_W / 2, cy = y + 25;
            for (int r = -1; r <= 1; r++) {
                tft.drawFastHLine(cx - 2, cy + r * 7, 15, iconColor);
                tft.fillCircle(cx - 10, cy + r * 7, 2, iconColor);
            }
        } else if (i == 2) {
            drawHeartIcon(x + MENU_BTN_W / 2, y + 25, 11, iconColor);
        } else {
            int cx = x + MENU_BTN_W / 2, cy = y + 25;
            tft.drawCircle(cx, cy, 4, iconColor);
            for (int tick = 0; tick < 8; tick++) {
                float angle = tick * 0.7854f;
                tft.drawLine(cx + (int)(8 * cos(angle)), cy + (int)(8 * sin(angle)),
                             cx + (int)(11 * cos(angle)), cy + (int)(11 * sin(angle)), iconColor);
            }
        }
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, menuLabelsNew[i], x + MENU_BTN_W / 2, y + 51, 1, GFX_MC);
        if (focusIndex == i) drawFocusRing(x, y, MENU_BTN_W, MENU_BTN_H);
    }
    tft.fillRoundRect(10, 208, SCREEN_W - 20, 62, 10, COLOR_PANEL);
    tft.drawRoundRect(10, 208, SCREEN_W - 20, 62, 10, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, "NOW PLAYING", 20, 216, 1, GFX_TL);
    String homeTrack = total_tracks > 0 ? trackDisplayName(playlist[current_track_index]) : "No tracks on SD";
    homeTrack = truncateToWidth(tft, homeTrack, 175, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, homeTrack, 20, 234, fontToSize(2), GFX_TL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, is_playing ? "Playing" : "Paused", 20, 252, fontToSize(2), GFX_TL);
    drawBottomNavigation(0);
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SETTINGS");
    const char *labels[] = {"Bluetooth", "Screen", "Playback", "Theme", "Information", "Reset settings"};
    for (uint8_t i = 0; i < 6; i++) {
        int y = 48 + i * 34;
        tft.fillRoundRect(10, y, SCREEN_W - 20, 28, 8, COLOR_PANEL);
        tft.drawRoundRect(10, y, SCREEN_W - 20, 28, 8, COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, labels[i], 20, y + 8, fontToSize(2), GFX_TL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        gfxDrawString(tft, ">", 218, y + 10, fontToSize(2), GFX_MC);
        if (focusIndex == i) drawFocusRing(10, y, SCREEN_W - 20, 28);
    }
    drawBottomNavigation(4);
}

void drawPlaybackSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("PLAYBACK");
    tft.fillRoundRect(10, 55, SCREEN_W - 20, 38, 8, COLOR_PANEL);
    tft.drawRoundRect(10, 55, SCREEN_W - 20, 38, 8, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "Auto-play on connect", 20, 67, fontToSize(2), GFX_TL);
    tft.setTextColor(autoPlayEnabled ? COLOR_ACCENT : COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, autoPlayEnabled ? "ON" : "OFF", 205, 75, fontToSize(2), GFX_MC);
    if (focusIndex == 0) drawFocusRing(10, 55, SCREEN_W - 20, 38);
    drawBottomNavigation(4);
}

// Brightness row dropped: backlight is hard-wired to 3V3, no PWM GPIO free (pins.h).
// Timeout is now the only control, so the two-level row/edit focus the original
// plan sketched is unnecessary -- focusIndex walks the 4 chips directly.
void drawScreenSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SCREEN");
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "Screen timeout", 18, 60, fontToSize(2), GFX_TL);
    const char *timeoutLabel = screenTimeoutMinutes == 0 ? "OFF" : (screenTimeoutMinutes == 1 ? "1 min" : (screenTimeoutMinutes == 5 ? "5 min" : "10 min"));
    tft.setTextColor(COLOR_ACCENT);
    gfxDrawString(tft, timeoutLabel, 220, 60, fontToSize(2), GFX_TR);
    const char *options[] = {"1 min", "5 min", "10 min", "OFF"};
    const uint8_t values[] = {1, 5, 10, 0};
    for (uint8_t i = 0; i < 4; i++) {
        int x = 12 + i * 57;
        bool selected = screenTimeoutMinutes == values[i];
        tft.fillRoundRect(x, 95, 51, 30, 7, selected ? COLOR_ACCENT_DIM : COLOR_PANEL);
        tft.drawRoundRect(x, 95, 51, 30, 7, selected ? COLOR_ACCENT : COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, options[i], x + 25, 103, 1, GFX_MC);
        if (focusIndex == i) drawFocusRing(x, 95, 51, 30);
    }
    drawBottomNavigation(4);
}

void drawThemeSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("THEME");
    const char *names[] = {"Classic Green", "Blue", "Purple", "Amber", "Rose"};
    for (uint8_t i = 0; i < 5; i++) {
        int y = 48 + i * 44;
        bool selected = themeIndex == i;
        tft.fillRoundRect(18, y, SCREEN_W - 36, 36, 9, selected ? COLOR_ACCENT_DIM : COLOR_PANEL);
        tft.drawRoundRect(18, y, SCREEN_W - 36, 36, 9, selected ? COLOR_ACCENT : COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, names[i], SCREEN_W / 2, y + 10, fontToSize(2), GFX_MC);
        if (focusIndex == i) drawFocusRing(18, y, SCREEN_W - 36, 36);
    }
    drawBottomNavigation(4);
}

// Original said "TAP / TO WAKE" (touch-only hardware). Touch is optional here, so
// the primary wake path is any button/encoder press; text says so but a tap still
// works too when ENABLE_TOUCH is on.
void drawScreenLocked() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    gfxDrawString(tft, "LOCKED", SCREEN_W / 2, SCREEN_H / 2 - 16, fontToSize(4), GFX_MC);
    tft.setTextColor(ILI9341_LIGHTGREY);
    gfxDrawString(tft, "PRESS ANY BUTTON", SCREEN_W / 2, SCREEN_H / 2 + 18, fontToSize(2), GFX_MC);
}

void drawInfoScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("INFORMATION");
    tft.setTextColor(COLOR_ACCENT);
    gfxDrawString(tft, "Stream32 Player", SCREEN_W / 2, 88, fontToSize(4), GFX_MC);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, String("Version ") + PLAYER_VERSION, SCREEN_W / 2, 125, fontToSize(2), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, "Developed with love", SCREEN_W / 2, 165, fontToSize(2), GFX_MC);
    tft.setTextColor(COLOR_HEART_ACTIVE);
    gfxDrawString(tft, "by Stream32 Team", SCREEN_W / 2, 190, fontToSize(2), GFX_MC);
    drawBottomNavigation(4);
}

void drawResetConfirmScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SETTINGS");
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "Reset all settings?", SCREEN_W / 2, 105, fontToSize(2), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, "Bluetooth and volume", SCREEN_W / 2, 128, fontToSize(2), GFX_MC);
    gfxDrawString(tft, "will return to defaults", SCREEN_W / 2, 146, fontToSize(2), GFX_MC);
    tft.fillRoundRect(18, 178, 96, 34, 9, COLOR_PANEL);
    tft.drawRoundRect(18, 178, 96, 34, 9, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "CANCEL", 66, 187, fontToSize(2), GFX_MC);
    if (focusIndex == 0) drawFocusRing(18, 178, 96, 34);
    tft.fillRoundRect(126, 178, 96, 34, 9, COLOR_ACCENT_DIM);
    tft.drawRoundRect(126, 178, 96, 34, 9, COLOR_ACCENT);
    gfxDrawString(tft, "RESET", 174, 187, fontToSize(2), GFX_MC);
    if (focusIndex == 1) drawFocusRing(126, 178, 96, 34);
    drawBottomNavigation(4);
}

void drawPlayerScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("NOW PLAYING");

    drawCoverArt(PL_ART_X, PL_ART_Y, PL_ART_W, PL_ART_H);

    String rawName = (total_tracks > 0) ? trackDisplayName(playlist[current_track_index]) : "No tracks!";

    String artist = "Unknown Artist";
    String title = rawName;

    int hyphenIdx = rawName.indexOf(" - ");
    if (hyphenIdx != -1) {
        artist = rawName.substring(0, hyphenIdx);
        title = rawName.substring(hyphenIdx + 3);
    }

    title = truncateToWidth(tft, title, SCREEN_W - PL_INFO_X - 35, 4);
    artist = truncateToWidth(tft, artist, SCREEN_W - PL_INFO_X - 35, 2);

    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, title, PL_INFO_X, PL_TITLE_Y, fontToSize(4), GFX_TL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, artist, PL_INFO_X, PL_ARTIST_Y, fontToSize(2), GFX_TL);

    updateLikeButton();

    tft.fillRoundRect(PL_PROGRESS_X, PL_PROGRESS_Y, PL_PROGRESS_W, PL_PROGRESS_H, PL_PROGRESS_H / 2, COLOR_TRACK_BG);
    updateProgressBar(true);

    tft.fillRoundRect(PL_PREV_X, PL_BTN_Y, PL_PREV_W, PL_BTN_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_PREV_X, PL_BTN_Y, PL_PREV_W, PL_BTN_H, 10, COLOR_PANEL_BORDER);
    drawPrevIcon(PL_PREV_X + PL_PREV_W / 2, PL_BTN_Y + PL_BTN_H / 2, 16, COLOR_TEXT_PRIMARY);

    tft.fillRoundRect(PL_NEXT_X, PL_BTN_Y, PL_NEXT_W, PL_BTN_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_NEXT_X, PL_BTN_Y, PL_NEXT_W, PL_BTN_H, 10, COLOR_PANEL_BORDER);
    drawNextIcon(PL_NEXT_X + PL_NEXT_W / 2, PL_BTN_Y + PL_BTN_H / 2, 16, COLOR_TEXT_PRIMARY);

    updatePlayPauseButton();

    tft.fillRoundRect(PL_VOLDOWN_X, PL_VOL_Y, PL_VOLDOWN_W, PL_VOL_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_VOLDOWN_X, PL_VOL_Y, PL_VOLDOWN_W, PL_VOL_H, 10, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "-", PL_VOLDOWN_X + PL_VOLDOWN_W / 2, PL_VOL_Y + PL_VOL_H / 2 - 7, fontToSize(4), GFX_MC);

    tft.fillRoundRect(PL_VOLUP_X, PL_VOL_Y, PL_VOLUP_W, PL_VOL_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_VOLUP_X, PL_VOL_Y, PL_VOLUP_W, PL_VOL_H, 10, COLOR_PANEL_BORDER);
    gfxDrawString(tft, "+", PL_VOLUP_X + PL_VOLUP_W / 2, PL_VOL_Y + PL_VOL_H / 2 - 7, fontToSize(4), GFX_MC);

    updateVolumeUI();
    drawBottomNavigation(0);
}

void updateLikeButton() {
    if (currentState != STATE_PLAYER) return;
    tft.fillRect(PL_HEART_CX - PL_HEART_R - 2, PL_HEART_CY - PL_HEART_R - 2, (PL_HEART_R + 2) * 2, (PL_HEART_R + 2) * 2, COLOR_BG);
    bool liked = (total_tracks > 0) ? isLiked[current_track_index] : false;
    drawHeartIcon(PL_HEART_CX, PL_HEART_CY, PL_HEART_R, liked ? COLOR_HEART_ACTIVE : COLOR_HEART_INACTIVE);
}

void updatePlayPauseButton() {
    if (currentState != STATE_PLAYER) return;
    tft.fillRoundRect(PL_PLAY_X, PL_BTN_Y, PL_PLAY_W, PL_BTN_H, 10, is_playing ? COLOR_ACCENT_DIM : COLOR_ACCENT);
    tft.drawRoundRect(PL_PLAY_X, PL_BTN_Y, PL_PLAY_W, PL_BTN_H, 10, COLOR_PANEL_BORDER);
    if (is_playing) {
        drawPauseIcon(PL_PLAY_X + PL_PLAY_W / 2, PL_BTN_Y + PL_BTN_H / 2, 16, COLOR_BG);
    } else {
        drawPlayIcon(PL_PLAY_X + PL_PLAY_W / 2, PL_BTN_Y + PL_BTN_H / 2, 16, COLOR_BG);
    }
}

static int last_drawn_progress_pct = -1;
void updateProgressBar(bool forceRedraw) {
    if (currentState != STATE_PLAYER) return;
    int pct = 0;
    size_t size = g_file_size;
    size_t readBytes = g_file_bytes_read;
    if (size > 0) {
        pct = (int)((uint64_t)readBytes * 100 / size);
        if (pct > 100) pct = 100;
    }

    if (!forceRedraw && pct == last_drawn_progress_pct) return;
    last_drawn_progress_pct = pct;

    int fillWidth = (PL_PROGRESS_W * pct) / 100;
    if (fillWidth > 0) {
        tft.fillRoundRect(PL_PROGRESS_X, PL_PROGRESS_Y, fillWidth, PL_PROGRESS_H, PL_PROGRESS_H / 2, COLOR_ACCENT);
    }
    if (fillWidth < PL_PROGRESS_W) {
        tft.fillRect(PL_PROGRESS_X + fillWidth, PL_PROGRESS_Y, PL_PROGRESS_W - fillWidth, PL_PROGRESS_H, COLOR_TRACK_BG);
    }
}

void updateVolumeUI() {
    if (currentState != STATE_PLAYER) return;
    tft.fillRect(PL_VOLTEXT_X, PL_VOL_Y, PL_VOLTEXT_W, PL_VOL_H, COLOR_BG);
    int volPercent = (bt_volume * 100) / 127;
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    gfxDrawString(tft, String(volPercent) + "%", PL_VOLTEXT_X + PL_VOLTEXT_W / 2, PL_VOL_Y + PL_VOL_H / 2 - 6, fontToSize(2), GFX_MC);
}

void drawTrackListScreen(bool onlyLikes) {
    tft.fillScreen(COLOR_BG);
    drawHeader(onlyLikes ? "FAVORITES" : "ALL TRACKS");

    int matchingTracks[MAX_TRACKS];
    int matchCount = 0;
    for (int i = 0; i < total_tracks; i++) {
        if (!onlyLikes || isLiked[i]) {
            matchingTracks[matchCount++] = i;
        }
    }

    int page = onlyLikes ? likesList_currentPage : trackList_currentPage;
    int maxPages = (matchCount + LIST_ITEMS_PER_PAGE - 1) / LIST_ITEMS_PER_PAGE;
    if (maxPages == 0) maxPages = 1;

    int startIdx = page * LIST_ITEMS_PER_PAGE;
    int endIdx = min(startIdx + LIST_ITEMS_PER_PAGE, matchCount);

    for (int i = 0; i < LIST_ITEMS_PER_PAGE; i++) {
        int listY = LIST_ITEM_Y_START + i * (LIST_ITEM_H + LIST_ITEM_GAP);
        int itemIdx = startIdx + i;

        if (itemIdx < endIdx) {
            int originalTrackIdx = matchingTracks[itemIdx];
            bool isCurrent = (originalTrackIdx == current_track_index);

            tft.fillRoundRect(10, listY, SCREEN_W - 20, LIST_ITEM_H, 8, isCurrent ? COLOR_PANEL_BORDER : COLOR_PANEL);
            tft.drawRoundRect(10, listY, SCREEN_W - 20, LIST_ITEM_H, 8, isCurrent ? COLOR_ACCENT : COLOR_PANEL_BORDER);

            String name = trackDisplayName(playlist[originalTrackIdx]);
            name = truncateToWidth(tft, name, SCREEN_W - 55, 2);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            gfxDrawString(tft, name, 20, listY + 10, fontToSize(2), GFX_TL);
            if (focusIndex == i) drawFocusRing(10, listY, SCREEN_W - 20, LIST_ITEM_H);
        }
    }

    if (matchCount == 0) {
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        gfxDrawString(tft, onlyLikes ? "No liked tracks" : "No MP3 files", SCREEN_W / 2, 140, fontToSize(2), GFX_MC);
    }

    tft.fillRoundRect(20, 230, 50, 30, 6, COLOR_PANEL);
    tft.drawRoundRect(20, 230, 50, 30, 6, COLOR_PANEL_BORDER);
    drawPrevIcon(45, 245, 14, COLOR_TEXT_SECONDARY);

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, String(page + 1) + "/" + String(maxPages), 120, 245, fontToSize(2), GFX_MC);

    tft.fillRoundRect(170, 230, 50, 30, 6, COLOR_PANEL);
    tft.drawRoundRect(170, 230, 50, 30, 6, COLOR_PANEL_BORDER);
    drawNextIcon(195, 245, 14, COLOR_TEXT_SECONDARY);
    drawBottomNavigation(onlyLikes ? 2 : 1);
}

void drawBTManagerScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BLUETOOTH");
    tft.drawCircle(120, 92, 43, COLOR_PANEL_BORDER);
    tft.drawCircle(120, 92, 37, COLOR_ACCENT_DIM);
    drawBluetoothLogo(120, 92, 42, bt_connected ? COLOR_BT_CONNECTED : COLOR_BT_DISCONNECTED);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, bt_connected ? "Connected" : (bt_connecting ? "Connecting..." : "Disconnected"), SCREEN_W / 2, 145, fontToSize(2), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, savedBTDevice, SCREEN_W / 2, 164, fontToSize(2), GFX_MC);

    tft.fillRoundRect(20, 182, SCREEN_W - 40, 34, 9, COLOR_PANEL);
    tft.drawRoundRect(20, 182, SCREEN_W - 40, 34, 9, COLOR_ACCENT_DIM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, bt_connected ? "DISCONNECT" : "CONNECT", SCREEN_W / 2, 191, fontToSize(2), GFX_MC);
    if (focusIndex == 0) drawFocusRing(20, 182, SCREEN_W - 40, 34);

    tft.fillRoundRect(20, 224, SCREEN_W - 40, 34, 9, COLOR_PANEL);
    tft.drawRoundRect(20, 224, SCREEN_W - 40, 34, 9, COLOR_PANEL_BORDER);
    gfxDrawString(tft, "SCAN DEVICES", SCREEN_W / 2, 233, fontToSize(2), GFX_MC);
    if (focusIndex == 1) drawFocusRing(20, 224, SCREEN_W - 40, 34);

    drawBottomNavigation(4);
}

void drawBluetoothScanScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BLUETOOTH");
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, btScanActive ? "Scanning..." : "Select device", SCREEN_W / 2, 48, fontToSize(2), GFX_MC);
    for (uint8_t i = 0; i < btScanCount; i++) {
        int y = 68 + i * 31;
        tft.fillRoundRect(10, y, SCREEN_W - 20, 26, 6, COLOR_PANEL);
        tft.drawRoundRect(10, y, SCREEN_W - 20, 26, 6, COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, btScanNames[i], 20, y + 7, fontToSize(2), GFX_TL);
        if (focusIndex == i) drawFocusRing(10, y, SCREEN_W - 20, 26);
    }
    drawBottomNavigation(4);
}

void drawCurrentStateUI() {
    last_drawn_progress_pct = -1;
    switch (currentState) {
        case STATE_MENU: drawMainMenu(); break;
        case STATE_PLAYER: drawPlayerScreen(); break;
        case STATE_TRACK_LIST: drawTrackListScreen(false); break;
        case STATE_LIKES_LIST: drawTrackListScreen(true); break;
        case STATE_BT_MANAGER: drawBTManagerScreen(); break;
        case STATE_SETTINGS: drawSettingsScreen(); break;
        case STATE_BT_SCAN: drawBluetoothScanScreen(); break;
        case STATE_SCREEN_SETTINGS: drawScreenSettingsScreen(); break;
        case STATE_PLAYBACK_SETTINGS: drawPlaybackSettingsScreen(); break;
        case STATE_INFO: drawInfoScreen(); break;
        case STATE_RESET_CONFIRM: drawResetConfirmScreen(); break;
        case STATE_THEME_SETTINGS: drawThemeSettingsScreen(); break;
    }
}
