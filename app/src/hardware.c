/**
 * @file hardware.c
 * @brief Hardware abstraction layer implementation for nRF52840 Development Kit
 * @details Implements LED control, GPIO operations, USB console management,
 * DFU boot process, and Bluetooth advertising for the medical wearable device.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This implementation provides a complete hardware abstraction layer
 * for medical device development on the nRF52840 platform.
 */

#include "hardware.h"
#include "common.h"
#include "diagnostics.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <string.h>
#include <math.h>

/*============================================================================*/
/* Private Constants and Macros                                               */
/*============================================================================*/

/** @brief LED pattern timing constants (in milliseconds) */
#define LED_PATTERN_SLOW_BLINK_PERIOD     1000U
#define LED_PATTERN_FAST_BLINK_PERIOD     250U
#define LED_PATTERN_BREATHING_PERIOD       2000U
#define LED_PATTERN_HEARTBEAT_PERIOD       600U
#define LED_PATTERN_SOS_PERIOD             100U
#define LED_PATTERN_DOUBLE_BLINK_PERIOD    200U

/** @brief Button debounce time in milliseconds */
#define BUTTON_DEBOUNCE_MS                 50U

/** @brief DFU boot timeout in milliseconds */
#define DFU_BOOT_TIMEOUT_MS                10000U

/** @brief Bluetooth advertising interval in milliseconds */
#define BLE_ADV_INTERVAL_MS                100U

/*============================================================================*/
/* Private Global Variables                                                   */
/*============================================================================*/

/** @brief GPIO device for LED control */
static const struct device *gpio_dev;

/** @brief UART device for serial Bluetooth communication */
static const struct device *uart_bt_dev;

/** @brief LED GPIO pins */
static const uint32_t led_pins[HW_LED_COUNT] = {
    HW_LED1_PIN, HW_LED2_PIN, HW_LED3_PIN, HW_LED4_PIN
};

/** @brief LED states for pattern management */
static hw_led_state_t led_states[HW_LED_COUNT];

/** @brief Button state tracking */
static struct {
    bool pressed;
    uint32_t press_count;
    uint32_t last_press_time;
    struct gpio_callback callback;
} button_state;

/** @brief DFU boot state */
static struct {
    bool initialized;
    bool boot_requested;
    bool in_boot_mode;
    uint32_t boot_start_time;
} dfu_state;

/** @brief Bluetooth advertising state */
static struct {
    bool initialized;
    bool advertising;
    char device_name[32];
    uint8_t advertising_data[31];
    uint8_t advertising_data_len;
} ble_state;

/** @brief Hardware initialization status */
static bool hw_initialized = false;

/*============================================================================*/
/* Private Function Declarations                                              */
/*============================================================================*/

static int init_gpio(void);
static int init_leds(void);
static int init_button(void);
static int init_uart_bt(void);
static int init_bluetooth(void);
static void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static void update_led_pattern(uint32_t led_id);
static uint32_t calculate_pattern_state(hw_led_pattern_t pattern, uint32_t elapsed_ms);
static int send_uart_data(const uint8_t *data, uint32_t length);

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

    DIAG_INFO(DIAG_CAT_HARDWARE, "Initializing hardware abstraction layer");

    /* Initialize GPIO subsystem */
    ret = init_gpio();
    if (ret != HW_OK) {
        DIAG_ERROR(DIAG_CAT_HARDWARE, "GPIO initialization failed: %d", ret);
        return ret;
    }

    /* Initialize LEDs */
    ret = init_leds();
    if (ret != HW_OK) {
        DIAG_ERROR(DIAG_CAT_HARDWARE, "LED initialization failed: %d", ret);
        return ret;
    }

    /* Initialize button for DFU */
    ret = init_button();
    if (ret != HW_OK) {
        DIAG_WARNING(DIAG_CAT_HARDWARE, "Button initialization failed: %d", ret);
        /* Non-critical error, continue */
    }

    /* Initialize UART for Bluetooth */
    ret = init_uart_bt();
    if (ret != HW_OK) {
        DIAG_WARNING(DIAG_CAT_HARDWARE, "UART Bluetooth initialization failed: %d", ret);
        /* Non-critical error, continue */
    }

    /* Initialize Bluetooth */
    ret = init_bluetooth();
    if (ret != HW_OK) {
        DIAG_WARNING(DIAG_CAT_HARDWARE, "Bluetooth initialization failed: %d", ret);
        /* Non-critical error, continue */
    }

    /* Initialize DFU state */
    dfu_state.initialized = true;
    dfu_state.boot_requested = false;
    dfu_state.in_boot_mode = false;

    hw_initialized = true;
    DIAG_INFO(DIAG_CAT_HARDWARE, "Hardware abstraction layer initialized successfully");

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
        DIAG_ERROR(DIAG_CAT_HARDWARE, "Failed to set LED %u state: %d", led_id, ret);
        return HW_ERROR_LED;
    }

    /* Update LED state */
    led_states[led_id].state = state;
    led_states[led_id].pattern = state ? HW_PULSE_ON : HW_PULSE_OFF;

    return HW_OK;
}

/**
 * @brief Set LED pattern
 */
int hw_led_set_pattern(uint32_t led_id, hw_led_pattern_t pattern)
{
    if (!hw_initialized || led_id >= HW_LED_COUNT || pattern >= HW_PULSE_PATTERN_MAX) {
        return HW_ERROR_INVALID_PARAM;
    }

    led_states[led_id].pattern = pattern;
    led_states[led_id].pattern_start_ms = k_uptime_get_32();
    led_states[led_id].cycle_count = 0;

    /* For static patterns, set immediately */
    if (pattern == HW_PULSE_OFF) {
        return hw_led_set_state(led_id, false);
    } else if (pattern == HW_PULSE_ON) {
        return hw_led_set_state(led_id, true);
    }

    return HW_OK;
}

/**
 * @brief Update LED patterns
 */
int hw_led_update_patterns(void)
{
    if (!hw_initialized) {
        return HW_ERROR_NOT_READY;
    }

    for (uint32_t i = 0; i < HW_LED_COUNT; i++) {
        update_led_pattern(i);
    }

    return HW_OK;
}

/**
 * @brief Show medical pulse on heartbeat LED
 */
int hw_show_medical_pulse(uint32_t heart_rate_bpm)
{
    if (!hw_initialized) {
        return HW_ERROR_NOT_READY;
    }

    /* Calculate heartbeat period based on BPM */
    uint32_t heartbeat_period_ms = 60000U / heart_rate_bpm;
    
    /* Update LED pattern timing */
    led_states[HW_LED_HEARTBEAT].pattern = HW_PULSE_HEARTBEAT;
    led_states[HW_LED_HEARTBEAT].pattern_start_ms = k_uptime_get_32();

    return HW_OK;
}

/**
 * @brief Test LED patterns
 */
int hw_led_test_patterns(hw_led_pattern_t pattern)
{
    if (!hw_initialized) {
        return HW_ERROR_NOT_READY;
    }

    if (pattern == HW_PULSE_PATTERN_MAX) {
        /* Test all patterns */
        for (hw_led_pattern_t p = HW_PULSE_OFF; p < HW_PULSE_PATTERN_MAX; p++) {
            DIAG_INFO(DIAG_CAT_HARDWARE, "Testing LED pattern: %d", p);
            
            /* Test on all LEDs */
            for (uint32_t led = 0; led < HW_LED_COUNT; led++) {
                hw_led_set_pattern(led, p);
                k_sleep(K_MSEC(1000));
            }
            
            /* Turn off all LEDs */
            for (uint32_t led = 0; led < HW_LED_COUNT; led++) {
                hw_led_set_pattern(led, HW_PULSE_OFF);
            }
            k_sleep(K_MSEC(500));
        }
    } else {
        /* Test specific pattern */
        for (uint32_t led = 0; led < HW_LED_COUNT; led++) {
            hw_led_set_pattern(led, pattern);
        }
        k_sleep(K_MSEC(3000));
        
        /* Turn off all LEDs */
        for (uint32_t led = 0; led < HW_LED_COUNT; led++) {
            hw_led_set_pattern(led, HW_PULSE_OFF);
        }
    }

    return HW_OK;
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
        DIAG_ERROR(DIAG_CAT_HARDWARE, "Failed to configure button pin: %d", ret);
        return HW_ERROR_GPIO;
    }

    /* Initialize button state */
    button_state.pressed = false;
    button_state.press_count = 0;
    button_state.last_press_time = 0;

    /* Set up button callback */
    gpio_init_callback(&button_state.callback, button_callback, BIT(HW_BUTTON_PIN));
    ret = gpio_add_callback(gpio_dev, &button_state.callback);
    if (ret != 0) {
        DIAG_ERROR(DIAG_CAT_HARDWARE, "Failed to add button callback: %d", ret);
        return HW_ERROR_GPIO;
    }

    /* Enable button interrupt */
    ret = gpio_pin_interrupt_configure(gpio_dev, HW_BUTTON_PIN, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        DIAG_ERROR(DIAG_CAT_HARDWARE, "Failed to enable button interrupt: %d", ret);
        return HW_ERROR_GPIO;
    }

    DIAG_INFO(DIAG_CAT_HARDWARE, "Button initialized for DFU boot process");
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

    DIAG_INFO(DIAG_CAT_HARDWARE, "Waiting for button press (timeout: %u ms)", timeout_ms);

    while (k_uptime_get_32() < timeout_time) {
        if (hw_button_is_pressed()) {
            DIAG_INFO(DIAG_CAT_HARDWARE, "Button press detected");
            return true;
        }
        k_sleep(K_MSEC(10));
    }

    DIAG_INFO(DIAG_CAT_HARDWARE, "Button press timeout");
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

    DIAG_INFO(DIAG_CAT_HARDWARE, "DFU boot process initialized");
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
        DIAG_INFO(DIAG_CAT_HARDWARE, "DFU boot requested via button press");
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
    dfu_state.boot_start_time = k_uptime_get_32();

    /* Indicate DFU mode with LED pattern */
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_FAST_BLINK);
    hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);

    DIAG_INFO(DIAG_CAT_HARDWARE, "Entered DFU boot mode");
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

    DIAG_INFO(DIAG_CAT_HARDWARE, "Exited DFU boot mode");
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
        DIAG_ERROR(DIAG_CAT_HARDWARE, "Bluetooth enable failed: %d", ret);
        return HW_ERROR_USB;
    }

    /* Set device name */
    strncpy(ble_state.device_name, "NISC-Medical-Device", sizeof(ble_state.device_name) - 1);
    ble_state.device_name[sizeof(ble_state.device_name) - 1] = '\0';

    /* Set advertising data */
    ble_state.advertising_data_len = 0;
    
    /* Add device name to advertising data */
    ble_state.advertising_data[ble_state.advertising_data_len++] = strlen(ble_state.device_name) + 1;
    ble_state.advertising_data[ble_state.advertising_data_len++] = BT_DATA_NAME_COMPLETE;
    memcpy(&ble_state.advertising_data[ble_state.advertising_data_len], 
           ble_state.device_name, strlen(ble_state.device_name));
    ble_state.advertising_data_len += strlen(ble_state.device_name);

    ble_state.initialized = true;
    ble_state.advertising = false;

    DIAG_INFO(DIAG_CAT_HARDWARE, "Bluetooth advertising initialized");
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
        DIAG_ERROR(DIAG_CAT_HARDWARE, "Bluetooth advertising start failed: %d", ret);
        return HW_ERROR_USB;
    }

    ble_state.advertising = true;
    hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_SLOW_BLINK);

    DIAG_INFO(DIAG_CAT_HARDWARE, "Bluetooth advertising started");
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
        DIAG_ERROR(DIAG_CAT_HARDWARE, "Bluetooth advertising stop failed: %d", ret);
        return HW_ERROR_USB;
    }

    ble_state.advertising = false;
    hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_OFF);

    DIAG_INFO(DIAG_CAT_HARDWARE, "Bluetooth advertising stopped");
    return HW_OK;
}

/**
 * @brief Set Bluetooth advertising data
 */
int hw_ble_set_advertising_data(const char *device_name, const void *medical_data)
{
    if (!ble_state.initialized || !device_name) {
        return HW_ERROR_INVALID_PARAM;
    }

    /* Update device name */
    strncpy(ble_state.device_name, device_name, sizeof(ble_state.device_name) - 1);
    ble_state.device_name[sizeof(ble_state.device_name) - 1] = '\0';

    DIAG_INFO(DIAG_CAT_HARDWARE, "Bluetooth advertising data updated: %s", device_name);
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

    /* UART is already initialized in init_uart_bt() */
    if (!uart_bt_dev) {
        DIAG_WARNING(DIAG_CAT_HARDWARE, "UART Bluetooth device not available");
        return HW_ERROR_USB;
    }

    DIAG_INFO(DIAG_CAT_HARDWARE, "Serial Bluetooth communication initialized");
    return HW_OK;
}

/**
 * @brief Send data via serial Bluetooth interface
 */
int hw_serial_bt_send(const uint8_t *data, uint32_t length)
{
    if (!data || length == 0) {
        return HW_ERROR_INVALID_PARAM;
    }

    if (!uart_bt_dev) {
        return HW_ERROR_NOT_READY;
    }

    return send_uart_data(data, length);
}

/**
 * @brief Receive data via serial Bluetooth interface
 */
int hw_serial_bt_receive(uint8_t *buffer, uint32_t max_length, uint32_t *received_length)
{
    if (!buffer || !received_length || max_length == 0) {
        return HW_ERROR_INVALID_PARAM;
    }

    if (!uart_bt_dev) {
        return HW_ERROR_NOT_READY;
    }

    *received_length = 0;
    
    /* Simple implementation - in real scenario, this would use UART interrupt */
    for (uint32_t i = 0; i < max_length; i++) {
        uint8_t byte;
        int ret = uart_poll_in(uart_bt_dev, &byte);
        if (ret == 0) {
            buffer[i] = byte;
            (*received_length)++;
        } else {
            break;
        }
    }

    return HW_OK;
}

/*============================================================================*/
/* Private Function Implementations                                           */
/*============================================================================*/

/**
 * @brief Initialize GPIO subsystem
 */
static int init_gpio(void)
{
    gpio_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio0));
    if (!gpio_dev || !device_is_ready(gpio_dev)) {
        DIAG_ERROR(DIAG_CAT_HARDWARE, "GPIO device not found");
        return HW_ERROR_GPIO;
    }

    DIAG_INFO(DIAG_CAT_HARDWARE, "GPIO subsystem initialized");
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
            DIAG_ERROR(DIAG_CAT_HARDWARE, "Failed to configure LED %u pin: %d", i, ret);
            return HW_ERROR_LED;
        }

        /* Initialize LED state */
        led_states[i].pattern = HW_PULSE_OFF;
        led_states[i].pattern_start_ms = 0;
        led_states[i].cycle_count = 0;
        led_states[i].state = false;
    }

    DIAG_INFO(DIAG_CAT_HARDWARE, "LEDs initialized");
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
 * @brief Initialize UART for Bluetooth communication
 */
static int init_uart_bt(void)
{
    /* Try to get UART device - this might not be available on all boards */
    uart_bt_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(uart1));
    if (!uart_bt_dev || !device_is_ready(uart_bt_dev)) {
        DIAG_WARNING(DIAG_CAT_HARDWARE, "UART Bluetooth device not found");
        return HW_ERROR_USB;
    }

    DIAG_INFO(DIAG_CAT_HARDWARE, "UART Bluetooth initialized");
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
 * @brief Button callback function
 */
static void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    uint32_t current_time = k_uptime_get_32();
    
    /* Debounce check */
    if (current_time - button_state.last_press_time < BUTTON_DEBOUNCE_MS) {
        return;
    }

    button_state.pressed = true;
    button_state.press_count++;
    button_state.last_press_time = current_time;

    DIAG_INFO(DIAG_CAT_HARDWARE, "Button pressed (count: %u)", button_state.press_count);
}

/**
 * @brief Update LED pattern
 */
static void update_led_pattern(uint32_t led_id)
{
    if (led_id >= HW_LED_COUNT) {
        return;
    }

    hw_led_pattern_t pattern = led_states[led_id].pattern;
    
    /* Handle static patterns */
    if (pattern == HW_PULSE_OFF || pattern == HW_PULSE_ON) {
        return; /* Already handled in hw_led_set_pattern */
    }

    uint32_t elapsed_ms = k_uptime_get_32() - led_states[led_id].pattern_start_ms;
    bool new_state = calculate_pattern_state(pattern, elapsed_ms);

    /* Update LED if state changed */
    if (new_state != led_states[led_id].state) {
        gpio_pin_set(gpio_dev, led_pins[led_id], new_state ? 1 : 0);
        led_states[led_id].state = new_state;
    }
}

/**
 * @brief Calculate pattern state based on elapsed time
 */
static uint32_t calculate_pattern_state(hw_led_pattern_t pattern, uint32_t elapsed_ms)
{
    switch (pattern) {
        case HW_PULSE_SLOW_BLINK:
            return (elapsed_ms / LED_PATTERN_SLOW_BLINK_PERIOD) % 2;

        case HW_PULSE_FAST_BLINK:
            return (elapsed_ms / LED_PATTERN_FAST_BLINK_PERIOD) % 2;

        case HW_PULSE_BREATHING:
            {
                uint32_t cycle = elapsed_ms % LED_PATTERN_BREATHING_PERIOD;
                uint32_t half_period = LED_PATTERN_BREATHING_PERIOD / 2;
                return (cycle < half_period) ? 1 : 0;
            }

        case HW_PULSE_HEARTBEAT:
            {
                uint32_t cycle = elapsed_ms % LED_PATTERN_HEARTBEAT_PERIOD;
                return (cycle < 100) ? 1 : 0; /* Short pulse */
            }

        case HW_PULSE_SOS:
            {
                uint32_t cycle = elapsed_ms % (LED_PATTERN_SOS_PERIOD * 21); /* SOS pattern */
                uint32_t pos = cycle / LED_PATTERN_SOS_PERIOD;
                /* SOS pattern: ... --- ... */
                return ((pos >= 0 && pos < 3) || (pos >= 4 && pos < 7) || 
                        (pos >= 8 && pos < 11) || (pos >= 12 && pos < 15) ||
                        (pos >= 16 && pos < 19) || (pos >= 20 && pos < 21)) ? 1 : 0;
            }

        case HW_PULSE_DOUBLE_BLINK:
            {
                uint32_t cycle = elapsed_ms % LED_PATTERN_DOUBLE_BLINK_PERIOD;
                return (cycle < 50 || (cycle >= 100 && cycle < 150)) ? 1 : 0;
            }

        default:
            return 0;
    }
}

/**
 * @brief Send UART data
 */
static int send_uart_data(const uint8_t *data, uint32_t length)
{
    if (!uart_bt_dev || !data || length == 0) {
        return HW_ERROR_INVALID_PARAM;
    }

    for (uint32_t i = 0; i < length; i++) {
        uart_poll_out(uart_bt_dev, data[i]);
    }

    return HW_OK;
}
