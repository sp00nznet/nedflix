# Nedflix GBA - Makefile for devkitARM

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment")
endif

include $(DEVKITPRO)/devkitARM/gba_rules

TARGET   := nedflix
BUILD    := build
SOURCES  := src
INCLUDES := src

ARCH := -mthumb -mthumb-interwork

CFLAGS := -g -Wall -O2 $(ARCH) $(INCLUDE)
LDFLAGS := -g $(ARCH) -specs=gba.specs

LIBS := -lmm -lgba

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
OFILES   := $(CFILES:.c=.o)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

.PHONY: all clean

all: $(TARGET).gba

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	gbafix $@

$(TARGET).elf: $(OFILES)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OFILES) $(TARGET).elf $(TARGET).gba
