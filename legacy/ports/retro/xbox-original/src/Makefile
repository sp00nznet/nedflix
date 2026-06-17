#
# Nedflix for Original Xbox
# nxdk Makefile
#

# nxdk environment
# NXDK_DIR should be set to the nxdk installation directory
NXDK_DIR ?= $(HOME)/nxdk

# Target name (spaces must be escaped)
XBE_TITLE = Nedflix

# Source files (use CURDIR for absolute paths as required by nxdk)
SRCS = \
	$(CURDIR)/main.c \
	$(CURDIR)/http_client.c \
	$(CURDIR)/json.c \
	$(CURDIR)/ui.c \
	$(CURDIR)/input.c \
	$(CURDIR)/video.c \
	$(CURDIR)/config.c \
	$(CURDIR)/api.c

# Build mode flags
# CLIENT=1 for client mode (connects to server)
# CLIENT=0 or unset for desktop mode (standalone)
CLIENT ?= 1

ifeq ($(CLIENT),1)
NXDK_CFLAGS += -DNEDFLIX_CLIENT_MODE=1
else
NXDK_CFLAGS += -DNEDFLIX_CLIENT_MODE=0
XBE_TITLE = Nedflix\ Desktop
endif

# Debug mode
ifdef DEBUG
NXDK_CFLAGS += -g -DDEBUG -O0
else
NXDK_CFLAGS += -O2 -DNDEBUG
endif

# Compiler flags
NXDK_CFLAGS += -Wall
NXDK_CFLAGS += -DNXDK
NXDK_CFLAGS += -I$(CURDIR)

# Enable network support (nxdk's lwIP)
NXDK_NET = y

# Enable SDL for input/graphics
NXDK_SDL = y

# Include nxdk build system
# This MUST come after all variable definitions
include $(NXDK_DIR)/Makefile
