#include "ui_common.h"
#include "ui_theme.h"
#include "ui_icons.h"
#include "display_pins.h"
#include "screen_manager.h"
#include "overlay_volume.h"

static void menu_badge_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_MENU);
}

static void vol_badge_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_VOLUME);
}

lv_obj_t *ui_screen_create(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    return scr;
}

lv_obj_t *ui_header_create(lv_obj_t *screen, const char *header_icon, const char *title,
                            bool show_menu_badge) {
    lv_obj_t *bar = lv_obj_create(screen);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, SCREEN_W, HEADER_H);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(bar, 10, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *left = lv_obj_create(bar);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, 6, 0);
    if (header_icon) icon_symbol(left, header_icon, COLOR_ACCENT, FONT_SMALL);
    lv_obj_t *title_lbl = lv_label_create(left);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title_lbl, FONT_SMALL, 0);

    lv_obj_t *right = lv_obj_create(bar);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    // START, not END: `right` already hugs its own content (LV_SIZE_CONTENT),
    // so there's no extra space to justify either way -- but END-placement
    // on a hug-sized flex container is exactly what broke when the badge
    // shrank from a wide text+icon pill to a small icon-only circle (it
    // rendered invisible/off-position). START sidesteps that LVGL
    // hug-resize/end-align interaction entirely; visually identical for a
    // box with no slack space.
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 8, 0);

    if (show_menu_badge) {
        // Icon-only circular badge -- no "MENU" text label. Simpler than
        // chasing nested-flex auto-width sizing for icon+text, and avoids
        // ever clipping the text again.
        lv_obj_t *badge = lv_obj_create(right);
        lv_obj_remove_style_all(badge);
        lv_obj_set_size(badge, 22, 22);
        lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(badge, COLOR_SELECTED, 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_center(icon_symbol(badge, LV_SYMBOL_HOME, COLOR_TEXT_PRIMARY, FONT_BODY));

        lv_obj_add_flag(badge, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(badge, menu_badge_clicked_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_opa(badge, LV_OPA_70, LV_STATE_PRESSED); // press feedback, see ui_button_create
        lv_group_add_obj(screen_manager_group(), badge); // reachable via PREV/NEXT on every screen that has it
    }

    // Was icon_battery(right, battery_percent) -- battery_percent was always
    // a hardcoded 78 at every call site (no real battery ADC exists), so
    // this slot is repurposed for the real, live volume level instead.
    int vol = overlay_volume_get();
    lv_obj_t *vol_row = lv_obj_create(right);
    lv_obj_remove_style_all(vol_row);
    lv_obj_set_size(vol_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(vol_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(vol_row, 3, 0);
    lv_obj_clear_flag(vol_row, LV_OBJ_FLAG_SCROLLABLE);

    const char *vol_glyph = vol <= 0 ? LV_SYMBOL_MUTE : vol <= 50 ? LV_SYMBOL_VOLUME_MID : LV_SYMBOL_VOLUME_MAX;
    lv_color_t vol_color = vol <= 0 ? COLOR_ERROR : COLOR_TEXT_MUTED;
    icon_symbol(vol_row, vol_glyph, vol_color, FONT_SMALL);
    lv_obj_t *vol_pct = lv_label_create(vol_row);
    lv_label_set_text_fmt(vol_pct, "%d%%", vol);
    lv_obj_set_style_text_color(vol_pct, vol_color, 0);
    lv_obj_set_style_text_font(vol_pct, FONT_SMALL, 0);

    lv_obj_add_flag(vol_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(vol_row, vol_badge_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_opa(vol_row, LV_OPA_60, LV_STATE_PRESSED); // press feedback, see ui_button_create
    lv_group_add_obj(screen_manager_group(), vol_row);

    return bar;
}

lv_obj_t *ui_footer_create(lv_obj_t *screen, const char *hint_text) {
    lv_obj_t *bar = lv_obj_create(screen);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, SCREEN_W, FOOTER_H);
    lv_obj_set_pos(bar, 0, SCREEN_H - FOOTER_H);
    lv_obj_set_style_bg_color(bar, COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(bar);
    lv_label_set_text(lbl, hint_text);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
    lv_obj_center(lbl);
    return bar;
}

lv_obj_t *ui_card_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                          lv_coord_t radius, lv_color_t bg) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_radius(card, radius, 0);
    lv_obj_set_style_bg_color(card, bg, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

lv_obj_t *ui_button_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                            lv_coord_t radius, lv_color_t bg) {
    lv_obj_t *btn = ui_card_create(parent, x, y, w, h, radius, bg);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    // Press feedback: dims while actually held down (LVGL's native PRESSED
    // state), reverts the instant it's released -- no "stays highlighted"
    // state, no manual toggle bookkeeping needed (ui_card_create already
    // strips the default theme's own press style via remove_style_all, so
    // every clickable widget in the app was giving zero visual feedback on
    // click before this).
    lv_obj_set_style_bg_opa(btn, LV_OPA_70, LV_STATE_PRESSED);
    lv_group_add_obj(screen_manager_group(), btn); // reachable via physical PREV/NEXT, see input.cpp
    return btn;
}

lv_obj_t *ui_list_row_create(lv_obj_t *parent, lv_coord_t y, lv_coord_t h, bool selected) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, SCREEN_W - 12, h);
    lv_obj_set_pos(row, 6, y);
    lv_obj_set_style_radius(row, selected ? RADIUS_ROW : 0, 0);
    lv_obj_set_style_bg_color(row, COLOR_SELECTED, 0);
    lv_obj_set_style_bg_opa(row, selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_hor(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    // Press feedback only while actually held down -- same reasoning as
    // ui_button_create. On an unselected row (bg_opa TRANSP by default) this
    // is a brief highlight flash; on the selected row it's a slight dim.
    // Either way it reverts the instant the press is released.
    lv_obj_set_style_bg_opa(row, LV_OPA_80, LV_STATE_PRESSED);
    return row;
}

lv_obj_t *ui_row_text_create(lv_obj_t *row, const char *title, const char *subtitle) {
    lv_obj_t *stack = lv_obj_create(row);
    lv_obj_remove_style_all(stack);
    lv_obj_set_size(stack, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(stack, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title_lbl = lv_label_create(stack);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title_lbl, FONT_BODY, 0);

    if (subtitle && subtitle[0]) {
        lv_obj_t *sub_lbl = lv_label_create(stack);
        lv_label_set_text(sub_lbl, subtitle);
        lv_obj_set_style_text_color(sub_lbl, COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_font(sub_lbl, FONT_SMALL, 0);
    }
    return stack;
}
