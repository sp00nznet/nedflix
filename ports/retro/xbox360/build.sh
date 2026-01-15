#!/bin/bash
#
# Nedflix Xbox 360 Build Script
#
# TECHNICAL DEMO / NOVELTY PORT
# Requires JTAG/RGH modified console for deployment.
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
#   ./build.sh          - Build ELF file
#   ./build.sh clean    - Clean build
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
echo -e "${BLUE}  Nedflix for Xbox 360${NC}"
echo -e "${BLUE}  TECHNICAL DEMO / NOVELTY PORT${NC}"
echo -e "${BLUE}======================================${NC}"
echo
echo -e "${YELLOW}WARNING: Requires JTAG/RGH modified Xbox 360${NC}"
echo

# Check for libxenon
check_libxenon() {
    if [ -z "$DEVKITXENON" ]; then
        # Try common locations
        if [ -d "/opt/libxenon" ]; then
            export DEVKITXENON=/opt/libxenon
        elif [ -d "$HOME/libxenon" ]; then
            export DEVKITXENON=$HOME/libxenon
        else
            echo -e "${RED}Error: DEVKITXENON environment variable not set${NC}"
            echo
            echo "Please install libxenon toolchain:"
            echo "  1. git clone https://github.com/Free60Project/libxenon"
            echo "  2. cd libxenon/toolchain"
            echo "  3. ./build-toolchain.sh"
            echo "  4. export DEVKITXENON=/path/to/libxenon"
            exit 1
        fi
    fi

    if [ ! -d "$DEVKITXENON" ]; then
        echo -e "${RED}Error: DEVKITXENON directory not found: $DEVKITXENON${NC}"
        exit 1
    fi

    # Check for compiler
    if [ ! -f "$DEVKITXENON/bin/xenon-gcc" ]; then
        echo -e "${RED}Error: xenon-gcc not found in $DEVKITXENON/bin${NC}"
        echo "Please build the toolchain first."
        exit 1
    fi

    export PATH=$DEVKITXENON/bin:$PATH

    echo -e "${GREEN}libxenon: $DEVKITXENON${NC}"
    echo -e "${GREEN}Compiler: $(xenon-gcc --version | head -1)${NC}"
}

# Build
build() {
    echo
    echo -e "${YELLOW}Building Nedflix for Xbox 360...${NC}"
    echo

    make

    if [ -f "nedflix.elf32" ]; then
        echo
        echo -e "${GREEN}Build successful!${NC}"
        echo
        echo "Output files:"
        ls -lh nedflix.elf nedflix.elf32 2>/dev/null
        echo
        echo -e "${YELLOW}Deployment:${NC}"
        echo "  1. Copy nedflix.elf32 to USB drive"
        echo "  2. Rename to default.xex (or use XeLL to launch)"
        echo "  3. Boot on JTAG/RGH Xbox 360"
        echo
        echo -e "${RED}IMPORTANT: This requires a modified console!${NC}"
    elif [ -f "nedflix.elf" ]; then
        echo
        echo -e "${GREEN}Build successful (ELF only)${NC}"
        echo
        ls -lh nedflix.elf
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

# Usage
usage() {
    echo "Usage: $0 [command]"
    echo
    echo "Commands:"
    echo "  (default)   Build the ELF file"
    echo "  clean       Clean build artifacts"
    echo "  help        Show this help"
    echo
    echo "Requirements:"
    echo "  - libxenon toolchain (Free60)"
    echo "  - DEVKITXENON environment variable"
    echo
    echo "Features (TECHNICAL DEMO):"
    echo "  - Xenos framebuffer UI"
    echo "  - Xbox 360 controller input"
    echo "  - lwIP network stack"
    echo "  - Audio streaming (basic)"
    echo
    echo "Limitations:"
    echo "  - Requires JTAG/RGH modified console"
    echo "  - No access to XMA hardware decoder"
    echo "  - Limited GPU functionality in homebrew"
    echo
}

# Main
main() {
    case "${1:-}" in
        ""|build)
            check_libxenon
            build
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
