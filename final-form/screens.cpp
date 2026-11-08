// Ported from old/Stream32-ESP-Player.ino lines 721-1134 (drawXScreen functions +
// drawCoverArt + the Player-screen partial-redraw helpers). TFT_eSPI text-datum
// calls become gfxDrawString/truncateToWidth; everything else (fillRoundRect,
// drawRoundRect, fillCircle, drawLine...) is unchanged Adafruit_GFX API.
//
// Added vs. the original: drawFocusRing() calls marking nav.h's `focusIndex` on
// every cursor-navigable screen, since encoder+button input has no finger to tap.
//
// Visual-fidelity pass vs. dap-ui-design/source/*.dc.html (portrait, now
// deleted): every drawXScreen calls drawHeader(title, icon) + drawFooter(hint);
// list-style screens draw plain rows with a COLOR_CHIP_BG pill only on the
// focused/current row instead of a bordered card on every row; the volume
// overlay moved to the top and gained a slider thumb.
//
// Landscape re-match pass vs. dap-ui-landscape/source/L_*.dc.html (the
// correct, purpose-built 320x240 reference -- the portrait mockup above was
// the wrong one): geometry/colors/row-styles re-matched screen-by-screen to
// this mockup's literal pixels; drawPlayerScreen's (Local Player/BT
// Transmitter) mode-identity panel now shows real per-track album art
// (drawCoverArt) instead of a decorative icon graphic; STATE_MODE_SELECT
// became a 2-column grid (drawModeSelectScreen); Transfer File became a
// two-column card layout. See that pass's handback report for exact
// deviations (missing data fields, off-limits files, etc).
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
// Not in dap-ui-design's screen list -- left as the existing grid, just wired
// into the new header/footer.
void drawMainMenu() {
    tft.fillScreen(COLOR_BG);
    drawHeader("STREAM 32");
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
    drawFooter("Klik: pilih - MENU: menu");
}

// Settings list: plain rows, COLOR_CHIP_BG pill only on the focused row
// (dap-ui-design pattern) instead of a bordered card on every row. Row
// geometry (34 + i*30, h26) is unchanged -- nav.cpp's touch handler hardcodes
// those same numbers.
void drawSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SETTINGS");
    const char *labels[] = {"Bluetooth", "Screen", "Playback", "Theme", "Information", "Reset settings"};
    for (uint8_t i = 0; i < 6; i++) {
        int y = 34 + i * 30;
        bool focused = (focusIndex == i);
        uint16_t bg = COLOR_BG;
        if (focused) {
            tft.fillRoundRect(10, y, SCREEN_W - 20, 26, 12, COLOR_CHIP_BG);
            bg = COLOR_CHIP_BG;
        }
        tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
        gfxDrawString(tft, labels[i], 20, y + 7, fontToSize(2), GFX_TL);
        tft.setTextColor(COLOR_TEXT_SECONDARY, bg);
        gfxDrawString(tft, ">", SCREEN_W - 22, y + 9, fontToSize(2), GFX_MC);
    }
    drawFooter("Klik: pilih - MENU: kembali");
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
    drawFooter("Klik: ubah - MENU: kembali");
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
    drawFooter("Klik: pilih - MENU: kembali");
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
    drawFooter("Klik: pilih - MENU: kembali");
}

// Original said "TAP / TO WAKE" (touch-only hardware). Touch is optional here, so
// the primary wake path is any button/encoder press; text says so but a tap still
// works too when ENABLE_TOUCH is on. No header/footer: full-bleed lock screen,
// same as before.
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
    drawFooter("MENU: kembali");
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
    // No drawFooter(): the CANCEL/RESET row (y178-212) already abuts the
    // footer band exactly, and nav.cpp hardcodes that y=178 touch rect.
}

// Connection-status badge: small filled circle, checkmark when connected
// (L_Receiver.dc.html / L_Transmitter.dc.html's literal top-right-of-panel
// badge), COLOR_WARNING dot with no glyph otherwise -- the mockup only ever
// demos the connected state, so the disconnected/connecting look is this
// pass's own reasonable fallback, not a literal mockup pixel-match.
static void drawConnectionBadge(int cx, int cy, bool connected) {
    int r = 11;
    tft.fillCircle(cx, cy, r, connected ? COLOR_SUCCESS : COLOR_WARNING);
    if (connected) {
        tft.drawLine(cx - 4, cy, cx - 1, cy + 3, COLOR_BG);
        tft.drawLine(cx - 1, cy + 3, cx + 5, cy - 4, COLOR_BG);
    }
}

// Local Player + BT Transmitter: both play from the local SD library, so
// both get real per-track album art (task's main ask) instead of the old
// decorative "cassette reel"/triple-icon panel graphic. Panel frame + mode
// color stay as the backdrop (L_Main.dc.html / L_Transmitter.dc.html's
// literal box); art is a square slice sized to the panel's height,
// left-anchored, with title/subtitle + (shuffle/repeat icons | connection
// badge) filling the remaining width -- see layout.h's PL_PANEL_INFO_* note
// on why this interior arrangement isn't mockup-literal (the mockup's own
// panel interior there is a decorative graphic sized for a wide panel, not
// an album-art thumbnail).
void drawPlayerScreen() {
    tft.fillScreen(COLOR_BG);
    bool isTransmitter = (operatingMode == MODE_BT_TRANSMITTER);
    drawHeader(isTransmitter ? "BT TRANSMITTER" : "LOCAL PLAYER", isTransmitter ? HICON_BT : HICON_NONE);

    uint16_t panelBg = isTransmitter ? COLOR_BT_PANEL : COLOR_ACCENT_DIM;
    tft.fillRoundRect(PL_PANEL_X, PL_PANEL_Y, PL_PANEL_W, PL_PANEL_H, 14, panelBg);

    drawCoverArt(PL_ART_X, PL_ART_Y, PL_ART_SIZE, PL_ART_SIZE);

    if (isTransmitter) {
        drawConnectionBadge(PL_PANEL_INFO_RIGHT - 11, PL_PANEL_BADGE_CY, bt_connected);
    } else {
        // Shuffle/repeat: decorative only, no such state exists in this codebase.
        drawRepeatIcon(PL_PANEL_INFO_RIGHT - 8, PL_PANEL_BADGE_CY, 8, COLOR_ON_ACCENT_DIM);
        drawShuffleIcon(PL_PANEL_INFO_RIGHT - 30, PL_PANEL_BADGE_CY, 8, COLOR_ON_ACCENT_DIM);
    }

    String rawName = (total_tracks > 0) ? trackDisplayName(playlist[current_track_index]) : "No tracks!";
    String artist = "Unknown Artist";
    String title = rawName;
    int hyphenIdx = rawName.indexOf(" - ");
    if (hyphenIdx != -1) {
        artist = rawName.substring(0, hyphenIdx);
        title = rawName.substring(hyphenIdx + 3);
    }
    int infoW = PL_PANEL_INFO_RIGHT - PL_PANEL_INFO_X;
    title = truncateToWidth(tft, title, infoW, 2);

    uint16_t onPanelPrimary = isTransmitter ? COLOR_BT_PANEL_TEXT : COLOR_ON_ACCENT_DIM;
    tft.setTextColor(onPanelPrimary, panelBg);
    gfxDrawString(tft, title, PL_PANEL_INFO_X, PL_PANEL_TITLE_Y, fontToSize(2), GFX_TL);

    // Subtitle: "favorit-state - position/total" for Local Player,
    // "position/total - format" for BT Transmitter. No duration/elapsed-time
    // data exists in this codebase (audio_decoder.h only tracks byte progress,
    // not mm:ss) -- deviation from the mockup's "3:42"-style text, reported.
    String subtitle;
    int totalForDisplay = total_tracks > 0 ? total_tracks : 1;
    if (isTransmitter) {
        subtitle = String(current_track_index + 1) + "/" + String(totalForDisplay) + " - MP3";
    } else {
        bool liked = (total_tracks > 0) ? isLiked[current_track_index] : false;
        subtitle = String(liked ? "Favorit" : artist) + " - " + String(current_track_index + 1) + "/" + String(totalForDisplay);
    }
    subtitle = truncateToWidth(tft, subtitle, infoW, 1);
    tft.setTextColor(onPanelPrimary, panelBg);
    gfxDrawString(tft, subtitle, PL_PANEL_INFO_X, PL_PANEL_SUBTITLE_Y, 1, GFX_TL);

    updateLikeButton();

    tft.fillRoundRect(PL_PROGRESS_X, PL_PROGRESS_Y, PL_PROGRESS_W, PL_PROGRESS_H, PL_PROGRESS_H / 2, COLOR_TRACK_BG);
    updateProgressBar(true);

    tft.fillCircle(PL_PREV_CX, PL_BTN_CY, PL_BTN_R, COLOR_BUTTON_BG);
    drawPrevIcon(PL_PREV_CX, PL_BTN_CY, 11, COLOR_TEXT_PRIMARY);

    tft.fillCircle(PL_NEXT_CX, PL_BTN_CY, PL_BTN_R, COLOR_BUTTON_BG);
    drawNextIcon(PL_NEXT_CX, PL_BTN_CY, 11, COLOR_TEXT_PRIMARY);

    updatePlayPauseButton();

    drawFooter("Klik: play/pause - MENU: menu");
}

void updateLikeButton() {
    if (currentState != STATE_PLAYER) return;
    tft.fillRect(PL_HEART_CX - PL_HEART_R - 2, PL_HEART_CY - PL_HEART_R - 2, (PL_HEART_R + 2) * 2, (PL_HEART_R + 2) * 2, COLOR_BG);
    bool liked = (total_tracks > 0) ? isLiked[current_track_index] : false;
    drawHeartIcon(PL_HEART_CX, PL_HEART_CY, PL_HEART_R, liked ? COLOR_HEART_ACTIVE : COLOR_HEART_INACTIVE);
}

// Play/Pause always fills COLOR_ACCENT with a COLOR_ACCENT_TEXT icon (the
// play/pause glyph swap alone is the "is playing" indicator). Shape is a
// rounded SQUARE (L_Main.dc.html: 56x56, border-radius 18 -- not 28, so not
// a circle), unlike the true-circle prev/next buttons either side of it.
// nav.cpp's touch hit-test still treats this as a PL_BTN_R circle (its
// optional touch path is off-limits to edit) -- a smaller touch target than
// the new visual square, cosmetic-only mismatch.
void updatePlayPauseButton() {
    if (currentState != STATE_PLAYER) return;
    int half = PL_PLAY_SIZE / 2;
    tft.fillRoundRect(PL_PLAY_CX - half, PL_BTN_CY - half, PL_PLAY_SIZE, PL_PLAY_SIZE, PL_PLAY_RADIUS, COLOR_ACCENT);
    if (is_playing) {
        drawPauseIcon(PL_PLAY_CX, PL_BTN_CY, 13, COLOR_ACCENT_TEXT);
    } else {
        drawPlayIcon(PL_PLAY_CX, PL_BTN_CY, 13, COLOR_ACCENT_TEXT);
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

// The inline +/- volume row is gone (dap-ui-design shows no persistent volume
// control on the Player screen -- only the floating drawVolumeOverlay() while
// the encoder turns). Kept as a no-op, not removed: nav.cpp calls
// updateVolumeUI() + drawVolumeOverlay() together on every encoder rotate, and
// nav.cpp is off-limits to edit. PL_VOLDOWN_X/PL_VOLUP_X/etc. (layout.h) stay
// defined for the same reason -- nav.cpp's touch handler still hit-tests them.
void updateVolumeUI() {}

// Shared no-card row for Track List/Likes List: plain row, COLOR_CHIP_BG pill
// only when focused or currently-playing (dap-ui-design). The currently-playing
// row gets a 3-bar equalizer glyph instead of its index number.
static void drawTrackRow(int row, int listY, int originalTrackIdx) {
    bool isCurrent = (originalTrackIdx == current_track_index);
    bool focused = (focusIndex == row);
    uint16_t bg = COLOR_BG;
    if (isCurrent || focused) {
        tft.fillRoundRect(6, listY, SCREEN_W - 12, LIST_ITEM_H, 12, COLOR_CHIP_BG);
        bg = COLOR_CHIP_BG;
    }

    int iconCx = 29, iconCy = listY + LIST_ITEM_H / 2;
    if (isCurrent) {
        drawEqualizerBars(iconCx, iconCy, 12, 14, COLOR_ACCENT);
    } else {
        String num = (originalTrackIdx + 1) < 10 ? "0" + String(originalTrackIdx + 1) : String(originalTrackIdx + 1);
        tft.setTextColor(COLOR_TEXT_SECONDARY, bg);
        gfxDrawString(tft, num, iconCx, iconCy - 4, 1, GFX_MC);
    }

    String name = trackDisplayName(playlist[originalTrackIdx]);
    name = truncateToWidth(tft, name, SCREEN_W - 66, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
    gfxDrawString(tft, name, 50, listY + (LIST_ITEM_H - 14) / 2, fontToSize(2), GFX_TL);
}

// Scroll-position thumb, right edge (L_Library.dc.html: right:2 top:34
// width:3 height:36 -- a fixed-looking thumb since the mockup only shows one
// state; here it's scaled/positioned by actual page count so it still means
// something when there's more than one page).
static void drawListScrollThumb(int page, int maxPages) {
    if (maxPages <= 1) return;
    int trackTop = LIST_ITEM_Y_START, trackBottom = SCREEN_H - FOOTER_H;
    int trackH = trackBottom - trackTop;
    int thumbH = trackH / maxPages;
    if (thumbH < 16) thumbH = 16;
    int thumbY = (maxPages > 1) ? trackTop + (trackH - thumbH) * page / (maxPages - 1) : trackTop;
    tft.fillRoundRect(SCREEN_W - 4, thumbY, 3, thumbH, 2, COLOR_TEXT_SECONDARY);
}

// No duration metadata exists in playlist.h (just file paths), so rows stay
// single-line (title only) -- the mockup's "3:42"-style secondary line is
// skipped for the same reason as the Player screen's subtitle above.
// The old bordered PREV/NEXT page-chip buttons are gone too: page position
// now lives in the footer hint instead (dap-ui-design drops the heavy
// bordered look here in favor of a quiet indicator).
void drawTrackListScreen(bool onlyLikes) {
    tft.fillScreen(COLOR_BG);
    drawHeader(onlyLikes ? "FAVORITES" : "ALL TRACKS", HICON_LIST);

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
        if (itemIdx >= endIdx) continue;
        drawTrackRow(i, listY, matchingTracks[itemIdx]);
    }

    if (matchCount == 0) {
        tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
        gfxDrawString(tft, onlyLikes ? "No liked tracks" : "No MP3 files", SCREEN_W / 2, 140, fontToSize(2), GFX_MC);
    }

    drawListScrollThumb(page, maxPages);
    drawFooter(String(page + 1) + "/" + String(maxPages) + " - PREV/NEXT pilih - Klik putar");
}

// drawBTManagerScreen deliberately has NO drawFooter() call: its CONNECT/SCAN
// buttons sit at y=166/202 h=30 (bottom 232), and nav.cpp's touch handler
// hardcodes those exact pixel rects. The new 28px footer band (y212-240)
// would visually clip the SCAN button, and nav.cpp is off-limits to move it.
// This screen isn't in dap-ui-design's explicit screen list either (BT
// Transmitter mode now lives in drawPlayerScreen/drawMainMenu) -- header
// wiring fixed, footer skipped, reported as a deviation.
void drawBTManagerScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BLUETOOTH", HICON_BT);
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

// Plain rows, pill only on focus (dap-ui-design) -- geometry (y=52+i*27, h24)
// unchanged, nav.cpp's touch handler hardcodes those same numbers.
void drawBluetoothScanScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BLUETOOTH", HICON_BT);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    gfxDrawString(tft, btScanActive ? "Scanning..." : "Select device", SCREEN_W / 2, 36, fontToSize(2), GFX_MC);
    for (uint8_t i = 0; i < btScanCount; i++) {
        int y = 52 + i * 27;
        bool focused = (focusIndex == i);
        uint16_t bg = COLOR_BG;
        if (focused) {
            tft.fillRoundRect(10, y, SCREEN_W - 20, 24, 12, COLOR_CHIP_BG);
            bg = COLOR_CHIP_BG;
        }
        tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
        gfxDrawString(tft, btScanNames[i], 20, y + 6, fontToSize(2), GFX_TL);
    }
    drawFooter("Klik: pilih - MENU: kembali");
}

// Playlists row (L_Playlists.dc.html): 28x28 rounded-rect color avatar with
// a small playlist glyph, NOT a plain index number -- one row per playlist,
// avatar color cycling through a small palette for visual variety. Colors
// reuse existing theme tokens only (COLOR_CHIP_BG/COLOR_ACCENT_DIM/
// COLOR_BT_PANEL match 3 of the mockup's 5 literal swatches exactly); the
// other 2 (an amber-brown and a dark green) have no existing token and
// aren't worth a new one for mere decoration, so the cycle is 3 long, not 5
// -- deliberate simplification, not an oversight.
static void drawPlaylistRow(int row, const String &name, const String &secondary) {
    int listY = LIST_ITEM_Y_START + row * LIST_ITEM_H;
    bool focused = (focusIndex == row);
    uint16_t bg = COLOR_BG;
    if (focused) {
        tft.fillRoundRect(6, listY, SCREEN_W - 12, LIST_ITEM_H, 12, COLOR_CHIP_BG);
        bg = COLOR_CHIP_BG;
    }

    uint16_t avatarColors[3] = {COLOR_CHIP_BG, COLOR_ACCENT_DIM, COLOR_BT_PANEL};
    int avatarCx = 18 + 14, avatarCy = listY + LIST_ITEM_H / 2;
    tft.fillRoundRect(18, avatarCy - 14, 28, 28, 8, avatarColors[row % 3]);
    drawPlaylistIcon(avatarCx, avatarCy, 7, COLOR_TEXT_PRIMARY);

    String title = truncateToWidth(tft, name, SCREEN_W - 66, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
    int titleY = secondary.length() ? listY + 3 : listY + (LIST_ITEM_H - 14) / 2;
    gfxDrawString(tft, title, 56, titleY, fontToSize(2), GFX_TL);
    if (secondary.length()) {
        tft.setTextColor(COLOR_TEXT_SECONDARY, bg);
        gfxDrawString(tft, secondary, 56, listY + 18, 1, GFX_TL);
    }
}

void drawPlaylistsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("PLAYLISTS", HICON_PLAYLIST);
    for (int i = 0; i < totalPlaylists; i++) {
        drawPlaylistRow(i, playlistNames[i], String(playlistTrackCount(i)) + " lagu");
    }
    if (totalPlaylists == 0) {
        tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
        gfxDrawString(tft, "No playlists", SCREEN_W / 2, 140, fontToSize(2), GFX_MC);
    }
    drawFooter("Playlist dari /Playlists - Klik buka");
}

// PlaylistDetail track row: plain number (no eq-bars -- playlist.h has no
// API to map a (playlist index, row) pair back to current_track_index
// without adding new playlist.cpp code, out of this pass's file scope),
// offset to the right column (PD_LIST_X) alongside the Putar/Acak/Ulang
// button column.
static void drawDetailTrackRow(int row, const String &name) {
    int listY = LIST_ITEM_Y_START + row * LIST_ITEM_H;
    int rowX = PD_LIST_X + 6, rowW = SCREEN_W - rowX - 6;
    bool focused = (focusIndex == row);
    uint16_t bg = COLOR_BG;
    if (focused) {
        tft.fillRoundRect(rowX, listY, rowW, LIST_ITEM_H, 12, COLOR_CHIP_BG);
        bg = COLOR_CHIP_BG;
    }

    int numCx = rowX + 12 + 10, numCy = listY + LIST_ITEM_H / 2;
    String num = (row + 1) < 10 ? "0" + String(row + 1) : String(row + 1);
    tft.setTextColor(COLOR_TEXT_SECONDARY, bg);
    gfxDrawString(tft, num, numCx, numCy - 4, 1, GFX_MC);

    String title = truncateToWidth(tft, name, rowW - 44, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
    gfxDrawString(tft, title, rowX + 12 + 22 + 10, listY + (LIST_ITEM_H - 14) / 2, fontToSize(2), GFX_TL);
}

// Drill-down screen (opened from Playlists): back-chevron header icon +
// real playlist name as the title. Per L_PlaylistDetail.dc.html: left
// column of Putar/Acak/Ulang buttons (Putar actually plays the first track;
// Acak/Ulang are visual-only decoration -- there's no shuffle/repeat state
// in this codebase, and wiring them as separately-focusable buttons would
// need nav.cpp changes outside this pass's allowed scope for this screen)
// plus a track-count stat, with the track list itself moved into a
// narrower right column (PD_LIST_X) alongside it.
void drawPlaylistDetailScreen() {
    tft.fillScreen(COLOR_BG);
    int count = playlistTrackCount(playlist_selectedIndex);
    String title = (playlist_selectedIndex >= 0 && playlist_selectedIndex < totalPlaylists)
                        ? playlistNames[playlist_selectedIndex]
                        : "PLAYLIST";
    drawHeader(title.c_str(), HICON_BACK);

    int by = PD_TOP_Y;
    tft.fillRoundRect(PD_LEFT_X, by, PD_LEFT_W, PD_BTN_H, PD_BTN_RADIUS, COLOR_ACCENT);
    drawPlayIcon(PD_LEFT_X + 16, by + PD_BTN_H / 2, 7, COLOR_ACCENT_TEXT);
    tft.setTextColor(COLOR_ACCENT_TEXT, COLOR_ACCENT);
    gfxDrawString(tft, "Putar", PD_LEFT_X + 30, by + PD_BTN_H / 2 - 4, 1, GFX_TL);

    by += PD_BTN_H + PD_BTN_GAP;
    tft.drawRoundRect(PD_LEFT_X, by, PD_LEFT_W, PD_BTN_H, PD_BTN_RADIUS, COLOR_PANEL_BORDER);
    drawShuffleIcon(PD_LEFT_X + 16, by + PD_BTN_H / 2, 7, COLOR_TEXT_PRIMARY);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    gfxDrawString(tft, "Acak", PD_LEFT_X + 30, by + PD_BTN_H / 2 - 4, 1, GFX_TL);

    by += PD_BTN_H + PD_BTN_GAP;
    tft.drawRoundRect(PD_LEFT_X, by, PD_LEFT_W, PD_BTN_H, PD_BTN_RADIUS, COLOR_PANEL_BORDER);
    drawRepeatIcon(PD_LEFT_X + 16, by + PD_BTN_H / 2, 7, COLOR_TEXT_PRIMARY);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    gfxDrawString(tft, "Ulang", PD_LEFT_X + 30, by + PD_BTN_H / 2 - 4, 1, GFX_TL);

    by += PD_BTN_H + PD_BTN_GAP + 2;
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    gfxDrawString(tft, String(count) + " lagu", PD_LEFT_X, by, 1, GFX_TL);

    for (int i = 0; i < count; i++) {
        drawDetailTrackRow(i, playlistTrackDisplayName(playlist_selectedIndex, i));
    }
    if (count == 0) {
        tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
        gfxDrawString(tft, "No tracks", PD_LIST_X + (SCREEN_W - PD_LIST_X) / 2, 140, fontToSize(2), GFX_MC);
    }
    drawFooter("Klik putar - MENU kembali");
}

// Mode Select / "Menu" (L_Menu.dc.html): a 2-column x 4-row grid of 7 cells
// -- Daftar Lagu, Playlist, Local Player, BT Receiver, BT Transmitter,
// Transfer File, Kembali -- NOT the old vertical list. No header MENU chip
// here (you're already in the menu). Icon circle shrunk to 28x28 (was 32),
// no subtitles (the mockup's compact 40px-tall cells have no room for one),
// "AKTIF" trailing label on whichever mode cell matches operatingMode.
// Cells 0/1 (Daftar Lagu/Playlist) are plain navigation shortcuts; cells 2-5
// are the 4 operating modes; cell 6 (Kembali) goes home -- see nav.cpp's
// handleSelect() STATE_MODE_SELECT case for the matching focusIndex mapping.
void drawModeSelectScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("MENU", HICON_NONE, false);
    const char *labels[] = {"Daftar Lagu", "Playlist", "Local Player", "BT Receiver", "BT Transmitter", "Transfer File", "Kembali"};

    for (uint8_t i = 0; i < MODE_GRID_COUNT; i++) {
        int col = i % 2, row = i / 2;
        int x = (col == 0) ? MODE_GRID_X0 : MODE_GRID_X1;
        int y = MODE_GRID_Y_START + row * (MODE_GRID_ROW_H + MODE_GRID_ROW_GAP);
        bool focused = (focusIndex == i);
        uint16_t bg = focused ? COLOR_CHIP_BG : COLOR_PANEL;
        tft.fillRoundRect(x, y, MODE_GRID_COL_W, MODE_GRID_ROW_H, 12, bg);

        int iconCx = x + 10 + 14, iconCy = y + MODE_GRID_ROW_H / 2;
        tft.fillCircle(iconCx, iconCy, 14, COLOR_BUTTON_BG);
        switch (i) {
            case 0: drawListIcon(iconCx, iconCy, 8, COLOR_ACCENT); break;
            case 1: drawPlaylistIcon(iconCx, iconCy, 8, COLOR_ACCENT); break;
            case 2: drawMusicNoteGlyph(iconCx, iconCy, 8, COLOR_ACCENT); break;
            case 3: drawBluetoothZigzag(iconCx, iconCy, 8, COLOR_ACCENT); break;
            case 4: drawBluetoothZigzag(iconCx, iconCy, 8, COLOR_ACCENT); break;
            case 5: drawUploadIcon(iconCx, iconCy, 8, COLOR_ACCENT); break;
            case 6: drawBackIcon(iconCx, iconCy, 7, COLOR_ACCENT); break;
        }

        bool isActiveMode = (i >= 2 && i <= 5) && ((OperatingMode)(i - 2) == operatingMode);
        int textX = x + 10 + 28 + 8;
        int textRight = x + MODE_GRID_COL_W - 8;
        if (isActiveMode) {
            tft.setTextColor(COLOR_SUCCESS, bg);
            int aktifW = gfxTextWidth(tft, "AKTIF", 1);
            gfxDrawString(tft, "AKTIF", textRight - aktifW, y + MODE_GRID_ROW_H / 2 - 3, 1, GFX_TL);
            textRight -= aktifW + 6;
        }
        tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
        String label = truncateToWidth(tft, labels[i], textRight - textX, 2);
        gfxDrawString(tft, label, textX, y + MODE_GRID_ROW_H / 2 - 4, fontToSize(2), GFX_TL);
    }
    drawFooter("Klik: OK - MENU: kembali");
}

// Full-screen transition (L_Switch.dc.html): icon box + "Beralih ke" label +
// target name + progress bar + description, centered. No header/footer: the
// device restarts ~600ms after this draws (see nav.cpp's
// selectOperatingMode()), so there's nothing to navigate back to.
// Deviation: the mockup's icon box is colored/iconed for the SPECIFIC
// destination mode and names it ("BT Receiver"), but selectOperatingMode()
// (nav.cpp, off-limits to edit outside the STATE_MODE_SELECT grid-nav change)
// never stores which mode is being switched TO anywhere screens.cpp can read
// -- it writes straight to Preferences and only takes effect after
// ESP.restart(). So this draws a generic swap icon/neutral panel and a
// generic "Beralih mode..." label instead of the real target; the progress
// fill is a fixed decorative value (there's no real per-tick progress to
// track during a single blocking delay(600)).
void drawModeSwitchingScreen() {
    tft.fillScreen(COLOR_BG);
    int boxSize = 84, boxX = 40, boxY = SCREEN_H / 2 - boxSize / 2;
    tft.fillRoundRect(boxX, boxY, boxSize, boxSize, 28, COLOR_PANEL);
    drawSwapIcon(boxX + boxSize / 2, boxY + boxSize / 2, 18, COLOR_ACCENT);

    int textX = boxX + boxSize + 20;
    int textW = SCREEN_W - 28 - textX;
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    gfxDrawString(tft, "Beralih mode", textX, SCREEN_H / 2 - 32, 1, GFX_TL);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    gfxDrawString(tft, "Mohon tunggu...", textX, SCREEN_H / 2 - 14, fontToSize(2), GFX_TL);

    int barY = SCREEN_H / 2 + 14, barW = min(150, textW), barH = 4;
    tft.fillRoundRect(textX, barY, barW, barH, barH / 2, COLOR_TRACK_BG);
    tft.fillRoundRect(textX, barY, (barW * 35) / 100, barH, barH / 2, COLOR_ACCENT);

    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    String line1 = truncateToWidth(tft, "ESP32 restart, tunggu sekitar 3 detik.", textW, 1);
    String line2 = truncateToWidth(tft, "Musik dan koneksi berhenti.", textW, 1);
    gfxDrawString(tft, line1, textX, barY + 16, 1, GFX_TL);
    gfxDrawString(tft, line2, textX, barY + 28, 1, GFX_TL);
}

// "Home" screen for MODE_BT_RECEIVER (no STATE_MENU grid in this mode, so the
// mode-switch hint doubles as the only way back to Settings/mode-select).
// Per L_Receiver.dc.html: taller panel (130, not 106) with a single centered
// bluetooth icon -- no album art, phone audio has no local file to read art
// for -- connection badge top-right, and title/subtitle in a scrim band
// along the panel's bottom edge (approximated as a flat COLOR_BG-ish fill;
// Adafruit_GFX can't alpha-blend the mockup's literal rgba(0,12,32,.5)
// overlay).
// Deviation: the mockup also shows prev/play/next transport buttons here,
// but bt_receiver.h exposes no AVRCP remote-control API (no play/pause/skip
// function exists for this mode) and it's off-limits to edit, so drawing
// those buttons would be non-functional decoration -- skipped, same as the
// pre-existing "Encoder: volume" footer already implies.
void drawBTReceiverScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BT RECEIVER", HICON_BT);

    tft.fillRoundRect(PL_PANEL_X, PL_PANEL_Y, PL_PANEL_W, BTRX_PANEL_H, 14, COLOR_BT_PANEL);
    int iconCy = PL_PANEL_Y + (BTRX_PANEL_H - BTRX_SCRIM_H) / 2;
    drawBluetoothZigzag(SCREEN_W / 2, iconCy, 20, COLOR_BT_PANEL_TEXT);
    drawConnectionBadge(PL_PANEL_X + PL_PANEL_W - 10 - 11, PL_PANEL_Y + 10 + 11, btrx_connected);

    int scrimY = PL_PANEL_Y + BTRX_PANEL_H - BTRX_SCRIM_H;
    tft.fillRect(PL_PANEL_X, scrimY, PL_PANEL_W, BTRX_SCRIM_H, COLOR_BG);

    String title = btrx_connected ? btrx_deviceName : "Menunggu koneksi";
    title = truncateToWidth(tft, title, PL_PANEL_W - 28, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    gfxDrawString(tft, title, PL_PANEL_X + 14, scrimY + 4, fontToSize(2), GFX_TL);

    String subtitle = String(btrx_playing ? "Playing" : "Idle") + " - Vol " + String((bt_volume * 100) / 127) + "%";
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    gfxDrawString(tft, subtitle, PL_PANEL_X + 14, scrimY + 20, 1, GFX_TL);

    drawFooter("Encoder: volume - Tahan MENU: ganti mode");
}

// Small rounded card, shared by the two Transfer File columns (L_Transfer.dc.html).
static void drawTFCard(int x, int y, int w, int h) {
    tft.fillRoundRect(x, y, w, h, TF_CARD_RADIUS, COLOR_PANEL);
}

// "Home" screen for MODE_TRANSFER_FILE (no STATE_MENU grid in this mode).
// Per L_Transfer.dc.html: two-column layout -- left column is a hotspot-info
// card + a browser-URL card; right column is an upload-progress card plus a
// status chip below it.
// Deviations (wifi_transfer.h/.cpp are off-limits to edit, and don't expose
// this data): no AP password shown (it's a private `static const char*` in
// wifi_transfer.cpp, not an extern -- copying its literal value here would
// silently go stale if that file ever changes it); no mDNS "dap.local" line
// (no mDNS responder is started anywhere in this firmware); the upload card
// shows a generic "Uploading..."/file count instead of a filename+byte-size
// (wifiTransfer_lastUploadPct/_filesReceived are the only upload-progress
// fields that exist -- no filename or size is tracked).
void drawTransferFileScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSFER FILE", HICON_TRANSFER);

    int ly = TF_TOP_Y;
    drawTFCard(TF_LEFT_X, ly, TF_LEFT_W, TF_CARD_H);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_PANEL);
    gfxDrawString(tft, "Hotspot ESP32", TF_LEFT_X + 10, ly + 6, 1, GFX_TL);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_PANEL);
    gfxDrawString(tft, wifiTransfer_ssid, TF_LEFT_X + 10, ly + 18, fontToSize(2), GFX_TL);

    ly += TF_CARD_H + TF_CARD_GAP;
    drawTFCard(TF_LEFT_X, ly, TF_LEFT_W, TF_CARD_H);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_PANEL);
    gfxDrawString(tft, "Buka di browser", TF_LEFT_X + 10, ly + 6, 1, GFX_TL);
    tft.setTextColor(COLOR_ACCENT, COLOR_PANEL);
    gfxDrawString(tft, String("http://") + wifiTransfer_ip, TF_LEFT_X + 10, ly + 18, 1, GFX_TL);

    int ry = TF_TOP_Y;
    drawTFCard(TF_RIGHT_X, ry, TF_RIGHT_W, TF_PROGRESS_CARD_H);
    int pct = wifiTransfer_lastUploadPct;
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_PANEL);
    gfxDrawString(tft, pct >= 0 ? "Menerima..." : "Siap menerima", TF_RIGHT_X + 10, ry + 6, fontToSize(2), GFX_TL);

    int barY = ry + 26, barW = TF_RIGHT_W - 20, barH = 4;
    tft.fillRoundRect(TF_RIGHT_X + 10, barY, barW, barH, barH / 2, COLOR_TRACK_BG);
    int fillW = (barW * constrain(pct, 0, 100)) / 100;
    if (fillW > 0) tft.fillRoundRect(TF_RIGHT_X + 10, barY, fillW, barH, barH / 2, COLOR_WARNING);

    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_PANEL);
    gfxDrawString(tft, pct >= 0 ? String(pct) + "%" : "Menunggu upload", TF_RIGHT_X + 10, barY + 10, 1, GFX_TL);
    String countStr = String(wifiTransfer_filesReceived) + " file";
    int countW = gfxTextWidth(tft, countStr, 1);
    gfxDrawString(tft, countStr, TF_RIGHT_X + TF_RIGHT_W - 10 - countW, barY + 10, 1, GFX_TL);

    // Static informational chip: BT/playback really are off in this mode
    // (MODE_TRANSFER_FILE owns WiFi exclusively, see nav.cpp's homeStateForMode()).
    int chipY = ry + TF_PROGRESS_CARD_H + TF_CARD_GAP;
    const char *chipLabel = "BT dan putar mati";
    int chipTextW = gfxTextWidth(tft, chipLabel, 1);
    int chipW = 10 + 8 + 6 + chipTextW + 10;
    tft.fillRoundRect(TF_RIGHT_X, chipY, chipW, TF_CHIP_H, TF_CHIP_H / 2, COLOR_BUTTON_BG);
    tft.fillCircle(TF_RIGHT_X + 14, chipY + TF_CHIP_H / 2, 4, COLOR_WARNING);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BUTTON_BG);
    gfxDrawString(tft, chipLabel, TF_RIGHT_X + 22, chipY + TF_CHIP_H / 2 - 4, 1, GFX_TL);

    drawFooter("MENU: menu / keluar");
}

// Floating auto-hiding volume pill, painted OVER whatever screen is current --
// does not fillScreen/drawHeader, only nav.cpp's checkVolumeOverlayTimeout()
// triggers the full redraw that erases it. Moved to the top of the content
// area (layout.h's VOL_OVERLAY_*), gained a slider thumb, and now reads
// COLOR_SUCCESS/COLOR_ERROR for the fill+readout instead of COLOR_ACCENT/WARNING.
void drawVolumeOverlay() {
    tft.fillRoundRect(VOL_OVERLAY_X, VOL_OVERLAY_Y, VOL_OVERLAY_W, VOL_OVERLAY_H, VOL_OVERLAY_H / 2, COLOR_BUTTON_BG);

    bool muted = (bt_volume == 0);
    int iconCx = VOL_OVERLAY_X + 22, iconCy = VOL_OVERLAY_Y + VOL_OVERLAY_H / 2;
    drawSpeakerIcon(iconCx, iconCy, 9, muted);

    int volPercent = (bt_volume * 100) / 127;
    String pctStr = String(volPercent) + "%";
    int pctW = gfxTextWidth(tft, pctStr, fontToSize(2));

    int barX = iconCx + 20;
    int barRight = VOL_OVERLAY_X + VOL_OVERLAY_W - 14 - pctW - 10;
    int barW = barRight - barX;
    int barH = 6, barY = iconCy - barH / 2;
    uint16_t fillColor = muted ? COLOR_ERROR : COLOR_SUCCESS;

    if (barW > 0) {
        tft.fillRoundRect(barX, barY, barW, barH, barH / 2, COLOR_OVERLAY_TRACK);
        int fillW = (barW * bt_volume) / 127;
        if (fillW > 2) tft.fillRoundRect(barX, barY, fillW, barH, barH / 2, fillColor);

        int thumbR = 7;
        int thumbCx = barX + fillW;
        if (thumbCx < barX + thumbR) thumbCx = barX + thumbR;
        if (thumbCx > barX + barW - thumbR) thumbCx = barX + barW - thumbR;
        tft.fillCircle(thumbCx, iconCy, thumbR, fillColor);
    }

    tft.setTextColor(fillColor, COLOR_BUTTON_BG);
    gfxDrawString(tft, pctStr, VOL_OVERLAY_X + VOL_OVERLAY_W - 14, iconCy - 7, fontToSize(2), GFX_TR);
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
