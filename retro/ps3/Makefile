# Nedflix PS3 - Makefile for PSL1GHT

ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment")
endif

include $(PSL1GHT)/ppu_rules

TARGET   := nedflix
BUILD    := build
SOURCES  := src
INCLUDES := src

TITLE    := Nedflix
APPID    := NEDFLIX01
CONTENTID := UP0001-$(APPID)_00-0000000000000000

CFLAGS   := -O2 -Wall -mcpu=cell $(MACHDEP) $(INCLUDE)
LDFLAGS  := $(MACHDEP) -Wl,-Map,$(notdir $@).map

LIBS     := -lrsx -lgcm_sys -lsysutil -lrt -lio -lnet -lnetctl -lsysfs -lm

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
OFILES   := $(CFILES:.c=.o)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

.PHONY: all clean pkg

all: $(TARGET).self

$(TARGET).elf: $(OFILES)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

$(TARGET).self: $(TARGET).elf
	$(FSELF) $< $@

pkg: $(TARGET).self
	$(PKG) --contentid $(CONTENTID) $(TARGET).self

clean:
	rm -f $(OFILES) $(TARGET).elf $(TARGET).self
