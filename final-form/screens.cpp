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
#include "bt_receiver.h"
#include "wifi_transfer.h"

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

// Smartwatch-style icon grid: 6 circular buttons (2x3), icon only, no labels
// and no "now playing" panel (track info lives in the Player screen instead).
void drawMainMenu() {
    tft.fillScreen(COLOR_BG);
    drawHeader("STREAM 32", false);
    for (int i = 0; i < 6; i++) {
        int cx = MENU_CELL_CX[i % 3];
        int cy = MENU_CELL_CY[i / 3];
        tft.fillCircle(cx, cy, MENU_CELL_R, COLOR_PANEL);
        tft.drawCircle(cx, cy, MENU_CELL_R, COLOR_PANEL_BORDER);
        switch (i) {
            case 0: drawPlayIcon(cx, cy, 18, COLOR_ACCENT); break;
            case 1: drawLibraryIcon(cx, cy, 14, COLOR_ACCENT); break;
            case 2: drawPlaylistIcon(cx, cy, 14, COLOR_ACCENT); break;
            case 3: drawHeartIcon(cx, cy, 15, COLOR_ACCENT); break;
            case 4: drawGearIcon(cx, cy, 14, COLOR_ACCENT); break;
            case 5: drawSwapIcon(cx, cy, 14, COLOR_ACCENT); break;
        }
        if (focusIndex == i) drawFocusRingCircle(cx, cy, MENU_CELL_R + 2);
    }
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SETTINGS");
    const char *labels[] = {"Bluetooth", "Screen", "Playback", "Theme", "Information", "Reset settings"};
    for (uint8_t i = 0; i < 6; i++) {
        int y = 34 + i * 30;
        tft.fillRoundRect(10, y, SCREEN_W - 20, 26, 8, COLOR_PANEL);
        tft.drawRoundRect(10, y, SCREEN_W - 20, 26, 8, COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, labels[i], 20, y + 7, fontToSize(2), GFX_TL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        gfxDrawString(tft, ">", SCREEN_W - 22, y + 9, fontToSize(2), GFX_MC);
        if (focusIndex == i) drawFocusRing(10, y, SCREEN_W - 20, 26);
    }
}

void drawPlaybackSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("PLAYBACK");
    tft.fillRoundRect(10, 40, SCREEN_W - 20, 34, 8, COLOR_PANEL);
    tft.drawRoundRect(10, 40, SCREEN_W - 20, 34, 8, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "Auto-play on connect", 20, 51, fontToSize(2), GFX_TL);
    tft.setTextColor(autoPlayEnabled ? COLOR_ACCENT : COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, autoPlayEnabled ? "ON" : "OFF", SCREEN_W - 35, 58, fontToSize(2), GFX_MC);
    if (focusIndex == 0) drawFocusRing(10, 40, SCREEN_W - 20, 34);
}

// Brightness row dropped: backlight is hard-wired to 3V3, no PWM GPIO free (pins.h).
// Timeout is now the only control, so the two-level row/edit focus the original
// plan sketched is unnecessary -- focusIndex walks the 4 chips directly.
void drawScreenSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SCREEN");
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "Screen timeout", 18, 40, fontToSize(2), GFX_TL);
    const char *timeoutLabel = screenTimeoutMinutes == 0 ? "OFF" : (screenTimeoutMinutes == 1 ? "1 min" : (screenTimeoutMinutes == 5 ? "5 min" : "10 min"));
    tft.setTextColor(COLOR_ACCENT);
    gfxDrawString(tft, timeoutLabel, SCREEN_W - 18, 40, fontToSize(2), GFX_TR);
    const char *options[] = {"1 min", "5 min", "10 min", "OFF"};
    const uint8_t values[] = {1, 5, 10, 0};
    for (uint8_t i = 0; i < 4; i++) {
        int x = 12 + i * 76;
        bool selected = screenTimeoutMinutes == values[i];
        tft.fillRoundRect(x, 78, 70, 32, 7, selected ? COLOR_ACCENT_DIM : COLOR_PANEL);
        tft.drawRoundRect(x, 78, 70, 32, 7, selected ? COLOR_ACCENT : COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, options[i], x + 35, 87, 1, GFX_MC);
        if (focusIndex == i) drawFocusRing(x, 78, 70, 32);
    }
}

void drawThemeSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("THEME");
    const char *names[] = {"Classic Green", "Blue", "Purple", "Amber", "Rose"};
    for (uint8_t i = 0; i < 5; i++) {
        int y = 36 + i * 34;
        bool selected = themeIndex == i;
        tft.fillRoundRect(18, y, SCREEN_W - 36, 30, 9, selected ? COLOR_ACCENT_DIM : COLOR_PANEL);
        tft.drawRoundRect(18, y, SCREEN_W - 36, 30, 9, selected ? COLOR_ACCENT : COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, names[i], SCREEN_W / 2, y + 7, fontToSize(2), GFX_MC);
        if (focusIndex == i) drawFocusRing(18, y, SCREEN_W - 36, 30);
    }
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

    tft.fillCircle(PL_PREV_CX, PL_BTN_CY, PL_BTN_R, COLOR_PANEL);
    tft.drawCircle(PL_PREV_CX, PL_BTN_CY, PL_BTN_R, COLOR_PANEL_BORDER);
    drawPrevIcon(PL_PREV_CX, PL_BTN_CY, 11, COLOR_TEXT_PRIMARY);

    tft.fillCircle(PL_NEXT_CX, PL_BTN_CY, PL_BTN_R, COLOR_PANEL);
    tft.drawCircle(PL_NEXT_CX, PL_BTN_CY, PL_BTN_R, COLOR_PANEL_BORDER);
    drawNextIcon(PL_NEXT_CX, PL_BTN_CY, 11, COLOR_TEXT_PRIMARY);

    updatePlayPauseButton();

    tft.fillRoundRect(PL_VOLDOWN_X, PL_VOL_Y, PL_VOLDOWN_W, PL_VOL_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_VOLDOWN_X, PL_VOL_Y, PL_VOLDOWN_W, PL_VOL_H, 10, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "-", PL_VOLDOWN_X + PL_VOLDOWN_W / 2, PL_VOL_Y + PL_VOL_H / 2 - 7, fontToSize(4), GFX_MC);

    tft.fillRoundRect(PL_VOLUP_X, PL_VOL_Y, PL_VOLUP_W, PL_VOL_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_VOLUP_X, PL_VOL_Y, PL_VOLUP_W, PL_VOL_H, 10, COLOR_PANEL_BORDER);
    gfxDrawString(tft, "+", PL_VOLUP_X + PL_VOLUP_W / 2, PL_VOL_Y + PL_VOL_H / 2 - 7, fontToSize(4), GFX_MC);

    updateVolumeUI();
}

void updateLikeButton() {
    if (currentState != STATE_PLAYER) return;
    tft.fillRect(PL_HEART_CX - PL_HEART_R - 2, PL_HEART_CY - PL_HEART_R - 2, (PL_HEART_R + 2) * 2, (PL_HEART_R + 2) * 2, COLOR_BG);
    bool liked = (total_tracks > 0) ? isLiked[current_track_index] : false;
    drawHeartIcon(PL_HEART_CX, PL_HEART_CY, PL_HEART_R, liked ? COLOR_HEART_ACTIVE : COLOR_HEART_INACTIVE);
}

void updatePlayPauseButton() {
    if (currentState != STATE_PLAYER) return;
    tft.fillCircle(PL_PLAY_CX, PL_BTN_CY, PL_BTN_R, is_playing ? COLOR_ACCENT_DIM : COLOR_ACCENT);
    tft.drawCircle(PL_PLAY_CX, PL_BTN_CY, PL_BTN_R, COLOR_PANEL_BORDER);
    if (is_playing) {
        drawPauseIcon(PL_PLAY_CX, PL_BTN_CY, 11, COLOR_BG);
    } else {
        drawPlayIcon(PL_PLAY_CX, PL_BTN_CY, 11, COLOR_BG);
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

    tft.fillRoundRect(20, 204, 50, 28, 6, COLOR_PANEL);
    tft.drawRoundRect(20, 204, 50, 28, 6, COLOR_PANEL_BORDER);
    drawPrevIcon(45, 218, 12, COLOR_TEXT_SECONDARY);

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, String(page + 1) + "/" + String(maxPages), SCREEN_W / 2, 218, fontToSize(2), GFX_MC);

    tft.fillRoundRect(SCREEN_W - 70, 204, 50, 28, 6, COLOR_PANEL);
    tft.drawRoundRect(SCREEN_W - 70, 204, 50, 28, 6, COLOR_PANEL_BORDER);
    drawNextIcon(SCREEN_W - 45, 218, 12, COLOR_TEXT_SECONDARY);
}

void drawBTManagerScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BLUETOOTH");
    tft.drawCircle(SCREEN_W / 2, 82, 38, COLOR_PANEL_BORDER);
    tft.drawCircle(SCREEN_W / 2, 82, 32, COLOR_ACCENT_DIM);
    drawBluetoothLogo(SCREEN_W / 2, 82, 37, bt_connected ? COLOR_BT_CONNECTED : COLOR_BT_DISCONNECTED);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, bt_connected ? "Connected" : (bt_connecting ? "Connecting..." : "Disconnected"), SCREEN_W / 2, 132, fontToSize(2), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, savedBTDevice, SCREEN_W / 2, 150, fontToSize(2), GFX_MC);

    tft.fillRoundRect(20, 166, SCREEN_W - 40, 30, 9, COLOR_PANEL);
    tft.drawRoundRect(20, 166, SCREEN_W - 40, 30, 9, COLOR_ACCENT_DIM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, bt_connected ? "DISCONNECT" : "CONNECT", SCREEN_W / 2, 174, fontToSize(2), GFX_MC);
    if (focusIndex == 0) drawFocusRing(20, 166, SCREEN_W - 40, 30);

    tft.fillRoundRect(20, 202, SCREEN_W - 40, 30, 9, COLOR_PANEL);
    tft.drawRoundRect(20, 202, SCREEN_W - 40, 30, 9, COLOR_PANEL_BORDER);
    gfxDrawString(tft, "SCAN DEVICES", SCREEN_W / 2, 210, fontToSize(2), GFX_MC);
    if (focusIndex == 1) drawFocusRing(20, 202, SCREEN_W - 40, 30);
}

void drawBluetoothScanScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BLUETOOTH");
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, btScanActive ? "Scanning..." : "Select device", SCREEN_W / 2, 36, fontToSize(2), GFX_MC);
    for (uint8_t i = 0; i < btScanCount; i++) {
        int y = 52 + i * 27;
        tft.fillRoundRect(10, y, SCREEN_W - 20, 24, 6, COLOR_PANEL);
        tft.drawRoundRect(10, y, SCREEN_W - 20, 24, 6, COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, btScanNames[i], 20, y + 6, fontToSize(2), GFX_TL);
        if (focusIndex == i) drawFocusRing(10, y, SCREEN_W - 20, 24);
    }
}

// Generic rounded-rect row list, shared by drawPlaylistsScreen/drawPlaylistDetailScreen
// (same visual pattern as drawTrackListScreen, minus paging -- lists are assumed
// to fit one screen per the redesign spec).
static void drawSimpleRowList(const String &name, int row) {
    int listY = LIST_ITEM_Y_START + row * (LIST_ITEM_H + LIST_ITEM_GAP);
    tft.fillRoundRect(10, listY, SCREEN_W - 20, LIST_ITEM_H, 8, COLOR_PANEL);
    tft.drawRoundRect(10, listY, SCREEN_W - 20, LIST_ITEM_H, 8, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, truncateToWidth(tft, name, SCREEN_W - 40, 2), 20, listY + 10, fontToSize(2), GFX_TL);
    if (focusIndex == row) drawFocusRing(10, listY, SCREEN_W - 20, LIST_ITEM_H);
}

void drawPlaylistsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("PLAYLISTS");
    for (int i = 0; i < totalPlaylists; i++) drawSimpleRowList(playlistNames[i], i);
    if (totalPlaylists == 0) {
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        gfxDrawString(tft, "No playlists", SCREEN_W / 2, 140, fontToSize(2), GFX_MC);
    }
}

void drawPlaylistDetailScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("PLAYLIST");
    int count = playlistTrackCount(playlist_selectedIndex);
    for (int i = 0; i < count; i++) drawSimpleRowList(playlistTrackDisplayName(playlist_selectedIndex, i), i);
    if (count == 0) {
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        gfxDrawString(tft, "No tracks", SCREEN_W / 2, 140, fontToSize(2), GFX_MC);
    }
}

void drawModeSelectScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("MODE");
    const char *labels[] = {"Local Player", "BT Receiver", "BT Transmitter", "Transfer File", "Kembali"};
    for (uint8_t i = 0; i < 5; i++) {
        int y = MODE_LIST_Y_START + i * (MODE_LIST_ITEM_H + MODE_LIST_GAP);
        tft.fillRoundRect(10, y, SCREEN_W - 20, MODE_LIST_ITEM_H, 8, COLOR_PANEL);
        tft.drawRoundRect(10, y, SCREEN_W - 20, MODE_LIST_ITEM_H, 8, COLOR_PANEL_BORDER);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, labels[i], 20, y + MODE_LIST_ITEM_H / 2 - 7, fontToSize(2), GFX_TL);
        if (i < 4 && (OperatingMode)i == operatingMode) {
            int chipW = 50, chipH = 18;
            int chipX = SCREEN_W - 20 - chipW - 6, chipY = y + (MODE_LIST_ITEM_H - chipH) / 2;
            tft.fillRoundRect(chipX, chipY, chipW, chipH, 6, COLOR_CHIP_BG);
            tft.setTextColor(COLOR_CHIP_TEXT);
            gfxDrawString(tft, "AKTIF", chipX + chipW / 2, chipY + chipH / 2 - 6, 1, GFX_MC);
        }
        if (focusIndex == i) drawFocusRing(10, y, SCREEN_W - 20, MODE_LIST_ITEM_H);
    }
}

// Full-screen transition message, mirroring drawScreenLocked()'s simple
// centered-text style. No header: the device restarts ~600ms after this draws
// (see nav.cpp's selectOperatingMode), so there's nothing to navigate back to.
void drawModeSwitchingScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "Beralih mode...", SCREEN_W / 2, SCREEN_H / 2 - 10, fontToSize(4), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, "Restarting...", SCREEN_W / 2, SCREEN_H / 2 + 20, fontToSize(2), GFX_MC);
}

// "Home" screen for MODE_BT_RECEIVER (no STATE_MENU grid in this mode, so the
// mode-switch hint doubles as the only way back to Settings/mode-select).
void drawBTReceiverScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BT RECEIVER");
    int cx = SCREEN_W / 2, cy = 78;
    uint16_t btColor = btrx_connected ? COLOR_BT_CONNECTED : COLOR_BT_DISCONNECTED;
    tft.drawCircle(cx, cy, 38, COLOR_PANEL_BORDER);
    tft.drawCircle(cx, cy, 32, COLOR_ACCENT_DIM);
    drawBluetoothLogo(cx, cy, 24, btColor);

    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, btrx_connected ? "Connected" : "Waiting for phone...", cx, 130, fontToSize(2), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, btrx_deviceName, cx, 148, fontToSize(2), GFX_MC);
    tft.setTextColor(btrx_playing ? COLOR_SUCCESS : COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, btrx_playing ? "Playing" : "Idle", cx, 168, fontToSize(2), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, String("Volume ") + String((bt_volume * 100) / 127) + "%", cx, 188, fontToSize(2), GFX_MC);

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, "Rotate encoder: volume", cx, 206, 1, GFX_MC);
    gfxDrawString(tft, "Hold MENU: switch mode", cx, 220, 1, GFX_MC);
}

// "Home" screen for MODE_TRANSFER_FILE (same no-grid situation as BT Receiver).
void drawTransferFileScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSFER FILE");

    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, String("WiFi AP: ") + wifiTransfer_ssid, SCREEN_W / 2, 42, fontToSize(2), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, wifiTransfer_ip, SCREEN_W / 2, 62, fontToSize(2), GFX_MC);

    int barX = 30, barY = 92, barW = SCREEN_W - 60, barH = 14;
    tft.fillRoundRect(barX, barY, barW, barH, barH / 2, COLOR_TRACK_BG);
    int pct = wifiTransfer_lastUploadPct;
    if (pct >= 0) {
        int fillW = (barW * constrain(pct, 0, 100)) / 100;
        if (fillW > 0) tft.fillRoundRect(barX, barY, fillW, barH, barH / 2, COLOR_ACCENT);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        gfxDrawString(tft, String(pct) + "%", SCREEN_W / 2, barY + barH + 8, fontToSize(2), GFX_MC);
    } else {
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        gfxDrawString(tft, "Waiting for upload...", SCREEN_W / 2, barY + barH + 8, fontToSize(2), GFX_MC);
    }

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, String(wifiTransfer_filesReceived) + " files received", SCREEN_W / 2, 154, fontToSize(2), GFX_MC);

    gfxDrawString(tft, "Hold MENU: switch mode", SCREEN_W / 2, SCREEN_H - 16, 1, GFX_MC);
}

// Floating auto-hiding volume pill, painted OVER whatever screen is current --
// does not fillScreen/drawHeader, only nav.cpp's checkVolumeOverlayTimeout()
// triggers the full redraw that erases it.
void drawVolumeOverlay() {
    tft.fillRoundRect(VOL_OVERLAY_X, VOL_OVERLAY_Y, VOL_OVERLAY_W, VOL_OVERLAY_H, 10, COLOR_PANEL);
    tft.drawRoundRect(VOL_OVERLAY_X, VOL_OVERLAY_Y, VOL_OVERLAY_W, VOL_OVERLAY_H, 10, COLOR_PANEL_BORDER);

    bool muted = (bt_volume == 0);
    int iconCx = VOL_OVERLAY_X + 18, iconCy = VOL_OVERLAY_Y + VOL_OVERLAY_H / 2;
    drawSpeakerIcon(iconCx, iconCy, 9, muted ? COLOR_WARNING : COLOR_TEXT_PRIMARY, muted);

    int barX = VOL_OVERLAY_X + 36, barY = iconCy - 3, barW = 110, barH = 6;
    tft.fillRoundRect(barX, barY, barW, barH, barH / 2, COLOR_TRACK_BG);
    int fillW = (barW * bt_volume) / 127;
    if (fillW > 0) tft.fillRoundRect(barX, barY, fillW, barH, barH / 2, muted ? COLOR_WARNING : COLOR_ACCENT);

    int volPercent = (bt_volume * 100) / 127;
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, String(volPercent) + "%", VOL_OVERLAY_X + VOL_OVERLAY_W - 10, VOL_OVERLAY_Y + 12, fontToSize(2), GFX_TR);
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
        case STATE_PLAYLISTS: drawPlaylistsScreen(); break;
        case STATE_PLAYLIST_DETAIL: drawPlaylistDetailScreen(); break;
        case STATE_MODE_SELECT: drawModeSelectScreen(); break;
        case STATE_MODE_SWITCHING: drawModeSwitchingScreen(); break;
        case STATE_BT_RECEIVER: drawBTReceiverScreen(); break;
        case STATE_TRANSFER_FILE: drawTransferFileScreen(); break;
    }
}
