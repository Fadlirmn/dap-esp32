// L5 "BT Transmitter" — plays a local source out to a BT speaker, so unlike
// Receiver it has its own seek bar + transport (it owns the playback clock).
#include "screen_transmitter.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"

static void badge_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_BT_DEVICES_TX); // "ganti perangkat" -- back to the device list
}

// Real, persistent state (not purely decorative) -- no real local audio
// source exists in this standalone module, but play/pause and prev/next
// actually change what's displayed, and that state survives screen rebuilds.
static bool s_is_playing = true;
struct DemoTrack { const char *title; int pos; };
#ifdef KODE_OPTIMASI_SIM
static const DemoTrack kDemoTracks[] = {
    {"Aurora", 5},
    {"Blue Hour", 6},
    {"City Lights", 7},
};
#else
// No real local-source track info in the real build -- a future pass wires
// this to the actual playlist/transport state.
static const DemoTrack kDemoTracks[] = {
    {"Tidak ada lagu", 0},
};
#endif
#define DEMO_TRACK_COUNT (int)(sizeof(kDemoTracks) / sizeof(kDemoTracks[0]))
static int s_track_idx = 0;
static lv_obj_t *s_title_lbl = NULL;
static lv_obj_t *s_pos_lbl = NULL;
static lv_obj_t *s_play_icon = NULL;

static void refresh_track_labels(void) {
    if (s_title_lbl) lv_label_set_text(s_title_lbl, kDemoTracks[s_track_idx].title);
    if (s_pos_lbl) lv_label_set_text_fmt(s_pos_lbl, "%d / 18", kDemoTracks[s_track_idx].pos);
}

static void play_pause_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_is_playing = !s_is_playing;
    if (s_play_icon) lv_label_set_text(s_play_icon, s_is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

static void prev_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_track_idx = (s_track_idx - 1 + DEMO_TRACK_COUNT) % DEMO_TRACK_COUNT;
    refresh_track_labels();
}

static void next_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_track_idx = (s_track_idx + 1) % DEMO_TRACK_COUNT;
    refresh_track_labels();
}

static void on_screen_deleted(lv_event_t *e) {
    LV_UNUSED(e);
    s_title_lbl = s_pos_lbl = s_play_icon = NULL;
}

void screen_transmitter_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_BLUETOOTH, "BT Transmitter", true, 78);

    const lv_coord_t card_w = SCREEN_W - 24, card_h = 92;
    lv_obj_t *card = ui_card_create(scr, 12, 30, card_w, card_h, 20, COLOR_BT_HERO);

    lv_obj_t *chain = lv_obj_create(card);
    lv_obj_remove_style_all(chain);
    lv_obj_set_size(chain, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(chain, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(chain, 10, 0);
    lv_obj_align(chain, LV_ALIGN_TOP_MID, 0, 8);
    icon_symbol(chain, LV_SYMBOL_PLAY, COLOR_BT_HERO_ICON, FONT_BODY);
    icon_symbol(chain, LV_SYMBOL_NEXT, COLOR_BT_HERO_ICON, FONT_BODY);
    icon_symbol(chain, LV_SYMBOL_WIFI, COLOR_BT_HERO_ICON, FONT_TITLE);

    lv_obj_t *badge = icon_connected_badge(card);
    lv_obj_set_pos(badge, card_w - 28, 6);
    lv_obj_add_flag(badge, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(badge, badge_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(card, on_screen_deleted, LV_EVENT_DELETE, NULL);

    lv_obj_t *caption = lv_obj_create(card);
    lv_obj_remove_style_all(caption);
    lv_obj_set_size(caption, card_w, 22);
    lv_obj_set_pos(caption, 0, card_h - 22);
    lv_obj_set_style_bg_color(caption, lv_color_hex(0x000C20), 0);
    lv_obj_set_style_bg_opa(caption, LV_OPA_50, 0);
    lv_obj_set_style_pad_left(caption, 8, 0);
    lv_obj_clear_flag(caption, LV_OBJ_FLAG_SCROLLABLE);

    s_title_lbl = lv_label_create(caption);
    lv_label_set_text(s_title_lbl, kDemoTracks[s_track_idx].title);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title_lbl, FONT_BODY, 0);
    lv_obj_align(s_title_lbl, LV_ALIGN_LEFT_MID, 0, -6);

    lv_obj_t *pos_row = lv_obj_create(caption);
    lv_obj_remove_style_all(pos_row);
    lv_obj_set_size(pos_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(pos_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(pos_row, 4, 0);
    lv_obj_align(pos_row, LV_ALIGN_LEFT_MID, 0, 6);
    icon_symbol(pos_row, LV_SYMBOL_LIST, COLOR_TEXT_MUTED, FONT_SMALL);
    s_pos_lbl = lv_label_create(pos_row);
    lv_label_set_text_fmt(s_pos_lbl, "%d / 18", kDemoTracks[s_track_idx].pos);
    lv_obj_set_style_text_color(s_pos_lbl, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(s_pos_lbl, FONT_SMALL, 0);

    lv_obj_t *bar = lv_bar_create(scr);
    lv_obj_set_size(bar, SCREEN_W - 32, 4);
    lv_obj_set_pos(bar, 16, 128);
    lv_obj_set_style_bg_color(bar, COLOR_TRACK, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, 100);
#ifdef KODE_OPTIMASI_SIM
    lv_bar_set_value(bar, 22, LV_ANIM_OFF);
#else
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
#endif

    lv_obj_t *time_row = lv_obj_create(scr);
    lv_obj_remove_style_all(time_row);
    lv_obj_set_size(time_row, SCREEN_W - 32, 10);
    lv_obj_set_pos(time_row, 16, 134);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(time_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *elapsed = lv_label_create(time_row);
    lv_obj_t *total = lv_label_create(time_row);
#ifdef KODE_OPTIMASI_SIM
    lv_label_set_text(elapsed, "0:48");
    lv_label_set_text(total, "3:42");
#else
    lv_label_set_text(elapsed, "0:00");
    lv_label_set_text(total, "0:00");
#endif
    lv_obj_set_style_text_color(elapsed, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(elapsed, FONT_SMALL, 0);
    lv_obj_set_style_text_color(total, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(total, FONT_SMALL, 0);

    lv_obj_t *prev_btn = ui_button_create(scr, 88, 148, 36, 36, RADIUS_BUTTON_CIRCLE, COLOR_SURFACE_BUTTON);
    lv_obj_center(icon_symbol(prev_btn, LV_SYMBOL_PREV, COLOR_TEXT_PRIMARY, FONT_BODY));
    lv_obj_add_event_cb(prev_btn, prev_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *play_btn = ui_button_create(scr, 136, 142, 48, 48, RADIUS_BUTTON_LG, COLOR_ACCENT);
    s_play_icon = icon_symbol(play_btn, s_is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY, COLOR_ACCENT_TEXT, FONT_BODY);
    lv_obj_center(s_play_icon);
    lv_obj_add_event_cb(play_btn, play_pause_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next_btn = ui_button_create(scr, 196, 148, 36, 36, RADIUS_BUTTON_CIRCLE, COLOR_SURFACE_BUTTON);
    lv_obj_center(icon_symbol(next_btn, LV_SYMBOL_NEXT, COLOR_TEXT_PRIMARY, FONT_BODY));
    lv_obj_add_event_cb(next_btn, next_clicked_cb, LV_EVENT_CLICKED, NULL);

    ui_footer_create(scr, "Memutar lokal - keluar via speaker BT");
}
