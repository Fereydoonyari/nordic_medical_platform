#ifndef MEDICAL_DEVICE_H
#define MEDICAL_DEVICE_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include "safe_queue.h"
#include "safe_buffer.h"

/**
 * @file medical_device.h
 * @brief Medical device specific functionality and data structures
 * @details Provides medical device abstractions, safety features, and
 * regulatory compliance helpers for the nRF52840 wearable device.
 */

/* Medical device error codes */
#define MEDICAL_OK                    0
#define MEDICAL_ERROR_INIT           -1
#define MEDICAL_ERROR_SENSOR         -2
#define MEDICAL_ERROR_CALIBRATION    -3
#define MEDICAL_ERROR_SAFETY         -4
#define MEDICAL_ERROR_COMMUNICATION  -5

/* Device states */
typedef enum {
    DEVICE_STATE_OFF = 0,
    DEVICE_STATE_INITIALIZING,
    DEVICE_STATE_CALIBRATING,
    DEVICE_STATE_MONITORING,
    DEVICE_STATE_ALERT,
    DEVICE_STATE_ERROR,
    DEVICE_STATE_MAINTENANCE
} device_state_t;

/* Safety alert levels */
typedef enum {
    ALERT_LEVEL_NONE = 0,
    ALERT_LEVEL_INFO,
    ALERT_LEVEL_WARNING,
    ALERT_LEVEL_CRITICAL,
    ALERT_LEVEL_EMERGENCY
} alert_level_t;

/* Sensor data types */
typedef enum {
    SENSOR_TYPE_HEART_RATE = 0,
    SENSOR_TYPE_TEMPERATURE,
    SENSOR_TYPE_MOTION,
    SENSOR_TYPE_BLOOD_OXYGEN,
    SENSOR_TYPE_MAX
} sensor_type_t;

/* Sensor data structure */
typedef struct {
    sensor_type_t type;
    float value;
    uint32_t timestamp;
    uint8_t quality;  /* Data quality indicator (0-100) */
    uint16_t flags;   /* Status flags */
} sensor_data_t;

/* Medical alert structure */
typedef struct {
    alert_level_t level;
    sensor_type_t sensor_type;
    const char *message;
    uint32_t timestamp;
    uint32_t alert_id;
} medical_alert_t;

/* Device configuration */
typedef struct {
    uint32_t sampling_rate_hz;
    uint32_t alert_thresholds[SENSOR_TYPE_MAX];
    bool safety_monitoring_enabled;
    uint32_t watchdog_timeout_ms;
} device_config_t;

/* Device statistics */
typedef struct {
    device_state_t current_state;
    uint32_t uptime_seconds;
    uint32_t total_samples;
    uint32_t alert_count;
    uint32_t error_count;
    uint8_t battery_level;
    uint8_t signal_quality;
} device_stats_t;

/**
 * @brief Initialize medical device subsystem
 * @param config Pointer to device configuration
 * @return MEDICAL_OK on success, error code otherwise
 */
int medical_device_init(const device_config_t *config);

/**
 * @brief Start medical monitoring
 * @return MEDICAL_OK on success, error code otherwise
 */
int medical_device_start_monitoring(void);

/**
 * @brief Stop medical monitoring
 * @return MEDICAL_OK on success, error code otherwise
 */
int medical_device_stop_monitoring(void);

/**
 * @brief Get current device state
 * @return Current device state
 */
device_state_t medical_device_get_state(void);

/**
 * @brief Add sensor data to processing queue
 * @param data Pointer to sensor data
 * @return MEDICAL_OK on success, error code otherwise
 */
int medical_device_add_sensor_data(const sensor_data_t *data);

/**
 * @brief Check for medical alerts
 * @param alert Pointer to alert structure to fill (if alert exists)
 * @return true if alert exists, false otherwise
 */
bool medical_device_check_alerts(medical_alert_t *alert);

/**
 * @brief Process safety checks
 * @return MEDICAL_OK if all checks pass, error code otherwise
 */
int medical_device_safety_check(void);

/**
 * @brief Get device statistics
 * @param stats Pointer to statistics structure to fill
 * @return MEDICAL_OK on success, error code otherwise
 */
int medical_device_get_stats(device_stats_t *stats);

/**
 * @brief Get next sensor data from processing queue (consumes data)
 * @param data Pointer to sensor_data_t structure to fill
 * @return MEDICAL_OK if data available, MEDICAL_ERROR_* otherwise
 */
int medical_device_get_sensor_data(sensor_data_t *data);

/**
 * @brief Process multiple sensor data items from queue
 * @param max_items Maximum number of items to process
 * @return Number of items actually processed
 */
int medical_device_process_sensor_data(uint32_t max_items);

/**
 * @brief Enter maintenance mode
 * @return MEDICAL_OK on success, error code otherwise
 */
int medical_device_enter_maintenance(void);

/**
 * @brief Exit maintenance mode
 * @return MEDICAL_OK on success, error code otherwise
 */
int medical_device_exit_maintenance(void);

/**
 * @brief Emergency shutdown procedure
 */
void medical_device_emergency_shutdown(void);

#endif /* MEDICAL_DEVICE_H */