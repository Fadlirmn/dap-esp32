// L11 "Daftar playlist" — categories are real/fixed; song-count meta is demo.
#include "screen_playlists.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"

static void row_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_PLAYLIST_DETAIL);
}

struct Playlist { const char *name; const char *meta; lv_color_t chip; };
// The categories themselves (names + chip colors) are real/fixed, kept
// identical in both builds. Only the song-count/duration meta text is demo
// content -- zeroed in the real build until wired to the actual SD catalog.
#ifdef KODE_OPTIMASI_SIM
static const Playlist kPlaylists[] = {
    {"Semua lagu", "42 lagu - 2 j 48 mnt", COLOR_CHIP_NEUTRAL},
    {"Favorit", "12 lagu - 48 mnt", COLOR_CHIP_BLUE},
    {"Santai", "18 lagu - 1 j 12 mnt", COLOR_CHIP_PURPLE},
    {"Olahraga", "9 lagu - 36 mnt", COLOR_CHIP_BROWN},
    {"Perjalanan", "7 lagu - 29 mnt", COLOR_CHIP_GREEN},
};
#else
static const Playlist kPlaylists[] = {
    {"Semua lagu", "0 lagu", COLOR_CHIP_NEUTRAL},
    {"Favorit", "0 lagu", COLOR_CHIP_BLUE},
    {"Santai", "0 lagu", COLOR_CHIP_PURPLE},
    {"Olahraga", "0 lagu", COLOR_CHIP_BROWN},
    {"Perjalanan", "0 lagu", COLOR_CHIP_GREEN},
};
#endif

void screen_playlists_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_AUDIO, "Playlist", true);

    const lv_coord_t row_h = 32, row_gap = 4, first_y = 30;
    for (size_t i = 0; i < sizeof(kPlaylists) / sizeof(kPlaylists[0]); i++) {
        lv_obj_t *row = ui_list_row_create(scr, first_y + i * (row_h + row_gap), row_h, false);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, row_clicked_cb, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(screen_manager_group(), row);

        lv_obj_t *chip = lv_obj_create(row);
        lv_obj_remove_style_all(chip);
        lv_obj_set_size(chip, 28, 28);
        lv_obj_set_style_radius(chip, RADIUS_CHIP, 0);
        lv_obj_set_style_bg_color(chip, kPlaylists[i].chip, 0);
        lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
        lv_obj_center(icon_symbol(chip, LV_SYMBOL_AUDIO, COLOR_TEXT_PRIMARY, FONT_SMALL));

        ui_row_text_create(row, kPlaylists[i].name, kPlaylists[i].meta);
    }

    ui_footer_create(scr, "Playlist dari /Playlists - Klik buka");
}
