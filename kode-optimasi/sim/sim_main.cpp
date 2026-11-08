// Desktop preview: real lvgl + the real screen_*/ui_*/overlay_* sources
// (unmodified, same pattern as final-form/sim/sim_main.cpp) rendered to an
// SDL window instead of a TFT_eSPI panel. No FreeRTOS, no hardware pins —
// everything screen-side is already hardware-agnostic (only kode-optimasi.ino
// itself touches TFT_eSPI/tasks, and this file replaces exactly that part).
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "display_pins.h"
#include "screen_manager.h"
#include "demo_cycle.h"
#include "overlay_volume.h"

#define ZOOM 2

static lv_color_t s_buf[SCREEN_W * SCREEN_H]; // full buffer: fine on desktop, no RAM budget here
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;
static SDL_Texture *s_texture;

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    SDL_Rect rect = {area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1};
    SDL_UpdateTexture(s_texture, &rect, color_p, rect.w * (int)sizeof(lv_color_t));
    lv_disp_flush_ready(drv);
}

// Mouse as an LVGL pointer indev -- without this nothing in the window is
// clickable (this was missing entirely at first, which is why "menu tidak
// bisa dinavigasi": there was no input device feeding LVGL any clicks).
static void mouse_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    LV_UNUSED(drv);
    int x, y;
    Uint32 buttons = SDL_GetMouseState(&x, &y);
    data->point.x = x / ZOOM;
    data->point.y = y / ZOOM;
    data->state = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("kode-optimasi sim", SDL_WINDOWPOS_CENTERED,
                                           SDL_WINDOWPOS_CENTERED, SCREEN_W * ZOOM, SCREEN_H * ZOOM,
                                           SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    s_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                   SCREEN_W, SCREEN_H);

    lv_init();
    lv_disp_draw_buf_init(&s_draw_buf, s_buf, NULL, SCREEN_W * SCREEN_H);
    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = SCREEN_W;
    s_disp_drv.ver_res = SCREEN_H;
    s_disp_drv.flush_cb = disp_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mouse_read_cb;
    lv_indev_drv_register(&indev_drv);

    screen_manager_init(SCREEN_NOW_PLAYING);
    demo_cycle_init();

    Uint32 last = SDL_GetTicks();
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            // Real volume control (mouse wheel) -- not a timer-driven demo
            // number: scrolling up/down actually raises/lowers a persistent
            // volume level (overlay_volume.cpp), same as real volume buttons
            // would on hardware.
            if (e.type == SDL_MOUSEWHEEL) overlay_volume_adjust(e.wheel.y * 5);
        }
        Uint32 now = SDL_GetTicks();
        lv_tick_inc(now - last);
        last = now;
        lv_timer_handler();

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, s_texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps desktop preview (device itself runs lv_timer_handler at 66ms/15fps)
    }

    SDL_DestroyTexture(s_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
