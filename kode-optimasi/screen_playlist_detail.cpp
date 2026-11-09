// L12 "Isi playlist" ("Favorit") — split layout: action column + track list.
#include "screen_playlist_detail.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"
#include <stdint.h>

static void back_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_PLAYLISTS);
}

static void putar_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_NOW_PLAYING); // "play this playlist" -> surfaces the player, a real action
}

struct Track { const char *title; const char *duration; };
#ifdef KODE_OPTIMASI_SIM
static const Track kTracks[] = {
    {"City Lights", "3:50"},
    {"Echoes", "2:58"},
    {"Aurora", "3:42"},
    {"Golden Road", "4:20"},
    {"Blue Hour", "4:05"},
};
#define TRACK_COUNT (int)(sizeof(kTracks) / sizeof(kTracks[0]))
#else
// No demo playlist contents in the real build -- the track loop below
// naturally renders nothing when TRACK_COUNT is 0; a future pass wires this
// to the real SD-card playlist contents.
static const Track *kTracks = NULL;
#define TRACK_COUNT 0
#endif
static int s_now_playing_idx = 0; // "City Lights" -- persists across rebuilds, moves on real row clicks

// Acak/Ulang have no real shuffle/repeat engine behind them (no audio
// backend exists in this module), so they stay plain outline buttons with
// no sticky "active" state -- faking a persistent mode toggle with nothing
// real behind it would itself be a dummy. They still get real press
// feedback (highlight only while held down) automatically from
// ui_button_create (see ui_common.cpp) -- that's the only "selected" state
// a click should produce here.

static void row_clicked_cb(lv_event_t *e) {
    s_now_playing_idx = (int)(intptr_t)lv_event_get_user_data(e);
    screen_manager_show(SCREEN_PLAYLIST_DETAIL);
}

static lv_obj_t *action_button(lv_obj_t *scr, lv_coord_t y, const char *symbol, const char *label, bool filled,
                                lv_obj_t **out_icon, lv_obj_t **out_label) {
    lv_obj_t *btn = ui_button_create(scr, 12, y, 92, 36, RADIUS_BUTTON_LG,
                                      filled ? COLOR_ACCENT : COLOR_BG);
    if (!filled) {
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, COLOR_SCROLLBAR, 0);
    }
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn, 6, 0);
    lv_color_t fg = filled ? COLOR_ACCENT_TEXT : COLOR_TEXT_PRIMARY;
    lv_obj_t *icon = icon_symbol(btn, symbol, fg, FONT_SMALL);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, fg, 0);
    lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
    if (out_icon) *out_icon = icon;
    if (out_label) *out_label = lbl;
    return btn;
}

// Right-column track row, narrower than the full-width ui_list_row_create.
static lv_obj_t *track_row(lv_obj_t *scr, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, bool selected) {
    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, w, h);
    lv_obj_set_pos(row, x, y);
    lv_obj_set_style_radius(row, selected ? RADIUS_ROW : 0, 0);
    lv_obj_set_style_bg_color(row, COLOR_SELECTED, 0);
    lv_obj_set_style_bg_opa(row, selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_80, LV_STATE_PRESSED); // press feedback, see ui_common.cpp's ui_list_row_create
    lv_obj_set_style_pad_hor(row, 8, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

void screen_playlist_detail_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    lv_obj_t *header = ui_header_create(scr, LV_SYMBOL_LEFT, "Favorit", true, 78);
    lv_obj_t *back_area = lv_obj_get_child(header, 0); // the icon+title container (see ui_header_create)
    lv_obj_add_flag(back_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back_area, back_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(screen_manager_group(), back_area);
    lv_obj_set_style_bg_color(back_area, COLOR_SELECTED, 0);
    lv_obj_set_style_bg_opa(back_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(back_area, LV_OPA_60, LV_STATE_PRESSED); // press feedback, see ui_button_create

    lv_obj_t *putar_btn = action_button(scr, 36, LV_SYMBOL_PLAY, "Putar", true, NULL, NULL);
    lv_obj_add_event_cb(putar_btn, putar_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *meta = lv_label_create(scr);
#ifdef KODE_OPTIMASI_SIM
    lv_label_set_text(meta, "12 lagu\n48 mnt");
#else
    lv_label_set_text(meta, "0 lagu\n0 mnt");
#endif
    lv_obj_set_style_text_color(meta, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(meta, FONT_SMALL, 0);
    lv_obj_set_pos(meta, 12, 84); // moved up now that Acak/Ulang are gone

    const lv_coord_t list_x = 104, list_w = SCREEN_W - list_x - 12;
    const lv_coord_t row_h = 32, row_gap = 4, first_y = 30;
    if (TRACK_COUNT == 0) {
        lv_obj_t *empty = lv_label_create(scr);
        lv_label_set_text(empty, "Playlist kosong");
        lv_obj_set_style_text_color(empty, COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_font(empty, FONT_SMALL, 0);
        lv_obj_set_pos(empty, list_x, first_y);
    }
    for (int i = 0; i < TRACK_COUNT; i++) {
        bool selected = (i == s_now_playing_idx);
        lv_obj_t *row = track_row(scr, list_x, first_y + i * (row_h + row_gap), list_w, row_h, selected);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, row_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_group_add_obj(screen_manager_group(), row);
        if (selected) {
            icon_equalizer_bars(row, COLOR_ACCENT);
        } else {
            lv_obj_t *idx = lv_label_create(row);
            lv_label_set_text_fmt(idx, "%02d", i + 1);
            lv_obj_set_style_text_color(idx, COLOR_TEXT_MUTED, 0);
            lv_obj_set_style_text_font(idx, FONT_SMALL, 0);
            lv_obj_set_width(idx, 18);
        }
        ui_row_text_create(row, kTracks[i].title, kTracks[i].duration);
    }

    ui_footer_create(scr, "Klik putar - MENU kembali");
}
