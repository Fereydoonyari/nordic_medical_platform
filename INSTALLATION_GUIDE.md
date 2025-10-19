# NISC Medical Device - Automated Zephyr Installation

This repository provides automated installation scripts for Zephyr RTOS development environment, specifically designed for the NISC Medical Wearable Device project.

## 🚀 Quick Installation (Recommended)

### For Ubuntu/WSL Users

**Option 1: Automated Script (Recommended)**
```bash
# Make the script executable and run it
chmod +x install_zephyr.sh
./install_zephyr.sh
```

**Option 2: One-liner Installation**
```bash
# Copy and paste this single command:
curl -fsSL https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/scripts/requirements.txt | xargs -I {} bash -c 'pip3 install --user {}' && pip3 install --user west && mkdir -p ~/zephyr && cd ~/zephyr && west init -m https://github.com/zephyrproject-rtos/zephyr && west update && pip3 install --user -r zephyr/scripts/requirements.txt && wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.5/zephyr-sdk-0.16.5_linux-x86_64.tar.xz && tar xf zephyr-sdk-0.16.5_linux-x86_64.tar.xz && cd zephyr-sdk-0.16.5 && ./setup.sh -t arm-zephyr-eabi && echo 'export ZEPHYR_BASE="$HOME/zephyr/zephyr"' >> ~/.bashrc && echo 'export ZEPHYR_SDK_INSTALL_DIR="$HOME/zephyr/zephyr-sdk-0.16.5"' >> ~/.bashrc && echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc && echo "Installation complete! Run 'source ~/.bashrc' to activate."
```

**Option 3: Windows WSL Integration**
```cmd
# Run from Windows Command Prompt
install_zephyr_wsl.bat
```

## 📋 What Gets Installed

The automated installation script installs:

### Core Tools
- **West**: Zephyr's meta-tool for managing workspaces
- **Zephyr SDK**: Cross-compilation toolchain for ARM and RISC-V
- **CMake**: Build system generator
- **Ninja**: Fast build system
- **Python 3.11+**: Required for West and Zephyr tools

### Development Tools
- **Git**: Version control
- **Build tools**: gcc, g++, make, etc.
- **DFU utilities**: For firmware flashing
- **Device tree compiler**: For hardware configuration
- **nRF tools**: Nordic-specific utilities

### Python Packages
- All Zephyr Python dependencies
- West tool and its dependencies
- nrfutil for Nordic device programming

## 🔧 Manual Installation (If Automated Fails)

If the automated script fails, you can install manually:

### 1. Install System Dependencies
```bash
sudo apt update
sudo apt install -y git wget curl build-essential cmake ninja-build \
    gperf ccache dfu-util device-tree-compiler python3-dev python3-pip \
    python3-setuptools python3-wheel python3-venv libffi-dev libssl-dev \
    libusb-1.0-0-dev libudev-dev pkg-config libtinfo5 libncurses5-dev \
    libncursesw5-dev libreadline-dev zlib1g-dev libbz2-dev libreadline-dev \
    libsqlite3-dev libncurses5-dev libncursesw5-dev xz-utils tk-dev \
    libxml2-dev libxmlsec1-dev liblzma-dev
```

### 2. Install West
```bash
pip3 install --user west
export PATH="$HOME/.local/bin:$PATH"
```

### 3. Initialize Zephyr Workspace
```bash
mkdir -p ~/zephyr
cd ~/zephyr
west init -m https://github.com/zephyrproject-rtos/zephyr
west update
```

### 4. Install Python Dependencies
```bash
pip3 install --user -r zephyr/scripts/requirements.txt
```

### 5. Install Zephyr SDK
```bash
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.5/zephyr-sdk-0.16.5_linux-x86_64.tar.xz
tar xf zephyr-sdk-0.16.5_linux-x86_64.tar.xz
cd zephyr-sdk-0.16.5
./setup.sh -t arm-zephyr-eabi
```

### 6. Set Environment Variables
```bash
echo 'export ZEPHYR_BASE="$HOME/zephyr/zephyr"' >> ~/.bashrc
echo 'export ZEPHYR_SDK_INSTALL_DIR="$HOME/zephyr/zephyr-sdk-0.16.5"' >> ~/.bashrc
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

## ✅ Verification

After installation, verify everything works:

```bash
# Check West installation
west --version

# Check Zephyr version
west zephyr-version

# Check available boards
west boards | grep nrf52840

# Check environment variables
echo $ZEPHYR_BASE
echo $ZEPHYR_SDK_INSTALL_DIR
```

## 🏗️ Building the NISC Medical Device

Once Zephyr is installed:

```bash
# Navigate to project directory
cd ~/zephyr/nordic_medical_platform

# Build the project
cd app
west build -b nrf52840dk_nrf52840

# Flash the device
west flash

# Monitor serial output
west espressif monitor
```

## 🐛 Troubleshooting

### Common Issues

**1. "west: command not found"**
```bash
# Add to PATH
export PATH="$HOME/.local/bin:$PATH"
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

**2. "Zephyr SDK not found"**
```bash
# Check environment variables
echo $ZEPHYR_SDK_INSTALL_DIR
# Should show: /home/username/zephyr/zephyr-sdk-0.16.5
```

**3. "Python dependencies missing"**
```bash
# Reinstall requirements
pip3 install --user -r zephyr/scripts/requirements.txt
```

**4. "Permission denied"**
```bash
# Make scripts executable
chmod +x install_zephyr.sh
chmod +x quick_install.sh
```

**5. "WSL not available"**
```cmd
# Install WSL2 on Windows
wsl --install
# Restart computer
```

### Getting Help

- **Zephyr Documentation**: https://docs.zephyrproject.org/
- **West Tool Guide**: https://docs.zephyrproject.org/latest/develop/west/
- **nRF52840 DK Guide**: https://docs.zephyrproject.org/latest/boards/arm/nrf52840dk_nrf52840/doc/index.html

## 📁 Project Structure After Installation

```
~/zephyr/
├── zephyr/                    # Zephyr RTOS source
├── zephyr-sdk-0.16.5/         # Zephyr SDK
├── nordic_medical_platform/   # Your project (symlinked)
└── .west/                     # West workspace configuration
```

## 🔄 Updating

To update Zephyr and tools:

```bash
cd ~/zephyr
west update
pip3 install --user -r zephyr/scripts/requirements.txt
```

## 📝 Notes

- The installation script is designed for Ubuntu 20.04+ and WSL2
- All tools are installed in user space (no sudo required for Python packages)
- Environment variables are automatically added to ~/.bashrc
- The script creates a symlink to your project in the Zephyr workspace
- nRF52840 DK is the target board for this project

## 🎯 Next Steps

After successful installation:

1. **Restart your terminal** or run `source ~/.bashrc`
2. **Navigate to your project**: `cd ~/zephyr/nordic_medical_platform`
3. **Build the project**: `cd app && west build -b nrf52840dk_nrf52840`
4. **Flash the device**: `west flash`
5. **Test DFU functionality**: Hold button for 3+ seconds during boot

The NISC Medical Device with DFU and booting system is now ready for development!
