// L4 "BT Receiver" — no seek bar in this mockup (receiver plays what the
// phone streams; the ESP32 side has no transport position of its own).
#include "screen_receiver.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"

static void badge_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_BT_DEVICES_RX); // "ganti perangkat" -- back to the device list
}

// Real, persistent state (not purely decorative) -- no real BT audio stream
// exists in this standalone module, but play/pause and prev/next actually
// change what's displayed, and that state survives screen rebuilds.
static bool s_is_playing = true;
struct DemoTrack { const char *title; const char *artist; };
#ifdef KODE_OPTIMASI_SIM
static const DemoTrack kDemoTracks[] = {
    {"City Lights", "Neon Harbor"},
    {"Aurora", "Night Drive"},
    {"Echoes", "Velvet Skyline"},
};
#else
// No real BT stream info in the real build -- a future pass wires this to
// the actual A2DP metadata.
static const DemoTrack kDemoTracks[] = {
    {"Tidak ada lagu", ""},
};
#endif
#define DEMO_TRACK_COUNT (int)(sizeof(kDemoTracks) / sizeof(kDemoTracks[0]))
static int s_track_idx = 0;
static lv_obj_t *s_title_lbl = NULL;
static lv_obj_t *s_artist_lbl = NULL;
static lv_obj_t *s_play_icon = NULL;

static void refresh_track_labels(void) {
    if (s_title_lbl) lv_label_set_text(s_title_lbl, kDemoTracks[s_track_idx].title);
    if (s_artist_lbl) lv_label_set_text(s_artist_lbl, kDemoTracks[s_track_idx].artist);
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
    s_title_lbl = s_artist_lbl = s_play_icon = NULL;
}

void screen_receiver_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_BLUETOOTH, "BT Receiver", true, 78);

    const lv_coord_t card_w = SCREEN_W - 24, card_h = 110;
    lv_obj_t *card = ui_card_create(scr, 12, 30, card_w, card_h, 20, COLOR_BT_HERO);
    lv_obj_t *bt_icon = icon_symbol(card, LV_SYMBOL_BLUETOOTH, COLOR_BT_HERO_ICON, FONT_TITLE);
    lv_obj_align(bt_icon, LV_ALIGN_TOP_MID, 0, 16);

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

    s_artist_lbl = lv_label_create(caption);
    lv_label_set_text(s_artist_lbl, kDemoTracks[s_track_idx].artist);
    lv_obj_set_style_text_color(s_artist_lbl, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(s_artist_lbl, FONT_SMALL, 0);
    lv_obj_align(s_artist_lbl, LV_ALIGN_LEFT_MID, 0, 6);

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

    ui_footer_create(scr, "Audio dari ponsel via Bluetooth");
}
