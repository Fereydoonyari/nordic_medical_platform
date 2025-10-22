# Quick Start: DFU Firmware Updates

## Prerequisites

Install required tools:

```bash
# Python packages
pip install imgtool mcumgr west

# Or using Go for mcumgr (recommended)
go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest

# Nordic tools (optional, for nRF Connect method)
pip install nrfutil
```

## Quick DFU Update (4 Steps)

### 1. Build Firmware with DFU Support

```bash
cd app
west build -b nrf52840dk_nrf52840 -- -DEXTRA_CONF_FILE=mcuboot.conf
west sign -t imgtool -- --version 1.0.0
```

### 2. Enter DFU Mode on Device

- Press **Button 1** on the nRF52840 DK
- LEDs will show:
  - Status LED: Fast blinking
  - Error LED: SOS pattern
- Console shows: `>>> Entering DFU Mode`

### 3. Upload New Firmware

```bash
# Using the helper script (recommended)
./scripts/dfu_update.sh app/build/zephyr/zephyr.signed.bin

# Or manually with mcumgr
mcumgr --conntype serial --connstring dev=/dev/ttyACM0,baud=115200 \
    image upload app/build/zephyr/zephyr.signed.bin
```

### 4. Test and Confirm

```bash
# Device will auto-reboot with new firmware
# Test the new firmware, then confirm:
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image confirm
```

Done! Your device is now running the new firmware.

---

## First-Time Setup

If this is your first time using DFU, you need to flash the MCUboot bootloader once:

### Step 1: Generate Signing Key (One-time)

```bash
mkdir -p app/bootloader
imgtool keygen -k app/bootloader/mcuboot-rsa-2048.pem -t rsa-2048
```

⚠️ **Important**: Keep this key secure! You'll need it to sign all future firmware updates.

### Step 2: Flash MCUboot Bootloader (One-time)

```bash
cd app
west build -b nrf52840dk_nrf52840 -- -DEXTRA_CONF_FILE=mcuboot.conf

# Flash the bootloader (one-time operation)
west flash --runner nrfjprog --hex-file build/mcuboot/zephyr/zephyr.hex

# Flash the application
west flash
```

Now your device is ready for DFU updates!

---

## Common Operations

### Check Current Firmware Version

```bash
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image list
```

Output:
```
Images:
 image=0 slot=0
    version: 1.0.0
    bootable: true
    flags: active confirmed
    hash: abc123...
 image=0 slot=1
    version: 1.1.0
    bootable: true
    flags: pending
    hash: def456...
```

### Rollback to Previous Firmware

```bash
# Just reset the device
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 reset

# If the new firmware doesn't boot, MCUboot automatically rolls back
```

### Monitor DFU Progress

```bash
# In another terminal, watch the device console
screen /dev/ttyACM0 115200

# You'll see DFU progress messages
```

---

## Automated Build & Release

Use the provided script to create complete release packages:

```bash
# Build everything and create release package
./scripts/build_dfu_package.sh 1.0.0

# This creates:
# - releases/nisc-medical-v1.0.0/
#   ├── mcuboot-bootloader.hex
#   ├── app-signed.hex
#   ├── app-signed.bin
#   ├── dfu-package.zip
#   └── version.txt
```

---

## Troubleshooting

### Device not responding to mcumgr

**Problem**: `Error: failed to connect`

**Solutions**:
1. Press Button 1 to enter DFU mode
2. Check device connection: `ls -l /dev/ttyACM*`
3. Try different baud rate: `--connstring dev=/dev/ttyACM0,baud=9600`
4. Restart device

### Upload fails or times out

**Problem**: Upload hangs or fails

**Solutions**:
1. Increase MTU in `prj.conf`: `CONFIG_MCUMGR_TRANSPORT_UART_MTU=512`
2. Use smaller chunk size: `mcumgr ... image upload -n 128 firmware.bin`
3. Check available flash space: Firmware must be < 472KB

### Device doesn't boot after update

**Problem**: Device stuck in boot loop

**Solutions**:
1. MCUboot validates signatures - check signing key
2. Image may be corrupted - re-upload
3. MCUboot automatically rolls back to previous version after 3 failed boots

### "Image in slot 1 is not pending or confirmed"

**Problem**: Can't test uploaded image

**Solution**:
```bash
# List images and get the hash
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image list

# Test the new image (use hash from slot 1)
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image test <hash>

# Reset to boot
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 reset
```

---

## Safety Features

### Image Verification

MCUboot verifies every firmware image before booting:
- ✅ RSA-2048 signature validation
- ✅ Image integrity check (SHA-256)
- ✅ Version compatibility check

### Automatic Rollback

If new firmware fails to boot 3 times:
- 🔄 MCUboot automatically reverts to previous working firmware
- 📋 Boot counter prevents permanent brick
- ✅ Device always recovers to last known-good firmware

### Anti-Downgrade

Prevent firmware downgrades for security:
```conf
# In prj.conf or mcuboot.conf
CONFIG_MCUBOOT_DOWNGRADE_PREVENTION=y
```

---

## Production Workflow

1. **Development**: Test on dev board with unsigned images
2. **QA**: Test with signed images, verify rollback
3. **Release**: Create signed packages with version tags
4. **Deploy**: Distribute signed `.bin` files to field devices
5. **Audit**: Log all firmware updates for medical device compliance

---

## Next Steps

- 📖 Read full [DFU Guide](DFU_GUIDE.md)
- 🔐 Set up [Secure Boot](SECURE_BOOT.md) (coming soon)
- 📱 Enable [BLE DFU](BLE_DFU.md) for wireless updates (coming soon)
- 🏥 Medical Device [Compliance Checklist](COMPLIANCE.md) (coming soon)

## Support

For issues or questions:
- Check [DFU_GUIDE.md](DFU_GUIDE.md) for detailed documentation
- Review Nordic's MCUboot documentation
- Check Zephyr DFU samples: `<zephyr>/samples/subsys/mgmt/mcumgr/`

