#!/bin/bash
# DFU Update Script - Upload new firmware to device
# Usage: ./scripts/dfu_update.sh <firmware.bin> [device]

set -e

FIRMWARE=${1:-"app/build/zephyr/zephyr.signed.bin"}
DEVICE=${2:-"/dev/ttyACM0"}
BAUD=${3:-"115200"}

if [ ! -f "$FIRMWARE" ]; then
    echo "Error: Firmware file not found: $FIRMWARE"
    echo "Usage: $0 <firmware.bin> [device] [baudrate]"
    exit 1
fi

echo "========================================="
echo "DFU Firmware Update"
echo "========================================="
echo "Firmware: $FIRMWARE"
echo "Device:   $DEVICE"
echo "Baud:     $BAUD"
echo ""

# Check if mcumgr is installed
if ! command -v mcumgr &> /dev/null; then
    echo "Error: mcumgr not found"
    echo "Install with: go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest"
    exit 1
fi

# Check if device is available
if [ ! -e "$DEVICE" ]; then
    echo "Error: Device not found: $DEVICE"
    echo "Available devices:"
    ls -l /dev/ttyACM* 2>/dev/null || echo "  No /dev/ttyACM* devices found"
    ls -l /dev/ttyUSB* 2>/dev/null || echo "  No /dev/ttyUSB* devices found"
    exit 1
fi

CONNSTRING="dev=$DEVICE,baud=$BAUD"

echo "Step 1: Checking device connection..."
if mcumgr --conntype serial --connstring "$CONNSTRING" echo "Hello" &>/dev/null; then
    echo "  ✓ Device connected"
else
    echo "  ✗ Device not responding"
    echo ""
    echo "Make sure:"
    echo "  1. Device is connected via USB"
    echo "  2. Device is in DFU mode (press Button 1)"
    echo "  3. MCUmgr is enabled in firmware"
    exit 1
fi

echo ""
echo "Step 2: Listing current images..."
mcumgr --conntype serial --connstring "$CONNSTRING" image list

echo ""
echo "Step 3: Uploading new firmware..."
echo "This may take 1-2 minutes..."
mcumgr --conntype serial --connstring "$CONNSTRING" image upload "$FIRMWARE"

echo ""
echo "Step 4: Listing images after upload..."
IMAGE_LIST=$(mcumgr --conntype serial --connstring "$CONNSTRING" image list)
echo "$IMAGE_LIST"

# Extract the hash of the newly uploaded image (in slot 1)
NEW_HASH=$(echo "$IMAGE_LIST" | grep -A 5 "slot=1" | grep "hash:" | awk '{print $2}')

if [ -z "$NEW_HASH" ]; then
    echo "Error: Could not find uploaded image"
    exit 1
fi

echo ""
echo "Step 5: Testing new image..."
echo "Hash: $NEW_HASH"
mcumgr --conntype serial --connstring "$CONNSTRING" image test "$NEW_HASH"

echo ""
echo "Step 6: Resetting device to boot new firmware..."
mcumgr --conntype serial --connstring "$CONNSTRING" reset

echo ""
echo "========================================="
echo "✓ Firmware Update Complete!"
echo "========================================="
echo ""
echo "The device is rebooting with the new firmware."
echo "After testing, confirm the update with:"
echo "  mcumgr --conntype serial --connstring $CONNSTRING image confirm"
echo ""
echo "Or to rollback:"
echo "  mcumgr --conntype serial --connstring $CONNSTRING reset"
echo ""

