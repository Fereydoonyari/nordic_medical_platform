/**
 * @file hardware.c
 * @brief Simplified hardware abstraction with working DFU boot
 */

 #include "hardware.h"
 #include "common.h"
 #include "diagnostics.h"
 #include <zephyr/kernel.h>
 #include <zephyr/device.h>
 #include <zephyr/drivers/gpio.h>
 #include <zephyr/sys/printk.h>
 #include <zephyr/devicetree.h>
 
 /*============================================================================*/
 /* GPIO Device Definitions                                                    */
 /*============================================================================*/
 
 static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
 static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
 static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
 static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
 static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);
 
 /*============================================================================*/
 /* LED Pattern State                                                          */
 /*============================================================================*/
 
 typedef struct {
     hw_led_pattern_t pattern;
     uint32_t start_time;
     bool state;
 } led_state_t;
 
 static led_state_t led_states[HW_LED_COUNT];
 static bool hw_initialized = false;
 
 /*============================================================================*/
 /* DFU Boot State                                                             */
 /*============================================================================*/
 
 static struct {
     bool in_dfu_mode;
     uint32_t button_presses;
 } dfu_state = {0};
 
 /*============================================================================*/
 /* LED Pattern Update                                                         */
 /*============================================================================*/
 
 static void update_led_pattern(uint32_t led_id, const struct gpio_dt_spec *led)
 {
     if (led_id >= HW_LED_COUNT) return;
     
     led_state_t *ls = &led_states[led_id];
     uint32_t elapsed = k_uptime_get_32() - ls->start_time;
     bool new_state = false;
     
     switch (ls->pattern) {
         case HW_PULSE_OFF:
             new_state = false;
             break;
         case HW_PULSE_ON:
             new_state = true;
             break;
         case HW_PULSE_SLOW_BLINK:
             new_state = (elapsed / 500) % 2;
             break;
         case HW_PULSE_FAST_BLINK:
             new_state = (elapsed / 125) % 2;
             break;
         case HW_PULSE_BREATHING:
             new_state = (elapsed / 1000) % 2;
             break;
         case HW_PULSE_HEARTBEAT:
             new_state = (elapsed % 600) < 100;
             break;
         default:
             new_state = false;
     }
     
     if (new_state != ls->state) {
         gpio_pin_set_dt(led, new_state);
         ls->state = new_state;
     }
 }
 
 /*============================================================================*/
 /* Public Functions                                                           */
 /*============================================================================*/
 
 int hw_init(void)
 {
     if (hw_initialized) return HW_OK;
     
     printk("Initializing hardware...\n");
     
     /* Initialize button */
     if (!gpio_is_ready_dt(&button)) {
         printk("Button GPIO not ready\n");
         return HW_ERROR_GPIO;
     }
     
     gpio_pin_configure_dt(&button, GPIO_INPUT);
     
     /* Initialize LEDs */
     const struct gpio_dt_spec *leds[] = {&led0, &led1, &led2, &led3};
     
     for (int i = 0; i < HW_LED_COUNT; i++) {
         if (!gpio_is_ready_dt(leds[i])) {
             printk("LED%d GPIO not ready\n", i);
             return HW_ERROR_LED;
         }
         gpio_pin_configure_dt(leds[i], GPIO_OUTPUT_INACTIVE);
         led_states[i].pattern = HW_PULSE_OFF;
         led_states[i].start_time = k_uptime_get_32();
         led_states[i].state = false;
     }
     
     hw_initialized = true;
     printk("Hardware initialized\n");
     return HW_OK;
 }
 
 int hw_get_info(hw_info_t *info)
 {
     if (!info || !hw_initialized) return HW_ERROR_INVALID_PARAM;
     
     memset(info, 0, sizeof(hw_info_t));
     info->leds_initialized = true;
     info->gpio_initialized = true;
     info->uptime_ms = k_uptime_get_32();
     
     return HW_OK;
 }
 
 bool hw_usb_console_ready(void)
 {
     return hw_initialized;
 }
 
 /*============================================================================*/
 /* LED Control                                                                */
 /*============================================================================*/
 
 int hw_led_set_state(uint32_t led_id, bool state)
 {
     if (!hw_initialized || led_id >= HW_LED_COUNT) {
         return HW_ERROR_INVALID_PARAM;
     }
     
     const struct gpio_dt_spec *leds[] = {&led0, &led1, &led2, &led3};
     gpio_pin_set_dt(leds[led_id], state);
     
     led_states[led_id].pattern = state ? HW_PULSE_ON : HW_PULSE_OFF;
     led_states[led_id].state = state;
     
     return HW_OK;
 }
 
 int hw_led_set_pattern(uint32_t led_id, hw_led_pattern_t pattern)
 {
     if (!hw_initialized || led_id >= HW_LED_COUNT) {
         return HW_ERROR_INVALID_PARAM;
     }
     
     led_states[led_id].pattern = pattern;
     led_states[led_id].start_time = k_uptime_get_32();
     
     return HW_OK;
 }
 
 int hw_led_update_patterns(void)
 {
     if (!hw_initialized) return HW_ERROR_NOT_READY;
     
     const struct gpio_dt_spec *leds[] = {&led0, &led1, &led2, &led3};
     
     for (uint32_t i = 0; i < HW_LED_COUNT; i++) {
         update_led_pattern(i, leds[i]);
     }
     
     return HW_OK;
 }
 
 int hw_show_medical_pulse(uint32_t heart_rate_bpm)
 {
     /* Just set heartbeat pattern - simple implementation */
     return hw_led_set_pattern(HW_LED_HEARTBEAT, HW_PULSE_HEARTBEAT);
 }
 
 int hw_led_test_patterns(hw_led_pattern_t pattern)
 {
     if (!hw_initialized) return HW_ERROR_NOT_READY;
     
     printk("Testing LED patterns...\n");
     
     for (uint32_t i = 0; i < HW_LED_COUNT; i++) {
         hw_led_set_pattern(i, pattern);
     }
     
     k_sleep(K_MSEC(2000));
     
     for (uint32_t i = 0; i < HW_LED_COUNT; i++) {
         hw_led_set_pattern(i, HW_PULSE_OFF);
     }
     
     return HW_OK;
 }
 
 /*============================================================================*/
 /* Button Functions                                                           */
 /*============================================================================*/
 
 int hw_button_init(void)
 {
     /* Button already initialized in hw_init */
     return HW_OK;
 }
 
 bool hw_button_is_pressed(void)
 {
     if (!hw_initialized) return false;
     return gpio_pin_get_dt(&button) == 0; /* Active low */
 }
 
 bool hw_button_wait_press(uint32_t timeout_ms)
 {
     if (!hw_initialized) return false;
     
     printk("Waiting for button press (timeout: %u ms)...\n", timeout_ms);
     
     uint32_t start = k_uptime_get_32();
     
     while ((k_uptime_get_32() - start) < timeout_ms) {
         if (hw_button_is_pressed()) {
             /* Wait for release to debounce */
             k_sleep(K_MSEC(50));
             while (hw_button_is_pressed()) {
                 k_sleep(K_MSEC(10));
             }
             printk("Button pressed!\n");
             return true;
         }
         k_sleep(K_MSEC(10));
     }
     
     printk("Button timeout\n");
     return false;
 }
 
 uint32_t hw_button_get_press_count(void)
 {
     return dfu_state.button_presses;
 }
 
 /*============================================================================*/
 /* DFU Boot Functions - SIMPLIFIED                                           */
 /*============================================================================*/
 
 int hw_dfu_init(void)
 {
     dfu_state.in_dfu_mode = false;
     dfu_state.button_presses = 0;
     printk("DFU boot initialized\n");
     return HW_OK;
 }
 
 bool hw_dfu_boot_requested(void)
 {
     /* Check if button is pressed at boot */
     bool pressed = hw_button_is_pressed();
     if (pressed) {
         dfu_state.button_presses++;
         printk("DFU boot requested (button pressed)\n");
     }
     return pressed;
 }
 
 int hw_dfu_enter_boot_mode(void)
 {
     printk("=== ENTERING DFU BOOT MODE ===\n");
     dfu_state.in_dfu_mode = true;
     
     /* Visual indication */
     hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_FAST_BLINK);
     hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_FAST_BLINK);
     
     return HW_OK;
 }
 
 int hw_dfu_exit_boot_mode(void)
 {
     printk("=== EXITING DFU BOOT MODE ===\n");
     dfu_state.in_dfu_mode = false;
     
     /* Clear LED patterns */
     hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_OFF);
     hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
     
     return HW_OK;
 }
 
 /*============================================================================*/
 /* Bluetooth Stubs - Minimal Implementation                                  */
 /*============================================================================*/
 
 int hw_ble_advertising_init(void)
 {
     printk("BLE advertising init (stub)\n");
     return HW_OK;
 }
 
 int hw_ble_advertising_start(void)
 {
     printk("BLE advertising start (stub)\n");
     hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_SLOW_BLINK);
     return HW_OK;
 }
 
 int hw_ble_advertising_stop(void)
 {
     printk("BLE advertising stop (stub)\n");
     hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_OFF);
     return HW_OK;
 }
 
 int hw_ble_set_advertising_data(const char *device_name, const void *medical_data)
 {
     printk("BLE set advertising data: %s (stub)\n", device_name);
     return HW_OK;
 }
 
 int hw_serial_bt_init(void)
 {
     printk("Serial BT init (stub)\n");
     return HW_OK;
 }
 
 int hw_serial_bt_send(const uint8_t *data, uint32_t length)
 {
     return HW_OK;
 }
 
 int hw_serial_bt_receive(uint8_t *buffer, uint32_t max_length, uint32_t *received_length)
 {
     if (received_length) *received_length = 0;
     return HW_OK;
 }