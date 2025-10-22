# BLE GATT Medical Data Service Guide

## Overview

The Nordic Medical Platform now includes a complete BLE GATT implementation that allows Android apps (like nRF Connect) to connect to the device and receive real-time medical data.

## What Was Implemented

### 1. BLE GATT Services
- **Medical Device Service**: A custom GATT service with 5 characteristics for medical data
- **UUID Base**: `12345678-1234-5678-1234-56789abcdef0`

### 2. GATT Characteristics

| Characteristic | UUID | Data Type | Access | Description |
|---------------|------|-----------|--------|-------------|
| Heart Rate | ...def1 | uint16_t | Read/Notify | Heart rate in bpm |
| Temperature | ...def2 | int16_t | Read/Notify | Temperature in 0.1°C (366 = 36.6°C) |
| SpO2 | ...def3 | uint16_t | Read/Notify | Blood oxygen in 0.1% (980 = 98.0%) |
| Motion | ...def4 | uint16_t | Read/Notify | Motion in 0.1g units |
| All Data | ...def5 | struct | Read/Notify | Combined data packet |

### 3. Connection Handling
- Automatic connection callbacks
- LED indicators for connection status
- Automatic advertising restart on disconnect
- Connection state tracking

### 4. Notifications
- Automatic notifications when data updates
- Individual characteristic notifications
- Combined data notifications
- Efficient binary data format

## How to Test with nRF Connect App

### Step 1: Build and Flash the Firmware

For nRF52840 DK:
```bash
cd app
west build -b nrf52840dk_nrf52840 -p
west flash
```

For QEMU (testing only, no real BLE):
```bash
cd app
west build -b qemu_cortex_m4 -p
west flash
```

### Step 2: Open nRF Connect on Android

1. **Download nRF Connect** from Google Play Store
2. **Enable Bluetooth** on your Android device
3. **Open nRF Connect**

### Step 3: Scan for Devices

1. Tap **SCAN** in nRF Connect
2. Look for **"NISC-Medical-Device"** in the list
3. The device should appear with:
   - Name: `NISC-Medical-Device`
   - RSSI: Signal strength indicator
   - Advertising data visible

### Step 4: Connect to the Device

1. Tap **CONNECT** next to "NISC-Medical-Device"
2. Wait for connection to establish
3. You should see:
   - Connection LED on the device turns ON (LED3)
   - Serial console prints: "BLE device connected!"
   - nRF Connect shows "Connected" status

### Step 5: Explore GATT Services

1. In nRF Connect, scroll down to see services
2. Look for **"Unknown Service"** (custom UUID)
3. Expand the service to see 5 characteristics
4. Each characteristic has a UUID ending in def1, def2, def3, def4, def5

### Step 6: Read Medical Data

For each characteristic:
1. Tap the **DOWN ARROW** (Read) icon
2. You'll see the raw data in hex format
3. Example readings:
   - Heart Rate: `48 00` = 0x0048 = 72 bpm
   - Temperature: `6E 01` = 0x016E = 366 = 36.6°C
   - SpO2: `D4 03` = 0x03D4 = 980 = 98.0%
   - Motion: `0A 00` = 0x000A = 10 = 1.0g

### Step 7: Enable Notifications

1. Tap the **TRIPLE ARROW** (Notify) icon for any characteristic
2. The icon will turn blue when notifications are enabled
3. You'll now receive automatic updates every 15 seconds
4. New data appears automatically without tapping Read

### Step 8: Monitor Real-Time Data

Once notifications are enabled:
- Heart Rate updates every 15 seconds
- Temperature changes gradually
- Motion shows activity bursts
- SpO2 remains stable
- All Data characteristic sends combined packet

## Data Format Details

### Individual Characteristics

All values are little-endian (LSB first):

**Heart Rate (2 bytes)**
```
Bytes: [LSB, MSB]
Example: 48 00 = 72 bpm
```

**Temperature (2 bytes, signed)**
```
Bytes: [LSB, MSB]
Example: 6E 01 = 366 = 36.6°C
Decode: value / 10.0
```

**SpO2 (2 bytes)**
```
Bytes: [LSB, MSB]
Example: D4 03 = 980 = 98.0%
Decode: value / 10.0
```

**Motion (2 bytes)**
```
Bytes: [LSB, MSB]
Example: 0A 00 = 10 = 1.0g
Decode: value / 10.0
```

### All Data Characteristic (8 bytes)

Combined packet structure:
```
Offset | Size | Field       | Example
-------|------|-------------|--------
0      | 2    | Heart Rate  | 48 00
2      | 2    | Temperature | 6E 01
4      | 2    | SpO2        | D4 03
6      | 2    | Motion      | 0A 00
```

## Troubleshooting

### Device Not Appearing

**Problem**: NISC-Medical-Device doesn't show in scan
- **Solution**: Check that Bluetooth is enabled on nRF52840 DK
- **Solution**: Verify firmware flashed successfully
- **Solution**: Check serial console for "Bluetooth advertising started"
- **Solution**: Try power cycling the device

### Can't Connect

**Problem**: Connection fails or disconnects immediately
- **Solution**: Make sure only one device tries to connect
- **Solution**: Clear bonding data in nRF Connect
- **Solution**: Restart both the nRF52840 and Android device
- **Solution**: Check for CONFIG_BT_MAX_CONN=1 in prj.conf

### No Services Visible

**Problem**: Connected but no GATT services shown
- **Solution**: Wait a few seconds for service discovery
- **Solution**: Tap "Refresh" in nRF Connect
- **Solution**: Disconnect and reconnect
- **Solution**: Check console for GATT initialization errors

### Notifications Not Working

**Problem**: Can read data but notifications don't arrive
- **Solution**: Verify you enabled notifications (icon should be blue)
- **Solution**: Check that characteristic has NOTIFY property
- **Solution**: Ensure medical data is being updated (check console)
- **Solution**: Try disabling and re-enabling notifications

### Data Appears Incorrect

**Problem**: Values don't make sense
- **Solution**: Remember data is in 0.1 units (divide by 10)
- **Solution**: Data is little-endian (LSB first)
- **Solution**: Temperature is signed int16_t
- **Solution**: Use hex viewer in nRF Connect for raw values

## Serial Console Output

When testing, monitor the serial console to see:

```
=== NISC Medical Wearable Device Starting ===
...
Bluetooth advertising started - Device discoverable
...
TRANSMITTING MEDICAL DATA PACKET #1
Via: Bluetooth Low Energy (BLE GATT)
Device Name: NISC-Medical-Device
Status: Advertising - Waiting for connection...
...
BLE device connected!
...
Status: CONNECTED - Data transmitted via GATT notifications
```

## LED Indicators

| LED | Pattern | Meaning |
|-----|---------|---------|
| LED1 (Status) | Breathing | System OK |
| LED2 (Heartbeat) | Medical pulse | Heart rate visualization |
| LED3 (Comm) | ON (solid) | BLE connected |
| LED3 (Comm) | Blinking | Advertising/no connection |
| LED4 (Error) | OFF | No errors |

## Configuration Files Modified

1. **app/prj.conf**: Added BLE GATT configuration
   - `CONFIG_BT_GATT_SERVICE_CHANGED=y`
   - `CONFIG_BT_SETTINGS=y`
   - `CONFIG_BT_GATT_DYNAMIC_DB=y`

2. **app/src/hardware.c**: Added GATT service implementation
   - Connection callbacks
   - 5 GATT characteristics
   - Notification support
   - Medical data updates

3. **app/src/hardware.h**: Added public API
   - `hw_ble_update_medical_data()`
   - `hw_ble_is_connected()`
   - `hw_ble_notify_characteristic()`

4. **app/src/main.c**: Integrated with communication thread
   - Calls `hw_ble_update_medical_data()` every 15 seconds
   - Shows connection status
   - Updates medical data automatically

## Next Steps

### For Mobile App Development

To create a custom Android app:

1. **Use Android BLE APIs**:
   ```java
   // Scan for devices
   bluetoothLeScanner.startScan(scanCallback);
   
   // Connect
   bluetoothGatt = device.connectGatt(context, false, gattCallback);
   
   // Discover services
   bluetoothGatt.discoverServices();
   
   // Read characteristic
   bluetoothGatt.readCharacteristic(characteristic);
   
   // Enable notifications
   bluetoothGatt.setCharacteristicNotification(characteristic, true);
   ```

2. **Service UUID**: `12345678-1234-5678-1234-56789abcdef0`

3. **Parse Data**:
   ```java
   // Heart rate (uint16_t)
   int heartRate = characteristic.getIntValue(FORMAT_UINT16, 0);
   
   // Temperature (int16_t)
   int tempRaw = characteristic.getIntValue(FORMAT_SINT16, 0);
   float temperature = tempRaw / 10.0f;
   
   // SpO2 (uint16_t)
   int spo2Raw = characteristic.getIntValue(FORMAT_UINT16, 0);
   float spo2 = spo2Raw / 10.0f;
   ```

### For iOS Development

Use CoreBluetooth framework:
```swift
// Scan
centralManager.scanForPeripherals(withServices: [medicalServiceUUID])

// Connect
centralManager.connect(peripheral)

// Enable notifications
peripheral.setNotifyValue(true, for: characteristic)
```

## Security Considerations

The current implementation has **no security** enabled. For production use:

1. **Enable Pairing**:
   ```
   CONFIG_BT_SMP=y
   CONFIG_BT_SIGNING=y
   ```

2. **Add Encryption**:
   ```
   CONFIG_BT_L2CAP_TX_BUF_COUNT=8
   CONFIG_BT_ATT_PREPARE_COUNT=4
   ```

3. **Implement Authentication**:
   - Add passkey display
   - Implement out-of-band pairing
   - Use numeric comparison

4. **Medical Device Compliance**:
   - Follow IEC 62304
   - Implement secure boot
   - Add data encryption
   - Audit logging

## Summary

You now have a fully functional BLE GATT service that:
✅ Advertises as "NISC-Medical-Device"
✅ Accepts connections from Android/iOS apps
✅ Provides 5 GATT characteristics for medical data
✅ Sends automatic notifications when data updates
✅ Shows connection status on LEDs
✅ Transmits real-time heart rate, temperature, SpO2, and motion data

Test it with nRF Connect to see your medical data streaming in real-time!

