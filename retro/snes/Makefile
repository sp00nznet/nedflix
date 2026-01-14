# Nedflix SNES - Makefile for PVSnesLib

ifeq ($(strip $(PVSNESLIB_HOME)),)
$(error "Please set PVSNESLIB_HOME in your environment")
endif

include $(PVSNESLIB_HOME)/devkitsnes/snes_rules

.PHONY: all clean

CFLAGS += -I$(PVSNESLIB_HOME)/include

SOURCES = src/main.c src/ui.c src/input.c src/link.c src/config.c
OBJECTS = $(SOURCES:.c=.o)

TARGET = nedflix

all: $(TARGET).sfc

$(TARGET).sfc: $(TARGET).smc
	@cp $< $@

$(TARGET).smc: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET).smc $(TARGET).sfc
