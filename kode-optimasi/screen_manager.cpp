#include "screen_manager.h"
#include "ui_common.h"
#include "screen_now_playing.h"
#include "screen_library.h"
#include "screen_menu.h"
#include "screen_receiver.h"
#include "screen_transmitter.h"
#include "screen_transfer.h"
#include "screen_switch.h"
#include "screen_playlists.h"
#include "screen_playlist_detail.h"

static lv_obj_t *s_root = NULL;   // one persistent screen, loaded exactly once
static ScreenId s_current = SCREEN_COUNT; // invalid until first show

static void build_screen(lv_obj_t *root, ScreenId id) {
    switch (id) {
        case SCREEN_NOW_PLAYING:     screen_now_playing_create(root); break;
        case SCREEN_LIBRARY:         screen_library_create(root); break;
        case SCREEN_MENU:            screen_menu_create(root); break;
        case SCREEN_RECEIVER:        screen_receiver_create(root); break;
        case SCREEN_TRANSMITTER:     screen_transmitter_create(root); break;
        case SCREEN_TRANSFER:        screen_transfer_create(root); break;
        case SCREEN_SWITCH:          screen_switch_create(root); break;
        case SCREEN_PLAYLISTS:       screen_playlists_create(root); break;
        case SCREEN_PLAYLIST_DETAIL: screen_playlist_detail_create(root); break;
        default:                     screen_now_playing_create(root); break;
    }
}

void screen_manager_init(ScreenId initial) {
    s_root = ui_screen_create();
    lv_scr_load(s_root); // the only lv_scr_load* call in this codebase
    screen_manager_show(initial);
}

void screen_manager_show(ScreenId id) {
    // lv_obj_clean deletes every child of s_root (freeing their memory + any
    // lv_anim_t targeting them) BEFORE build_screen creates the next batch,
    // so peak RAM is always one screen's worth, never two. s_root itself is
    // never deleted/reloaded, which also sidesteps LVGL's act_scr/prev_scr
    // screen-stack bookkeeping entirely (manually lv_obj_del'ing the active
    // screen behind lv_scr_load's back corrupted that bookkeeping and
    // crashed -- caught by the desktop sim).
    lv_obj_clean(s_root);
    build_screen(s_root, id);
    s_current = id;
}

ScreenId screen_manager_current(void) {
    return s_current;
}
