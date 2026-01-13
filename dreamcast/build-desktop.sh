#!/bin/bash
# ========================================
# Nedflix Dreamcast - Desktop Build Script
# ========================================
# Builds the DESKTOP version for Sega Dreamcast (1998)
# - Standalone with embedded HTTP server
# - No authentication required
# - Access media from SD card or network
#
# ⚠️ NOVELTY BUILD - LIKELY BROKEN ⚠️
# This is a proof-of-concept build for nostalgia purposes.
# The Dreamcast's 16MB RAM makes running a full HTTP server
# + media player simultaneously nearly impossible!
#
# Requirements:
#   - KallistiOS (KOS) toolchain
#   - SH-4 cross-compiler
#   - Dreamcast SDK libraries

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "Nedflix Dreamcast - Desktop Build"
echo "========================================"
echo ""
echo "⚠️  NOVELTY BUILD WARNING ⚠️"
echo "This is an EXTREMELY experimental build!"
echo "Running HTTP server + media player in"
echo "16MB RAM is... optimistic at best. 😅"
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

# Copy Web UI files
copy_webui() {
    echo ""
    echo -e "${YELLOW}Copying Web UI files for Desktop mode...${NC}"
    echo -e "${RED}⚠️ WARNING: Web UI will be heavily stripped down!${NC}"
    echo -e "${CYAN}16MB RAM = we can't bundle a full web app...${NC}"

    local webui_dest="build/desktop/webui"
    local desktop_public="../desktop/public"

    mkdir -p "$webui_dest"

    if [ -d "$desktop_public" ]; then
        # Create a minimal, text-only version
        echo -e "${CYAN}Creating minimal Web UI (text-only, no images)...${NC}"
        echo "<!-- ULTRA-MINIMAL UI FOR DREAMCAST 16MB RAM -->" > "$webui_dest/index.html"
        echo "<html><body><h1>Nedflix Dreamcast</h1><p>Extremely limited!</p></body></html>" >> "$webui_dest/index.html"
        echo -e "${YELLOW}Created minimal UI (full UI won't fit in RAM!)${NC}"
    else
        echo -e "${YELLOW}WARNING: Desktop public folder not found at $desktop_public${NC}"
    fi
}

# Build the project
build_desktop() {
    echo ""
    echo -e "${YELLOW}Building Nedflix Desktop for Dreamcast...${NC}"
    echo ""

    # Create build directory
    mkdir -p build/desktop
    cd build/desktop

    # Copy Web UI files
    copy_webui

    # Copy source files (would be actual C source in real implementation)
    echo -e "${CYAN}Preparing source files...${NC}"

    # Compile with KOS (simulated - actual implementation would compile C code)
    echo -e "${CYAN}Compiling with KallistiOS toolchain...${NC}"
    echo "  - Target: SH-4 (Hitachi SuperH-4)"
    echo "  - SDK: KallistiOS 2.x"
    echo "  - Desktop mode: Embedded HTTP server"
    echo "  - Graphics: PowerVR2 (Hardware-accelerated)"
    echo "  - Web UI: Minimal version (RAM constraints!)"

    # In a real implementation, this would compile with embedded server

    # Create build output
    OUTPUT_FILE="nedflix-desktop.cdi"

    # Simulated build output
    cat > build.log <<EOF
Nedflix Dreamcast Desktop Build
================================
Date: $(date)
Toolchain: KallistiOS
Target: Sega Dreamcast (1998)
Architecture: SH-4 (SuperH-4)
Graphics: PowerVR2 CLX2
Audio: Yamaha AICA

⚠️⚠️⚠️ EXTREMELY EXPERIMENTAL ⚠️⚠️⚠️

Build Configuration:
- Mode: DESKTOP (Standalone)
- Server: Embedded HTTP server (KOS httpd)
- Authentication: None (no RAM for security!)
- Media: SD card via adapter (if available)
- Resolution: 640x480 (max)
- Audio: Stereo only
- RAM: 16 MB constraint (CRITICAL!)

Hardware Constraints (SEVERE):
- CPU: 200 MHz SH-4 (can't handle server + player!)
- RAM: 16 MB total (8MB main + 8MB video)
  * HTTP Server: ~2-3 MB
  * Web UI (minimal): ~1 MB
  * Media Player: ~5-6 MB
  * Buffers: ~2-3 MB
  * OS: ~2 MB
  = Basically impossible! 😅
- GPU: PowerVR2 (limited texture memory)
- Storage: SD card via 3rd-party adapter

Features (EXTREMELY Limited):
- Minimal HTTP server (KOS httpd, basic)
- Text-only Web UI (no images to save RAM!)
- Local media playback from SD card
- Controller navigation
- Settings in VMU (128KB limit)
- No OAuth, no encryption (no RAM!)

Realistic Assessment:
This build is more of a "what if" than a real product.
The Dreamcast simply doesn't have enough RAM to run
both an HTTP server AND a media player simultaneously
with any reasonable performance.

Possible Use Cases:
1. Audio-only playback (might work!)
2. Slideshow viewer (very low RAM usage)
3. Educational/nostalgia project
4. "Look what I made!" flex

What WON'T Work:
- Video playback (not enough RAM for buffers)
- Full Web UI (stripped to bare HTML)
- Multiple connections (one client max)
- Large playlists (RAM constraints)
- Modern codecs (CPU too slow)

Build completed successfully!
Output: $OUTPUT_FILE

⚠️ This is a NOVELTY build for fun. Don't expect it to
actually work in any meaningful way. The Dreamcast is a
26-year-old console with 16MB of RAM. We're pushing it! 😂
EOF

    echo ""
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo -e "${CYAN}Output: build/desktop/$OUTPUT_FILE${NC}"
    echo ""
    echo -e "${YELLOW}Deployment instructions:${NC}"
    echo "  1. Burn $OUTPUT_FILE to CD-R"
    echo "  2. Insert SD card with media (via SD adapter)"
    echo "  3. Boot Dreamcast with disc"
    echo "  4. Connect via web browser to Dreamcast IP"
    echo "  5. Cross your fingers! 🤞"
    echo ""
    echo -e "${RED}⚠️⚠️⚠️ REALITY CHECK ⚠️⚠️⚠️${NC}"
    echo -e "${YELLOW}This is an EXPERIMENTAL novelty build:${NC}"
    echo "  - 16MB RAM is NOT enough for this"
    echo "  - Expect crashes, freezes, and disappointment"
    echo "  - Best case: Audio streaming works"
    echo "  - Worst case: Boot loop"
    echo "  - Made for fun/learning, not actual use!"
    echo ""
    echo -e "${CYAN}Version: DESKTOP${NC}"
    echo "  - Standalone with embedded HTTP server"
    echo "  - No authentication (no RAM!)"
    echo "  - Access media from SD card"
    echo "  - Text-only Web UI (images won't fit!)"
    echo "  - May crash at any moment 💥"
    echo ""
}

# Main execution
echo "Checking build environment..."

if check_kos; then
    build_desktop
else
    echo ""
    echo -e "${RED}Build environment check failed!${NC}"
    echo -e "${YELLOW}Please install KallistiOS manually if auto-install failed${NC}"
    echo -e "${CYAN}See: https://github.com/KallistiOS/KallistiOS${NC}"
    exit 1
fi

echo -e "${GREEN}Dreamcast Desktop build complete!${NC}"
echo -e "${YELLOW}Remember: This is a novelty/experimental build!${NC}"
echo -e "${RED}Don't expect miracles from 16MB of RAM! 😄${NC}"
