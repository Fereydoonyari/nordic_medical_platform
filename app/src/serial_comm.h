/**
 * @file serial_comm.h
 * @brief Serial communication interface for NISC Medical Wearable Device
 * @details Provides UART and USB serial communication for debugging and DFU
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 */

#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* Serial Configuration                                                       */
/*============================================================================*/

/** @brief Maximum serial buffer size */
#define SERIAL_MAX_BUFFER_SIZE       512

/** @brief Serial communication modes */
typedef enum {
    SERIAL_MODE_UART = 0,
    SERIAL_MODE_USB_CDC,
    SERIAL_MODE_BOTH
} serial_mode_t;

/** @brief Serial status */
typedef enum {
    SERIAL_STATUS_OK = 0,
    SERIAL_STATUS_ERROR_INIT,
    SERIAL_STATUS_ERROR_DEVICE,
    SERIAL_STATUS_ERROR_TIMEOUT,
    SERIAL_STATUS_ERROR_BUFFER_FULL
} serial_status_t;

/*============================================================================*/
/* Serial Data Structures                                                     */
/*============================================================================*/

typedef struct {
    serial_mode_t mode;
    const struct device *uart_dev;
    bool usb_ready;
    uint32_t baud_rate;
    bool flow_control;
} serial_config_t;

typedef struct {
    uint8_t buffer[SERIAL_MAX_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
    bool overflow;
} serial_buffer_t;

typedef struct {
    serial_status_t status;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t errors;
    bool connected;
} serial_stats_t;

/*============================================================================*/
/* Public API Functions                                                      */
/*============================================================================*/

/**
 * @brief Initialize serial communication
 * @param config Pointer to serial configuration
 * @return 0 on success, negative error code otherwise
 */
int serial_comm_init(const serial_config_t *config);

/**
 * @brief Send data via serial
 * @param data Pointer to data buffer
 * @param length Number of bytes to send
 * @return Number of bytes sent, negative error code otherwise
 */
int serial_comm_send(const uint8_t *data, uint16_t length);

/**
 * @brief Receive data from serial
 * @param buffer Pointer to receive buffer
 * @param max_length Maximum number of bytes to receive
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes received, negative error code otherwise
 */
int serial_comm_receive(uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms);

/**
 * @brief Check if data is available
 * @return true if data is available
 */
bool serial_comm_data_available(void);

/**
 * @brief Flush serial buffers
 * @return 0 on success
 */
int serial_comm_flush(void);

/**
 * @brief Get serial statistics
 * @param stats Pointer to serial_stats_t structure
 * @return 0 on success
 */
int serial_comm_get_stats(serial_stats_t *stats);

/**
 * @brief Print formatted string via serial
 * @param format Printf-style format string
 * @param ... Variable arguments
 * @return Number of characters printed
 */
int serial_comm_printf(const char *format, ...);

/**
 * @brief Set serial mode
 * @param mode Serial communication mode
 * @return 0 on success
 */
int serial_comm_set_mode(serial_mode_t mode);

#endif /* SERIAL_COMM_H */
