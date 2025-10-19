/**
 * @file bluetooth_dfu.c
 * @brief Bluetooth Low Energy DFU service implementation
 * @details Implements BLE advertising and DFU service for firmware updates
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/crc.h>
#include <zephyr/storage/flash_map.h>

#include "bluetooth_dfu.h"
#include "bootloader.h"

/*============================================================================*/
/* DFU Service UUIDs                                                          */
/*============================================================================*/

static struct bt_uuid_128 dfu_service_uuid = BT_UUID_INIT_128(
    0x30, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
);

static struct bt_uuid_128 dfu_char_uuid = BT_UUID_INIT_128(
    0x31, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
);

static struct bt_uuid_128 dfu_notify_uuid = BT_UUID_INIT_128(
    0x32, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
);

/*============================================================================*/
/* Global Variables                                                           */
/*============================================================================*/

static dfu_context_t dfu_ctx = {
    .state = DFU_STATE_IDLE,
    .total_size = 0,
    .received_size = 0,
    .crc32 = 0,
    .connected = false,
    .conn = NULL
};

static struct bt_gatt_service dfu_service;
static struct bt_gatt_attr dfu_attrs[3];
static struct bt_gatt_ccc_cfg dfu_ccc_cfg[BT_GATT_CCC_MAX] = {};

/*============================================================================*/
/* DFU Characteristic Callbacks                                               */
/*============================================================================*/

static ssize_t dfu_char_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                              const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(offset);
    ARG_UNUSED(flags);
    
    if (len < 3) {
        printk("DFU: Invalid packet length: %u\n", len);
        bluetooth_dfu_send_status(DFU_STATUS_INVALID_DATA);
        return -EINVAL;
    }
    
    dfu_packet_t packet;
    packet.command = ((uint8_t*)buf)[0];
    packet.length = ((uint16_t*)buf)[1];
    
    if (packet.length > DFU_MAX_PACKET_SIZE) {
        printk("DFU: Packet too large: %u\n", packet.length);
        bluetooth_dfu_send_status(DFU_STATUS_INVALID_DATA);
        return -EINVAL;
    }
    
    if (len < (3 + packet.length)) {
        printk("DFU: Incomplete packet\n");
        bluetooth_dfu_send_status(DFU_STATUS_INVALID_DATA);
        return -EINVAL;
    }
    
    memcpy(packet.data, &((uint8_t*)buf)[3], packet.length);
    
    return bluetooth_dfu_process_packet(&packet);
}

static void dfu_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    ARG_UNUSED(attr);
    
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    
    printk("DFU notifications %s\n", notif_enabled ? "enabled" : "disabled");
}

/*============================================================================*/
/* GATT Service Definition                                                    */
/*============================================================================*/

BT_GATT_SERVICE_DEFINE(dfu_svc,
    BT_GATT_PRIMARY_SERVICE(&dfu_service_uuid),
    BT_GATT_CHARACTERISTIC(&dfu_char_uuid.uuid,
                          BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                          BT_GATT_PERM_WRITE,
                          NULL, dfu_char_write, NULL),
    BT_GATT_CHARACTERISTIC(&dfu_notify_uuid.uuid,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(dfu_ccc_cfg, dfu_ccc_cfg_changed),
);

/*============================================================================*/
/* Connection Callbacks                                                       */
/*============================================================================*/

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("DFU: Connection failed (err %u)\n", err);
        return;
    }
    
    dfu_ctx.connected = true;
    dfu_ctx.conn = conn;
    
    printk("DFU: Client connected\n");
    bluetooth_dfu_send_status(DFU_STATUS_OK);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(conn);
    
    dfu_ctx.connected = false;
    dfu_ctx.conn = NULL;
    dfu_ctx.state = DFU_STATE_IDLE;
    
    printk("DFU: Client disconnected (reason %u)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/*============================================================================*/
/* DFU Processing Functions                                                   */
/*============================================================================*/

static int dfu_start_update(const dfu_packet_t *packet)
{
    if (packet->length < 8) {
        printk("DFU: Start command too short\n");
        return -EINVAL;
    }
    
    dfu_ctx.total_size = *(uint32_t*)&packet->data[0];
    dfu_ctx.crc32 = *(uint32_t*)&packet->data[4];
    dfu_ctx.received_size = 0;
    dfu_ctx.state = DFU_STATE_RECEIVING;
    
    printk("DFU: Starting update - Size: %u bytes, CRC32: 0x%08x\n", 
           dfu_ctx.total_size, dfu_ctx.crc32);
    
    return 0;
}

static int dfu_process_data(const dfu_packet_t *packet)
{
    if (dfu_ctx.state != DFU_STATE_RECEIVING) {
        printk("DFU: Not in receiving state\n");
        return -EINVAL;
    }
    
    if ((dfu_ctx.received_size + packet->length) > dfu_ctx.total_size) {
        printk("DFU: Data exceeds total size\n");
        return -EINVAL;
    }
    
    /* TODO: Write data to flash */
    printk("DFU: Received %u bytes (total: %u/%u)\n", 
           packet->length, dfu_ctx.received_size + packet->length, dfu_ctx.total_size);
    
    dfu_ctx.received_size += packet->length;
    
    if (dfu_ctx.received_size >= dfu_ctx.total_size) {
        dfu_ctx.state = DFU_STATE_VALIDATING;
        printk("DFU: All data received, validating...\n");
        
        /* TODO: Validate CRC32 */
        dfu_ctx.state = DFU_STATE_COMPLETE;
        bluetooth_dfu_send_status(DFU_STATUS_OK);
    }
    
    return 0;
}

static int dfu_end_update(const dfu_packet_t *packet)
{
    ARG_UNUSED(packet);
    
    if (dfu_ctx.state != DFU_STATE_RECEIVING) {
        printk("DFU: Not in receiving state\n");
        return -EINVAL;
    }
    
    if (dfu_ctx.received_size != dfu_ctx.total_size) {
        printk("DFU: Incomplete data received\n");
        return -EINVAL;
    }
    
    dfu_ctx.state = DFU_STATE_VALIDATING;
    printk("DFU: Update complete, validating...\n");
    
    /* TODO: Validate and install firmware */
    dfu_ctx.state = DFU_STATE_COMPLETE;
    bluetooth_dfu_send_status(DFU_STATUS_OK);
    
    return 0;
}

static int dfu_abort_update(const dfu_packet_t *packet)
{
    ARG_UNUSED(packet);
    
    printk("DFU: Update aborted\n");
    dfu_ctx.state = DFU_STATE_IDLE;
    dfu_ctx.received_size = 0;
    dfu_ctx.total_size = 0;
    
    bluetooth_dfu_send_status(DFU_STATUS_OK);
    return 0;
}

/*============================================================================*/
/* Public API Implementation                                                  */
/*============================================================================*/

int bluetooth_dfu_init(void)
{
    int ret;
    
    printk("Initializing Bluetooth DFU service...\n");
    
    /* Register connection callbacks */
    bt_conn_cb_register(&conn_callbacks);
    
    /* Initialize DFU context */
    dfu_ctx.state = DFU_STATE_IDLE;
    dfu_ctx.connected = false;
    dfu_ctx.conn = NULL;
    
    printk("Bluetooth DFU service initialized\n");
    return 0;
}

int bluetooth_dfu_start_advertising(void)
{
    struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    };
    
    const char *adv_name = "NISC-DFU";
    int ret;
    
    ret = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);
    if (ret) {
        printk("Error: DFU advertising failed to start (ret: %d)\n", ret);
        return ret;
    }
    
    printk("DFU advertising started: %s\n", adv_name);
    return 0;
}

int bluetooth_dfu_stop_advertising(void)
{
    int ret = bt_le_adv_stop();
    if (ret) {
        printk("Error: Failed to stop DFU advertising (ret: %d)\n", ret);
        return ret;
    }
    
    printk("DFU advertising stopped\n");
    return 0;
}

int bluetooth_dfu_get_context(dfu_context_t *ctx)
{
    if (!ctx) {
        return -EINVAL;
    }
    
    *ctx = dfu_ctx;
    return 0;
}

int bluetooth_dfu_process_packet(const dfu_packet_t *packet)
{
    if (!packet) {
        return -EINVAL;
    }
    
    switch (packet->command) {
        case DFU_CMD_START:
            return dfu_start_update(packet);
            
        case DFU_CMD_DATA:
            return dfu_process_data(packet);
            
        case DFU_CMD_END:
            return dfu_end_update(packet);
            
        case DFU_CMD_ABORT:
            return dfu_abort_update(packet);
            
        case DFU_CMD_STATUS:
            bluetooth_dfu_send_status(DFU_STATUS_OK);
            return 0;
            
        default:
            printk("DFU: Unknown command: 0x%02x\n", packet->command);
            bluetooth_dfu_send_status(DFU_STATUS_ERROR);
            return -EINVAL;
    }
}

int bluetooth_dfu_send_status(uint8_t status)
{
    if (!dfu_ctx.connected || !dfu_ctx.conn) {
        return -ENOTCONN;
    }
    
    int ret = bt_gatt_notify(dfu_ctx.conn, &dfu_svc.attrs[2], &status, sizeof(status));
    if (ret) {
        printk("Error: Failed to send DFU status notification (ret: %d)\n", ret);
        return ret;
    }
    
    printk("DFU status sent: 0x%02x\n", status);
    return 0;
}
