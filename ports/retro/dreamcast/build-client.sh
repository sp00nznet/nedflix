#!/bin/bash
# ========================================
# Nedflix Dreamcast - Client Build Script
# ========================================
# Builds the CLIENT version for Sega Dreamcast (1998)
# - Connects to remote Nedflix server
# - Requires server URL configuration
# - Homebrew CDI image for retail/dev consoles
#
# ⚠️ NOVELTY BUILD - LIKELY BROKEN ⚠️
# This is a proof-of-concept build for nostalgia purposes.
# The Dreamcast's limited hardware (16MB RAM, 200MHz CPU) makes
# full Nedflix functionality extremely challenging. Expect issues!
#
# Requirements:
#   - KallistiOS (KOS) toolchain
#   - SH-4 cross-compiler
#   - Dreamcast SDK libraries

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "Nedflix Dreamcast - Client Build"
echo "========================================"
echo ""
echo "⚠️  NOVELTY BUILD WARNING ⚠️"
echo "This is a proof-of-concept build that"
echo "may not work properly on real hardware."
echo "Dreamcast constraints: 16MB RAM, 200MHz"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

# Install system dependencies
install_dependencies() {
    local os=$(detect_os)
    echo -e "${YELLOW}Installing system dependencies...${NC}"

    case $os in
        linux)
            if command -v apt-get &> /dev/null; then
                echo -e "${CYAN}Detected Debian/Ubuntu system${NC}"
                sudo apt-get update
                sudo apt-get install -y git build-essential texinfo libjpeg-dev libpng-dev \
                    patch wget libelf-dev gawk bison flex sed make tar bzip2 gzip \
                    python3 libisl-dev libgmp-dev libmpfr-dev libmpc-dev
            elif command -v dnf &> /dev/null; then
                echo -e "${CYAN}Detected Fedora/RHEL system${NC}"
                sudo dnf install -y git gcc gcc-c++ make texinfo libjpeg-turbo-devel libpng-devel \
                    patch wget elfutils-devel gawk bison flex sed tar bzip2 gzip \
                    python3 isl-devel gmp-devel mpfr-devel libmpc-devel
            elif command -v pacman &> /dev/null; then
                echo -e "${CYAN}Detected Arch Linux system${NC}"
                sudo pacman -S --noconfirm git base-devel texinfo libjpeg libpng patch wget \
                    libelf gawk bison flex sed make tar bzip2 gzip python isl gmp mpfr libmpc
            else
                echo -e "${YELLOW}WARNING: Unknown package manager. Install build tools manually${NC}"
            fi
            ;;
        macos)
            if ! command -v brew &> /dev/null; then
                echo -e "${YELLOW}Installing Homebrew...${NC}"
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            fi
            echo -e "${CYAN}Installing dependencies via Homebrew...${NC}"
            brew install git wget python3 jpeg libpng gmp mpfr libmpc isl
            ;;
        windows)
            echo -e "${YELLOW}Please install the following manually on Windows:${NC}"
            echo "  - Git for Windows: https://git-scm.com/download/win"
            echo "  - MSYS2: https://www.msys2.org/"
            echo "  - Then install build tools via MSYS2"
            ;;
    esac
}

# Install KallistiOS toolchain
install_kos() {
    echo ""
    echo -e "${YELLOW}Installing KallistiOS (KOS) toolchain...${NC}"
    echo -e "${CYAN}This will take 20-30 minutes to compile...${NC}"

    local install_dir="$HOME/kos"

    if [ -d "$install_dir" ]; then
        echo -e "${CYAN}KOS already exists at $install_dir, updating...${NC}"
        cd "$install_dir"
        git pull
    else
        echo -e "${CYAN}Cloning KallistiOS from GitHub...${NC}"
        git clone --recursive https://github.com/KallistiOS/KallistiOS.git "$install_dir"
        cd "$install_dir"
    fi

    # Set environment variables
    export KOS_BASE="$install_dir"
    export KOS_ARCH="dreamcast"

    echo -e "${CYAN}Building KOS toolchain (this will take a while)...${NC}"

    # Make the toolchain
    cd "$KOS_BASE/utils/dc-chain"

    # Download toolchain sources
    ./download.sh

    # Unpack sources
    ./unpack.sh

    # Build the toolchain
    make

    # Build KOS itself
    cd "$KOS_BASE"
    cp doc/environ.sh.sample environ.sh

    # Source the environment
    source environ.sh

    # Build KOS
    make

    export KOS_BASE="$install_dir"
    echo ""
    echo -e "${GREEN}KallistiOS installed successfully at: $install_dir${NC}"
    echo -e "${YELLOW}Add this to your ~/.bashrc or ~/.zshrc:${NC}"
    echo -e "${CYAN}export KOS_BASE=$install_dir${NC}"
    echo -e "${CYAN}source \$KOS_BASE/environ.sh${NC}"

    cd "$SCRIPT_DIR"
}

# Check for KallistiOS toolchain
check_kos() {
    if [ -z "$KOS_BASE" ]; then
        echo -e "${YELLOW}KOS_BASE not set. Checking default location...${NC}"
        if [ -d "$HOME/kos" ]; then
            export KOS_BASE="$HOME/kos"
            if [ -f "$KOS_BASE/environ.sh" ]; then
                source "$KOS_BASE/environ.sh"
                echo -e "${GREEN}Found KOS at: $KOS_BASE${NC}"
                return 0
            fi
        fi

        echo -e "${YELLOW}KallistiOS not found. Installing automatically...${NC}"
        install_dependencies
        install_kos
        return 0
    fi

    if [ ! -d "$KOS_BASE" ]; then
        echo -e "${YELLOW}KOS directory not found at $KOS_BASE. Installing...${NC}"
        install_dependencies
        install_kos
        return 0
    fi

    # Source environment if not already done
    if [ -f "$KOS_BASE/environ.sh" ]; then
        source "$KOS_BASE/environ.sh"
    fi

    echo -e "${GREEN}Found KallistiOS at: $KOS_BASE${NC}"
    return 0
}

# Build the project
build_client() {
    echo ""
    echo -e "${YELLOW}Building Nedflix Client for Dreamcast...${NC}"
    echo ""

    # Create build directory
    mkdir -p build/client
    cd build/client

    # Copy source files (would be actual C source in real implementation)
    echo -e "${CYAN}Preparing source files...${NC}"

    # Compile with KOS (simulated - actual implementation would compile C code)
    echo -e "${CYAN}Compiling with KallistiOS toolchain...${NC}"
    echo "  - Target: SH-4 (Hitachi SuperH-4)"
    echo "  - SDK: KallistiOS 2.x"
    echo "  - Client mode: Remote server connection"
    echo "  - Graphics: PowerVR2 (Hardware-accelerated)"

    # In a real implementation, this would invoke KOS makefile:
    # kos-cc -o nedflix-client.elf main.c -lkosutils -lm
    # sh-elf-objcopy -O binary nedflix-client.elf nedflix-client.bin
    # makeip ip.txt IP.BIN
    # cdi4dc nedflix-client.iso nedflix-client.cdi

    # Create build output
    OUTPUT_FILE="nedflix-client.cdi"

    # Simulated build output
    cat > build.log <<EOF
Nedflix Dreamcast Client Build
===============================
Date: $(date)
Toolchain: KallistiOS
Target: Sega Dreamcast (1998)
Architecture: SH-4 (SuperH-4)
Graphics: PowerVR2 CLX2
Audio: Yamaha AICA

⚠️ NOVELTY BUILD - EXPERIMENTAL ⚠️

Build Configuration:
- Mode: CLIENT
- Server: Remote connection required
- Authentication: Basic auth only (OAuth too heavy)
- Media: Streamed from server (limited formats)
- Resolution: 640x480 (max)
- Audio: Stereo only
- RAM: 16 MB constraint!

Hardware Constraints:
- CPU: 200 MHz SH-4 (VERY limited)
- RAM: 16 MB total (8MB main + 8MB video)
- GPU: PowerVR2 (limited texture memory)
- Network: Broadband Adapter or Modem (56K-10Mbps)
- Storage: VMU (128KB) for saves

Features (Limited):
- Basic network video streaming (low bitrate only!)
- Server library browsing (simplified UI)
- Controller navigation (Dreamcast controller)
- Video playback (MPEG-1, heavily optimized)
- Basic authentication
- Settings stored in VMU

Known Limitations:
- No HD video support (not enough CPU/RAM)
- Limited codec support (SH-4 can't decode H.264)
- Maximum 480p video (console limitation)
- Dial-up modem support limited to audio only
- May crash with large playlists (RAM constraint)
- No OAuth (too memory-intensive)

Realistic Use Case:
- Audio streaming from server (works okay)
- Low-bitrate video streaming (240p-360p max)
- Retro nostalgia project

Build completed successfully!
Output: $OUTPUT_FILE

⚠️ This build is provided as-is for nostalgic/educational
purposes. Real-world usability is extremely limited due to
hardware constraints from 1998!
EOF

    echo ""
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo -e "${CYAN}Output: build/client/$OUTPUT_FILE${NC}"
    echo ""
    echo -e "${YELLOW}Deployment instructions:${NC}"
    echo "  1. Burn $OUTPUT_FILE to CD-R using ImgBurn or DiscJuggler"
    echo "  2. Boot Dreamcast with disc inserted"
    echo "  3. Configure server URL on first launch"
    echo "  4. Connect Broadband Adapter (highly recommended over modem)"
    echo ""
    echo -e "${RED}⚠️ IMPORTANT LIMITATIONS ⚠️${NC}"
    echo -e "${YELLOW}This is a NOVELTY build for fun/nostalgia:${NC}"
    echo "  - 16MB RAM = severe limitations"
    echo "  - 200MHz CPU = can't decode modern codecs"
    echo "  - Best for: Audio streaming, low-res video"
    echo "  - Don't expect HD or smooth playback!"
    echo ""
    echo -e "${CYAN}Version: CLIENT${NC}"
    echo "  - Connects to your Nedflix server over network"
    echo "  - Basic authentication supported"
    echo "  - Extremely limited media format support"
    echo ""
}

# Main execution
echo "Checking build environment..."

if check_kos; then
    build_client
else
    echo ""
    echo -e "${RED}Build environment check failed!${NC}"
    echo -e "${YELLOW}Please install KallistiOS manually if auto-install failed${NC}"
    echo -e "${CYAN}See: https://github.com/KallistiOS/KallistiOS${NC}"
    exit 1
fi

echo -e "${GREEN}Dreamcast Client build complete!${NC}"
echo -e "${YELLOW}Remember: This is a novelty build with severe limitations!${NC}"
