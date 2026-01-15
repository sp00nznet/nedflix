#!/bin/bash
#
# Nedflix Dreamcast - Unix Build Script
# For Linux and macOS
#
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================"
echo "Nedflix Dreamcast - Unix Build Script"
echo "========================================"
echo ""
echo -e "${YELLOW}WARNING: NOVELTY BUILD - LIKELY BROKEN${NC}"
echo "The Dreamcast's limited hardware (16MB RAM, 200MHz CPU) makes"
echo "full Nedflix functionality extremely challenging. Expect issues!"
echo ""

# Function to show novelty warning
show_novelty_warning() {
    echo ""
    echo "========================================"
    echo "        NOVELTY BUILD WARNING"
    echo "========================================"
    echo "This is a proof-of-concept build for"
    echo "nostalgia purposes. The Dreamcast has:"
    echo "  - 16MB RAM (8MB main + 8MB video)"
    echo "  - 200MHz SH-4 CPU"
    echo "  - Can't decode modern video codecs"
    echo ""
    echo "Realistic expectations:"
    echo "  - Audio streaming: Probably works"
    echo "  - Low-res video (240p-360p): Maybe"
    echo "  - HD video: Absolutely not"
    echo "  - Modern codecs (H.264+): No way"
    echo "========================================"
    echo ""
}

# Function to check for KallistiOS
check_kos() {
    echo "Checking for KallistiOS toolchain..."

    # Check common KOS locations
    if [ -n "$KOS_BASE" ] && [ -f "$KOS_BASE/environ.sh" ]; then
        echo -e "${GREEN}Found KOS at: $KOS_BASE${NC}"
        return 0
    fi

    for kos_path in \
        "$HOME/kos" \
        "/opt/toolchains/dc/kos" \
        "/opt/kos" \
        "/usr/local/dc/kos"
    do
        if [ -f "$kos_path/environ.sh" ]; then
            export KOS_BASE="$kos_path"
            echo -e "${GREEN}Found KOS at: $KOS_BASE${NC}"
            return 0
        fi
    done

    echo -e "${RED}KallistiOS not found!${NC}"
    echo ""
    echo "Please install KallistiOS first:"
    echo "  1. git clone https://github.com/KallistiOS/KallistiOS.git ~/kos"
    echo "  2. cd ~/kos/utils/dc-chain"
    echo "  3. ./download.sh && ./unpack.sh && make"
    echo "  4. cd ~/kos && cp doc/environ.sh.sample environ.sh"
    echo "  5. source environ.sh && make"
    echo ""
    return 1
}

# Function to build
do_build() {
    local mode="$1"

    if [ ! -f "src/main.c" ]; then
        echo -e "${RED}ERROR: Source code not found at src/main.c${NC}"
        exit 1
    fi

    show_novelty_warning

    # Source KOS environment
    if ! check_kos; then
        exit 1
    fi
    source "$KOS_BASE/environ.sh"

    cd "$SCRIPT_DIR/src"

    echo ""
    echo "Building Nedflix for Dreamcast..."
    echo "KOS_BASE: $KOS_BASE"
    echo "Mode: $mode"
    echo ""

    case "$mode" in
        debug)
            make clean
            make debug
            ;;
        release)
            make clean
            make release
            ;;
        cdi)
            make clean
            make release
            make cdi
            ;;
        clean)
            make clean
            echo -e "${GREEN}Clean complete.${NC}"
            exit 0
            ;;
        *)
            make clean
            make
            ;;
    esac

    echo ""
    if [ -f "nedflix.elf" ]; then
        echo -e "${GREEN}========================================"
        echo "Build completed successfully!"
        echo "========================================${NC}"
        echo ""
        echo "Output files:"
        [ -f "nedflix.elf" ] && echo "  - nedflix.elf (ELF executable)"
        [ -f "nedflix.bin" ] && echo "  - nedflix.bin (Raw binary)"
        [ -f "nedflix.cdi" ] && echo "  - nedflix.cdi (CD Image)"
        echo ""
        echo "To create a bootable disc:"
        echo "  1. Use cdi4dc to create CDI: cdi4dc . nedflix.cdi"
        echo "  2. Burn to CD-R using DiscJuggler or similar"
        echo "  3. Boot Dreamcast with disc"
    else
        echo -e "${RED}Build may have failed - nedflix.elf not found${NC}"
        exit 1
    fi
}

# Function to show help
show_help() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  debug      Build debug version"
    echo "  release    Build release version"
    echo "  cdi        Build and create CDI image"
    echo "  clean      Clean build artifacts"
    echo "  help       Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 release    # Build optimized release"
    echo "  $0 cdi        # Build and package as CDI"
    echo ""
}

# Main
case "${1:-menu}" in
    debug)
        do_build debug
        ;;
    release)
        do_build release
        ;;
    cdi)
        do_build cdi
        ;;
    clean)
        do_build clean
        ;;
    help|--help|-h)
        show_help
        ;;
    menu|*)
        echo "Build Options:"
        echo "  1. Build Debug"
        echo "  2. Build Release"
        echo "  3. Create CDI Image"
        echo "  4. Clean"
        echo "  0. Exit"
        echo ""
        read -p "Enter choice (0-4): " choice

        case "$choice" in
            1) do_build debug ;;
            2) do_build release ;;
            3) do_build cdi ;;
            4) do_build clean ;;
            0) exit 0 ;;
            *) echo "Invalid choice"; exit 1 ;;
        esac
        ;;
esac
