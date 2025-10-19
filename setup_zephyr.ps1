# Zephyr Development Environment Setup Script
# Run this script to set up Zephyr for the NISC Medical Device project

Write-Host "=== NISC Medical Device - Zephyr Setup ===" -ForegroundColor Green
Write-Host ""

# Check if Python is installed
Write-Host "Checking Python installation..." -ForegroundColor Yellow
try {
    $pythonVersion = python --version 2>&1
    Write-Host "Python found: $pythonVersion" -ForegroundColor Green
} catch {
    Write-Host "Python not found! Please install Python from https://www.python.org/downloads/" -ForegroundColor Red
    Write-Host "Make sure to check 'Add Python to PATH' during installation" -ForegroundColor Red
    exit 1
}

# Check if pip is installed
Write-Host "Checking pip installation..." -ForegroundColor Yellow
try {
    $pipVersion = pip --version 2>&1
    Write-Host "Pip found: $pipVersion" -ForegroundColor Green
} catch {
    Write-Host "Pip not found! Please install pip" -ForegroundColor Red
    exit 1
}

# Install West
Write-Host "Installing West tool..." -ForegroundColor Yellow
pip install west

# Verify West installation
Write-Host "Verifying West installation..." -ForegroundColor Yellow
try {
    $westVersion = west --version 2>&1
    Write-Host "West installed: $westVersion" -ForegroundColor Green
} catch {
    Write-Host "West installation failed!" -ForegroundColor Red
    exit 1
}

# Initialize Zephyr workspace
Write-Host "Initializing Zephyr workspace..." -ForegroundColor Yellow
west init -m https://github.com/zephyrproject-rtos/zephyr

# Update workspace
Write-Host "Updating Zephyr workspace..." -ForegroundColor Yellow
west update

# Install Python dependencies
Write-Host "Installing Python dependencies..." -ForegroundColor Yellow
pip install -r zephyr/scripts/requirements.txt

Write-Host ""
Write-Host "=== Setup Complete! ===" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Download Zephyr SDK from: https://github.com/zephyrproject-rtos/sdk-ng/releases" -ForegroundColor White
Write-Host "2. Download: zephyr-sdk-0.16.5_windows-x86_64.zip" -ForegroundColor White
Write-Host "3. Extract to C:\zephyr-sdk-0.16.5" -ForegroundColor White
Write-Host "4. Run: C:\zephyr-sdk-0.16.5\setup.cmd" -ForegroundColor White
Write-Host "5. Set environment variables:" -ForegroundColor White
Write-Host "   `$env:ZEPHYR_BASE = `"$PWD\zephyr`"" -ForegroundColor White
Write-Host "   `$env:ZEPHYR_SDK_INSTALL_DIR = `"C:\zephyr-sdk-0.16.5`"" -ForegroundColor White
Write-Host "6. Build the project:" -ForegroundColor White
Write-Host "   cd app" -ForegroundColor White
Write-Host "   west build -b nrf52840dk_nrf52840" -ForegroundColor White
Write-Host ""
Write-Host "For detailed instructions, see ZEPHYR_SETUP_GUIDE.md" -ForegroundColor Cyan
