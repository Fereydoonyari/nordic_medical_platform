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
     
     printk("[HW Thread] Hardware update thread started\n");
     
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
     printk("  Device: %s\n", DEVICE_MODEL);
     printk("  Build: %s %s\n", __DATE__, __TIME__);
     printk("========================================\n\n");
     
     /* Initialize hardware */
     printk("[INIT] Initializing hardware...\n");
     int ret = hw_init();
     if (ret != HW_OK) {
         printk("[ERROR] Hardware init failed: %d\n", ret);
         return -1;
     }
     printk("[OK] Hardware initialized successfully\n");
     
     /* Initialize DFU */
     printk("[INIT] Initializing DFU boot system...\n");
     hw_dfu_init();
     printk("[OK] DFU boot system ready\n");
     
     /* Flash all LEDs briefly to show startup */
     printk("[INFO] Starting LED test sequence...\n");
     for (int i = 0; i < 4; i++) {
         hw_led_set_state(i, true);
         printk("[LED] LED %d ON\n", i);
     }
     k_sleep(K_MSEC(200));
     for (int i = 0; i < 4; i++) {
         hw_led_set_state(i, false);
         printk("[LED] LED %d OFF\n", i);
     }
     printk("[OK] LED test completed\n\n");
     
     /* Check for DFU boot request */
     printk("╔═══════════════════════════════════════╗\n");
     printk("║       DFU BOOT CHECK                  ║\n");
     printk("║                                       ║\n");
     printk("║  Press Button 1 within 5 seconds     ║\n");
     printk("║  to enter DFU (firmware update) mode  ║\n");
     printk("║                                       ║\n");
     printk("║  Waiting... (5 second timeout)        ║\n");
     printk("╚═══════════════════════════════════════╝\n\n");
     
     bool dfu_requested = hw_button_wait_press(5000);
     
     if (dfu_requested) {
         /* Enter DFU mode */
         printk("\n[DFU] Button detected! Entering DFU boot mode...\n");
         hw_dfu_enter_boot_mode();
         
         printk("\n");
         printk("╔═══════════════════════════════════════╗\n");
         printk("║     🔥 DFU BOOT MODE ACTIVE 🔥        ║\n");
         printk("║                                       ║\n");
         printk("║  Status:                              ║\n");
         printk("║  • LEDs 0 & 3 blinking = DFU active  ║\n");
         printk("║  • Ready for firmware update          ║\n");
         printk("║  • Device in bootloader mode          ║\n");
         printk("║                                       ║\n");
         printk("║  Actions:                             ║\n");
         printk("║  • Press Button 1 to exit DFU mode    ║\n");
         printk("║  • Or use update tool to flash        ║\n");
         printk("╚═══════════════════════════════════════╝\n");
         printk("\n");
         
         /* Stay in DFU mode until button pressed */
         printk("[DFU] Waiting for exit command (press Button 1)...\n");
         uint32_t dfu_seconds = 0;
         while (true) {
             if (hw_button_wait_press(1000)) {
                 printk("\n[DFU] Exit button pressed!\n");
                 break;
             }
             dfu_seconds++;
             printk("[DFU] Still in DFU mode... (%u seconds elapsed)\n", dfu_seconds);
         }
         
         hw_dfu_exit_boot_mode();
         printk("[OK] DFU mode exited - transitioning to normal operation\n\n");
     } else {
         printk("\n[INFO] No DFU request detected - proceeding to normal boot\n\n");
     }
     
     /* Normal operation */
     printk("╔═══════════════════════════════════════╗\n");
     printk("║   📱 NORMAL OPERATION MODE 📱         ║\n");
     printk("╚═══════════════════════════════════════╝\n\n");
     
     printk("[INFO] LED Pattern Configuration:\n");
     printk("  • LED 0 (Status):        Breathing (system alive)\n");
     printk("  • LED 1 (Heartbeat):     Pulse (medical monitoring)\n");
     printk("  • LED 2 (Communication): Slow blink (data transmission)\n");
     printk("  • LED 3 (Error):         Off (no errors)\n\n");
     
     /* Get hardware info */
     hw_info_t hw_info;
     if (hw_get_info(&hw_info) == HW_OK) {
         printk("[INFO] Hardware Status:\n");
         printk("  • LEDs initialized:      %s\n", hw_info.leds_initialized ? "YES" : "NO");
         printk("  • GPIO initialized:      %s\n", hw_info.gpio_initialized ? "YES" : "NO");
         printk("  • System uptime:         %u ms\n", hw_info.uptime_ms);
         printk("  • Button press count:    %u\n\n", hw_button_get_press_count());
     }
     
     /* Set LED patterns for normal operation */
     printk("[LED] Activating LED patterns...\n");
     hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_BREATHING);
     printk("  ✓ LED 0: Breathing pattern active\n");
     
     hw_led_set_pattern(HW_LED_HEARTBEAT, HW_PULSE_HEARTBEAT);
     printk("  ✓ LED 1: Heartbeat pattern active\n");
     
     hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_SLOW_BLINK);
     printk("  ✓ LED 2: Slow blink active\n");
     
     hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
     printk("  ✓ LED 3: Off (no errors)\n\n");
     
     printk("╔═══════════════════════════════════════╗\n");
     printk("║        SYSTEM READY                   ║\n");
     printk("║                                       ║\n");
     printk("║  Device is now operational            ║\n");
     printk("║  Press Button 1 anytime to interact   ║\n");
     printk("╚═══════════════════════════════════════╝\n\n");
     
     /* Main loop - monitor and report status */
     uint32_t heartbeat = 0;
     while (1) {
         k_sleep(K_SECONDS(10));
         heartbeat++;
         
         uint32_t uptime_sec = k_uptime_get_32() / 1000;
         uint32_t uptime_min = uptime_sec / 60;
         uint32_t uptime_hours = uptime_min / 60;
         
         printk("\n[HEARTBEAT #%u] ═══════════════════════════\n", heartbeat);
         printk("  Uptime:         %uh %um %us\n", 
                uptime_hours, uptime_min % 60, uptime_sec % 60);
         printk("  Button presses: %u\n", hw_button_get_press_count());
         printk("  System status:  RUNNING\n");
         printk("  LEDs active:    4/4\n");
         printk("═══════════════════════════════════════════\n");
         
         /* Check for button press to demonstrate interaction */
         if (hw_button_is_pressed()) {
             printk("\n[EVENT] Button press detected!\n");
             printk("[ACTION] Flashing error LED as acknowledgment...\n");
             
             /* Flash LED3 briefly */
             for (int i = 0; i < 3; i++) {
                 hw_led_set_state(HW_LED_ERROR, true);
                 k_sleep(K_MSEC(100));
                 hw_led_set_state(HW_LED_ERROR, false);
                 k_sleep(K_MSEC(100));
             }
             
             printk("[OK] Button event processed\n");
         }
     }
     
     return 0;
 }