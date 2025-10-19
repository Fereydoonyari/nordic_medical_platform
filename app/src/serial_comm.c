/**
 * @file serial_comm.c
 * @brief Serial communication implementation
 * @details Implements UART and USB CDC serial communication
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>
#include <stdarg.h>
#include <stdio.h>

#include "serial_comm.h"

/*============================================================================*/
/* Global Variables                                                           */
/*============================================================================*/

static serial_config_t serial_config = {
    .mode = SERIAL_MODE_USB_CDC,
    .uart_dev = NULL,
    .usb_ready = false,
    .baud_rate = 115200,
    .flow_control = false
};

static serial_buffer_t rx_buffer = {
    .head = 0,
    .tail = 0,
    .count = 0,
    .overflow = false
};

static serial_stats_t serial_stats = {
    .status = SERIAL_STATUS_OK,
    .bytes_sent = 0,
    .bytes_received = 0,
    .errors = 0,
    .connected = false
};

static struct k_sem rx_sem;

/*============================================================================*/
/* UART Callback Functions                                                    */
/*============================================================================*/

static void uart_callback(const struct device *dev, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);
    
    uint8_t byte;
    int ret;
    
    /* Read available data */
    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            ret = uart_fifo_read(dev, &byte, 1);
            if (ret > 0) {
                /* Add to ring buffer */
                if (rx_buffer.count < SERIAL_MAX_BUFFER_SIZE) {
                    rx_buffer.buffer[rx_buffer.head] = byte;
                    rx_buffer.head = (rx_buffer.head + 1) % SERIAL_MAX_BUFFER_SIZE;
                    rx_buffer.count++;
                    serial_stats.bytes_received++;
                } else {
                    rx_buffer.overflow = true;
                    serial_stats.errors++;
                }
                
                k_sem_give(&rx_sem);
            }
        }
    }
}

/*============================================================================*/
/* USB CDC Callback Functions                                                 */
/*============================================================================*/

static void usb_cdc_callback(const struct device *dev, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);
    
    uint8_t byte;
    int ret;
    
    /* Read available data from USB CDC */
    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            ret = uart_fifo_read(dev, &byte, 1);
            if (ret > 0) {
                /* Add to ring buffer */
                if (rx_buffer.count < SERIAL_MAX_BUFFER_SIZE) {
                    rx_buffer.buffer[rx_buffer.head] = byte;
                    rx_buffer.head = (rx_buffer.head + 1) % SERIAL_MAX_BUFFER_SIZE;
                    rx_buffer.count++;
                    serial_stats.bytes_received++;
                } else {
                    rx_buffer.overflow = true;
                    serial_stats.errors++;
                }
                
                k_sem_give(&rx_sem);
            }
        }
    }
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static int serial_init_uart(void)
{
    int ret;
    
    /* Get UART device */
    serial_config.uart_dev = device_get_binding("UART_0");
    if (!serial_config.uart_dev) {
        printk("Error: UART device not found\n");
        return -ENODEV;
    }
    
    /* Configure UART */
    ret = uart_configure(serial_config.uart_dev, &(struct uart_config){
        .baudrate = serial_config.baud_rate,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = serial_config.flow_control ? UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE
    });
    
    if (ret) {
        printk("Error: UART configuration failed (ret: %d)\n", ret);
        return ret;
    }
    
    /* Setup UART interrupt */
    uart_irq_callback_user_data_set(serial_config.uart_dev, uart_callback, NULL);
    uart_irq_rx_enable(serial_config.uart_dev);
    
    printk("UART initialized at %u baud\n", serial_config.baud_rate);
    return 0;
}

static int serial_init_usb_cdc(void)
{
    int ret;
    
    /* Initialize USB */
    ret = usb_enable(NULL);
    if (ret) {
        printk("Error: USB initialization failed (ret: %d)\n", ret);
        return ret;
    }
    
    /* Wait for USB to be ready */
    k_sleep(K_MSEC(1000));
    
    /* Get USB CDC device */
    const struct device *usb_cdc_dev = device_get_binding("CDC_ACM_0");
    if (!usb_cdc_dev) {
        printk("Error: USB CDC device not found\n");
        return -ENODEV;
    }
    
    /* Setup USB CDC interrupt */
    uart_irq_callback_user_data_set(usb_cdc_dev, usb_cdc_callback, NULL);
    uart_irq_rx_enable(usb_cdc_dev);
    
    serial_config.usb_ready = true;
    serial_stats.connected = true;
    
    printk("USB CDC initialized\n");
    return 0;
}

/*============================================================================*/
/* Public API Implementation                                                  */
/*============================================================================*/

int serial_comm_init(const serial_config_t *config)
{
    int ret;
    
    if (!config) {
        return -EINVAL;
    }
    
    /* Copy configuration */
    serial_config = *config;
    
    /* Initialize semaphore */
    k_sem_init(&rx_sem, 0, 1);
    
    /* Initialize selected mode */
    switch (serial_config.mode) {
        case SERIAL_MODE_UART:
            ret = serial_init_uart();
            break;
            
        case SERIAL_MODE_USB_CDC:
            ret = serial_init_usb_cdc();
            break;
            
        case SERIAL_MODE_BOTH:
            ret = serial_init_uart();
            if (ret == 0) {
                ret = serial_init_usb_cdc();
            }
            break;
            
        default:
            printk("Error: Invalid serial mode\n");
            return -EINVAL;
    }
    
    if (ret) {
        serial_stats.status = SERIAL_STATUS_ERROR_INIT;
        serial_stats.errors++;
        return ret;
    }
    
    serial_stats.status = SERIAL_STATUS_OK;
    printk("Serial communication initialized\n");
    return 0;
}

int serial_comm_send(const uint8_t *data, uint16_t length)
{
    if (!data || length == 0) {
        return -EINVAL;
    }
    
    int ret = 0;
    int total_sent = 0;
    
    /* Send via UART if enabled */
    if (serial_config.mode == SERIAL_MODE_UART || serial_config.mode == SERIAL_MODE_BOTH) {
        if (serial_config.uart_dev) {
            for (uint16_t i = 0; i < length; i++) {
                uart_poll_out(serial_config.uart_dev, data[i]);
                total_sent++;
            }
        }
    }
    
    /* Send via USB CDC if enabled */
    if (serial_config.mode == SERIAL_MODE_USB_CDC || serial_config.mode == SERIAL_MODE_BOTH) {
        if (serial_config.usb_ready) {
            const struct device *usb_cdc_dev = device_get_binding("CDC_ACM_0");
            if (usb_cdc_dev) {
                for (uint16_t i = 0; i < length; i++) {
                    uart_poll_out(usb_cdc_dev, data[i]);
                }
            }
        }
    }
    
    serial_stats.bytes_sent += total_sent;
    return total_sent;
}

int serial_comm_receive(uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms)
{
    if (!buffer || max_length == 0) {
        return -EINVAL;
    }
    
    int received = 0;
    int ret;
    
    /* Wait for data with timeout */
    ret = k_sem_take(&rx_sem, K_MSEC(timeout_ms));
    if (ret) {
        return -ETIMEDOUT;
    }
    
    /* Read available data */
    while (received < max_length && rx_buffer.count > 0) {
        buffer[received] = rx_buffer.buffer[rx_buffer.tail];
        rx_buffer.tail = (rx_buffer.tail + 1) % SERIAL_MAX_BUFFER_SIZE;
        rx_buffer.count--;
        received++;
        
        /* Check for more data without blocking */
        if (rx_buffer.count == 0) {
            ret = k_sem_take(&rx_sem, K_NO_WAIT);
            if (ret) {
                break; /* No more data available */
            }
        }
    }
    
    return received;
}

bool serial_comm_data_available(void)
{
    return rx_buffer.count > 0;
}

int serial_comm_flush(void)
{
    rx_buffer.head = 0;
    rx_buffer.tail = 0;
    rx_buffer.count = 0;
    rx_buffer.overflow = false;
    
    return 0;
}

int serial_comm_get_stats(serial_stats_t *stats)
{
    if (!stats) {
        return -EINVAL;
    }
    
    *stats = serial_stats;
    return 0;
}

int serial_comm_printf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    int len;
    
    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        return serial_comm_send((uint8_t*)buffer, len);
    }
    
    return -EINVAL;
}

int serial_comm_set_mode(serial_mode_t mode)
{
    if (mode >= SERIAL_MODE_BOTH + 1) {
        return -EINVAL;
    }
    
    serial_config.mode = mode;
    return 0;
}
