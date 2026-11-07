# Desktop SDL simulator: compiles the real firmware logic (../theme.cpp etc.)
# unmodified against stub headers in this directory instead of the ESP32 core.
CXX = g++
CXXFLAGS = -std=c++17 -O1 -Wall -I.. -I. $(shell pkg-config --cflags sdl2 freetype2)
LDFLAGS = $(shell pkg-config --libs sdl2 freetype2)

SRCS = sim_main.cpp sim_text.cpp audio_decoder_stub.cpp \
       ../theme.cpp ../gfx_helpers.cpp ../icons.cpp ../screens.cpp \
       ../nav.cpp ../playlist.cpp ../bluetooth.cpp \
       ../bt_receiver.cpp ../wifi_transfer.cpp ../i2s_output.cpp

sim: $(SRCS) *.h ../*.h
	$(CXX) $(CXXFLAGS) $(SRCS) -o sim $(LDFLAGS)

clean:
	rm -f sim

.PHONY: clean
