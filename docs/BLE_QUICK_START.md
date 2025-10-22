# BLE Quick Start Guide

## 🚀 Quick Start: Connect to Medical Device via Bluetooth

### What You Need
- nRF52840 Development Kit (or compatible Nordic board)
- Android phone with nRF Connect app installed
- USB cable for programming and power

### 5-Minute Setup

#### 1. Flash the Firmware (2 min)
```bash
cd app
west build -b nrf52840dk_nrf52840 -p
west flash
```

#### 2. Install nRF Connect (1 min)
- Download **nRF Connect for Mobile** from Google Play Store
- Grant Bluetooth and Location permissions

#### 3. Connect (2 min)
1. Open nRF Connect app
2. Tap **SCAN**
3. Find **"NISC-Medical-Device"**
4. Tap **CONNECT**
5. Wait for "Connected" status

#### 4. View Medical Data (1 min)
1. Expand the "Unknown Service" 
2. Tap **DOWN ARROW** to read any characteristic
3. Tap **TRIPLE ARROW** to enable notifications
4. Watch medical data update automatically!

### Understanding the Data

| Characteristic | What It Shows | Example Reading |
|---------------|---------------|-----------------|
| Heart Rate | Beats per minute | 72 bpm |
| Temperature | Body temperature | 36.6°C |
| SpO2 | Blood oxygen | 98.0% |
| Motion | Activity level | 1.0g |
| All Data | Everything at once | All 4 values |

### Raw Data Format

The values are shown in hex (little-endian):
- `48 00` = 72 bpm (heart rate)
- `6E 01` = 366 → 36.6°C (divide by 10)
- `D4 03` = 980 → 98.0% (divide by 10)
- `0A 00` = 10 → 1.0g (divide by 10)

### Troubleshooting

**Can't find device?**
- Check serial console shows "Bluetooth advertising started"
- Power cycle the nRF52840 DK
- Ensure Bluetooth is enabled on phone

**Connection fails?**
- Only one device can connect at a time
- Try clearing bonded devices in nRF Connect
- Restart both devices

**No data showing?**
- Wait a few seconds for service discovery
- Try the "Refresh" button
- Check that notifications are enabled (blue icon)

### What's Happening Behind the Scenes

The device:
1. ✅ Advertises as "NISC-Medical-Device"
2. ✅ Accepts your connection
3. ✅ Provides GATT service with 5 characteristics
4. ✅ Sends medical data notifications every 15 seconds
5. ✅ Shows connection status on LED3 (solid = connected)

### Next Steps

- See [BLE_GATT_GUIDE.md](BLE_GATT_GUIDE.md) for detailed documentation
- Build your own Android/iOS app using the GATT service
- Customize the medical data characteristics
- Add security/pairing for production use

---

**That's it!** You now have a working BLE medical device that sends real-time data to your phone! 🎉

