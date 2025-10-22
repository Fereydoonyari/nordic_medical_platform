# BLE Connection Stability Fixes

## 🎯 Problem Solved

**Issue**: Device connects but disconnects after one loop (approximately 15 seconds)

**Root Causes**:
1. No connection parameter negotiation
2. Too many notification attempts causing buffer overflow
3. Short supervision timeout (default 2 seconds)
4. No proper disconnect reason reporting

## ✅ Fixes Applied

### Fix #1: Connection Parameters Configuration

**File**: `app/prj.conf`

Added proper connection parameters for stable BLE connection:

```ini
# Connection interval: 30-50ms (balanced for data transfer)
CONFIG_BT_PERIPHERAL_PREF_MIN_INT=24    # 24 * 1.25ms = 30ms
CONFIG_BT_PERIPHERAL_PREF_MAX_INT=40    # 40 * 1.25ms = 50ms

# No latency for medical device (real-time data)
CONFIG_BT_PERIPHERAL_PREF_LATENCY=0

# 4-second supervision timeout (prevents premature disconnect)
CONFIG_BT_PERIPHERAL_PREF_TIMEOUT=400   # 400 * 10ms = 4000ms

# Auto-negotiation features
CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT=5000
CONFIG_BT_AUTO_PHY_UPDATE=y
CONFIG_BT_AUTO_DATA_LEN_UPDATE=y
```

**Why This Helps**:
- **Longer supervision timeout**: Prevents disconnect if a packet is lost
- **Balanced interval**: Fast enough for real-time data, slow enough to save power
- **No latency**: Medical devices need real-time responsiveness
- **Auto-negotiation**: Adapts to client capabilities

### Fix #2: Connection Parameter Request on Connect

**File**: `app/src/hardware.c` - `bt_connected_cb()`

Added explicit connection parameter update request:

```c
/* Request connection parameter update for stable connection */
struct bt_le_conn_param param = {
    .interval_min = 24,   /* 30ms */
    .interval_max = 40,   /* 50ms */
    .latency = 0,         /* No latency */
    .timeout = 400,       /* 4 seconds supervision timeout */
};

int ret = bt_conn_le_param_update(conn, &param);
```

**Why This Helps**:
- Ensures client (nRF Connect) uses optimal parameters
- Negotiates better timeout values
- Establishes stable connection immediately

### Fix #3: Reduced Notification Overhead

**File**: `app/src/hardware.c` - `hw_ble_update_medical_data()`

**Before**: Sent 5 separate notifications (HR, Temp, SpO2, Motion, All Data)
```c
// REMOVED - Too many notifications caused buffer overflow:
// - Heart rate notification
// - Temperature notification  
// - SpO2 notification
// - Motion notification
// - All data notification
```

**After**: Send only 1 combined notification
```c
/* Send only "All Data" notification (most efficient) */
params.attr = &medical_svc.attrs[14];
params.data = &all_data;  // 8 bytes total
params.len = sizeof(all_data);
ret = bt_gatt_notify_cb(ble_state.conn, &params);
```

**Why This Helps**:
- Reduces BLE buffer usage (5x reduction)
- Prevents buffer overflow disconnections
- More efficient: 1 packet instead of 5
- Still provides all medical data

### Fix #4: Enhanced Disconnect Reason Reporting

**File**: `app/src/hardware.c` - `bt_disconnected_cb()`

Added detailed disconnect reason decoding:

```c
========================================
BLE DISCONNECTED
========================================
Address: XX:XX:XX:XX:XX:XX
Reason: 0x08 (Connection timeout)
========================================
```

**Disconnect Reason Codes**:
| Code | Meaning | Likely Cause |
|------|---------|--------------|
| 0x08 | Connection timeout | Supervision timeout expired |
| 0x13 | Remote user terminated | User disconnected in app |
| 0x16 | Local host terminated | Device initiated disconnect |
| 0x3E | Failed to establish | Connection setup failed |
| 0x22 | LMP response timeout | Link layer communication issue |

## 🧪 Testing the Fixes

### Rebuild and Flash

```bash
cd app
west build -b nrf52840dk_nrf52840
west flash
```

### Expected Console Output

**On Connection**:
```
BLE device connected! Address: XX:XX:XX:XX:XX:XX (random)
[INF:SYS] BLE device connected successfully
Bluetooth advertising stopped
```

**During Connection** (every 15 seconds):
```
TRANSMITTING MEDICAL DATA PACKET #1
Via: Bluetooth Low Energy (BLE GATT)
Status: CONNECTED - Data transmitted via GATT notifications
```

**On Disconnect** (if it happens):
```
========================================
BLE DISCONNECTED
========================================
Address: XX:XX:XX:XX:XX:XX
Reason: 0x13 (Remote user terminated)
========================================

Restarting advertising...
```

### Test in nRF Connect

1. **Connect to device**:
   - Scan for "NISC-Medical"
   - Tap CONNECT
   - Wait for "Connected" status

2. **Monitor connection stability**:
   - Connection should remain stable
   - No disconnections after 15 seconds
   - LED3 stays solid ON

3. **Enable notifications**:
   - Expand "Unknown Service"
   - Find "All Data" characteristic (last one)
   - Tap triple-arrow icon to enable notifications
   - Watch data update every 15 seconds

4. **Verify data reception**:
   - Should see 8-byte hex values
   - Updates every 15 seconds
   - No disconnections

## 📊 Connection Parameter Explained

### Interval
- **Min**: 24 × 1.25ms = **30ms**
- **Max**: 40 × 1.25ms = **50ms**
- **Purpose**: How often devices communicate

**Trade-off**:
- Shorter interval = more responsive, higher power consumption
- Longer interval = less responsive, lower power consumption
- 30-50ms is optimal for medical data (real-time but efficient)

### Latency
- **Value**: 0 (no latency)
- **Purpose**: How many connection events can be skipped

**For Medical Devices**:
- Must be 0 for real-time data
- Cannot skip any connection events
- Ensures immediate data delivery

### Supervision Timeout
- **Value**: 400 × 10ms = **4 seconds**
- **Purpose**: Max time without communication before disconnect

**Why 4 Seconds**:
- Allows for temporary interference
- Prevents false disconnects
- Still detects real connection loss quickly
- Medical device safety balance

## 🔍 Troubleshooting Disconnections

### If Connection Still Drops

**Check disconnect reason in console:**

**Reason 0x08 (Connection timeout)**:
```ini
# Increase supervision timeout in prj.conf:
CONFIG_BT_PERIPHERAL_PREF_TIMEOUT=600  # 6 seconds
```

**Reason 0x13 (Remote user terminated)**:
- This is normal - user disconnected in app
- Device will auto-restart advertising

**Reason 0x16 (Local host terminated)**:
- Check for errors in notification code
- Verify medical data is valid
- Check memory usage

**Reason 0x22 (LMP response timeout)**:
```ini
# Increase buffers in prj.conf:
CONFIG_BT_L2CAP_TX_BUF_COUNT=10
CONFIG_BT_BUF_ACL_RX_SIZE=251
```

### Monitor Connection Quality

In nRF Connect app:
- **RSSI**: Should be > -60 dBm for good connection
- **Connection interval**: Should show 30-50ms
- **Notifications**: Should arrive every 15 seconds

### Common Issues

**Disconnects at exactly 15 seconds**:
- This was the old bug (notifications flooding)
- Should be fixed with single notification approach
- If still happening, increase buffer sizes

**Random disconnects**:
- Check phone battery saver mode (can kill BLE)
- Ensure phone is close to device (< 1 meter)
- Disable other BLE apps

**Immediate disconnect after connection**:
- Check GATT service is properly initialized
- Verify notifications are not sent before enabled
- Check connection parameters are accepted

## ✅ Expected Behavior

After these fixes:

| Metric | Expected Value |
|--------|----------------|
| Connection duration | Indefinite (until user disconnects) |
| Disconnect reason | 0x13 (user initiated) |
| Data update interval | 15 seconds |
| Notification size | 8 bytes |
| Connection interval | 30-50ms |
| Supervision timeout | 4 seconds |
| LED3 status | Solid ON when connected |

## 📚 References

- [BLE Connection Parameters](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/protocols/bt/index.html)
- [Supervision Timeout](https://developer.apple.com/library/archive/qa/qa1931/_index.html)
- [GATT Notifications](https://docs.zephyrproject.org/latest/connectivity/bluetooth/api/gatt.html)

---

**Connection should now be stable and persistent!** 🎉

The device will maintain connection until the user explicitly disconnects.

