# Nedflix Game Boy - Makefile for GBDK-2020

GBDK ?= /opt/gbdk

CC = $(GBDK)/bin/lcc
CFLAGS = -Wa-l -Wl-m -Wl-j

TARGET = nedflix

SOURCES = src/main.c src/ui.c src/input.c src/audio.c src/link.c src/config.c

.PHONY: all clean

all: $(TARGET).gb

$(TARGET).gb: $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET).gb $(TARGET).map $(TARGET).sym
