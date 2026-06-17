#!/bin/bash
#
# Nedflix Android Build Script
#
# Prerequisites:
#   - Java 17 or later
#   - Android SDK (via Android Studio or command line tools)
#   - ANDROID_HOME environment variable set
#
# Usage:
#   ./build.sh [command]
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
echo -e "${BLUE}  Nedflix Android Build Script${NC}"
echo -e "${BLUE}================================${NC}"
echo

# Check environment
check_environment() {
    echo -e "${YELLOW}Checking environment...${NC}"

    # Check Java
    if ! command -v java &> /dev/null; then
        echo -e "${RED}Error: Java not found${NC}"
        echo "Please install Java 17 or later"
        exit 1
    fi

    JAVA_VERSION=$(java -version 2>&1 | head -1 | cut -d'"' -f2 | cut -d'.' -f1)
    echo -e "${GREEN}Java version: $JAVA_VERSION${NC}"

    if [ "$JAVA_VERSION" -lt 17 ]; then
        echo -e "${RED}Error: Java 17 or later required${NC}"
        exit 1
    fi

    # Check Android SDK
    if [ -z "$ANDROID_HOME" ]; then
        # Try common locations
        if [ -d "$HOME/Android/Sdk" ]; then
            export ANDROID_HOME="$HOME/Android/Sdk"
        elif [ -d "$HOME/Library/Android/sdk" ]; then
            export ANDROID_HOME="$HOME/Library/Android/sdk"
        else
            echo -e "${RED}Error: ANDROID_HOME not set and Android SDK not found${NC}"
            echo "Install Android Studio or set ANDROID_HOME manually"
            exit 1
        fi
    fi

    echo -e "${GREEN}Android SDK: $ANDROID_HOME${NC}"
}

# Download Gradle wrapper if missing
setup_gradle() {
    if [ ! -f "gradlew" ]; then
        echo -e "${YELLOW}Setting up Gradle wrapper...${NC}"

        # Create gradle wrapper
        mkdir -p gradle/wrapper

        cat > gradle/wrapper/gradle-wrapper.properties << 'EOF'
distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-8.4-bin.zip
networkTimeout=10000
validateDistributionUrl=true
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
EOF

        # Download gradle wrapper jar
        WRAPPER_URL="https://raw.githubusercontent.com/gradle/gradle/v8.4.0/gradle/wrapper/gradle-wrapper.jar"
        curl -sL "$WRAPPER_URL" -o gradle/wrapper/gradle-wrapper.jar 2>/dev/null || {
            echo -e "${YELLOW}Note: Could not download gradle-wrapper.jar${NC}"
            echo "Run 'gradle wrapper' or use Android Studio to set up the project"
        }

        # Create gradlew script
        cat > gradlew << 'GRADLEW'
#!/bin/sh
exec java -jar "$0/../gradle/wrapper/gradle-wrapper.jar" "$@"
GRADLEW
        chmod +x gradlew
    fi
}

# Build debug APK
build_debug() {
    echo
    echo -e "${YELLOW}Building debug APK...${NC}"

    ./gradlew assembleDebug

    APK_PATH="app/build/outputs/apk/debug/app-debug.apk"
    if [ -f "$APK_PATH" ]; then
        echo
        echo -e "${GREEN}Build successful!${NC}"
        echo "APK: $APK_PATH"
        ls -lh "$APK_PATH"
    else
        echo -e "${RED}Build failed${NC}"
        exit 1
    fi
}

# Build release APK
build_release() {
    echo
    echo -e "${YELLOW}Building release APK...${NC}"

    ./gradlew assembleRelease

    APK_PATH="app/build/outputs/apk/release/app-release-unsigned.apk"
    if [ -f "$APK_PATH" ]; then
        echo
        echo -e "${GREEN}Build successful!${NC}"
        echo "APK: $APK_PATH"
        echo
        echo -e "${YELLOW}Note: Release APK is unsigned${NC}"
        echo "Sign with: apksigner sign --ks keystore.jks $APK_PATH"
    fi
}

# Build AAB (Android App Bundle)
build_bundle() {
    echo
    echo -e "${YELLOW}Building Android App Bundle...${NC}"

    ./gradlew bundleRelease

    AAB_PATH="app/build/outputs/bundle/release/app-release.aab"
    if [ -f "$AAB_PATH" ]; then
        echo
        echo -e "${GREEN}Build successful!${NC}"
        echo "AAB: $AAB_PATH"
    fi
}

# Install on connected device
install_debug() {
    echo
    echo -e "${YELLOW}Installing debug APK...${NC}"

    ./gradlew installDebug

    echo -e "${GREEN}Installation complete${NC}"
}

# Clean build
clean_build() {
    echo -e "${YELLOW}Cleaning build...${NC}"
    ./gradlew clean
    echo -e "${GREEN}Clean complete${NC}"
}

# Run tests
run_tests() {
    echo
    echo -e "${YELLOW}Running tests...${NC}"
    ./gradlew test
    echo -e "${GREEN}Tests complete${NC}"
}

# Usage
usage() {
    echo "Usage: $0 [command]"
    echo
    echo "Commands:"
    echo "  debug       Build debug APK (default)"
    echo "  release     Build release APK (unsigned)"
    echo "  bundle      Build Android App Bundle for Play Store"
    echo "  install     Build and install debug APK on device"
    echo "  test        Run unit tests"
    echo "  clean       Clean build artifacts"
    echo "  help        Show this help"
    echo
    echo "Requirements:"
    echo "  - Java 17 or later"
    echo "  - Android SDK (ANDROID_HOME)"
    echo
    echo "For signing release builds, create a keystore:"
    echo "  keytool -genkey -v -keystore keystore.jks -keyalg RSA -keysize 2048 -validity 10000 -alias nedflix"
    echo
}

# Main
main() {
    case "${1:-debug}" in
        debug)
            check_environment
            setup_gradle
            build_debug
            ;;
        release)
            check_environment
            setup_gradle
            build_release
            ;;
        bundle|aab)
            check_environment
            setup_gradle
            build_bundle
            ;;
        install)
            check_environment
            setup_gradle
            install_debug
            ;;
        test)
            check_environment
            setup_gradle
            run_tests
            ;;
        clean)
            setup_gradle
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
