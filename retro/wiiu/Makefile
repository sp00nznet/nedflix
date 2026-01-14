# Nedflix Wii U - Makefile for wut

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment")
endif

include $(DEVKITPRO)/wut/share/wut_rules

TARGET   := nedflix
BUILD    := build
SOURCES  := src
INCLUDES := src

CFLAGS   := -g -Wall -O2 $(MACHDEP)
CXXFLAGS := $(CFLAGS)
LDFLAGS  := -g $(MACHDEP) $(RPXSPECS)

LIBS := -lwut

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
OFILES   := $(CFILES:.c=.o)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

.PHONY: all clean

all: $(TARGET).rpx

$(TARGET).rpx: $(TARGET).elf

$(TARGET).elf: $(OFILES)

clean:
	rm -rf $(BUILD) $(TARGET).rpx $(TARGET).elf
