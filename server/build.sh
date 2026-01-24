#!/bin/bash
# Build script for Nedflix Headless Server (Linux)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/dist/server"
VERSION=$(node -p "require('$ROOT_DIR/package.json').version")

echo "Building Nedflix Headless Server v$VERSION..."

# Clean previous build
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Copy server files
cp "$ROOT_DIR/server.js" "$BUILD_DIR/"
cp "$ROOT_DIR/package.json" "$BUILD_DIR/"
cp "$ROOT_DIR/package-lock.json" "$BUILD_DIR/" 2>/dev/null || true

# Copy public files for web interface
cp -r "$ROOT_DIR/public" "$BUILD_DIR/"

# Copy additional files if they exist
[ -f "$ROOT_DIR/generate-certs.js" ] && cp "$ROOT_DIR/generate-certs.js" "$BUILD_DIR/"
[ -f "$ROOT_DIR/.env.example" ] && cp "$ROOT_DIR/.env.example" "$BUILD_DIR/"
[ -f "$ROOT_DIR/README.md" ] && cp "$ROOT_DIR/README.md" "$BUILD_DIR/"

# Create startup scripts
cat > "$BUILD_DIR/start.sh" << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
node server.js "$@"
EOF
chmod +x "$BUILD_DIR/start.sh"

cat > "$BUILD_DIR/install-service.sh" << 'EOF'
#!/bin/bash
# Install Nedflix as a systemd service

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo)"
    exit 1
fi

INSTALL_DIR="/opt/nedflix"
SERVICE_USER="${1:-nedflix}"

# Create user if doesn't exist
if ! id "$SERVICE_USER" &>/dev/null; then
    useradd -r -s /bin/false "$SERVICE_USER"
fi

# Copy files
mkdir -p "$INSTALL_DIR"
cp -r . "$INSTALL_DIR/"
chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR"

# Install dependencies
cd "$INSTALL_DIR"
npm install --production

# Create systemd service
cat > /etc/systemd/system/nedflix.service << SERVICEEOF
[Unit]
Description=Nedflix Headless Server
After=network.target

[Service]
Type=simple
User=$SERVICE_USER
WorkingDirectory=$INSTALL_DIR
ExecStart=/usr/bin/node server.js
Restart=always
RestartSec=10
Environment=NODE_ENV=production

[Install]
WantedBy=multi-user.target
SERVICEEOF

# Reload and enable service
systemctl daemon-reload
systemctl enable nedflix
echo "Nedflix service installed. Start with: sudo systemctl start nedflix"
EOF
chmod +x "$BUILD_DIR/install-service.sh"

# Install production dependencies
cd "$BUILD_DIR"
npm install --production --ignore-scripts 2>/dev/null || npm install --production

# Create tarball
cd "$ROOT_DIR/dist"
tar -czvf "nedflix-server-${VERSION}-linux.tar.gz" server/

echo ""
echo "Build complete: dist/nedflix-server-${VERSION}-linux.tar.gz"
echo ""
echo "Contents:"
echo "  - server.js        : Main server"
echo "  - public/          : Web interface"
echo "  - start.sh         : Quick start script"
echo "  - install-service.sh : Install as systemd service"
