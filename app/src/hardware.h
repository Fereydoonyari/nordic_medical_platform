/**
 * @file hardware.h
 * @brief Hardware abstraction layer for nRF52840 Development Kit
 * @details Provides unified interface for LED control, GPIO operations, USB console,
 * and hardware information for the medical wearable device platform.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This module abstracts Nordic-specific hardware details and provides
 * a clean interface for medical device application development.
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/hwinfo.h>
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* Hardware Configuration Constants                                           */
/*============================================================================*/

/** @defgroup HwConfig Hardware Configuration
 * @brief Hardware-specific configuration constants for nRF52840DK
 * @{
 */

/** @brief Number of user-controllable LEDs on nRF52840DK */
#define HW_LED_COUNT                   4U

/** @brief LED identifiers for nRF52840DK */
#define HW_LED_STATUS                  0U  /**< LED1 - System status indicator */
#define HW_LED_HEARTBEAT              1U  /**< LED2 - Medical pulse indicator */
#define HW_LED_COMMUNICATION          2U  /**< LED3 - Communication activity */
#define HW_LED_ERROR                  3U  /**< LED4 - Error/warning indicator */

/** @brief GPIO pin assignments for nRF52840DK LEDs */
#define HW_LED1_PIN                   13U  /**< P0.13 - LED1 (Status) */
#define HW_LED2_PIN                   14U  /**< P0.14 - LED2 (Heartbeat) */
#define HW_LED3_PIN                   15U  /**< P0.15 - LED3 (Communication) */
#define HW_LED4_PIN                   16U  /**< P0.16 - LED4 (Error) */

/** @brief Button pin for DFU boot process */
#define HW_BUTTON_PIN                 11U  /**< P0.11 - Button 1 */

/** @brief USB console ready timeout in milliseconds */
#define HW_USB_CONSOLE_TIMEOUT_MS     3000U

/** @} */ /* End of HwConfig group */

/*============================================================================*/
/* LED Pattern Definitions                                                   */
/*============================================================================*/

/** @defgroup HwLedPatterns LED Pattern Types
 * @brief Predefined LED patterns for medical device status indication
 * @details These patterns provide visual feedback for different system states
 * and medical conditions, following medical device UI/UX best practices.
 * @{
 */

/** @brief LED pattern enumeration */
typedef enum {
    HW_PULSE_OFF = 0,              /**< LED off (no activity) */
    HW_PULSE_ON,                   /**< LED on (constant) */
    HW_PULSE_SLOW_BLINK,           /**< Slow blink (1Hz) - warning */
    HW_PULSE_FAST_BLINK,           /**< Fast blink (4Hz) - alert */
    HW_PULSE_BREATHING,            /**< Breathing pattern - system OK */
    HW_PULSE_HEARTBEAT,            /**< Heartbeat pattern - medical pulse */
    HW_PULSE_SOS,                  /**< SOS pattern - critical error */
    HW_PULSE_DOUBLE_BLINK,         /**< Double blink - caution */
    HW_PULSE_PATTERN_MAX           /**< Maximum pattern count */
} hw_led_pattern_t;

/** @} */ /* End of HwLedPatterns group */

/*============================================================================*/
/* Hardware Information Structures                                            */
/*============================================================================*/

/** @defgroup HwInfo Hardware Information
 * @brief Structures for hardware status and device information
 * @{
 */

/** @brief Hardware information structure */
typedef struct {
    uint8_t device_id[8];          /**< Unique device identifier */
    uint32_t reset_cause;           /**< Reset cause register value */
    bool usb_console_ready;        /**< USB console connection status */
    bool leds_initialized;          /**< LED subsystem initialization status */
    bool gpio_initialized;         /**< GPIO subsystem initialization status */
    uint32_t uptime_ms;            /**< System uptime in milliseconds */
} hw_info_t;

/** @brief LED pattern state structure */
typedef struct {
    hw_led_pattern_t pattern;       /**< Current pattern type */
    uint32_t pattern_start_ms;      /**< Pattern start timestamp */
    uint32_t cycle_count;           /**< Pattern cycle counter */
    bool state;                     /**< Current LED state */
} hw_led_state_t;

/** @} */ /* End of HwInfo group */

/*============================================================================*/
/* Function Return Codes                                                      */
/*============================================================================*/

/** @defgroup HwReturnCodes Hardware Return Codes
 * @brief Return codes for hardware operations
 * @{
 */

#define HW_OK                        0   /**< Operation successful */
#define HW_ERROR_INIT                -1  /**< Hardware initialization failed */
#define HW_ERROR_GPIO                -2  /**< GPIO operation failed */
#define HW_ERROR_LED                 -3  /**< LED operation failed */
#define HW_ERROR_USB                 -4  /**< USB operation failed */
#define HW_ERROR_INVALID_PARAM       -5  /**< Invalid parameter */
#define HW_ERROR_NOT_READY           -6  /**< Hardware not ready */

/** @} */ /* End of HwReturnCodes group */

/*============================================================================*/
/* Public Function Declarations                                               */
/*============================================================================*/

/**
 * @brief Initialize hardware abstraction layer
 * @details Initializes all hardware subsystems including GPIO, LEDs, and USB console.
 * This function must be called before any other hardware operations.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_init(void);

/**
 * @brief Get hardware information
 * @details Retrieves comprehensive hardware status and device information.
 * 
 * @param[out] info Pointer to hardware info structure to populate
 * @return HW_OK on success, error code on failure
 */
int hw_get_info(hw_info_t *info);

/**
 * @brief Check if USB console is ready
 * @details Determines if USB CDC-ACM console is available and ready for use.
 * 
 * @return true if USB console is ready, false otherwise
 */
bool hw_usb_console_ready(void);

/*============================================================================*/
/* LED Control Functions                                                      */
/*============================================================================*/

/**
 * @brief Set LED state (on/off)
 * @details Directly controls LED state without pattern animation.
 * 
 * @param led_id LED identifier (HW_LED_STATUS, HW_LED_HEARTBEAT, etc.)
 * @param state true for on, false for off
 * @return HW_OK on success, error code on failure
 */
int hw_led_set_state(uint32_t led_id, bool state);

/**
 * @brief Set LED pattern
 * @details Sets a predefined pattern for the specified LED. The pattern
 * will be automatically animated by the hardware update thread.
 * 
 * @param led_id LED identifier (HW_LED_STATUS, HW_LED_HEARTBEAT, etc.)
 * @param pattern Pattern type to set
 * @return HW_OK on success, error code on failure
 */
int hw_led_set_pattern(uint32_t led_id, hw_led_pattern_t pattern);

/**
 * @brief Update LED patterns
 * @details Updates all active LED patterns. This function should be called
 * periodically (e.g., every 50ms) to maintain smooth pattern animation.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_led_update_patterns(void);

/**
 * @brief Show medical pulse on heartbeat LED
 * @details Displays a medical pulse pattern synchronized with heart rate data.
 * 
 * @param heart_rate_bpm Heart rate in beats per minute
 * @return HW_OK on success, error code on failure
 */
int hw_show_medical_pulse(uint32_t heart_rate_bpm);

/**
 * @brief Test LED patterns
 * @details Tests all LED patterns for hardware validation and debugging.
 * 
 * @param pattern Pattern to test (or HW_PULSE_PATTERN_MAX for all)
 * @return HW_OK on success, error code on failure
 */
int hw_led_test_patterns(hw_led_pattern_t pattern);

/*============================================================================*/
/* GPIO and Button Functions                                                  */
/*============================================================================*/

/**
 * @brief Initialize button for DFU boot process
 * @details Sets up button GPIO with interrupt support for DFU boot detection.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_button_init(void);

/**
 * @brief Check if DFU button is pressed
 * @details Checks current state of the DFU boot button.
 * 
 * @return true if button is pressed, false otherwise
 */
bool hw_button_is_pressed(void);

/**
 * @brief Wait for button press with timeout
 * @details Waits for button press with optional timeout for DFU boot process.
 * 
 * @param timeout_ms Timeout in milliseconds (0 for no timeout)
 * @return true if button was pressed, false if timeout occurred
 */
bool hw_button_wait_press(uint32_t timeout_ms);

/**
 * @brief Get button press count since last reset
 * @details Returns the number of button presses detected since system reset.
 * 
 * @return Number of button presses
 */
uint32_t hw_button_get_press_count(void);

/*============================================================================*/
/* DFU Boot Process Functions                                                 */
/*============================================================================*/

/**
 * @brief Initialize DFU boot process
 * @details Sets up DFU boot process with button detection and bootloader interface.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_dfu_init(void);

/**
 * @brief Check if DFU boot is requested
 * @details Determines if DFU boot process should be activated based on button state.
 * 
 * @return true if DFU boot is requested, false for normal boot
 */
bool hw_dfu_boot_requested(void);

/**
 * @brief Enter DFU boot mode
 * @details Activates DFU boot mode with LED indication and bootloader communication.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_dfu_enter_boot_mode(void);

/**
 * @brief Exit DFU boot mode
 * @details Exits DFU boot mode and returns to normal application operation.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_dfu_exit_boot_mode(void);

/*============================================================================*/
/* Bluetooth and Serial Communication Functions                               */
/*============================================================================*/

/**
 * @brief Initialize Bluetooth Low Energy advertising
 * @details Sets up BLE advertising for medical device discovery and communication.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_ble_advertising_init(void);

/**
 * @brief Start Bluetooth advertising
 * @details Starts BLE advertising with medical device information.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_ble_advertising_start(void);

/**
 * @brief Stop Bluetooth advertising
 * @details Stops BLE advertising.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_ble_advertising_stop(void);

/**
 * @brief Set Bluetooth advertising data
 * @details Updates BLE advertising data with current medical device information.
 * 
 * @param device_name Device name for advertising
 * @param medical_data Pointer to medical data to advertise
 * @return HW_OK on success, error code on failure
 */
int hw_ble_set_advertising_data(const char *device_name, const void *medical_data);

/**
 * @brief Initialize serial communication for Bluetooth
 * @details Sets up serial communication interface for Bluetooth module communication.
 * 
 * @return HW_OK on success, error code on failure
 */
int hw_serial_bt_init(void);

/**
 * @brief Send data via serial Bluetooth interface
 * @details Sends data through serial interface to Bluetooth module.
 * 
 * @param data Pointer to data to send
 * @param length Data length in bytes
 * @return HW_OK on success, error code on failure
 */
int hw_serial_bt_send(const uint8_t *data, uint32_t length);

/**
 * @brief Receive data via serial Bluetooth interface
 * @details Receives data from serial interface from Bluetooth module.
 * 
 * @param buffer Pointer to buffer for received data
 * @param max_length Maximum buffer length
 * @param[out] received_length Actual received data length
 * @return HW_OK on success, error code on failure
 */
int hw_serial_bt_receive(uint8_t *buffer, uint32_t max_length, uint32_t *received_length);

#endif /* HARDWARE_H */
