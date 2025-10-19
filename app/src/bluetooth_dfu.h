/**
 * @file bluetooth_dfu.h
 * @brief Bluetooth Low Energy DFU service for NISC Medical Wearable Device
 * @details Provides BLE advertising and DFU service for firmware updates
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 */

#ifndef BLUETOOTH_DFU_H
#define BLUETOOTH_DFU_H

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* DFU Service Configuration                                                  */
/*============================================================================*/

/** @brief DFU Service UUID */
#define DFU_SERVICE_UUID             0x00001530
#define DFU_CHAR_UUID                0x00001531
#define DFU_CHAR_NOTIFY_UUID         0x00001532

/** @brief DFU Commands */
#define DFU_CMD_START                0x01
#define DFU_CMD_DATA                 0x02
#define DFU_CMD_END                  0x03
#define DFU_CMD_ABORT                0x04
#define DFU_CMD_STATUS               0x05

/** @brief DFU Status Codes */
#define DFU_STATUS_OK                0x00
#define DFU_STATUS_ERROR             0x01
#define DFU_STATUS_BUSY              0x02
#define DFU_STATUS_INVALID_DATA      0x03

/** @brief Maximum DFU data packet size */
#define DFU_MAX_PACKET_SIZE          244

/*============================================================================*/
/* DFU State Machine                                                          */
/*============================================================================*/

typedef enum {
    DFU_STATE_IDLE = 0,
    DFU_STATE_RECEIVING,
    DFU_STATE_VALIDATING,
    DFU_STATE_WRITING,
    DFU_STATE_COMPLETE,
    DFU_STATE_ERROR
} dfu_state_t;

/*============================================================================*/
/* DFU Data Structures                                                        */
/*============================================================================*/

typedef struct {
    uint8_t command;
    uint16_t length;
    uint8_t data[DFU_MAX_PACKET_SIZE];
} dfu_packet_t;

typedef struct {
    dfu_state_t state;
    uint32_t total_size;
    uint32_t received_size;
    uint32_t crc32;
    bool connected;
    struct bt_conn *conn;
} dfu_context_t;

/*============================================================================*/
/* Public API Functions                                                       */
/*============================================================================*/

/**
 * @brief Initialize Bluetooth DFU service
 * @return 0 on success, negative error code otherwise
 */
int bluetooth_dfu_init(void);

/**
 * @brief Start DFU advertising
 * @return 0 on success, negative error code otherwise
 */
int bluetooth_dfu_start_advertising(void);

/**
 * @brief Stop DFU advertising
 * @return 0 on success, negative error code otherwise
 */
int bluetooth_dfu_stop_advertising(void);

/**
 * @brief Get DFU context
 * @param ctx Pointer to dfu_context_t structure
 * @return 0 on success
 */
int bluetooth_dfu_get_context(dfu_context_t *ctx);

/**
 * @brief Process DFU packet
 * @param packet Pointer to dfu_packet_t
 * @return 0 on success, negative error code otherwise
 */
int bluetooth_dfu_process_packet(const dfu_packet_t *packet);

/**
 * @brief Send DFU status notification
 * @param status Status code to send
 * @return 0 on success, negative error code otherwise
 */
int bluetooth_dfu_send_status(uint8_t status);

#endif /* BLUETOOTH_DFU_H */
