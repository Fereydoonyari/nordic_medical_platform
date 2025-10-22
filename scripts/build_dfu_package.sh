#!/bin/bash
# Build DFU Package Script
# This script builds the application with MCUboot and creates a DFU package

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
APP_DIR="$PROJECT_ROOT/app"
BUILD_DIR="$APP_DIR/build"
RELEASE_DIR="$PROJECT_ROOT/releases"
VERSION=${1:-"1.0.0"}

echo "========================================="
echo "Building DFU Package v$VERSION"
echo "========================================="

# Create release directory
mkdir -p "$RELEASE_DIR"

# Step 1: Build application with MCUboot
echo ""
echo "Step 1: Building application with MCUboot..."
cd "$APP_DIR"
west build -b nrf52840dk_nrf52840 -- -DEXTRA_CONF_FILE=mcuboot.conf

# Step 2: Check if MCUboot bootloader was built
if [ -f "$BUILD_DIR/mcuboot/zephyr/zephyr.hex" ]; then
    echo ""
    echo "Step 2: Copying MCUboot bootloader..."
    cp "$BUILD_DIR/mcuboot/zephyr/zephyr.hex" "$RELEASE_DIR/mcuboot-bootloader.hex"
    echo "  ✓ Bootloader: $RELEASE_DIR/mcuboot-bootloader.hex"
else
    echo "  ⚠ Warning: MCUboot bootloader not found"
fi

# Step 3: Sign the application image
echo ""
echo "Step 3: Signing application image..."
west sign -t imgtool -- --version "$VERSION"

# Step 4: Copy signed binaries to release directory
echo ""
echo "Step 4: Creating release package..."
RELEASE_NAME="nisc-medical-v$VERSION"
RELEASE_PKG="$RELEASE_DIR/$RELEASE_NAME"

mkdir -p "$RELEASE_PKG"

# Copy application binaries
cp "$BUILD_DIR/zephyr/zephyr.hex" "$RELEASE_PKG/app-unsigned.hex"
cp "$BUILD_DIR/zephyr/zephyr.bin" "$RELEASE_PKG/app-unsigned.bin"

if [ -f "$BUILD_DIR/zephyr/zephyr.signed.hex" ]; then
    cp "$BUILD_DIR/zephyr/zephyr.signed.hex" "$RELEASE_PKG/app-signed.hex"
    cp "$BUILD_DIR/zephyr/zephyr.signed.bin" "$RELEASE_PKG/app-signed.bin"
    echo "  ✓ Signed application: $RELEASE_PKG/app-signed.bin"
else
    echo "  ⚠ Warning: Signed image not found"
fi

# Step 5: Create version info file
echo ""
echo "Step 5: Creating version information..."
cat > "$RELEASE_PKG/version.txt" << EOF
Product: NISC Medical Wearable Device
Version: $VERSION
Build Date: $(date)
Build Host: $(hostname)
Git Commit: $(git rev-parse --short HEAD 2>/dev/null || echo "unknown")

Files:
- mcuboot-bootloader.hex : MCUboot bootloader (flash once)
- app-signed.hex         : Signed application (for DFU)
- app-signed.bin         : Signed application binary (for mcumgr)

Flash Instructions:
1. Flash bootloader (one-time):
   nrfjprog --program mcuboot-bootloader.hex --chiperase --verify

2. Flash application:
   nrfjprog --program app-signed.hex --sectorerase --verify --reset

DFU Upload Instructions:
   mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image upload app-signed.bin
EOF

echo "  ✓ Version info: $RELEASE_PKG/version.txt"

# Step 6: Create DFU package for nRF Connect (if nrfutil is available)
echo ""
echo "Step 6: Creating nRF Connect DFU package..."
if command -v nrfutil &> /dev/null; then
    nrfutil pkg generate --hw-version 52 --sd-req 0x00 \
        --application "$BUILD_DIR/zephyr/zephyr.hex" \
        --application-version-string "$VERSION" \
        "$RELEASE_PKG/dfu-package.zip"
    echo "  ✓ DFU package: $RELEASE_PKG/dfu-package.zip"
else
    echo "  ⚠ nrfutil not found, skipping DFU package creation"
    echo "    Install: pip install nrfutil"
fi

# Step 7: Create archive
echo ""
echo "Step 7: Creating release archive..."
cd "$RELEASE_DIR"
tar -czf "${RELEASE_NAME}.tar.gz" "$RELEASE_NAME"
echo "  ✓ Release archive: $RELEASE_DIR/${RELEASE_NAME}.tar.gz"

# Summary
echo ""
echo "========================================="
echo "✓ Build Complete!"
echo "========================================="
echo "Release package: $RELEASE_PKG"
echo ""
echo "Next steps:"
echo "1. Flash bootloader (one-time): west flash --hex-file $RELEASE_DIR/mcuboot-bootloader.hex"
echo "2. Flash application: west flash"
echo "3. Test DFU update:"
echo "   - Press Button 1 to enter DFU mode"
echo "   - mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image upload $RELEASE_PKG/app-signed.bin"
echo ""

