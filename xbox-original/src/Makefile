#
# Nedflix for Original Xbox
# nxdk Makefile
#

# nxdk environment
# NXDK_DIR should be set to the nxdk installation directory
ifndef NXDK_DIR
$(error NXDK_DIR is not set. Please set it to your nxdk installation path)
endif

# Include nxdk environment
include $(NXDK_DIR)/lib/nxdk.mk

# Target name
XBE_TITLE = Nedflix
GEN_XISO = $(XBE_TITLE).iso

# Source files
SRCS = main.c http_client.c json.c ui.c input.c video.c config.c api.c

# Object files
OBJS = $(SRCS:.c=.obj)

# Build mode flags
# CLIENT=1 for client mode (connects to server)
# CLIENT=0 or unset for desktop mode (standalone)
CLIENT ?= 1

ifeq ($(CLIENT),1)
NXDK_CFLAGS += -DNEDFLIX_CLIENT_MODE=1
XBE_TITLE = Nedflix
else
NXDK_CFLAGS += -DNEDFLIX_CLIENT_MODE=0
XBE_TITLE = Nedflix Desktop
endif

# Debug mode
ifdef DEBUG
NXDK_CFLAGS += -g -DDEBUG -O0
else
NXDK_CFLAGS += -O2 -DNDEBUG
endif

# Compiler flags
NXDK_CFLAGS += -Wall -Wextra
NXDK_CFLAGS += -DNXDK
NXDK_CFLAGS += -I.

# nxdk libraries
# - pdclib: C standard library
# - nxdk: Xbox-specific functions
# - hal: Hardware abstraction layer
# - winapi: Windows API compatibility
# - pbkit: Low-level graphics (alternative to DirectX)
# - SDL: If using SDL for graphics/input
NXDK_LIBS = -lnxdk -lhal -lwinapi -lpdclib

# Network support (lwIP)
NXDK_CFLAGS += -DNXDK_NET
NXDK_LIBS += -llwip

# DirectSound for audio
NXDK_LIBS += -ldsound

# Default target
all: $(OUTPUT_DIR)/default.xbe

# Clean target
clean:
	$(VE)rm -f $(OBJS) $(OUTPUT_DIR)/default.xbe $(OUTPUT_DIR)/default.exe
	$(VE)rm -f *.obj *.exe *.xbe

# Build rules from nxdk.mk handle the rest
# nxdk.mk provides:
#   - Compilation rules for .c -> .obj
#   - Linking rules for .obj -> .exe
#   - XBE creation from .exe

# Individual source dependencies
main.obj: main.c nedflix.h
http_client.obj: http_client.c nedflix.h
json.obj: json.c nedflix.h
ui.obj: ui.c nedflix.h
input.obj: input.c nedflix.h
video.obj: video.c nedflix.h
config.obj: config.c nedflix.h
api.obj: api.c nedflix.h

# Package target - creates XBE with resources
package: all
	@echo "XBE package created: $(OUTPUT_DIR)/default.xbe"
	@echo ""
	@echo "To deploy to Xbox:"
	@echo "  1. FTP to Xbox (xbox:xbox@<xbox-ip>)"
	@echo "  2. Copy to E:\\Apps\\Nedflix\\"
	@echo "  3. Launch from dashboard"

# Create ISO for burning
iso: all
	@echo "Creating ISO image..."
	$(VE)$(NXDK_DIR)/tools/extract-xiso/build/extract-xiso -c $(OUTPUT_DIR) $(GEN_XISO)
	@echo "ISO created: $(GEN_XISO)"

.PHONY: all clean package iso
