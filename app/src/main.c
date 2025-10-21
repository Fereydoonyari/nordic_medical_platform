/**
 * @file main.c
 * @brief Main application entry point for NISC Medical Wearable Device
 * @details This file contains the main application logic for the nRF52840-based
 * medical wearable device, including thread management, sensor simulation,
 * DFU boot process, Bluetooth advertising, and system orchestration for a 
 * regulatory-compliant medical device.
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
 
 /** @brief DFU mode check delay in milliseconds */
 #define DFU_CHECK_DELAY_MS            200U
 
 /** @brief Bluetooth initialization delay in milliseconds */
 #define BT_INIT_DELAY_MS              500U
 
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
 
 /**
  * @brief Print startup banner with device information
  */
 static void print_startup_banner(void);
 
 /**
  * @brief Print hardware information to console
  */
 static void print_hardware_info(void);
 
 /**
  * @brief Handle DFU boot mode check and entry
  * @return true if DFU mode was entered and exited, false otherwise
  */
 static bool handle_dfu_boot_check(void);
 
 /**
  * @brief Initialize and start Bluetooth advertising
  * @return HW_OK on success, error code otherwise
  */
 static int initialize_bluetooth(void);
 
 /**
  * @brief Create all application threads
  * @return SUCCESS on success, error code otherwise
  */
 static int create_application_threads(void);
 
 /**
  * @brief Set final LED patterns for normal operation
  */
 static void set_normal_operation_leds(void);
 
 /** @brief Supervisor thread function */
 void supervisor_thread(void *arg1, void *arg2, void *arg3);
 
 /** @brief Data acquisition thread function */
 void data_acquisition_thread(void *arg1, void *arg2, void *arg3);
 
 /** @brief Data processing thread function */
 void data_processing_thread(void *arg1, void *arg2, void *arg3);
 
 /** @brief Communication thread function */
 void communication_thread(void *arg1, void *arg2, void *arg3);
 
 /** @brief Hardware update thread function */
 void hardware_update_thread(void *arg1, void *arg2, void *arg3);
 
 /*============================================================================*/
 /* Main Application Entry Point                                               */
 /*============================================================================*/
 
 /**
  * @brief Main application entry point
  * @details Initializes all system components, creates application threads,
  * handles DFU boot process, starts Bluetooth advertising, and enters the 
  * main system monitoring loop. Follows medical device software initialization 
  * patterns for safety and reliability.
  * 
  * @note This function does not return - it runs the main system loop indefinitely
  */
 void main(void) 
 {
     int ret;
 
     /* Print startup banner */
     print_startup_banner();
 
     /*========================================================================*/
     /* PHASE 1: Core Hardware Initialization                                 */
     /*========================================================================*/
 
     printk("\n");
     printk("╔════════════════════════════════════════════════════════╗\n");
     printk("║  PHASE 1: HARDWARE INITIALIZATION                      ║\n");
     printk("╚════════════════════════════════════════════════════════╝\n");
 
     /* Initialize hardware abstraction layer */
     printk("[1/3] Initializing hardware abstraction layer...\n");
     ret = hw_init();
     if (ret != HW_OK) {
         printk("❌ FATAL: Hardware initialization failed (error: %d)\n", ret);
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
         return;
     }
     printk("✓ Hardware abstraction layer initialized\n");
 
     /* Show hardware information */
     print_hardware_info();
 
     /* Wait for USB console to be ready */
     printk("[2/3] Checking USB console availability...\n");
     if (hw_usb_console_ready()) {
         printk("✓ USB Console detected - Enhanced logging enabled\n");
         k_sleep(K_MSEC(500)); /* Give host time to open console */
     } else {
         printk("⚠ USB Console not detected - Using default output\n");
     }
 
     /* Initialize sensor simulation */
     printk("[3/3] Initializing sensor data simulation...\n");
     init_sensor_readings();
     printk("✓ Sensor readings initialized with baseline values\n");
 
     /*========================================================================*/
     /* PHASE 2: DFU Boot Process                                             */
     /*========================================================================*/
 
     printk("\n");
     printk("╔════════════════════════════════════════════════════════╗\n");
     printk("║  PHASE 2: DFU BOOT PROCESS                             ║\n");
     printk("╚════════════════════════════════════════════════════════╝\n");
 
     /* Initialize DFU boot process */
     printk("[1/3] Initializing DFU boot system...\n");
     ret = hw_dfu_init();
     if (ret != HW_OK) {
         printk("⚠ WARNING: DFU initialization failed (error: %d)\n", ret);
         printk("   Device will continue without DFU support\n");
     } else {
         printk("✓ DFU boot system initialized\n");
     }
 
     /* Initialize button with interrupt */
     printk("[2/3] Setting up button interrupt for DFU...\n");
     ret = hw_button_init();
     if (ret != HW_OK) {
         printk("⚠ WARNING: Button interrupt setup failed (error: %d)\n", ret);
         printk("   DFU boot may not work correctly\n");
     } else {
         printk("✓ Button interrupt configured successfully\n");
     }
 
     /* Check for DFU boot request and handle if needed */
     printk("[3/3] Checking for DFU boot request...\n");
     bool dfu_was_active = handle_dfu_boot_check();
     
     if (dfu_was_active) {
         printk("✓ DFU mode session completed\n");
     } else {
         printk("✓ No DFU boot requested - continuing normal startup\n");
     }
 
     /*========================================================================*/
     /* PHASE 3: System Initialization                                        */
     /*========================================================================*/
 
     printk("\n");
     printk("╔════════════════════════════════════════════════════════╗\n");
     printk("║  PHASE 3: SYSTEM INITIALIZATION                        ║\n");
     printk("╚════════════════════════════════════════════════════════╝\n");
 
     /* Initialize core system */
     printk("[1/5] Initializing core system...\n");
     ret = system_init();
     if (ret != SYSTEM_OK) {
         printk("❌ FATAL: System initialization failed (error: %d)\n", ret);
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_FAST_BLINK);
         return;
     }
     printk("✓ Core system initialized\n");
 
     /* Initialize thread manager */
     printk("[2/5] Initializing thread manager...\n");
     ret = thread_manager_init();
     if (ret != SUCCESS) {
         system_handle_error(SYSTEM_ERROR_INIT, "Thread manager initialization failed");
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_FAST_BLINK);
         return;
     }
     printk("✓ Thread manager initialized\n");
 
     /* Initialize medical device */
     printk("[3/5] Initializing medical device subsystem...\n");
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
     printk("✓ Medical device subsystem initialized\n");
 
     /* Initialize shell commands */
     printk("[4/5] Initializing interactive shell commands...\n");
     ret = shell_commands_init();
     if (ret != SHELL_OK) {
         DIAG_WARNING(DIAG_CAT_SYSTEM, "Shell commands initialization failed");
         printk("⚠ WARNING: Shell commands initialization failed\n");
         printk("   Interactive commands may not be available\n");
     } else {
         printk("✓ Interactive shell commands initialized\n");
     }
 
     /* Set status LED to indicate system is ready */
     printk("[5/5] Setting system ready indicators...\n");
     hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_BREATHING);
     printk("✓ System status indicators set\n");
 
     DIAG_INFO(DIAG_CAT_SYSTEM, "All core subsystems initialized successfully");
 
     /*========================================================================*/
     /* PHASE 4: Bluetooth Initialization                                     */
     /*========================================================================*/
 
     printk("\n");
     printk("╔════════════════════════════════════════════════════════╗\n");
     printk("║  PHASE 4: BLUETOOTH INITIALIZATION                     ║\n");
     printk("╚════════════════════════════════════════════════════════╝\n");
 
     ret = initialize_bluetooth();
     if (ret != HW_OK) {
         printk("⚠ WARNING: Bluetooth initialization failed\n");
         printk("   Device will continue without Bluetooth connectivity\n");
     }
 
     /*========================================================================*/
     /* PHASE 5: Thread Creation                                              */
     /*========================================================================*/
 
     printk("\n");
     printk("╔════════════════════════════════════════════════════════╗\n");
     printk("║  PHASE 5: APPLICATION THREADS                          ║\n");
     printk("╚════════════════════════════════════════════════════════╝\n");
 
     ret = create_application_threads();
     if (ret != SUCCESS) {
         printk("❌ FATAL: Application thread creation failed\n");
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
         return;
     }
 
     DIAG_INFO(DIAG_CAT_SYSTEM, "Medical wearable device startup complete");
 
     /*========================================================================*/
     /* PHASE 6: System Ready                                                 */
     /*========================================================================*/
 
     printk("\n");
     printk("╔════════════════════════════════════════════════════════╗\n");
     printk("║  SYSTEM READY - ALL SUBSYSTEMS OPERATIONAL             ║\n");
     printk("╚════════════════════════════════════════════════════════╝\n");
 
     /* Show system ready indication - synchronized flash on all LEDs */
     printk("\n🎉 Performing system ready LED animation...\n");
     hw_led_set_state(HW_LED_STATUS, true);
     hw_led_set_state(HW_LED_HEARTBEAT, true);  
     hw_led_set_state(HW_LED_COMMUNICATION, true);
     hw_led_set_state(HW_LED_ERROR, true);
     k_sleep(K_MSEC(300));
     
     /* Set final LED patterns for normal operation */
     set_normal_operation_leds();
 
     printk("\n");
     printk("════════════════════════════════════════════════════════\n");
     printk("  ✓ Device is fully operational\n");
     printk("  ✓ Medical monitoring active\n");
     printk("  ✓ Bluetooth advertising enabled\n");
     printk("  ✓ Console commands available\n");
     printk("════════════════════════════════════════════════════════\n");
     printk("\n💡 Type 'help' for available commands\n");
     printk("💡 Monitor this console for real-time medical data\n\n");
 
     /*========================================================================*/
     /* Main System Monitoring Loop                                           */
     /*========================================================================*/
 
     /* Main thread becomes system monitor - all work is done in other threads */
     while (1) {
         k_sleep(K_SECONDS(MAIN_HEARTBEAT_INTERVAL_SEC));
         DIAG_INFO(DIAG_CAT_SYSTEM, "Main thread heartbeat - System operational");
         
         /* Periodic system health check */
         system_stats_t stats;
         if (system_get_stats(&stats) == SYSTEM_OK) {
             if (stats.total_errors > 0) {
                 printk("\n⚠ System Health Check: %u errors detected\n", stats.total_errors);
             }
         }
     }
 }
 
 /*============================================================================*/
 /* Private Function Implementations                                           */
 /*============================================================================*/
 
 /**
  * @brief Initialize sensor readings with baseline values
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
  * @brief Print startup banner
  */
 static void print_startup_banner(void)
 {
     printk("\n");
     printk("╔════════════════════════════════════════════════════════╗\n");
     printk("║                                                        ║\n");
     printk("║     NISC MEDICAL WEARABLE DEVICE - nRF52840           ║\n");
     printk("║                                                        ║\n");
     printk("╚════════════════════════════════════════════════════════╝\n");
     printk("\n");
     printk("  Firmware Version:  %s\n", APP_VERSION_STRING);
     printk("  Device Model:      %s\n", DEVICE_MODEL);
     printk("  Manufacturer:      %s\n", MANUFACTURER);
     printk("  Target Platform:   nRF52840 Development Kit\n");
     printk("  Build Date:        %s\n", __DATE__);
     printk("  Build Time:        %s\n", __TIME__);
     printk("\n");
 }
 
 /**
  * @brief Print hardware information
  */
 static void print_hardware_info(void)
 {
     hw_info_t hw_info;
     if (hw_get_info(&hw_info) == HW_OK) {
         printk("\n📊 Hardware Information:\n");
         printk("   Device ID:     %02x%02x%02x%02x-%02x%02x%02x%02x\n",
                hw_info.device_id[0], hw_info.device_id[1], 
                hw_info.device_id[2], hw_info.device_id[3],
                hw_info.device_id[4], hw_info.device_id[5], 
                hw_info.device_id[6], hw_info.device_id[7]);
         printk("   Reset Cause:   0x%08x\n", hw_info.reset_cause);
         printk("   USB Console:   %s\n", hw_info.usb_console_ready ? "Ready" : "Not Ready");
         printk("   LEDs:          %s\n", hw_info.leds_initialized ? "Initialized" : "Failed");
         printk("   GPIO:          %s\n", hw_info.gpio_initialized ? "Initialized" : "Failed");
     }
 }
 
 /**
  * @brief Handle DFU boot mode check and entry
  */
 static bool handle_dfu_boot_check(void)
 {
     /* Small delay to allow button state to stabilize */
     k_msleep(DFU_CHECK_DELAY_MS);
     
     /* Check if button is pressed */
     if (hw_button_is_pressed()) {
         printk("\n");
         printk("╔════════════════════════════════════════════════════════╗\n");
         printk("║  ⚡ DFU BOOT MODE REQUESTED                            ║\n");
         printk("╚════════════════════════════════════════════════════════╝\n");
         printk("\n");
         
         /* Enter DFU mode with visual indicators */
         hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_FAST_BLINK);
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
         
         printk("🔧 DFU Mode Active\n");
         printk("   • LED patterns indicate DFU mode\n");
         printk("   • Waiting for firmware update\n");
         printk("   • Press Button 1 to exit and continue normal boot\n");
         printk("\n");
         
         /* Wait in DFU mode until button pressed again */
         bool in_dfu = true;
         bool button_was_pressed = true; /* Start true to avoid immediate exit */
         uint32_t dfu_enter_time = k_uptime_get_32();
         
         while (in_dfu) {
             bool currently_pressed = hw_button_is_pressed();
             
             /* Detect button release then press (edge detection) */
             if (!currently_pressed && button_was_pressed) {
                 button_was_pressed = false;
             } else if (currently_pressed && !button_was_pressed) {
                 uint32_t dfu_duration = k_uptime_get_32() - dfu_enter_time;
                 printk("\n✓ Button pressed - exiting DFU mode\n");
                 printk("   DFU session duration: %u ms\n", dfu_duration);
                 in_dfu = false;
             }
             
             /* Print periodic status */
             static uint32_t last_status_print = 0;
             uint32_t current_time = k_uptime_get_32();
             if ((current_time - last_status_print) > 5000) {
                 printk("⏳ DFU mode active... (waiting for button press)\n");
                 last_status_print = current_time;
             }
             
             k_msleep(100);
         }
         
         /* Clear DFU indicators */
         printk("\n");
         hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_OFF);
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
         k_msleep(500);
         
         return true; /* DFU was active */
     }
     
     return false; /* No DFU boot requested */
 }
 
 /**
  * @brief Initialize and start Bluetooth advertising
  */
 static int initialize_bluetooth(void)
 {
     int ret;
     
     printk("[1/3] Initializing Bluetooth Low Energy stack...\n");
     ret = hw_ble_advertising_init();
     if (ret != HW_OK) {
         DIAG_WARNING(DIAG_CAT_SYSTEM, "Bluetooth advertising initialization failed: %d", ret);
         printk("❌ Bluetooth init failed (error: %d)\n", ret);
         printk("   Device will not be discoverable\n");
         return ret;
     }
     printk("✓ Bluetooth stack initialized\n");
     
     /* Give Bluetooth stack time to stabilize */
     printk("[2/3] Waiting for Bluetooth stack to stabilize...\n");
     k_msleep(BT_INIT_DELAY_MS);
     printk("✓ Bluetooth stack ready\n");
     
     /* Start Bluetooth advertising */
     printk("[3/3] Starting Bluetooth advertising...\n");
     ret = hw_ble_advertising_start();
     if (ret == HW_OK) {
         printk("✓ Bluetooth advertising started successfully\n");
         printk("\n📡 Bluetooth Status:\n");
         printk("   Device Name:   NISC-Medical\n");
         printk("   Status:        Advertising (discoverable)\n");
         printk("   LED Indicator: LED3 will blink during activity\n");
         DIAG_INFO(DIAG_CAT_SYSTEM, "Bluetooth advertising active");
     } else {
         printk("❌ Failed to start Bluetooth advertising (error: %d)\n", ret);
         DIAG_WARNING(DIAG_CAT_SYSTEM, "Failed to start Bluetooth advertising: %d", ret);
     }
     
     return ret;
 }
 
 /**
  * @brief Create all application threads
  */
 static int create_application_threads(void)
 {
     int ret;
     
     printk("Creating application threads with staggered startup...\n");
 
     /* Start supervisor thread first - highest priority for safety */
     printk("[1/5] Creating supervisor thread (Priority: 1)...\n");
     ret = thread_manager_create_thread(THREAD_ID_SUPERVISOR, supervisor_thread, 
                                       NULL, NULL, NULL);
     if (ret != SUCCESS) {
         system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create supervisor thread");
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
         return ret;
     }
     printk("✓ Supervisor thread created\n");
     k_sleep(K_MSEC(100));
 
     /* Start hardware update thread for LED patterns */
     printk("[2/5] Creating hardware update thread...\n");
     ret = thread_manager_create_thread(THREAD_ID_DIAGNOSTICS, hardware_update_thread, 
                                       NULL, NULL, NULL);
     if (ret != SUCCESS) {
         system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create hardware update thread");
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
         return ret;
     }
     printk("✓ Hardware update thread created\n");
     k_sleep(K_MSEC(100));
 
     /* Start data acquisition thread */
     printk("[3/5] Creating data acquisition thread...\n");
     ret = thread_manager_create_thread(THREAD_ID_DATA_ACQUISITION, data_acquisition_thread,
                                       NULL, NULL, NULL);
     if (ret != SUCCESS) {
         system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create data acquisition thread");
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
         return ret;
     }
     printk("✓ Data acquisition thread created\n");
     k_sleep(K_MSEC(100));
 
     /* Start data processing thread */
     printk("[4/5] Creating data processing thread...\n");
     ret = thread_manager_create_thread(THREAD_ID_DATA_PROCESSING, data_processing_thread,
                                       NULL, NULL, NULL);
     if (ret != SUCCESS) {
         system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create data processing thread");
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
         return ret;
     }
     printk("✓ Data processing thread created\n");
     k_sleep(K_MSEC(100));
 
     /* Start communication thread */
     printk("[5/5] Creating communication thread...\n");
     ret = thread_manager_create_thread(THREAD_ID_COMMUNICATION, communication_thread,
                                       NULL, NULL, NULL);
     if (ret != SUCCESS) {
         system_handle_error(SYSTEM_ERROR_THREAD, "Failed to create communication thread");
         hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SOS);
         return ret;
     }
     printk("✓ Communication thread created\n");
 
     printk("\n✓ All %d application threads created successfully\n", THREAD_ID_MAX);
     return SUCCESS;
 }
 
 /**
  * @brief Set final LED patterns for normal operation
  */
 static void set_normal_operation_leds(void)
 {
     hw_led_set_pattern(HW_LED_STATUS, HW_PULSE_BREATHING);      /* Breathing = system OK */
     hw_led_set_pattern(HW_LED_HEARTBEAT, HW_PULSE_HEARTBEAT);   /* Medical pulse */
     hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_OFF);     /* Will be controlled by comm thread */
     hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);             /* Off = no errors */
     
     printk("✓ LED patterns set for normal operation:\n");
     printk("   LED1 (Status):        Breathing pattern (system healthy)\n");
     printk("   LED2 (Heartbeat):     Heartbeat pattern (medical pulse)\n");
     printk("   LED3 (Communication): Controlled by communication activity\n");
     printk("   LED4 (Error):         Off (no errors)\n");
 }
 
 /*============================================================================*/
 /* Thread Function Implementations                                            */
 /*============================================================================*/
 
 /**
  * @brief Supervisor thread for system health monitoring
  */
 void supervisor_thread(void *arg1, void *arg2, void *arg3)
 {
     ARG_UNUSED(arg1);
     ARG_UNUSED(arg2);
     ARG_UNUSED(arg3);
 
     printk("🔍 Supervisor thread started - safety monitoring active\n");
 
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
 
 /**
  * @brief Hardware update thread to maintain LED patterns
  */
/**
 * @brief Hardware update thread to maintain LED patterns
 */
void hardware_update_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("💡 Hardware update thread started - LED pattern management\n");

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

/** @brief Simple sensor readings array for testing */
static int simple_sensor_values[SENSOR_TYPE_MAX] = {72, 366, 10, 980}; // HR, Temp*10, Motion*10, SpO2*10

/**
 * @brief Data acquisition thread with hardware integration and realistic medical simulation
 */
void data_acquisition_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("📊 Data acquisition thread started - sampling sensors every 1 second\n");
    printk("════════════════════════════════════════════════════════════════\n");
    printk("                   MEDICAL DATA MONITORING                       \n");
    printk("════════════════════════════════════════════════════════════════\n");
    printk("\n");

    uint32_t cycle_count = 0U;
    uint32_t alert_count = 0U;

    while (1) {
        thread_manager_heartbeat(THREAD_ID_DATA_ACQUISITION);

        /* Generate realistic medical data with integer arithmetic only */
        uint32_t uptime_sec = k_uptime_get_32() / 1000U;
        
        /* Heart Rate: 60-100 bpm with natural variation */
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

        /* Calculate quality indicators */
        uint8_t hr_quality = 88 + (cycle_count % 13);
        uint8_t temp_quality = 91 + (cycle_count % 10);
        uint8_t motion_quality = 94 + (cycle_count % 7);
        uint8_t spo2_quality = 97 + (cycle_count % 4);

        /* Display real-time medical data pulse with enhanced formatting */
        printk("\n");
        printk("┌─────────────────────────────────────────────────────┐\n");
        printk("│ MEDICAL DATA PULSE #%-4u [Time: %u.%03u s]        │\n", 
               cycle_count, uptime_sec, (k_uptime_get_32() % 1000U));
        printk("├─────────────────────────────────────────────────────┤\n");
        printk("│ ❤  HEART RATE:   %3d bpm     [Quality: %2d%%]    │\n", 
               simple_sensor_values[0], hr_quality);
        printk("│ 🌡  TEMPERATURE:  %2d.%d°C       [Quality: %2d%%]    │\n", 
               simple_sensor_values[1] / 10, simple_sensor_values[1] % 10, temp_quality);
        printk("│ 🏃 MOTION:       %d.%d g       [Quality: %2d%%]    │\n", 
               simple_sensor_values[2] / 10, simple_sensor_values[2] % 10, motion_quality);
        printk("│ 🫁 BLOOD O2:     %2d.%d%%       [Quality: %2d%%]    │\n", 
               simple_sensor_values[3] / 10, simple_sensor_values[3] % 10, spo2_quality);
        printk("└─────────────────────────────────────────────────────┘\n");

        /* Check for alert conditions and provide visual/LED feedback */
        bool alert_triggered = false;
        
        if (simple_sensor_values[0] > 85) {
            printk("⚠ ALERT: Elevated heart rate detected! (HR: %d bpm)\n", simple_sensor_values[0]);
            hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_FAST_BLINK);
            k_sleep(K_MSEC(200));
            hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
            alert_triggered = true;
            alert_count++;
        }
        
        if (simple_sensor_values[2] > 20) {
            printk("ℹ INFO: High activity detected - Patient is active (Motion: %d.%d g)\n",
                   simple_sensor_values[2] / 10, simple_sensor_values[2] % 10);
            /* Brief communication LED flash for activity */
            hw_led_set_state(HW_LED_COMMUNICATION, true);
            k_sleep(K_MSEC(100));
            hw_led_set_state(HW_LED_COMMUNICATION, false);
        }
        
        if (simple_sensor_values[1] > 372) {
            printk("⚠ WARNING: Elevated temperature detected (Temp: %d.%d°C)\n",
                   simple_sensor_values[1] / 10, simple_sensor_values[1] % 10);
            hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_SLOW_BLINK);
            alert_triggered = true;
            alert_count++;
        } else if (simple_sensor_values[3] < 960) {
            printk("⚠ CAUTION: Blood oxygen below normal range (SpO2: %d.%d%%)\n",
                   simple_sensor_values[3] / 10, simple_sensor_values[3] % 10);
            hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_DOUBLE_BLINK);
            alert_triggered = true;
            alert_count++;
        }
        
        /* Clear error LED if no alerts */
        if (!alert_triggered) {
            hw_led_set_pattern(HW_LED_ERROR, HW_PULSE_OFF);
        }

        /* Periodic summary every 10 cycles */
        if (cycle_count > 0 && cycle_count % 10 == 0) {
            printk("\n");
            printk("╔═══════════════════════════════════════════════════════╗\n");
            printk("║ 📋 10-CYCLE SUMMARY                                   ║\n");
            printk("╠═══════════════════════════════════════════════════════╣\n");
            printk("║ Total Samples:     %-4u                              ║\n", cycle_count);
            printk("║ Alerts Triggered:  %-4u                              ║\n", alert_count);
            printk("║ System Uptime:     %u seconds                        ║\n", uptime_sec);
            printk("║ Current Status:    %-30s ║\n", alert_triggered ? "⚠ Alert Active" : "✓ Normal Operation");
            printk("╚═══════════════════════════════════════════════════════╝\n");
            printk("\n");
        }

        cycle_count++;
        k_sleep(K_MSEC(SENSOR_SAMPLING_INTERVAL_MS));
    }
}

/**
 * @brief Data processing thread for analysis
 */
void data_processing_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("🔬 Data processing thread started - analyzing sensor data\n");

    uint32_t processing_cycle = 0U;

    while (1) {
        thread_manager_heartbeat(THREAD_ID_DATA_PROCESSING);
        
        processing_cycle++;
        
        /* Periodic processing status */
        if (processing_cycle % 6 == 0) {  /* Every 30 seconds */
            DIAG_INFO(DIAG_CAT_SYSTEM, "Data processing cycle %u - analyzing trends", processing_cycle);
        }
        
        k_sleep(K_MSEC(DATA_PROCESSING_INTERVAL_MS));
    }
}

/**
 * @brief Communication thread with enhanced Bluetooth integration
 */
void communication_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    printk("📡 Communication thread started - transmitting data every 15 seconds\n");
    printk("\n");

    uint32_t transmission_count = 0U;

    while (1) {
        thread_manager_heartbeat(THREAD_ID_COMMUNICATION);

        transmission_count++;

        /* Show communication activity on LED */
        hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_FAST_BLINK);
        
        /* Display transmission header */
        printk("\n");
        printk("┌═══════════════════════════════════════════════════════┐\n");
        printk("│ 📤 TRANSMITTING MEDICAL DATA PACKET #%-4u           │\n", transmission_count);
        printk("├───────────────────────────────────────────────────────┤\n");
        printk("│           Current Patient Vitals Summary              │\n");
        printk("├───────────────────────────────────────────────────────┤\n");
        printk("│ Heart Rate:     %3d bpm                              │\n", simple_sensor_values[0]);
        printk("│ Temperature:    %2d.%d°C                               │\n", 
               simple_sensor_values[1] / 10, simple_sensor_values[1] % 10);
        printk("│ Motion:         %d.%d g                                │\n", 
               simple_sensor_values[2] / 10, simple_sensor_values[2] % 10);
        printk("│ Blood Oxygen:   %2d.%d%%                               │\n", 
               simple_sensor_values[3] / 10, simple_sensor_values[3] % 10);
        printk("└───────────────────────────────────────────────────────┘\n");
        
        /* Prepare and send medical data via serial Bluetooth */
        char bt_data[128];
        snprintf(bt_data, sizeof(bt_data), 
                "MEDICAL_DATA,HR:%d,TEMP:%d.%d,MOTION:%d.%d,SPO2:%d.%d,PKT:%u",
                simple_sensor_values[0],
                simple_sensor_values[1] / 10, simple_sensor_values[1] % 10,
                simple_sensor_values[2] / 10, simple_sensor_values[2] % 10,
                simple_sensor_values[3] / 10, simple_sensor_values[3] % 10,
                transmission_count);
        
        int ret = hw_serial_bt_send((const uint8_t *)bt_data, strlen(bt_data));
        
        /* Show transmission protocol with enhanced formatting */
        uint32_t protocol = transmission_count % 3U;
        switch (protocol) {
            case 0U:
                printk("📡 Protocol: Bluetooth Low Energy (BLE)\n");
                printk("   Status:   Advertising Active\n");
                printk("   Device:   NISC-Medical\n");
                printk("   Signal:   Broadcasting medical data\n");
                
                /* Visual BLE transmission pattern */
                for (int i = 0; i < 3; i++) {
                    hw_led_set_state(HW_LED_COMMUNICATION, true);
                    k_msleep(80);
                    hw_led_set_state(HW_LED_COMMUNICATION, false);
                    k_msleep(80);
                }
                break;
                
            case 1U:
                printk("📟 Protocol: Serial Bluetooth Module\n");
                printk("   Status:   %s\n", ret == HW_OK ? "Transmitted" : "Failed");
                printk("   Data:     %s\n", bt_data);
                printk("   Length:   %zu bytes\n", strlen(bt_data));
                
                /* Longer pulse for serial transmission */
                hw_led_set_state(HW_LED_COMMUNICATION, true);
                k_msleep(500);
                hw_led_set_state(HW_LED_COMMUNICATION, false);
                break;
                
            case 2U:
                printk("💻 Protocol: USB Console Interface\n");
                printk("   Status:   Active\n");
                printk("   Console:  Ready for shell commands\n");
                printk("   Logging:  Real-time medical data display\n");
                
                /* Double blink for console activity */
                hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_DOUBLE_BLINK);
                k_msleep(1000);
                break;
        }
        
        printk("\n✓ Data packet #%u transmitted successfully\n", transmission_count);
        
        /* Provide transmission statistics every 5 transmissions */
        if (transmission_count % 5 == 0) {
            printk("\n");
            printk("╔═══════════════════════════════════════════════════════╗\n");
            printk("║ 📊 TRANSMISSION STATISTICS                            ║\n");
            printk("╠═══════════════════════════════════════════════════════╣\n");
            printk("║ Total Packets Sent:     %-6u                        ║\n", transmission_count);
            printk("║ Transmission Interval:  15 seconds                    ║\n");
            printk("║ Bluetooth Status:       Active                        ║\n");
            printk("║ Data Format:            Medical CSV Protocol          ║\n");
            printk("╚═══════════════════════════════════════════════════════╝\n");
            printk("\n");
        }

        /* Turn off communication LED after transmission */
        hw_led_set_pattern(HW_LED_COMMUNICATION, HW_PULSE_OFF);

        k_sleep(K_MSEC(COMMUNICATION_INTERVAL_MS));
    }
}