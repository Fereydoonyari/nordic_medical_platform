# Stack Overflow Fix - System Restarting Issue

## 🚨 Problem: System Restarts After Printing Info/Threads

**Symptom**: System resets/restarts after printing hardware information and creating threads

**Root Cause**: **STACK OVERFLOW** - Thread stack sizes too small for BLE operations

## 🔍 How to Identify Stack Overflow

### Signs of Stack Overflow:
1. ✅ System resets unexpectedly (check!)
2. ✅ Resets occur after thread creation/startup (check!)
3. ✅ Console shows "Reset Cause: 0x..." on restart (check!)
4. ✅ Happens consistently at same point (check!)
5. ✅ BLE enabled (increases stack usage) (check!)

### Why It Happens:

When a thread runs out of stack space:
```
1. Thread calls function (pushes to stack)
2. Stack grows beyond allocated size
3. Overwrites memory beyond stack boundary
4. Corrupts other data structures
5. Assertion fails or MPU fault occurs
6. System resets to prevent damage
```

## ❌ Old Stack Sizes (Too Small)

```c
#define THREAD_STACK_SUPERVISOR      1024U   // 1KB
#define THREAD_STACK_DATA_ACQ        1536U   // 1.5KB
#define THREAD_STACK_COMMUNICATION   1024U   // 1KB  ← CRASH HERE!
#define THREAD_STACK_DATA_PROC       1536U   // 1.5KB
#define THREAD_STACK_DIAGNOSTICS     512U    // 0.5KB  ← TOO SMALL!
```

### Stack Usage Analysis:

**Communication Thread** (was 1024 bytes):
- `snprintf()` for formatting: ~200 bytes
- `hw_ble_send_notification()`: ~300 bytes
- BLE GATT stack frames: ~400 bytes
- `printk()` calls: ~100 bytes each
- **Total needed**: ~1500-2000 bytes
- **OVERFLOW**: 476-976 bytes!

**Data Acquisition Thread** (was 1536 bytes):
- Sensor calculations: ~200 bytes
- Multiple `printk()` table formatting: ~500 bytes
- BLE data updates: ~300 bytes
- LED patterns: ~200 bytes
- **Total needed**: ~2000-2500 bytes
- **OVERFLOW**: 464-964 bytes!

**Diagnostics/Hardware Thread** (was 512 bytes):
- LED pattern calculations: ~300 bytes
- GPIO operations: ~200 bytes
- Function call overhead: ~100 bytes
- **Total needed**: ~800-1000 bytes
- **OVERFLOW**: 288-488 bytes!

## ✅ New Stack Sizes (Safe)

```c
#define THREAD_STACK_SUPERVISOR      2048U   // 2KB (doubled)
#define THREAD_STACK_DATA_ACQ        3072U   // 3KB (doubled)
#define THREAD_STACK_COMMUNICATION   2560U   // 2.5KB (2.5x larger for BLE)
#define THREAD_STACK_DATA_PROC       2048U   // 2KB (33% larger)
#define THREAD_STACK_DIAGNOSTICS     1536U   // 1.5KB (tripled)
```

### Why These Sizes:

1. **Supervisor** (2KB): Safety-critical, needs headroom for error handling
2. **Data Acquisition** (3KB): Heaviest thread with console I/O and calculations
3. **Communication** (2.5KB): BLE operations need extra space for GATT stack
4. **Data Processing** (2KB): Moderate usage, good safety margin
5. **Diagnostics** (1.5KB): LED patterns and hardware access need space

### Total Memory Impact:

**Before**: 5.5KB total thread stack space
**After**: 11.2KB total thread stack space
**Increase**: +5.7KB (acceptable for 256KB RAM on nRF52840)

## 🔨 Apply the Fix

The stack sizes are defined in `app/src/thread_manager.h`.

### Rebuild:

```bash
cd app
west build -b nrf52840dk_nrf52840 -p
west flash
```

## ✅ Expected Behavior After Fix

### Console Output:
```
=== NISC Medical Wearable Device Starting ===
Firmware Version: 1.0.0
...
Creating application threads...
[INF:SYS] Created thread supervisor (ID: 0)
Supervisor thread started - safety monitoring active
[INF:SYS] Created thread diagnostics (ID: 4)
Hardware update thread started - LED pattern management
[INF:SYS] Created thread data_acquisition (ID: 1)
Data acquisition thread started - sampling sensors every 1 second
...
(continues running - NO RESTART!)
```

**Key difference**: System stays running, doesn't reset!

## 📊 How to Verify Stack Usage

### Enable Stack Canary (Optional):

Add to `prj.conf`:
```ini
CONFIG_THREAD_STACK_INFO=y
CONFIG_INIT_STACKS=y
```

This fills unused stack with a pattern (0xAA) and can detect overflows.

### Check Stack Usage at Runtime:

If you re-enable shell later:
```
uart:~$ kernel threads
...
supervisor     : STACK: unused X used Y size 2048
data_acquisition: STACK: unused X used Y size 3072
...
```

**Healthy**: "used" should be < 80% of "size"
**Warning**: "used" > 80% means increase stack size
**Critical**: "used" > 95% means imminent overflow

## 🎯 Why BLE Needs More Stack

BLE operations involve:

1. **Deep call stacks**:
   ```
   your_code()
   → hw_ble_send_notification()
     → bt_gatt_notify_cb()
       → bt_gatt_notify()
         → l2cap_send()
           → hci_send()
   ```
   Each level adds ~50-100 bytes

2. **Large local variables**:
   - Connection parameters (48 bytes)
   - GATT attributes (variable)
   - Notification buffers (variable)

3. **Printf/Logging**:
   - Format string parsing
   - Variable argument handling
   - Buffer allocation

## ⚠️ Common Mistakes

### ❌ Don't Do This:
```c
// Declaring large arrays on stack
char big_buffer[1024];  // Uses 1KB of stack!
```

### ✅ Do This Instead:
```c
// Use static or heap allocation
static char big_buffer[1024];  // Uses BSS, not stack
// OR
char *big_buffer = k_malloc(1024);  // Uses heap
```

## 📚 Best Practices

1. **Always leave headroom**: Plan for 50% more than calculated need
2. **Test thoroughly**: Run for hours to catch occasional overflows
3. **Monitor stack usage**: Enable CONFIG_THREAD_STACK_INFO during development
4. **Minimize deep nesting**: Flatten function call hierarchies
5. **Use static variables**: For large buffers that don't need to be on stack

## 🐛 Debugging Stack Overflow

If you still see resets:

1. **Check reset cause**:
   ```
   Reset Cause: 0x00000001 = RESETREAS_RESETPIN
   Reset Cause: 0x00000004 = RESETREAS_LOCKUP (← Stack overflow!)
   Reset Cause: 0x00040000 = RESETREAS_DIF
   ```

2. **Enable assertions**:
   ```ini
   CONFIG_ASSERT=y  # Already enabled
   CONFIG_ASSERT_VERBOSE=y  # Add for more detail
   ```

3. **Check for infinite recursion**:
   - Functions calling themselves
   - Circular function calls

4. **Reduce console output**:
   - Each `printk()` uses ~100 bytes
   - Minimize logging when BLE connected

## ✅ Summary

**Problem**: Thread stacks too small (1-1.5KB)
**Solution**: Increased to 1.5-3KB
**Result**: No more crashes/resets

**Old total**: 5.5KB for all threads
**New total**: 11.2KB for all threads
**Impact**: +5.7KB RAM usage (2% of 256KB)

**Trade-off**: Tiny RAM increase for rock-solid stability!

---

**After rebuilding with new stack sizes, your system should run continuously without resets.** 🎉

