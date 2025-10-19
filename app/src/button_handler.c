/**
 * @file button_handler.c
 * @brief Button handling implementation
 * @details Implements button press detection and debouncing for boot mode selection
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#include "button_handler.h"

/*============================================================================*/
/* Global Variables                                                           */
/*============================================================================*/

static const struct device *button_dev;
static struct gpio_callback button_callback;
static button_state_t button_state = {
    .pressed = false,
    .last_state = false,
    .press_start_time = 0,
    .last_event_time = 0,
    .last_event = BUTTON_EVENT_NONE,
    .press_count = 0,
    .hold_count = 0
};

static struct k_sem button_sem;
static bool interrupts_enabled = true;

/*============================================================================*/
/* Button Interrupt Handler                                                   */
/*============================================================================*/

static void button_interrupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);
    
    if (interrupts_enabled) {
        k_sem_give(&button_sem);
    }
}

/*============================================================================*/
/* Button Processing Functions                                                */
/*============================================================================*/

static button_event_t process_button_state(void)
{
    bool current_state = !gpio_pin_get(button_dev, BUTTON_PIN);
    uint32_t current_time = k_uptime_get_32();
    button_event_t event = BUTTON_EVENT_NONE;
    
    /* Detect button press */
    if (current_state && !button_state.last_state) {
        button_state.pressed = true;
        button_state.press_start_time = current_time;
        button_state.press_count++;
        event = BUTTON_EVENT_PRESSED;
        
        printk("Button pressed (count: %u)\n", button_state.press_count);
    }
    
    /* Detect button release */
    if (!current_state && button_state.last_state) {
        button_state.pressed = false;
        uint32_t press_duration = current_time - button_state.press_start_time;
        
        if (press_duration >= BUTTON_HOLD_RESET_MS) {
            event = BUTTON_EVENT_RESET_HOLD;
            button_state.hold_count++;
            printk("Button released after %u ms - RESET HOLD\n", press_duration);
        } else if (press_duration >= BUTTON_HOLD_DFU_MS) {
            event = BUTTON_EVENT_DFU_HOLD;
            button_state.hold_count++;
            printk("Button released after %u ms - DFU HOLD\n", press_duration);
        } else if (press_duration >= BUTTON_DEBOUNCE_MS) {
            event = BUTTON_EVENT_SHORT_PRESS;
            printk("Button released after %u ms - SHORT PRESS\n", press_duration);
        } else {
            /* Ignore very short presses (debounce) */
            event = BUTTON_EVENT_NONE;
        }
        
        button_state.last_event_time = current_time;
    }
    
    /* Detect long press while held */
    if (button_state.pressed && current_state) {
        uint32_t hold_duration = current_time - button_state.press_start_time;
        
        if (hold_duration >= BUTTON_HOLD_RESET_MS && 
            button_state.last_event != BUTTON_EVENT_RESET_HOLD) {
            event = BUTTON_EVENT_RESET_HOLD;
            printk("Button held for %u ms - RESET HOLD\n", hold_duration);
        } else if (hold_duration >= BUTTON_HOLD_DFU_MS && 
                   button_state.last_event != BUTTON_EVENT_DFU_HOLD) {
            event = BUTTON_EVENT_DFU_HOLD;
            printk("Button held for %u ms - DFU HOLD\n", hold_duration);
        }
    }
    
    button_state.last_state = current_state;
    button_state.last_event = event;
    
    return event;
}

/*============================================================================*/
/* Public API Implementation                                                  */
/*============================================================================*/

int button_handler_init(void)
{
    int ret;
    
    printk("Initializing button handler...\n");
    
    /* Get button device */
    button_dev = device_get_binding(BUTTON_PORT);
    if (!button_dev) {
        printk("Error: Button device not found\n");
        return -ENODEV;
    }
    
    /* Configure button GPIO */
    ret = gpio_pin_configure(button_dev, BUTTON_PIN, 
                            GPIO_INPUT | BUTTON_FLAGS);
    if (ret) {
        printk("Error: Failed to configure button GPIO (ret: %d)\n", ret);
        return ret;
    }
    
    /* Initialize semaphore */
    k_sem_init(&button_sem, 0, 1);
    
    /* Setup button interrupt callback */
    gpio_init_callback(&button_callback, button_interrupt_handler, 
                      BIT(BUTTON_PIN));
    ret = gpio_add_callback(button_dev, &button_callback);
    if (ret) {
        printk("Error: Failed to add button callback (ret: %d)\n", ret);
        return ret;
    }
    
    /* Enable button interrupt */
    ret = gpio_pin_interrupt_configure(button_dev, BUTTON_PIN, 
                                      GPIO_INT_EDGE_FALLING);
    if (ret) {
        printk("Error: Failed to configure button interrupt (ret: %d)\n", ret);
        return ret;
    }
    
    /* Initialize button state */
    button_state.pressed = !gpio_pin_get(button_dev, BUTTON_PIN);
    button_state.last_state = button_state.pressed;
    
    printk("Button handler initialized successfully\n");
    return 0;
}

button_event_t button_handler_check_event(void)
{
    button_event_t event = BUTTON_EVENT_NONE;
    
    /* Check if interrupt occurred */
    if (k_sem_take(&button_sem, K_NO_WAIT) == 0) {
        /* Process button state */
        event = process_button_state();
        
        /* Re-enable interrupt for next press */
        if (interrupts_enabled) {
            gpio_pin_interrupt_configure(button_dev, BUTTON_PIN, 
                                        GPIO_INT_EDGE_FALLING);
        }
    }
    
    /* Also check current state for long presses */
    if (button_state.pressed) {
        uint32_t current_time = k_uptime_get_32();
        uint32_t hold_duration = current_time - button_state.press_start_time;
        
        if (hold_duration >= BUTTON_HOLD_RESET_MS && 
            button_state.last_event != BUTTON_EVENT_RESET_HOLD) {
            event = BUTTON_EVENT_RESET_HOLD;
            button_state.last_event = event;
        } else if (hold_duration >= BUTTON_HOLD_DFU_MS && 
                   button_state.last_event != BUTTON_EVENT_DFU_HOLD) {
            event = BUTTON_EVENT_DFU_HOLD;
            button_state.last_event = event;
        }
    }
    
    return event;
}

bool button_handler_wait_for_press(uint32_t timeout_ms)
{
    uint32_t start_time = k_uptime_get_32();
    uint32_t elapsed_time = 0;
    
    printk("Waiting for button press (timeout: %u ms)...\n", timeout_ms);
    
    while (elapsed_time < timeout_ms) {
        button_event_t event = button_handler_check_event();
        
        if (event == BUTTON_EVENT_PRESSED || 
            event == BUTTON_EVENT_SHORT_PRESS ||
            event == BUTTON_EVENT_DFU_HOLD ||
            event == BUTTON_EVENT_RESET_HOLD) {
            printk("Button event detected: %d\n", event);
            return true;
        }
        
        k_sleep(K_MSEC(10));
        elapsed_time = k_uptime_get_32() - start_time;
    }
    
    printk("Button wait timeout\n");
    return false;
}

int button_handler_get_state(button_state_t *state)
{
    if (!state) {
        return -EINVAL;
    }
    
    *state = button_state;
    return 0;
}

int button_handler_reset_state(void)
{
    button_state.pressed = false;
    button_state.last_state = false;
    button_state.press_start_time = 0;
    button_state.last_event_time = 0;
    button_state.last_event = BUTTON_EVENT_NONE;
    button_state.press_count = 0;
    button_state.hold_count = 0;
    
    printk("Button state reset\n");
    return 0;
}

int button_handler_set_interrupts(bool enable)
{
    interrupts_enabled = enable;
    
    if (enable) {
        gpio_pin_interrupt_configure(button_dev, BUTTON_PIN, 
                                    GPIO_INT_EDGE_FALLING);
        printk("Button interrupts enabled\n");
    } else {
        gpio_pin_interrupt_configure(button_dev, BUTTON_PIN, 
                                    GPIO_INT_DISABLE);
        printk("Button interrupts disabled\n");
    }
    
    return 0;
}
