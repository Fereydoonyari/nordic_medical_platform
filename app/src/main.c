#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "hardware.h"

void main(void)
{
    printk("\n\n=== NISC Medical Device Starting ===\n");
    printk("Firmware Version: 1.0.0\n\n");

    /* Initialize hardware */
    printk("Step 1: Init hardware...\n");
    int ret = hw_init();
    if (ret != 0) {
        printk("FATAL: Hardware init failed: %d\n", ret);
        while (1) {
            k_sleep(K_SECONDS(1));
        }
    }
    printk("SUCCESS: Hardware initialized\n\n");

    /* Turn on LED1 to show we reached here */
    printk("Step 2: Testing LED1...\n");
    hw_led_set_state(0, true);
    k_sleep(K_MSEC(500));
    hw_led_set_state(0, false);
    printk("SUCCESS: LED1 tested\n\n");

    /* Initialize Bluetooth */
    printk("Step 3: Init Bluetooth...\n");
    ret = hw_ble_advertising_init();
    if (ret != 0) {
        printk("WARNING: BT init failed: %d\n", ret);
    } else {
        printk("SUCCESS: Bluetooth initialized\n");
        
        k_sleep(K_MSEC(500));
        
        printk("Step 4: Start BT advertising...\n");
        ret = hw_ble_advertising_start();
        if (ret != 0) {
            printk("WARNING: BT advertising failed: %d\n", ret);
        } else {
            printk("SUCCESS: Bluetooth advertising started\n");
        }
    }

    printk("\n=== System Ready ===\n");
    printk("LED1 will blink every second\n\n");

    /* Simple main loop */
    while (1) {
        hw_led_set_state(0, true);
        k_sleep(K_MSEC(100));
        hw_led_set_state(0, false);
        k_sleep(K_MSEC(900));
        
        printk("Heartbeat...\n");
    }
}