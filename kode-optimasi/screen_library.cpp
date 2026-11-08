// L2 "Daftar Lagu" (song list). Demo data only — a future pass wires this to
// the real SD-card playlist.
#include "screen_library.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"

struct Song { const char *title; const char *duration; };
#ifdef KODE_OPTIMASI_SIM
static const Song kSongs[] = {
    {"Aurora", "3:42"},
    {"Blue Hour", "4:05"},
    {"City Lights", "3:50"},
    {"Dreamcatcher", "5:12"},
    {"Echoes", "2:58"},
    {"Golden Road", "4:20"},
    {"Night Drive", "3:15"},
    {"Velvet Skyline", "4:48"},
    {"Paper Moon", "3:33"},
    {"Neon Harbor", "4:02"},
};
#define SONG_COUNT (int)(sizeof(kSongs) / sizeof(kSongs[0]))
#else
// No demo catalog in the real build -- empty-state handled below, a future
// pass wires this to the real SD-card playlist.
static const Song *kSongs = NULL;
#define SONG_COUNT 0
#endif
#define PAGE_SIZE 5
#define PAGE_COUNT ((SONG_COUNT + PAGE_SIZE - 1) / PAGE_SIZE)
// Paginated, not a continuous scroll list: always exactly <=PAGE_SIZE rows
// exist at a time, so memory use per screen build never grows with the
// catalog size (a real scrollable list of N songs allocated N rows' worth of
// widgets at once -- confirmed via gdb that this overflowed the LVGL memory
// pool at 16 songs; pagination sidesteps that ceiling entirely).
static int s_page = 0;

// No "now playing" row highlight by design (none of these rows are
// selected/highlighted) -- a row click is just "play this song", a real
// action (-> Now Playing), not a local state change with nothing to show.
static void row_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_NOW_PLAYING);
}

static void prev_page_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_page = (s_page - 1 + PAGE_COUNT) % PAGE_COUNT;
    screen_manager_show(SCREEN_LIBRARY);
}

static void next_page_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_page = (s_page + 1) % PAGE_COUNT;
    screen_manager_show(SCREEN_LIBRARY);
}

void screen_library_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_LIST, "Daftar Lagu", true, 78);

    if (SONG_COUNT == 0) {
        // No catalog loaded (real build) -- PAGE_COUNT would be 0 here, so
        // skip straight to an empty state instead of the paginated rows
        // below (which assume at least one page exists).
        lv_obj_t *empty = lv_label_create(scr);
        lv_label_set_text(empty, "Tidak ada lagu di SD card");
        lv_obj_set_style_text_color(empty, COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_font(empty, FONT_BODY, 0);
        lv_obj_center(empty);
        ui_footer_create(scr, "MENU: kembali");
        return;
    }

    const lv_coord_t row_h = 32, row_gap = 4, first_y = 30;
    const int page_start = s_page * PAGE_SIZE;
    const int page_rows = (SONG_COUNT - page_start < PAGE_SIZE) ? (SONG_COUNT - page_start) : PAGE_SIZE;
    for (int row = 0; row < page_rows; row++) {
        int i = page_start + row;
        lv_obj_t *r = ui_list_row_create(scr, first_y + row * (row_h + row_gap), row_h, false);
        lv_obj_add_flag(r, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(r, row_clicked_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *idx = lv_label_create(r);
        lv_label_set_text_fmt(idx, "%02d", i + 1);
        lv_obj_set_style_text_color(idx, COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_font(idx, FONT_SMALL, 0);
        lv_obj_set_width(idx, 22);
        ui_row_text_create(r, kSongs[i].title, kSongs[i].duration);
    }

    lv_obj_t *footer = ui_footer_create(scr, "");
    lv_obj_t *footer_lbl = lv_obj_get_child(footer, 0);
    lv_label_set_text_fmt(footer_lbl, "Hal %d/%d - Klik putar", s_page + 1, PAGE_COUNT);

    lv_obj_t *prev_page = icon_symbol(footer, LV_SYMBOL_LEFT, COLOR_TEXT_MUTED, FONT_SMALL);
    lv_obj_align(prev_page, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_flag(prev_page, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(prev_page, prev_page_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next_page = icon_symbol(footer, LV_SYMBOL_RIGHT, COLOR_TEXT_MUTED, FONT_SMALL);
    lv_obj_align(next_page, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_flag(next_page, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(next_page, next_page_cb, LV_EVENT_CLICKED, NULL);
}
