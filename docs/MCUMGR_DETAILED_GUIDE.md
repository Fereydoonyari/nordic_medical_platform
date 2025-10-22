# MCUmgr Detailed Guide - Production Firmware Updates

## What is MCUmgr?

**MCUmgr** (MCU Manager) is a management subsystem for embedded devices that provides:
- **Firmware updates** over various transports (UART, BLE, UDP)
- **Image management** with versioning and rollback
- **File system access** for configuration/data
- **Statistics and monitoring**
- **Shell command execution**

It's the **industry-standard** way to update Zephyr-based devices in production.

## Why Use MCUmgr?

### Advantages over Direct Flash:
✅ **No hardware debugger needed** - Update via USB/UART/Bluetooth  
✅ **Secure updates** - Image verification with digital signatures  
✅ **Safe rollback** - Automatically reverts bad firmware  
✅ **Remote updates** - Update devices in the field  
✅ **Version control** - Track firmware versions  
✅ **Medical device compliant** - Meets regulatory requirements for updates  

### Use Cases:
- 🏥 **Medical devices** - Update patient monitors without removing from service
- 🏭 **Industrial IoT** - Remote firmware updates
- 🚗 **Automotive** - Over-the-air (OTA) updates
- 📱 **Wearables** - Update via smartphone app

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Host Computer                         │
│  ┌────────────┐    ┌──────────────┐    ┌────────────┐  │
│  │ mcumgr CLI │    │ nRF Connect  │    │ Custom App │  │
│  └─────┬──────┘    └──────┬───────┘    └─────┬──────┘  │
│        │                  │                    │         │
│        └──────────────────┴────────────────────┘         │
│                          │                               │
│                  ┌───────▼────────┐                      │
│                  │ SMP Protocol   │                      │
│                  └───────┬────────┘                      │
└──────────────────────────┼──────────────────────────────┘
                           │ (USB/UART/BLE)
┌──────────────────────────▼──────────────────────────────┐
│                  nRF52840 Device                         │
│  ┌───────────────────────────────────────────────────┐  │
│  │              MCUmgr Subsystem                     │  │
│  │  ┌──────────┐  ┌────────┐  ┌──────────────────┐  │  │
│  │  │  Image   │  │  File  │  │  Stats/Shell     │  │  │
│  │  │  Manager │  │  System│  │  Management      │  │  │
│  │  └──────────┘  └────────┘  └──────────────────┘  │  │
│  └───────────────────────────────────────────────────┘  │
│                          │                               │
│  ┌───────────────────────▼───────────────────────────┐  │
│  │              MCUboot Bootloader                   │  │
│  │  - Verifies image signatures                      │  │
│  │  - Manages boot process                           │  │
│  │  - Handles image swapping                         │  │
│  │  - Rollback on failure                            │  │
│  └───────────────────────────────────────────────────┘  │
│                          │                               │
│  ┌───────────────────────▼───────────────────────────┐  │
│  │              Flash Memory Layout                  │  │
│  │  ┌──────────────────────────────────────────────┐ │  │
│  │  │ Slot 0 (Primary)  │ Slot 1 (Secondary)       │ │  │
│  │  │ Running Firmware  │ New Firmware Upload      │ │  │
│  │  └──────────────────────────────────────────────┘ │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## Complete Setup Guide

### Step 1: Install MCUmgr Tools

#### Option A: Using Go (Recommended)
```bash
# Install Go first if not installed
# Then install mcumgr
go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest

# Verify installation
mcumgr version
```

#### Option B: Using Python
```bash
pip install mcumgr

# Verify installation
mcumgr version
```

#### Option C: Pre-built Binaries
Download from: https://github.com/apache/mynewt-mcumgr-cli/releases

---

### Step 2: Configure Your Firmware

#### Enable MCUmgr in `app/prj.conf`:

```conf
#=============================================================================
# MCUmgr Configuration for Firmware Updates
#=============================================================================

# Core MCUmgr subsystem
CONFIG_MCUMGR=y

# Transport layer - UART (USB serial)
CONFIG_MCUMGR_TRANSPORT_UART=y
CONFIG_MCUMGR_TRANSPORT_UART_MTU=256

# SMP (Simple Management Protocol) server
CONFIG_MCUMGR_SMP_UART=y
CONFIG_BASE64=y
CONFIG_CRC=y

# Image management commands
CONFIG_MCUMGR_GRP_IMG=y
CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK=y
CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS=y

# OS management commands (reset, echo, etc)
CONFIG_MCUMGR_GRP_OS=y
CONFIG_MCUMGR_GRP_OS_RESET_HOOK=y

# Statistics (optional, useful for debugging)
CONFIG_MCUMGR_GRP_STAT=y
CONFIG_STATS=y
CONFIG_STATS_NAMES=y

# Shell integration (optional)
CONFIG_MCUMGR_GRP_SHELL=y

# File system access (optional)
# CONFIG_MCUMGR_GRP_FS=y
# CONFIG_FILE_SYSTEM=y

# Increase buffer sizes for DFU operations
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2304
CONFIG_MAIN_STACK_SIZE=4096

# Enable reboot command
CONFIG_REBOOT=y

# Image manager
CONFIG_IMG_MANAGER=y
CONFIG_IMG_ENABLE_IMAGE_CHECK=y
CONFIG_FLASH=y
CONFIG_FLASH_PAGE_LAYOUT=y
CONFIG_FLASH_MAP=y
CONFIG_STREAM_FLASH=y

#=============================================================================
# MCUboot Bootloader Support
#=============================================================================

CONFIG_BOOTLOADER_MCUBOOT=y

# Image signing (critical for production!)
CONFIG_MCUBOOT_SIGNATURE_KEY_FILE="bootloader/mcuboot-rsa-2048.pem"

# Boot behavior
CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="1.0.0"
CONFIG_BOOT_SWAP_USING_MOVE=y

# Security features (enable for production)
# CONFIG_MCUBOOT_DOWNGRADE_PREVENTION=y  # Prevent downgrade attacks
# CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION=y  # Hardware-based prevention

# Logging for debugging (disable in production for space)
CONFIG_LOG=y
CONFIG_MCUBOOT_LOG_LEVEL_INF=y
```

---

### Step 3: Generate Signing Key (One-time Setup)

**⚠️ CRITICAL**: This key signs all your firmware. **Keep it secure!**

```bash
# Create bootloader directory
mkdir -p app/bootloader

# Generate RSA-2048 key pair
imgtool keygen -k app/bootloader/mcuboot-rsa-2048.pem -t rsa-2048

# Verify key
imgtool verify -k app/bootloader/mcuboot-rsa-2048.pem

# IMPORTANT: Back up this key securely!
# - Store in secure location (password manager, HSM)
# - Do NOT commit to Git (add to .gitignore)
# - Without this key, you cannot update devices!
```

**For Production:**
- Use Hardware Security Module (HSM) for key storage
- Implement key rotation policy
- Maintain backup keys in secure facility
- Document key management procedures

---

### Step 4: Build Firmware with MCUboot

```bash
cd app

# Clean build with MCUboot support
west build -b nrf52840dk_nrf52840 -p -- -DEXTRA_CONF_FILE=mcuboot.conf

# This builds:
# 1. MCUboot bootloader: build/mcuboot/zephyr/zephyr.hex
# 2. Your application: build/zephyr/zephyr.hex
# 3. Signed application: build/zephyr/zephyr.signed.hex
```

---

### Step 5: Flash MCUboot Bootloader (One-time)

**This step is only needed ONCE per device:**

```bash
# Method 1: Using west flash
west flash --runner nrfjprog --hex-file build/mcuboot/zephyr/zephyr.hex

# Method 2: Using nrfjprog directly
nrfjprog --program build/mcuboot/zephyr/zephyr.hex --chiperase --verify

# Method 3: Using J-Link Commander
JLinkExe -device nRF52840_xxAA -if SWD -speed 4000 \
    -CommandFile flash_mcuboot.jlink
```

**After this, you can update firmware via MCUmgr!** No more J-Link needed.

---

### Step 6: Flash Initial Application

```bash
# Flash your application to Slot 0
west flash

# Or manually:
nrfjprog --program build/zephyr/zephyr.signed.hex --sectorerase --verify --reset
```

---

## Using MCUmgr - Step by Step

### Connection Setup

#### Find Your Device

**Linux:**
```bash
ls -l /dev/ttyACM*
# Output: /dev/ttyACM0 -> Your device
```

**macOS:**
```bash
ls -l /dev/tty.usbmodem*
# Output: /dev/tty.usbmodem14201
```

**Windows:**
```powershell
mode
# Look for COM ports
# Or use Device Manager
```

#### Test Connection

```bash
# Replace /dev/ttyACM0 with your device
export DEVICE=/dev/ttyACM0

# Test echo command
mcumgr --conntype serial --connstring "dev=$DEVICE,baud=115200" echo "Hello MCUmgr"

# Expected output:
# {"r":"Hello MCUmgr"}
```

If this works, MCUmgr is ready! ✅

---

### Firmware Update Process

#### 1. Enter DFU Mode

**Press Button 1** on your device:
- Status LED: Fast blinking
- Error LED: SOS pattern
- Console shows: ">>> Entering DFU Mode"

#### 2. Check Current Firmware

```bash
mcumgr --conntype serial --connstring "dev=$DEVICE" image list
```

**Output:**
```
Images:
 image=0 slot=0
    version: 1.0.0
    bootable: true
    flags: active confirmed
    hash: abc123def456...
 image=0 slot=1
    <No image installed>
```

**Understanding the output:**
- `slot=0`: Currently running firmware (Primary)
- `slot=1`: Update slot (Secondary) - empty
- `active confirmed`: This firmware is stable and confirmed
- `hash`: Unique identifier for this firmware version

#### 3. Build New Firmware

```bash
cd app

# Make your changes to code
# Then build and sign
west build -b nrf52840dk_nrf52840
west sign -t imgtool -- --version "1.1.0"

# This creates: build/zephyr/zephyr.signed.bin
```

#### 4. Upload New Firmware

```bash
# Upload to Slot 1 (this takes 1-2 minutes)
mcumgr --conntype serial --connstring "dev=$DEVICE" \
    image upload build/zephyr/zephyr.signed.bin

# You'll see progress:
# 28.38 KiB / 234.15 KiB [=====>----------------]  12.12% 2.15 KiB/s
# ...
# Done
```

#### 5. Verify Upload

```bash
mcumgr --conntype serial --connstring "dev=$DEVICE" image list
```

**Output:**
```
Images:
 image=0 slot=0
    version: 1.0.0
    bootable: true
    flags: active confirmed
    hash: abc123def456...
 image=0 slot=1
    version: 1.1.0
    bootable: true
    flags: 
    hash: fed654cba321...
```

✅ New firmware in Slot 1!

#### 6. Test New Firmware

```bash
# Mark new image for testing
# Copy the hash from slot=1
mcumgr --conntype serial --connstring "dev=$DEVICE" \
    image test fed654cba321...

# Output:
# Images:
#  image=0 slot=0
#     ...
#  image=0 slot=1
#     version: 1.1.0
#     bootable: true
#     flags: pending     ← Now marked for next boot
#     hash: fed654cba321...
```

#### 7. Reboot Device

```bash
mcumgr --conntype serial --connstring "dev=$DEVICE" reset

# Device reboots...
# MCUboot swaps Slot 0 ↔ Slot 1
# New firmware starts running!
```

#### 8. Verify New Firmware is Running

```bash
# Wait for device to boot (5-10 seconds)
# Then check images
mcumgr --conntype serial --connstring "dev=$DEVICE" image list
```

**Output:**
```
Images:
 image=0 slot=0
    version: 1.1.0        ← New version running!
    bootable: true
    flags: active         ← Active but not confirmed yet!
    hash: fed654cba321...
 image=0 slot=1
    version: 1.0.0        ← Old version kept as backup
    bootable: true
    flags: 
    hash: abc123def456...
```

**Important**: New firmware is running but **NOT confirmed** yet!

#### 9. Test New Firmware

Test your new firmware thoroughly:
- ✓ All features working?
- ✓ Sensors reading correctly?
- ✓ No crashes or errors?
- ✓ LEDs behaving properly?

#### 10. Confirm or Revert

**If firmware is good - Confirm it:**
```bash
mcumgr --conntype serial --connstring "dev=$DEVICE" image confirm

# Output:
# Images:
#  image=0 slot=0
#     version: 1.1.0
#     flags: active confirmed  ← Now permanent!
```

**If firmware is bad - Revert:**
```bash
# Just reset without confirming
mcumgr --conntype serial --connstring "dev=$DEVICE" reset

# MCUboot automatically reverts to old firmware!
# Or: Device auto-reverts after 3 failed boot attempts
```

---

## Advanced Features

### 1. Image Upload Progress Tracking

```bash
# Upload with detailed progress
mcumgr --conntype serial --connstring "dev=$DEVICE" \
    -v image upload build/zephyr/zephyr.signed.bin

# -v: verbose mode shows detailed progress
```

### 2. Smaller Chunk Size (for slow connections)

```bash
# Use smaller chunks if upload fails
mcumgr --conntype serial --connstring "dev=$DEVICE" \
    image upload -n 128 build/zephyr/zephyr.signed.bin

# -n 128: 128-byte chunks (default is 512)
```

### 3. Erase Slot Before Upload

```bash
# Erase slot 1 before uploading
mcumgr --conntype serial --connstring "dev=$DEVICE" image erase 1

# Then upload
mcumgr --conntype serial --connstring "dev=$DEVICE" \
    image upload build/zephyr/zephyr.signed.bin
```

### 4. Direct Image Confirm (Skip Testing)

```bash
# Upload and immediately confirm (use with caution!)
mcumgr --conntype serial --connstring "dev=$DEVICE" \
    image upload build/zephyr/zephyr.signed.bin

# Confirm with hash
mcumgr --conntype serial --connstring "dev=$DEVICE" \
    image confirm <hash>

# Reset
mcumgr --conntype serial --connstring "dev=$DEVICE" reset
```

### 5. Get Device Statistics

```bash
# Get system stats
mcumgr --conntype serial --connstring "dev=$DEVICE" stat show
```

### 6. Execute Shell Commands

```bash
# Run shell command on device
mcumgr --conntype serial --connstring "dev=$DEVICE" \
    shell exec "device list"
```

---

## Automated Update Script

I've created `scripts/dfu_update.sh` for you:

```bash
#!/bin/bash
# Automated DFU update with safety checks

./scripts/dfu_update.sh build/zephyr/zephyr.signed.bin

# This script:
# 1. Checks device connection
# 2. Lists current firmware
# 3. Uploads new firmware
# 4. Marks for testing
# 5. Reboots device
# 6. Prompts for confirmation
```

**Usage:**
```bash
# Update with auto-test
./scripts/dfu_update.sh app/build/zephyr/zephyr.signed.bin

# Update specific device
./scripts/dfu_update.sh firmware.bin /dev/ttyACM1

# Update with custom baud rate
./scripts/dfu_update.sh firmware.bin /dev/ttyACM0 9600
```

---

## Production Workflow

### Development Phase
```bash
1. Make code changes
2. west build && west flash  # Direct flash for speed
3. Test on bench
4. Iterate quickly
```

### QA/Testing Phase
```bash
1. Build signed image: west build && west sign
2. Upload via MCUmgr: ./scripts/dfu_update.sh
3. Test thoroughly
4. Confirm or revert
5. Document results
```

### Production Deployment
```bash
1. Build release: ./scripts/build_dfu_package.sh 2.0.0
2. QA approval of release package
3. Distribute .bin files to field technicians
4. Field update via MCUmgr over USB/BLE
5. Confirm after 24-hour soak test
6. Log update in device history
```

---

## Troubleshooting

### "Error: failed to connect"

**Causes:**
- Device not connected
- Wrong port
- Device not in DFU mode
- Baud rate mismatch

**Solutions:**
```bash
# Check connection
ls -l /dev/ttyACM*

# Press Button 1 (enter DFU mode)

# Try different baud rates
mcumgr --conntype serial --connstring "dev=$DEVICE,baud=9600" echo test
mcumgr --conntype serial --connstring "dev=$DEVICE,baud=115200" echo test

# Check if another program is using the port
lsof /dev/ttyACM0
```

### "Upload failed" or "Timeout"

**Solutions:**
```bash
# Increase timeout
mcumgr --conntype serial --connstring "dev=$DEVICE,mtu=128" image upload firmware.bin

# Use smaller chunks
mcumgr --conntype serial --connstring "dev=$DEVICE" image upload -n 128 firmware.bin

# Check available flash space
mcumgr --conntype serial --connstring "dev=$DEVICE" image list
# Firmware must be < 472KB (slot size)
```

### "Image invalid" or "Failed to verify"

**Causes:**
- Image not signed
- Wrong signing key
- Corrupted upload

**Solutions:**
```bash
# Re-sign image
west sign -t imgtool -- --key bootloader/mcuboot-rsa-2048.pem

# Verify signature locally
imgtool verify -k bootloader/mcuboot-rsa-2048.pem build/zephyr/zephyr.signed.bin

# Re-upload
mcumgr --conntype serial --connstring "dev=$DEVICE" image erase 1
mcumgr --conntype serial --connstring "dev=$DEVICE" image upload firmware.bin
```

---

## Security Best Practices

### For Production Medical Devices:

1. **Always Sign Images**
   - Use strong RSA-2048 or ECDSA P-256 keys
   - Store keys in HSM or secure vault
   - Never commit keys to version control

2. **Enable Downgrade Prevention**
   ```conf
   CONFIG_MCUBOOT_DOWNGRADE_PREVENTION=y
   ```

3. **Version Every Build**
   ```bash
   west sign -t imgtool -- --version "2.1.3"
   ```

4. **Log All Updates**
   - Record who updated
   - Record when updated
   - Record from which version to which version
   - Store device serial number with update log

5. **Test Before Confirm**
   - Always use test/confirm workflow
   - Never auto-confirm in production
   - Require manual validation

6. **Maintain Update Audit Trail**
   - FDA/MDR compliance requires documentation
   - Log every firmware update
   - Store signatures and hashes

---

## Next Steps

✅ You now understand MCUmgr thoroughly!

**Try it:**
1. Add MCUmgr config to `prj.conf`
2. Build with MCUboot: `west build`
3. Flash bootloader (one-time)
4. Test update: `./scripts/dfu_update.sh`

**Resources:**
- [Full DFU Guide](DFU_GUIDE.md)
- [Quick Start](QUICK_START_DFU.md)
- [MCUmgr Official Docs](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html)
- [MCUboot Documentation](https://docs.mcuboot.com/)

