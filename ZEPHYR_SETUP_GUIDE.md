# Zephyr Development Environment Setup Guide

## Prerequisites
You need to install Zephyr SDK and West tool to build the project. Here's the complete setup:

## Step 1: Install Python and pip
```powershell
# Check if Python is installed
python --version
pip --version

# If not installed, download from https://www.python.org/downloads/
# Make sure to check "Add Python to PATH" during installation
```

## Step 2: Install West
```powershell
# Install West tool
pip install west

# Verify installation
west --version
```

## Step 3: Install Zephyr SDK
```powershell
# Download Zephyr SDK from:
# https://github.com/zephyrproject-rtos/sdk-ng/releases

# For Windows, download: zephyr-sdk-0.16.5_windows-x86_64.zip
# Extract to C:\zephyr-sdk-0.16.5
# Run the setup script:
C:\zephyr-sdk-0.16.5\setup.cmd
```

## Step 4: Initialize Zephyr Workspace
```powershell
# Navigate to your project directory
cd C:\Users\zephyrus\Documents\GitHub\nordic_medical_platform

# Initialize West workspace
west init -m https://github.com/zephyrproject-rtos/zephyr

# Update workspace
west update

# Install Python dependencies
pip install -r zephyr/scripts/requirements.txt
```

## Step 5: Set Environment Variables
```powershell
# Set Zephyr environment variables
$env:ZEPHYR_BASE = "C:\Users\zephyrus\Documents\GitHub\nordic_medical_platform\zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.16.5"

# Add to PATH
$env:PATH += ";C:\zephyr-sdk-0.16.5\bin"
```

## Step 6: Build the Project
```powershell
# Navigate to app directory
cd app

# Build for nRF52840 DK
west build -b nrf52840dk_nrf52840

# Flash the device
west flash
```

## Alternative: Use Docker (Easier Setup)
If you prefer not to install everything locally, you can use Docker:

```powershell
# Pull Zephyr Docker image
docker pull zephyrprojectrtos/zephyr-build:latest

# Run build in container
docker run -it --rm -v ${PWD}:/workspace zephyrprojectrtos/zephyr-build:latest bash
cd /workspace/app
west build -b nrf52840dk_nrf52840
```

## Troubleshooting

### Common Issues:
1. **West not found**: Make sure pip install west completed successfully
2. **Zephyr SDK not found**: Verify SDK installation and PATH
3. **Python dependencies**: Run pip install -r zephyr/scripts/requirements.txt
4. **Board not found**: Make sure you're using the correct board name

### Verify Installation:
```powershell
# Check West
west --version

# Check Zephyr
west zephyr-version

# Check available boards
west boards

# Check environment
west env
```

## Quick Setup Script
Save this as `setup_zephyr.ps1` and run it:

```powershell
# Install West
pip install west

# Download and setup Zephyr SDK (manual step)
Write-Host "Please download Zephyr SDK from: https://github.com/zephyrproject-rtos/sdk-ng/releases"
Write-Host "Extract to C:\zephyr-sdk-0.16.5 and run setup.cmd"

# Initialize workspace
west init -m https://github.com/zephyrproject-rtos/zephyr
west update

# Install dependencies
pip install -r zephyr/scripts/requirements.txt

Write-Host "Setup complete! Don't forget to set environment variables."
```
