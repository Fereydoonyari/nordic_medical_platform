# BLE Troubleshooting Guide

## ❌ Problem: Bluetooth Not Discoverable

If your device is not appearing in nRF Connect or other BLE scanner apps, follow this systematic troubleshooting guide.

## 🔍 Step-by-Step Debugging

### Step 1: Check Serial Console Output

Connect via USB and monitor the serial console. You should see:

```
=== NISC Medical Wearable Device Starting ===
...
Initializing Bluetooth advertising...
Enabling Bluetooth...
Bluetooth enabled successfully
Bluetooth advertising initialized successfully
...
Starting BLE advertising as 'NISC-Medical-Device'...
✓ Bluetooth advertising started successfully - Device should be discoverable now!
  Device Name: NISC-Medical-Device
  Look for it in nRF Connect app
```

**What to Check**:
- ✅ "Bluetooth enabled successfully" appears
- ✅ "Bluetooth advertising started successfully" appears
- ✅ No error messages about Bluetooth

**If you see errors**:
- Note the error code (e.g., "Bluetooth enable failed with error -22")
- See error code reference below

### Step 2: Verify Build Configuration

Check that your `prj.conf` has these settings:

```ini
# Essential BLE settings
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_BROADCASTER=y
CONFIG_BT_DEVICE_NAME="NISC-Medical-Device"
CONFIG_BT_GATT_SERVICE_CHANGED=y

# Memory (must be sufficient)
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=4096

# BLE buffers
CONFIG_BT_L2CAP_TX_BUF_COUNT=8
CONFIG_BT_ATT_PREPARE_COUNT=2
```

### Step 3: Rebuild with Clean Build

```bash
cd app
rm -rf build
west build -b nrf52840dk_nrf52840 -p
west flash
```

The `-p` flag ensures a pristine build.

### Step 4: Check Board Connection

**nRF52840 DK**:
- USB cable connected to "nRF USB" port (not external power)
- Power LED should be ON
- Reset button (SW1) pressed after flashing
- No other USB cables connected

**Button Check**:
- Press Button 1 - LED should respond
- If no response, power cycle the board

### Step 5: Check Android Settings

**Bluetooth**:
- Bluetooth must be ON
- Location services must be ON (Android requirement for BLE scanning)
- Grant nRF Connect all permissions

**nRF Connect App**:
- Latest version installed
- Permissions granted: Bluetooth, Location, Nearby Devices
- Try force-closing and reopening the app
- Clear app cache if issues persist

### Step 6: Test on Different Device

Try scanning with:
- Different Android phone
- iOS device with LightBlue app
- Another nRF52840 DK running nRF Connect Desktop
- Raspberry Pi with `hcitool lescan`

## 🐛 Common Error Codes

### Error -12 (ENOMEM)
**Meaning**: Out of memory

**Solutions**:
1. Increase stack sizes in `prj.conf`:
   ```ini
   CONFIG_MAIN_STACK_SIZE=8192
   CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=8192
   ```
2. Increase BLE buffer count:
   ```ini
   CONFIG_BT_L2CAP_TX_BUF_COUNT=10
   CONFIG_BT_BUF_ACL_RX_SIZE=251
   CONFIG_BT_BUF_ACL_TX_SIZE=251
   ```

### Error -22 (EINVAL)
**Meaning**: Invalid parameter

**Solutions**:
1. Check device name length (must be < 30 characters)
2. Verify advertising data format
3. Ensure CONFIG_BT_DEVICE_NAME matches code

### Error -5 (EIO)
**Meaning**: I/O error, hardware issue

**Solutions**:
1. Check board is nRF52840 (not nRF52832)
2. Verify correct board target in build
3. Flash SoftDevice if using older SDK
4. Power cycle the board

### Error -114 (EALREADY)
**Meaning**: Bluetooth already enabled

**Solutions**:
1. This is usually harmless
2. Code might be calling bt_enable() twice
3. Check initialization order

## 🔧 Advanced Debugging

### Enable Full BLE Debug Logging

Add to `prj.conf`:
```ini
CONFIG_BT_DEBUG_LOG=y
CONFIG_BT_DEBUG_HCI_CORE=y
CONFIG_LOG_MODE_IMMEDIATE=y
CONFIG_LOG_DEFAULT_LEVEL=4
```

Rebuild and check console for detailed BLE stack messages.

### Check with West Build Verbose

```bash
west build -b nrf52840dk_nrf52840 -p -- -DCMAKE_VERBOSE_MAKEFILE=ON
```

Look for compilation warnings about BLE.

### Verify Zephyr SDK Version

```bash
west --version
```

Should be 3.0 or newer. Update if older:
```bash
west update
```

### Test Minimal BLE Sample

Test with Zephyr's minimal BLE peripheral sample:
```bash
cd ~/zephyrproject/zephyr
west build -b nrf52840dk_nrf52840 samples/bluetooth/peripheral -p
west flash
```

If this doesn't work, it's a toolchain/SDK issue, not your code.

## 📊 Expected Behavior Checklist

When everything is working:

- [ ] Serial console shows "Bluetooth enabled successfully"
- [ ] Serial console shows "Bluetooth advertising started successfully"
- [ ] LED3 (Communication) is blinking slowly
- [ ] nRF Connect shows "NISC-Medical-Device" in scan list
- [ ] Device shows as "Connectable" in nRF Connect
- [ ] RSSI value appears (signal strength)
- [ ] Can tap "CONNECT" button

## 🎯 Quick Fixes

### Fix 1: Remove Settings Storage

If you see errors about settings/flash, disable them:

```ini
# In prj.conf, comment out:
# CONFIG_BT_SETTINGS=y
# CONFIG_SETTINGS=y
# CONFIG_FLASH=y
# CONFIG_NVS=y
```

These are optional and can cause issues.

### Fix 2: Use Simpler Advertising

Replace advertising code with basic parameters:

```c
// In hw_ble_advertising_start()
int ret = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
```

This uses standard connectable advertising.

### Fix 3: Check Board Overlay

Ensure `nrf52840dk_nrf52840.overlay` doesn't disable Bluetooth pins or peripherals.

### Fix 4: Flash Bootloader

Some boards need MCUboot bootloader:

```bash
west build -b nrf52840dk_nrf52840 -p -- -DCONFIG_BOOTLOADER_MCUBOOT=y
west flash
```

### Fix 5: Erase Flash Completely

```bash
nrfjprog --eraseall
west flash
```

This clears any bad settings stored in flash.

## 📞 Getting More Help

If still not working:

1. **Capture Full Console Log**:
   ```bash
   west build -b nrf52840dk_nrf52840 -p
   west flash
   # Copy all console output to file
   ```

2. **Check Zephyr Documentation**:
   - https://docs.zephyrproject.org/latest/connectivity/bluetooth/index.html

3. **Nordic DevZone**:
   - https://devzone.nordicsemi.com/

4. **Zephyr Discord**:
   - https://discord.gg/zephyr

## 📋 Information to Provide

When asking for help, include:

1. **Hardware**: nRF52840 DK Rev X.X.X
2. **Zephyr Version**: `west --version` output
3. **Build Command**: Full command used
4. **Console Output**: Complete log from power-on
5. **Error Code**: Specific error number if any
6. **Phone Details**: Android/iOS version, app version

## ✅ Success Indicators

You know it's working when:

1. Console shows success messages
2. LED3 blinks slowly (advertising)
3. Device appears in nRF Connect scan
4. Can connect and see GATT services
5. LED3 goes solid when connected
6. Can read characteristics

---

**Most Common Issues**:
1. ⚠️ **Stack size too small** → Increase to 4096+
2. ⚠️ **Settings storage failing** → Disable CONFIG_BT_SETTINGS
3. ⚠️ **Wrong board target** → Use nrf52840dk_nrf52840
4. ⚠️ **Android location off** → Must enable for BLE scanning
5. ⚠️ **Dirty build** → Always use `-p` for clean builds

Follow this guide systematically and you'll find the issue! 🎯

