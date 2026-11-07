// Desktop entry point: replaces final-form.ino's setup()/loop(). Renders the
// real screens.cpp UI to an SDL window; keyboard simulates encoder + 3 buttons
// (touch is compiled out, see nav.h: ENABLE_TOUCH is undefined in the sim).
#include <SDL2/SDL.h>
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

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("final-form sim", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    Adafruit_GFX::renderer = ren;
    simTextInit(ren);

    tft.begin();
    tft.setRotation(0);
    initColorTheme();
    applyTheme(0);

    loadOperatingMode();
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
                    case SDLK_UP: handleEncoderRotate(1); redraw = true; break;
                    case SDLK_DOWN: handleEncoderRotate(-1); redraw = true; break;
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
