// Stream32 player UI/feature-set, ported onto this board's real wiring
// (old/files/test_input.ino + old/files/plan.md). setup()/loop() glue only -- see
// pins.h for the wiring and nav.h for the control mapping.
//
// layout.h is landscape 320x240 (not test_input.ino's portrait reading) --
// rotation 1 is what puts this physical panel in that orientation.
//
// One OperatingMode (nav.h) runs per boot, chosen by selectOperatingMode()
// and persisted in Preferences; switching modes is a full ESP.restart(), so
// setup()/loop() only bring up the subsystems the active mode actually needs.
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
#include "bt_receiver.h"
#include "wifi_transfer.h"
#include "i2s_output.h"
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
    loadOperatingMode();  // before anything else: decides which subsystems below actually start

    tft.begin();
    tft.setRotation(1);
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
    loadPlaylistIndex();

    // Local Player / BT Transmitter both play from the SD library through the
    // same STATE_MENU/STATE_PLAYER UI -- resume state, likes and the decoder
    // task only matter for those two. BT Receiver plays whatever the phone
    // streams; Transfer File plays nothing.
    bool playsLocalLibrary = (operatingMode == MODE_LOCAL_PLAYER || operatingMode == MODE_BT_TRANSMITTER);
    if (playsLocalLibrary && total_tracks > 0) {
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

    switch (operatingMode) {
        case MODE_LOCAL_PLAYER: i2sOutputSetup(); startDecoderTask(); break;
        case MODE_BT_RECEIVER: i2sOutputSetup(); btReceiverSetup(); break;
        case MODE_BT_TRANSMITTER: btSetup(); startDecoderTask(); break;
        case MODE_TRANSFER_FILE: wifiTransferSetup(); break;
    }

#if ENABLE_TOUCH
    touchSetup();
#endif

    currentState = homeStateForMode();
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
    checkVolumeOverlayTimeout();

    if (operatingMode == MODE_TRANSFER_FILE) wifiTransferPoll();

    // Everything below is BT Transmitter-specific (A2DP source polling/scan)
    // or local-library playback (auto-advance/resume save) -- both only ever
    // reachable in MODE_LOCAL_PLAYER/MODE_BT_TRANSMITTER, since no other mode
    // can drive is_playing or STATE_BT_SCAN, but gate explicitly for clarity.
    if (operatingMode == MODE_BT_TRANSMITTER) {
        checkBluetoothScanTimeout();

        if (currentState == STATE_BT_SCAN && btScanCount != lastScanCount) {
            lastScanCount = btScanCount;
            if (!screenLocked) drawCurrentStateUI();
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
    }

    if (operatingMode == MODE_LOCAL_PLAYER || operatingMode == MODE_BT_TRANSMITTER) {
        if (need_next_track) {
            need_next_track = false;
            switchTrack(1);
        }
        if (is_playing && now - lastResumeSave > 5000) {
            saveResumeState();
            lastResumeSave = now;
        }
    }

    if (!screenLocked && is_playing && currentState == STATE_PLAYER && now - lastProgressUpdate > 400) {
        updateProgressBar(false);
        lastProgressUpdate = now;
    }

    delay(10);
}
