# DFU Update Cheat Sheet

## TL;DR - Update Firmware Now

Your device already has DFU button working! Here's how to update firmware:

### Option 1: Using MCUmgr (Recommended)

```bash
# 1. Install mcumgr
pip install mcumgr

# 2. Build new firmware
cd app && west build -b nrf52840dk_nrf52840

# 3. Press Button 1 on device to enter DFU mode

# 4. Upload firmware
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 \
    image upload build/zephyr/zephyr.bin

# 5. Device will reboot with new firmware
```

### Option 2: Using J-Link (Development)

```bash
# Just flash directly (easier for development)
cd app
west build -b nrf52840dk_nrf52840
west flash
```

### Option 3: Using nRF Connect (GUI)

1. Press Button 1 to enter DFU mode
2. Open nRF Connect for Desktop
3. Select Programmer
4. Drag & drop `app/build/zephyr/zephyr.hex`
5. Click "Write"

---

## What Happens When You Press Button 1

```
[Normal Operation]
       ↓
  Press Button 1
       ↓
[DFU Mode Activated]
- Status LED: Fast Blink
- Error LED: SOS Pattern
- Console: ">>> Entering DFU Mode"
- MCUmgr Ready: Listening for firmware updates
       ↓
  Upload new firmware via mcumgr/nRF Connect
       ↓
  Press Button 1 again (or device auto-reboots)
       ↓
[New Firmware Running]
```

---

## Quick Commands

### Check device connection
```bash
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 echo hello
```

### List current firmware
```bash
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 image list
```

### Upload firmware
```bash
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 \
    image upload app/build/zephyr/zephyr.bin
```

### Reset device
```bash
mcumgr --conntype serial --connstring dev=/dev/ttyACM0 reset
```

---

## For Full DFU with MCUboot

If you want proper bootloader support (recommended for production):

**One-time setup:**
```bash
# 1. Enable MCUboot in app/prj.conf
echo "CONFIG_BOOTLOADER_MCUBOOT=y" >> app/prj.conf

# 2. Build with MCUboot
cd app
west build -b nrf52840dk_nrf52840 -- -DEXTRA_CONF_FILE=mcuboot.conf

# 3. Flash bootloader (one time only)
west flash --hex-file build/mcuboot/zephyr/zephyr.hex
```

**Every update:**
```bash
cd app
west build -b nrf52840dk_nrf52840
west sign -t imgtool
./scripts/dfu_update.sh build/zephyr/zephyr.signed.bin
```

---

## Files Created

📁 **docs/DFU_GUIDE.md** - Complete DFU documentation
📁 **docs/QUICK_START_DFU.md** - Quick start guide
📁 **app/mcuboot.conf** - MCUboot configuration
📁 **scripts/build_dfu_package.sh** - Automated build script
📁 **scripts/dfu_update.sh** - Automated update script

Read `docs/QUICK_START_DFU.md` for detailed instructions!

