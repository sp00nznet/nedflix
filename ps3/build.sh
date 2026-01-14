#!/bin/bash
#
# Nedflix PS3 Build Script
# Builds using PSL1GHT SDK
#
# Prerequisites:
#   - ps3toolchain: https://github.com/ps3dev/ps3toolchain
#   - PSL1GHT SDK: https://github.com/ps3dev/PSL1GHT
#   - ps3libraries: https://github.com/ps3dev/ps3libraries
#
# Environment:
#   PSL1GHT - Path to PSL1GHT installation
#   PS3DEV  - Path to ps3toolchain installation
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

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}  Nedflix PS3 Build Script${NC}"
echo -e "${BLUE}================================${NC}"
echo

# Check for PSL1GHT SDK
check_sdk() {
    echo -e "${YELLOW}Checking PSL1GHT SDK...${NC}"

    if [ -z "$PSL1GHT" ]; then
        echo -e "${RED}Error: PSL1GHT environment variable not set${NC}"
        echo
        echo "Please install PSL1GHT SDK:"
        echo "  1. Clone: git clone https://github.com/ps3dev/ps3toolchain"
        echo "  2. Build: cd ps3toolchain && ./toolchain.sh"
        echo "  3. Clone: git clone https://github.com/ps3dev/PSL1GHT"
        echo "  4. Build: cd PSL1GHT && make && make install"
        echo "  5. Set: export PSL1GHT=/path/to/psl1ght"
        echo "  6. Set: export PS3DEV=/path/to/ps3dev"
        echo
        exit 1
    fi

    if [ ! -f "$PSL1GHT/ppu_rules" ]; then
        echo -e "${RED}Error: PSL1GHT/ppu_rules not found${NC}"
        echo "PSL1GHT path: $PSL1GHT"
        exit 1
    fi

    echo -e "${GREEN}PSL1GHT found: $PSL1GHT${NC}"
}

# Check for required tools
check_tools() {
    echo -e "${YELLOW}Checking build tools...${NC}"

    local tools=("ppu-gcc" "ppu-ld" "make")
    local missing=()

    for tool in "${tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing+=("$tool")
        fi
    done

    if [ ${#missing[@]} -gt 0 ]; then
        echo -e "${RED}Missing tools: ${missing[*]}${NC}"
        echo "Please ensure PS3DEV/bin is in your PATH"
        exit 1
    fi

    echo -e "${GREEN}All tools found${NC}"
}

# Build
build() {
    echo
    echo -e "${YELLOW}Building Nedflix for PS3...${NC}"
    echo

    make clean 2>/dev/null || true
    make -j$(nproc) 2>&1

    if [ -f "nedflix.self" ]; then
        echo
        echo -e "${GREEN}Build successful!${NC}"
        echo
        echo "Output files:"
        ls -la nedflix.self 2>/dev/null || true
        ls -la nedflix.elf 2>/dev/null || true
    else
        echo -e "${RED}Build failed${NC}"
        exit 1
    fi
}

# Create PKG
create_pkg() {
    echo
    echo -e "${YELLOW}Creating PKG...${NC}"

    make pkg

    if [ -f "nedflix.pkg" ]; then
        echo -e "${GREEN}PKG created: nedflix.pkg${NC}"
    fi
}

# Usage
usage() {
    echo "Usage: $0 [command]"
    echo
    echo "Commands:"
    echo "  build    Build SELF executable (default)"
    echo "  pkg      Build and create PKG for installation"
    echo "  clean    Clean build artifacts"
    echo "  run      Build and run via ps3load"
    echo "  help     Show this help"
    echo
    echo "Environment variables:"
    echo "  PSL1GHT  Path to PSL1GHT SDK (required)"
    echo "  PS3DEV   Path to ps3toolchain (required)"
    echo
}

# Main
main() {
    case "${1:-build}" in
        build)
            check_sdk
            check_tools
            build
            ;;
        pkg)
            check_sdk
            check_tools
            build
            create_pkg
            ;;
        clean)
            echo -e "${YELLOW}Cleaning...${NC}"
            make clean 2>/dev/null || true
            echo -e "${GREEN}Done${NC}"
            ;;
        run)
            check_sdk
            check_tools
            build
            echo -e "${YELLOW}Running via ps3load...${NC}"
            make run
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
