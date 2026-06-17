#!/bin/bash
#
# Nedflix Original Xbox - Unix Build Script
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
echo "Nedflix Original Xbox - Unix Build Script"
echo "========================================"
echo ""

# Function to show help
show_help() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  client       Build client version (connects to server)"
    echo "  desktop      Build desktop version (standalone)"
    echo "  debug        Build debug version (client)"
    echo "  release      Build release version (client)"
    echo "  clean        Clean build artifacts"
    echo "  install      Install nxdk toolchain"
    echo "  help         Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 client    # Build client release"
    echo "  $0 debug     # Build client debug"
    echo "  $0 desktop   # Build desktop release"
    echo ""
}

# Function to check for nxdk
check_nxdk() {
    echo "Checking for nxdk toolchain..."

    # Check environment variable first
    if [ -n "$NXDK_DIR" ] && [ -d "$NXDK_DIR" ]; then
        echo -e "${GREEN}Found nxdk at: $NXDK_DIR${NC}"
        return 0
    fi

    # Check common locations
    for nxdk_path in \
        "$HOME/nxdk" \
        "/opt/nxdk" \
        "/usr/local/nxdk"
    do
        if [ -d "$nxdk_path" ]; then
            export NXDK_DIR="$nxdk_path"
            echo -e "${GREEN}Found nxdk at: $NXDK_DIR${NC}"
            return 0
        fi
    done

    echo -e "${RED}nxdk toolchain not found!${NC}"
    echo ""
    echo "Please install nxdk first:"
    echo "  1. git clone --recursive https://github.com/XboxDev/nxdk.git ~/nxdk"
    echo "  2. cd ~/nxdk && ./build.sh"
    echo "  3. export NXDK_DIR=~/nxdk"
    echo ""
    echo "Or run: $0 install"
    echo ""
    return 1
}

# Function to install nxdk
install_nxdk() {
    echo "Installing nxdk toolchain..."
    echo ""

    local install_dir="$HOME/nxdk"

    # Install dependencies based on OS
    if command -v apt-get &> /dev/null; then
        echo "Installing dependencies (Debian/Ubuntu)..."
        sudo apt-get update
        sudo apt-get install -y git build-essential cmake flex bison clang lld llvm
    elif command -v dnf &> /dev/null; then
        echo "Installing dependencies (Fedora/RHEL)..."
        sudo dnf install -y git gcc gcc-c++ make cmake flex bison clang lld llvm
    elif command -v pacman &> /dev/null; then
        echo "Installing dependencies (Arch)..."
        sudo pacman -S --noconfirm git base-devel cmake flex bison clang lld llvm
    elif command -v brew &> /dev/null; then
        echo "Installing dependencies (macOS)..."
        brew install cmake flex bison llvm
    else
        echo -e "${YELLOW}WARNING: Unknown package manager.${NC}"
        echo "Please install manually: git, cmake, flex, bison, clang, lld, llvm"
    fi

    # Clone nxdk
    if [ -d "$install_dir" ]; then
        echo "Updating existing nxdk..."
        cd "$install_dir"
        git pull
    else
        echo "Cloning nxdk..."
        git clone --recursive https://github.com/XboxDev/nxdk.git "$install_dir"
        cd "$install_dir"
    fi

    # Build nxdk
    echo ""
    echo "Building nxdk toolchain..."
    ./build.sh

    export NXDK_DIR="$install_dir"

    echo ""
    echo -e "${GREEN}nxdk installed at: $NXDK_DIR${NC}"
    echo ""
    echo "Add this to your shell config (~/.bashrc or ~/.zshrc):"
    echo "  export NXDK_DIR=$NXDK_DIR"
    echo ""

    cd "$SCRIPT_DIR"
}

# Function to build
do_build() {
    local mode="$1"
    local build_type="$2"

    if [ ! -f "src/main.c" ]; then
        echo -e "${RED}ERROR: Source code not found at src/main.c${NC}"
        exit 1
    fi

    if ! check_nxdk; then
        exit 1
    fi

    cd "$SCRIPT_DIR/src"

    echo ""
    echo "Building Nedflix for Original Xbox..."
    echo "NXDK_DIR: $NXDK_DIR"
    echo "Mode: $mode"
    echo "Type: $build_type"
    echo ""

    # Set CLIENT flag
    local client_flag="CLIENT=1"
    if [ "$mode" = "desktop" ]; then
        client_flag="CLIENT=0"
    fi

    # Set DEBUG flag
    local debug_flag=""
    if [ "$build_type" = "debug" ]; then
        debug_flag="DEBUG=1"
    fi

    # Clean and build
    make clean 2>/dev/null || true
    make $client_flag $debug_flag

    echo ""
    if [ -f "default.xbe" ]; then
        echo -e "${GREEN}========================================"
        echo "Build completed successfully!"
        echo "========================================${NC}"
        echo ""
        echo "Output: src/default.xbe"
        echo ""
        echo "Deployment instructions:"
        echo "  1. FTP the .xbe to your Xbox"
        echo "  2. Place in E:\\Apps\\Nedflix\\"
        echo "  3. Launch from dashboard"
    else
        echo -e "${RED}Build may have failed - default.xbe not found${NC}"
        exit 1
    fi
}

# Main
case "${1:-menu}" in
    client)
        do_build client release
        ;;
    desktop)
        do_build desktop release
        ;;
    debug)
        do_build client debug
        ;;
    release)
        do_build client release
        ;;
    clean)
        echo "Cleaning build artifacts..."
        cd "$SCRIPT_DIR/src"
        make clean 2>/dev/null || true
        echo -e "${GREEN}Clean complete.${NC}"
        ;;
    install)
        install_nxdk
        ;;
    help|--help|-h)
        show_help
        ;;
    menu|*)
        echo "Build Options:"
        echo "  1. Build Client (Release)"
        echo "  2. Build Client (Debug)"
        echo "  3. Build Desktop (Release)"
        echo "  4. Build Desktop (Debug)"
        echo "  5. Clean"
        echo "  6. Install nxdk"
        echo "  0. Exit"
        echo ""
        read -p "Enter choice (0-6): " choice

        case "$choice" in
            1) do_build client release ;;
            2) do_build client debug ;;
            3) do_build desktop release ;;
            4) do_build desktop debug ;;
            5)
                cd "$SCRIPT_DIR/src"
                make clean 2>/dev/null || true
                echo -e "${GREEN}Clean complete.${NC}"
                ;;
            6) install_nxdk ;;
            0) exit 0 ;;
            *) echo "Invalid choice"; exit 1 ;;
        esac
        ;;
esac
