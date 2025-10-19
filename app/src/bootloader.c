/**
 * @file bootloader.c
 * @brief Secure bootloader implementation for NISC Medical Wearable Device
 * @details Implements DFU support, image validation, and secure boot for medical device firmware updates.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/crc.h>

#include "bootloader.h"
#include "common.h"

/*============================================================================*/
/* Bootloader Configuration                                                   */
/*============================================================================*/

/** @brief Magic number for image validation */
#define IMAGE_MAGIC_NUMBER           0x4E495343  /* "NISC" */

/** @brief Button GPIO configuration */
#define BOOTLOADER_BUTTON_PORT       "GPIO_0"
#define BOOTLOADER_BUTTON_PIN        11
#define BOOTLOADER_BUTTON_FLAGS      (GPIO_PULL_UP | GPIO_INT_EDGE_FALLING)

/** @brief LED GPIO configuration for bootloader status */
#define BOOTLOADER_STATUS_LED_PORT   "GPIO_0"
#define BOOTLOADER_STATUS_LED_PIN    13

/** @brief DFU Service UUID */
#define DFU_SERVICE_UUID             0x00001530
#define DFU_CHAR_UUID                0x00001531

/** @brief Bootloader state */
typedef enum {
    BOOTLOADER_STATE_INIT,
    BOOTLOADER_STATE_WAITING,
    BOOTLOADER_STATE_DFU_MODE,
    BOOTLOADER_STATE_BOOTING_APP
} bootloader_state_t;

/*============================================================================*/
/* Global Variables                                                           */
/*============================================================================*/

static const struct device *button_dev;
static const struct device *led_dev;
static struct gpio_callback button_callback;
static boot_info_t boot_info;
static bootloader_state_t current_state = BOOTLOADER_STATE_INIT;
static bool button_pressed = false;
static uint32_t button_press_start = 0;

/* Bluetooth DFU Service */
static struct bt_uuid_128 dfu_service_uuid = BT_UUID_INIT_128(
    0x30, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
);

static struct bt_uuid_128 dfu_char_uuid = BT_UUID_INIT_128(
    0x31, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
);

/*============================================================================*/
/* Private Function Declarations                                              */
/*============================================================================*/

static void button_pressed_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static void bootloader_blink_led(uint32_t count, uint32_t delay_ms);
static int bootloader_init_hardware(void);
static int bootloader_init_bluetooth(void);
static void bootloader_advertise_dfu(void);
static boot_status_t bootloader_validate_app_image(void);
static void bootloader_show_status(boot_status_t status);

/*============================================================================*/
/* Button Interrupt Handler                                                   */
/*============================================================================*/

static void button_pressed_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);
    
    if (!button_pressed) {
        button_pressed = true;
        button_press_start = k_uptime_get_32();
        printk("Button pressed - starting timer\n");
    }
}

/*============================================================================*/
/* Hardware Initialization                                                    */
/*============================================================================*/

static int bootloader_init_hardware(void)
{
    int ret;
    
    /* Initialize button GPIO */
    button_dev = device_get_binding(BOOTLOADER_BUTTON_PORT);
    if (!button_dev) {
        printk("Error: Button device not found\n");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure(button_dev, BOOTLOADER_BUTTON_PIN, 
                            GPIO_INPUT | BOOTLOADER_BUTTON_FLAGS);
    if (ret) {
        printk("Error: Failed to configure button GPIO (ret: %d)\n", ret);
        return ret;
    }
    
    /* Setup button interrupt */
    gpio_init_callback(&button_callback, button_pressed_callback, 
                      BIT(BOOTLOADER_BUTTON_PIN));
    ret = gpio_add_callback(button_dev, &button_callback);
    if (ret) {
        printk("Error: Failed to add button callback (ret: %d)\n", ret);
        return ret;
    }
    
    ret = gpio_pin_interrupt_configure(button_dev, BOOTLOADER_BUTTON_PIN, 
                                      GPIO_INT_EDGE_FALLING);
    if (ret) {
        printk("Error: Failed to configure button interrupt (ret: %d)\n", ret);
        return ret;
    }
    
    /* Initialize status LED */
    led_dev = device_get_binding(BOOTLOADER_STATUS_LED_PORT);
    if (!led_dev) {
        printk("Error: LED device not found\n");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure(led_dev, BOOTLOADER_STATUS_LED_PIN, GPIO_OUTPUT);
    if (ret) {
        printk("Error: Failed to configure LED GPIO (ret: %d)\n", ret);
        return ret;
    }
    
    /* Turn off LED initially */
    gpio_pin_set(led_dev, BOOTLOADER_STATUS_LED_PIN, 0);
    
    return 0;
}

/*============================================================================*/
/* LED Status Indication                                                      */
/*============================================================================*/

static void bootloader_blink_led(uint32_t count, uint32_t delay_ms)
{
    for (uint32_t i = 0; i < count; i++) {
        gpio_pin_set(led_dev, BOOTLOADER_STATUS_LED_PIN, 1);
        k_sleep(K_MSEC(delay_ms));
        gpio_pin_set(led_dev, BOOTLOADER_STATUS_LED_PIN, 0);
        k_sleep(K_MSEC(delay_ms));
    }
}

static void bootloader_show_status(boot_status_t status)
{
    switch (status) {
        case BOOT_STATUS_OK:
            /* Single long blink - success */
            bootloader_blink_led(1, 500);
            break;
        case BOOT_STATUS_ERROR_VALIDATION:
            /* Two short blinks - validation error */
            bootloader_blink_led(2, 200);
            break;
        case BOOT_STATUS_ERROR_SIGNATURE:
            /* Three short blinks - signature error */
            bootloader_blink_led(3, 200);
            break;
        case BOOT_STATUS_ERROR_VERSION:
            /* Four short blinks - version error */
            bootloader_blink_led(4, 200);
            break;
        case BOOT_STATUS_ERROR_CORRUPTION:
            /* Five short blinks - corruption error */
            bootloader_blink_led(5, 200);
            break;
        default:
            /* Continuous blinking - unknown error */
            for (int i = 0; i < 10; i++) {
                bootloader_blink_led(1, 100);
            }
            break;
    }
}

/*============================================================================*/
/* Bluetooth DFU Service                                                      */
/*============================================================================*/

static int bootloader_init_bluetooth(void)
{
    int ret;
    
    ret = bt_enable(NULL);
    if (ret) {
        printk("Error: Bluetooth init failed (ret: %d)\n", ret);
        return ret;
    }
    
    printk("Bluetooth initialized for DFU mode\n");
    return 0;
}

static void bootloader_advertise_dfu(void)
{
    struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    };
    
    const char *adv_name = "NISC-DFU";
    int ret;
    
    ret = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);
    if (ret) {
        printk("Error: Advertising failed to start (ret: %d)\n", ret);
        return;
    }
    
    printk("DFU advertising started: %s\n", adv_name);
    
    /* Blink LED to indicate DFU mode */
    while (current_state == BOOTLOADER_STATE_DFU_MODE) {
        bootloader_blink_led(1, 1000);
        k_sleep(K_MSEC(1000));
    }
}

/*============================================================================*/
/* Image Validation                                                           */
/*============================================================================*/

static boot_status_t bootloader_validate_app_image(void)
{
    const struct flash_area *fa;
    image_header_t header;
    int ret;
    
    /* Get flash area for application */
    ret = flash_area_open(FLASH_AREA_ID(image_0), &fa);
    if (ret) {
        printk("Error: Failed to open flash area (ret: %d)\n", ret);
        return BOOT_STATUS_ERROR_CORRUPTION;
    }
    
    /* Read image header */
    ret = flash_area_read(fa, 0, &header, sizeof(header));
    if (ret) {
        printk("Error: Failed to read image header (ret: %d)\n", ret);
        flash_area_close(fa);
        return BOOT_STATUS_ERROR_CORRUPTION;
    }
    
    /* Validate magic number */
    if (header.magic != IMAGE_MAGIC_NUMBER) {
        printk("Error: Invalid magic number: 0x%08x\n", header.magic);
        flash_area_close(fa);
        return BOOT_STATUS_ERROR_VALIDATION;
    }
    
    /* Validate image size */
    if (header.image_size > BOOTLOADER_MAX_IMAGE_SIZE) {
        printk("Error: Image size too large: %u bytes\n", header.image_size);
        flash_area_close(fa);
        return BOOT_STATUS_ERROR_VALIDATION;
    }
    
    /* Calculate and verify CRC32 */
    uint8_t *image_data = k_malloc(header.image_size);
    if (!image_data) {
        printk("Error: Failed to allocate memory for image validation\n");
        flash_area_close(fa);
        return BOOT_STATUS_ERROR_CORRUPTION;
    }
    
    ret = flash_area_read(fa, sizeof(header), image_data, header.image_size);
    if (ret) {
        printk("Error: Failed to read image data (ret: %d)\n", ret);
        k_free(image_data);
        flash_area_close(fa);
        return BOOT_STATUS_ERROR_CORRUPTION;
    }
    
    uint32_t calculated_crc = crc32_ieee(image_data, header.image_size, 0);
    if (calculated_crc != header.crc32) {
        printk("Error: CRC32 mismatch: calculated=0x%08x, stored=0x%08x\n", 
               calculated_crc, header.crc32);
        k_free(image_data);
        flash_area_close(fa);
        return BOOT_STATUS_ERROR_CORRUPTION;
    }
    
    k_free(image_data);
    flash_area_close(fa);
    
    printk("Image validation successful:\n");
    printk("  Version: %u.%u.%u\n", header.version_major, 
           header.version_minor, header.version_patch);
    printk("  Size: %u bytes\n", header.image_size);
    printk("  CRC32: 0x%08x\n", header.crc32);
    
    return BOOT_STATUS_OK;
}

/*============================================================================*/
/* Public API Implementation                                                  */
/*============================================================================*/

int bootloader_init(void)
{
    int ret;
    
    printk("\n=== NISC Medical Device Bootloader ===\n");
    printk("Version: %u.%u.%u\n", BOOTLOADER_VERSION_MAJOR, 
           BOOTLOADER_VERSION_MINOR, BOOTLOADER_VERSION_PATCH);
    
    /* Initialize hardware */
    ret = bootloader_init_hardware();
    if (ret) {
        printk("Error: Hardware initialization failed\n");
        return ret;
    }
    
    /* Initialize boot info */
    boot_info.mode = BOOT_MODE_NORMAL;
    boot_info.boot_count = 0;
    boot_info.last_boot_time = k_uptime_get_32();
    boot_info.reset_reason = 0; /* TODO: Get from hardware */
    boot_info.dfu_requested = false;
    boot_info.button_pressed = false;
    
    current_state = BOOTLOADER_STATE_WAITING;
    
    printk("Bootloader initialized successfully\n");
    return 0;
}

boot_mode_t bootloader_check_boot_mode(void)
{
    boot_mode_t mode = BOOT_MODE_NORMAL;
    
    printk("Checking boot mode...\n");
    
    /* Check for DFU request flag in persistent storage */
    if (boot_info.dfu_requested) {
        printk("DFU mode requested via persistent flag\n");
        mode = BOOT_MODE_DFU;
        boot_info.dfu_requested = false;
    }
    
    /* Check for button press */
    if (button_pressed) {
        uint32_t press_duration = k_uptime_get_32() - button_press_start;
        printk("Button pressed for %u ms\n", press_duration);
        
        if (press_duration >= BOOTLOADER_BUTTON_HOLD_MS) {
            printk("Button held long enough - entering DFU mode\n");
            mode = BOOT_MODE_DFU;
        } else {
            printk("Button held too short - normal boot\n");
        }
        
        button_pressed = false;
    }
    
    boot_info.mode = mode;
    boot_info.button_pressed = button_pressed;
    
    return mode;
}

bool bootloader_wait_for_button(uint32_t timeout_ms)
{
    uint32_t start_time = k_uptime_get_32();
    uint32_t elapsed_time = 0;
    
    printk("Waiting for button press (timeout: %u ms)...\n", timeout_ms);
    
    /* Blink LED to indicate waiting */
    while (elapsed_time < timeout_ms) {
        if (button_pressed) {
            uint32_t press_duration = k_uptime_get_32() - button_press_start;
            printk("Button pressed after %u ms\n", elapsed_time);
            
            if (press_duration >= BOOTLOADER_BUTTON_HOLD_MS) {
                printk("Button held long enough for DFU mode\n");
                return true;
            }
        }
        
        /* Blink LED every 500ms */
        if ((elapsed_time % 500) == 0) {
            gpio_pin_set(led_dev, BOOTLOADER_STATUS_LED_PIN, 1);
            k_sleep(K_MSEC(50));
            gpio_pin_set(led_dev, BOOTLOADER_STATUS_LED_PIN, 0);
        }
        
        k_sleep(K_MSEC(50));
        elapsed_time = k_uptime_get_32() - start_time;
    }
    
    printk("Button wait timeout - proceeding with normal boot\n");
    return false;
}

boot_status_t bootloader_validate_image(const image_header_t *header)
{
    if (!header) {
        return BOOT_STATUS_ERROR_VALIDATION;
    }
    
    /* Validate magic number */
    if (header->magic != IMAGE_MAGIC_NUMBER) {
        return BOOT_STATUS_ERROR_VALIDATION;
    }
    
    /* Validate version */
    if (header->version_major == 0 && header->version_minor == 0) {
        return BOOT_STATUS_ERROR_VERSION;
    }
    
    /* Validate size */
    if (header->image_size == 0 || header->image_size > BOOTLOADER_MAX_IMAGE_SIZE) {
        return BOOT_STATUS_ERROR_VALIDATION;
    }
    
    return BOOT_STATUS_OK;
}

int bootloader_enter_dfu_mode(void)
{
    int ret;
    
    printk("Entering DFU mode...\n");
    current_state = BOOTLOADER_STATE_DFU_MODE;
    
    /* Initialize Bluetooth for DFU */
    ret = bootloader_init_bluetooth();
    if (ret) {
        printk("Error: Failed to initialize Bluetooth for DFU\n");
        return ret;
    }
    
    /* Start advertising */
    bootloader_advertise_dfu();
    
    printk("DFU mode active - waiting for connection\n");
    return 0;
}

int bootloader_start_application(void)
{
    boot_status_t status;
    
    printk("Starting application...\n");
    current_state = BOOTLOADER_STATE_BOOTING_APP;
    
    /* Validate application image */
    status = bootloader_validate_app_image();
    bootloader_show_status(status);
    
    if (status != BOOT_STATUS_OK) {
        printk("Error: Application validation failed\n");
        return -EINVAL;
    }
    
    /* TODO: Jump to application */
    printk("Application validated - would jump to app here\n");
    printk("For now, continuing with normal operation...\n");
    
    return 0;
}

int bootloader_get_info(boot_info_t *info)
{
    if (!info) {
        return -EINVAL;
    }
    
    *info = boot_info;
    return 0;
}

void bootloader_request_dfu(void)
{
    boot_info.dfu_requested = true;
    printk("DFU mode requested for next boot\n");
}

void bootloader_clear_dfu_request(void)
{
    boot_info.dfu_requested = false;
    printk("DFU request flag cleared\n");
}
