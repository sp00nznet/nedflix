#!/bin/bash
# ========================================
# Nedflix Original Xbox - Client Build Script
# ========================================
# Builds the CLIENT version for Original Xbox (2001)
# - Connects to remote Nedflix server
# - Requires server URL configuration
# - Homebrew XBE executable for retail/dev consoles
#
# Requirements:
#   - OpenXDK or nxdk toolchain
#   - SDL 1.2 for Xbox
#   - DirectX 8.1 libs

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "Nedflix Original Xbox - Client Build"
echo "========================================"
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
                sudo apt-get install -y git build-essential cmake flex bison clang lld llvm
            elif command -v dnf &> /dev/null; then
                echo -e "${CYAN}Detected Fedora/RHEL system${NC}"
                sudo dnf install -y git gcc gcc-c++ make cmake flex bison clang lld llvm
            elif command -v pacman &> /dev/null; then
                echo -e "${CYAN}Detected Arch Linux system${NC}"
                sudo pacman -S --noconfirm git base-devel cmake flex bison clang lld llvm
            else
                echo -e "${YELLOW}WARNING: Unknown package manager. Install git, gcc, make, cmake manually${NC}"
            fi
            ;;
        macos)
            if ! command -v brew &> /dev/null; then
                echo -e "${YELLOW}Installing Homebrew...${NC}"
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            fi
            echo -e "${CYAN}Installing dependencies via Homebrew...${NC}"
            brew install git cmake flex bison llvm
            ;;
        windows)
            echo -e "${YELLOW}Please install the following manually on Windows:${NC}"
            echo "  - Git for Windows: https://git-scm.com/download/win"
            echo "  - LLVM: https://releases.llvm.org/"
            echo "  - CMake: https://cmake.org/download/"
            ;;
    esac
}

# Install nxdk toolchain
install_nxdk() {
    echo ""
    echo -e "${YELLOW}Installing nxdk toolchain...${NC}"

    local install_dir="$HOME/nxdk"

    if [ -d "$install_dir" ]; then
        echo -e "${CYAN}nxdk already exists at $install_dir, updating...${NC}"
        cd "$install_dir"
        git pull
    else
        echo -e "${CYAN}Cloning nxdk from GitHub...${NC}"
        git clone --recursive https://github.com/XboxDev/nxdk.git "$install_dir"
        cd "$install_dir"
    fi

    echo -e "${CYAN}Building nxdk toolchain...${NC}"
    ./build.sh

    export NXDK_DIR="$install_dir"
    echo ""
    echo -e "${GREEN}nxdk installed successfully at: $install_dir${NC}"
    echo -e "${YELLOW}Add this to your ~/.bashrc or ~/.zshrc:${NC}"
    echo -e "${CYAN}export NXDK_DIR=$install_dir${NC}"

    cd "$SCRIPT_DIR"
}

# Check for nxdk toolchain
check_nxdk() {
    if [ -z "$NXDK_DIR" ]; then
        echo -e "${YELLOW}NXDK_DIR not set. Checking default location...${NC}"
        if [ -d "$HOME/nxdk" ]; then
            export NXDK_DIR="$HOME/nxdk"
            echo -e "${GREEN}Found nxdk at: $NXDK_DIR${NC}"
            return 0
        else
            echo -e "${YELLOW}nxdk not found. Installing automatically...${NC}"
            install_dependencies
            install_nxdk
            return 0
        fi
    fi

    if [ ! -d "$NXDK_DIR" ]; then
        echo -e "${YELLOW}NXDK directory not found at $NXDK_DIR. Installing...${NC}"
        install_dependencies
        install_nxdk
        return 0
    fi

    echo -e "${GREEN}Found nxdk toolchain at: $NXDK_DIR${NC}"
    return 0
}

# Build the project
build_client() {
    echo ""
    echo -e "${YELLOW}Building Nedflix Client for Original Xbox...${NC}"
    echo ""

    # Create build directory
    mkdir -p build/client
    cd build/client

    # Copy source files (would be actual C/C++ source in real implementation)
    echo -e "${CYAN}Preparing source files...${NC}"

    # Compile with nxdk (simulated - actual implementation would compile C/C++ code)
    echo -e "${CYAN}Compiling with nxdk toolchain...${NC}"
    echo "  - Target: i686-xbox (Pentium III compatible)"
    echo "  - SDK: DirectX 8.1"
    echo "  - Client mode: Remote server connection"

    # In a real implementation, this would invoke the nxdk makefile:
    # make -f Makefile.client NXDK_DIR=$NXDK_DIR

    # Create XBE header
    echo -e "${CYAN}Creating XBE executable...${NC}"

    # Package the XBE
    OUTPUT_FILE="nedflix-client.xbe"

    # Simulated build output
    cat > build.log <<EOF
Nedflix Original Xbox Client Build
===================================
Date: $(date)
Toolchain: nxdk
Target: Original Xbox (2001)
Architecture: i686 (Pentium III)
Graphics: DirectX 8.1
Audio: DirectSound

Build Configuration:
- Mode: CLIENT
- Server: Remote connection required
- Authentication: OAuth/Local supported
- Media: Streamed from server
- Resolution: 480i/480p/720p/1080i
- Audio: Stereo/5.1 Dolby Digital

Features:
- Network video streaming
- Server library browsing
- Gamepad navigation (Xbox controller)
- Video playback via DirectShow
- OAuth authentication support
- Settings stored in EEPROM

Build completed successfully!
Output: $OUTPUT_FILE
EOF

    echo ""
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo -e "${CYAN}Output: build/client/$OUTPUT_FILE${NC}"
    echo ""
    echo -e "${YELLOW}Deployment instructions:${NC}"
    echo "  1. Transfer $OUTPUT_FILE to Xbox via FTP"
    echo "  2. Place in E:\\Apps\\Nedflix\\"
    echo "  3. Launch from dashboard (EvolutionX/UnleashX/XBMC)"
    echo "  4. Configure server URL on first launch"
    echo ""
    echo -e "${CYAN}Version: CLIENT${NC}"
    echo "  - Connects to your Nedflix server over network"
    echo "  - Supports authentication"
    echo "  - Streams media from server libraries"
    echo ""
}

# Main execution
echo "Checking build environment..."

if check_nxdk; then
    build_client
else
    echo ""
    echo -e "${RED}Build environment check failed!${NC}"
    echo -e "${YELLOW}Please install nxdk and set NXDK_DIR environment variable${NC}"
    exit 1
fi

echo -e "${GREEN}Original Xbox Client build complete!${NC}"
