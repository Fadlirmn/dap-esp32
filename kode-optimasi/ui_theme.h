#pragma once
// Single source of truth for color/radius/font tokens, shared by all 12
// screens/overlays. Values are the dap-ui-landscape mockup palette, with the
// 5 tokens the user explicitly named (bg, card, accent, track, muted text)
// overridden to their exact Material You Dark spec hex codes.
#include "lvgl.h"

// --- user-specified Material You Dark tokens (override mockup values) ---
#define COLOR_BG          lv_color_hex(0x121619)  // mockup had 0x101418
#define COLOR_CARD_BLUE   lv_color_hex(0x0F3870)  // mockup had 0x00468C (Local Player hero + "Favorit" chip)
#define COLOR_ACCENT      lv_color_hex(0xA4C8FF)  // mockup had 0xA8C8FF
#define COLOR_TRACK        lv_color_hex(0x3B4C5E) // mockup had 0x33383F
#define COLOR_TEXT_MUTED   lv_color_hex(0x8C9BAE) // mockup had 0xC3C6CF / 0xA3ACB8

// --- rest of the mockup palette (no equivalent in the 5-value spec) ---
#define COLOR_SURFACE          lv_color_hex(0x1C2026) // header/footer bars, info cards, menu tiles
#define COLOR_SURFACE_BUTTON   lv_color_hex(0x272A30) // transport buttons, outline buttons, status pill
#define COLOR_SELECTED         lv_color_hex(0x3A4658) // selected row/tile bg, "MENU" pill bg
#define COLOR_ACCENT_TEXT      lv_color_hex(0x003062) // icon color on top of COLOR_ACCENT fills
#define COLOR_TEXT_PRIMARY     lv_color_hex(0xE2E2E9)
#define COLOR_SUCCESS          lv_color_hex(0x7DDB9A) // connected badge, battery fill, volume-normal
#define COLOR_SUCCESS_DARK     lv_color_hex(0x00391F) // icon on top of COLOR_SUCCESS
#define COLOR_ERROR            lv_color_hex(0xFFB4AB) // low battery, SD error, volume-muted
#define COLOR_WARNING          lv_color_hex(0xFFD97D) // transfer progress amber
#define COLOR_BT_HERO          lv_color_hex(0x3B2A5F) // BT receiver/transmitter hero card, "Santai" chip
#define COLOR_BT_HERO_ICON     lv_color_hex(0xE9DDFF) // icon stroke on COLOR_BT_HERO
#define COLOR_ON_ACCENT_DIM    lv_color_hex(0xD6E3FF) // shuffle/repeat icons on COLOR_CARD_BLUE
#define COLOR_CASSETTE_WINDOW  lv_color_hex(0x001B3E) // tape window between cassette reels
#define COLOR_SCROLLBAR        lv_color_hex(0x8B9199)
#define COLOR_TOAST_BG         lv_color_hex(0xE2E2E9) // inverted light toast (SD error)
#define COLOR_TOAST_TEXT       lv_color_hex(0x2E3036)
#define COLOR_OVERLAY_DIM      lv_color_hex(0x0B0E12) // dimmed backdrop behind a modal dialog
#define COLOR_VOLUME_TRACK     lv_color_hex(0x4A5059) // volume HUD slider track (distinct from COLOR_TRACK)

// Per-playlist chip colors (screen_playlists)
#define COLOR_CHIP_NEUTRAL  COLOR_SELECTED
#define COLOR_CHIP_BLUE     COLOR_CARD_BLUE
#define COLOR_CHIP_PURPLE   COLOR_BT_HERO
#define COLOR_CHIP_BROWN    lv_color_hex(0x5A3A1A)
#define COLOR_CHIP_GREEN    lv_color_hex(0x1F4A3A)

// --- corner radii ---
#define RADIUS_CARD          24 // main "kaset"/hero card (user spec)
#define RADIUS_BUTTON_LG     18 // play/pause button, playlist-detail action buttons
#define RADIUS_BUTTON_CIRCLE 21 // prev/next transport buttons
#define RADIUS_BADGE         10 // "MENU" pill, battery chip
#define RADIUS_ROW           12 // selected list row / menu tile
#define RADIUS_CHIP           8 // playlist icon chip
#define RADIUS_ICON_BADGE    14 // menu-tile icon badge
#define RADIUS_DIALOG        28 // low-battery alert dialog
#define RADIUS_TOAST          8 // SD-error toast
#define RADIUS_PILL          22 // volume HUD pill

// --- fonts (LVGL built-in Montserrat; no custom font build) ---
// ponytail: LVGL's built-in Montserrat has no bold weight, so the spec's
// "bold" title font is approximated with the regular 18px face. Upgrade
// path: build a custom bold font with lv_font_conv if real bold is needed.
#define FONT_TITLE  (&lv_font_montserrat_18)
#define FONT_BODY   (&lv_font_montserrat_14)
#define FONT_SMALL  (&lv_font_montserrat_12)

#define HEADER_H 26
#define FOOTER_H 28
