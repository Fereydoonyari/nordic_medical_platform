.PHONY: help init setup build-hw build-qemu flash run-qemu clean deps-update docs docs-clean docs-open

#==============================================================================
# PROJECT CONFIGURATION
#==============================================================================

# Hardware and build configuration
BOARD_HW := nrf52840dk_nrf52840
BOARD_QEMU := qemu_cortex_m3
BUILD_DIR := build
APP_DIR := app

# Documentation configuration
DOCS_OUTPUT_DIR := docs/generated
DOCS_HTML_DIR := $(DOCS_OUTPUT_DIR)/html
DOXYFILE := Doxyfile

#==============================================================================
# COLOR DEFINITIONS FOR TERMINAL OUTPUT
#==============================================================================

# ANSI color codes for formatted output
GREEN := \033[0;32m
YELLOW := \033[1;33m
RED := \033[0;31m
BLUE := \033[0;34m
CYAN := \033[0;36m
PURPLE := \033[0;35m
NC := \033[0m

#==============================================================================
# HELP AND INFORMATION TARGETS
#==============================================================================

# Default target - shows comprehensive help menu
help: ## Show this help message
	@printf "$(GREEN)╔════════════════════════════════════════════════════════════╗$(NC)\n"
	@printf "$(GREEN)║               nRF52840 Zephyr Project Makefile             ║$(NC)\n"
	@printf "$(GREEN)╚════════════════════════════════════════════════════════════╝$(NC)\n\n"
	@printf "$(YELLOW)📦 Setup Commands:$(NC)\n"
	@printf "  $(CYAN)init$(NC)        - Complete first-time setup for new developers\n"
	@printf "  $(CYAN)reinit$(NC)      - Reinitialize project without fresh west init\n"
	@printf "  $(CYAN)setup$(NC)       - Setup Python environment and install west\n\n"
	@printf "$(YELLOW)🔨 Development Commands:$(NC)\n"
	@printf "  $(CYAN)build-hw$(NC)    - Build firmware for nRF52840 hardware\n"
	@printf "  $(CYAN)build-qemu$(NC)  - Build firmware for QEMU emulation\n"
	@printf "  $(CYAN)flash$(NC)       - Flash firmware to nRF52840 hardware\n"
	@printf "  $(CYAN)run-qemu$(NC)    - Run firmware in QEMU emulation\n\n"
	@printf "$(YELLOW)📚 Documentation Commands:$(NC)\n"
	@printf "  $(CYAN)docs$(NC)        - Generate complete API documentation with Doxygen\n"
	@printf "  $(CYAN)docs-clean$(NC)  - Clean generated documentation files\n"
	@printf "  $(CYAN)docs-open$(NC)   - Open generated documentation in default browser\n"
	@printf "  $(CYAN)docs-serve$(NC)  - Serve documentation locally on port 8080\n\n"
	@printf "$(YELLOW)🧹 Maintenance Commands:$(NC)\n"
	@printf "  $(CYAN)deps-update$(NC) - Update Zephyr dependencies\n"
	@printf "  $(CYAN)clean$(NC)       - Clean build artifacts\n"
	@printf "  $(CYAN)clean-all$(NC)   - Clean everything including dependencies and docs\n"
	@printf "  $(CYAN)check-env$(NC)   - Verify development environment setup\n"
	@printf "  $(CYAN)info$(NC)        - Display project configuration information\n\n"
	@printf "$(YELLOW)⚡ Quick Development Workflows:$(NC)\n"
	@printf "  $(CYAN)dev-hw$(NC)      - Build and flash hardware in one step\n"
	@printf "  $(CYAN)dev-qemu$(NC)    - Build and run QEMU in one step\n"
	@printf "  $(CYAN)build$(NC)       - Build for hardware (default target)\n\n"

#==============================================================================
# ENVIRONMENT CHECK AND SETUP TARGETS
#==============================================================================

# Check if uv (Python package manager) is installed
check-uv:
	@which uv > /dev/null || (printf "$(RED)❌ Error: uv not found. Install with: curl -LsSf https://astral.sh/uv/install.sh | sh$(NC)\n" && exit 1)

# Check if Doxygen is installed for documentation generation
check-doxygen:
	@which doxygen > /dev/null || (printf "$(YELLOW)⚠️  Warning: Doxygen not found. Install it to generate documentation.$(NC)\n" && exit 1)
	@printf "$(GREEN)✅ Doxygen found: $(NC)"; doxygen --version

#==============================================================================
# PROJECT SETUP AND INITIALIZATION TARGETS
#==============================================================================

# Complete first-time setup for new developers
init: check-uv ## Complete setup for new developers after cloning
	@printf "$(GREEN)🚀 Starting complete project setup...$(NC)\n"
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	@printf "$(YELLOW)Step 1/6: Installing west with uv$(NC)\n"
	uv add west
	@printf "$(YELLOW)Step 2/6: Initializing west workspace$(NC)\n"
	uv run west init -l $(APP_DIR)
	@printf "$(YELLOW)Step 3/6: Updating dependencies (this may take a few minutes)$(NC)\n"
	uv run west update
	@printf "$(YELLOW)Step 4/6: Installing Python requirements$(NC)\n"
	uv pip install -r deps/zephyr/scripts/requirements.txt
	@printf "$(YELLOW)Step 5/6: Setting up Zephyr environment$(NC)\n"
	uv run west zephyr-export
	@printf "$(YELLOW)Step 6/6: SDK Installation Notice$(NC)\n"
	@printf "$(RED)⚠️  MANUAL STEP REQUIRED: Install Zephyr SDK manually$(NC)\n"
	@printf "$(CYAN)   Download from: https://github.com/zephyrproject-rtos/sdk-ng/releases$(NC)\n"
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	@printf "$(GREEN)✅ Setup complete! Run 'make build-hw' or 'make build-qemu' next$(NC)\n"

# Reinitialize project without fresh west init (for existing projects)
reinit: check-uv ## Complete setup for existing project (skip west init)
	@printf "$(GREEN)🔄 Reinitializing existing project...$(NC)\n"
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	@printf "$(YELLOW)Step 1/5: Installing west with uv$(NC)\n"
	uv add west
	@printf "$(GREEN)Step 2/5: Skipping west workspace initialization$(NC)\n"
	@printf "$(YELLOW)Step 3/5: Updating dependencies (this may take a few minutes)$(NC)\n"
	uv run west update
	@printf "$(YELLOW)Step 4/5: Installing Python requirements$(NC)\n"
	uv pip install -r deps/zephyr/scripts/requirements.txt
	@printf "$(YELLOW)Step 5/5: Setting up Zephyr environment$(NC)\n"
	uv run west zephyr-export
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	@printf "$(GREEN)✅ Reinitialization complete!$(NC)\n"

# Setup Python environment and west (for existing projects)
setup: check-uv ## Setup Python environment and install west
	@printf "$(GREEN)⚙️  Setting up development environment...$(NC)\n"
	uv add west
	@if [ ! -f ".west/config" ]; then \
		printf "$(YELLOW)Initializing west workspace...$(NC)\n"; \
		uv run west init -l $(APP_DIR); \
	fi
	@printf "$(GREEN)✅ Environment ready$(NC)\n"

#==============================================================================
# BUILD TARGETS FOR FIRMWARE COMPILATION
#==============================================================================

# Default target - hardware build for convenience
build: build-hw ## Default build target (hardware)

# Build firmware for nRF52840 hardware target
build-hw: ## Build for nRF52840 hardware
	@printf "$(GREEN)🔨 Building firmware for nRF52840 hardware...$(NC)\n"
	@printf "$(CYAN)Board: $(BOARD_HW)$(NC)\n"
	@cd $(APP_DIR) && uv run west build -p -b $(BOARD_HW) -d ../$(BUILD_DIR)
	@printf "$(GREEN)✅ Hardware build complete$(NC)\n"
	@printf "$(BLUE)Binary location: $(BUILD_DIR)/zephyr/zephyr.hex$(NC)\n"

# Build firmware for QEMU emulation target
build-qemu: ## Build for QEMU emulation
	@printf "$(GREEN)🔨 Building firmware for QEMU emulation...$(NC)\n"
	@printf "$(CYAN)Board: $(BOARD_QEMU)$(NC)\n"
	@cd $(APP_DIR) && uv run west build -p -b $(BOARD_QEMU) -d ../$(BUILD_DIR)
	@printf "$(GREEN)✅ QEMU build complete$(NC)\n"
	@printf "$(BLUE)Binary location: $(BUILD_DIR)/zephyr/zephyr.elf$(NC)\n"

#==============================================================================
# FIRMWARE DEPLOYMENT AND EXECUTION TARGETS
#==============================================================================

# Flash firmware to nRF52840 hardware
flash: ## Flash to nRF52840 hardware
	@printf "$(GREEN)⚡ Flashing firmware to nRF52840...$(NC)\n"
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		printf "$(RED)❌ Error: Build directory not found. Run 'make build-hw' first$(NC)\n"; \
		exit 1; \
	fi
	@cd $(APP_DIR) && uv run west flash -d ../$(BUILD_DIR)
	@printf "$(GREEN)✅ Flash complete$(NC)\n"

# Run firmware in QEMU emulator
run-qemu: ## Run in QEMU emulation
	@printf "$(GREEN)🖥️  Starting QEMU emulation...$(NC)\n"
	@printf "$(YELLOW)Press Ctrl+A then X to exit QEMU$(NC)\n"
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		printf "$(RED)❌ Error: Build directory not found. Run 'make build-qemu' first$(NC)\n"; \
		exit 1; \
	fi
	@cd $(APP_DIR) && uv run west build -t run -d ../$(BUILD_DIR)

#==============================================================================
# DOCUMENTATION GENERATION TARGETS (DOXYGEN)
#==============================================================================

# Generate comprehensive API documentation using Doxygen
docs: check-doxygen ## Generate complete API documentation with Doxygen
	@printf "$(GREEN)📚 Generating API documentation with Doxygen...$(NC)\n"
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	@printf "$(CYAN)Configuration file: $(DOXYFILE)$(NC)\n"
	@printf "$(CYAN)Output directory: $(DOCS_OUTPUT_DIR)$(NC)\n"
	@mkdir -p $(DOCS_OUTPUT_DIR)
	@doxygen $(DOXYFILE)
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	@printf "$(GREEN)✅ Documentation generated successfully!$(NC)\n"
	@printf "$(BLUE)📖 Open docs/generated/html/index.html in your browser$(NC)\n"
	@printf "$(YELLOW)💡 Tip: Use 'make docs-open' to open automatically$(NC)\n"

# Clean all generated documentation files
docs-clean: ## Clean generated documentation files
	@printf "$(GREEN)🧹 Cleaning documentation files...$(NC)\n"
	@rm -rf $(DOCS_OUTPUT_DIR)
	@printf "$(GREEN)✅ Documentation files cleaned$(NC)\n"

# Open generated documentation in the default browser
docs-open: ## Open generated documentation in default browser
	@printf "$(GREEN)🌐 Opening documentation in browser...$(NC)\n"
	@if [ ! -f "$(DOCS_HTML_DIR)/index.html" ]; then \
		printf "$(RED)❌ Error: Documentation not found. Run 'make docs' first$(NC)\n"; \
		exit 1; \
	fi
	@if command -v xdg-open > /dev/null; then \
		xdg-open $(DOCS_HTML_DIR)/index.html; \
	elif command -v open > /dev/null; then \
		open $(DOCS_HTML_DIR)/index.html; \
	else \
		printf "$(YELLOW)⚠️  Cannot detect browser opener. Manually open: $(DOCS_HTML_DIR)/index.html$(NC)\n"; \
	fi

# Serve documentation locally using Python's built-in HTTP server
docs-serve: ## Serve documentation locally on port 8080
	@printf "$(GREEN)🌐 Starting local documentation server...$(NC)\n"
	@if [ ! -f "$(DOCS_HTML_DIR)/index.html" ]; then \
		printf "$(RED)❌ Error: Documentation not found. Run 'make docs' first$(NC)\n"; \
		exit 1; \
	fi
	@printf "$(CYAN)Server URL: http://localhost:8080$(NC)\n"
	@printf "$(YELLOW)Press Ctrl+C to stop the server$(NC)\n"
	@cd $(DOCS_HTML_DIR) && python3 -m http.server 8080

#==============================================================================
# DEPENDENCY MANAGEMENT AND MAINTENANCE TARGETS
#==============================================================================

# Update Zephyr and project dependencies
deps-update: ## Update Zephyr dependencies
	@printf "$(GREEN)📦 Updating Zephyr dependencies...$(NC)\n"
	@printf "$(YELLOW)This may take several minutes...$(NC)\n"
	uv run west update
	@printf "$(GREEN)✅ Dependencies updated$(NC)\n"

#==============================================================================
# CLEANUP AND MAINTENANCE TARGETS
#==============================================================================

# Clean build artifacts only
clean: ## Clean build artifacts
	@printf "$(GREEN)🧹 Cleaning build artifacts...$(NC)\n"
	@rm -rf $(BUILD_DIR)
	@printf "$(GREEN)✅ Build artifacts cleaned$(NC)\n"

# Clean everything including dependencies and documentation
clean-all: ## Clean everything including dependencies and docs
	@printf "$(GREEN)🧹 Cleaning everything (build, deps, docs)...$(NC)\n"
	@printf "$(YELLOW)This will remove: $(BUILD_DIR), deps, .west, docs/generated$(NC)\n"
	@rm -rf $(BUILD_DIR) deps .west $(DOCS_OUTPUT_DIR) uv.lock
	@printf "$(GREEN)✅ Everything cleaned$(NC)\n"

#==============================================================================
# DEVELOPMENT WORKFLOW SHORTCUTS
#==============================================================================

# Build and flash hardware in one command
dev-hw: build-hw flash ## Build and flash hardware in one step
	@printf "$(GREEN)🚀 Hardware development cycle complete!$(NC)\n"

# Build and run QEMU in one command
dev-qemu: build-qemu run-qemu ## Build and run QEMU in one step
	@printf "$(GREEN)🚀 QEMU development cycle complete!$(NC)\n"

#==============================================================================
# ENVIRONMENT VALIDATION AND INFORMATION TARGETS
#==============================================================================

# Comprehensive environment check
check-env: ## Check if environment is properly set up
	@printf "$(GREEN)🔍 Checking development environment...$(NC)\n"
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	
	# Check UV
	@printf "$(CYAN)Checking UV package manager...$(NC)\n"
	@if ! command -v uv > /dev/null 2>&1; then \
		printf "$(RED)❌ UV not available. Run installation command$(NC)\n"; \
		exit 1; \
	else \
		printf "$(GREEN)✅ UV available: $(NC)"; uv --version; \
	fi
	
	# Check West
	@printf "$(CYAN)Checking West build tool...$(NC)\n"
	@if ! uv run west --version > /dev/null 2>&1; then \
		printf "$(RED)❌ West not available. Run 'make setup' first$(NC)\n"; \
		exit 1; \
	else \
		printf "$(GREEN)✅ West available: $(NC)"; uv run west --version; \
	fi
	
	# Check Zephyr dependencies
	@printf "$(CYAN)Checking Zephyr dependencies...$(NC)\n"
	@if [ ! -d "deps/zephyr" ]; then \
		printf "$(RED)❌ Zephyr dependencies not found. Run 'make init' or 'make deps-update'$(NC)\n"; \
		exit 1; \
	else \
		printf "$(GREEN)✅ Zephyr dependencies found$(NC)\n"; \
	fi
	
	# Check Doxygen (optional)
	@printf "$(CYAN)Checking Doxygen (for documentation)...$(NC)\n"
	@if ! command -v doxygen > /dev/null 2>&1; then \
		printf "$(YELLOW)⚠️  Doxygen not available (optional for docs generation)$(NC)\n"; \
	else \
		printf "$(GREEN)✅ Doxygen available: $(NC)"; doxygen --version; \
	fi
	
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	@printf "$(GREEN)✅ Environment check complete$(NC)\n"

# Display comprehensive project information
info: ## Show project information
	@printf "$(GREEN)📋 Project Information$(NC)\n"
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	@printf "$(CYAN)Project:$(NC)         NISC Wearable Device\n"
	@printf "$(CYAN)Hardware board:$(NC)  $(BOARD_HW)\n"
	@printf "$(CYAN)QEMU board:$(NC)      $(BOARD_QEMU)\n"
	@printf "$(CYAN)Build directory:$(NC) $(BUILD_DIR)\n"
	@printf "$(CYAN)App directory:$(NC)   $(APP_DIR)\n"
	@printf "$(CYAN)Docs directory:$(NC)  $(DOCS_OUTPUT_DIR)\n"
	@printf "$(PURPLE)═══════════════════════════════════════════════$(NC)\n"
	
	# Show tool versions if available
	@if uv run west --version > /dev/null 2>&1; then \
		printf "$(CYAN)West version:$(NC)    "; \
		uv run west --version; \
	fi
	@if command -v doxygen > /dev/null 2>&1; then \
		printf "$(CYAN)Doxygen version:$(NC) "; \
		doxygen --version; \
	fi
	@if [ -d "deps/zephyr" ]; then \
		printf "$(CYAN)Zephyr path:$(NC)     deps/zephyr\n"; \
	fi

#==============================================================================
# TERMINAL COLOR SUPPORT
#==============================================================================

# Support for NO_COLOR standard (disable colors if requested)
ifdef NO_COLOR
	GREEN :=
	YELLOW :=
	RED :=
	BLUE :=
	CYAN :=
	PURPLE :=
	NC :=
endif

