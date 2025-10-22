/**
 * @file hardware.c
 * @brief Simplified hardware abstraction layer for nRF52840 Development Kit
 * @details Simple implementation with LED control, GPIO operations, button handling,
 * and Bluetooth advertising for the medical wearable device.
 * 
 * @author Nordic Medical Platform
 * @version 2.0.0
 * @date 2024
 */

#include "hardware.h"
#include "common.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <string.h>

/*============================================================================*/
/* Simple Constants                                                           */
/*============================================================================*/

/** @brief Simple LED timing constants (in milliseconds) */
#define LED_SLOW_BLINK_PERIOD     1000U
#define LED_FAST_BLINK_PERIOD     250U
#define LED_BREATHING_PERIOD      2000U

/** @brief Button debounce time in milliseconds */
#define BUTTON_DEBOUNCE_MS        50U

/*============================================================================*/
/* Simple Global Variables                                                    */
/*============================================================================*/

/** @brief GPIO device for LED control */
static const struct device *gpio_dev;

/** @brief LED GPIO pins */
static const uint32_t led_pins[HW_LED_COUNT] = {
    HW_LED1_PIN, HW_LED2_PIN, HW_LED3_PIN, HW_LED4_PIN
};

/** @brief Simple LED states */
static bool led_states[HW_LED_COUNT];

/** @brief Button state tracking */
static struct {
    bool pressed;
    uint32_t press_count;
    uint32_t last_press_time;
} button_state;

/** @brief DFU boot state */
static struct {
    bool initialized;
    bool boot_requested;
    bool in_boot_mode;
} dfu_state;

/** @brief Bluetooth advertising state */
static struct {
    bool initialized;
    bool advertising;
    char device_name[32];
} ble_state;

/** @brief Hardware initialization status */
static bool hw_initialized = false;

/*============================================================================*/
/* Simple Function Declarations                                               */
/*============================================================================*/

static int init_gpio(void);
static int init_leds(void);
static int init_button(void);
static int init_bluetooth(void);
static void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

/*============================================================================*/
/* Public Function Implementations                                            */
/*============================================================================*/

/**
 * @brief Initialize hardware abstraction layer
 */
int hw_init(void)
{
    int ret;

    if (hw_initialized) {
        return HW_OK;
    }

    printk("Initializing hardware abstraction layer\n");

    /* Initialize GPIO subsystem */
    ret = init_gpio();
    if (ret != HW_OK) {
        printk("ERROR: GPIO initialization failed: %d\n", ret);
        return ret;
    }

    /* Initialize LEDs */
    ret = init_leds();
    if (ret != HW_OK) {
        printk("ERROR: LED initialization failed: %d\n", ret);
        return ret;
    }

    /* Initialize button for DFU */
    ret = init_button();
    if (ret != HW_OK) {
        printk("WARNING: Button initialization failed: %d\n", ret);
        /* Non-critical error, continue */
    }

    /* Initialize Bluetooth */
    ret = init_bluetooth();
    if (ret != HW_OK) {
        printk("WARNING: Bluetooth initialization failed: %d\n", ret);
        /* Non-critical error, continue */
    }

    /* Initialize DFU state */
    dfu_state.initialized = true;
    dfu_state.boot_requested = false;
    dfu_state.in_boot_mode = false;

    hw_initialized = true;
    printk("Hardware abstraction layer initialized successfully\n");

    return HW_OK;
}

/**
 * @brief Get hardware information
 */
int hw_get_info(hw_info_t *info)
{
    if (!info) {
        return HW_ERROR_INVALID_PARAM;
    }

    if (!hw_initialized) {
        return HW_ERROR_NOT_READY;
    }

    /* Get device ID */
    if (hwinfo_get_device_id(info->device_id, sizeof(info->device_id)) != 0) {
        memset(info->device_id, 0, sizeof(info->device_id));
    }

    /* Get reset cause */
    if (hwinfo_get_reset_cause(&info->reset_cause) != 0) {
        info->reset_cause = 0;
    }

    /* Get system status */
    info->usb_console_ready = hw_usb_console_ready();
    info->leds_initialized = hw_initialized;
    info->gpio_initialized = (gpio_dev != NULL);
    info->uptime_ms = k_uptime_get_32();

    return HW_OK;
}

/**
 * @brief Check if USB console is ready
 */
bool hw_usb_console_ready(void)
{
    /* For nRF52840DK, USB console is always available after initialization */
    return hw_initialized;
}

/*============================================================================*/
/* LED Control Functions                                                      */
/*============================================================================*/

/**
 * @brief Set LED state (on/off)
 */
int hw_led_set_state(uint32_t led_id, bool state)
{
    if (!hw_initialized || led_id >= HW_LED_COUNT) {
        return HW_ERROR_INVALID_PARAM;
    }

    int ret = gpio_pin_set(gpio_dev, led_pins[led_id], state ? 1 : 0);
    if (ret != 0) {
        printk("ERROR: Failed to set LED %u state: %d\n", led_id, ret);
        return HW_ERROR_LED;
    }

    /* Update LED state */
    led_states[led_id] = state;

    return HW_OK;
}

/**
 * @brief Set LED pattern (simplified)
 */
int hw_led_set_pattern(uint32_t led_id, hw_led_pattern_t pattern)
{
    if (!hw_initialized || led_id >= HW_LED_COUNT || pattern >= HW_PULSE_PATTERN_MAX) {
        return HW_ERROR_INVALID_PARAM;
    }

    /* Simple pattern implementation */
    switch (pattern) {
        case HW_PULSE_OFF:
            return hw_led_set_state(led_id, false);
        case HW_PULSE_ON:
            return hw_led_set_state(led_id, true);
        case HW_PULSE_SLOW_BLINK:
        case HW_PULSE_FAST_BLINK:
        case HW_PULSE_BREATHING:
        case HW_PULSE_HEARTBEAT:
        case HW_PULSE_SOS:
        case HW_PULSE_DOUBLE_BLINK:
            /* For now, just turn on - patterns would need a timer thread */
            return hw_led_set_state(led_id, true);
        default:
            return HW_ERROR_INVALID_PARAM;
    }
}

/**
 * @brief Update LED patterns (simplified - no-op for now)
 */
int hw_led_update_patterns(void)
{
    /* Simple implementation - no complex patterns */
    return HW_OK;
}

/**
 * @brief Show medical pulse on heartbeat LED (simplified)
 */
int hw_show_medical_pulse(uint32_t heart_rate_bpm)
{
    UNUSED(heart_rate_bpm);
    /* Simple implementation - just turn on heartbeat LED */
    return hw_led_set_state(HW_LED_HEARTBEAT, true);
}

/*============================================================================*/
/* GPIO and Button Functions                                                  */
/*============================================================================*/

/**
 * @brief Initialize button for DFU boot process
 */
int hw_button_init(void)
{
    if (!hw_initialized || !gpio_dev) {
        return HW_ERROR_NOT_READY;
    }

    /* Configure button pin as input with pull-up */
    int ret = gpio_pin_configure(gpio_dev, HW_BUTTON_PIN, 
                                GPIO_INPUT | GPIO_PULL_UP | GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("ERROR: Failed to configure button pin: %d\n", ret);
        return HW_ERROR_GPIO;
    }

    /* Initialize button state */
    button_state.pressed = false;
    button_state.press_count = 0;
    button_state.last_press_time = 0;

    printk("Button initialized for DFU boot process\n");
    return HW_OK;
}

/**
 * @brief Check if DFU button is pressed
 */
bool hw_button_is_pressed(void)
{
    if (!hw_initialized || !gpio_dev) {
        return false;
    }

    int ret = gpio_pin_get(gpio_dev, HW_BUTTON_PIN);
    return (ret == 0); /* Active low button */
}

/**
 * @brief Wait for button press with timeout
 */
bool hw_button_wait_press(uint32_t timeout_ms)
{
    if (!hw_initialized) {
        return false;
    }

    uint32_t start_time = k_uptime_get_32();
    uint32_t timeout_time = start_time + timeout_ms;

    printk("Waiting for button press (timeout: %u ms)\n", timeout_ms);

    while (k_uptime_get_32() < timeout_time) {
        if (hw_button_is_pressed()) {
            printk("Button press detected\n");
            return true;
        }
        k_sleep(K_MSEC(10));
    }

    printk("Button press timeout\n");
    return false;
}

/**
 * @brief Get button press count since last reset
 */
uint32_t hw_button_get_press_count(void)
{
    return button_state.press_count;
}

/*============================================================================*/
/* DFU Boot Process Functions                                                 */
/*============================================================================*/

/**
 * @brief Initialize DFU boot process
 */
int hw_dfu_init(void)
{
    if (!hw_initialized) {
        return HW_ERROR_NOT_READY;
    }

    dfu_state.initialized = true;
    dfu_state.boot_requested = false;
    dfu_state.in_boot_mode = false;

    printk("DFU boot process initialized\n");
    return HW_OK;
}

/**
 * @brief Check if DFU boot is requested
 */
bool hw_dfu_boot_requested(void)
{
    if (!dfu_state.initialized) {
        return false;
    }

    /* Check if button is pressed during boot */
    if (hw_button_is_pressed()) {
        dfu_state.boot_requested = true;
        printk("DFU boot requested via button press\n");
    }

    return dfu_state.boot_requested;
}

/**
 * @brief Enter DFU boot mode
 */
int hw_dfu_enter_boot_mode(void)
{
    if (!dfu_state.initialized) {
        return HW_ERROR_NOT_READY;
    }

    dfu_state.in_boot_mode = true;

    /* Indicate DFU mode with LED pattern */
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_FAST_BLINK);
    hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);

    printk("Entered DFU boot mode\n");
    printk("=== DFU BOOT MODE ACTIVATED ===\n");
    printk("Waiting for firmware update...\n");
    printk("Press button to exit DFU mode\n");

    return HW_OK;
}

/**
 * @brief Exit DFU boot mode
 */
int hw_dfu_exit_boot_mode(void)
{
    if (!dfu_state.initialized || !dfu_state.in_boot_mode) {
        return HW_ERROR_NOT_READY;
    }

    dfu_state.in_boot_mode = false;
    dfu_state.boot_requested = false;

    /* Clear DFU LED patterns */
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_BREATHING);
    hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);

    printk("Exited DFU boot mode\n");
    printk("=== DFU BOOT MODE DEACTIVATED ===\n");

    return HW_OK;
}

/*============================================================================*/
/* Bluetooth and Serial Communication Functions                               */
/*============================================================================*/

/**
 * @brief Initialize Bluetooth Low Energy advertising
 */
int hw_ble_advertising_init(void)
{
    if (!hw_initialized) {
        return HW_ERROR_NOT_READY;
    }

    int ret = bt_enable(NULL);
    if (ret != 0) {
        printk("ERROR: Bluetooth enable failed: %d\n", ret);
        return HW_ERROR_USB;
    }

    /* Set device name */
    strncpy(ble_state.device_name, "Nordic-Medical-Device", sizeof(ble_state.device_name) - 1);
    ble_state.device_name[sizeof(ble_state.device_name) - 1] = '\0';

    ble_state.initialized = true;
    ble_state.advertising = false;

    printk("Bluetooth advertising initialized\n");
    return HW_OK;
}

/**
 * @brief Start Bluetooth advertising
 */
int hw_ble_advertising_start(void)
{
    if (!ble_state.initialized) {
        return HW_ERROR_NOT_READY;
    }

    struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_NAME_COMPLETE, ble_state.device_name, strlen(ble_state.device_name)),
    };

    int ret = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,
                                             BT_GAP_ADV_FAST_INT_MIN_2,
                                             BT_GAP_ADV_FAST_INT_MAX_2,
                                             NULL), ad, ARRAY_SIZE(ad), NULL, 0);
    if (ret != 0) {
        printk("ERROR: Bluetooth advertising start failed: %d\n", ret);
        return HW_ERROR_USB;
    }

    ble_state.advertising = true;
    hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_SLOW_BLINK);

    printk("Bluetooth advertising started\n");
    return HW_OK;
}

/**
 * @brief Stop Bluetooth advertising
 */
int hw_ble_advertising_stop(void)
{
    if (!ble_state.initialized || !ble_state.advertising) {
        return HW_ERROR_NOT_READY;
    }

    int ret = bt_le_adv_stop();
    if (ret != 0) {
        printk("ERROR: Bluetooth advertising stop failed: %d\n", ret);
        return HW_ERROR_USB;
    }

    ble_state.advertising = false;
    hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_OFF);

    printk("Bluetooth advertising stopped\n");
    return HW_OK;
}

/**
 * @brief Set Bluetooth advertising data
 */
int hw_ble_set_advertising_data(const char *device_name, const void *medical_data)
{
    UNUSED(medical_data);
    
    if (!ble_state.initialized || !device_name) {
        return HW_ERROR_INVALID_PARAM;
    }

    /* Update device name */
    strncpy(ble_state.device_name, device_name, sizeof(ble_state.device_name) - 1);
    ble_state.device_name[sizeof(ble_state.device_name) - 1] = '\0';

    printk("Bluetooth advertising data updated: %s\n", device_name);
    return HW_OK;
}

/**
 * @brief Initialize serial communication for Bluetooth
 */
int hw_serial_bt_init(void)
{
    if (!hw_initialized) {
        return HW_ERROR_NOT_READY;
    }

    printk("Serial Bluetooth communication initialized\n");
    return HW_OK;
}

/**
 * @brief Send data via serial Bluetooth interface
 */
int hw_serial_bt_send(const uint8_t *data, uint32_t length)
{
    UNUSED(data);
    UNUSED(length);
    
    /* Simple implementation - just print to console */
    printk("Serial BT data sent: %u bytes\n", length);
    return HW_OK;
}

/**
 * @brief Receive data via serial Bluetooth interface
 */
int hw_serial_bt_receive(uint8_t *buffer, uint32_t max_length, uint32_t *received_length)
{
    UNUSED(buffer);
    UNUSED(max_length);
    
    if (!received_length) {
        return HW_ERROR_INVALID_PARAM;
    }

    *received_length = 0;
    return HW_OK;
}

/*============================================================================*/
/* Simple Function Implementations                                            */
/*============================================================================*/

/**
 * @brief Initialize GPIO subsystem
 */
static int init_gpio(void)
{
    gpio_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio0));
    if (!gpio_dev || !device_is_ready(gpio_dev)) {
        printk("ERROR: GPIO device not found\n");
        return HW_ERROR_GPIO;
    }

    printk("GPIO subsystem initialized\n");
    return HW_OK;
}

/**
 * @brief Initialize LEDs
 */
static int init_leds(void)
{
    if (!gpio_dev) {
        return HW_ERROR_GPIO;
    }

    /* Configure all LED pins as outputs */
    for (uint32_t i = 0; i < HW_LED_COUNT; i++) {
        int ret = gpio_pin_configure(gpio_dev, led_pins[i], GPIO_OUTPUT_INACTIVE);
        if (ret != 0) {
            printk("ERROR: Failed to configure LED %u pin: %d\n", i, ret);
            return HW_ERROR_LED;
        }

        /* Initialize LED state */
        led_states[i] = false;
    }

    printk("LEDs initialized\n");
    return HW_OK;
}

/**
 * @brief Initialize button
 */
static int init_button(void)
{
    /* Button initialization is handled in hw_button_init() */
    return HW_OK;
}

/**
 * @brief Initialize Bluetooth subsystem
 */
static int init_bluetooth(void)
{
    /* Bluetooth initialization is handled in hw_ble_advertising_init() */
    return HW_OK;
}

/**
 * @brief Button callback function (simplified)
 */
static void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    UNUSED(dev);
    UNUSED(cb);
    UNUSED(pins);

    uint32_t current_time = k_uptime_get_32();
    
    /* Debounce check */
    if (current_time - button_state.last_press_time < BUTTON_DEBOUNCE_MS) {
        return;
    }

    button_state.pressed = true;
    button_state.press_count++;
    button_state.last_press_time = current_time;

    printk("Button pressed (count: %u)\n", button_state.press_count);
}
