# nRF52840 Zephyr Medical Wearable Platform

A comprehensive Zephyr RTOS development platform for the **nRF52840 Development Kit**, specifically designed for medical wearable device research and development. This repository provides a production-ready baseline with hardware abstraction, visual status indicators, and USB console interface.

## Overview

This project establishes a complete development environment for the nRF52840 microcontroller with comprehensive hardware integration including LED-based status indication, USB console debugging, and medical device simulation capabilities. The platform is designed for both QEMU emulation and real hardware deployment.

## Architecture

### Hardware Platform
- **Primary Target**: nRF52840 Development Kit (nRF52840DK)
- **Secondary Target**: QEMU ARM Cortex-M3 emulation  
- **LED Integration**: 4-LED status and medical pulse visualization
- **Console Interface**: USB-C virtual COM port for debugging and control

### Build System
- **Build tool**: West (Zephyr's meta-tool)
- **Package management**: uv for Python dependencies
- **Target platforms**: nRF52840 hardware (primary) and QEMU emulation
- **Dependencies**: Minimal module set (cmsis, hal_nordic, nrfx, segger)

### Development Environment
- **Toolchain**: Zephyr SDK with ARM GCC
- **Hardware Features**: GPIO, USB CDC-ACM, Hardware Info, Shell Commands
- **Emulation**: QEMU ARM Cortex-M3 for hardware-independent development
- **Version control**: Git with dependency tracking via west.yml manifest

## Quick Start

### Initial Setup

```bash
make init
```

### Hardware Development (Recommended)

```bash
# Build and flash to nRF52840DK in one step
make dev-hw

# OR step by step:
make build      # Build for hardware (default)
make flash      # Flash to connected nRF52840DK
```

### DFU Boot Process

The device now includes a DFU (Device Firmware Update) boot process:

1. **Boot Sequence**: Device checks for button press during startup
2. **DFU Mode**: If button is pressed, device enters DFU mode for firmware updates
3. **Button Wait**: Device waits for button press before starting normal operation
4. **Bluetooth Advertising**: Device starts Bluetooth Low Energy advertising automatically

### Console Connection

Connect to USB console to see real-time medical data and use interactive commands:

```bash
# Linux
screen /dev/ttyACM0 115200

# macOS  
screen /dev/tty.usbmodem* 115200

# Windows - use PuTTY with COM port at 115200 baud
```

### Emulation Development

```bash
make build-qemu  # Build for QEMU
make run-qemu    # Execute in emulator
```

## Hardware Features

### LED Status System

The nRF52840DK's four LEDs provide comprehensive visual feedback:

| LED | Function | Normal Pattern | Alert Patterns |
|-----|----------|---------------|----------------|
| **LED1** | System Status | Breathing | Slow blink (starting), SOS (critical error) |
| **LED2** | Medical Pulse | Heartbeat pattern (synchronized with sensor data) | Off (sensor failure) |
| **LED3** | Communication | Off (idle) | Fast blink (transmitting data) |
| **LED4** | Error/Warning | Off (no errors) | Various patterns for different alert types |

### USB Console Interface

- **Real-time medical data display** with formatted vital signs
- **Interactive shell commands** for device control and testing
- **Comprehensive system logging** with multiple log levels
- **Hardware information** display including device ID and reset cause

### Medical Data Simulation

Realistic medical sensor simulation with:
- **Heart Rate**: 60-100 BPM with natural variation
- **Body Temperature**: 36.0-37.5°C with thermal regulation patterns  
- **Motion Activity**: 0-5g with activity burst simulation
- **Blood Oxygen**: 95-100% with breathing pattern variation

## Build Commands

| Command | Description | Use Case |
|---------|-------------|----------|
| `make build` | Build for nRF52840 hardware (default) | Primary development target |
| `make build-hw` | Build for nRF52840 hardware | Explicit hardware build |
| `make build-qemu` | Build for QEMU emulation | Algorithm development and testing |
| `make flash` | Flash to nRF52840DK | Hardware deployment |
| `make dev-hw` | Build and flash in one step | Rapid hardware development |
| `make dev-qemu` | Build and run QEMU in one step | Rapid emulation testing |

## Project Structure

```
├── app/
│ ├── src/
│ │   ├── main.c              # Application entry point with hardware integration
│ │   ├── hardware.c/.h       # Hardware abstraction layer (LEDs, USB, GPIO)
│ │   ├── shell_commands.c/.h # Interactive console commands
│ │   ├── system.c/.h         # Core system management
│ │   ├── thread_manager.c/.h # Thread orchestration
│ │   ├── medical_device.c/.h # Medical device simulation
│ │   ├── diagnostics.c/.h    # Logging and diagnostics
│ │   └── safe_*.c/.h         # Safe data structures
│ ├── CMakeLists.txt          # Build configuration
│ ├── prj.conf                # Zephyr project configuration with USB/GPIO support
│ ├── west.yml                # Dependency manifest
│ └── *.overlay               # Hardware-specific device tree overlays
├── docs/                     # Research and development documentation  
├── HARDWARE_SETUP.md         # Comprehensive hardware setup guide
├── build/                    # Build artifacts (generated)
├── deps/                     # Zephyr dependencies (generated)
└── Makefile                  # Build automation with hardware focus
```

## Interactive Shell Commands

Once connected to the USB console:

```bash
# System information
uart:~$ sysinfo               # Complete system information
uart:~$ hwinfo               # Hardware information
uart:~$ threadinfo           # Thread status

# LED control and testing
uart:~$ led test heartbeat    # Test heartbeat pattern on all LEDs
uart:~$ led test breathing    # Test breathing pattern
uart:~$ led test sos          # Test SOS emergency pattern
uart:~$ led set 0 on          # Turn on LED 0 (Status)
uart:~$ led pattern 1 heartbeat # Set LED 1 to heartbeat pattern

# Medical device control  
uart:~$ medical pulse 85      # Set heartbeat LED to 85 BPM
uart:~$ medical test          # Run medical device self-test
uart:~$ medical status        # Show current medical data

# DFU Boot control
uart:~$ dfu status            # Show DFU status
uart:~$ dfu enter            # Enter DFU boot mode
uart:~$ dfu exit             # Exit DFU boot mode
uart:~$ dfu wait 5000        # Wait 5 seconds for button press

# Bluetooth control
uart:~$ bt status            # Show Bluetooth status
uart:~$ bt start             # Start Bluetooth advertising
uart:~$ bt stop              # Stop Bluetooth advertising
uart:~$ bt setname MyDevice  # Set device name
uart:~$ bt send "Hello"      # Send data via serial Bluetooth

# Diagnostic control
uart:~$ diag status          # Show diagnostic status
uart:~$ diag test            # Run diagnostic tests
uart:~$ diag clear           # Clear error counters
uart:~$ diag log 3           # Set log level to Info

# Built-in Zephyr commands
uart:~$ kernel threads        # Show running threads
uart:~$ device list           # Show available devices
uart:~$ hwinfo devid         # Show hardware device ID
```

## Dependencies

### System Requirements
- Linux, macOS, or Windows development environment
- [uv](https://docs.astral.sh/uv/) Python package manager
- USB connection for hardware deployment and console access
- nRF52840 Development Kit for hardware testing

### Zephyr Modules
- **cmsis**: ARM CMSIS headers for Cortex-M4 support
- **hal_nordic**: Nordic hardware abstraction layer
- **nrfx**: Nordic peripheral drivers (GPIO, USB, etc.)
- **segger**: J-Link RTT debugging support

## Hardware Configuration

### nRF52840 Development Kit
- **Board**: nRF52840 Development Kit (nrf52840dk_nrf52840)
- **SoC**: nRF52840 (ARM Cortex-M4F @ 64MHz)  
- **Memory**: 1MB Flash, 256KB RAM
- **Connectivity**: Bluetooth 5.0, IEEE 802.15.4, USB 2.0
- **LEDs**: 4x user-controllable LEDs for status indication
- **Console**: USB CDC-ACM virtual COM port

### Emulation Target
- **Platform**: QEMU ARM Cortex-M3
- **Limitations**: Basic CPU emulation, no Nordic-specific peripherals or LEDs
- **Use case**: Algorithm development and testing without hardware

## Development Workflow

1. **Hardware Setup**: Connect nRF52840DK via USB-C cable
2. **Environment Setup**: Run `make init` for complete toolchain installation
3. **Development**: Modify application source in `app/src/`
4. **Hardware Testing**: Use `make dev-hw` for build and flash
5. **Console Monitoring**: Connect to USB console for real-time feedback
6. **LED Verification**: Observe LED patterns for system status
7. **Interactive Testing**: Use shell commands for device control
8. **Documentation**: Update research notes in `./docs`

## Expected Output

### Hardware Startup Sequence
1. **LED Animation**: All LEDs flash in sequence during initialization
2. **DFU Check**: Device checks for button press to enter DFU mode
3. **Button Wait**: Device waits for button press before starting normal operation
4. **USB Enumeration**: Virtual COM port becomes available
5. **Bluetooth Advertising**: BLE advertising starts automatically
6. **System Ready**: All LEDs briefly blink together
7. **Normal Operation**: LEDs settle into assigned patterns:
   - LED1: Breathing (system healthy)
   - LED2: Heartbeat (medical pulse)
   - LED3: Communication activity (Bluetooth advertising)
   - LED4: Error indication (off = no errors)

### Console Output Example
```
=== NISC Medical Wearable Device Starting ===
Firmware Version: 1.0.0
Device Model: NMW-nRF52840
Target Platform: nRF52840 Development Kit

Initializing hardware abstraction layer...
Initializing DFU boot process...
Waiting for button press to start normal operation...
Button pressed - starting normal operation

Hardware Info:
  Device ID: deadbeef-12345678
  Reset Cause: 0x00000001  
  USB Console: Ready
  LEDs: Initialized

Initializing Bluetooth advertising...
Bluetooth advertising started - Device discoverable
Serial Bluetooth communication ready

=== System Ready - All LEDs Active ===

MEDICAL DATA PULSE #1 [Time: 15.234 s]
+-----------------------------------------+
| HEART RATE:   72 bpm   [Quality: 89%] |
| TEMPERATURE:  36.8°C    [Quality: 94%] |
| MOTION:       0.3 g     [Quality: 97%] |
| BLOOD O2:     98.2%     [Quality: 99%] |
+-----------------------------------------+

TRANSMITTING MEDICAL DATA PACKET #1
+--- Current Patient Vitals Summary ---+
| HR:  72 bpm  | Temp: 36.8°C         |
| Motion: 0.3g | SpO2: 98.2%         |
+-------------------------------------+
Via: Bluetooth Low Energy (BLE) - Advertising Active
Device Name: NISC-Medical-Device
Data packet transmitted successfully

uart:~$ sysinfo
=== NISC Medical Wearable System Information ===
Hardware:
  Device ID: deadbeef-12345678
  Reset Cause: 0x00000001
  USB Console: Ready
  LEDs: Initialized
System:
  Uptime: 15234 ms
  Total Errors: 0
  Current State: 2

uart:~$ bt status
=== Bluetooth Status ===
Bluetooth:
  Status: Initialized
  Advertising: Active
  Device Name: NISC-Medical-Device
  Serial Interface: Ready
========================

uart:~$ dfu status
=== DFU Boot Status ===
DFU Boot:
  Initialized: Yes
  Boot Requested: No
  Button Press Count: 1
  Button State: Released
========================
```

## Maintenance and Debugging

### Environment Verification
```bash
make check-env    # Validate development environment
make info         # Display configuration details  
```

### Dependency Updates
```bash
make deps-update  # Update Zephyr dependencies
make clean-all    # Full environment reset
```

### Hardware Troubleshooting
- **No LEDs**: Check power and firmware flash
- **No console**: Verify USB cable and COM port settings (115200 baud)
- **Build errors**: Ensure nRF SDK is properly installed

## Advanced Features

### LED Pattern Customization
The hardware abstraction layer supports custom LED patterns for:
- Medical pulse synchronization
- Communication protocol indication  
- Error severity visualization
- System status breathing patterns

### Medical Data Integration
- Real-time sensor data simulation with clinical accuracy
- Configurable alert thresholds with visual/audio feedback
- Data logging and transmission simulation
- Regulatory compliance framework for medical devices

## Safety and Compliance

⚠️ **Medical Device Warning**: This is a development platform for research purposes only. Do not use for actual medical monitoring without proper validation, testing, and regulatory approval.

- **Development Use Only**: Not intended for clinical or patient use
- **Regulatory Compliance**: Ensure compliance with medical device regulations
- **Safety Testing**: Comprehensive safety testing required for medical applications
- **Power Safety**: Use only USB power during development

## Documentation

- **[HARDWARE_SETUP.md](HARDWARE_SETUP.md)**: Detailed hardware setup and troubleshooting guide
- **[docs/](docs/)**: Research documentation and technical specifications  
- **Source Documentation**: Comprehensive Doxygen-style documentation in source files

For hardware-specific setup instructions, LED pattern details, console commands, and troubleshooting, see the **[Hardware Setup Guide](HARDWARE_SETUP.md)**.
