# Nedflix N64 - Makefile for libdragon

# Paths
N64_INST ?= /opt/libdragon

# Project
PROG_NAME = nedflix
BUILD_DIR = build

# Sources
SRCS = src/main.c src/ui.c src/input.c src/audio.c src/network.c src/api.c src/config.c src/json.c
OBJS = $(SRCS:src/%.c=$(BUILD_DIR)/%.o)

# Flags
CFLAGS = -std=gnu99 -O2 -Wall -Wextra
CFLAGS += -DN64 -DNEDFLIX_CLIENT_MODE=1

# Debug build
ifeq ($(DEBUG),1)
CFLAGS += -g -DDEBUG
endif

include $(N64_INST)/include/n64.mk

all: $(PROG_NAME).z64

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(PROG_NAME).elf: $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS) -ldragon -lc -lm -ldragonsys

$(PROG_NAME).z64: $(BUILD_DIR)/$(PROG_NAME).elf
	$(N64TOOL) -l 2M -h $(N64_INST)/mips64-elf/lib/header -o $@ $<
	$(CHKSUM64) $@

clean:
	rm -rf $(BUILD_DIR) *.z64

.PHONY: all clean
