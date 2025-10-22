/**
 * @file main.c
 * @brief Simple Nordic Medical Platform with DFU mode and Bluetooth advertising
 * @details Simple implementation with DFU mode detection, button wait, and Bluetooth advertising
 * 
 * @author Nordic Medical Platform
 * @version 2.0.0
 * @date 2024
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>

/* Include only necessary components */
#include "common.h"
#include "hardware.h"

/*============================================================================*/
/* Simple Configuration                                                        */
/*============================================================================*/

/** @brief DFU button wait timeout in milliseconds */
#define DFU_BUTTON_WAIT_MS      5000U

/** @brief Normal operation LED blink interval */
#define LED_BLINK_INTERVAL_MS   1000U

/**
 * @brief Main application entry point
 * @details Simple implementation with DFU mode detection, button wait, and Bluetooth advertising
 * 
 * @note This function does not return - it runs the main system loop indefinitely
 */
void main(void) 
{
    int ret;

    printk("\n=== Nordic Medical Platform Starting ===\n");
    printk("Firmware Version: %s\n", APP_VERSION_STRING);
    printk("Device Model: %s\n", DEVICE_MODEL);
    printk("Target Platform: nRF52840 Development Kit\n");

    /* Initialize hardware abstraction layer */
    printk("Initializing hardware...\n");
    ret = hw_init();
    if (ret != HW_OK) {
        printk("FATAL: Hardware initialization failed (error: %d)\n", ret);
        return;
    }

    /* Initialize DFU boot process */
    printk("Initializing DFU boot process...\n");
    ret = hw_dfu_init();
    if (ret != HW_OK) {
        printk("WARNING: DFU initialization failed (error: %d)\n", ret);
    }

    /* Check if DFU boot is requested (button pressed during startup) */
    if (hw_dfu_boot_requested()) {
        printk("DFU boot requested - entering DFU mode\n");
        ret = hw_dfu_enter_boot_mode();
        if (ret == HW_OK) {
            /* Wait for button press to exit DFU mode */
            printk("Waiting for button press to exit DFU mode...\n");
            while (hw_dfu_boot_requested()) {
                if (hw_button_is_pressed()) {
                    printk("Button pressed - exiting DFU mode\n");
                    hw_dfu_exit_boot_mode();
                    break;
                }
                k_sleep(K_MSEC(100));
            }
        }
    }

    /* Wait 5 seconds for button press to start normal operation */
    printk("Waiting 5 seconds for button press to start normal operation...\n");
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_SLOW_BLINK);
    
    bool button_pressed = hw_button_wait_press(DFU_BUTTON_WAIT_MS);
    if (button_pressed) {
        printk("Button pressed - starting normal operation\n");
    } else {
        printk("Timeout - starting normal operation automatically\n");
    }
    
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_OFF);

    /* Initialize Bluetooth advertising */
    printk("Initializing Bluetooth advertising...\n");
    ret = hw_ble_advertising_init();
    if (ret != HW_OK) {
        printk("WARNING: Bluetooth advertising initialization failed: %d\n", ret);
    } else {
        /* Start Bluetooth advertising */
        ret = hw_ble_advertising_start();
        if (ret == HW_OK) {
            printk("Bluetooth advertising started - Device discoverable\n");
        } else {
            printk("WARNING: Failed to start Bluetooth advertising: %d\n", ret);
        }
    }

    printk("=== System Ready - Normal Operation ===\n");

    /* Set LED patterns for normal operation */
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_BREATHING);      /* Breathing = system OK */
    hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_SLOW_BLINK); /* Bluetooth advertising */

    /* Main loop - simple LED blinking and system monitoring */
    while (1) {
        /* Simple LED blink to show system is alive */
        hw_led_set_state(HW_LED_HEARTBEAT, true);
        k_sleep(K_MSEC(LED_BLINK_INTERVAL_MS));
        hw_led_set_state(HW_LED_HEARTBEAT, false);
        k_sleep(K_MSEC(LED_BLINK_INTERVAL_MS));
        
        printk("System running - Uptime: %u ms\n", k_uptime_get_32());
    }
}
