#!/bin/bash
#
# Nedflix GameCube Build Script
#
# TECHNICAL DEMO / NOVELTY PORT
#
# Prerequisites:
#   - devkitPro with devkitPPC (https://devkitpro.org/wiki/Getting_Started)
#   - libogc, libfat
#
# Installation (Debian/Ubuntu):
#   wget https://apt.devkitpro.org/install-devkitpro-pacman
#   chmod +x install-devkitpro-pacman
#   sudo ./install-devkitpro-pacman
#   sudo dkp-pacman -S gamecube-dev
#
# Installation (macOS):
#   brew install devkitpro-pacman
#   sudo dkp-pacman -S gamecube-dev
#
# Usage:
#   ./build.sh          - Build DOL file
#   ./build.sh clean    - Clean build
#   ./build.sh run      - Run in Dolphin emulator
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}  Nedflix for Nintendo GameCube${NC}"
echo -e "${BLUE}  TECHNICAL DEMO / NOVELTY PORT${NC}"
echo -e "${BLUE}======================================${NC}"
echo

# Check for devkitPro
check_devkitpro() {
    if [ -z "$DEVKITPRO" ]; then
        # Try default location
        if [ -d "/opt/devkitpro" ]; then
            export DEVKITPRO=/opt/devkitpro
        else
            echo -e "${RED}Error: DEVKITPRO environment variable not set${NC}"
            echo
            echo "Please install devkitPro:"
            echo "  https://devkitpro.org/wiki/Getting_Started"
            echo
            echo "Then set the environment:"
            echo "  export DEVKITPRO=/opt/devkitpro"
            echo "  export DEVKITPPC=\$DEVKITPRO/devkitPPC"
            exit 1
        fi
    fi

    if [ -z "$DEVKITPPC" ]; then
        export DEVKITPPC=$DEVKITPRO/devkitPPC
    fi

    if [ ! -d "$DEVKITPPC" ]; then
        echo -e "${RED}Error: devkitPPC not found at $DEVKITPPC${NC}"
        echo
        echo "Install with:"
        echo "  sudo dkp-pacman -S gamecube-dev"
        exit 1
    fi

    # Add devkitPPC to PATH
    export PATH=$DEVKITPPC/bin:$PATH

    echo -e "${GREEN}devkitPro: $DEVKITPRO${NC}"
    echo -e "${GREEN}devkitPPC: $DEVKITPPC${NC}"

    # Check for required tools
    if ! command -v powerpc-eabi-gcc &> /dev/null; then
        echo -e "${RED}Error: powerpc-eabi-gcc not found${NC}"
        exit 1
    fi

    echo -e "${GREEN}Compiler: $(powerpc-eabi-gcc --version | head -1)${NC}"
}

# Build
build() {
    echo
    echo -e "${YELLOW}Building Nedflix for GameCube...${NC}"
    echo

    make

    if [ -f "nedflix.dol" ]; then
        echo
        echo -e "${GREEN}Build successful!${NC}"
        echo
        echo "Output files:"
        ls -lh nedflix.dol nedflix.elf 2>/dev/null
        echo
        echo -e "${YELLOW}Deployment:${NC}"
        echo "  1. Copy nedflix.dol to SD card"
        echo "  2. Create folders: /nedflix/music/ and /nedflix/audiobooks/"
        echo "  3. Add .wav audio files to those folders"
        echo "  4. Launch with SD Media Launcher, Swiss, or similar"
        echo
        echo -e "${YELLOW}Testing:${NC}"
        echo "  Run in Dolphin: ./build.sh run"
    else
        echo -e "${RED}Build failed${NC}"
        exit 1
    fi
}

# Clean
clean() {
    echo -e "${YELLOW}Cleaning build...${NC}"
    make clean
    echo -e "${GREEN}Clean complete${NC}"
}

# Run in Dolphin
run() {
    if [ ! -f "nedflix.dol" ]; then
        echo -e "${YELLOW}No DOL file found, building first...${NC}"
        build
    fi

    echo
    echo -e "${YELLOW}Launching in Dolphin emulator...${NC}"

    if command -v dolphin-emu &> /dev/null; then
        dolphin-emu -e nedflix.dol
    elif command -v dolphin-emu-qt &> /dev/null; then
        dolphin-emu-qt -e nedflix.dol
    elif [ -f "/Applications/Dolphin.app/Contents/MacOS/Dolphin" ]; then
        /Applications/Dolphin.app/Contents/MacOS/Dolphin -e nedflix.dol
    else
        echo -e "${RED}Dolphin emulator not found${NC}"
        echo "Install from: https://dolphin-emu.org"
        exit 1
    fi
}

# Usage
usage() {
    echo "Usage: $0 [command]"
    echo
    echo "Commands:"
    echo "  (default)   Build the DOL file"
    echo "  clean       Clean build artifacts"
    echo "  run         Build and run in Dolphin emulator"
    echo "  help        Show this help"
    echo
    echo "Requirements:"
    echo "  - devkitPro with gamecube-dev package"
    echo "  - DEVKITPRO and DEVKITPPC environment variables"
    echo
    echo "Features (TECHNICAL DEMO):"
    echo "  - Audio playback (WAV files only)"
    echo "  - File browser for SD card media"
    echo "  - GX-based UI rendering"
    echo "  - GameCube controller support"
    echo
    echo "Limitations:"
    echo "  - No video playback (no hardware decoder)"
    echo "  - No network (BBA is rare accessory)"
    echo "  - Limited RAM for large files"
    echo
}

# Main
main() {
    case "${1:-}" in
        ""|build)
            check_devkitpro
            build
            ;;
        clean)
            clean
            ;;
        run)
            check_devkitpro
            run
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
