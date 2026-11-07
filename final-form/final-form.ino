// Stream32 player UI/feature-set, ported onto this board's real wiring
// (old/files/test_input.ino + old/files/plan.md). setup()/loop() glue only -- see
// pins.h for the wiring and nav.h for the control mapping.
//
// Rotation differs from test_input.ino on purpose: Stream32's layout (layout.h)
// is portrait 240x320, so rotation is 0 here, not test_input.ino's 1.
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SD.h>
#include <Preferences.h>

#include "pins.h"
#include "theme.h"
#include "layout.h"
#include "gfx_helpers.h"
#include "icons.h"
#include "screens.h"
#include "nav.h"
#include "input.h"
#include "touch.h"
#include "playlist.h"
#include "bluetooth.h"
#include "audio_decoder.h"

Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
Preferences preferences;

static void fatalError(const char *msg) {
    tft.fillScreen(ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE);
    gfxDrawString(tft, msg, SCREEN_W / 2, 110, fontToSize(4), GFX_MC);
    while (true) delay(1000);
}

void setup() {
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(0);
    initColorTheme();

    tft.fillScreen(COLOR_BG);
    drawHeader("SYSTEM START", false);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    gfxDrawString(tft, "Initializing...", SCREEN_W / 2, 110, fontToSize(2), GFX_MC);

    inputSetup();
    audioDecoderSetup();

    bool sdReady = SD.begin(PIN_SD_CS);
    if (!sdReady) {
        Serial.println("[SD] First init failed, retrying...");
        delay(250);
        sdReady = SD.begin(PIN_SD_CS);
    }
    if (!sdReady) fatalError("SD Error!");

    loadPlaylist();
    if (total_tracks > 0) {
        loadLikes();
        preferences.begin("resume_pref", true);
        String resumeTrack = preferences.getString("track", "");
        uint32_t resumeBytes = preferences.getUInt("bytes", 0);
        preferences.end();

        current_track_index = 0;
        for (int i = 0; i < total_tracks; i++) {
            if (playlist[i] == resumeTrack) { current_track_index = i; break; }
        }
        if (openCurrentTrack()) {
            size_t maxResume = g_file_size > 4096 ? (size_t)g_file_size - 4096 : (size_t)0;
            size_t safeResume = resumeBytes > maxResume ? maxResume : resumeBytes;
            // Resume slightly before the saved read position so minimp3 can find a complete frame.
            if (safeResume > 2048) safeResume -= 2048;
            if (safeResume > 0) audioFile.seek(safeResume);
            g_file_bytes_read = safeResume;
            is_playing = false;
        }
    }

    preferences.begin("screen_pref", true);
    screenTimeoutMinutes = preferences.getUChar("timeout", 0);
    themeIndex = preferences.getUChar("theme", 0);
    preferences.end();
    applyTheme(themeIndex);

    btSetup();
    startDecoderTask();

#if ENABLE_TOUCH
    touchSetup();
#endif

    currentState = STATE_MENU;
    drawCurrentStateUI();
}

void loop() {
    static unsigned long lastResumeSave = 0;
    static unsigned long lastProgressUpdate = 0;
    static unsigned long lastBtStatusCheck = 0;
    static uint8_t lastScanCount = 0;
    unsigned long now = millis();

    inputPoll();
#if ENABLE_TOUCH
    touchPoll();
#endif
    checkScreenTimeout();
    checkBluetoothScanTimeout();

    if (need_next_track) {
        need_next_track = false;
        switchTrack(1);
    }

    if (currentState == STATE_BT_SCAN && btScanCount != lastScanCount) {
        lastScanCount = btScanCount;
        if (!screenLocked) drawCurrentStateUI();
    }

    if (is_playing && now - lastResumeSave > 5000) {
        saveResumeState();
        lastResumeSave = now;
    }

    if (now - lastBtStatusCheck > 250) {
        bool currentStatus = a2dp_source.is_connected();
        if (currentStatus != bt_connected) {
            bt_connected = currentStatus;
            if (!currentStatus) bt_audio_started = false;
            bt_state_changed = true;
        }
        lastBtStatusCheck = now;
    }
    if (bt_state_changed) {
        bt_state_changed = false;
        updatePlaybackFromBluetooth();
        if (!screenLocked) drawCurrentStateUI();
    }
    if (play_requested && !is_playing && bt_connected &&
        now - play_request_started_at >= A2DP_PLAY_PRIME_DELAY_MS) {
        updatePlaybackFromBluetooth();
        updatePlayPauseButton();
    }

    if (!screenLocked && is_playing && currentState == STATE_PLAYER && now - lastProgressUpdate > 400) {
        updateProgressBar(false);
        lastProgressUpdate = now;
    }

    delay(10);
}
