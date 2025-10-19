/**
 * @file bootloader.h
 * @brief Secure bootloader interface for NISC Medical Wearable Device
 * @details Provides bootloader functionality including DFU support, image
 * validation, and secure boot for medical device firmware updates.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* Bootloader Configuration                                                   */
/*============================================================================*/

/** @brief Maximum firmware image size (256KB) */
#define BOOTLOADER_MAX_IMAGE_SIZE    (256 * 1024)

/** @brief Bootloader version */
#define BOOTLOADER_VERSION_MAJOR     1U
#define BOOTLOADER_VERSION_MINOR     0U
#define BOOTLOADER_VERSION_PATCH     0U

/** @brief Button press duration for DFU mode (ms) */
#define BOOTLOADER_BUTTON_HOLD_MS    3000U

/*============================================================================*/
/* Boot Modes                                                                 */
/*============================================================================*/

typedef enum {
    BOOT_MODE_NORMAL = 0,         /**< Normal application boot */
    BOOT_MODE_DFU,                /**< DFU mode for firmware update */
    BOOT_MODE_RECOVERY,           /**< Recovery mode */
    BOOT_MODE_FACTORY_RESET       /**< Factory reset mode */
} boot_mode_t;

/*============================================================================*/
/* Boot Status                                                                */
/*============================================================================*/

typedef enum {
    BOOT_STATUS_OK = 0,
    BOOT_STATUS_ERROR_VALIDATION,
    BOOT_STATUS_ERROR_SIGNATURE,
    BOOT_STATUS_ERROR_VERSION,
    BOOT_STATUS_ERROR_CORRUPTION
} boot_status_t;

/*============================================================================*/
/* Image Information                                                          */
/*============================================================================*/

typedef struct {
    uint32_t magic;               /**< Magic number for validation */
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    uint32_t image_size;
    uint32_t crc32;
    uint32_t timestamp;
    uint8_t  signature[64];       /**< Digital signature */
} image_header_t;

/*============================================================================*/
/* Boot Information                                                           */
/*============================================================================*/

typedef struct {
    boot_mode_t mode;
    uint32_t boot_count;
    uint32_t last_boot_time;
    uint32_t reset_reason;
    bool dfu_requested;
    bool button_pressed;
} boot_info_t;

/*============================================================================*/
/* Public API Functions                                                       */
/*============================================================================*/

/**
 * @brief Initialize bootloader subsystem
 * @return 0 on success, negative error code otherwise
 */
int bootloader_init(void);

/**
 * @brief Check boot mode (normal vs DFU)
 * @details Checks for button press or DFU flag to determine boot mode
 * @return Detected boot mode
 */
boot_mode_t bootloader_check_boot_mode(void);

/**
 * @brief Wait for button press to determine boot mode
 * @details Waits for specified timeout or button press
 * @param timeout_ms Timeout in milliseconds
 * @return true if DFU mode requested (button held)
 */
bool bootloader_wait_for_button(uint32_t timeout_ms);

/**
 * @brief Validate firmware image
 * @param header Pointer to image header
 * @return BOOT_STATUS_OK if valid
 */
boot_status_t bootloader_validate_image(const image_header_t *header);

/**
 * @brief Enter DFU mode
 * @details Initializes Bluetooth advertising for DFU
 * @return 0 on success
 */
int bootloader_enter_dfu_mode(void);

/**
 * @brief Start normal application
 * @return 0 on success (should not return)
 */
int bootloader_start_application(void);

/**
 * @brief Get boot information
 * @param info Pointer to boot_info_t structure
 * @return 0 on success
 */
int bootloader_get_info(boot_info_t *info);

/**
 * @brief Set DFU request flag
 * @details Sets flag to enter DFU mode on next boot
 */
void bootloader_request_dfu(void);

/**
 * @brief Clear DFU request flag
 */
void bootloader_clear_dfu_request(void);

#endif /* BOOTLOADER_H */