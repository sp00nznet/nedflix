#!/bin/bash
# ========================================
# Nedflix Desktop - Linux Build Script
# ========================================

set -e

echo "========================================"
echo "Nedflix Desktop - Linux Build Script"
echo "========================================"
echo ""

# Navigate to script directory
cd "$(dirname "$0")"
echo "Working directory: $(pwd)"
echo ""

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

# Build menu
echo "Build Options:"
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

# ========================================
# Function to install Node.js
# ========================================
install_nodejs() {
    echo "----------------------------------------"
    echo "Installing Node.js..."
    echo "----------------------------------------"

    # Detect package manager
    if command -v apt-get &> /dev/null; then
        # Debian/Ubuntu
        echo "Detected Debian/Ubuntu system"

        # Install Node.js via NodeSource
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

    echo ""
    echo "Node.js installation complete."
    echo "----------------------------------------"
}
