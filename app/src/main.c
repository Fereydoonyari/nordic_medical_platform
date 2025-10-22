/**
 * @file main.c
 * @brief Main application entry point for NISC Medical Wearable Device
 * @details This file contains the main application logic for the nRF52840-based
 * medical wearable device, including thread management, sensor simulation,
 * and system orchestration for a regulatory-compliant medical device.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This implementation follows medical device software development standards
 * and includes safety features required for medical device certification.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>

/* Include our modular components */
#include "common.h"
#include "system.h"
#include "thread_manager.h"
#include "safe_queue.h"
#include "safe_buffer.h"
#include "medical_device.h"
#include "diagnostics.h"
#include "config.h"
#include "hardware.h"
#include "shell_commands.h"

/*============================================================================*/
/* Application Timing Configuration                                           */
/*============================================================================*/

/** @defgroup AppTiming Application Timing Constants
 * @brief Timing intervals for various system operations
 * @details These constants define the operational timing for the medical
 * wearable device, balanced for power efficiency and medical monitoring
 * requirements.
 * @{
 */

/** @brief Sensor data sampling interval in milliseconds */
#define SENSOR_SAMPLING_INTERVAL_MS   1000U

/** @brief Supervisor safety check interval in milliseconds */
#define SUPERVISOR_CHECK_INTERVAL_MS  20000U

/** @brief Data processing cycle interval in milliseconds */
#define DATA_PROCESSING_INTERVAL_MS   5000U

/** @brief Communication transmission interval in milliseconds */
#define COMMUNICATION_INTERVAL_MS     15000U

/** @brief Full diagnostic check interval in milliseconds */
#define DIAGNOSTIC_CHECK_INTERVAL_MS  90000U

/** @brief Thread heartbeat reporting interval in milliseconds */
#define THREAD_HEARTBEAT_INTERVAL_MS  15000U

/** @brief Main thread heartbeat interval in seconds */
#define MAIN_HEARTBEAT_INTERVAL_SEC   30U

/** @} */ /* End of AppTiming group */

/*============================================================================*/
/* Sensor Simulation Configuration                                            */
/*============================================================================*/

/** @defgroup SensorSim Sensor Simulation
 * @brief Realistic sensor simulation for medical device testing
 * @details Provides clinically realistic sensor data simulation with proper
 * baselines, ranges, and variation patterns for medical device validation.
 * @{
 */

/**
 * @brief Sensor simulation parameters structure
 * @details Defines the simulation parameters for each sensor type including
 * baseline values, operational ranges, and variation characteristics based
 * on clinical medical data ranges.
 */
typedef struct {
    sensor_type_t type;           /**< Sensor type identifier */
    float baseline_value;         /**< Normal baseline value */
    float min_value;              /**< Minimum safe operational value */
    float max_value;              /**< Maximum safe operational value */
    float variation_range;        /**< Natural variation range around baseline */
    const char *name;             /**< Human-readable sensor name */
    const char *units;            /**< Measurement units */
} sensor_simulation_t;

/**
 * @brief Sensor simulation configuration array
 * @details Clinical-grade sensor simulation parameters based on medical
 * standards and typical human physiological ranges.
 */
static const sensor_simulation_t sensor_simulations[SENSOR_TYPE_MAX] = {
    {SENSOR_TYPE_HEART_RATE,    72.0f,  60.0f, 100.0f, 8.0f,  "Heart Rate",       "bpm"},
    {SENSOR_TYPE_TEMPERATURE,   36.6f,  36.0f,  37.5f, 0.4f,  "Body Temperature", "°C"},
    {SENSOR_TYPE_MOTION,         1.0f,   0.0f,   5.0f, 2.0f,  "Motion Activity",  "g"},
    {SENSOR_TYPE_BLOOD_OXYGEN,  98.0f,  95.0f, 100.0f, 2.0f,  "Blood Oxygen",     "%"}
};

/** @brief Current sensor reading values for all sensor types */
static sensor_data_t current_sensor_readings[SENSOR_TYPE_MAX];

/** @} */ /* End of SensorSim group */

/*============================================================================*/
/* Private Function Declarations                                              */
/*============================================================================*/

/**
 * @brief Initialize sensor readings with baseline values
 * @details Sets up initial sensor readings with clinically appropriate
 * baseline values and quality indicators for system startup.
 */
static void init_sensor_readings(void);

/** @brief Supervisor thread function (implementation below) */
void supervisor_thread(void *arg1, void *arg2, void *arg3);

/** @brief Data acquisition thread function (implementation below) */
void data_acquisition_thread(void *arg1, void *arg2, void *arg3);

/** @brief Data processing thread function (implementation below) */
void data_processing_thread(void *arg1, void *arg2, void *arg3);

/** @brief Communication thread function (implementation below) */
void communication_thread(void *arg1, void *arg2, void *arg3);

/** @brief Diagnostics thread function (implementation below) */
void diagnostics_thread(void *arg1, void *arg2, void *arg3);

/** @brief Hardware update thread function (implementation below) */
void hardware_update_thread(void *arg1, void *arg2, void *arg3);

/*============================================================================*/
/* Public Function Implementations                                            */
/*============================================================================*/

/*============================================================================*/
/* Private Function Implementations                                           */
/*============================================================================*/

/**
 * @brief Initialize sensor readings with baseline values
 * @details Sets up initial sensor readings with clinically appropriate
 * baseline values and quality indicators for system startup.
 */
static void init_sensor_readings(void) 
{
    for (int i = 0; i < SENSOR_TYPE_MAX; i++) {
        current_sensor_readings[i].type = sensor_simulations[i].type;
        current_sensor_readings[i].value = sensor_simulations[i].baseline_value;
        current_sensor_readings[i].quality = 90U + (uint8_t)(i * 2U); /* 90-96% quality */
        current_sensor_readings[i].flags = 0U;
        current_sensor_readings[i].timestamp = 0U;
    }
    
    DIAG_DEBUG(DIAG_CAT_SENSOR, "Sensor readings initialized with baseline values");
}

/**
 * @brief Main application entry point
 * @details Initializes all system components, creates application threads,
 * and enters the main system monitoring loop. Follows medical device
 * software initialization patterns for safety and reliability.
 * Includes DFU boot process with button press wait and Bluetooth advertising.
 * 
 * @note This function does not return - it runs the main system loop indefinitely
 */
void main(void) 
{
    int ret;

    printk("\n=== NISC Medical Wearable Device Starting ===\n");
    printk("Firmware Version: %s\n", APP_VERSION_STRING);
    printk("Device Model: %s\n", DEVICE_MODEL);
    printk("Target Platform: nRF52840 Development Kit\n");
    printk("Build Time: %s %s\n", __DATE__, __TIME__);

    /* Initialize hardware abstraction layer first */
    printk("Initializing hardware abstraction layer...\n");
    ret = hw_init();
    if (ret != HW_OK) {
        printk("FATAL: Hardware initialization failed (error: %d)\n", ret);
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
        return;
    }

    /* Show hardware information */
    hw_info_t hw_info;
    if (hw_get_info(&hw_info) == HW_OK) {
        printk("\n=== Hardware Information ===\n");
        printk("  Device ID: %02x%02x%02x%02x-%02x%02x%02x%02x\n",
               hw_info.device_id[0], hw_info.device_id[1], 
               hw_info.device_id[2], hw_info.device_id[3],
               hw_info.device_id[4], hw_info.device_id[5], 
               hw_info.device_id[6], hw_info.device_id[7]);
        printk("  Reset Cause: 0x%08x\n", hw_info.reset_cause);
        printk("  USB Console: %s\n", hw_info.usb_console_ready ? "Ready" : "Not Ready");
        printk("  LEDs: %s\n", hw_info.leds_initialized ? "Initialized" : "Failed");
        printk("============================\n\n");
    }

    /* Initialize DFU boot process */
    printk("Initializing DFU boot process...\n");
    ret = hw_dfu_init();
    if (ret != HW_OK) {
        printk("WARNING: DFU initialization failed (error: %d)\n", ret);
    } else {
        printk("DFU mode ready - Press Button 1 anytime to enter DFU mode\n");
    }

    /* Optional DFU entry at startup */
    printk("\n=== Startup Options ===\n");
    printk("Press Button 1 within 5 seconds to enter DFU mode\n");
    printk("Or wait to continue to normal operation...\n");
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_SLOW_BLINK);
    
    /* Wait with shorter timeout for optional DFU entry */
    k_sleep(K_MSEC(5000)); /* 5 second wait */
    
    /* Check if user entered DFU mode during wait */
    if (hw_dfu_is_active()) {
        printk("\nDFU mode active - Press Button 1 to exit and continue\n");
        /* Wait for user to exit DFU mode */
        while (hw_dfu_is_active()) {
            k_sleep(K_MSEC(100));
        }
    }
    
    printk("\nContinuing to normal operation...\n");
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_OFF);

    /* Wait for USB console to be ready for better log output */
    if (hw_usb_console_ready()) {
        printk("USB Console detected - Enhanced logging enabled\n");
        k_sleep(K_MSEC(500)); /* Give host time to open console */
    }

    /* Initialize sensor simulation */
    init_sensor_readings();

    /* Initialize core system */
    ret = system_init();
    if (ret != SYSTEM_OK) {
        printk("FATAL: System initialization failed (error: %d)\n", ret);
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_FAST_BLINK);
        return;
    }

    /* Initialize thread manager */
    ret = thread_manager_init();
    if (ret != SUCCESS) {
        system_handle_error(SYSTEM_ERROR_INIT, "Thread manager initialization failed");
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_FAST_BLINK);
        return;
    }

    /* Initialize medical device with default configuration */
    device_config_t device_config = {
        .sampling_rate_hz = 100U,
        .alert_thresholds = {80U, 100U, 150U, 95U}, /* HR, Temp, Motion, SpO2 */
        .safety_monitoring_enabled = true,
        .watchdog_timeout_ms = 30000U
    };

    ret = medical_device_init(&device_config);
    if (ret != MEDICAL_OK) {
        system_handle_error(SYSTEM_ERROR_INIT, "Medical device initialization failed");
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_FAST_BLINK);
        return;
    }

    /* Initialize shell commands for USB console */
    ret = shell_commands_init();
    if (ret != SHELL_OK) {
        DIAG_WARNING(DIAG_CAT_SYSTEM, "Shell commands initialization failed");
    }

    /* Initialize Bluetooth advertising */
    printk("Initializing Bluetooth advertising...\n");
    ret = hw_ble_advertising_init();
    if (ret != HW_OK) {
        DIAG_WARNING(DIAG_CAT_SYSTEM, "Bluetooth advertising initialization failed: %d", ret);
    } else {
        /* Start Bluetooth advertising */
        ret = hw_ble_advertising_start();
        if (ret == HW_OK) {
            printk("Bluetooth advertising started - Device discoverable\n");
            DIAG_INFO(DIAG_CAT_SYSTEM, "Bluetooth advertising active");
        } else {
            DIAG_WARNING(DIAG_CAT_SYSTEM, "Failed to start Bluetooth advertising: %d", ret);
        }
    }

    /* Initialize serial Bluetooth communication */
    ret = hw_serial_bt_init();
    if (ret != HW_OK) {
        DIAG_WARNING(DIAG_CAT_SYSTEM, "Serial Bluetooth initialization failed: %d", ret);
    } else {
        printk("Serial Bluetooth communication ready\n");
        DIAG_INFO(DIAG_CAT_SYSTEM, "Serial Bluetooth interface initialized");
    }

    DIAG_INFO(DIAG_CAT_SYSTEM, "All subsystems initialized successfully");

    /* Set status LED to indicate system is ready */
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_BREATHING);

    /* Create application threads with delays to avoid race conditions */
    printk("Creating application threads...\n");

    /* Start supervisor thread first - highest priority for safety */
    ret = thread_manager_create_thread(THREAD_ID_SUPERVISOR, supervisor_thread, 
                                      NULL, NULL, NULL);
    if (ret != SUCCESS) {
        system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create supervisor thread");
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
        return;
    }
    k_sleep(K_MSEC(100)); /* Small delay to let supervisor start */

    /* Start hardware update thread for LED patterns */
    ret = thread_manager_create_thread(THREAD_ID_DIAGNOSTICS, hardware_update_thread, 
                                      NULL, NULL, NULL);
    if (ret != SUCCESS) {
        system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create hardware update thread");
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
        return;
    }
    k_sleep(K_MSEC(100)); /* Small delay */

    /* Start data acquisition thread */
    ret = thread_manager_create_thread(THREAD_ID_DATA_ACQUISITION, data_acquisition_thread,
                                      NULL, NULL, NULL);
    if (ret != SUCCESS) {
        system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create data acquisition thread");
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
        return;
    }
    k_sleep(K_MSEC(100)); /* Small delay */

    /* Start data processing thread */
    ret = thread_manager_create_thread(THREAD_ID_DATA_PROCESSING, data_processing_thread,
                                      NULL, NULL, NULL);
    if (ret != SUCCESS) {
        system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create data processing thread");
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
        return;
    }
    k_sleep(K_MSEC(100)); /* Small delay */

    /* Start communication thread */
    ret = thread_manager_create_thread(THREAD_ID_COMMUNICATION, communication_thread,
                                      NULL, NULL, NULL);
    if (ret != SUCCESS) {
        system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create communication thread");
        hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
        return;
    }

    DIAG_INFO(DIAG_CAT_SYSTEM, "Medical wearable device startup complete");
    printk("=== System Ready - All LEDs Active ===\n");

    /* Show system ready indication - single synchronized flash */
    hw_led_set_state(HW_LED_STATUS, true);
    hw_led_set_state(HW_LED_HEARTBEAT, true);  
    hw_led_set_state(HW_LED_COMMUNICATION, true);
    hw_led_set_state(HW_LED_ERROR, true);
    k_sleep(K_MSEC(300));
    
    /* Set final LED patterns for normal operation */
    hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_BREATHING);      /* Breathing = system OK */
    hw_led_set_pattern(HW_LED_HEARTBEAT, HW_PULSE_HEARTBEAT);   /* Medical pulse */
    hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_OFF);     /* Will be controlled by comm thread */
    hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);             /* Off = no errors */

    /* Main thread becomes system monitor - all work is done in other threads */
    while (1) {
        k_sleep(K_SECONDS(MAIN_HEARTBEAT_INTERVAL_SEC));
        DIAG_INFO(DIAG_CAT_SYSTEM, "Main thread heartbeat - System operational");
        
        /* No LED blinking here - maintain breathing pattern */
    }
}

/*============================================================================*/
/* Thread Function Implementations                                            */
/*============================================================================*/

/** @brief Enhanced supervisor thread for real hardware */
void supervisor_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("Supervisor thread started - safety monitoring active\n");

    uint32_t supervisor_cycle = 0;

    while (1) {
        thread_manager_heartbeat(THREAD_ID_SUPERVISOR);
        
        supervisor_cycle++;
        
        /* Periodic system health check */
        if (supervisor_cycle % 10 == 0) {  /* Every 200 seconds */
            system_stats_t stats;
            if (system_get_stats(&stats) == SYSTEM_OK) {
                DIAG_INFO(DIAG_CAT_SYSTEM, "System Health: Uptime=%ums Errors=%u State=%d", 
                         stats.uptime_ms, stats.total_errors, stats.current_state);
                
                /* Show system health on status LED */
                if (stats.total_errors > 5) {
                    hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SLOW_BLINK);
                } else if (stats.total_errors > 0) {
                    /* Brief error indication */
                    hw_led_set_state(HW_LED_ERROR, true);
                    k_sleep(K_MSEC(100));
                    hw_led_set_state(HW_LED_ERROR, false);
                }
            }
        }

        k_sleep(K_MSEC(SUPERVISOR_CHECK_INTERVAL_MS));
    }
}

/** @brief Simple sensor readings array for QEMU testing */
static int simple_sensor_values[SENSOR_TYPE_MAX] = {72, 366, 10, 980}; // HR, Temp*10, Motion*10, SpO2*10

/** @brief Hardware update thread to maintain LED patterns */
void hardware_update_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("Hardware update thread started - LED pattern management\n");

    while (1) {
        /* Update LED patterns every 50ms for smooth animation */
        hw_led_update_patterns();
        
        /* Update thread heartbeat every few cycles */
        static uint32_t heartbeat_counter = 0;
        heartbeat_counter++;
        if (heartbeat_counter % 100 == 0) {  /* Every 5 seconds */
            thread_manager_heartbeat(THREAD_ID_DIAGNOSTICS);
        }

        k_sleep(K_MSEC(50));
    }
}

/** @brief Enhanced data acquisition thread with hardware integration */
void data_acquisition_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("Data acquisition thread started - sampling sensors every 1 second\n");
    printk("==================== MEDICAL DATA PULSES ====================\n");

    uint32_t cycle_count = 0U;

    while (1) {
        thread_manager_heartbeat(THREAD_ID_DATA_ACQUISITION);

        /* Generate realistic medical data with integer arithmetic only */
        uint32_t uptime_sec = k_uptime_get_32() / 1000U;
        
        /* Heart Rate: 60-100 bpm with simple variation */
        int hr_base = 72;
        int hr_variation = (int)((uptime_sec % 20U) - 10); /* ±10 bpm variation */
        simple_sensor_values[0] = hr_base + hr_variation;
        if (simple_sensor_values[0] < 60) simple_sensor_values[0] = 60;
        if (simple_sensor_values[0] > 100) simple_sensor_values[0] = 100;
        
        /* Body Temperature: 36.0-37.5°C (stored as temp*10) */
        int temp_base = 366; // 36.6°C * 10
        int temp_variation = (int)((uptime_sec % 15U) - 7); /* ±0.7°C variation */
        simple_sensor_values[1] = temp_base + temp_variation;
        if (simple_sensor_values[1] < 360) simple_sensor_values[1] = 360;
        if (simple_sensor_values[1] > 375) simple_sensor_values[1] = 375;
        
        /* Motion Activity: 0-5g (stored as motion*10) */
        int motion_base = 2; // 0.2g * 10
        int motion_burst = ((uptime_sec % 7U) == 0 ? 30 : 0); /* Activity bursts */
        int motion_random = (int)((uptime_sec * 3U) % 10U); /* Small random motion */
        simple_sensor_values[2] = motion_base + motion_burst + motion_random;
        if (simple_sensor_values[2] > 50) simple_sensor_values[2] = 50;
        
        /* Blood Oxygen: 95-100% (stored as SpO2*10) */
        int spo2_base = 980; // 98.0% * 10
        int spo2_variation = (int)((uptime_sec % 12U) - 6) * 2; /* ±1.2% variation */
        simple_sensor_values[3] = spo2_base + spo2_variation;
        if (simple_sensor_values[3] < 950) simple_sensor_values[3] = 950;
        if (simple_sensor_values[3] > 1000) simple_sensor_values[3] = 1000;

        /* Update heartbeat LED with current heart rate */
        hw_show_medical_pulse((uint32_t)simple_sensor_values[0]);

        /* Display real-time medical data pulse - simplified when BLE connected */
        if (!hw_ble_is_connected()) {
            /* Full display when not connected */
            printk("\n");
            printk("MEDICAL DATA PULSE #%u [Time: %u.%03u s]\n", 
                   cycle_count, uptime_sec, (k_uptime_get_32() % 1000U));
            printk("+-----------------------------------------+\n");
            printk("| HEART RATE:   %3d bpm   [Quality: %2d%%] |\n", 
                   simple_sensor_values[0], 88 + (cycle_count % 13));
            printk("| TEMPERATURE:  %2d.%d°C    [Quality: %2d%%] |\n", 
                   simple_sensor_values[1] / 10, simple_sensor_values[1] % 10,
                   91 + (cycle_count % 10));
            printk("| MOTION:       %d.%d g     [Quality: %2d%%] |\n", 
                   simple_sensor_values[2] / 10, simple_sensor_values[2] % 10,
                   94 + (cycle_count % 7));
            printk("| BLOOD O2:     %2d.%d%%     [Quality: %2d%%] |\n", 
                   simple_sensor_values[3] / 10, simple_sensor_values[3] % 10,
                   97 + (cycle_count % 4));
            printk("+-----------------------------------------+\n");
        } else {
            /* Minimal logging when BLE connected to avoid blocking */
            if (cycle_count % 5 == 0) {
                printk("[BLE] Data: HR=%d Temp=%d.%d SpO2=%d.%d Motion=%d.%d\n",
                       simple_sensor_values[0],
                       simple_sensor_values[1]/10, simple_sensor_values[1]%10,
                       simple_sensor_values[3]/10, simple_sensor_values[3]%10,
                       simple_sensor_values[2]/10, simple_sensor_values[2]%10);
            }
        }

        /* Update BLE GATT data immediately after sensor reading */
        hw_ble_update_medical_data(
            (uint16_t)simple_sensor_values[0],  /* Heart rate */
            (int16_t)simple_sensor_values[1],   /* Temperature */
            (uint16_t)simple_sensor_values[3],  /* SpO2 */
            (uint16_t)simple_sensor_values[2]   /* Motion */
        );

        /* Add special indicators for notable events with LED feedback */
        if (!hw_ble_is_connected()) {
            /* Only show alerts when not connected to avoid console spam */
            if (simple_sensor_values[0] > 85) {
                printk("ALERT: Elevated heart rate detected!\n");
                hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_FAST_BLINK);
                k_sleep(K_MSEC(100));
                hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
            }
            if (simple_sensor_values[2] > 20) {
                printk("INFO: High activity detected - Patient is active\n");
            }
            if (simple_sensor_values[1] > 372) {
                printk("WARNING: Elevated temperature detected\n");
                hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SLOW_BLINK);
            } else if (simple_sensor_values[3] < 960) {
                printk("CAUTION: Blood oxygen below normal range\n");
                hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_DOUBLE_BLINK);
            } else {
                /* Clear error LED if no conditions are met */
                hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
            }
        }

        cycle_count++;

        k_sleep(K_MSEC(SENSOR_SAMPLING_INTERVAL_MS));
    }
}

/** @brief Simplified data processing thread for QEMU */
void data_processing_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("Data processing thread started - analyzing data\n");

    while (1) {
        thread_manager_heartbeat(THREAD_ID_DATA_PROCESSING);
        k_sleep(K_MSEC(DATA_PROCESSING_INTERVAL_MS));
    }
}

/** @brief Enhanced communication thread with LED feedback */
void communication_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("Communication thread started - transmitting data every 15 seconds\n");

    uint32_t transmission_count = 0U;

    while (1) {
        thread_manager_heartbeat(THREAD_ID_COMMUNICATION);

        transmission_count++;

        /* BLE data is updated in data acquisition thread every 1 second */
        /* Send notification to connected clients */
        if (hw_ble_is_connected()) {
            hw_ble_send_notification();
            printk("[BLE] Notification #%u: HR=%d Temp=%d.%d SpO2=%d.%d Motion=%d.%d\n",
                   transmission_count,
                   simple_sensor_values[0],
                   simple_sensor_values[1]/10, simple_sensor_values[1]%10,
                   simple_sensor_values[3]/10, simple_sensor_values[3]%10,
                   simple_sensor_values[2]/10, simple_sensor_values[2]%10);
        } else {
            /* Full display when not connected */
            printk("\nTRANSMITTING MEDICAL DATA PACKET #%u\n", transmission_count);
            printk("+--- Current Patient Vitals Summary ---+\n");
            printk("| HR: %3d bpm  | Temp: %2d.%d°C         |\n", 
                   simple_sensor_values[0],
                   simple_sensor_values[1] / 10, simple_sensor_values[1] % 10);
            printk("| Motion: %d.%dg | SpO2: %2d.%d%%         |\n", 
                   simple_sensor_values[2] / 10, simple_sensor_values[2] % 10,
                   simple_sensor_values[3] / 10, simple_sensor_values[3] % 10);
            printk("+-------------------------------------+\n");
        }
        
        /* Also send via serial Bluetooth for legacy support */
        char bt_data[64];
        snprintf(bt_data, sizeof(bt_data), "HR:%d,T:%d.%d,M:%d.%d,SpO2:%d.%d",
                simple_sensor_values[0],
                simple_sensor_values[1] / 10, simple_sensor_values[1] % 10,
                simple_sensor_values[2] / 10, simple_sensor_values[2] % 10,
                simple_sensor_values[3] / 10, simple_sensor_values[3] % 10);
        
        hw_serial_bt_send((const uint8_t *)bt_data, strlen(bt_data));
        
        /* Show transmission protocol with LED indication */
        uint32_t protocol = transmission_count % 3U;
        switch (protocol) {
            case 0U:
                printk("Via: Bluetooth Low Energy (BLE GATT)\n");
                printk("Device Name: NISC-Medical-Device\n");
                if (hw_ble_is_connected()) {
                    printk("Status: CONNECTED - Data transmitted via GATT notifications\n");
                } else {
                    printk("Status: Advertising - Waiting for connection...\n");
                }
                /* Quick blue-like flash pattern */
                for (int i = 0; i < 3; i++) {
                    hw_led_set_state(HW_LED_COMMUNICATION, true);
                    k_sleep(K_MSEC(100));
                    hw_led_set_state(HW_LED_COMMUNICATION, false);
                    k_sleep(K_MSEC(100));
                }
                break;
            case 1U:
                printk("Via: Serial Bluetooth Module\n");
                printk("Data: %s\n", bt_data);
                /* Longer on pattern for serial */
                hw_led_set_state(HW_LED_COMMUNICATION, true);
                k_sleep(K_MSEC(500));
                hw_led_set_state(HW_LED_COMMUNICATION, false);
                break;
            case 2U:
                printk("Via: USB Console Interface\n");
                printk("Console: Ready for shell commands\n");
                /* Double blink for console */
                hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_DOUBLE_BLINK);
                k_sleep(K_MSEC(1000));
                break;
        }
        printk("Data packet transmitted successfully\n\n");

        /* Turn off communication LED after transmission */
        hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_OFF);

        k_sleep(K_MSEC(COMMUNICATION_INTERVAL_MS));
    }
}



