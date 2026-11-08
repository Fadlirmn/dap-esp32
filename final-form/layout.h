// Screen geometry: landscape 320x240. Re-matched against the dedicated
// landscape mockup (dap-ui-landscape/source/L_*.dc.html), NOT the portrait
// one a prior pass used (dap-ui-design/, since deleted) -- every constant
// below is read off that mockup's literal inline pixel styles, not adapted
// from portrait proportions.
#pragma once

constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

// Every L_*.dc.html header div overrides its CSS class's 28px default down
// to 26px inline -- that 26 is what's actually on screen in every mockup.
constexpr int HEADER_H = 26;
constexpr int FOOTER_H = 28;  // footer keeps its 28px default, no override anywhere

// Dummy battery %: no ADC pin wired for real battery sensing (pins.h has
// none), so the header just shows this constant.
constexpr uint8_t DUMMY_BATTERY_PCT = 78;

// Mode Select (STATE_MODE_SELECT / L_Menu.dc.html): a 3-column x 3-row grid
// of 9 cells -- Daftar Lagu, Playlist, Likes, Local Player, BT Receiver,
// BT Transmitter, Transfer File, Settings, Kembali. This is also the ONLY
// way into Settings/Likes now: the old separate STATE_MENU icon-grid home
// screen (no L_*.dc.html counterpart at all, since the mockup's "home" for
// Local Player/BT Transmitter is just Now Playing directly) is gone, and
// this grid already covered Daftar Lagu/Playlist/mode-switching, so Likes
// and Settings join it rather than getting a dead screen kept alive just
// for two entries. 3 columns (not the mockup's literal 2) to fit all 9
// cells in 3 rows instead of 5 -- 5 rows would overflow past the footer.
constexpr int MODE_GRID_X0 = 8, MODE_GRID_X1 = 111, MODE_GRID_X2 = 214;
constexpr int MODE_GRID_COL_W = 98;
constexpr int MODE_GRID_Y_START = 34;
constexpr int MODE_GRID_ROW_H = 40;
constexpr int MODE_GRID_ROW_GAP = 6;
constexpr int MODE_GRID_COUNT = 9;

// Floating auto-hiding volume overlay (L_Volume/L_VolumeList.dc.html): pill
// at left14/right14/top34/height44, same spot regardless of which screen
// it's floating over.
constexpr int VOL_OVERLAY_X = 14, VOL_OVERLAY_W = SCREEN_W - 28, VOL_OVERLAY_H = 44;
constexpr int VOL_OVERLAY_Y = 34;

// ---- Player-type screens (drawPlayerScreen: Local Player + BT Transmitter) ----
// Panel geometry per L_Main.dc.html / L_Transmitter.dc.html (identical for
// both): left12/right12/top34/height106.
constexpr int PL_PANEL_X = 12, PL_PANEL_Y = 34, PL_PANEL_W = SCREEN_W - 24, PL_PANEL_H = 106;

// Album art now lives INSIDE the panel: a square slice sized to the panel's
// height minus a little padding, left-anchored -- not stretched across the
// panel's ~296px width (the mockup's own graphic there is decorative, sized
// for a wide panel, not a thumbnail). 94x94 = 106 - 2*6 padding.
// drawCoverArt()'s row-read assumes a .raw file matches exactly whatever w/h
// is passed in, and this redesign is the first live caller (the pass before
// this one removed its only other caller and never shipped art at any
// fixed size), so there is no existing SD card to corrupt.
constexpr int PL_ART_PAD = 6;
constexpr int PL_ART_SIZE = PL_PANEL_H - 2 * PL_ART_PAD;  // 94
constexpr int PL_ART_X = PL_PANEL_X + PL_ART_PAD;
constexpr int PL_ART_Y = PL_PANEL_Y + PL_ART_PAD;

// Right-hand info column inside the panel (title/subtitle + either the
// shuffle/repeat icons [Local Player] or the connection badge [BT
// Transmitter]) -- the mockup's own panel interior is a decorative graphic
// being replaced by real art, so this sub-arrangement is a deliberate
// redesign, not mockup-literal pixels; only the panel's outer box (above)
// and the progress/transport rhythm (below) are mockup-literal.
constexpr int PL_PANEL_INFO_X = PL_ART_X + PL_ART_SIZE + 10;
constexpr int PL_PANEL_INFO_RIGHT = PL_PANEL_X + PL_PANEL_W - 10;
constexpr int PL_PANEL_BADGE_CY = PL_PANEL_Y + 18;
constexpr int PL_PANEL_TITLE_Y = PL_PANEL_Y + 38;
constexpr int PL_PANEL_SUBTITLE_Y = PL_PANEL_TITLE_Y + 22;

// Progress bar: left16/right16/top148/height4 (L_Main.dc.html's `.k`). No
// mm:ss time labels under it -- audio_decoder.h/playlist.h track byte
// progress only, no duration, so there's nothing truthful to print there
// (same deviation the previous pass already made and documented).
constexpr int PL_PROGRESS_X = 16, PL_PROGRESS_Y = 148, PL_PROGRESS_W = SCREEN_W - 32, PL_PROGRESS_H = 4;

// Transport row: prev/next are true circles (diameter42, radius21); the
// center play/pause button is a rounded SQUARE (diameter56, corner radius
// 18 -- NOT half of 56, so not a circle) per L_Main.dc.html's literal
// border-radius values. bottom:10, gap:28, all three vertically centered
// on PL_BTN_CY.
constexpr int PL_BTN_CY = 202;
constexpr int PL_BTN_R = 21;  // prev/next circle radius; also nav.cpp's touch hitbox radius for all 3 buttons
constexpr int PL_PREV_CX = 83;
constexpr int PL_PLAY_CX = 160;
constexpr int PL_NEXT_CX = 237;
constexpr int PL_PLAY_SIZE = 56, PL_PLAY_RADIUS = 18;  // play/pause rounded-square button

// Like/heart toggle: no mockup counterpart at all (favorit-state is shown as
// plain text in the Local Player subtitle, "Favorit"), but
// handleEncoderLongPress()/nav.cpp's touch path still call toggleLike() and
// hit-test this exact rect, so it stays -- relocated into the otherwise-empty
// gap between the progress bar and the transport row.
constexpr int PL_HEART_CX = 293, PL_HEART_CY = 166, PL_HEART_R = 9;

// ---- BT Receiver (drawBTReceiverScreen / L_Receiver.dc.html) ----
// Same panel box as the Player screens but TALLER (130, not 106) and with a
// single centered icon -- no album art (phone audio has no local file to
// read art for), so this one keeps its "single icon" concept, just
// re-pixeled.
constexpr int BTRX_PANEL_H = 130;
constexpr int BTRX_SCRIM_H = 34;  // bottom title/subtitle band inside the panel

// ---- Track/Likes list rows (L_Library.dc.html) ----
// `.r{height:37px;margin:0 6px}`, stacked with NO gap, first row starting
// right at the header's bottom edge -- 5 rows fill exactly to the footer
// (26 + 5*37 = 211, footer top is 212), not 6.
constexpr int LIST_ITEMS_PER_PAGE = 5;
constexpr int LIST_ITEM_H = 37;
constexpr int LIST_ITEM_Y_START = HEADER_H;
constexpr int LIST_ITEM_GAP = 0;

// ---- Transfer File (drawTransferFileScreen / L_Transfer.dc.html) ----
// Two-column layout: left column = hotspot SSID card + URL card; right
// column = upload-progress card + status chip.
constexpr int TF_LEFT_X = 12, TF_LEFT_W = 150;
constexpr int TF_RIGHT_X = 172, TF_RIGHT_W = SCREEN_W - TF_RIGHT_X - 12;
constexpr int TF_TOP_Y = 36;
constexpr int TF_CARD_RADIUS = 16;
constexpr int TF_CARD_GAP = 8;
constexpr int TF_CARD_H = 40;           // left column cards (2 lines each)
constexpr int TF_PROGRESS_CARD_H = 62;  // right column's upload card
constexpr int TF_CHIP_H = 24;

// ---- Playlist Detail (drawPlaylistDetailScreen / L_PlaylistDetail.dc.html) ----
// Left column: Putar/Acak/Ulang pill buttons + stats text. Right column:
// track rows, same row rhythm as the track list but starting at x=104.
constexpr int PD_LEFT_X = 12, PD_LEFT_W = 92, PD_TOP_Y = 36;
constexpr int PD_BTN_H = 36, PD_BTN_RADIUS = 18, PD_BTN_GAP = 8;
constexpr int PD_LIST_X = 104;

// Legacy inline +/- volume row: drawPlayerScreen draws nothing here anymore
// (the mockup has no persistent on-screen volume control, only the floating
// overlay above) -- these stay defined only because nav.cpp's optional
// touch path hit-tests these exact rects and nav.cpp is off-limits to edit
// outside the STATE_MODE_SELECT grid-nav change.
constexpr int PL_VOL_Y = 212, PL_VOL_H = 22;
constexpr int PL_VOLDOWN_X = 14, PL_VOLDOWN_W = 36;
constexpr int PL_VOLUP_X = 270, PL_VOLUP_W = 36;
constexpr int PL_VOLTEXT_X = 60, PL_VOLTEXT_W = 200;
