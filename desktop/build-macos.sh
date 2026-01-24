#!/bin/bash
# ========================================
# Nedflix Desktop - macOS Build Script
# ========================================

set -e

echo "========================================"
echo "Nedflix Desktop - macOS Build Script"
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

    if command -v brew &> /dev/null; then
        echo "Installing via Homebrew..."
        brew install node@20
        brew link node@20 --force --overwrite
    else
        echo "Homebrew not found. Installing Homebrew first..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

        # Add Homebrew to PATH for Apple Silicon
        if [[ $(uname -m) == "arm64" ]]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        fi

        brew install node@20
        brew link node@20 --force --overwrite
    fi

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

# Detect architecture
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"
if [[ "$ARCH" == "arm64" ]]; then
    echo "Running on Apple Silicon (M1/M2/M3)"
else
    echo "Running on Intel Mac"
fi
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
        echo ""
        echo "Step 1/3: Building Intel (x64)..."
        npm run build:mac
        echo ""
        echo "Step 2/3: Building Apple Silicon (ARM64)..."
        npm run build:mac-arm
        echo ""
        echo "Step 3/3: Building Universal Binary..."
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

echo ""
if [ $? -eq 0 ]; then
    echo "========================================"
    echo "Build completed successfully!"
    echo "Output files are in the 'dist' folder."
    echo "========================================"
    echo ""
    echo "Built files:"
    ls -la dist/*.dmg dist/*.zip 2>/dev/null || echo "No DMG or ZIP files found"
else
    echo "========================================"
    echo "Build failed with error code $?"
    echo "========================================"
fi
