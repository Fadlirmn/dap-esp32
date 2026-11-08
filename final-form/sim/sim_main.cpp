// Desktop entry point: replaces final-form.ino's setup()/loop(). Renders the
// real screens.cpp UI to an SDL window; keyboard simulates encoder + 3 buttons
// (touch is compiled out, see nav.h: ENABLE_TOUCH is undefined in the sim).
#include <SDL2/SDL.h>
#include <cstring>
#include <cstdlib>
#include "sim_text.h"
#include <Adafruit_ILI9341.h>
#include <Preferences.h>
#include <SD.h>
#include "theme.h"
#include "screens.h"
#include "nav.h"
#include "playlist.h"
#include "bluetooth.h"
#include "bt_receiver.h"
#include "wifi_transfer.h"
#include "i2s_output.h"
#include "audio_decoder.h"
#include "layout.h"

Adafruit_ILI9341 tft(0, 0, 0);
Preferences preferences;

// Headless screenshot helper: dumps the renderer's current pixels as a PPM
// (trivial to write by hand, no SDL_image dependency) -- used by the
// SIM_SCRIPT env var below, not part of normal interactive use.
static void saveScreenshot(SDL_Renderer *ren, const char *path) {
    SDL_Surface *surf = SDL_CreateRGBSurface(0, SCREEN_W, SCREEN_H, 24,
                                              0, 0, 0, 0);
    if (SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_RGB24, surf->pixels, surf->pitch) != 0) {
        fprintf(stderr, "SDL_RenderReadPixels failed: %s\n", SDL_GetError());
    }
    FILE *f = fopen(path, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
        for (int y = 0; y < SCREEN_H; y++) {
            fwrite((uint8_t *)surf->pixels + y * surf->pitch, 1, SCREEN_W * 3, f);
        }
        fclose(f);
    }
    SDL_FreeSurface(surf);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("final-form sim", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, 0);
    // SIM_SCRIPT (headless capture mode, see below) uses a software renderer:
    // SDL_RenderReadPixels() against an accelerated renderer lagged by a
    // couple of frames in this environment (likely a driver/compositor
    // buffering quirk), making screenshots read back stale or black. The
    // software renderer draws straight into a CPU surface, no such lag.
    bool headless = getenv("SIM_SCRIPT") != nullptr;
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, headless ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    Adafruit_GFX::renderer = ren;
    simTextInit(ren);

    tft.begin();
    tft.setRotation(0);
    initColorTheme();
    applyTheme(1);  // theme 1 (Blue) is the default matching dap-ui-landscape's #A8C8FF accent

    loadOperatingMode();
    // SIM_MODE=0..3 overrides the boot mode for testing/demo (sim's stub
    // Preferences always returns the default, so there's otherwise no way
    // to boot into anything but MODE_LOCAL_PLAYER here).
    if (const char *m = getenv("SIM_MODE")) operatingMode = (OperatingMode)atoi(m);
    audioDecoderSetup();
    SD.begin(0);
    loadPlaylist();
    loadPlaylistIndex();
    if (total_tracks > 0) openCurrentTrack();

    switch (operatingMode) {
        case MODE_BT_TRANSMITTER: btSetup(); break;
        case MODE_LOCAL_PLAYER: i2sOutputSetup(); break;
        case MODE_BT_RECEIVER: i2sOutputSetup(); btReceiverSetup(); break;
        case MODE_TRANSFER_FILE: wifiTransferSetup(); break;
    }

    currentState = homeStateForMode();
    drawCurrentStateUI();
    SDL_RenderPresent(ren);

    // Headless capture mode: SIM_SCRIPT="step,step,..." runs each step (same
    // names as the interactive keys below, lowercase) then screenshots into
    // SIM_SHOTS_DIR (default /tmp), then exits -- lets this be driven without
    // a visible desktop. Not part of normal interactive use.
    if (const char *script = getenv("SIM_SCRIPT")) {
        const char *shotsDir = getenv("SIM_SHOTS_DIR");
        if (!shotsDir) shotsDir = "/tmp";
        char path[512];
        snprintf(path, sizeof(path), "%s/00-initial.ppm", shotsDir);
        saveScreenshot(ren, path);

        int step = 1;
        char buf[2048];
        strncpy(buf, script, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = 0;
        for (char *tok = strtok(buf, ","); tok; tok = strtok(nullptr, ",")) {
            if (!strcmp(tok, "left")) handlePrevButton();
            else if (!strcmp(tok, "right")) handleNextButton();
            else if (!strcmp(tok, "enter")) handleEncoderClick();
            else if (!strcmp(tok, "space")) handleEncoderDoubleClick();
            else if (!strcmp(tok, "l")) handleEncoderLongPress();
            else if (!strcmp(tok, "up")) handleEncoderRotate(1);
            else if (!strcmp(tok, "down")) handleEncoderRotate(-1);
            else if (!strcmp(tok, "m")) handleMenuButton();
            else if (!strcmp(tok, "n")) handleMenuLongPress();
            else if (!strcmp(tok, "b")) { bt_connected = !bt_connected; bt_state_changed = true; updatePlaybackFromBluetooth(); }
            else { fprintf(stderr, "SIM_SCRIPT: unknown step '%s'\n", tok); continue; }

            // Rotate already paints its own volume overlay (nav.cpp); a full
            // drawCurrentStateUI() here would immediately erase it again,
            // same as the real firmware's loop() never force-redraws after
            // a rotate either -- only checkVolumeOverlayTimeout() does, once
            // it actually expires.
            bool isRotate = !strcmp(tok, "up") || !strcmp(tok, "down");
            if (!isRotate) drawCurrentStateUI();
            SDL_RenderPresent(ren);
            snprintf(path, sizeof(path), "%s/%02d-%s.ppm", shotsDir, step++, tok);
            saveScreenshot(ren, path);
        }
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 0;
    }

    bool running = true;
    while (running) {
        bool redraw = false;
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q: running = false; break;
                    case SDLK_LEFT: handlePrevButton(); redraw = true; break;
                    case SDLK_RIGHT: handleNextButton(); redraw = true; break;
                    case SDLK_RETURN: handleEncoderClick(); redraw = true; break;
                    case SDLK_SPACE: handleEncoderDoubleClick(); redraw = true; break;
                    case SDLK_l: handleEncoderLongPress(); redraw = true; break;
                    // Rotate already paints its own volume overlay
                    // (nav.cpp) -- present that directly instead of setting
                    // `redraw` (which would trigger a full drawCurrentStateUI
                    // below and immediately erase it), matching the real
                    // firmware's loop(), which never force-redraws after a
                    // rotate either.
                    case SDLK_UP: handleEncoderRotate(1); SDL_RenderPresent(ren); break;
                    case SDLK_DOWN: handleEncoderRotate(-1); SDL_RenderPresent(ren); break;
                    case SDLK_m: handleMenuButton(); redraw = true; break;
                    case SDLK_n: handleMenuLongPress(); redraw = true; break;  // open mode switcher (MENU long-press)
                    case SDLK_b:
                        bt_connected = !bt_connected;
                        bt_state_changed = true;
                        break;
                    default: break;
                }
            }
        }

        checkScreenTimeout();
        checkVolumeOverlayTimeout();
        checkLowBattery();
        checkSDHealth();
        checkBluetoothScanTimeout();
        if (operatingMode == MODE_TRANSFER_FILE) wifiTransferPoll();
        if (bt_state_changed) {
            updatePlaybackFromBluetooth();
            redraw = true;
        }
        if (redraw) {
            drawCurrentStateUI();
            SDL_RenderPresent(ren);
        }
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
