#pragma once
// Cosmetic background timers only (volume HUD / SD toast / Now Playing
// elapsed tick) -- real screen navigation is click-driven (tap the "MENU"
// badge, a Menu tile, or a Playlists row; see ui_common.cpp / screen_menu.cpp
// / screen_playlists.cpp). Shared by the real .ino and sim/sim_main.cpp.
// Call once, after screen_manager_init().
void demo_cycle_init(void);
