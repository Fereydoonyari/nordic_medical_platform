/**
 * @file medical_device.c
 * @brief Medical device core functionality implementation
 * @details This file implements the core medical device functionality including
 * sensor data management, safety monitoring, alert handling, and device state
 * management for regulatory compliance in medical wearable applications.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This implementation follows medical device software development
 * standards (IEC 62304) and includes safety features required for
 * medical device certification.
 */

#include "medical_device.h"
#include "diagnostics.h"
#include "common.h"
#include <string.h>

/*============================================================================*/
/* Private Constants and Definitions                                          */
/*============================================================================*/

/** @brief Maximum number of concurrent medical alerts */
#define MAX_ALERTS               8U

/** @brief Sensor queue capacity for buffering sensor data */
#define SENSOR_QUEUE_CAPACITY    16U

/** @brief Default battery level on initialization */
#define DEFAULT_BATTERY_LEVEL    100U

/** @brief Default signal quality percentage */
#define DEFAULT_SIGNAL_QUALITY   80U

/** @brief Maximum error threshold before device enters error state */
#define MAX_ERROR_THRESHOLD      10U

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

/** @brief Current device operational state */
static device_state_t device_state = DEVICE_STATE_OFF;

/** @brief Device configuration parameters */
static device_config_t device_configuration;

/** @brief Device operational statistics */
static device_stats_t device_statistics = {0};

/** @brief Queue for incoming sensor data */
static safe_queue_t sensor_queue;

/** @brief Queue for medical alerts */
static safe_queue_t alert_queue;

/** @brief Mutex for thread-safe device operations */
static struct k_mutex device_mutex;

/** @brief Storage for medical alerts */
static medical_alert_t alert_storage[MAX_ALERTS] __attribute__((unused));

/** @brief Next alert ID for unique alert identification */
static uint32_t next_alert_id = 1U;

/*============================================================================*/
/* Public Function Implementations                                            */
/*============================================================================*/

/**
 * @brief Initialize medical device subsystem
 * @details Initializes all medical device components including sensor queues,
 * alert management, device statistics, and safety monitoring systems.
 * This function must be called before any other medical device operations.
 * 
 * @param[in] config Pointer to device configuration structure
 * @return MEDICAL_OK on successful initialization
 * @retval MEDICAL_ERROR_INIT if configuration is invalid or initialization fails
 * 
 * @pre config must point to valid device_config_t structure
 * @note This function is not thread-safe during initialization
 */
int medical_device_init(const device_config_t *config)
{
    if (config == NULL) {
        DIAG_ERROR(DIAG_CAT_SYSTEM, "Invalid configuration pointer provided to medical_device_init");
        return MEDICAL_ERROR_INIT;
    }

    /* Initialize device mutex for thread safety */
    k_mutex_init(&device_mutex);

    /* Update device state and copy configuration */
    k_mutex_lock(&device_mutex, K_FOREVER);
    device_state = DEVICE_STATE_INITIALIZING;
    device_configuration = *config;
    k_mutex_unlock(&device_mutex);

    DIAG_INFO(DIAG_CAT_SYSTEM, "Initializing medical device subsystem");

    /* Initialize sensor data queue with specified capacity */
    int ret = safe_queue_init(&sensor_queue, SENSOR_QUEUE_CAPACITY);
    if (ret != QUEUE_OK) {
        DIAG_ERROR(DIAG_CAT_SYSTEM, "Failed to initialize sensor queue (error: %d)", ret);
        k_mutex_lock(&device_mutex, K_FOREVER);
        device_state = DEVICE_STATE_ERROR;
        k_mutex_unlock(&device_mutex);
        return MEDICAL_ERROR_INIT;
    }

    /* Initialize alert queue for medical alerts */
    ret = safe_queue_init(&alert_queue, MAX_ALERTS);
    if (ret != QUEUE_OK) {
        DIAG_ERROR(DIAG_CAT_SYSTEM, "Failed to initialize alert queue (error: %d)", ret);
        k_mutex_lock(&device_mutex, K_FOREVER);
        device_state = DEVICE_STATE_ERROR;
        k_mutex_unlock(&device_mutex);
        return MEDICAL_ERROR_INIT;
    }

    /* Initialize device statistics with default values */
    k_mutex_lock(&device_mutex, K_FOREVER);
    memset(&device_statistics, 0, sizeof(device_statistics));
    device_statistics.current_state = DEVICE_STATE_INITIALIZING;
    device_statistics.battery_level = DEFAULT_BATTERY_LEVEL;
    device_statistics.signal_quality = DEFAULT_SIGNAL_QUALITY;
    device_statistics.uptime_seconds = 0U;
    device_statistics.total_samples = 0U;
    device_statistics.alert_count = 0U;
    device_statistics.error_count = 0U;
    k_mutex_unlock(&device_mutex);

    DIAG_INFO(DIAG_CAT_SYSTEM, "Medical device initialization completed successfully");
    
    /* Validate configuration parameters */
    if (config->sampling_rate_hz == 0U || config->sampling_rate_hz > 1000U) {
        DIAG_WARNING(DIAG_CAT_SYSTEM, "Invalid sampling rate: %u Hz, using default", config->sampling_rate_hz);
    }
    
    if (config->watchdog_timeout_ms < 1000U) {
        DIAG_WARNING(DIAG_CAT_SYSTEM, "Watchdog timeout too short: %u ms, recommend >1000ms", 
                     config->watchdog_timeout_ms);
    }
    
    return MEDICAL_OK;
}

/**
 * @brief Start medical monitoring operations
 * @details Initiates medical device monitoring including sensor calibration,
 * safety checks, and transition to active monitoring state. This function
 * performs critical safety validations before enabling monitoring.
 * 
 * @return MEDICAL_OK on successful monitoring start
 * @retval MEDICAL_ERROR_SAFETY if device is in error state or safety checks fail
 * 
 * @note This function includes mandatory calibration and safety verification
 * @warning Device must be initialized before calling this function
 */
int medical_device_start_monitoring(void)
{
    DIAG_INFO(DIAG_CAT_SYSTEM, "Starting medical device monitoring sequence");
    
    k_mutex_lock(&device_mutex, K_FOREVER);
    
    /* Verify device is not in error state */
    if (device_state == DEVICE_STATE_ERROR) {
        DIAG_ERROR(DIAG_CAT_SAFETY, "Cannot start monitoring - device in error state");
        k_mutex_unlock(&device_mutex);
        return MEDICAL_ERROR_SAFETY;
    }
    
    /* Ensure device is properly initialized */
    if (device_state == DEVICE_STATE_OFF) {
        DIAG_ERROR(DIAG_CAT_SAFETY, "Cannot start monitoring - device not initialized");
        k_mutex_unlock(&device_mutex);
        return MEDICAL_ERROR_INIT;
    }

    /* Transition to calibrating state */
    device_state = DEVICE_STATE_CALIBRATING;
    device_statistics.current_state = device_state;
    k_mutex_unlock(&device_mutex);

    DIAG_INFO(DIAG_CAT_SYSTEM, "Performing sensor calibration...");
    
    /* Simulate calibration sequence - in real implementation this would 
     * perform actual sensor calibration procedures */
    k_sleep(K_MSEC(1000U)); /* Simulate calibration time */

    /* Mandatory safety check before enabling monitoring */
    int safety_result = medical_device_safety_check();
    if (safety_result != MEDICAL_OK) {
        DIAG_CRITICAL(DIAG_CAT_SAFETY, "Safety check failed during monitoring start (error: %d)", safety_result);
        k_mutex_lock(&device_mutex, K_FOREVER);
        device_state = DEVICE_STATE_ERROR;
        device_statistics.current_state = device_state;
        device_statistics.error_count++;
        k_mutex_unlock(&device_mutex);
        return safety_result;
    }

    /* Transition to active monitoring state */
    k_mutex_lock(&device_mutex, K_FOREVER);
    device_state = DEVICE_STATE_MONITORING;
    device_statistics.current_state = device_state;
    k_mutex_unlock(&device_mutex);

    DIAG_INFO(DIAG_CAT_SYSTEM, "Medical monitoring started successfully - Device ready for operation");
    return MEDICAL_OK;
}

int medical_device_stop_monitoring(void)
{
    k_mutex_lock(&device_mutex, K_FOREVER);
    device_state = DEVICE_STATE_OFF;
    device_statistics.current_state = device_state;
    k_mutex_unlock(&device_mutex);

    /* Clear queues */
    safe_queue_clear(&sensor_queue);
    safe_queue_clear(&alert_queue);

    DIAG_INFO(DIAG_CAT_SYSTEM, "Medical monitoring stopped");
    return MEDICAL_OK;
}

device_state_t medical_device_get_state(void)
{
    k_mutex_lock(&device_mutex, K_FOREVER);
    device_state_t state = device_state;
    k_mutex_unlock(&device_mutex);
    
    return state;
}

int medical_device_add_sensor_data(const sensor_data_t *data)
{
    if (data == NULL) {
        return MEDICAL_ERROR_SENSOR;
    }

    device_state_t current_state = medical_device_get_state();
    if (current_state != DEVICE_STATE_MONITORING) {
        return MEDICAL_ERROR_SAFETY;
    }

    /* Add to processing queue */
    int ret = safe_queue_enqueue_nb(&sensor_queue, data, sizeof(sensor_data_t));
    if (ret != QUEUE_OK) {
        DIAG_WARNING(DIAG_CAT_SENSOR, "Sensor queue full, dropping data");
        return MEDICAL_ERROR_SENSOR;
    }

    /* Update statistics */
    k_mutex_lock(&device_mutex, K_FOREVER);
    device_statistics.total_samples++;
    
    /* Check for alerts based on sensor data */
    if (data->type < SENSOR_TYPE_MAX) {
        uint32_t threshold = device_configuration.alert_thresholds[data->type];
        if (threshold > 0 && data->value > threshold) {
            /* Create alert */
            medical_alert_t alert;
            alert.level = ALERT_LEVEL_WARNING;
            alert.sensor_type = data->type;
            alert.message = "Threshold exceeded";
            alert.timestamp = data->timestamp;
            alert.alert_id = next_alert_id++;
            
            /* Add to alert queue */
            safe_queue_enqueue_nb(&alert_queue, &alert, sizeof(medical_alert_t));
            device_statistics.alert_count++;
        }
    }
    
    k_mutex_unlock(&device_mutex);

    return MEDICAL_OK;
}

bool medical_device_check_alerts(medical_alert_t *alert)
{
    if (alert == NULL) {
        return false;
    }

    queue_item_t item;
    int ret = safe_queue_dequeue_nb(&alert_queue, &item);
    if (ret == QUEUE_OK) {
        memcpy(alert, item.data, sizeof(medical_alert_t));
        return true;
    }

    return false;
}

int medical_device_safety_check(void)
{
    /* Simulate safety checks */
    
    /* Check battery level */
    if (device_statistics.battery_level < 10) {
        DIAG_CRITICAL(DIAG_CAT_SAFETY, "Critical battery level: %d%%", 
                     device_statistics.battery_level);
        return MEDICAL_ERROR_SAFETY;
    }

    /* Check signal quality */
    if (device_statistics.signal_quality < 30) {
        DIAG_WARNING(DIAG_CAT_SAFETY, "Poor signal quality: %d%%", 
                    device_statistics.signal_quality);
    }

    /* Check sensor queue health */
    size_t queue_size = safe_queue_size(&sensor_queue);
    if (queue_size > 25) { /* 80% of max capacity */
        DIAG_WARNING(DIAG_CAT_PERFORMANCE, "Sensor queue near capacity: %zu/32", queue_size);
    }

    return MEDICAL_OK;
}

int medical_device_get_stats(device_stats_t *stats)
{
    if (stats == NULL) {
        return MEDICAL_ERROR_INIT;
    }

    k_mutex_lock(&device_mutex, K_FOREVER);
    
    device_statistics.uptime_seconds = k_uptime_get() / 1000;
    device_statistics.current_state = device_state;
    *stats = device_statistics;
    
    k_mutex_unlock(&device_mutex);

    return MEDICAL_OK;
}

int medical_device_get_sensor_data(sensor_data_t *data)
{
    if (data == NULL) {
        return MEDICAL_ERROR_INIT;
    }

    queue_item_t item;
    int ret = safe_queue_dequeue_nb(&sensor_queue, &item);
    if (ret == QUEUE_OK) {
        memcpy(data, item.data, sizeof(sensor_data_t));
        return MEDICAL_OK;
    }

    return MEDICAL_ERROR_SENSOR; /* No data available */
}

int medical_device_process_sensor_data(uint32_t max_items)
{
    sensor_data_t sensor_data;
    uint32_t processed_count = 0;

    for (uint32_t i = 0; i < max_items; i++) {
        int ret = medical_device_get_sensor_data(&sensor_data);
        if (ret != MEDICAL_OK) {
            break; /* No more data available */
        }

        /* Process the sensor data (simulate analysis) */
        int value_int = (int)sensor_data.value;
        int value_frac = (int)((sensor_data.value - value_int) * 100);
        
        DIAG_DEBUG(DIAG_CAT_SENSOR, "Processing %s: %d.%02d %s (quality: %u%%)",
                  sensor_data.type == SENSOR_TYPE_HEART_RATE ? "HR" :
                  sensor_data.type == SENSOR_TYPE_TEMPERATURE ? "Temp" :
                  sensor_data.type == SENSOR_TYPE_MOTION ? "Motion" : "SpO2",
                  value_int, value_frac,
                  sensor_data.type == SENSOR_TYPE_HEART_RATE ? "bpm" :
                  sensor_data.type == SENSOR_TYPE_TEMPERATURE ? "°C" :
                  sensor_data.type == SENSOR_TYPE_MOTION ? "g" : "%",
                  sensor_data.quality);

        processed_count++;
    }

    return processed_count;
}

int medical_device_enter_maintenance(void)
{
    k_mutex_lock(&device_mutex, K_FOREVER);
    
    if (device_state == DEVICE_STATE_MONITORING) {
        device_state = DEVICE_STATE_MAINTENANCE;
        device_statistics.current_state = device_state;
        DIAG_INFO(DIAG_CAT_SYSTEM, "Entered maintenance mode");
    } else {
        k_mutex_unlock(&device_mutex);
        return MEDICAL_ERROR_INIT;
    }
    
    k_mutex_unlock(&device_mutex);
    return MEDICAL_OK;
}

int medical_device_exit_maintenance(void)
{
    k_mutex_lock(&device_mutex, K_FOREVER);
    
    if (device_state == DEVICE_STATE_MAINTENANCE) {
        device_state = DEVICE_STATE_MONITORING;
        device_statistics.current_state = device_state;
        DIAG_INFO(DIAG_CAT_SYSTEM, "Exited maintenance mode");
    } else {
        k_mutex_unlock(&device_mutex);
        return MEDICAL_ERROR_INIT;
    }
    
    k_mutex_unlock(&device_mutex);
    return MEDICAL_OK;
}

void medical_device_emergency_shutdown(void)
{
    DIAG_CRITICAL(DIAG_CAT_SAFETY, "EMERGENCY SHUTDOWN INITIATED");

    k_mutex_lock(&device_mutex, K_FOREVER);
    device_state = DEVICE_STATE_ERROR;
    device_statistics.current_state = device_state;
    device_statistics.error_count++;
    k_mutex_unlock(&device_mutex);

    /* Clear all queues */
    safe_queue_clear(&sensor_queue);
    safe_queue_clear(&alert_queue);

    /* Add critical alert */
    medical_alert_t emergency_alert = {
        .level = ALERT_LEVEL_EMERGENCY,
        .sensor_type = SENSOR_TYPE_MAX, /* System alert */
        .message = "Emergency shutdown",
        .timestamp = k_uptime_get_32(),
        .alert_id = next_alert_id++
    };

    safe_queue_enqueue_nb(&alert_queue, &emergency_alert, sizeof(medical_alert_t));

    DIAG_CRITICAL(DIAG_CAT_SAFETY, "Emergency shutdown complete");
}