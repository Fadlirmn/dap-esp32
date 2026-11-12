# Desktop SDL preview. Compiles the real application sources (../screen_*.cpp
# etc, unmodified) against a real, freshly-cloned lvgl checkout -- lvgl's own
# .c sources must be compiled as C (gcc), never as C++ (g++ rejects valid C
# like implicit void*->T* conversions), while our app sources are C++.
#
# `sim` clones lvgl on first run (needs network once), then re-invokes make
# so $(wildcard ...) below sees the cloned files -- wildcard is expanded at
# parse time, before the clone would otherwise exist.
LVGL_DIR = lvgl
LVGL_TAG = release/v8.3

CC = gcc
CXX = g++
# KODE_OPTIMASI_SIM gates demo/dummy content (song lists, track names, fake
# file transfers, etc.) so it only ever exists in this desktop preview, never
# in the real ESP32 build (kode-optimasi.ino / platformio.ini don't define
# it) -- see the #ifdef KODE_OPTIMASI_SIM blocks in screen_*.cpp.
INCLUDES = -I.. -I$(LVGL_DIR) -DLV_CONF_INCLUDE_SIMPLE=1 -DKODE_OPTIMASI_SIM=1
CFLAGS = -O1 -Wall $(INCLUDES)
CXXFLAGS = -std=c++17 -O1 -Wall $(INCLUDES) $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)

APP_SRCS = sim_main.cpp \
    ../demo_cycle.cpp ../screen_manager.cpp ../ui_common.cpp ../ui_icons.cpp \
    ../cassette_anim.cpp ../screen_now_playing.cpp ../screen_library.cpp \
    ../screen_menu.cpp ../screen_receiver.cpp ../screen_transmitter.cpp \
    ../screen_transfer.cpp ../screen_switch.cpp ../screen_playlists.cpp \
    ../screen_playlist_detail.cpp ../overlay_volume.cpp ../overlay_alert.cpp \
    ../screen_bt_devices.cpp ../screen_equalizer.cpp ../screen_equalizer_custom.cpp ../equalizer.cpp \
    ../screen_volume.cpp
APP_OBJS = $(APP_SRCS:.cpp=.o)
LVGL_OBJS = $(patsubst %.c,%.o,$(shell find $(LVGL_DIR)/src -name '*.c' 2>/dev/null))

sim:
	@if [ ! -d $(LVGL_DIR) ]; then git clone --depth 1 -b $(LVGL_TAG) https://github.com/lvgl/lvgl.git $(LVGL_DIR); fi
	$(MAKE) build

build: $(APP_OBJS) $(LVGL_OBJS)
	$(CXX) $(APP_OBJS) $(LVGL_OBJS) -o sim $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f sim $(APP_OBJS)
	find $(LVGL_DIR)/src -name '*.o' -delete 2>/dev/null || true

.PHONY: sim build clean
