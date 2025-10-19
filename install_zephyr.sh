#!/bin/bash

# NISC Medical Device - Automated Zephyr Installation Script for Ubuntu/WSL
# This script automatically installs Zephyr SDK, West tool, and all dependencies
# Run with: chmod +x install_zephyr.sh && ./install_zephyr.sh

set -e  # Exit on any error

echo "=== NISC Medical Device - Automated Zephyr Installation ==="
echo "This script will install Zephyr SDK, West tool, and all dependencies"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running on Ubuntu/WSL
if ! grep -q "Ubuntu\|WSL" /etc/os-release 2>/dev/null; then
    print_warning "This script is designed for Ubuntu/WSL. Proceeding anyway..."
fi

# Update package list
print_status "Updating package list..."
sudo apt update

# Install required packages
print_status "Installing required packages..."
sudo apt install -y \
    git \
    wget \
    curl \
    build-essential \
    cmake \
    ninja-build \
    gperf \
    ccache \
    dfu-util \
    device-tree-compiler \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    python3-venv \
    libffi-dev \
    libssl-dev \
    libusb-1.0-0-dev \
    libudev-dev \
    pkg-config \
    libtinfo5 \
    libncurses5-dev \
    libncursesw5-dev \
    libreadline-dev \
    libffi-dev \
    libssl-dev \
    zlib1g-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev \
    libncurses5-dev \
    libncursesw5-dev \
    xz-utils \
    tk-dev \
    libxml2-dev \
    libxmlsec1-dev \
    libffi-dev \
    liblzma-dev

print_success "Required packages installed"

# Install Python 3.11 if not available
PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2 | cut -d'.' -f1,2)
if [[ "$PYTHON_VERSION" < "3.11" ]]; then
    print_status "Installing Python 3.11..."
    sudo apt install -y software-properties-common
    sudo add-apt-repository -y ppa:deadsnakes/ppa
    sudo apt update
    sudo apt install -y python3.11 python3.11-venv python3.11-dev
    sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.11 1
fi

# Create Zephyr directory
ZEPHYR_DIR="$HOME/zephyr"
print_status "Creating Zephyr directory: $ZEPHYR_DIR"
mkdir -p "$ZEPHYR_DIR"
cd "$ZEPHYR_DIR"

# Install West
print_status "Installing West tool..."
pip3 install --user west

# Add pip user bin to PATH
export PATH="$HOME/.local/bin:$PATH"
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc

# Initialize West workspace
print_status "Initializing West workspace..."
west init -m https://github.com/zephyrproject-rtos/zephyr

# Update workspace
print_status "Updating Zephyr workspace..."
west update

# Install Python dependencies
print_status "Installing Python dependencies..."
pip3 install --user -r zephyr/scripts/requirements.txt

# Download and install Zephyr SDK
print_status "Downloading Zephyr SDK..."
SDK_VERSION="0.16.5"
SDK_URL="https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${SDK_VERSION}/zephyr-sdk-${SDK_VERSION}_linux-x86_64.tar.xz"
SDK_FILE="zephyr-sdk-${SDK_VERSION}_linux-x86_64.tar.xz"

if [ ! -f "$SDK_FILE" ]; then
    wget "$SDK_URL"
fi

print_status "Extracting Zephyr SDK..."
tar xf "$SDK_FILE"

# Setup SDK
print_status "Setting up Zephyr SDK..."
cd "zephyr-sdk-${SDK_VERSION}"
./setup.sh -t arm-zephyr-eabi
./setup.sh -t riscv64-zephyr-elf
cd ..

# Set environment variables
print_status "Setting up environment variables..."
cat >> ~/.bashrc << 'EOF'

# Zephyr Environment Variables
export ZEPHYR_BASE="$HOME/zephyr/zephyr"
export ZEPHYR_SDK_INSTALL_DIR="$HOME/zephyr/zephyr-sdk-0.16.5"
export PATH="$ZEPHYR_SDK_INSTALL_DIR/sysroots/x86_64-pokysdk-linux/usr/bin:$PATH"

# West workspace
export WEST_ROOT="$HOME/zephyr"
export WEST_ZEPHYR_MODULES="$HOME/zephyr/zephyr/modules"

# Add West to PATH
export PATH="$HOME/.local/bin:$PATH"
EOF

# Source the environment
source ~/.bashrc

# Verify installation
print_status "Verifying installation..."
west --version
west zephyr-version

# Install additional tools for nRF52840
print_status "Installing nRF tools..."
pip3 install --user nrfutil

# Create project symlink
print_status "Setting up project workspace..."
PROJECT_DIR="/mnt/c/Users/zephyrus/Documents/GitHub/nordic_medical_platform"
if [ -d "$PROJECT_DIR" ]; then
    ln -sf "$PROJECT_DIR" "$ZEPHYR_DIR/nordic_medical_platform"
    print_success "Project linked to Zephyr workspace"
fi

print_success "Installation completed successfully!"
echo ""
echo "=== Next Steps ==="
echo "1. Restart your terminal or run: source ~/.bashrc"
echo "2. Navigate to your project: cd ~/zephyr/nordic_medical_platform"
echo "3. Build the project: cd app && west build -b nrf52840dk_nrf52840"
echo "4. Flash the device: west flash"
echo ""
echo "=== Environment Variables Set ==="
echo "ZEPHYR_BASE: $ZEPHYR_BASE"
echo "ZEPHYR_SDK_INSTALL_DIR: $ZEPHYR_SDK_INSTALL_DIR"
echo "WEST_ROOT: $WEST_ROOT"
echo ""
echo "=== Available Boards ==="
west boards | grep nrf52840
