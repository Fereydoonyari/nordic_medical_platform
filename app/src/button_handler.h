/**
 * @file button_handler.h
 * @brief Button handling for NISC Medical Wearable Device
 * @details Provides button press detection and debouncing for boot mode selection
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* Button Configuration                                                       */
/*============================================================================*/

/** @brief Button debounce time in milliseconds */
#define BUTTON_DEBOUNCE_MS           50U

/** @brief Button hold time for DFU mode in milliseconds */
#define BUTTON_HOLD_DFU_MS           3000U

/** @brief Button hold time for factory reset in milliseconds */
#define BUTTON_HOLD_RESET_MS         10000U

/** @brief Button GPIO configuration */
#define BUTTON_PORT                  "GPIO_0"
#define BUTTON_PIN                   11
#define BUTTON_FLAGS                 (GPIO_PULL_UP | GPIO_INT_EDGE_FALLING)

/*============================================================================*/
/* Button Events                                                              */
/*============================================================================*/

typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_DFU_HOLD,
    BUTTON_EVENT_RESET_HOLD
} button_event_t;

/*============================================================================*/
/* Button State                                                               */
/*============================================================================*/

typedef struct {
    bool pressed;
    bool last_state;
    uint32_t press_start_time;
    uint32_t last_event_time;
    button_event_t last_event;
    uint32_t press_count;
    uint32_t hold_count;
} button_state_t;

/*============================================================================*/
/* Public API Functions                                                       */
/*============================================================================*/

/**
 * @brief Initialize button handler
 * @return 0 on success, negative error code otherwise
 */
int button_handler_init(void);

/**
 * @brief Check for button events
 * @return Button event that occurred
 */
button_event_t button_handler_check_event(void);

/**
 * @brief Wait for button press with timeout
 * @param timeout_ms Timeout in milliseconds
 * @return true if button was pressed
 */
bool button_handler_wait_for_press(uint32_t timeout_ms);

/**
 * @brief Get button state
 * @param state Pointer to button_state_t structure
 * @return 0 on success
 */
int button_handler_get_state(button_state_t *state);

/**
 * @brief Reset button state
 * @return 0 on success
 */
int button_handler_reset_state(void);

/**
 * @brief Enable/disable button interrupts
 * @param enable true to enable, false to disable
 * @return 0 on success
 */
int button_handler_set_interrupts(bool enable);

#endif /* BUTTON_HANDLER_H */
