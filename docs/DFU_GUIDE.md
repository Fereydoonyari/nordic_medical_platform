# DFU (Device Firmware Update) Guide

## Overview

This guide explains how to update firmware on the nRF52840 medical device using various DFU methods.

## Flash Memory Layout

Your device has the following flash partitions (already configured):

```
┌─────────────────────────────────┐ 0x00000000
│  MCUboot Bootloader (48KB)      │
├─────────────────────────────────┤ 0x0000C000
│  Slot 0 - Primary App (472KB)   │ ← Running firmware
├─────────────────────────────────┤ 0x00082000
│  Slot 1 - Update App (472KB)    │ ← New firmware uploaded here
├─────────────────────────────────┤ 0x000F8000
│  Scratch Area (32KB)            │ ← Used during update
└─────────────────────────────────┘ 0x00100000
```

## DFU Update Methods

### Method 1: MCUboot with MCUmgr (Recommended)

This method uses MCUboot bootloader with MCUmgr for secure firmware updates over USB/UART/BLE.

#### Step 1: Enable MCUboot in `prj.conf`

```conf
# Enable MCUboot bootloader
CONFIG_BOOTLOADER_MCUBOOT=y

# Enable MCUmgr for firmware updates
CONFIG_MCUMGR=y
CONFIG_MCUMGR_TRANSPORT_UART=y
CONFIG_MCUMGR_CMD_IMG_MGMT=y
CONFIG_MCUMGR_CMD_OS_MGMT=y

# Enable image signing (required for MCUboot)
CONFIG_MCUBOOT_SIGNATURE_KEY_FILE="bootloader/mcuboot-rsa-2048.pem"

# Increase stack for MCUmgr
CONFIG_MCUMGR_TRANSPORT_UART_MTU=256
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2304
```

#### Step 2: Build the Application with MCUboot

```bash
cd app

# Build the application
west build -b nrf52840dk_nrf52840 -p

# This creates:
# - build/zephyr/zephyr.hex       (application)
# - build/zephyr/zephyr.signed.hex (signed application)
# - build/mcuboot/zephyr/zephyr.hex (MCUboot bootloader)
```

#### Step 3: Flash MCUboot Bootloader (One-time)

```bash
# Flash MCUboot bootloader first (only needed once)
west flash --runner nrfjprog --hex-file build/mcuboot/zephyr/zephyr.hex

# Then flash your application
west flash
```

#### Step 4: Create DFU Update Package

```bash
# Install mcumgr tool
pip install imgtool
pip install mcumgr

# Create signed update image
west sign -t imgtool -- --key bootloader/mcuboot-rsa-2048.pem

# This creates: build/zephyr/zephyr.signed.bin
```

#### Step 5: Upload Firmware Update

**Via USB Serial:**
```bash
# Install mcumgr CLI
go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest

# Upload new firmware
mcumgr --conntype serial --connstring dev=/dev/ttyACM0,baud=115200 image upload build/zephyr/zephyr.signed.bin

# List images
mcumgr --conntype serial --connstring dev=/dev/ttyACM0,baud=115200 image list

# Test the new image (mark for boot)
mcumgr --conntype serial --connstring dev=/dev/ttyACM0,baud=115200 image test <hash>

# Reset device to boot new firmware
mcumgr --conntype serial --connstring dev=/dev/ttyACM0,baud=115200 reset

# After testing, confirm the image
mcumgr --conntype serial --connstring dev=/dev/ttyACM0,baud=115200 image confirm
```

**Via Bluetooth:**
```bash
# Upload via BLE (if BLE transport is enabled)
mcumgr --conntype ble --connstring peer_name='NISC-Medical-Device' image upload build/zephyr/zephyr.signed.bin
```

---

### Method 2: USB DFU (Nordic nRF Connect)

Use Nordic's nRF Connect for Desktop to perform DFU updates.

#### Step 1: Enable USB DFU Support

Add to `prj.conf`:
```conf
CONFIG_USB_DFU_CLASS=y
CONFIG_USB_DEVICE_DFU=y
CONFIG_IMG_MANAGER=y
CONFIG_FLASH=y
CONFIG_IMG_BLOCK_BUF_SIZE=512
```

#### Step 2: Build and Create Update Package

```bash
cd app
west build -b nrf52840dk_nrf52840

# Create DFU package
nrfutil pkg generate --hw-version 52 --sd-req 0x00 \
    --application build/zephyr/zephyr.hex \
    --application-version 1 \
    firmware_update.zip
```

#### Step 3: Upload via nRF Connect

1. Enter DFU mode on device (press Button 1)
2. Open nRF Connect for Desktop → Programmer
3. Select your device
4. Click "Add file" and select `firmware_update.zip`
5. Click "Write"

---

### Method 3: Serial DFU (via Shell)

Use the Zephyr shell for firmware updates over USB/UART.

#### Step 1: Enable DFU Shell Commands

Already enabled in your project! The shell commands are available.

#### Step 2: Upload Firmware via Shell

```bash
# Connect to device shell
screen /dev/ttyACM0 115200

# In device shell, enter DFU mode
uart:~$ dfu enter

# From host PC, use mcumgr to upload
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image upload firmware.bin
```

---

### Method 4: J-Link Direct Flash (Development)

For development, you can directly flash without DFU mode.

```bash
cd app

# Flash via J-Link
west flash

# Or use nrfjprog directly
nrfjprog --program build/zephyr/zephyr.hex --chiperase --verify --reset
```

---

## DFU Workflow in Your Application

When Button 1 is pressed, your application enters DFU mode:

```
1. User presses Button 1
   ↓
2. System enters DFU mode
   - LED Status: Fast Blink
   - LED Error: SOS pattern
   ↓
3. Device is ready for firmware update
   - MCUmgr listening on UART
   - Shell available for commands
   ↓
4. Upload new firmware using mcumgr/nRF Connect
   ↓
5. Press Button 1 again to exit DFU mode
   ↓
6. System resets and boots new firmware
```

## Security Considerations

### Image Signing (Production)

For production devices, always sign firmware images:

```bash
# Generate RSA key pair (one-time)
imgtool keygen -k bootloader/mcuboot-rsa-2048.pem -t rsa-2048

# Sign firmware
imgtool sign --key bootloader/mcuboot-rsa-2048.pem \
    --header-size 0x200 \
    --align 4 \
    --version 1.0.0 \
    --slot-size 0x76000 \
    build/zephyr/zephyr.bin \
    signed_firmware.bin
```

### Rollback Protection

Enable anti-rollback in `prj.conf`:
```conf
CONFIG_MCUBOOT_DOWNGRADE_PREVENTION=y
CONFIG_BOOT_SWAP_USING_MOVE=y
```

## Troubleshooting

### Issue: Device won't enter DFU mode
- **Solution**: Check button GPIO configuration
- Verify button callback is registered: `shell> device list`

### Issue: MCUmgr can't connect
- **Solution**: Check UART settings
- Verify: `CONFIG_MCUMGR_TRANSPORT_UART=y`
- Baud rate: 115200

### Issue: Image upload fails
- **Solution**: Check image size < 472KB (slot size)
- Verify image is signed correctly
- Check available flash space

### Issue: Device doesn't boot after update
- **Solution**: MCUboot validates signature
- Check boot logs: `west debug`
- Revert to previous image: MCUboot keeps backup

## Quick Reference

### Build Commands
```bash
# Clean build with MCUboot
west build -b nrf52840dk_nrf52840 -p

# Flash bootloader (one-time)
west flash --runner nrfjprog --hex-file build/mcuboot/zephyr/zephyr.hex

# Flash application
west flash

# Create signed image
west sign -t imgtool
```

### MCUmgr Commands
```bash
# Upload firmware
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image upload firmware.bin

# List images
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image list

# Test new image
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image test <hash>

# Reset device
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 reset

# Confirm image
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image confirm
```

## Next Steps

1. **Enable MCUboot**: Uncomment MCUboot settings in `prj.conf`
2. **Generate Keys**: Create RSA key for signing
3. **Test DFU**: Build and test firmware update process
4. **Automate**: Create build scripts for release packages

For medical device compliance, ensure all firmware updates are:
- ✅ Digitally signed
- ✅ Version controlled
- ✅ Validated before deployment
- ✅ Logged and auditable

