#pragma once
// Shared chrome reused by every standalone screen: base screen, header bar
// (icon+title, optional "MENU" pill, battery), footer hint bar, a generic
// rounded card, a generic clickable button, and a list-row container +
// title/subtitle text block. One copy of each instead of 8 copies.
#include "lvgl.h"

// Full-screen base lv_obj, COLOR_BG, no padding/scrollbar. Caller fills it in.
lv_obj_t *ui_screen_create(void);

// header_icon may be NULL (e.g. the Menu screen's plain title, no icon/no MENU pill).
// The right side always shows the live volume level (click -> SCREEN_VOLUME),
// replacing what used to be a battery readout with no real battery to read.
lv_obj_t *ui_header_create(lv_obj_t *screen, const char *header_icon, const char *title,
                            bool show_menu_badge);

lv_obj_t *ui_footer_create(lv_obj_t *screen, const char *hint_text);

lv_obj_t *ui_card_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                          lv_coord_t radius, lv_color_t bg);

// Generic clickable rounded button; caller adds its own icon/label as a child, centered.
lv_obj_t *ui_button_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                            lv_coord_t radius, lv_color_t bg);

// Absolute-positioned list row container (flex row, centered, gap). Caller
// appends a leading widget (index label / icon chip / equalizer bars) then
// calls ui_row_text_create for the title/subtitle stack.
lv_obj_t *ui_list_row_create(lv_obj_t *parent, lv_coord_t y, lv_coord_t h, bool selected);

lv_obj_t *ui_row_text_create(lv_obj_t *row, const char *title, const char *subtitle);
