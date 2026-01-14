# Nedflix Switch - Makefile for libnx

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment")
endif

include $(DEVKITPRO)/libnx/switch_rules

TARGET   := nedflix
BUILD    := build
SOURCES  := src
INCLUDES := src

APP_TITLE   := Nedflix
APP_AUTHOR  := Nedflix
APP_VERSION := 1.0.0

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS := -g -Wall -O2 $(ARCH) $(DEFINES) $(INCLUDE)
LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lnx

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
OFILES   := $(CFILES:.c=.o)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

.PHONY: all clean

all: $(TARGET).nro

$(TARGET).nro: $(TARGET).elf
	$(NACPTOOL) --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $*.nacp
	$(ELF2NRO) $< $@ --nacp=$*.nacp

$(TARGET).elf: $(OFILES)

clean:
	rm -rf $(BUILD) $(TARGET).nro $(TARGET).elf $(TARGET).nacp
