# NISC Medical Wearable Device - DFU and Booting System

This project implements a complete Device Firmware Update (DFU) and booting system for the NISC Medical Wearable Device based on nRF52840. The system includes button press detection, Bluetooth Low Energy advertising, and serial communication for firmware updates.

## Features

### 🔄 DFU (Device Firmware Update) System
- **Secure Bootloader**: Validates firmware images with CRC32 checksums and digital signatures
- **Bluetooth DFU Service**: Custom BLE service for wireless firmware updates
- **Serial DFU**: USB CDC and UART support for wired firmware updates
- **Image Validation**: Comprehensive firmware image validation before installation

### 🔘 Button Press Detection
- **Debounced Input**: Reliable button press detection with hardware debouncing
- **Multiple Hold Times**: Different actions based on button hold duration:
  - Short press (< 3s): Normal operation
  - Long press (3-10s): Enter DFU mode
  - Very long press (> 10s): Factory reset
- **Interrupt-driven**: Efficient GPIO interrupt handling

### 📡 Bluetooth Low Energy Advertising
- **DFU Service**: Custom GATT service for firmware updates
- **Advertising**: "NISC-DFU" device name for easy identification
- **Connection Management**: Automatic connection handling and status notifications

### 🔌 Serial Communication
- **USB CDC**: Virtual serial port over USB for debugging and DFU
- **UART Support**: Hardware UART for external communication
- **Dual Mode**: Support for both USB and UART simultaneously

## Boot Process

### 1. Bootloader Initialization
```
=== NISC Medical Wearable Device Starting ===
Firmware Version: 1.0.0
Device Model: NISC-MW-001
Target Platform: nRF52840 Development Kit
Build Time: Dec 15 2024 10:30:45

Initializing bootloader...
Bootloader initialized successfully
```

### 2. Button Detection
The system waits for button input to determine boot mode:
- **No button press**: Normal application boot
- **Button held 3+ seconds**: Enter DFU mode
- **Button held 10+ seconds**: Factory reset mode

### 3. Boot Mode Selection
```
Checking boot mode...
DFU mode requested - entering DFU mode
DFU Mode: Waiting for firmware update...
DFU mode active - Bluetooth advertising started
DFU Mode: Bluetooth advertising active
```

### 4. Application Startup
After button press or timeout:
```
Press button to start main application...
Button pressed - starting main application
Normal Mode: Starting medical device application...
```

## DFU Process

### Bluetooth DFU
1. **Advertising**: Device advertises as "NISC-DFU"
2. **Connection**: Client connects to the device
3. **Service Discovery**: DFU service is discovered
4. **Firmware Transfer**: Firmware is sent in packets
5. **Validation**: CRC32 and signature validation
6. **Installation**: Firmware is written to flash
7. **Reboot**: Device reboots with new firmware

### Serial DFU
1. **Connection**: Connect via USB CDC or UART
2. **Command Interface**: Send DFU commands via serial
3. **Data Transfer**: Firmware data is transmitted
4. **Validation**: Same validation process as Bluetooth
5. **Installation**: Firmware installation and reboot

## File Structure

```
app/src/
├── main.c                 # Main application with DFU integration
├── bootloader.c/.h        # Secure bootloader implementation
├── bluetooth_dfu.c/.h     # Bluetooth DFU service
├── serial_comm.c/.h       # Serial communication (USB/UART)
├── button_handler.c/.h    # Button press detection
├── system.c/.h           # System management
├── thread_manager.c/.h   # Thread management
├── medical_device.c/.h   # Medical device functionality
├── diagnostics.c/.h      # System diagnostics
├── config.c/.h           # Configuration management
├── hardware.c/.h         # Hardware abstraction
└── shell_commands.c/.h   # Shell command interface
```

## Configuration

### Project Configuration (prj.conf)
```conf
# Bluetooth Low Energy Support
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="NISC-DFU"

# DFU Support
CONFIG_DFU=y
CONFIG_IMG_MANAGER=y
CONFIG_MCUBOOT=y
CONFIG_BOOTLOADER_MCUBOOT=y

# Flash and Storage
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_STREAM_FLASH=y

# UART Support
CONFIG_SERIAL=y
CONFIG_UART_INTERRUPT_DRIVEN=y

# GPIO and Interrupts
CONFIG_GPIO=y
CONFIG_GPIO_INT=y
```

### Hardware Configuration
- **Button**: GPIO pin 11 (with pull-up)
- **Status LED**: GPIO pin 13
- **UART**: UART_0 device
- **USB CDC**: CDC_ACM_0 device

## Usage

### Building the Project
```bash
# Build for nRF52840 DK
west build -b nrf52840dk_nrf52840 app/

# Flash the firmware
west flash

# Monitor serial output
west espressif monitor
```

### DFU Mode Entry
1. **Power on** the device
2. **Hold button** for 3+ seconds during boot
3. **Release button** when LED starts blinking
4. Device enters DFU mode and starts Bluetooth advertising

### Normal Operation
1. **Power on** the device
2. **Press button** briefly (< 3 seconds) or wait for timeout
3. Device starts normal medical application

### Serial Communication
- **USB CDC**: Connect via USB cable (appears as COM port)
- **UART**: Connect to UART_0 pins (TX/RX/GND)
- **Baud Rate**: 115200 bps
- **Flow Control**: None

## API Reference

### Bootloader API
```c
// Initialize bootloader
int bootloader_init(void);

// Check boot mode
boot_mode_t bootloader_check_boot_mode(void);

// Enter DFU mode
int bootloader_enter_dfu_mode(void);

// Start application
int bootloader_start_application(void);
```

### Button Handler API
```c
// Initialize button handler
int button_handler_init(void);

// Check for button events
button_event_t button_handler_check_event(void);

// Wait for button press
bool button_handler_wait_for_press(uint32_t timeout_ms);
```

### Bluetooth DFU API
```c
// Initialize DFU service
int bluetooth_dfu_init(void);

// Start advertising
int bluetooth_dfu_start_advertising(void);

// Process DFU packet
int bluetooth_dfu_process_packet(const dfu_packet_t *packet);
```

### Serial Communication API
```c
// Initialize serial communication
int serial_comm_init(const serial_config_t *config);

// Send data
int serial_comm_send(const uint8_t *data, uint16_t length);

// Receive data
int serial_comm_receive(uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms);

// Print formatted string
int serial_comm_printf(const char *format, ...);
```

## Security Features

- **Image Validation**: CRC32 checksums and digital signatures
- **Secure Boot**: Validates firmware before execution
- **Encrypted Communication**: BLE security for DFU transfers
- **Access Control**: Authentication for firmware updates

## Troubleshooting

### Common Issues

1. **Button not detected**
   - Check GPIO configuration
   - Verify button hardware connections
   - Ensure pull-up resistor is present

2. **Bluetooth not advertising**
   - Check Bluetooth configuration
   - Verify antenna connections
   - Check power supply stability

3. **Serial communication fails**
   - Verify USB/UART connections
   - Check baud rate settings
   - Ensure proper drivers installed

4. **DFU transfer fails**
   - Check image validation
   - Verify CRC32 checksums
   - Ensure sufficient flash space

### Debug Commands
```bash
# Enable verbose logging
west build -DCONFIG_LOG_DEFAULT_LEVEL=4

# Monitor with debug output
west espressif monitor --log-level debug

# Check device tree
west build -t menuconfig
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Support

For technical support or questions:
- **Email**: support@niscmedical.com
- **Documentation**: [docs.niscmedical.com](https://docs.niscmedical.com)
- **Issues**: [GitHub Issues](https://github.com/niscmedical/nordic_medical_platform/issues)
