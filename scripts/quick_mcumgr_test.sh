#!/bin/bash
# Quick MCUmgr Test Script
# Tests MCUmgr connectivity and basic functions

set -e

DEVICE=${1:-"/dev/ttyACM0"}
BAUD=${2:-"115200"}

echo "========================================="
echo "MCUmgr Quick Test"
echo "========================================="
echo "Device: $DEVICE"
echo "Baud:   $BAUD"
echo ""

# Check if mcumgr is installed
if ! command -v mcumgr &> /dev/null; then
    echo "❌ Error: mcumgr not found"
    echo ""
    echo "Install with:"
    echo "  pip install mcumgr"
    echo "OR"
    echo "  go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest"
    exit 1
fi

# Check if device exists
if [ ! -e "$DEVICE" ]; then
    echo "❌ Error: Device not found: $DEVICE"
    echo ""
    echo "Available devices:"
    ls -l /dev/ttyACM* 2>/dev/null || echo "  No /dev/ttyACM* devices"
    ls -l /dev/ttyUSB* 2>/dev/null || echo "  No /dev/ttyUSB* devices"
    exit 1
fi

CONN="--conntype serial --connstring dev=$DEVICE,baud=$BAUD"

echo "Test 1: Echo (Basic Connectivity)"
echo "-----------------------------------"
if mcumgr $CONN echo "Hello from MCUmgr!" 2>/dev/null; then
    echo "✅ Echo test passed"
else
    echo "❌ Echo test failed"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Make sure device is connected"
    echo "  2. Press Button 1 to enter DFU mode"
    echo "  3. Check if MCUmgr is enabled in firmware"
    echo "  4. Try: mcumgr $CONN echo test"
    exit 1
fi

echo ""
echo "Test 2: Image List (Firmware Info)"
echo "-----------------------------------"
if mcumgr $CONN image list 2>/dev/null; then
    echo "✅ Image list test passed"
else
    echo "⚠️  Image list failed (MCUboot may not be enabled)"
fi

echo ""
echo "Test 3: Task Statistics"
echo "-----------------------------------"
if mcumgr $CONN taskstat 2>/dev/null; then
    echo "✅ Task statistics test passed"
else
    echo "⚠️  Task statistics failed (may not be enabled)"
fi

echo ""
echo "Test 4: System Statistics"
echo "-----------------------------------"
if mcumgr $CONN stat show 2>/dev/null; then
    echo "✅ Statistics test passed"
else
    echo "⚠️  Statistics failed (may not be enabled)"
fi

echo ""
echo "========================================="
echo "✅ MCUmgr is working!"
echo "========================================="
echo ""
echo "You can now:"
echo "  • Upload firmware: mcumgr $CONN image upload firmware.bin"
echo "  • List images:     mcumgr $CONN image list"
echo "  • Reset device:    mcumgr $CONN reset"
echo ""
echo "Or use the automated script:"
echo "  ./scripts/dfu_update.sh app/build/zephyr/zephyr.signed.bin"
echo ""

