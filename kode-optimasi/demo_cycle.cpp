#include "demo_cycle.h"
#include "lvgl.h"
#include "screen_manager.h"
#include "screen_now_playing.h"
#include "overlay_alert.h"

// Screen navigation is now real (click the "MENU" badge / a Menu tile / a
// Playlists row -- see ui_common.cpp, screen_menu.cpp, screen_playlists.cpp),
// so this no longer force-cycles screens -- that fought manual clicks and
// made the UI feel unresponsive. Volume is also real now (scroll wheel in
// sim/sim_main.cpp -> overlay_volume_adjust()), so it's no longer shown here
// on a timer with a fixed dummy number either. Only the SD-error toast stays
// as a cosmetic, non-blocking auto-demo (it auto-hides itself in a few
// seconds regardless -- see overlay_alert.cpp). The low-battery modal is
// intentionally left out of the auto-cycle since its backdrop deliberately
// blocks clicks (that's correct modal behavior -- just not something to pop
// up unprompted while someone is trying to click around).
static void demo_cycle_overlays_cb(lv_timer_t *t) {
    LV_UNUSED(t);
    overlay_alert_show_sd_error();
}

// Elapsed-time tick for Now Playing, independent of the cassette lv_anim_t
// (see cassette_anim.cpp) — only touches the time label/progress bar. The
// actual elapsed/duration math lives in screen_now_playing.cpp since it's
// the one that knows which track is selected and how long THAT track runs;
// this just decides *whether* to tick (right screen, actually playing).
static void demo_elapsed_tick_cb(lv_timer_t *t) {
    LV_UNUSED(t);
    if (screen_manager_current() != SCREEN_NOW_PLAYING) return;
    if (!now_playing_is_playing()) return; // paused -> the clock stops, like a real player
    now_playing_tick_elapsed();
}

void demo_cycle_init(void) {
    lv_timer_create(demo_cycle_overlays_cb, 12000, NULL);
    lv_timer_create(demo_elapsed_tick_cb, 1000, NULL);
}
