/**
 * @page api_documentation API Documentation
 * 
 * @section intro_sec Introduction
 * 
 * Welcome to the NISC Wearable Device firmware API documentation. This project implements
 * a Zephyr RTOS-based firmware for wearable devices targeting the nRF52840 microcontroller.
 * 
 * @section features_sec Features
 * 
 * - **Hardware Support**: nRF52840 Development Kit
 * - **Emulation Support**: QEMU Cortex-M3 for development and testing  
 * - **RTOS**: Zephyr Real-Time Operating System
 * - **LED Control**: Basic GPIO LED blinking functionality
 * - **Console Output**: Debug and status messages via UART
 * - **Cross-Platform**: Builds on Linux, macOS, and Windows
 * 
 * @section getting_started_sec Getting Started
 * 
 * ### Prerequisites
 * 
 * - **UV Package Manager**: Install from https://astral.sh/uv/install.sh
 * - **Zephyr SDK**: Download from GitHub releases
 * - **Hardware**: nRF52840 Development Kit (optional, can use QEMU)
 * - **Doxygen**: For generating documentation (optional)
 * 
 * ### Quick Start
 * 
 * 1. **Initial Setup**:
 *    ```bash
 *    make init
 *    ```
 * 
 * 2. **Build for Hardware**:
 *    ```bash
 *    make build-hw
 *    ```
 * 
 * 3. **Flash to Device**:
 *    ```bash
 *    make flash
 *    ```
 * 
 * 4. **Build and Run in Emulator**:
 *    ```bash
 *    make dev-qemu
 *    ```
 * 
 * @section documentation_sec Documentation
 * 
 * ### Generate API Documentation
 * 
 * ```bash
 * make docs          # Generate documentation
 * make docs-open     # Open in browser  
 * make docs-serve    # Serve locally on port 8080
 * make docs-clean    # Clean generated docs
 * ```
 * 
 * @section architecture_sec Architecture
 * 
 * The firmware follows a simple single-threaded architecture:
 * 
 * - **main()**: Application entry point with LED control logic
 * - **Device Tree**: Hardware abstraction for different board configurations
 * - **Zephyr APIs**: GPIO control, timing, and console output
 * - **Conditional Compilation**: Supports boards with/without LEDs
 * 
 * @section development_sec Development Workflow
 * 
 * ### Hardware Development
 * ```bash
 * make dev-hw        # Build and flash in one step
 * ```
 * 
 * ### Emulator Development  
 * ```bash
 * make dev-qemu      # Build and run in QEMU
 * ```
 * 
 * ### Environment Check
 * ```bash
 * make check-env     # Verify development setup
 * make info          # Show project information
 * ```
 * 
 * ### Maintenance
 * ```bash
 * make deps-update   # Update Zephyr dependencies
 * make clean         # Clean build artifacts
 * make clean-all     # Clean everything including dependencies
 * ```
 * 
 * @section makefile_sec Makefile Targets
 * 
 * The project includes a comprehensive Makefile with the following target categories:
 * 
 * - **📦 Setup**: `init`, `reinit`, `setup`
 * - **🔨 Development**: `build-hw`, `build-qemu`, `flash`, `run-qemu`  
 * - **📚 Documentation**: `docs`, `docs-clean`, `docs-open`, `docs-serve`
 * - **🧹 Maintenance**: `deps-update`, `clean`, `clean-all`, `check-env`, `info`
 * - **⚡ Workflows**: `dev-hw`, `dev-qemu`
 * 
 * @section contributing_sec Contributing
 * 
 * ### Code Documentation Standards
 * 
 * - Use Doxygen-style comments for all functions, variables, and macros
 * - Include @brief, @details, @param, @return, and @note tags where appropriate
 * - Document hardware dependencies and platform-specific behavior
 * - Add examples for complex functions
 * 
 * ### Build System
 * 
 * - All build targets should include proper status messages with colors
 * - Error checking should provide helpful guidance for resolution
 * - Support NO_COLOR environment variable for CI/CD systems
 * 
 * @section license_sec License
 * 
 * Copyright (c) 2024 NISC Wearable Project. All rights reserved.
 * 
 * @author NISC Wearable Team
 * @version 1.0.0
 * @date 2024
 */