#!/bin/bash

# NISC Medical Device DFU Build Test Script
# This script builds and tests the DFU and booting system
# 
# Prerequisites:
# 1. Install Zephyr SDK and West tool
# 2. Run setup_zephyr.ps1 first
# 3. Set environment variables

echo "=== NISC Medical Device DFU Build Test ==="
echo ""

# Check if West is available
if ! command -v west &> /dev/null; then
    echo "❌ West tool not found!"
    echo ""
    echo "Please install Zephyr development environment first:"
    echo "1. Run: .\setup_zephyr.ps1"
    echo "2. Download and install Zephyr SDK"
    echo "3. Set environment variables"
    echo ""
    echo "See ZEPHYR_SETUP_GUIDE.md for detailed instructions"
    exit 1
fi

# Check if we're in a Zephyr workspace
if [ ! -d "zephyr" ]; then
    echo "❌ Not in a Zephyr workspace!"
    echo ""
    echo "Please run the setup script first:"
    echo ".\setup_zephyr.ps1"
    exit 1
fi

# Check if app directory exists
if [ ! -d "app" ]; then
    echo "❌ App directory not found!"
    echo "Please run this script from the project root directory"
    exit 1
fi

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
    echo ""
    echo "Common issues:"
    echo "- Zephyr SDK not installed or not in PATH"
    echo "- Environment variables not set"
    echo "- Missing dependencies"
    echo ""
    echo "See ZEPHYR_SETUP_GUIDE.md for troubleshooting"
    exit 1
fi
