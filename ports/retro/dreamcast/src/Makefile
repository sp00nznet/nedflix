#
# Nedflix for Sega Dreamcast
# KallistiOS Makefile
#
# Build with: make
# Build client mode: make CLIENT=1
# Build desktop mode: make CLIENT=0
# Debug build: make debug
#

# KallistiOS environment
# This expects KOS_BASE to be set in environment
ifndef KOS_BASE
$(error KOS_BASE is not set. Please source environ.sh first)
endif

# Include KOS rules
include $(KOS_BASE)/Makefile.rules

# Build mode (1 = client mode, 0 = desktop mode)
CLIENT ?= 1

# Target name
TARGET = nedflix.elf
TARGET_BIN = nedflix.bin
TARGET_CDI = nedflix.cdi

# Source files
SRCS = main.c network.c ui.c input.c audio.c api.c config.c json.c

# Object files
OBJS = $(SRCS:.c=.o)

# Compiler flags
KOS_CFLAGS += -Wall -Wextra -O2 -fno-strict-aliasing
KOS_CFLAGS += -DDREAMCAST

# Client/Desktop mode
ifeq ($(CLIENT),1)
KOS_CFLAGS += -DNEDFLIX_CLIENT_MODE=1
else
KOS_CFLAGS += -DNEDFLIX_CLIENT_MODE=0
endif

# Include paths
KOS_CFLAGS += -I.

# Libraries
# - kos: Core KallistiOS
# - m: Math library
# - kosutils: KOS utilities (VMU, etc.)
LIBS = -lm -lkosutils

# Network support (BBA required)
KOS_CFLAGS += -D_arch_dreamcast
LIBS += -lkosext2fs

# Default target
all: rm-elf $(TARGET) $(TARGET_BIN)

# Clean up old elf to force relink
rm-elf:
	-rm -f $(TARGET)

# Link the ELF
$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS) $(LIBS)

# Create raw binary from ELF
$(TARGET_BIN): $(TARGET)
	$(KOS_OBJCOPY) -R .stack -O binary $< $@

# Create bootable CDI image (requires cdi4dc tool)
cdi: $(TARGET_BIN)
	@echo "Creating CDI image..."
	@if command -v cdi4dc > /dev/null 2>&1; then \
		scramble $(TARGET_BIN) 1ST_READ.BIN; \
		cdi4dc . $(TARGET_CDI); \
	else \
		echo "cdi4dc not found. Install it to create CDI images."; \
		echo "Binary $(TARGET_BIN) created. Use other tools to create bootable disc."; \
	fi

# Create scrambled binary for IP.BIN bootstrapping
scramble: $(TARGET_BIN)
	@if command -v scramble > /dev/null 2>&1; then \
		scramble $(TARGET_BIN) 1ST_READ.BIN; \
	else \
		echo "scramble tool not found. Please build KOS tools."; \
	fi

# Install to disc structure
install: $(TARGET_BIN)
	@mkdir -p disc
	@cp $(TARGET_BIN) disc/1ST_READ.BIN
	@echo "Disc structure created in ./disc/"
	@echo "Add IP.BIN bootstrap and burn with appropriate tools."

# Clean all build artifacts
clean:
	-rm -f $(OBJS) $(TARGET) $(TARGET_BIN) $(TARGET_CDI)
	-rm -f 1ST_READ.BIN
	-rm -rf disc

# Dependency generation
depend: .depend

.depend: $(SRCS)
	@rm -f .depend
	@for src in $(SRCS); do \
		$(KOS_CC) $(KOS_CFLAGS) -MM $$src >> .depend; \
	done

# Include dependencies if available
-include .depend

# Individual source file rules
main.o: main.c nedflix.h
network.o: network.c nedflix.h
ui.o: ui.c nedflix.h
input.o: input.c nedflix.h
audio.o: audio.c nedflix.h
api.o: api.c nedflix.h
config.o: config.c nedflix.h
json.o: json.c nedflix.h

# Run in emulator (lxdream or similar)
run: $(TARGET_BIN)
	@if command -v lxdream > /dev/null 2>&1; then \
		lxdream -b $(TARGET_BIN); \
	else \
		echo "lxdream emulator not found."; \
	fi

# Debug build
debug: KOS_CFLAGS += -g -DDEBUG
debug: clean all

# Release build (optimized)
release: KOS_CFLAGS += -O3 -DNDEBUG
release: clean all

# Client build
client:
	$(MAKE) CLIENT=1

# Desktop build
desktop:
	$(MAKE) CLIENT=0

# Build info
info:
	@echo "Nedflix for Sega Dreamcast"
	@echo "=========================="
	@echo "Build mode: $(if $(filter 1,$(CLIENT)),CLIENT (connects to server),DESKTOP (local media))"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build current mode (CLIENT=$(CLIENT))"
	@echo "  make client   - Build client mode"
	@echo "  make desktop  - Build desktop mode"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make release  - Build optimized release"
	@echo "  make cdi      - Create CDI disc image"
	@echo "  make clean    - Clean build artifacts"

.PHONY: all clean rm-elf cdi scramble install depend run debug release client desktop info
