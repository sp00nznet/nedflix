# Nedflix NES - Makefile for cc65

CC = cl65
AS = ca65
LD = ld65

TARGET = nedflix
CONFIG = nes.cfg

CFLAGS = -t nes -Oirs --add-source
ASFLAGS = -t nes
LDFLAGS = -t nes -C $(CONFIG)

SOURCES = src/main.c src/ui.c src/input.c src/audio.c src/config.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET).nes

$(TARGET).nes: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET).nes
