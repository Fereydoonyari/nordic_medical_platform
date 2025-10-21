/**
 * @file hardware.c
 * @brief Hardware abstraction layer implementation for nRF52840 Development Kit
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
 
 /*============================================================================*/
 /* Private Global Variables                                                   */
 /*============================================================================*/
 
 /** @brief GPIO device for LED control */
 static const struct device *gpio_dev;
 
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
 /* Private Function Declarations                                              */
 /*============================================================================*/
 
 static int init_gpio(void);
 static int init_leds(void);
 static int init_button(void);
 static void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
 static void update_led_pattern(uint32_t led_id);
 static uint32_t calculate_pattern_state(hw_led_pattern_t pattern, uint32_t elapsed_ms);
 
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
 
     printk("Initializing hardware abstraction layer...\n");
 
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
 
     /* Initialize button */
     ret = init_button();
     if (ret != HW_OK) {
         printk("WARNING: Button initialization failed: %d\n", ret);
         /* Non-critical, continue */
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
     info->usb_console_ready = false; /* Using UART console */
     info->leds_initialized = hw_initialized;
     info->gpio_initialized = (gpio_dev != NULL);
     info->uptime_ms = k_uptime_get_32();
 
     return HW_OK;
 }
 
 /**
  * @brief Check if USB console is ready (always false for UART)
  */
 bool hw_usb_console_ready(void)
 {
     return false; /* Using UART console, not USB */
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
         return HW_ERROR_LED;
     }
 
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
 
     ARG_UNUSED(heart_rate_bpm); /* Avoid unused warning */
     
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
             for (uint32_t led = 0; led < HW_LED_COUNT; led++) {
                 hw_led_set_pattern(led, p);
                 k_sleep(K_MSEC(1000));
             }
             
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
         
         for (uint32_t led = 0; led < HW_LED_COUNT; led++) {
             hw_led_set_pattern(led, HW_PULSE_OFF);
         }
     }
 
     return HW_OK;
 }
 
 /*============================================================================*/
 /* Button Functions                                                           */
 /*============================================================================*/
 
 /**
  * @brief Initialize button with interrupt
  */
 int hw_button_init(void)
 {
     if (!hw_initialized || !gpio_dev) {
         return HW_ERROR_NOT_READY;
     }
 
     gpio_init_callback(&button_state.callback, button_callback, BIT(HW_BUTTON_PIN));
     int ret = gpio_add_callback(gpio_dev, &button_state.callback);
     if (ret != 0) {
         printk("ERROR: Failed to add button callback: %d\n", ret);
         return HW_ERROR_GPIO;
     }
 
     ret = gpio_pin_interrupt_configure(gpio_dev, HW_BUTTON_PIN, GPIO_INT_EDGE_BOTH);
     if (ret != 0) {
         printk("ERROR: Failed to enable button interrupt: %d\n", ret);
         return HW_ERROR_GPIO;
     }
 
     printk("Button interrupt configured\n");
     return HW_OK;
 }
 
 /**
  * @brief Check if button is pressed
  */
 bool hw_button_is_pressed(void)
 {
     if (!hw_initialized || !gpio_dev) {
         return false;
     }
 
     int ret = gpio_pin_get(gpio_dev, HW_BUTTON_PIN);
     return (ret == 0); /* Active low */
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
 
     while (k_uptime_get_32() < timeout_time) {
         if (hw_button_is_pressed()) {
             return true;
         }
         k_sleep(K_MSEC(10));
     }
 
     return false;
 }
 
 /**
  * @brief Get button press count
  */
 uint32_t hw_button_get_press_count(void)
 {
     return button_state.press_count;
 }
 
 /*============================================================================*/
 /* DFU Functions                                                              */
 /*============================================================================*/
 
 int hw_dfu_init(void)
 {
     dfu_state.initialized = true;
     dfu_state.boot_requested = false;
     dfu_state.in_boot_mode = false;
     printk("DFU boot process initialized\n");
     return HW_OK;
 }
 
 bool hw_dfu_boot_requested(void)
 {
     return hw_button_is_pressed();
 }
 
 int hw_dfu_enter_boot_mode(void)
 {
     dfu_state.in_boot_mode = true;
     hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_FAST_BLINK);
     hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
     printk("Entered DFU boot mode\n");
     return HW_OK;
 }
 
 int hw_dfu_exit_boot_mode(void)
 {
     dfu_state.in_boot_mode = false;
     hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_OFF);
     hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
     printk("Exited DFU boot mode\n");
     return HW_OK;
 }
 
 /*============================================================================*/
 /* Bluetooth Functions - FIXED                                                */
 /*============================================================================*/
 
 int hw_ble_advertising_init(void)
 {
     if (!hw_initialized) {
         return HW_ERROR_NOT_READY;
     }
 
     printk("Initializing Bluetooth stack...\n");
     
     int ret = bt_enable(NULL);
     if (ret != 0) {
         printk("ERROR: Bluetooth enable failed: %d\n", ret);
         return HW_ERROR_USB;
     }
 
     strncpy(ble_state.device_name, "NISC-Medical", sizeof(ble_state.device_name) - 1);
     ble_state.device_name[sizeof(ble_state.device_name) - 1] = '\0';
 
     ret = bt_set_name(ble_state.device_name);
     if (ret != 0) {
         printk("WARNING: Failed to set BT name: %d\n", ret);
     }
 
     ble_state.initialized = true;
     ble_state.advertising = false;
 
     printk("Bluetooth stack initialized\n");
     return HW_OK;
 }
 
 int hw_ble_advertising_start(void)
 {
     if (!ble_state.initialized) {
         return HW_ERROR_NOT_READY;
     }
 
     printk("Starting Bluetooth advertising...\n");
 
     /* Prepare static advertising data */
     static uint8_t device_name_data[32];
     size_t name_len = strlen(ble_state.device_name);
     if (name_len > sizeof(device_name_data)) {
         name_len = sizeof(device_name_data);
     }
     memcpy(device_name_data, ble_state.device_name, name_len);
 
     static struct bt_data ad[2];
     static const uint8_t flags_data[] = {BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR};
     
     ad[0].type = BT_DATA_FLAGS;
     ad[0].data_len = sizeof(flags_data);
     ad[0].data = flags_data;
     
     ad[1].type = BT_DATA_NAME_COMPLETE;
     ad[1].data_len = name_len;
     ad[1].data = device_name_data;
 
     static struct bt_data sd[1];
     static const uint8_t service_uuids[] = {
         0x0D, 0x18,  /* Heart Rate Service */
         0x0A, 0x18   /* Device Info Service */
     };
     sd[0].type = BT_DATA_UUID16_ALL;
     sd[0].data_len = sizeof(service_uuids);
     sd[0].data = service_uuids;
 
     struct bt_le_adv_param adv_param = {
         .id = BT_ID_DEFAULT,
         .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
         .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
         .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
         .peer = NULL,
     };
 
     int ret = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
     if (ret != 0) {
         printk("ERROR: Bluetooth advertising start failed: %d\n", ret);
         return HW_ERROR_USB;
     }
 
     ble_state.advertising = true;
     printk("Bluetooth advertising started successfully\n");
     
     return HW_OK;
 }
 
 int hw_ble_advertising_stop(void)
 {
     if (!ble_state.advertising) {
         return HW_ERROR_NOT_READY;
     }
 
     int ret = bt_le_adv_stop();
     if (ret != 0) {
         return HW_ERROR_USB;
     }
 
     ble_state.advertising = false;
     return HW_OK;
 }
 
 int hw_ble_set_advertising_data(const char *device_name, const void *medical_data)
 {
     ARG_UNUSED(medical_data);
     
     if (!ble_state.initialized || !device_name) {
         return HW_ERROR_INVALID_PARAM;
     }
 
     strncpy(ble_state.device_name, device_name, sizeof(ble_state.device_name) - 1);
     ble_state.device_name[sizeof(ble_state.device_name) - 1] = '\0';
 
     return HW_OK;
 }
 
 int hw_serial_bt_init(void)
 {
     return HW_OK;
 }
 
 int hw_serial_bt_send(const uint8_t *data, uint32_t length)
 {
     ARG_UNUSED(data);
     ARG_UNUSED(length);
     return HW_OK;
 }
 
 int hw_serial_bt_receive(uint8_t *buffer, uint32_t max_length, uint32_t *received_length)
 {
     ARG_UNUSED(buffer);
     ARG_UNUSED(max_length);
     
     if (received_length) {
         *received_length = 0;
     }
     return HW_OK;
 }
 
 /*============================================================================*/
 /* Private Functions                                                          */
 /*============================================================================*/
 
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
 
 static int init_leds(void)
 {
     if (!gpio_dev) {
         return HW_ERROR_GPIO;
     }
 
     for (uint32_t i = 0; i < HW_LED_COUNT; i++) {
         int ret = gpio_pin_configure(gpio_dev, led_pins[i], GPIO_OUTPUT_INACTIVE);
         if (ret != 0) {
             printk("ERROR: Failed to configure LED %u: %d\n", i, ret);
             return HW_ERROR_LED;
         }
 
         led_states[i].pattern = HW_PULSE_OFF;
         led_states[i].pattern_start_ms = 0;
         led_states[i].cycle_count = 0;
         led_states[i].state = false;
     }
 
     printk("LEDs initialized\n");
     return HW_OK;
 }
 
 static int init_button(void)
 {
     if (!gpio_dev) {
         return HW_ERROR_GPIO;
     }
 
     int ret = gpio_pin_configure(gpio_dev, HW_BUTTON_PIN, GPIO_INPUT | GPIO_PULL_UP);
     if (ret != 0) {
         printk("ERROR: Failed to configure button: %d\n", ret);
         return HW_ERROR_GPIO;
     }
 
     button_state.pressed = false;
     button_state.press_count = 0;
     button_state.last_press_time = 0;
 
     printk("Button initialized\n");
     return HW_OK;
 }
 
 static void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
 {
     ARG_UNUSED(dev);
     ARG_UNUSED(cb);
     ARG_UNUSED(pins);
 
     uint32_t current_time = k_uptime_get_32();
     
     if (current_time - button_state.last_press_time < BUTTON_DEBOUNCE_MS) {
         return;
     }
 
     int button_value = gpio_pin_get(gpio_dev, HW_BUTTON_PIN);
     
     if (button_value == 0) {
         button_state.pressed = true;
         button_state.press_count++;
         button_state.last_press_time = current_time;
     }
 }
 
 static void update_led_pattern(uint32_t led_id)
 {
     if (led_id >= HW_LED_COUNT) {
         return;
     }
 
     hw_led_pattern_t pattern = led_states[led_id].pattern;
     
     if (pattern == HW_PULSE_OFF || pattern == HW_PULSE_ON) {
         return;
     }
 
     uint32_t elapsed_ms = k_uptime_get_32() - led_states[led_id].pattern_start_ms;
     bool new_state = calculate_pattern_state(pattern, elapsed_ms);
 
     if (new_state != led_states[led_id].state) {
         gpio_pin_set(gpio_dev, led_pins[led_id], new_state ? 1 : 0);
         led_states[led_id].state = new_state;
     }
 }
 
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
                 return (cycle < 100) ? 1 : 0;
             }
 
         case HW_PULSE_SOS:
             {
                 uint32_t cycle = elapsed_ms % (LED_PATTERN_SOS_PERIOD * 21);
                 uint32_t pos = cycle / LED_PATTERN_SOS_PERIOD;
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