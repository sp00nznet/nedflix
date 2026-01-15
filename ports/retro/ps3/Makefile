# Nedflix PS3 - Makefile for PSL1GHT SDK
# Technical demo build

ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment. See: https://github.com/ps3dev/PSL1GHT")
endif

include $(PSL1GHT)/ppu_rules

TARGET   := nedflix
BUILD    := build
SOURCES  := src
INCLUDES := src

# PKG metadata
TITLE     := Nedflix
APPID     := NEDFLIX01
CONTENTID := UP0001-$(APPID)_00-0000000000000000
SFOXML    := sfo.xml

# Compiler flags
CFLAGS   := -O2 -Wall -Wextra -mcpu=cell $(MACHDEP) $(INCLUDE)
CFLAGS   += -DNEDFLIX_CLIENT_MODE=1

# Linker flags
LDFLAGS  := $(MACHDEP) -Wl,-Map,$(notdir $@).map

# Libraries
# RSX graphics, GCM, system utilities, network, I/O
LIBS     := -lrsx -lgcm_sys -lsysutil -lrt -lio -lnet -lnetctl -lsysfs -lm

# Source files
CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
OFILES   := $(CFILES:.c=.o)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

.PHONY: all clean pkg self elf run

all: $(TARGET).self

# Link ELF
$(TARGET).elf: $(OFILES)
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

# Create SELF (signed executable)
$(TARGET).self: $(TARGET).elf
	@echo "Creating SELF..."
	$(FSELF) $< $@

# Create PKG for installation
pkg: $(TARGET).self
	@echo "Creating PKG..."
	$(SFO) --title "$(TITLE)" --appid "$(APPID)" PARAM.SFO
	$(PKG) --contentid $(CONTENTID) $< $@.pkg

# Alias targets
self: $(TARGET).self
elf: $(TARGET).elf

# Run via ps3load (requires network setup)
run: $(TARGET).self
	$(PS3LOAD) $<

# Clean build
clean:
	@echo "Cleaning..."
	rm -f $(OFILES) $(TARGET).elf $(TARGET).self $(TARGET).pkg PARAM.SFO *.map

# Dependency generation
-include $(OFILES:.o=.d)

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -MMD -c $< -o $@
