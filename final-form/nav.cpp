// State machine + input handlers. The original drove everything through
// processGlobalTouch (ported below, unchanged, as the optional secondary path);
// this adds the primary encoder+button path per the approved control mapping:
//   rotate = volume (always) | PREV/NEXT = cursor move, or track skip in Player
//   click = select/play-pause | double-click = next track (Player) | long-press = like (Player)
//   MENU = back, reproducing the original's exact per-screen back targets.
#include "nav.h"
#include <Preferences.h>
#include "screens.h"
#include "theme.h"
#include "layout.h"
#include "bluetooth.h"
#include "audio_decoder.h"
#include "playlist.h"
#include "bt_receiver.h"
#include "i2s_output.h"

extern Preferences preferences;

UIState currentState = STATE_MENU;
int focusIndex = 0;
int trackList_currentPage = 0;
int likesList_currentPage = 0;
uint8_t screenTimeoutMinutes = 0;
bool screenLocked = false;
static unsigned long lastActivityAt = 0;

OperatingMode operatingMode = MODE_LOCAL_PLAYER;
unsigned long lastVolumeChangeAt = 0;

// Must run before audioDecoderSetup()/btSetup()/i2sOutputSetup()/etc. so
// final-form.ino's setup() knows which subsystems to bring up.
void loadOperatingMode() {
    preferences.begin("mode_pref", true);
    operatingMode = (OperatingMode)preferences.getUChar("mode", MODE_LOCAL_PLAYER);
    preferences.end();
    if (operatingMode > MODE_TRANSFER_FILE) operatingMode = MODE_LOCAL_PLAYER;
}

void enterModeSelect() {
    currentState = STATE_MODE_SELECT;
    focusIndex = (int)operatingMode;
    drawCurrentStateUI();
}

// "Beralih mode" transition screen, then restart into the new mode. Only one
// mode runs per boot (Local Player / BT Receiver / BT Transmitter / Transfer
// File each own the radio/I2S/WiFi hardware exclusively), so switching is a
// full restart rather than a live subsystem swap.
void selectOperatingMode(OperatingMode m) {
    if (m == operatingMode) { enterState(STATE_MENU); return; }
    preferences.begin("mode_pref", false);
    preferences.putUChar("mode", (uint8_t)m);
    preferences.end();
    currentState = STATE_MODE_SWITCHING;
    drawCurrentStateUI();
    delay(600);
    ESP.restart();
}

void checkVolumeOverlayTimeout() {
    if (lastVolumeChangeAt != 0 && millis() - lastVolumeChangeAt >= VOLUME_OVERLAY_MS) {
        lastVolumeChangeAt = 0;
        if (!screenLocked) drawCurrentStateUI();
    }
}

// Any input handler starts with this: wakes a locked screen (consuming the
// event as just a wake-up, same as the original's touch early-return) and
// otherwise just resets the idle timer.
static bool consumeWakeOrNoteActivity() {
    lastActivityAt = millis();
    if (screenLocked) {
        screenLocked = false;
        drawCurrentStateUI();
        return true;
    }
    return false;
}

void checkScreenTimeout() {
    if (screenLocked || screenTimeoutMinutes == 0) return;
    if (millis() - lastActivityAt >= (unsigned long)screenTimeoutMinutes * 60000UL) {
        screenLocked = true;
        drawScreenLocked();
    }
}

void enterState(UIState newState) {
    currentState = newState;
    focusIndex = 0;
    drawCurrentStateUI();
}

// ---- item counts for the generic focus-cursor screens ----
static int settingsListCount() { return 6; }
static int themeListCount() { return 5; }
static int resetConfirmCount() { return 2; }
static int btManagerCount() { return 2; }
static int screenTimeoutCount() { return 4; }
static int menuGridCount() { return 6; }  // Player/Tracks/Playlists/Likes/Settings/Mode-switch
static int modeSelectCount() { return 5; }  // Local Player/BT Receiver/BT Transmitter/Transfer File/Kembali
static int playlistsCount() { return totalPlaylists; }

// Which playlist STATE_PLAYLIST_DETAIL is showing. ponytail: no paging within
// a playlist yet (focusIndex maps 1:1 to track index) -- add LIST_ITEMS_PER_PAGE
// paging like the track list if playlists ever need to exceed one screen.
int playlist_selectedIndex = 0;

// STATE_MENU, STATE_BT_RECEIVER and STATE_TRANSFER_FILE are each only valid
// for specific modes; "home" is whichever one boots for the active mode, and
// is also where cancelling out of the mode-select screen lands.
UIState homeStateForMode() {
    switch (operatingMode) {
        case MODE_BT_RECEIVER: return STATE_BT_RECEIVER;
        case MODE_TRANSFER_FILE: return STATE_TRANSFER_FILE;
        default: return STATE_MENU;  // MODE_LOCAL_PLAYER, MODE_BT_TRANSMITTER
    }
}

static int trackListMatchCount(bool onlyLikes) {
    int n = 0;
    for (int i = 0; i < total_tracks; i++) {
        if (!onlyLikes || isLiked[i]) n++;
    }
    return n;
}

// ---- select/activate the focused item (encoder click, non-Player states) ----
static void selectTrackListItem(bool onlyLikes) {
    int matchingTracks[MAX_TRACKS];
    int matchCount = 0;
    for (int i = 0; i < total_tracks; i++) {
        if (!onlyLikes || isLiked[i]) matchingTracks[matchCount++] = i;
    }
    int page = onlyLikes ? likesList_currentPage : trackList_currentPage;
    int itemIdx = page * LIST_ITEMS_PER_PAGE + focusIndex;
    if (itemIdx < matchCount) {
        current_track_index = matchingTracks[itemIdx];
        switchTrack(0);
        drawCurrentStateUI();
    }
}

static void doReset() {
    preferences.begin("bt_pref", false); preferences.clear(); preferences.end();
    preferences.begin("vol_pref", false); preferences.clear(); preferences.end();
    preferences.begin("play_pref", false); preferences.clear(); preferences.end();
    preferences.begin("resume_pref", false); preferences.clear(); preferences.end();
    preferences.begin("screen_pref", false); preferences.clear(); preferences.end();
    autoPlayEnabled = false;
    bt_volume = 70;
    screenTimeoutMinutes = 0;
    themeIndex = 0;
    applyTheme(themeIndex);
    hasSavedBTAddress = false;
    savedBTDevice = "No device saved";
}

static void handleSelect() {
    switch (currentState) {
        case STATE_MENU:
            switch (focusIndex) {
                case 0: enterState(STATE_PLAYER); break;
                case 1: trackList_currentPage = 0; enterState(STATE_TRACK_LIST); break;
                case 2: enterState(STATE_PLAYLISTS); break;
                case 3: likesList_currentPage = 0; enterState(STATE_LIKES_LIST); break;
                case 4: enterState(STATE_SETTINGS); break;
                case 5: enterModeSelect(); break;
            }
            break;
        case STATE_TRACK_LIST: selectTrackListItem(false); break;
        case STATE_LIKES_LIST: selectTrackListItem(true); break;
        case STATE_PLAYLISTS:
            if (focusIndex < totalPlaylists) {
                playlist_selectedIndex = focusIndex;
                enterState(STATE_PLAYLIST_DETAIL);
            }
            break;
        case STATE_PLAYLIST_DETAIL:
            if (playPlaylistTrack(playlist_selectedIndex, focusIndex)) enterState(STATE_PLAYER);
            break;
        case STATE_MODE_SELECT:
            if (focusIndex == 4) { enterState(homeStateForMode()); }  // Kembali
            else selectOperatingMode((OperatingMode)focusIndex);
            break;
        case STATE_SETTINGS:
            switch (focusIndex) {
                case 0: enterState(STATE_BT_MANAGER); break;
                case 1: enterState(STATE_SCREEN_SETTINGS); break;
                case 2: enterState(STATE_PLAYBACK_SETTINGS); break;
                case 3: enterState(STATE_THEME_SETTINGS); break;
                case 4: enterState(STATE_INFO); break;
                case 5: enterState(STATE_RESET_CONFIRM); break;
            }
            break;
        case STATE_PLAYBACK_SETTINGS:
            autoPlayEnabled = !autoPlayEnabled;
            preferences.begin("play_pref", false);
            preferences.putBool("autoplay", autoPlayEnabled);
            preferences.end();
            drawCurrentStateUI();
            break;
        case STATE_SCREEN_SETTINGS: {
            const uint8_t values[] = {1, 5, 10, 0};
            screenTimeoutMinutes = values[focusIndex];
            preferences.begin("screen_pref", false);
            preferences.putUChar("timeout", screenTimeoutMinutes);
            preferences.end();
            drawCurrentStateUI();
            break;
        }
        case STATE_THEME_SETTINGS:
            themeIndex = focusIndex;
            applyTheme(themeIndex);
            preferences.begin("screen_pref", false);
            preferences.putUChar("theme", themeIndex);
            preferences.end();
            drawCurrentStateUI();
            break;
        case STATE_RESET_CONFIRM:
            if (focusIndex == 0) {
                enterState(STATE_SETTINGS);
            } else {
                doReset();
                enterState(STATE_SETTINGS);
            }
            break;
        case STATE_BT_MANAGER:
            if (focusIndex == 0) toggleBluetoothConnection();
            else startBluetoothScan();
            drawCurrentStateUI();
            break;
        case STATE_BT_SCAN:
            if (focusIndex < btScanCount) selectBluetoothScanResult(focusIndex);
            break;
        default: break;
    }
}

void handleEncoderClick() {
    if (consumeWakeOrNoteActivity()) return;
    if (currentState == STATE_PLAYER) {
        bool wasPlaying = is_playing;
        if (wasPlaying) {
            is_playing = false;
            play_requested = false;
            saveResumeState();
        } else {
            play_requested = true;
            play_request_started_at = millis();
            updatePlaybackFromBluetooth();
        }
        updatePlayPauseButton();
        return;
    }
    handleSelect();
}

void handleEncoderDoubleClick() {
    if (consumeWakeOrNoteActivity()) return;
    if (currentState == STATE_PLAYER) switchTrack(1);
}

void handleEncoderLongPress() {
    if (consumeWakeOrNoteActivity()) return;
    if (currentState == STATE_PLAYER) toggleLike();
}

void handleEncoderRotate(int delta) {
    if (consumeWakeOrNoteActivity()) return;
    if (delta == 0) return;
    constexpr int VOLUME_STEP = 4;
    int v = (int)bt_volume + delta * VOLUME_STEP;
    bt_volume = (uint8_t)constrain(v, 0, 127);
    switch (operatingMode) {
        case MODE_BT_TRANSMITTER: a2dp_source.set_volume(bt_volume); break;
        case MODE_LOCAL_PLAYER: i2sOutputSetVolume(bt_volume); break;
        case MODE_BT_RECEIVER: btReceiverSetVolume(bt_volume); break;
        case MODE_TRANSFER_FILE: break;  // no audio path in this mode
    }
    preferences.begin("vol_pref", false);
    preferences.putUChar("volume", bt_volume);
    preferences.end();
    lastVolumeChangeAt = millis();
    updateVolumeUI();
    drawVolumeOverlay();
}

// ---- PREV/NEXT: track skip in Player, cursor move (with paging) elsewhere ----
static void moveFocus(int dir, int count) {
    if (count <= 0) return;
    focusIndex += dir;
    if (focusIndex < 0) focusIndex = 0;
    if (focusIndex > count - 1) focusIndex = count - 1;
    drawCurrentStateUI();
}

static void moveListFocus(bool onlyLikes, int dir) {
    int matchCount = trackListMatchCount(onlyLikes);
    int maxPages = (matchCount + LIST_ITEMS_PER_PAGE - 1) / LIST_ITEMS_PER_PAGE;
    if (maxPages == 0) maxPages = 1;
    int &page = onlyLikes ? likesList_currentPage : trackList_currentPage;
    int itemsOnPage = min(LIST_ITEMS_PER_PAGE, matchCount - page * LIST_ITEMS_PER_PAGE);
    if (itemsOnPage < 0) itemsOnPage = 0;

    if (dir > 0) {
        if (focusIndex < itemsOnPage - 1) { focusIndex++; }
        else if (page < maxPages - 1) { page++; focusIndex = 0; }
    } else {
        if (focusIndex > 0) { focusIndex--; }
        else if (page > 0) { page--; focusIndex = LIST_ITEMS_PER_PAGE - 1; }
    }
    drawCurrentStateUI();
}

void handlePrevButton() {
    if (consumeWakeOrNoteActivity()) return;
    if (currentState == STATE_PLAYER) { switchTrack(-1); return; }
    switch (currentState) {
        case STATE_MENU: moveFocus(-1, menuGridCount()); break;
        case STATE_TRACK_LIST: moveListFocus(false, -1); break;
        case STATE_LIKES_LIST: moveListFocus(true, -1); break;
        case STATE_SETTINGS: moveFocus(-1, settingsListCount()); break;
        case STATE_THEME_SETTINGS: moveFocus(-1, themeListCount()); break;
        case STATE_RESET_CONFIRM: moveFocus(-1, resetConfirmCount()); break;
        case STATE_BT_MANAGER: moveFocus(-1, btManagerCount()); break;
        case STATE_SCREEN_SETTINGS: moveFocus(-1, screenTimeoutCount()); break;
        case STATE_BT_SCAN: moveFocus(-1, btScanCount); break;
        case STATE_PLAYLISTS: moveFocus(-1, playlistsCount()); break;
        case STATE_PLAYLIST_DETAIL: moveFocus(-1, playlistTrackCount(playlist_selectedIndex)); break;
        case STATE_MODE_SELECT: moveFocus(-1, modeSelectCount()); break;
        default: break;
    }
}

void handleNextButton() {
    if (consumeWakeOrNoteActivity()) return;
    if (currentState == STATE_PLAYER) { switchTrack(1); return; }
    switch (currentState) {
        case STATE_MENU: moveFocus(1, menuGridCount()); break;
        case STATE_TRACK_LIST: moveListFocus(false, 1); break;
        case STATE_LIKES_LIST: moveListFocus(true, 1); break;
        case STATE_SETTINGS: moveFocus(1, settingsListCount()); break;
        case STATE_THEME_SETTINGS: moveFocus(1, themeListCount()); break;
        case STATE_RESET_CONFIRM: moveFocus(1, resetConfirmCount()); break;
        case STATE_BT_MANAGER: moveFocus(1, btManagerCount()); break;
        case STATE_SCREEN_SETTINGS: moveFocus(1, screenTimeoutCount()); break;
        case STATE_BT_SCAN: moveFocus(1, btScanCount); break;
        case STATE_PLAYLISTS: moveFocus(1, playlistsCount()); break;
        case STATE_PLAYLIST_DETAIL: moveFocus(1, playlistTrackCount(playlist_selectedIndex)); break;
        case STATE_MODE_SELECT: moveFocus(1, modeSelectCount()); break;
        default: break;
    }
}

// MENU button: reproduces the original back-button's exact per-screen targets
// (ported from the pointInRect(x,y,8,5,55,24) branch of processGlobalTouch).
void handleMenuButton() {
    if (consumeWakeOrNoteActivity()) return;
    if (currentState == homeStateForMode()) return;
    switch (currentState) {
        case STATE_SCREEN_SETTINGS:
        case STATE_PLAYBACK_SETTINGS:
        case STATE_INFO:
        case STATE_RESET_CONFIRM:
        case STATE_THEME_SETTINGS:
            enterState(STATE_SETTINGS);
            break;
        case STATE_BT_SCAN:
            enterState(STATE_BT_MANAGER);
            break;
        case STATE_PLAYLIST_DETAIL:
            enterState(STATE_PLAYLISTS);
            break;
        case STATE_MODE_SELECT:
        case STATE_MODE_SWITCHING:
            enterState(homeStateForMode());
            break;
        default:
            enterState(homeStateForMode());
            break;
    }
}

// Long press (wired in input.cpp): open the mode switcher from anywhere,
// including the two dedicated-screen modes that have no STATE_MENU grid.
void handleMenuLongPress() {
    if (consumeWakeOrNoteActivity()) return;
    if (currentState == STATE_MODE_SWITCHING) return;  // mid-restart, ignore
    enterModeSelect();
}

#if ENABLE_TOUCH
// ---- optional secondary input path: ported verbatim from old/Stream32-ESP-Player.ino
// lines 1308-1590, swapping only the globals for their final-form equivalents. ----
static bool pointInRect(int x, int y, int rx, int ry, int rw, int rh) {
    return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

static bool pointInCircle(int x, int y, int cx, int cy, int r) {
    int dx = x - cx, dy = y - cy;
    return dx * dx + dy * dy <= r * r;
}

void processGlobalTouch(int x, int y) {
    if (consumeWakeOrNoteActivity()) return;
    if (y < HEADER_H && x >= SCREEN_W - 42) {
        currentState = STATE_BT_MANAGER;
        drawCurrentStateUI();
        return;
    }
    if (currentState != homeStateForMode()) {
        if (pointInRect(x, y, 8, 5, 55, 24)) {
            handleMenuButton();
            return;
        }
    }

    switch (currentState) {
        case STATE_MENU: {
            for (int i = 0; i < 6; i++) {
                if (pointInCircle(x, y, MENU_CELL_CX[i % 3], MENU_CELL_CY[i / 3], MENU_CELL_R)) {
                    focusIndex = i;
                    handleSelect();
                    break;
                }
            }
            break;
        }

        case STATE_PLAYER: {
            if (pointInCircle(x, y, PL_PREV_CX, PL_BTN_CY, PL_BTN_R)) {
                switchTrack(-1);
            } else if (pointInCircle(x, y, PL_PLAY_CX, PL_BTN_CY, PL_BTN_R)) {
                handleEncoderClick();
            } else if (pointInCircle(x, y, PL_NEXT_CX, PL_BTN_CY, PL_BTN_R)) {
                switchTrack(1);
            } else if (pointInRect(x, y, PL_HEART_CX - PL_HEART_R - 6, PL_HEART_CY - PL_HEART_R - 6, (PL_HEART_R + 6) * 2, (PL_HEART_R + 6) * 2)) {
                toggleLike();
            } else if (pointInRect(x, y, PL_PROGRESS_X - 6, PL_PROGRESS_Y - 10, PL_PROGRESS_W + 12, PL_PROGRESS_H + 20)) {
                float ratio = (float)(x - PL_PROGRESS_X) / PL_PROGRESS_W;
                if (ratio < 0.0f) ratio = 0.0f;
                if (ratio > 1.0f) ratio = 1.0f;
                seekToRatio(ratio);
            } else if (pointInRect(x, y, PL_VOLDOWN_X, PL_VOL_Y, PL_VOLDOWN_W, PL_VOL_H)) {
                handleEncoderRotate(-2);
            } else if (pointInRect(x, y, PL_VOLUP_X, PL_VOL_Y, PL_VOLUP_W, PL_VOL_H)) {
                handleEncoderRotate(2);
            }
            break;
        }

        case STATE_TRACK_LIST:
        case STATE_LIKES_LIST: {
            bool onlyLikes = (currentState == STATE_LIKES_LIST);
            int matchCount = trackListMatchCount(onlyLikes);
            int page = onlyLikes ? likesList_currentPage : trackList_currentPage;
            int maxPages = (matchCount + LIST_ITEMS_PER_PAGE - 1) / LIST_ITEMS_PER_PAGE;
            if (maxPages == 0) maxPages = 1;

            bool hit = false;
            for (int i = 0; i < LIST_ITEMS_PER_PAGE; i++) {
                int listY = LIST_ITEM_Y_START + i * (LIST_ITEM_H + LIST_ITEM_GAP);
                if (pointInRect(x, y, 10, listY, SCREEN_W - 20, LIST_ITEM_H)) {
                    focusIndex = i;
                    selectTrackListItem(onlyLikes);
                    hit = true;
                    break;
                }
            }
            if (hit) break;

            if (pointInRect(x, y, 20, 204, 50, 28)) {
                if (page > 0) { (onlyLikes ? likesList_currentPage : trackList_currentPage)--; focusIndex = 0; drawCurrentStateUI(); }
            } else if (pointInRect(x, y, SCREEN_W - 70, 204, 50, 28)) {
                if (page < maxPages - 1) { (onlyLikes ? likesList_currentPage : trackList_currentPage)++; focusIndex = 0; drawCurrentStateUI(); }
            }
            break;
        }

        case STATE_SETTINGS: {
            bool hit = false;
            for (uint8_t i = 0; i < 6; i++) {
                if (pointInRect(x, y, 10, 34 + i * 30, SCREEN_W - 20, 26)) { focusIndex = i; hit = true; break; }
            }
            if (!hit) return;
            handleSelect();
            break;
        }

        case STATE_PLAYBACK_SETTINGS: {
            if (pointInRect(x, y, 10, 40, SCREEN_W - 20, 34)) handleSelect();
            break;
        }

        case STATE_SCREEN_SETTINGS: {
            for (uint8_t i = 0; i < 4; i++) {
                int sx = 12 + i * 76;
                if (pointInRect(x, y, sx, 78, 70, 32)) { focusIndex = i; handleSelect(); break; }
            }
            break;
        }

        case STATE_THEME_SETTINGS: {
            for (uint8_t i = 0; i < 5; i++) {
                if (pointInRect(x, y, 18, 36 + i * 34, SCREEN_W - 36, 30)) { focusIndex = i; handleSelect(); break; }
            }
            break;
        }

        case STATE_RESET_CONFIRM: {
            if (pointInRect(x, y, 18, 178, 96, 34)) { focusIndex = 0; handleSelect(); }
            else if (pointInRect(x, y, 126, 178, 96, 34)) { focusIndex = 1; handleSelect(); }
            break;
        }

        case STATE_BT_MANAGER: {
            if (pointInRect(x, y, 20, 166, SCREEN_W - 40, 30)) { focusIndex = 0; handleSelect(); }
            else if (pointInRect(x, y, 20, 202, SCREEN_W - 40, 30)) { focusIndex = 1; handleSelect(); }
            break;
        }

        case STATE_BT_SCAN: {
            for (uint8_t i = 0; i < btScanCount; i++) {
                int rowY = 52 + i * 27;
                if (pointInRect(x, y, 10, rowY, SCREEN_W - 20, 24)) { focusIndex = i; handleSelect(); return; }
            }
            break;
        }
    }
}
#endif
