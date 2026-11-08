#include "cassette_anim.h"
#include "ui_theme.h"

#define REEL_DIAMETER 28

static lv_obj_t *s_spoke[2] = {NULL, NULL};
static lv_point_t s_spoke_pts[2] = {{0, 0}, {REEL_DIAMETER, 0}};

static void set_angle_cb(void *obj, int32_t v) {
    lv_obj_set_style_transform_angle((lv_obj_t *)obj, (int16_t)v, 0);
}

static lv_obj_t *make_reel(lv_obj_t *card, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *reel = lv_obj_create(card);
    lv_obj_remove_style_all(reel);
    lv_obj_set_size(reel, REEL_DIAMETER, REEL_DIAMETER);
    lv_obj_set_pos(reel, x, y);
    lv_obj_set_style_radius(reel, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(reel, 2, 0);
    lv_obj_set_style_border_color(reel, COLOR_ON_ACCENT_DIM, 0);
    lv_obj_set_style_bg_opa(reel, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(reel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *spoke = lv_line_create(reel);
    lv_line_set_points(spoke, s_spoke_pts, 2);
    lv_obj_set_style_line_width(spoke, 2, 0);
    lv_obj_set_style_line_color(spoke, COLOR_ON_ACCENT_DIM, 0);
    lv_obj_center(spoke);
    lv_obj_set_style_transform_pivot_x(spoke, REEL_DIAMETER / 2, 0);
    lv_obj_set_style_transform_pivot_y(spoke, 1, 0);
    return spoke;
}

static void start_spin(lv_obj_t *spoke) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, spoke);
    lv_anim_set_exec_cb(&a, set_angle_cb);
    lv_anim_set_values(&a, 0, 3600); // tenths of a degree, one full turn
    lv_anim_set_time(&a, 4000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

void cassette_anim_create(lv_obj_t *card, lv_coord_t card_w, lv_coord_t card_h) {
    lv_coord_t reel_y = (card_h - REEL_DIAMETER) / 2;
    s_spoke[0] = make_reel(card, card_w / 4 - REEL_DIAMETER / 2, reel_y);
    s_spoke[1] = make_reel(card, 3 * card_w / 4 - REEL_DIAMETER / 2, reel_y);

    lv_obj_t *window = lv_obj_create(card);
    lv_obj_remove_style_all(window);
    lv_obj_set_size(window, 56, 20);
    lv_obj_align(window, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(window, 8, 0);
    lv_obj_set_style_bg_color(window, COLOR_CASSETTE_WINDOW, 0);
    lv_obj_set_style_bg_opa(window, LV_OPA_COVER, 0);

    start_spin(s_spoke[0]);
    start_spin(s_spoke[1]);
}

void cassette_anim_set_spinning(bool spinning) {
    for (int i = 0; i < 2; i++) {
        if (!s_spoke[i]) continue;
        if (spinning) {
            if (!lv_anim_get(s_spoke[i], set_angle_cb)) start_spin(s_spoke[i]);
        } else {
            lv_anim_del(s_spoke[i], set_angle_cb);
        }
    }
}
