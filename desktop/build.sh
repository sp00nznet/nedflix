#!/bin/bash
# ========================================
# Nedflix Desktop - Build Script
# Supports Linux and macOS
# ========================================

set -e

echo "========================================"
echo "Nedflix Desktop - Build Script"
echo "========================================"
echo ""

# Navigate to script directory
cd "$(dirname "$0")"
echo "Working directory: $(pwd)"
echo ""

# ========================================
# Function to install Node.js
# ========================================
install_nodejs() {
    echo "----------------------------------------"
    echo "Installing Node.js..."
    echo "----------------------------------------"

    # Detect OS
    OS="$(uname -s)"

    case "$OS" in
        Darwin)
            # macOS
            if command -v brew &> /dev/null; then
                echo "Installing via Homebrew..."
                brew install node@20
                brew link node@20 --force --overwrite 2>/dev/null || true
            else
                echo "Homebrew not found. Installing Homebrew first..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

                # Add Homebrew to PATH for Apple Silicon
                if [[ $(uname -m) == "arm64" ]]; then
                    eval "$(/opt/homebrew/bin/brew shellenv)"
                fi

                brew install node@20
                brew link node@20 --force --overwrite 2>/dev/null || true
            fi
            ;;
        Linux)
            # Linux - Detect package manager
            if command -v apt-get &> /dev/null; then
                # Debian/Ubuntu
                echo "Detected Debian/Ubuntu system"
                curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
                sudo apt-get install -y nodejs

            elif command -v dnf &> /dev/null; then
                # Fedora
                echo "Detected Fedora system"
                sudo dnf install -y nodejs npm

            elif command -v pacman &> /dev/null; then
                # Arch Linux
                echo "Detected Arch Linux system"
                sudo pacman -S --noconfirm nodejs npm

            elif command -v zypper &> /dev/null; then
                # openSUSE
                echo "Detected openSUSE system"
                sudo zypper install -y nodejs npm

            else
                echo "Could not detect package manager. Please install Node.js manually:"
                echo "  https://nodejs.org/en/download/"
                exit 1
            fi
            ;;
        *)
            echo "Unsupported operating system: $OS"
            exit 1
            ;;
    esac

    echo ""
    echo "Node.js installation complete."
    echo "----------------------------------------"
}

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo "Node.js is not installed. Installing automatically..."
    echo ""
    install_nodejs
fi

# Check Node.js version
NODE_VERSION=$(node -v)
echo "Found Node.js $NODE_VERSION"
echo ""

# Detect OS and architecture
OS="$(uname -s)"
ARCH="$(uname -m)"
echo "Operating System: $OS"
echo "Architecture: $ARCH"
echo ""

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
    echo "Installing dependencies..."
    npm install
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install dependencies"
        exit 1
    fi
    echo "Dependencies installed successfully."
    echo ""
fi

# Show appropriate build menu based on OS
case "$OS" in
    Darwin)
        echo "Build Options (macOS):"
        echo "  1. Build DMG (Intel x64)"
        echo "  2. Build DMG (Apple Silicon ARM64)"
        echo "  3. Build Universal DMG (Intel + Apple Silicon)"
        echo "  4. Build ZIP Archive (current architecture)"
        echo "  5. Build All macOS Formats"
        echo "  6. Run Development Mode"
        echo "  7. Exit"
        echo ""
        read -p "Enter your choice (1-7): " choice

        case $choice in
            1)
                echo ""
                echo "Building DMG for Intel (x64)..."
                npm run build:mac
                ;;
            2)
                echo ""
                echo "Building DMG for Apple Silicon (ARM64)..."
                npm run build:mac-arm
                ;;
            3)
                echo ""
                echo "Building Universal DMG..."
                npm run build:mac-universal
                ;;
            4)
                echo ""
                echo "Building ZIP Archive..."
                if [[ "$ARCH" == "arm64" ]]; then
                    npx electron-builder --mac zip --arm64
                else
                    npx electron-builder --mac zip --x64
                fi
                ;;
            5)
                echo ""
                echo "Building All macOS Formats..."
                npm run build:mac
                npm run build:mac-arm
                npm run build:mac-universal
                ;;
            6)
                echo ""
                echo "Starting Development Mode..."
                npm run dev
                exit 0
                ;;
            7)
                exit 0
                ;;
            *)
                echo "Invalid choice. Please run the script again."
                exit 1
                ;;
        esac
        ;;

    Linux)
        echo "Build Options (Linux):"
        echo "  1. Build Debian Package (x64)"
        echo "  2. Build Debian Package (ARM64)"
        echo "  3. Build AppImage (x64)"
        echo "  4. Build tar.gz Archive"
        echo "  5. Build All Linux Formats"
        echo "  6. Run Development Mode"
        echo "  7. Exit"
        echo ""
        read -p "Enter your choice (1-7): " choice

        case $choice in
            1)
                echo ""
                echo "Building Debian Package (x64)..."
                npm run build:deb
                ;;
            2)
                echo ""
                echo "Building Debian Package (ARM64)..."
                npx electron-builder --linux deb --arm64
                ;;
            3)
                echo ""
                echo "Building AppImage (x64)..."
                npm run build:appimage
                ;;
            4)
                echo ""
                echo "Building tar.gz Archive..."
                npx electron-builder --linux tar.gz --x64
                ;;
            5)
                echo ""
                echo "Building All Linux Formats..."
                npm run build:linux
                ;;
            6)
                echo ""
                echo "Starting Development Mode..."
                npm run dev
                exit 0
                ;;
            7)
                exit 0
                ;;
            *)
                echo "Invalid choice. Please run the script again."
                exit 1
                ;;
        esac
        ;;

    *)
        echo "Unsupported operating system: $OS"
        echo "Please use build.bat for Windows or run on Linux/macOS."
        exit 1
        ;;
esac

echo ""
if [ $? -eq 0 ]; then
    echo "========================================"
    echo "Build completed successfully!"
    echo "Output files are in the 'dist' folder."
    echo "========================================"
else
    echo "========================================"
    echo "Build failed with error code $?"
    echo "========================================"
fi
