#!/bin/bash
#
# Nedflix iOS Build Script
#
# Prerequisites:
#   - macOS with Xcode 15+
#   - Xcode Command Line Tools
#   - Valid Apple Developer account for device deployment
#
# Usage:
#   ./build.sh [command]
#
# Commands:
#   build       Build for simulator (default)
#   device      Build for physical device
#   archive     Create archive for App Store
#   clean       Clean build artifacts
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PROJECT="Nedflix.xcodeproj"
SCHEME="Nedflix"
CONFIGURATION="Release"
BUILD_DIR="build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}  Nedflix iOS Build Script${NC}"
echo -e "${BLUE}================================${NC}"
echo

# Check environment
check_environment() {
    echo -e "${YELLOW}Checking environment...${NC}"

    if [[ "$(uname)" != "Darwin" ]]; then
        echo -e "${RED}Error: This script requires macOS${NC}"
        exit 1
    fi

    if ! command -v xcodebuild &> /dev/null; then
        echo -e "${RED}Error: Xcode command line tools not found${NC}"
        echo "Install with: xcode-select --install"
        exit 1
    fi

    XCODE_VERSION=$(xcodebuild -version | head -1 | awk '{print $2}')
    echo -e "${GREEN}Xcode version: $XCODE_VERSION${NC}"
}

# Build for simulator
build_simulator() {
    echo
    echo -e "${YELLOW}Building for iOS Simulator...${NC}"

    xcodebuild \
        -project "$PROJECT" \
        -scheme "$SCHEME" \
        -configuration "$CONFIGURATION" \
        -destination 'platform=iOS Simulator,name=iPhone 15' \
        -derivedDataPath "$BUILD_DIR" \
        build

    echo
    echo -e "${GREEN}Build successful!${NC}"
    echo "App location: $BUILD_DIR/Build/Products/$CONFIGURATION-iphonesimulator/Nedflix.app"
}

# Build for device
build_device() {
    echo
    echo -e "${YELLOW}Building for iOS Device...${NC}"

    xcodebuild \
        -project "$PROJECT" \
        -scheme "$SCHEME" \
        -configuration "$CONFIGURATION" \
        -destination 'generic/platform=iOS' \
        -derivedDataPath "$BUILD_DIR" \
        build

    echo
    echo -e "${GREEN}Build successful!${NC}"
}

# Create archive
create_archive() {
    echo
    echo -e "${YELLOW}Creating archive...${NC}"

    ARCHIVE_PATH="$BUILD_DIR/Nedflix.xcarchive"

    xcodebuild \
        -project "$PROJECT" \
        -scheme "$SCHEME" \
        -configuration Release \
        -archivePath "$ARCHIVE_PATH" \
        archive

    echo
    echo -e "${GREEN}Archive created: $ARCHIVE_PATH${NC}"

    echo
    echo -e "${YELLOW}To export IPA for App Store:${NC}"
    echo "xcodebuild -exportArchive -archivePath $ARCHIVE_PATH -exportPath $BUILD_DIR -exportOptionsPlist ExportOptions.plist"
}

# Clean build
clean_build() {
    echo -e "${YELLOW}Cleaning build artifacts...${NC}"

    xcodebuild \
        -project "$PROJECT" \
        -scheme "$SCHEME" \
        clean

    rm -rf "$BUILD_DIR"

    echo -e "${GREEN}Clean complete${NC}"
}

# Usage
usage() {
    echo "Usage: $0 [command]"
    echo
    echo "Commands:"
    echo "  build       Build for iOS Simulator (default)"
    echo "  device      Build for physical iOS device"
    echo "  archive     Create archive for App Store submission"
    echo "  clean       Clean build artifacts"
    echo "  help        Show this help"
    echo
    echo "Requirements:"
    echo "  - macOS with Xcode 15 or later"
    echo "  - For device builds: Apple Developer account"
    echo
}

# Main
main() {
    case "${1:-build}" in
        build|simulator)
            check_environment
            build_simulator
            ;;
        device)
            check_environment
            build_device
            ;;
        archive)
            check_environment
            create_archive
            ;;
        clean)
            clean_build
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
