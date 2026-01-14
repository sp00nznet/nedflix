# Nedflix PSX - Makefile for PSn00bSDK

PREFIX = mipsel-none-elf-
CC = $(PREFIX)gcc
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy

PSN00BSDK ?= /opt/psn00bsdk

CFLAGS = -c -O2 -Wall -fno-builtin -fdata-sections -ffunction-sections
CFLAGS += -I$(PSN00BSDK)/include -DNDEBUG
LDFLAGS = -T$(PSN00BSDK)/lib/ldscripts/exe.ld -L$(PSN00BSDK)/lib
LIBS = -lpsn00b -lc

SRCS = src/main.c src/ui.c src/input.c src/audio.c src/network.c src/api.c src/config.c
OBJS = $(SRCS:.c=.o)

TARGET = nedflix

all: $(TARGET).exe

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(TARGET).exe: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).exe

.PHONY: all clean
