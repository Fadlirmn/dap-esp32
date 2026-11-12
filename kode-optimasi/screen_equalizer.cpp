// EQ preset picker -- same row-list layout as screen_playlists.cpp. No
// per-band manual sliders: PREV/NEXT only tabs focus between rows (the
// physical rotary knob is hard-wired to volume, see input.cpp), so a
// draggable slider has no input path here. Tapping a row (its full width,
// same single tap-to-pick target as every other row-list screen) applies
// that preset immediately and stays on the list. A per-row "edit" button was
// tried here but it carved a second click target out of the row, breaking
// that single-tap pick; editing instead lives in the footer, one button for
// whichever preset is currently active.
#include "screen_equalizer.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"
#include "equalizer.h"

static void preset_clicked_cb(lv_event_t *e) {
    EqPreset p = (EqPreset)(intptr_t)lv_event_get_user_data(e);
    equalizerSetPreset(p);
    screen_manager_show(SCREEN_EQUALIZER); // rebuild so the checkmark follows the new pick
}

static void edit_active_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_EQUALIZER_CUSTOM); // edits whichever preset is active (picked above)
}

void screen_equalizer_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_SETTINGS, "Equalizer", true);

    EqPreset active = equalizerGetPreset();
    // Tighter than screen_playlists.cpp's 32/4/30 -- 6 rows (5 fixed presets
    // + Custom) have to fit between the header (ends y=26) and footer
    // (starts y=212), which 32/4 only had room for 5.
    const lv_coord_t row_h = 28, row_gap = 3, first_y = 26;
    for (int i = 0; i < EQ_PRESET_COUNT; i++) {
        bool selected = (i == active);
        lv_obj_t *row = ui_list_row_create(scr, first_y + i * (row_h + row_gap), row_h, selected);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, preset_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_group_add_obj(screen_manager_group(), row);

        if (selected) icon_symbol(row, LV_SYMBOL_OK, COLOR_SUCCESS, FONT_SMALL);
        else icon_symbol(row, LV_SYMBOL_AUDIO, COLOR_ACCENT, FONT_SMALL);

        ui_row_text_create(row, equalizerPresetName((EqPreset)i), NULL);
    }

    lv_obj_t *footer = ui_footer_create(scr, "");
    lv_obj_t *footer_lbl = lv_obj_get_child(footer, 0);
    lv_label_set_text_fmt(footer_lbl, "Atur/reset \"%s\" (tekan di sini)", equalizerPresetName(active));
    lv_obj_set_style_text_color(footer_lbl, COLOR_ACCENT, 0);
    lv_obj_add_flag(footer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(footer, edit_active_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_opa(footer, LV_OPA_80, LV_STATE_PRESSED);
    lv_group_add_obj(screen_manager_group(), footer);
}
