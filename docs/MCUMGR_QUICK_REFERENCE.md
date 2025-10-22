# MCUmgr Quick Reference Card

## Installation

```bash
# Option 1: Python (easiest)
pip install mcumgr imgtool

# Option 2: Go (latest version)
go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest

# Verify
mcumgr version
```

## Connection String

```bash
# Set once per session
export MCUMGR_CONN="--conntype serial --connstring dev=/dev/ttyACM0,baud=115200"

# Then use
mcumgr $MCUMGR_CONN <command>
```

## Common Commands

### Connection & Status

```bash
# Test connection
mcumgr $MCUMGR_CONN echo "hello"

# List firmware images
mcumgr $MCUMGR_CONN image list

# Get device info
mcumgr $MCUMGR_CONN taskstat
```

### Firmware Update

```bash
# 1. Upload new firmware
mcumgr $MCUMGR_CONN image upload firmware.bin

# 2. List images (get hash of new image)
mcumgr $MCUMGR_CONN image list

# 3. Test new image
mcumgr $MCUMGR_CONN image test <hash>

# 4. Reboot to test
mcumgr $MCUMGR_CONN reset

# 5. Confirm if good (or device auto-reverts after 3 boots)
mcumgr $MCUMGR_CONN image confirm
```

### Device Management

```bash
# Reset device
mcumgr $MCUMGR_CONN reset

# Erase image slot
mcumgr $MCUMGR_CONN image erase 1

# Get statistics
mcumgr $MCUMGR_CONN stat show
```

### Shell Commands

```bash
# Execute shell command
mcumgr $MCUMGR_CONN shell exec "device list"
mcumgr $MCUMGR_CONN shell exec "kernel version"
```

## Build Commands

```bash
# Build with MCUmgr support
cd app
west build -b nrf52840dk_nrf52840 -- -DEXTRA_CONF_FILE=mcumgr_full.conf

# Sign image
west sign -t imgtool -- --version "1.2.3"

# Flash bootloader (one-time)
west flash --hex-file build/mcuboot/zephyr/zephyr.hex

# Flash application
west flash
```

## Typical Update Flow

```
┌─────────────────────────┐
│  1. Build new firmware  │
│  west build && sign     │
└───────────┬─────────────┘
            ↓
┌─────────────────────────┐
│  2. Enter DFU mode      │
│  Press Button 1         │
└───────────┬─────────────┘
            ↓
┌─────────────────────────┐
│  3. Upload firmware     │
│  mcumgr image upload    │
└───────────┬─────────────┘
            ↓
┌─────────────────────────┐
│  4. Test new image      │
│  mcumgr image test      │
└───────────┬─────────────┘
            ↓
┌─────────────────────────┐
│  5. Reset device        │
│  mcumgr reset           │
└───────────┬─────────────┘
            ↓
┌─────────────────────────┐
│  6. Verify & confirm    │
│  mcumgr image confirm   │
└─────────────────────────┘
```

## Image States

| State | Meaning |
|-------|---------|
| `active confirmed` | Current running firmware, stable |
| `active` | Running but not confirmed (will revert if rebooted) |
| `pending` | Will boot on next reboot |
| `confirmed` | Marked as good, won't revert |

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Connection failed | Press Button 1 (DFU mode) |
| Upload timeout | Use `-n 128` for smaller chunks |
| Image invalid | Re-sign with correct key |
| Device won't boot | Auto-reverts after 3 attempts |
| Wrong version | Check `image list` output |

## One-Line Update

```bash
# Upload, test, reboot in one command (automated script)
./scripts/dfu_update.sh app/build/zephyr/zephyr.signed.bin
```

## Safety Rules

1. ✅ **Always test before confirm**
2. ✅ **Keep backup of working firmware**
3. ✅ **Sign all production images**
4. ✅ **Document all updates**
5. ❌ **Never skip test phase in production**

## File Locations

```
your-project/
├── app/
│   ├── mcumgr_full.conf     ← MCUmgr configuration
│   ├── bootloader/
│   │   └── mcuboot-rsa-2048.pem  ← Signing key
│   └── build/
│       └── zephyr/
│           └── zephyr.signed.bin  ← Update file
├── scripts/
│   ├── dfu_update.sh         ← Automated update
│   └── quick_mcumgr_test.sh  ← Test MCUmgr
└── docs/
    └── MCUMGR_DETAILED_GUIDE.md  ← Full documentation
```

## Quick Test

```bash
# Test if MCUmgr is working
./scripts/quick_mcumgr_test.sh

# Or manually
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 echo "test"
```

## Emergency Recovery

If device is bricked:

```bash
# Flash bootloader and firmware via J-Link
cd app
west build -b nrf52840dk_nrf52840
nrfjprog --program build/mcuboot/zephyr/zephyr.hex --chiperase
nrfjprog --program build/zephyr/zephyr.signed.hex --sectorerase --reset
```

---

**Full Guide**: See [MCUMGR_DETAILED_GUIDE.md](MCUMGR_DETAILED_GUIDE.md)

