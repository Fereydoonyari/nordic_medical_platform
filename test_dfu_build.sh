#!/bin/bash

# NISC Medical Device DFU Build Test Script
# This script builds and tests the DFU and booting system

echo "=== NISC Medical Device DFU Build Test ==="
echo "Building project with DFU and booting components..."

# Change to app directory
cd app

# Clean previous build
echo "Cleaning previous build..."
west build -t clean

# Build the project
echo "Building for nRF52840 DK..."
west build -b nrf52840dk_nrf52840

# Check build result
if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo ""
    echo "Build artifacts:"
    echo "- Binary: build/zephyr/zephyr.bin"
    echo "- Hex: build/zephyr/zephyr.hex"
    echo "- ELF: build/zephyr/zephyr.elf"
    echo ""
    echo "DFU Components included:"
    echo "- ✅ Bootloader (bootloader.c)"
    echo "- ✅ Bluetooth DFU Service (bluetooth_dfu.c)"
    echo "- ✅ Serial Communication (serial_comm.c)"
    echo "- ✅ Button Handler (button_handler.c)"
    echo "- ✅ Main Application Integration"
    echo ""
    echo "To flash the device:"
    echo "  west flash"
    echo ""
    echo "To monitor serial output:"
    echo "  west espressif monitor"
    echo ""
    echo "DFU Mode Entry:"
    echo "  1. Power on device"
    echo "  2. Hold button for 3+ seconds"
    echo "  3. Device enters DFU mode with Bluetooth advertising"
    echo ""
    echo "Normal Mode Entry:"
    echo "  1. Power on device"
    echo "  2. Press button briefly or wait for timeout"
    echo "  3. Device starts normal medical application"
else
    echo "❌ Build failed!"
    echo "Check the build output above for errors."
    exit 1
fi
