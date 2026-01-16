#---------------------------------------------------------------------------------
# Nedflix for Xbox 360
# Makefile for libxenon (Free60)
#
# TECHNICAL DEMO / NOVELTY PORT
# Produces XEX file compatible with Xenia emulator and JTAG/RGH consoles
#
# Prerequisites:
#   - libxenon toolchain (https://github.com/Free60Project/libxenon)
#   - devkitPPC cross-compiler
#
# Build:
#   make          - Build ELF and XEX files
#   make elf      - Build ELF only
#   make xex      - Build XEX from ELF
#   make clean    - Clean build
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITXENON)),)
$(error "Please set DEVKITXENON in your environment. Run ./build.sh install to install toolchain.")
endif

include $(DEVKITXENON)/rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	nedflix
BUILD		:=	build
SOURCES		:=	src
INCLUDES	:=	src

#---------------------------------------------------------------------------------
# Xenia-compatible XEX settings
#---------------------------------------------------------------------------------
TITLE_ID	:=	0xFFFFFFFF
TITLE_NAME	:=	"Nedflix"
XEX_VERSION	:=	1.0.0

#---------------------------------------------------------------------------------
# options for code generation - optimized for Xenia compatibility
#---------------------------------------------------------------------------------
# Use -O2 for better performance, -g for debugging
# -ffunction-sections -fdata-sections enables dead code elimination
CFLAGS		:=	-g -O2 -Wall -Wextra $(MACHDEP) $(INCLUDE) \
			-DDEBUG \
			-DTITLE_ID=$(TITLE_ID) \
			-ffunction-sections -fdata-sections \
			-fno-strict-aliasing

CXXFLAGS	:=	$(CFLAGS)

# Linker flags for proper XEX generation
# --gc-sections removes unused code, reduces binary size
LDFLAGS		:=	-g $(MACHDEP) \
			-Wl,-Map,$(notdir $@).map \
			-Wl,--gc-sections \
			-Wl,-Ttext,0x82000000

#---------------------------------------------------------------------------------
# libraries - order matters for linking
#---------------------------------------------------------------------------------
LIBS		:=	-lxenon -lfat -lntfs -lext2fs -lm -lc

#---------------------------------------------------------------------------------
# list of directories containing libraries
#---------------------------------------------------------------------------------
LIBDIRS		:=	$(DEVKITXENON)/xenon/lib/32

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD) \
			-I$(DEVKITXENON)/xenon/include

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)) \
			-L$(DEVKITXENON)/xenon/lib/32

.PHONY: all $(BUILD) clean elf xex info

#---------------------------------------------------------------------------------
# Default target: build everything including XEX
#---------------------------------------------------------------------------------
all: $(BUILD)
	@echo ""
	@echo "Build complete! Output files:"
	@ls -la $(OUTPUT).elf $(OUTPUT).elf32 2>/dev/null || true
	@echo ""
	@echo "Run './build.sh' to create XEX for Xenia"

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# ELF only target
#---------------------------------------------------------------------------------
elf: $(BUILD)

#---------------------------------------------------------------------------------
# Clean
#---------------------------------------------------------------------------------
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).elf32 $(OUTPUT).xex $(OUTPUT).map *.bin

#---------------------------------------------------------------------------------
# Show build info
#---------------------------------------------------------------------------------
info:
	@echo "Nedflix Xbox 360 Build Configuration"
	@echo "====================================="
	@echo "DEVKITXENON: $(DEVKITXENON)"
	@echo "TARGET:      $(TARGET)"
	@echo "CFLAGS:      $(CFLAGS)"
	@echo "LDFLAGS:     $(LDFLAGS)"
	@echo "LIBS:        $(LIBS)"
	@echo ""
	@echo "Source files: $(CFILES) $(CPPFILES)"
	@echo ""

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).elf32: $(OUTPUT).elf
	@echo "Creating 32-bit ELF for XEX conversion..."
	@$(DEVKITXENON)/bin/xenon-objcopy -O elf32-powerpc $< $@ 2>/dev/null || \
		$(DEVKITXENON)/bin/xenon-objcopy -O binary $< $(OUTPUT).bin

$(OUTPUT).elf: $(OFILES)
	@echo "Linking $(notdir $@)..."
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@echo "ELF created: $@"
	@$(DEVKITXENON)/bin/xenon-size $@ 2>/dev/null || true

#---------------------------------------------------------------------------------
# Compile rules
#---------------------------------------------------------------------------------
%.o: %.c
	@echo "Compiling $(notdir $<)..."
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@echo "Compiling $(notdir $<)..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.S
	@echo "Assembling $(notdir $<)..."
	@$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
