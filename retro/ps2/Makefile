# Nedflix PS2 - Makefile for PS2SDK

EE_BIN = nedflix.elf
EE_OBJS = src/main.o src/ui.o src/input.o src/audio.o src/video.o src/network.o src/api.o src/config.o src/json.o

EE_CFLAGS = -O2 -Wall -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include
EE_LDFLAGS = -L$(PS2SDK)/ee/lib
EE_LIBS = -lpatches -ldma -lgraph -ldraw -lc -lkernel

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
