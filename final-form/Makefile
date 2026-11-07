# Thin wrapper around PlatformIO (see platformio.ini) for the real ESP32
# firmware. For the desktop UI sim instead, use sim/Makefile.
#
# Usage: make [build|upload|monitor|clean]  [PORT=/dev/ttyUSB0]
PORT ?=
PORTFLAG = $(if $(PORT),--upload-port $(PORT) --monitor-port $(PORT),)

.PHONY: build upload monitor clean

build:
	pio run

upload:
	pio run -t upload $(PORTFLAG)

monitor:
	pio device monitor $(if $(PORT),-p $(PORT),)

clean:
	pio run -t clean
