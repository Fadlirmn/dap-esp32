// Ported from old/Stream32-ESP-Player.ino lines 721-1134 (drawXScreen functions +
// drawCoverArt + the Player-screen partial-redraw helpers). TFT_eSPI text-datum
// calls become gfxDrawString/truncateToWidth; everything else (fillRoundRect,
// drawRoundRect, fillCircle, drawLine...) is unchanged Adafruit_GFX API.
//
// Added vs. the original: drawFocusRing() calls marking nav.h's `focusIndex` on
// every cursor-navigable screen, since encoder+button input has no finger to tap.
//
// Visual-fidelity pass vs. dap-ui-design/source/*.dc.html: every drawXScreen now
// calls drawHeader(title, icon) for real (it used to silently ignore `title`)
// and drawFooter(hint) at the end; list-style screens (Track/Likes/Playlists/
// PlaylistDetail/BT scan/Settings) draw plain rows with a COLOR_CHIP_BG pill
// only on the focused/current row instead of a bordered card on every row;
// Mode Select rows got an icon circle + subtitle; the Player/BT Receiver
// screens' cover-art square became a mode-identity panel (see
// drawNowPlayingPanel); the volume overlay moved to the top and gained a
// slider thumb. See that pass's handback report for exact deviations.
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

// Shared by drawPlayerScreen (Local Player + BT Transmitter) and
// drawBTReceiverScreen: the mockup has no album art, just a wide
// mode-identity panel whose background/content depend on OperatingMode.
static void drawNowPlayingPanel(int x, int y, int w, int h, OperatingMode mode) {
    int iconY = y + h / 2;
    int iconSize = h / 3;
    if (mode == MODE_BT_RECEIVER) {
        tft.fillRoundRect(x, y, w, h, 14, COLOR_BT_PANEL);
        drawBluetoothZigzag(x + w / 2, iconY, h / 2 - 8, COLOR_BT_PANEL_TEXT);
    } else if (mode == MODE_BT_TRANSMITTER) {
        tft.fillRoundRect(x, y, w, h, 14, COLOR_BT_PANEL);
        drawMusicNoteGlyph(x + w / 5, iconY, iconSize, COLOR_BT_PANEL_TEXT);
        drawFlowArrowIcon(x + w / 2, iconY, iconSize, COLOR_BT_PANEL_TEXT);
        drawBluetoothZigzag(x + w * 4 / 5, iconY, iconSize, COLOR_BT_PANEL_TEXT);
    } else {  // MODE_LOCAL_PLAYER
        tft.fillRoundRect(x, y, w, h, 14, COLOR_ACCENT_DIM);
        drawRadialSignalIcon(x + w / 4, iconY, iconSize, COLOR_ON_ACCENT_DIM);
        drawRadialSignalIcon(x + w * 3 / 4, iconY, iconSize, COLOR_ON_ACCENT_DIM);
        int dividerW = w / 10, dividerH = h / 2;
        if (dividerW < 6) dividerW = 6;
        tft.fillRoundRect(x + w / 2 - dividerW / 2, y + (h - dividerH) / 2, dividerW, dividerH, 4, COLOR_BG);
    }
}

// Shared connection-status pill: dot (COLOR_SUCCESS/COLOR_WARNING) + label on
// COLOR_BUTTON_BG, centered at centerX. Used by BT Transmitter (in
// drawPlayerScreen) and BT Receiver -- Local Player has no connection concept
// and skips this entirely.
static void drawStatusChip(int centerX, int topY, bool ok, const String &label) {
    int textW = gfxTextWidth(tft, label, 1);
    int dotR = 4;
    int chipW = dotR * 2 + 8 + textW + 20;
    int chipH = PL_CHIP_H;
    int x = centerX - chipW / 2;
    tft.fillRoundRect(x, topY, chipW, chipH, chipH / 2, COLOR_BUTTON_BG);
    int dotCx = x + 14, dotCy = topY + chipH / 2;
    tft.fillCircle(dotCx, dotCy, dotR, ok ? COLOR_SUCCESS : COLOR_WARNING);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BUTTON_BG);
    gfxDrawString(tft, label, dotCx + dotR + 6, topY + chipH / 2 - 4, 1, GFX_TL);
}

void drawPlayerScreen() {
    tft.fillScreen(COLOR_BG);
    bool isTransmitter = (operatingMode == MODE_BT_TRANSMITTER);
    drawHeader("NOW PLAYING", isTransmitter ? HICON_BT : HICON_NONE);

    drawNowPlayingPanel(PL_PANEL_X, PL_PANEL_Y, PL_PANEL_W, PL_PANEL_H,
                         isTransmitter ? MODE_BT_TRANSMITTER : MODE_LOCAL_PLAYER);

    if (isTransmitter) {
        drawStatusChip(SCREEN_W / 2, PL_CHIP_Y, bt_connected,
                        bt_connected ? "Terhubung" : (bt_connecting ? "Menghubungkan..." : "Terputus"));
    }

    String rawName = (total_tracks > 0) ? trackDisplayName(playlist[current_track_index]) : "No tracks!";
    String artist = "Unknown Artist";
    String title = rawName;
    int hyphenIdx = rawName.indexOf(" - ");
    if (hyphenIdx != -1) {
        artist = rawName.substring(0, hyphenIdx);
        title = rawName.substring(hyphenIdx + 3);
    }
    title = truncateToWidth(tft, title, SCREEN_W - PL_INFO_X - 40, 2);

    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    gfxDrawString(tft, title, PL_INFO_X, PL_TITLE_Y, fontToSize(2), GFX_TL);

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
    subtitle = truncateToWidth(tft, subtitle, SCREEN_W - PL_INFO_X - 40, 1);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    gfxDrawString(tft, subtitle, PL_INFO_X, PL_ARTIST_Y, 1, GFX_TL);

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

// Play/Pause always fills COLOR_ACCENT with a COLOR_ACCENT_TEXT icon now (the
// previous port swapped to COLOR_ACCENT_DIM fill + COLOR_BG icon while
// playing, both wrong per dap-ui-design -- the play/pause glyph swap alone is
// the "is playing" indicator).
void updatePlayPauseButton() {
    if (currentState != STATE_PLAYER) return;
    tft.fillCircle(PL_PLAY_CX, PL_BTN_CY, PL_BTN_R, COLOR_ACCENT);
    if (is_playing) {
        drawPauseIcon(PL_PLAY_CX, PL_BTN_CY, 11, COLOR_ACCENT_TEXT);
    } else {
        drawPlayIcon(PL_PLAY_CX, PL_BTN_CY, 11, COLOR_ACCENT_TEXT);
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
        tft.fillRoundRect(10, listY, SCREEN_W - 20, LIST_ITEM_H, 12, COLOR_CHIP_BG);
        bg = COLOR_CHIP_BG;
    }

    int iconCx = 26, iconCy = listY + LIST_ITEM_H / 2;
    if (isCurrent) {
        drawEqualizerBars(iconCx, iconCy, 12, 14, COLOR_ACCENT);
    } else {
        String num = (originalTrackIdx + 1) < 10 ? "0" + String(originalTrackIdx + 1) : String(originalTrackIdx + 1);
        tft.setTextColor(COLOR_TEXT_SECONDARY, bg);
        gfxDrawString(tft, num, iconCx, iconCy - 4, 1, GFX_MC);
    }

    String name = trackDisplayName(playlist[originalTrackIdx]);
    name = truncateToWidth(tft, name, SCREEN_W - 60, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
    gfxDrawString(tft, name, 44, listY + (LIST_ITEM_H - 14) / 2, fontToSize(2), GFX_TL);
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

// Shared no-card row for Playlists/PlaylistDetail: plain row, COLOR_CHIP_BG
// pill only when focused, optional secondary line (dap-ui-design's "N lagu").
// No touch hit-testing depends on this screen's geometry (grep-verified), so
// row height/position are free to retune independent of Track List's.
static void drawSimpleRowList(const String &name, const String &secondary, int row) {
    int listY = LIST_ITEM_Y_START + row * (LIST_ITEM_H + LIST_ITEM_GAP);
    bool focused = (focusIndex == row);
    uint16_t bg = COLOR_BG;
    if (focused) {
        tft.fillRoundRect(10, listY, SCREEN_W - 20, LIST_ITEM_H, 12, COLOR_CHIP_BG);
        bg = COLOR_CHIP_BG;
    }

    String num = (row + 1) < 10 ? "0" + String(row + 1) : String(row + 1);
    tft.setTextColor(COLOR_TEXT_SECONDARY, bg);
    gfxDrawString(tft, num, 26, listY + LIST_ITEM_H / 2 - 4, 1, GFX_MC);

    String title = truncateToWidth(tft, name, SCREEN_W - 60, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
    int titleY = secondary.length() ? listY + 3 : listY + (LIST_ITEM_H - 14) / 2;
    gfxDrawString(tft, title, 44, titleY, fontToSize(2), GFX_TL);
    if (secondary.length()) {
        tft.setTextColor(COLOR_TEXT_SECONDARY, bg);
        gfxDrawString(tft, secondary, 44, listY + 18, 1, GFX_TL);
    }
}

void drawPlaylistsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("PLAYLISTS", HICON_LIST);
    for (int i = 0; i < totalPlaylists; i++) {
        drawSimpleRowList(playlistNames[i], String(playlistTrackCount(i)) + " lagu", i);
    }
    if (totalPlaylists == 0) {
        tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
        gfxDrawString(tft, "No playlists", SCREEN_W / 2, 140, fontToSize(2), GFX_MC);
    }
    drawFooter(String(totalPlaylists) + " playlist - Klik buka");
}

// Drill-down screen (opened from Playlists): back-chevron header icon +
// real playlist name as the title, per dap-ui-design's "Favorit" example.
void drawPlaylistDetailScreen() {
    tft.fillScreen(COLOR_BG);
    int count = playlistTrackCount(playlist_selectedIndex);
    String title = (playlist_selectedIndex >= 0 && playlist_selectedIndex < totalPlaylists)
                        ? playlistNames[playlist_selectedIndex]
                        : "PLAYLIST";
    drawHeader(title.c_str(), HICON_BACK);
    // No "currently playing" eq-bars here: playlist.h has no API to map a
    // (playlist index, row) pair back to current_track_index without adding
    // new playlist.cpp code, which is out of this pass's file scope.
    for (int i = 0; i < count; i++) {
        drawSimpleRowList(playlistTrackDisplayName(playlist_selectedIndex, i), "", i);
    }
    if (count == 0) {
        tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
        gfxDrawString(tft, "No tracks", SCREEN_W / 2, 140, fontToSize(2), GFX_MC);
    }
    drawFooter(String(count) + " lagu - Klik putar");
}

// Mode Select row: 32x32 icon circle (COLOR_BUTTON_BG bg, COLOR_ACCENT 18x18
// stroked glyph) + title + subtitle, COLOR_CHIP_BG pill on the focused row
// (replacing the old bordered-card + drawFocusRing), "AKTIF" chip on whichever
// row matches the live operatingMode.
void drawModeSelectScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("MODE");
    const char *labels[] = {"Local Player", "BT Receiver", "BT Transmitter", "Transfer File", "Kembali"};
    const char *subtitles[] = {"SD card ke IEM", "HP ke IEM", "SD card ke headset BT", "Upload lewat WiFi", ""};
    for (uint8_t i = 0; i < 5; i++) {
        int y = MODE_LIST_Y_START + i * (MODE_LIST_ITEM_H + MODE_LIST_GAP);
        bool focused = (focusIndex == i);
        uint16_t bg = COLOR_BG;
        if (focused) {
            tft.fillRoundRect(10, y, SCREEN_W - 20, MODE_LIST_ITEM_H, 12, COLOR_CHIP_BG);
            bg = COLOR_CHIP_BG;
        }

        int iconCx = 26, iconCy = y + MODE_LIST_ITEM_H / 2;
        tft.fillCircle(iconCx, iconCy, 16, COLOR_BUTTON_BG);
        switch (i) {
            case 0: drawMusicNoteGlyph(iconCx, iconCy, 9, COLOR_ACCENT); break;
            case 1: drawBluetoothZigzag(iconCx, iconCy, 9, COLOR_ACCENT); break;
            case 2: drawBluetoothZigzag(iconCx, iconCy, 9, COLOR_ACCENT); break;
            case 3: drawUploadIcon(iconCx, iconCy, 9, COLOR_ACCENT); break;
            case 4: drawBackIcon(iconCx, iconCy, 8, COLOR_ACCENT); break;
        }

        tft.setTextColor(COLOR_TEXT_PRIMARY, bg);
        gfxDrawString(tft, labels[i], 50, y + 4, fontToSize(2), GFX_TL);
        if (subtitles[i][0]) {
            tft.setTextColor(COLOR_TEXT_SECONDARY, bg);
            gfxDrawString(tft, subtitles[i], 50, y + 19, 1, GFX_TL);
        }

        if (i < 4 && (OperatingMode)i == operatingMode) {
            int chipW = 44, chipH = 16;
            int chipX = SCREEN_W - 20 - chipW - 6, chipY = y + (MODE_LIST_ITEM_H - chipH) / 2;
            tft.fillRoundRect(chipX, chipY, chipW, chipH, chipH / 2, COLOR_CHIP_BG);
            tft.setTextColor(COLOR_SUCCESS, COLOR_CHIP_BG);
            gfxDrawString(tft, "AKTIF", chipX + chipW / 2, chipY + chipH / 2 - 3, 1, GFX_MC);
        }
    }
    drawFooter("Klik: OK - MENU: kembali");
}

// Full-screen transition message, mirroring drawScreenLocked()'s simple
// centered-text style. No header/footer: the device restarts ~600ms after
// this draws (see nav.cpp's selectOperatingMode), so there's nothing to
// navigate back to.
void drawModeSwitchingScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    gfxDrawString(tft, "Beralih mode...", SCREEN_W / 2, SCREEN_H / 2 - 10, fontToSize(4), GFX_MC);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    gfxDrawString(tft, "Restarting...", SCREEN_W / 2, SCREEN_H / 2 + 20, fontToSize(2), GFX_MC);
}

// "Home" screen for MODE_BT_RECEIVER (no STATE_MENU grid in this mode, so the
// mode-switch hint doubles as the only way back to Settings/mode-select).
// Same panel+chip+title/subtitle hierarchy as drawPlayerScreen's BT
// Transmitter case, minus progress/transport (there's no scrubbing a live
// A2DP stream) -- restyled from the same btrx_*/bt_volume variables, no new
// data source invented.
void drawBTReceiverScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BT RECEIVER", HICON_BT);

    drawNowPlayingPanel(PL_PANEL_X, PL_PANEL_Y, PL_PANEL_W, PL_PANEL_H, MODE_BT_RECEIVER);

    drawStatusChip(SCREEN_W / 2, PL_CHIP_Y, btrx_connected, btrx_connected ? "Terhubung" : "Menunggu HP...");

    String title = btrx_connected ? btrx_deviceName : "Menunggu koneksi";
    title = truncateToWidth(tft, title, SCREEN_W - 40, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    gfxDrawString(tft, title, SCREEN_W / 2, PL_TITLE_Y, fontToSize(2), GFX_MC);

    String subtitle = String(btrx_playing ? "Playing" : "Idle") + " - Vol " + String((bt_volume * 100) / 127) + "%";
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    gfxDrawString(tft, subtitle, SCREEN_W / 2, PL_ARTIST_Y, 1, GFX_MC);

    drawFooter("Encoder: volume - Tahan MENU: ganti mode");
}

// "Home" screen for MODE_TRANSFER_FILE (same no-grid situation as BT Receiver).
// No "now playing" panel here (doesn't apply, per the redesign spec) -- just
// header/footer wiring and dropping the old inline "Hold MENU" line (now in
// the footer).
void drawTransferFileScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSFER FILE", HICON_TRANSFER);

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
