#include "screen_now_playing.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "cassette_anim.h"
#include "display_pins.h"
#include <stdio.h>

static lv_obj_t *s_title_lbl = NULL;
static lv_obj_t *s_subtitle_lbl = NULL;
static lv_obj_t *s_elapsed_lbl = NULL;
static lv_obj_t *s_total_lbl = NULL;
static lv_obj_t *s_bar = NULL;
static lv_obj_t *s_play_icon = NULL;

// Real, persistent playback state (survives screen rebuilds, not re-derived
// from a fixed demo string every time) -- no real audio backend exists in
// this standalone module, but prev/next/play-pause actually change this
// state and the screen reflects it, instead of being purely decorative.
static bool s_is_playing = true;
struct DemoTrack { const char *title; const char *subtitle; int duration_sec; };
#ifdef KODE_OPTIMASI_SIM
// Durations match the same song titles' durations shown in screen_library.cpp
// (Aurora 3:42, Blue Hour 4:05, City Lights 3:50, Dreamcatcher 5:12, Echoes 2:58).
static const DemoTrack kDemoTracks[] = {
    {"City Lights", "Favorit - 3 / 12", 230},
    {"Dreamcatcher", "Favorit - 4 / 12", 312},
    {"Echoes", "Favorit - 5 / 12", 178},
    {"Aurora", "Favorit - 1 / 12", 222},
    {"Blue Hour", "Favorit - 2 / 12", 245},
};
static int s_elapsed_sec = 92; // starts at 1:32, matching the mockup; resets to 0 on prev/next
#else
// No demo catalog in the real build -- a future pass wires this to whatever
// is actually loaded from the SD card. duration_sec=0 keeps the progress
// math safe (see refresh_progress_display/now_playing_tick_elapsed below).
static const DemoTrack kDemoTracks[] = {
    {"Tidak ada lagu", "", 0},
};
static int s_elapsed_sec = 0;
#endif
#define DEMO_TRACK_COUNT (int)(sizeof(kDemoTracks) / sizeof(kDemoTracks[0]))
static int s_track_idx = 0; // starts at "City Lights" in sim, the lone placeholder otherwise

static void format_mmss(int sec, char *buf, size_t buf_len) {
    snprintf(buf, buf_len, "%d:%02d", sec / 60, sec % 60);
}

// Pushes the current track's elapsed/total/percent to the widgets -- the
// single place that knows both s_elapsed_sec and this track's own duration,
// so the progress bar always matches the song actually selected instead of
// a fixed demo duration shared by every track.
static void refresh_progress_display(void) {
    int total = kDemoTracks[s_track_idx].duration_sec;
    char elapsed_buf[12], total_buf[12];
    format_mmss(s_elapsed_sec, elapsed_buf, sizeof(elapsed_buf));
    format_mmss(total, total_buf, sizeof(total_buf));
    uint8_t percent = (total > 0) ? (uint8_t)(100 * s_elapsed_sec / total) : 0;
    now_playing_set_progress(percent, elapsed_buf, total_buf);
}

static void on_screen_deleted(lv_event_t *e) {
    LV_UNUSED(e);
    s_title_lbl = s_subtitle_lbl = s_elapsed_lbl = s_total_lbl = s_bar = s_play_icon = NULL;
}

static void play_pause_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_is_playing = !s_is_playing;
    now_playing_set_playing(s_is_playing);
}

static void prev_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_track_idx = (s_track_idx - 1 + DEMO_TRACK_COUNT) % DEMO_TRACK_COUNT;
    s_elapsed_sec = 0; // new track starts from its own beginning, not the previous track's position
    now_playing_set_track_info(kDemoTracks[s_track_idx].title, kDemoTracks[s_track_idx].subtitle);
    refresh_progress_display();
}

static void next_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_track_idx = (s_track_idx + 1) % DEMO_TRACK_COUNT;
    s_elapsed_sec = 0;
    now_playing_set_track_info(kDemoTracks[s_track_idx].title, kDemoTracks[s_track_idx].subtitle);
    refresh_progress_display();
}

void screen_now_playing_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;

    ui_header_create(scr, NULL, "Local Player", true, 78);

    const lv_coord_t card_x = 12, card_y = 30, card_w = SCREEN_W - 24, card_h = 92;
    lv_obj_t *card = ui_card_create(scr, card_x, card_y, card_w, card_h, RADIUS_CARD, COLOR_CARD_BLUE);
    // card (unlike scr, the persistent screen_manager root) is actually
    // deleted on screen switch -- lv_obj_clean(scr) deletes children, not
    // scr itself -- so this is where the stale-pointer guard belongs.
    lv_obj_add_event_cb(card, on_screen_deleted, LV_EVENT_DELETE, NULL);
    cassette_anim_create(card, card_w, card_h);
    cassette_anim_set_spinning(s_is_playing); // sync reel to persisted play state on rebuild


    lv_obj_t *caption_bar = lv_obj_create(card);
    lv_obj_remove_style_all(caption_bar);
    lv_obj_set_size(caption_bar, card_w, 22);
    lv_obj_set_pos(caption_bar, 0, card_h - 22);
    lv_obj_set_style_bg_color(caption_bar, lv_color_hex(0x000C20), 0);
    lv_obj_set_style_bg_opa(caption_bar, LV_OPA_50, 0);
    lv_obj_set_style_pad_left(caption_bar, 8, 0);
    lv_obj_clear_flag(caption_bar, LV_OBJ_FLAG_SCROLLABLE);

    s_title_lbl = lv_label_create(caption_bar);
    lv_label_set_text(s_title_lbl, kDemoTracks[s_track_idx].title);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title_lbl, FONT_BODY, 0);
    lv_obj_align(s_title_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    s_subtitle_lbl = lv_label_create(scr);
    lv_label_set_text(s_subtitle_lbl, kDemoTracks[s_track_idx].subtitle);
    lv_obj_set_style_text_color(s_subtitle_lbl, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(s_subtitle_lbl, FONT_SMALL, 0);
    lv_obj_set_pos(s_subtitle_lbl, card_x, card_y + card_h + 2);

    s_bar = lv_bar_create(scr);
    lv_obj_set_size(s_bar, SCREEN_W - 32, 4);
    lv_obj_set_pos(s_bar, 16, 140);
    lv_obj_set_style_bg_color(s_bar, COLOR_TRACK, 0);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_bar, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 40, LV_ANIM_OFF);

    lv_obj_t *time_row = lv_obj_create(scr);
    lv_obj_remove_style_all(time_row);
    lv_obj_set_size(time_row, SCREEN_W - 32, 10);
    lv_obj_set_pos(time_row, 16, 146);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(time_row, LV_OBJ_FLAG_SCROLLABLE);

    s_elapsed_lbl = lv_label_create(time_row);
    lv_label_set_text(s_elapsed_lbl, "1:32");
    lv_obj_set_style_text_color(s_elapsed_lbl, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(s_elapsed_lbl, FONT_SMALL, 0);

    s_total_lbl = lv_label_create(time_row);
    lv_label_set_text(s_total_lbl, "3:50");
    lv_obj_set_style_text_color(s_total_lbl, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(s_total_lbl, FONT_SMALL, 0);

    lv_obj_t *prev_btn = ui_button_create(scr, 88, 166, 36, 36, RADIUS_BUTTON_CIRCLE, COLOR_SURFACE_BUTTON);
    lv_obj_center(icon_symbol(prev_btn, LV_SYMBOL_PREV, COLOR_TEXT_PRIMARY, FONT_BODY));
    lv_obj_add_event_cb(prev_btn, prev_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *play_btn = ui_button_create(scr, 136, 160, 48, 48, RADIUS_BUTTON_LG, COLOR_ACCENT);
    s_play_icon = icon_symbol(play_btn, s_is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY, COLOR_ACCENT_TEXT, FONT_BODY);
    lv_obj_center(s_play_icon);
    lv_obj_add_event_cb(play_btn, play_pause_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next_btn = ui_button_create(scr, 196, 166, 36, 36, RADIUS_BUTTON_CIRCLE, COLOR_SURFACE_BUTTON);
    lv_obj_center(icon_symbol(next_btn, LV_SYMBOL_NEXT, COLOR_TEXT_PRIMARY, FONT_BODY));
    lv_obj_add_event_cb(next_btn, next_clicked_cb, LV_EVENT_CLICKED, NULL);

    ui_footer_create(scr, "PREV/NEXT pilih lagu - Klik putar/pause");

    refresh_progress_display(); // sync bar/labels to the persisted elapsed/track state instead of placeholder text
}

void now_playing_set_track_info(const char *title, const char *subtitle) {
    if (s_title_lbl) lv_label_set_text(s_title_lbl, title);
    if (s_subtitle_lbl) lv_label_set_text(s_subtitle_lbl, subtitle);
}

void now_playing_set_progress(uint8_t percent, const char *elapsed, const char *total) {
    if (s_bar) lv_bar_set_value(s_bar, percent, LV_ANIM_OFF);
    if (s_elapsed_lbl) lv_label_set_text(s_elapsed_lbl, elapsed);
    if (s_total_lbl) lv_label_set_text(s_total_lbl, total);
}

void now_playing_set_playing(bool is_playing) {
    if (s_play_icon) lv_label_set_text(s_play_icon, is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    cassette_anim_set_spinning(is_playing);
}

void now_playing_set_battery(int percent) {
    LV_UNUSED(percent); // ponytail: header battery is demo-static for now; wire a stored handle if live battery needed later
}

bool now_playing_is_playing(void) {
    return s_is_playing;
}

void now_playing_tick_elapsed(void) {
    // Caller (demo_cycle.cpp) already guards "screen active + actually playing";
    // this only owns the per-track duration math so elapsed/total always
    // match whichever track prev/next last selected, not a fixed duration
    // shared by every song.
    int duration = kDemoTracks[s_track_idx].duration_sec;
    if (duration <= 0) return; // no real track loaded (real build, no SD catalog yet) -- nothing to tick
    s_elapsed_sec = (s_elapsed_sec + 1) % duration;
    refresh_progress_display();
}
