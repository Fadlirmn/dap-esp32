#pragma once
// L7 "Baterai rendah dan error" — two unrelated top-layer popup types bundled
// in one mockup board: a modal alert dialog and a non-modal bottom toast.
#include "lvgl.h"

// Centered modal dialog with a dimmed backdrop; dismissed by its OK button.
void overlay_alert_show_low_battery(int percent);

// Non-modal bottom toast; auto-dismisses after 3s.
void overlay_alert_show_sd_error(void);
