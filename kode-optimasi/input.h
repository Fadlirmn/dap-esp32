// Rotary encoder (volume) + 3 physical buttons (PREV/NEXT = universal
// focus-navigation via screen_manager.h's lv_group, ENC_SW = confirm/select,
// MENU = jump to the mode grid). ISR/debounce timing ported from
// final-form/input.cpp; retargeted at an LVGL LV_INDEV_TYPE_ENCODER instead
// of final-form's own nav.h handlers (see input.cpp).
#pragma once

void inputSetup();  // pinMode + attachInterrupt + registers the LVGL encoder indev; call once from setup(), after screen_manager_init()
void inputPoll();   // call every guiTask tick
