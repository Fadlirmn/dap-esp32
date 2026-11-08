#include "ui_icons.h"
#include "ui_theme.h"

lv_obj_t *icon_symbol(lv_obj_t *parent, const char *symbol, lv_color_t color, const lv_font_t *font) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, font, 0);
    return label;
}

lv_obj_t *icon_battery(lv_obj_t *parent, int percent) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 3, 0);

    const char *glyph = percent > 80 ? LV_SYMBOL_BATTERY_FULL
                       : percent > 55 ? LV_SYMBOL_BATTERY_3
                       : percent > 30 ? LV_SYMBOL_BATTERY_2
                       : percent > 15 ? LV_SYMBOL_BATTERY_1
                       : LV_SYMBOL_BATTERY_EMPTY;
    lv_color_t color = percent <= 15 ? COLOR_ERROR : COLOR_TEXT_MUTED;

    icon_symbol(row, glyph, color, FONT_SMALL);

    lv_obj_t *pct = lv_label_create(row);
    lv_label_set_text_fmt(pct, "%d%%", percent);
    lv_obj_set_style_text_color(pct, color, 0);
    lv_obj_set_style_text_font(pct, FONT_SMALL, 0);
    return row;
}

lv_obj_t *icon_connected_badge(lv_obj_t *parent) {
    lv_obj_t *badge = lv_obj_create(parent);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, 22, 22);
    lv_obj_set_style_radius(badge, 11, 0);
    lv_obj_set_style_bg_color(badge, COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);

    lv_obj_t *check = lv_label_create(badge);
    lv_label_set_text(check, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(check, COLOR_SUCCESS_DARK, 0);
    lv_obj_center(check);
    return badge;
}

lv_obj_t *icon_equalizer_bars(lv_obj_t *parent, lv_color_t color) {
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, 16, 16);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(box, 2, 0);

    static const uint8_t heights[3] = {6, 12, 9};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *bar = lv_obj_create(box);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, 3, heights[i]);
        lv_obj_set_style_bg_color(bar, color, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 1, 0);
    }
    return box;
}
