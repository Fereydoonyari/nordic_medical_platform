# Nordic Medical Platform

A simple Zephyr RTOS application for the **nRF52840 Development Kit** with DFU mode detection, button handling, and Bluetooth advertising.

## Overview

This project provides a clean, simple implementation for the nRF52840 microcontroller with:
- **DFU Mode Detection**: Device checks for button press during startup to enter DFU mode
- **Button Wait**: Device waits 5 seconds for button press before starting normal operation
- **Bluetooth Advertising**: Device starts Bluetooth Low Energy advertising automatically
- **LED Status**: Simple LED control for visual feedback

## Quick Start

### Hardware Development

```bash
# Build and flash to nRF52840DK
make dev-hw

# OR step by step:
make build      # Build for hardware
make flash      # Flash to connected nRF52840DK
```

### Boot Process

1. **Power On**: Device starts up
2. **DFU Check**: If button 1 is pressed during startup, device enters DFU mode
3. **Button Wait**: Device waits 5 seconds for button press to start normal operation
4. **Normal Operation**: If no button press, device automatically starts normal operation
5. **Bluetooth Advertising**: Device starts BLE advertising and LED patterns

### Console Connection

Connect to USB console to see real-time output:

```bash
# Linux
screen /dev/ttyACM0 115200

# macOS  
screen /dev/tty.usbmodem* 115200

# Windows - use PuTTY with COM port at 115200 baud
```

## Hardware Features

### LED Status System

The nRF52840DK's four LEDs provide visual feedback:

| LED | Function | Pattern |
|-----|----------|---------|
| **LED1** | System Status | Breathing pattern (system OK) |
| **LED2** | Heartbeat | Simple blink (system alive) |
| **LED3** | Communication | Slow blink (Bluetooth advertising) |
| **LED4** | Error/Warning | Off (no errors) |

### Button Control

- **Button 1**: Used for DFU mode detection and normal operation start
- **DFU Mode**: Press and hold during startup to enter firmware update mode
- **Normal Start**: Press within 5 seconds of startup to begin normal operation

## Project Structure

```
├── app/
│ ├── src/
│ │   ├── main.c              # Simple main application
│ │   ├── hardware.c/.h       # Hardware abstraction layer
│ │   └── common.h            # Common definitions
│ ├── CMakeLists.txt          # Build configuration
│ ├── prj.conf                # Zephyr project configuration
│ └── west.yml                # Dependency manifest
├── docs/                     # Documentation
└── README.md                 # This file
```

## Expected Output

### Startup Sequence
```
=== Nordic Medical Platform Starting ===
Firmware Version: 1.0.0
Device Model: NMW-nRF52840
Target Platform: nRF52840 Development Kit

Initializing hardware...
Initializing hardware abstraction layer
GPIO subsystem initialized
LEDs initialized
Button initialized for DFU boot process
Bluetooth advertising initialized
Hardware abstraction layer initialized successfully

Initializing DFU boot process...
DFU boot process initialized

Waiting 5 seconds for button press to start normal operation...
Waiting for button press (timeout: 5000 ms)
Button press timeout
Timeout - starting normal operation automatically

Initializing Bluetooth advertising...
Bluetooth advertising initialized
Bluetooth advertising started
Bluetooth advertising started - Device discoverable

=== System Ready - Normal Operation ===

System running - Uptime: 2000 ms
System running - Uptime: 4000 ms
System running - Uptime: 6000 ms
...
```

## Build Commands

| Command | Description |
|---------|-------------|
| `make build` | Build for nRF52840 hardware |
| `make flash` | Flash to nRF52840DK |
| `make dev-hw` | Build and flash in one step |

## Hardware Configuration

### nRF52840 Development Kit
- **Board**: nRF52840 Development Kit (nrf52840dk_nrf52840)
- **SoC**: nRF52840 (ARM Cortex-M4F @ 64MHz)  
- **Memory**: 1MB Flash, 256KB RAM
- **Connectivity**: Bluetooth 5.0, USB 2.0
- **LEDs**: 4x user-controllable LEDs
- **Button**: Button 1 for DFU and operation control

## Dependencies

### System Requirements
- Linux, macOS, or Windows development environment
- [West](https://docs.zephyrproject.org/latest/develop/west/index.html) Zephyr meta-tool
- nRF52840 Development Kit for hardware testing

### Zephyr Modules
- **cmsis**: ARM CMSIS headers
- **hal_nordic**: Nordic hardware abstraction layer
- **nrfx**: Nordic peripheral drivers
- **segger**: J-Link RTT debugging support

## Development Workflow

1. **Hardware Setup**: Connect nRF52840DK via USB-C cable
2. **Environment Setup**: Install Zephyr SDK and West
3. **Development**: Modify application source in `app/src/`
4. **Hardware Testing**: Use `make dev-hw` for build and flash
5. **Console Monitoring**: Connect to USB console for real-time feedback
6. **LED Verification**: Observe LED patterns for system status

## Safety and Compliance

⚠️ **Development Use Only**: This is a development platform for research purposes only. Do not use for actual medical monitoring without proper validation, testing, and regulatory approval.

- **Development Use Only**: Not intended for clinical or patient use
- **Regulatory Compliance**: Ensure compliance with medical device regulations
- **Safety Testing**: Comprehensive safety testing required for medical applications
- **Power Safety**: Use only USB power during development
