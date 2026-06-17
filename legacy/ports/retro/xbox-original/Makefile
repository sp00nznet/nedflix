#
# Nedflix for Original Xbox
# nxdk Makefile
#
# Build with: make
# Clean with: make clean
# Build client mode: make CLIENT=1
# Build desktop mode: make CLIENT=0
#

# Output name
XBE_TITLE = Nedflix

# Build mode (1 = client mode, 0 = desktop mode)
CLIENT ?= 1

ifeq ($(CLIENT),1)
    OUTPUT_DIR = build/client
    XBE_TITLE = Nedflix-Client
    CFLAGS += -DNEDFLIX_CLIENT_MODE=1
else
    OUTPUT_DIR = build/desktop
    XBE_TITLE = Nedflix-Desktop
    CFLAGS += -DNEDFLIX_CLIENT_MODE=0
endif

# Source files
SRCS = \
    src/main.c \
    src/http_client.c \
    src/json.c \
    src/ui.c \
    src/input.c \
    src/video.c \
    src/config.c \
    src/api.c

# nxdk SDK path (set via environment or here)
NXDK_DIR ?= $(HOME)/nxdk

# Compiler flags
CFLAGS += -I$(CURDIR)/src
CFLAGS += -Wall -Wextra
CFLAGS += -O2
CFLAGS += -DNXDK

# Debug build
ifdef DEBUG
    CFLAGS += -g -DDEBUG
    CFLAGS := $(filter-out -O2,$(CFLAGS)) -O0
endif

# Libraries to link
NXDK_SDL = y
NXDK_NET = y
NXDK_CXX =

# Include nxdk build system
include $(NXDK_DIR)/Makefile

# Custom targets
.PHONY: client desktop both clean-all

client:
	$(MAKE) CLIENT=1

desktop:
	$(MAKE) CLIENT=0

both: client desktop

clean-all:
	rm -rf build/client build/desktop

# Create output directory
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Info target
info:
	@echo "Nedflix for Original Xbox"
	@echo "========================="
	@echo "Build mode: $(if $(filter 1,$(CLIENT)),CLIENT (connects to server),DESKTOP (local media))"
	@echo "Output: $(OUTPUT_DIR)/$(XBE_TITLE).xbe"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build current mode (CLIENT=$(CLIENT))"
	@echo "  make client   - Build client mode"
	@echo "  make desktop  - Build desktop mode"
	@echo "  make both     - Build both modes"
	@echo "  make clean    - Clean current mode"
	@echo "  make clean-all - Clean all builds"
	@echo ""
	@echo "Options:"
	@echo "  CLIENT=1|0    - Set build mode"
	@echo "  DEBUG=1       - Enable debug build"
