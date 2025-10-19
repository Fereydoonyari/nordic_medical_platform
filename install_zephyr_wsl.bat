@echo off
REM NISC Medical Device - Windows WSL Zephyr Installation Script
REM This script runs the automated Zephyr installation in WSL

echo === NISC Medical Device - WSL Zephyr Installation ===
echo.

REM Check if WSL is available
wsl --status >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: WSL is not available or not running
    echo Please install WSL2 and Ubuntu first
    echo.
    echo To install WSL2:
    echo 1. Run: wsl --install
    echo 2. Restart your computer
    echo 3. Run this script again
    pause
    exit /b 1
)

echo WSL is available. Starting installation...
echo.

REM Copy the installation script to WSL
echo Copying installation script to WSL...
wsl cp /mnt/c/Users/zephyrus/Documents/GitHub/nordic_medical_platform/install_zephyr.sh ~/install_zephyr.sh

REM Make the script executable and run it
echo Running Zephyr installation in WSL...
wsl chmod +x ~/install_zephyr.sh
wsl ~/install_zephyr.sh

if %errorlevel% equ 0 (
    echo.
    echo === Installation Completed Successfully! ===
    echo.
    echo Next steps:
    echo 1. Open WSL terminal
    echo 2. Run: source ~/.bashrc
    echo 3. Navigate to project: cd ~/zephyr/nordic_medical_platform
    echo 4. Build project: cd app ^&^& west build -b nrf52840dk_nrf52840
    echo 5. Flash device: west flash
    echo.
) else (
    echo.
    echo === Installation Failed! ===
    echo Please check the error messages above and try again.
    echo.
)

pause
