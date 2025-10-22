/**
 * @file main.c
 * @brief Simplified main with working DFU boot process
 */

 #include <zephyr/kernel.h>
 #include <zephyr/sys/printk.h>
 #include "hardware.h"
 #include "common.h"
 
 /*============================================================================*/
 /* Simple Thread Functions                                                    */
 /*============================================================================*/
 
 void hardware_update_thread(void *a, void *b, void *c)
 {
     ARG_UNUSED(a);
     ARG_UNUSED(b);
     ARG_UNUSED(c);
     
     while (1) {
         hw_led_update_patterns();
         k_sleep(K_MSEC(50));
     }
 }
 
 K_THREAD_DEFINE(hw_update_tid, 512, hardware_update_thread, 
                 NULL, NULL, NULL, 7, 0, 0);
 
 /*============================================================================*/
 /* Main Application                                                           */
 /*============================================================================*/
 
 int main(void)
 {
     printk("\n\n");
     printk("========================================\n");
     printk("  NISC Medical Wearable - DFU Boot Demo\n");
     printk("  Version: %s\n", APP_VERSION_STRING);
     printk("========================================\n\n");
     
     /* Initialize hardware */
     int ret = hw_init();
     if (ret != HW_OK) {
         printk("FATAL: Hardware init failed: %d\n", ret);
         return;
     }
     
     /* Initialize DFU */
     hw_dfu_init();
     
     /* Flash all LEDs briefly to show startup */
     printk("Starting up...\n");
     for (int i = 0; i < 4; i++) {
         hw_led_set_state(i, true);
     }
     k_sleep(K_MSEC(200));
     for (int i = 0; i < 4; i++) {
         hw_led_set_state(i, false);
     }
     
     /* Check for DFU boot request */
     printk("\n--- DFU Boot Check ---\n");
     printk("Press Button 1 within 5 seconds to enter DFU mode\n");
     
     bool dfu_requested = hw_button_wait_press(5000);
     
     if (dfu_requested) {
         /* Enter DFU mode */
         hw_dfu_enter_boot_mode();
         
         printk("\n");
         printk("╔═══════════════════════════════════╗\n");
         printk("║     DFU BOOT MODE ACTIVE          ║\n");
         printk("║                                   ║\n");
         printk("║  LEDs 0 & 3 blinking = DFU mode  ║\n");
         printk("║  Press button to exit DFU         ║\n");
         printk("╚═══════════════════════════════════╝\n");
         printk("\n");
         
         /* Stay in DFU mode until button pressed */
         printk("Waiting for button press to exit DFU...\n");
         while (true) {
             if (hw_button_wait_press(1000)) {
                 printk("Button pressed - exiting DFU mode\n");
                 break;
             }
             printk("Still in DFU mode... (press button to exit)\n");
         }
         
         hw_dfu_exit_boot_mode();
         printk("DFU mode exited - starting normal operation\n\n");
     } else {
         printk("No DFU request - starting normal operation\n\n");
     }
     
     /* Normal operation */
     printk("--- Normal Operation Started ---\n");
     printk("LED patterns:\n");
     printk("  LED 0: Breathing (system alive)\n");
     printk("  LED 1: Heartbeat (medical pulse)\n");
     printk("  LED 2: Slow blink (communication)\n");
     printk("  LED 3: Off (no errors)\n\n");
     
     /* Set LED patterns for normal operation */
     hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_BREATHING);
     hw_led_set_pattern(HW_LED_HEARTBEAT, HW_PULSE_HEARTBEAT);
     hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_SLOW_BLINK);
     hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
     
     /* Main loop - just monitor button presses */
     uint32_t heartbeat = 0;
     while (1) {
         k_sleep(K_SECONDS(10));
         heartbeat++;
         printk("[%u] System running normally (button presses: %u)\n", 
                heartbeat, hw_button_get_press_count());
         
         /* Check for button press to demonstrate interaction */
         if (hw_button_is_pressed()) {
             printk("Button pressed during operation!\n");
             /* Flash LED3 briefly */
             hw_led_set_state(HW_LED_ERROR, true);
             k_sleep(K_MSEC(100));
             hw_led_set_state(HW_LED_ERROR, false);
         }
     }
 }