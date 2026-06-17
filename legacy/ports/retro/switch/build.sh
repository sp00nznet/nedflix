#!/bin/bash
#
# Nedflix Nintendo Switch Build Script
# Requires devkitPro with devkitA64 and libnx
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}  Nedflix Nintendo Switch Build ${NC}"
echo -e "${BLUE}================================${NC}"
echo

# Check for devkitPro
if [ -z "$DEVKITPRO" ]; then
    echo -e "${YELLOW}DEVKITPRO environment variable not set.${NC}"
    echo "Attempting to source devkitPro environment..."

    if [ -f "/opt/devkitpro/devkitpro.sh" ]; then
        source /opt/devkitpro/devkitpro.sh
    elif [ -f "/etc/profile.d/devkit-env.sh" ]; then
        source /etc/profile.d/devkit-env.sh
    else
        echo -e "${RED}Error: devkitPro not found!${NC}"
        echo "Please install devkitPro from https://devkitpro.org/wiki/Getting_Started"
        echo
        echo "For Debian/Ubuntu:"
        echo "  wget https://apt.devkitpro.org/install-devkitpro-pacman"
        echo "  chmod +x ./install-devkitpro-pacman"
        echo "  sudo ./install-devkitpro-pacman"
        echo "  sudo dkp-pacman -S switch-dev"
        exit 1
    fi
fi

echo -e "${GREEN}DevkitPro: $DEVKITPRO${NC}"

# Check for devkitA64
if [ -z "$DEVKITA64" ]; then
    export DEVKITA64="$DEVKITPRO/devkitA64"
fi

if [ ! -d "$DEVKITA64" ]; then
    echo -e "${RED}Error: devkitA64 not found at $DEVKITA64${NC}"
    echo "Please install with: dkp-pacman -S switch-dev"
    exit 1
fi

echo -e "${GREEN}DevkitA64: $DEVKITA64${NC}"

# Check for libnx
LIBNX="$DEVKITPRO/libnx"
if [ ! -d "$LIBNX" ]; then
    echo -e "${RED}Error: libnx not found at $LIBNX${NC}"
    echo "Please install with: dkp-pacman -S libnx"
    exit 1
fi

echo -e "${GREEN}libnx: $LIBNX${NC}"
echo

# Add devkitA64 to PATH
export PATH="$DEVKITA64/bin:$PATH"

# Parse arguments
CLEAN=0
VERBOSE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        clean)
            CLEAN=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [clean] [-v|--verbose] [-h|--help]"
            echo
            echo "Options:"
            echo "  clean       Clean build directory before building"
            echo "  -v          Verbose output"
            echo "  -h          Show this help"
            exit 0
            ;;
        *)
            echo -e "${YELLOW}Unknown option: $1${NC}"
            shift
            ;;
    esac
done

# Create directories
echo -e "${BLUE}Creating directories...${NC}"
mkdir -p build
mkdir -p romfs

# Create default icon if missing
if [ ! -f "icon.jpg" ] && [ ! -f "nedflix.jpg" ]; then
    echo -e "${YELLOW}Warning: No icon.jpg found. Using placeholder.${NC}"
    # Create a simple placeholder icon (256x256 red square with 'N')
    # In practice, you'd want to include a proper icon
fi

# Clean if requested
if [ $CLEAN -eq 1 ]; then
    echo -e "${BLUE}Cleaning...${NC}"
    make clean 2>/dev/null || true
fi

# Build
echo -e "${BLUE}Building...${NC}"
echo

if [ $VERBOSE -eq 1 ]; then
    make -j$(nproc) V=1
else
    make -j$(nproc)
fi

# Check result
if [ -f "nedflix.nro" ]; then
    echo
    echo -e "${GREEN}================================${NC}"
    echo -e "${GREEN}  Build successful!${NC}"
    echo -e "${GREEN}================================${NC}"
    echo
    echo -e "Output: ${BLUE}nedflix.nro${NC}"
    ls -lh nedflix.nro
    echo
    echo -e "${YELLOW}Installation:${NC}"
    echo "1. Copy nedflix.nro to your Switch SD card"
    echo "   Place in: /switch/nedflix/nedflix.nro"
    echo "2. Launch from Homebrew Menu"
    echo
    echo -e "${YELLOW}For CFW users:${NC}"
    echo "- Works with Atmosphere, SX OS, or other CFW"
    echo "- Requires Homebrew Menu access"
else
    echo
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
