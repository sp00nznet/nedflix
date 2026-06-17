#!/bin/bash
#
# Nedflix Xbox 360 Build Script
#
# TECHNICAL DEMO / NOVELTY PORT
# Produces XEX file compatible with Xenia emulator and JTAG/RGH consoles.
#
# Prerequisites:
#   - libxenon toolchain (Free60 project)
#   - devkitPPC cross-compiler
#
# Installation:
#   1. Clone libxenon: git clone https://github.com/Free60Project/libxenon
#   2. Build toolchain: cd libxenon/toolchain && ./build-toolchain.sh
#   3. Set environment: export DEVKITXENON=/path/to/libxenon
#
# Usage:
#   ./build.sh              - Build XEX file for Xenia/Xbox 360
#   ./build.sh install      - Install libxenon toolchain automatically
#   ./build.sh clean        - Clean build
#   ./build.sh xex          - Build XEX only (skip ELF if exists)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# XEX header constants
XEX_MAGIC="XEX2"
XEX_MODULE_FLAGS=0x00000001  # Title module
XEX_HEADER_SIZE=0x180

# Convert path for WSL if needed
convert_path() {
    local path="$1"
    if grep -qi microsoft /proc/version 2>/dev/null; then
        # Running in WSL - check if path is Windows-style
        if [[ "$path" =~ ^[A-Za-z]: ]]; then
            # Convert Windows path to WSL path
            local drive="${path:0:1}"
            drive=$(echo "$drive" | tr '[:upper:]' '[:lower:]')
            echo "/mnt/${drive}${path:2}" | sed 's|\\|/|g'
            return
        fi
    fi
    echo "$path"
}

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}  Nedflix for Xbox 360${NC}"
echo -e "${BLUE}  XEX Build for Xenia Emulator${NC}"
echo -e "${BLUE}======================================${NC}"
echo
echo -e "${CYAN}Target: Xenia emulator / JTAG-RGH consoles${NC}"
echo

# Install libxenon toolchain
install_toolchain() {
    echo -e "${YELLOW}Installing libxenon toolchain...${NC}"
    echo
    echo "This will install the Free60 libxenon SDK to /opt/libxenon"
    echo "Requires sudo access and about 2GB of disk space."
    echo

    read -p "Continue? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Aborted."
        exit 0
    fi

    # Install dependencies
    echo -e "${YELLOW}Installing build dependencies...${NC}"
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y build-essential git wget curl \
            libgmp-dev libmpfr-dev libmpc-dev flex bison \
            texinfo libncurses5-dev
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y gcc gcc-c++ make git wget curl \
            gmp-devel mpfr-devel libmpc-devel flex bison \
            texinfo ncurses-devel
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm base-devel git wget curl \
            gmp mpfr libmpc flex bison texinfo ncurses
    else
        echo -e "${RED}Unsupported package manager. Please install dependencies manually.${NC}"
        exit 1
    fi

    # Clone libxenon
    local INSTALL_DIR="/opt/libxenon"
    if [ -d "$INSTALL_DIR" ]; then
        echo -e "${YELLOW}libxenon already exists at $INSTALL_DIR${NC}"
        read -p "Remove and reinstall? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            sudo rm -rf "$INSTALL_DIR"
        else
            echo "Using existing installation."
            export DEVKITXENON="$INSTALL_DIR"
            return 0
        fi
    fi

    echo -e "${YELLOW}Cloning libxenon...${NC}"
    sudo mkdir -p "$INSTALL_DIR"
    sudo chown $USER:$USER "$INSTALL_DIR"
    git clone --depth 1 https://github.com/Free60Project/libxenon "$INSTALL_DIR"

    # Build toolchain
    echo -e "${YELLOW}Building toolchain (this may take 30-60 minutes)...${NC}"
    cd "$INSTALL_DIR/toolchain"
    ./build-toolchain.sh

    # Set up environment
    export DEVKITXENON="$INSTALL_DIR"
    export PATH="$DEVKITXENON/bin:$PATH"

    # Add to shell profile
    local PROFILE="$HOME/.bashrc"
    if [ -f "$HOME/.zshrc" ] && [ "$SHELL" = "/bin/zsh" ]; then
        PROFILE="$HOME/.zshrc"
    fi

    if ! grep -q "DEVKITXENON" "$PROFILE" 2>/dev/null; then
        echo "" >> "$PROFILE"
        echo "# Xbox 360 libxenon toolchain" >> "$PROFILE"
        echo "export DEVKITXENON=$INSTALL_DIR" >> "$PROFILE"
        echo 'export PATH="$DEVKITXENON/bin:$PATH"' >> "$PROFILE"
        echo -e "${GREEN}Added DEVKITXENON to $PROFILE${NC}"
    fi

    cd "$SCRIPT_DIR"
    echo
    echo -e "${GREEN}Toolchain installation complete!${NC}"
    echo -e "DEVKITXENON=${DEVKITXENON}"
}

# Check for libxenon
check_libxenon() {
    if [ -z "$DEVKITXENON" ]; then
        # Try common locations
        DEVKITXENON=$(convert_path "${DEVKITXENON:-}")
        if [ -d "/opt/libxenon" ]; then
            export DEVKITXENON=/opt/libxenon
        elif [ -d "$HOME/libxenon" ]; then
            export DEVKITXENON=$HOME/libxenon
        elif [ -d "/mnt/c/libxenon" ]; then
            export DEVKITXENON=/mnt/c/libxenon
        else
            echo -e "${RED}Error: DEVKITXENON environment variable not set${NC}"
            echo
            echo "Options:"
            echo "  1. Run './build.sh install' to install toolchain automatically"
            echo "  2. Install manually:"
            echo "     git clone https://github.com/Free60Project/libxenon"
            echo "     cd libxenon/toolchain && ./build-toolchain.sh"
            echo "     export DEVKITXENON=/path/to/libxenon"
            echo
            exit 1
        fi
    else
        DEVKITXENON=$(convert_path "$DEVKITXENON")
        export DEVKITXENON
    fi

    if [ ! -d "$DEVKITXENON" ]; then
        echo -e "${RED}Error: DEVKITXENON directory not found: $DEVKITXENON${NC}"
        exit 1
    fi

    # Check for compiler
    if [ ! -f "$DEVKITXENON/bin/xenon-gcc" ]; then
        echo -e "${RED}Error: xenon-gcc not found in $DEVKITXENON/bin${NC}"
        echo "Run './build.sh install' to install the toolchain."
        exit 1
    fi

    export PATH=$DEVKITXENON/bin:$PATH

    echo -e "${GREEN}libxenon: $DEVKITXENON${NC}"
    echo -e "${GREEN}Compiler: $(xenon-gcc --version | head -1)${NC}"
}

# Create XEX header
# This creates a minimal XEX2 header that Xenia can load
create_xex_header() {
    local elf_file="$1"
    local xex_file="$2"
    local temp_dir=$(mktemp -d)

    echo -e "${YELLOW}Creating XEX file...${NC}"

    # Get ELF info
    local elf_size=$(stat -c%s "$elf_file")
    local entry_point=$(xenon-readelf -h "$elf_file" 2>/dev/null | grep "Entry point" | awk '{print $4}' || echo "0x82000000")

    # Remove 0x prefix and convert
    entry_point="${entry_point#0x}"
    entry_point=$((16#${entry_point:-82000000}))

    echo "  ELF size: $elf_size bytes"
    echo "  Entry point: 0x$(printf '%08X' $entry_point)"

    # Create XEX using Python for binary manipulation
    python3 - "$elf_file" "$xex_file" "$entry_point" << 'PYTHON_SCRIPT'
import sys
import struct
import os

elf_file = sys.argv[1]
xex_file = sys.argv[2]
entry_point = int(sys.argv[3])

# Read ELF data
with open(elf_file, 'rb') as f:
    elf_data = f.read()

# XEX2 constants
XEX2_MAGIC = b'XEX2'
XEX2_MODULE_FLAG_TITLE = 0x00000001
XEX2_HEADER_SECTION_CODE = 0x00010001
XEX2_FILE_DATA_DESCRIPTOR = 0x00000003

# Calculate sizes
code_size = len(elf_data)
# Align to 4KB page boundary
aligned_size = (code_size + 0xFFF) & ~0xFFF
header_size = 0x200  # Minimal header

# Build XEX2 header
xex_header = bytearray()

# Magic
xex_header.extend(XEX2_MAGIC)

# Module flags
xex_header.extend(struct.pack('>I', XEX2_MODULE_FLAG_TITLE))

# Data offset (where code starts)
data_offset = header_size
xex_header.extend(struct.pack('>I', data_offset))

# Reserved
xex_header.extend(struct.pack('>I', 0))

# Certificate offset (we'll use a minimal one)
cert_offset = 0x24
xex_header.extend(struct.pack('>I', cert_offset))

# Optional header count
xex_header.extend(struct.pack('>I', 3))

# Pad to certificate offset
while len(xex_header) < cert_offset:
    xex_header.append(0)

# Certificate (minimal)
# Size
xex_header.extend(struct.pack('>I', 0x1A8))
# Info flags
xex_header.extend(struct.pack('>I', 0))
# Execution info
xex_header.extend(struct.pack('>I', 0))  # Media ID
xex_header.extend(struct.pack('>I', 0x00000000))  # Version
xex_header.extend(struct.pack('>I', 0x00000000))  # Base version
xex_header.extend(struct.pack('>I', 0xFFFFFFFF))  # Title ID (homebrew)
xex_header.extend(struct.pack('>B', 0))  # Platform
xex_header.extend(struct.pack('>B', 0))  # Executable type
xex_header.extend(struct.pack('>B', 0))  # Disc number
xex_header.extend(struct.pack('>B', 0))  # Disc count

# Entry point
xex_header.extend(struct.pack('>I', entry_point))

# Image base address
image_base = 0x82000000
xex_header.extend(struct.pack('>I', image_base))

# Image size
xex_header.extend(struct.pack('>I', aligned_size))

# Load address
xex_header.extend(struct.pack('>I', image_base))

# Stack size
xex_header.extend(struct.pack('>I', 0x00040000))

# Heap size
xex_header.extend(struct.pack('>I', 0x00040000))

# Padding for minimal resources
xex_header.extend(b'\x00' * 0x148)

# Optional headers
# System flags
xex_header.extend(struct.pack('>I', 0x00030000))  # Header ID
xex_header.extend(struct.pack('>I', 0x00000000))  # System flags value

# Execution ID
xex_header.extend(struct.pack('>I', 0x00040006))  # Header ID
xex_header.extend(struct.pack('>I', 0x00000000))  # Value

# File format info
xex_header.extend(struct.pack('>I', 0x000003FF))  # Header ID
xex_header.extend(struct.pack('>I', aligned_size))  # Uncompressed size

# Pad to data offset
while len(xex_header) < data_offset:
    xex_header.append(0)

# Write XEX file
with open(xex_file, 'wb') as f:
    f.write(xex_header)
    f.write(elf_data)
    # Pad to aligned size
    padding = aligned_size - code_size
    if padding > 0:
        f.write(b'\x00' * padding)

print(f"  XEX created: {os.path.getsize(xex_file)} bytes")
PYTHON_SCRIPT

    rm -rf "$temp_dir"

    if [ -f "$xex_file" ]; then
        echo -e "${GREEN}XEX file created successfully${NC}"
        return 0
    else
        echo -e "${RED}Failed to create XEX file${NC}"
        return 1
    fi
}

# Alternative: Create XEX using xenon tools if available
create_xex_xenon() {
    local elf_file="$1"
    local xex_file="$2"

    # Check for xuxtool or xenon-xextool
    if command -v xuxtool &> /dev/null; then
        echo -e "${YELLOW}Creating XEX using xuxtool...${NC}"
        xuxtool -c "$elf_file" -o "$xex_file"
        return $?
    elif [ -f "$DEVKITXENON/bin/xenon-xextool" ]; then
        echo -e "${YELLOW}Creating XEX using xenon-xextool...${NC}"
        "$DEVKITXENON/bin/xenon-xextool" -c "$elf_file" -o "$xex_file"
        return $?
    fi

    return 1
}

# Build
build() {
    echo
    echo -e "${YELLOW}Building Nedflix for Xbox 360...${NC}"
    echo

    make clean 2>/dev/null || true
    make -j$(nproc 2>/dev/null || echo 1)

    if [ -f "nedflix.elf32" ]; then
        echo
        echo -e "${GREEN}ELF build successful!${NC}"
        echo

        # Create XEX file
        if ! create_xex_xenon "nedflix.elf32" "nedflix.xex"; then
            create_xex_header "nedflix.elf32" "nedflix.xex"
        fi

        echo
        echo -e "${GREEN}======================================${NC}"
        echo -e "${GREEN}  Build Complete!${NC}"
        echo -e "${GREEN}======================================${NC}"
        echo
        echo "Output files:"
        ls -lh nedflix.elf nedflix.elf32 nedflix.xex 2>/dev/null || ls -lh nedflix.elf nedflix.elf32
        echo
        echo -e "${CYAN}For Xenia Emulator:${NC}"
        echo "  1. Open Xenia Canary"
        echo "  2. File -> Open -> select nedflix.xex"
        echo "  3. The app should boot directly"
        echo
        echo -e "${CYAN}For Real Hardware (JTAG/RGH):${NC}"
        echo "  1. Copy nedflix.xex to USB drive"
        echo "  2. Place in: USB:/Apps/Nedflix/default.xex"
        echo "  3. Launch from dashboard or XeLL"
        echo
        echo -e "${YELLOW}Note: Real hardware requires JTAG/RGH modification${NC}"
    elif [ -f "nedflix.elf" ]; then
        echo
        echo -e "${YELLOW}ELF built but elf32 conversion failed${NC}"
        ls -lh nedflix.elf
    else
        echo -e "${RED}Build failed${NC}"
        exit 1
    fi
}

# Build XEX only (if ELF already exists)
build_xex() {
    if [ ! -f "nedflix.elf32" ]; then
        echo -e "${YELLOW}ELF file not found, running full build...${NC}"
        build
        return
    fi

    echo -e "${YELLOW}Creating XEX from existing ELF...${NC}"
    if ! create_xex_xenon "nedflix.elf32" "nedflix.xex"; then
        create_xex_header "nedflix.elf32" "nedflix.xex"
    fi

    if [ -f "nedflix.xex" ]; then
        echo -e "${GREEN}XEX created: nedflix.xex${NC}"
        ls -lh nedflix.xex
    fi
}

# Clean
clean() {
    echo -e "${YELLOW}Cleaning build...${NC}"
    make clean 2>/dev/null || true
    rm -f nedflix.xex
    echo -e "${GREEN}Clean complete${NC}"
}

# Usage
usage() {
    echo "Usage: $0 [command]"
    echo
    echo "Commands:"
    echo "  (default)   Build XEX file for Xenia emulator"
    echo "  xex         Create XEX from existing ELF"
    echo "  install     Install libxenon toolchain automatically"
    echo "  clean       Clean build artifacts"
    echo "  help        Show this help"
    echo
    echo "Environment:"
    echo "  DEVKITXENON   Path to libxenon installation"
    echo
    echo "Output:"
    echo "  nedflix.elf     PowerPC ELF executable"
    echo "  nedflix.elf32   32-bit ELF for XeLL loader"
    echo "  nedflix.xex     Xbox 360 executable for Xenia"
    echo
    echo "Requirements for build:"
    echo "  - libxenon toolchain (Free60)"
    echo "  - Python 3 (for XEX creation)"
    echo
    echo "Features (TECHNICAL DEMO):"
    echo "  - Xenos framebuffer UI (720p)"
    echo "  - Xbox 360 controller input"
    echo "  - lwIP network stack (DHCP)"
    echo "  - PCM audio streaming"
    echo
    echo "Tested with:"
    echo "  - Xenia Canary (emulator)"
    echo "  - JTAG/RGH Xbox 360 consoles"
    echo
}

# Main
main() {
    case "${1:-}" in
        ""|build)
            check_libxenon
            build
            ;;
        xex)
            check_libxenon
            build_xex
            ;;
        install)
            install_toolchain
            ;;
        clean)
            clean
            ;;
        help|--help|-h)
            usage
            ;;
        *)
            echo -e "${RED}Unknown command: $1${NC}"
            usage
            exit 1
            ;;
    esac
}

main "$@"
