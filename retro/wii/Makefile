# Nedflix Wii - Makefile for devkitPPC

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment")
endif

include $(DEVKITPRO)/devkitPPC/wii_rules

TARGET   := nedflix
BUILD    := build
SOURCES  := src
INCLUDES := src

CFLAGS   := -g -O2 -Wall $(MACHDEP) $(INCLUDE) -DGEKKO -DWII
CXXFLAGS := $(CFLAGS)
LDFLAGS  := -g $(MACHDEP) -Wl,-Map,$(notdir $@).map

LIBS := -lwiiuse -lbte -lfat -logc -lm

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
OFILES   := $(CFILES:.c=.o)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

.PHONY: all clean

all: $(BUILD) $(TARGET).dol

$(BUILD):
	@mkdir -p $@

$(TARGET).dol: $(TARGET).elf
	@elf2dol $< $@

$(TARGET).elf: $(OFILES)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).dol *.map
