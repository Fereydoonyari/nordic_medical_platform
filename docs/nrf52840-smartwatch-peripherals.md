# nRF52840 Peripherals 

### Target Platform
- **MCU**: nRF52840 (ARM Cortex-M4F @ 64MHz)
- **Memory**: 1MB Flash, 256KB RAM
- **RTOS**: Zephyr Real-Time Operating System
- **Connectivity**: Bluetooth 5.0, IEEE 802.15.4
- **Power**: Ultra-low power with multiple sleep modes

---

## 1. OLED Display (SSD1306/SH1106)

### Overview
OLED displays are essential for smartwatch user interfaces, providing low power consumption and excellent visibility.

### Specifications
- **Common Models**: SSD1306, SH1106
- **Resolution**: 128x64, 128x32 pixels
- **Interface**: I2C, SPI
- **Power**: 3.3V, ~20mA active, <1µA sleep
- **Viewing Angle**: >160°
- **Temperature**: -40°C to +85°C

### Hardware Connection (I2C)
```
nRF52840    SSD1306
---------   -------
VCC      -> VCC (3.3V)
GND      -> GND
P0.26    -> SCL (I2C Clock)
P0.27    -> SDA (I2C Data)
```

### Device Tree Configuration
```dts
&i2c0 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>; // 400kHz
    
    ssd1306: ssd1306@3c {
        compatible = "solomon,ssd1306fb";
        reg = <0x3c>;
        width = <128>;
        height = <64>;
        segment-offset = <0>;
        page-offset = <0>;
        display-offset = <0>;
        multiplex-ratio = <63>;
        segment-remap;
        com-invdir;
        com-sequential;
        prechargep = <0x22>;
    };
};
```

### Zephyr Configuration (prj.conf)
```ini
CONFIG_I2C=y
CONFIG_DISPLAY=y
CONFIG_SSD1306=y
CONFIG_LVGL=y
CONFIG_LVGL_DISPLAY_DEV_NAME="SSD1306"
```

### C Code Example
```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(oled_display, LOG_LEVEL_DBG);

#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, solomon_ssd1306fb))

static const struct device *display_dev;

int oled_init(void)
{
    display_dev = device_get_binding(DISPLAY_DEV_NAME);
    if (!display_dev) {
        LOG_ERR("Display device not found");
        return -ENODEV;
    }

    // Clear display
    display_blanking_off(display_dev);
    
    LOG_INF("OLED display initialized successfully");
    return 0;
}

int oled_write_text(const char *text, uint16_t x, uint16_t y)
{
    struct display_buffer_descriptor desc;
    
    desc.buf_size = strlen(text) * 8; // 8 pixels per character
    desc.width = strlen(text) * 8;
    desc.height = 8;
    desc.pitch = strlen(text) * 8;
    
    return display_write(display_dev, x, y, &desc, text);
}

void oled_clear_screen(void)
{
    struct display_buffer_descriptor desc;
    uint8_t buffer[128 * 64 / 8] = {0}; // Clear buffer
    
    desc.buf_size = sizeof(buffer);
    desc.width = 128;
    desc.height = 64;
    desc.pitch = 128;
    
    display_write(display_dev, 0, 0, &desc, buffer);
}

// Usage example
void display_heart_rate(uint16_t bpm)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "HR: %d BPM", bpm);
    
    oled_clear_screen();
    oled_write_text(buffer, 10, 20);
}
```

---

## 2. Heart Rate & SpO2 Sensor (MAX30102)

### Overview
The MAX30102 is an integrated pulse oximetry and heart rate monitor sensor, perfect for wearable health devices.

### Specifications
- **Measurement**: Heart rate, SpO2 (Blood oxygen saturation)
- **Interface**: I2C (up to 400kHz)
- **Supply Voltage**: 1.8V - 3.3V
- **Current**: 600µA to 50mA (depending on LED current)
- **Wavelengths**: 660nm (Red), 880nm (IR)
- **Resolution**: 18-bit ADC
- **Sample Rate**: 50Hz - 3200Hz

### Hardware Connection
```
nRF52840    MAX30102
---------   --------
VCC      -> VIN (3.3V)
GND      -> GND
P0.26    -> SCL (I2C Clock)
P0.27    -> SDA (I2C Data)
P0.28    -> INT (Optional interrupt)
```

### Device Tree Configuration
```dts
&i2c0 {
    max30102: max30102@57 {
        compatible = "maxim,max30102";
        reg = <0x57>;
        int-gpios = <&gpio0 28 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
    };
};
```

### Zephyr Configuration (prj.conf)
```ini
CONFIG_I2C=y
CONFIG_SENSOR=y
CONFIG_MAX30102=y
CONFIG_GPIO=y
```

### C Code Example
```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(max30102, LOG_LEVEL_DBG);

// MAX30102 I2C address
#define MAX30102_I2C_ADDR 0x57

// Register definitions
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C
#define MAX30102_REG_LED2_PA        0x0D
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07

static const struct device *i2c_dev;

struct max30102_data {
    uint32_t red_samples[100];
    uint32_t ir_samples[100];
    uint16_t sample_count;
    bool sensor_ready;
};

static struct max30102_data sensor_data;

int max30102_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_write(i2c_dev, data, sizeof(data), MAX30102_I2C_ADDR);
}

int max30102_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_write_read(i2c_dev, MAX30102_I2C_ADDR, &reg, 1, value, 1);
}

int max30102_init(void)
{
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    // Reset sensor
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x40);
    k_msleep(100);

    // Configure sensor
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x03);    // Heart rate + SpO2 mode
    max30102_write_reg(MAX30102_REG_SPO2_CONFIG, 0x27);    // 100Hz sample rate, 411µs pulse width
    max30102_write_reg(MAX30102_REG_LED1_PA, 0x24);        // Red LED current
    max30102_write_reg(MAX30102_REG_LED2_PA, 0x24);        // IR LED current

    sensor_data.sensor_ready = true;
    LOG_INF("MAX30102 sensor initialized");
    return 0;
}

int max30102_read_fifo(uint32_t *red, uint32_t *ir)
{
    uint8_t wr_ptr, rd_ptr;
    uint8_t samples_available;
    uint8_t fifo_data[6];
    
    max30102_read_reg(MAX30102_REG_FIFO_WR_PTR, &wr_ptr);
    max30102_read_reg(MAX30102_REG_FIFO_RD_PTR, &rd_ptr);
    
    samples_available = (wr_ptr - rd_ptr) & 0x1F;
    
    if (samples_available > 0) {
        uint8_t reg = MAX30102_REG_FIFO_DATA;
        i2c_write_read(i2c_dev, MAX30102_I2C_ADDR, &reg, 1, fifo_data, 6);
        
        *red = ((uint32_t)fifo_data[0] << 16) | 
               ((uint32_t)fifo_data[1] << 8) | 
               fifo_data[2];
        *red &= 0x03FFFF; // 18-bit resolution
        
        *ir = ((uint32_t)fifo_data[3] << 16) | 
              ((uint32_t)fifo_data[4] << 8) | 
              fifo_data[5];
        *ir &= 0x03FFFF;   // 18-bit resolution
        
        return 1; // One sample read
    }
    
    return 0; // No samples available
}

// Simple heart rate calculation using peak detection
uint16_t calculate_heart_rate(uint32_t *samples, uint16_t count, uint16_t sample_rate)
{
    if (count < 20) return 0;
    
    uint16_t peaks = 0;
    uint32_t threshold = 0;
    
    // Calculate average as threshold
    for (int i = 0; i < count; i++) {
        threshold += samples[i];
    }
    threshold /= count;
    threshold += 1000; // Add offset above average
    
    // Count peaks
    bool above_threshold = false;
    for (int i = 1; i < count; i++) {
        if (samples[i] > threshold && !above_threshold) {
            peaks++;
            above_threshold = true;
        } else if (samples[i] < threshold) {
            above_threshold = false;
        }
    }
    
    // Calculate BPM: (peaks * 60 * sample_rate) / count
    if (peaks > 0) {
        return (peaks * 60 * sample_rate) / count;
    }
    
    return 0;
}

// Main sensor reading thread
void max30102_thread(void)
{
    uint32_t red, ir;
    uint16_t bpm = 0;
    
    if (max30102_init() != 0) {
        return;
    }
    
    while (1) {
        if (max30102_read_fifo(&red, &ir) > 0) {
            // Store samples for heart rate calculation
            if (sensor_data.sample_count < 100) {
                sensor_data.red_samples[sensor_data.sample_count] = red;
                sensor_data.ir_samples[sensor_data.sample_count] = ir;
                sensor_data.sample_count++;
            } else {
                // Calculate heart rate
                bpm = calculate_heart_rate(sensor_data.red_samples, 
                                         sensor_data.sample_count, 100);
                
                LOG_INF("Heart Rate: %d BPM, Red: %d, IR: %d", bpm, red, ir);
                
                // Reset sample buffer
                sensor_data.sample_count = 0;
            }
        }
        
        k_msleep(10); // 100Hz sampling
    }
}

K_THREAD_DEFINE(max30102_tid, 1024, max30102_thread, NULL, NULL, NULL, 5, 0, 0);
```

---

## 3. Motion Sensor (MPU6050 IMU)

### Overview
The MPU6050 is a 6-axis IMU (3-axis gyroscope + 3-axis accelerometer) commonly used for motion tracking in wearables.

### Specifications
- **Sensors**: 3-axis gyroscope, 3-axis accelerometer
- **Interface**: I2C (up to 400kHz)
- **Supply Voltage**: 2.375V - 3.46V
- **Current**: 3.9mA (normal mode)
- **Gyroscope Range**: ±250, ±500, ±1000, ±2000 °/sec
- **Accelerometer Range**: ±2g, ±4g, ±8g, ±16g
- **Resolution**: 16-bit ADC
- **Built-in DMP**: Digital Motion Processor

### Hardware Connection
```
nRF52840    MPU6050
---------   -------
VCC      -> VCC (3.3V)
GND      -> GND
P0.26    -> SCL (I2C Clock)
P0.27    -> SDA (I2C Data)
P0.29    -> INT (Optional interrupt)
```

### Device Tree Configuration
```dts
&i2c0 {
    mpu6050: mpu6050@68 {
        compatible = "invensense,mpu6050";
        reg = <0x68>;
        int-gpios = <&gpio0 29 GPIO_ACTIVE_HIGH>;
    };
};
```

### Zephyr Configuration (prj.conf)
```ini
CONFIG_I2C=y
CONFIG_SENSOR=y
CONFIG_MPU6050=y
CONFIG_GPIO=y
```

### C Code Example
```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_REGISTER(mpu6050, LOG_LEVEL_DBG);

static const struct device *mpu6050_dev;

struct motion_data {
    double accel_x, accel_y, accel_z;
    double gyro_x, gyro_y, gyro_z;
    double temperature;
    uint32_t step_count;
    bool motion_detected;
};

static struct motion_data motion;

int mpu6050_init(void)
{
    mpu6050_dev = DEVICE_DT_GET_ONE(invensense_mpu6050);
    if (!device_is_ready(mpu6050_dev)) {
        LOG_ERR("MPU6050 device not ready");
        return -ENODEV;
    }

    // Configure sensor ranges
    struct sensor_value accel_range = { .val1 = 2 }; // ±2g
    struct sensor_value gyro_range = { .val1 = 250 }; // ±250°/s

    sensor_attr_set(mpu6050_dev, SENSOR_CHAN_ACCEL_XYZ,
                    SENSOR_ATTR_FULL_SCALE, &accel_range);
    sensor_attr_set(mpu6050_dev, SENSOR_CHAN_GYRO_XYZ,
                    SENSOR_ATTR_FULL_SCALE, &gyro_range);

    LOG_INF("MPU6050 initialized successfully");
    return 0;
}

int mpu6050_read_sensors(void)
{
    struct sensor_value accel[3], gyro[3], temp;
    
    if (sensor_sample_fetch(mpu6050_dev) < 0) {
        LOG_ERR("Failed to fetch sensor sample");
        return -EIO;
    }

    // Read accelerometer data
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_X, &accel[0]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_Y, &accel[1]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_Z, &accel[2]);

    // Read gyroscope data
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_GYRO_X, &gyro[0]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_GYRO_Y, &gyro[1]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_GYRO_Z, &gyro[2]);

    // Read temperature
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_DIE_TEMP, &temp);

    // Convert to double
    motion.accel_x = sensor_value_to_double(&accel[0]);
    motion.accel_y = sensor_value_to_double(&accel[1]);
    motion.accel_z = sensor_value_to_double(&accel[2]);
    
    motion.gyro_x = sensor_value_to_double(&gyro[0]);
    motion.gyro_y = sensor_value_to_double(&gyro[1]);
    motion.gyro_z = sensor_value_to_double(&gyro[2]);
    
    motion.temperature = sensor_value_to_double(&temp);

    return 0;
}

// Simple step detection algorithm
bool detect_step(double accel_magnitude)
{
    static double prev_magnitude = 0;
    static bool step_detected = false;
    static int64_t last_step_time = 0;
    
    int64_t current_time = k_uptime_get();
    
    // Threshold-based step detection
    const double step_threshold = 1.2; // Adjust based on sensitivity needed
    const double min_step_interval = 300; // Minimum 300ms between steps
    
    if (accel_magnitude > step_threshold && 
        prev_magnitude <= step_threshold &&
        (current_time - last_step_time) > min_step_interval) {
        
        last_step_time = current_time;
        step_detected = true;
        motion.step_count++;
        return true;
    }
    
    prev_magnitude = accel_magnitude;
    return false;
}

// Calculate accelerometer magnitude
double calculate_accel_magnitude(void)
{
    return sqrt(motion.accel_x * motion.accel_x + 
                motion.accel_y * motion.accel_y + 
                motion.accel_z * motion.accel_z);
}

// Motion detection for activity monitoring
bool detect_motion(void)
{
    static double prev_accel[3] = {0, 0, 0};
    const double motion_threshold = 0.1; // m/s²
    
    double diff_x = fabs(motion.accel_x - prev_accel[0]);
    double diff_y = fabs(motion.accel_y - prev_accel[1]);
    double diff_z = fabs(motion.accel_z - prev_accel[2]);
    
    prev_accel[0] = motion.accel_x;
    prev_accel[1] = motion.accel_y;
    prev_accel[2] = motion.accel_z;
    
    return (diff_x > motion_threshold || 
            diff_y > motion_threshold || 
            diff_z > motion_threshold);
}

// Main motion sensing thread
void motion_thread(void)
{
    double accel_mag;
    bool step_detected;
    
    if (mpu6050_init() != 0) {
        return;
    }
    
    while (1) {
        if (mpu6050_read_sensors() == 0) {
            accel_mag = calculate_accel_magnitude();
            step_detected = detect_step(accel_mag);
            motion.motion_detected = detect_motion();
            
            if (step_detected) {
                LOG_INF("Step detected! Total steps: %d", motion.step_count);
            }
            
            LOG_DBG("Accel: X=%.2f Y=%.2f Z=%.2f (mag=%.2f)", 
                    motion.accel_x, motion.accel_y, motion.accel_z, accel_mag);
            LOG_DBG("Gyro: X=%.2f Y=%.2f Z=%.2f", 
                    motion.gyro_x, motion.gyro_y, motion.gyro_z);
            LOG_DBG("Temperature: %.2f°C", motion.temperature);
        }
        
        k_msleep(50); // 20Hz sampling rate
    }
}

K_THREAD_DEFINE(motion_tid, 2048, motion_thread, NULL, NULL, NULL, 6, 0, 0);
```

---

## 4. System Integration Example

### Complete Smartwatch Application
```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

LOG_MODULE_REGISTER(smartwatch, LOG_LEVEL_INF);

// Global sensor data structure
struct smartwatch_data {
    uint16_t heart_rate;
    uint8_t spo2;
    uint32_t step_count;
    double temperature;
    bool motion_active;
    int64_t last_update;
};

static struct smartwatch_data watch_data = {0};

// Bluetooth characteristic for health data
static ssize_t read_health_data(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               void *buf, uint16_t len, uint16_t offset)
{
    const struct smartwatch_data *data = &watch_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, data, sizeof(*data));
}

BT_GATT_SERVICE_DEFINE(health_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0x180D)), // Heart Rate Service
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2A37),   // Heart Rate Measurement
                          BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_READ,
                          read_health_data, NULL, &watch_data),
);

// Main application thread
void smartwatch_main_thread(void)
{
    // Initialize all sensors
    oled_init();
    max30102_init();
    mpu6050_init();
    
    // Initialize Bluetooth
    bt_enable(NULL);
    
    LOG_INF("Smartwatch application started");
    
    while (1) {
        // Update display with current data
        char display_buffer[64];
        snprintf(display_buffer, sizeof(display_buffer), 
                "HR: %d BPM\nSteps: %d\nTemp: %.1f°C", 
                watch_data.heart_rate, 
                watch_data.step_count, 
                watch_data.temperature);
        
        oled_clear_screen();
        oled_write_text(display_buffer, 0, 0);
        
        // Update timestamp
        watch_data.last_update = k_uptime_get();
        
        LOG_INF("Health Data - HR: %d, Steps: %d, Temp: %.1f°C", 
                watch_data.heart_rate, 
                watch_data.step_count, 
                watch_data.temperature);
        
        k_msleep(1000); // Update every second
    }
}

K_THREAD_DEFINE(main_tid, 2048, smartwatch_main_thread, NULL, NULL, NULL, 7, 0, 0);

// Data collection callback functions
void update_heart_rate(uint16_t bpm, uint8_t spo2)
{
    watch_data.heart_rate = bpm;
    watch_data.spo2 = spo2;
}

void update_motion_data(uint32_t steps, double temp, bool active)
{
    watch_data.step_count = steps;
    watch_data.temperature = temp;
    watch_data.motion_active = active;
}
```

### Power Management Configuration
```c
#include <zephyr/pm/pm.h>

// Power management configuration
void smartwatch_pm_init(void)
{
    // Enable automatic power management
    pm_state_set(PM_STATE_SUSPEND_TO_IDLE);
    
    // Configure wake sources
    // - Motion sensor interrupt
    // - Heart rate sensor interrupt
    // - Button press
    // - Bluetooth activity
}

// Sleep mode when inactive
void enter_sleep_mode(void)
{
    // Turn off display backlight
    oled_clear_screen();
    
    // Reduce sensor sampling rates
    // Put sensors in low-power mode
    
    // Enter deep sleep
    pm_state_force(0u, &(struct pm_state_info){PM_STATE_SUSPEND_TO_RAM, 0, 0});
}
```

---

## 5. Configuration Files

### Complete prj.conf
```ini
# Core Zephyr configurations
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048

# Peripheral support
CONFIG_I2C=y
CONFIG_SPI=y
CONFIG_GPIO=y

# Display support
CONFIG_DISPLAY=y
CONFIG_SSD1306=y
CONFIG_LVGL=y
CONFIG_LVGL_DISPLAY_DEV_NAME="SSD1306"

# Sensor support
CONFIG_SENSOR=y
CONFIG_MAX30102=y
CONFIG_MPU6050=y

# Bluetooth support
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="nRF52840-Watch"
CONFIG_BT_GATT_SERVICE_CHANGED=y

# Power management
CONFIG_PM=y
CONFIG_PM_DEVICE=y

# Logging
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3

# Threading
CONFIG_THREAD_ANALYZER=y
CONFIG_THREAD_NAME=y

# Memory optimization
CONFIG_HEAP_MEM_POOL_SIZE=4096
```

### Board-specific Device Tree Overlay (nrf52840dk_nrf52840.overlay)
```dts
&i2c0 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;
    
    ssd1306: ssd1306@3c {
        compatible = "solomon,ssd1306fb";
        reg = <0x3c>;
        width = <128>;
        height = <64>;
        segment-offset = <0>;
        page-offset = <0>;
        display-offset = <0>;
        multiplex-ratio = <63>;
        segment-remap;
        com-invdir;
        prechargep = <0x22>;
    };
    
    max30102: max30102@57 {
        compatible = "maxim,max30102";
        reg = <0x57>;
        int-gpios = <&gpio0 28 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
    };
    
    mpu6050: mpu6050@68 {
        compatible = "invensense,mpu6050";
        reg = <0x68>;
        int-gpios = <&gpio0 29 GPIO_ACTIVE_HIGH>;
    };
};

&gpio0 {
    status = "okay";
};

&uart0 {
    status = "okay";
    current-speed = <115200>;
    pinctrl-0 = <&uart0_default>;
    pinctrl-names = "default";
};
```

---

## 6. Build and Flash Instructions

### West Build Commands
```bash
# Initialize workspace (first time only)
west init -m https://github.com/zephyrproject-rtos/zephyr.git zephyr-workspace
cd zephyr-workspace
west update

# Build for nRF52840 DK
west build -b nrf52840dk_nrf52840 app

# Flash to device
west flash

# Monitor serial output
west build -t menuconfig  # Optional: configure build
```

### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf52840_smartwatch)

# Application sources
target_sources(app PRIVATE
    src/main.c
    src/oled_display.c
    src/max30102_sensor.c
    src/mpu6050_sensor.c
    src/bluetooth_service.c
)

# Include directories
target_include_directories(app PRIVATE
    include/
)
```

---

## 7. Testing and Debugging

### Serial Debug Output
```c
// Enable comprehensive logging
LOG_MODULE_REGISTER(smartwatch_debug, LOG_LEVEL_DBG);

void debug_print_sensor_status(void)
{
    LOG_DBG("=== Sensor Status ===");
    LOG_DBG("OLED: %s", display_dev ? "OK" : "ERROR");
    LOG_DBG("MAX30102: %s", max30102_dev ? "OK" : "ERROR");
    LOG_DBG("MPU6050: %s", mpu6050_dev ? "OK" : "ERROR");
    LOG_DBG("==================");
}
```

### Performance Monitoring
```c
void monitor_system_performance(void)
{
    // Memory usage
    LOG_INF("Free heap: %zu bytes", k_heap_free_get(&k_malloc_heap));
    
    // Thread stack usage
    LOG_INF("Main thread stack usage: %zu/%zu", 
            k_thread_stack_space_get(&main_tid),
            CONFIG_MAIN_STACK_SIZE);
}
```

