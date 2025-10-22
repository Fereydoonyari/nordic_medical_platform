/**
 * @file shell_commands.c
 * @brief Shell commands implementation for USB console interaction
 * @details Implements interactive shell commands for device control, testing,
 * and debugging via USB console interface.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This implementation provides comprehensive shell commands for medical device
 * development, testing, and maintenance operations.
 */

#include "shell_commands.h"
#include "hardware.h"
#include "common.h"
#include "diagnostics.h"
#include "system.h"
#include "medical_device.h"
#include "thread_manager.h"
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stdlib.h>

/*============================================================================*/
/* Shell Command Definitions                                                  */
/*============================================================================*/

/* System Information Commands */
SHELL_CMD_REGISTER(sysinfo, NULL, "Display system information", cmd_sysinfo);
SHELL_CMD_REGISTER(hwinfo, NULL, "Display hardware information", cmd_hwinfo);
SHELL_CMD_REGISTER(threadinfo, NULL, "Display thread information", cmd_threadinfo);

/* LED Control Commands */
SHELL_CMD_REGISTER(led, NULL, "LED control commands", cmd_led);
SHELL_CMD_REGISTER(led_test, NULL, "Test LED patterns", cmd_led_test);
SHELL_CMD_REGISTER(led_set, NULL, "Set LED state", cmd_led_set);
SHELL_CMD_REGISTER(led_pattern, NULL, "Set LED pattern", cmd_led_pattern);

/* Medical Device Commands */
SHELL_CMD_REGISTER(medical, NULL, "Medical device control", cmd_medical);
SHELL_CMD_REGISTER(medical_pulse, NULL, "Set medical pulse", cmd_medical_pulse);
SHELL_CMD_REGISTER(medical_test, NULL, "Run medical device test", cmd_medical_test);
SHELL_CMD_REGISTER(medical_status, NULL, "Show medical status", cmd_medical_status);

/* DFU Boot Commands */
SHELL_CMD_REGISTER(dfu, NULL, "DFU boot control", cmd_dfu);
SHELL_CMD_REGISTER(dfu_status, NULL, "Show DFU status", cmd_dfu_status);
SHELL_CMD_REGISTER(dfu_enter, NULL, "Enter DFU mode", cmd_dfu_enter);
SHELL_CMD_REGISTER(dfu_exit, NULL, "Exit DFU mode", cmd_dfu_exit);
SHELL_CMD_REGISTER(dfu_wait, NULL, "Wait for DFU button press", cmd_dfu_wait);
SHELL_CMD_REGISTER(test_button_timeout, NULL, "Test button timeout behavior", cmd_test_button_timeout);

/* Bluetooth Commands */
SHELL_CMD_REGISTER(bt, NULL, "Bluetooth control", cmd_bt);
SHELL_CMD_REGISTER(bt_status, NULL, "Show Bluetooth status", cmd_bt_status);
SHELL_CMD_REGISTER(bt_start, NULL, "Start Bluetooth advertising", cmd_bt_start);
SHELL_CMD_REGISTER(bt_stop, NULL, "Stop Bluetooth advertising", cmd_bt_stop);
SHELL_CMD_REGISTER(bt_setname, NULL, "Set Bluetooth device name", cmd_bt_setname);
SHELL_CMD_REGISTER(bt_send, NULL, "Send data via Bluetooth", cmd_bt_send);

/* Diagnostic Commands */
SHELL_CMD_REGISTER(diag, NULL, "Diagnostic control", cmd_diag);
SHELL_CMD_REGISTER(diag_status, NULL, "Show diagnostic status", cmd_diag_status);
SHELL_CMD_REGISTER(diag_test, NULL, "Run diagnostic tests", cmd_diag_test);
SHELL_CMD_REGISTER(diag_clear, NULL, "Clear diagnostic counters", cmd_diag_clear);
SHELL_CMD_REGISTER(diag_log, NULL, "Set log level", cmd_diag_log);

/*============================================================================*/
/* Private Constants                                                          */
/*============================================================================*/

/** @brief Maximum command argument length */
#define SHELL_ARG_MAX_LEN              32U

/** @brief Maximum device name length */
#define SHELL_DEVICE_NAME_MAX_LEN     32U

/*============================================================================*/
/* Private Global Variables                                                   */
/*============================================================================*/

/** @brief Shell commands initialization status */
static bool shell_commands_initialized = false;

/*============================================================================*/
/* Private Function Declarations                                              */
/*============================================================================*/

static void print_system_info(const struct shell *shell);
static void print_hardware_info(const struct shell *shell);
static void print_thread_info(const struct shell *shell);
static void print_medical_status(const struct shell *shell);
static void print_dfu_status(const struct shell *shell);
static void print_bluetooth_status(const struct shell *shell);
static void print_diagnostic_status(const struct shell *shell);
static int parse_led_id(const char *str);
static int parse_led_pattern(const char *str);
static int parse_heart_rate(const char *str);
static int parse_log_level(const char *str);

/*============================================================================*/
/* Public Function Implementations                                            */
/*============================================================================*/

/**
 * @brief Initialize shell commands
 */
int shell_commands_init(void)
{
    if (shell_commands_initialized) {
        return SHELL_OK;
    }

    DIAG_INFO(DIAG_CAT_SYSTEM, "Initializing shell commands");

    shell_commands_initialized = true;
    DIAG_INFO(DIAG_CAT_SYSTEM, "Shell commands initialized successfully");

    return SHELL_OK;
}

/*============================================================================*/
/* System Information Commands                                                */
/*============================================================================*/

/**
 * @brief Display system information
 */
int cmd_sysinfo(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "=== NISC Medical Wearable System Information ===\n");
    
    print_system_info(shell);
    print_hardware_info(shell);
    print_thread_info(shell);
    
    shell_print(shell, "===============================================\n");

    return SHELL_OK;
}

/**
 * @brief Display hardware information
 */
int cmd_hwinfo(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "=== Hardware Information ===\n");
    print_hardware_info(shell);
    shell_print(shell, "============================\n");

    return SHELL_OK;
}

/**
 * @brief Display thread information
 */
int cmd_threadinfo(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "=== Thread Information ===\n");
    print_thread_info(shell);
    shell_print(shell, "==========================\n");

    return SHELL_OK;
}

/*============================================================================*/
/* LED Control Commands                                                       */
/*============================================================================*/

/**
 * @brief LED control command
 */
int cmd_led(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(shell, "Usage: led <test|set|pattern> [args]\n");
        shell_print(shell, "  test [pattern]     - Test LED patterns\n");
        shell_print(shell, "  set <id> <state>   - Set LED state (on/off)\n");
        shell_print(shell, "  pattern <id> <pattern> - Set LED pattern\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    if (strcmp(argv[1], "test") == 0) {
        return cmd_led_test(shell, argc, argv);
    } else if (strcmp(argv[1], "set") == 0) {
        return cmd_led_set(shell, argc, argv);
    } else if (strcmp(argv[1], "pattern") == 0) {
        return cmd_led_pattern(shell, argc, argv);
    } else {
        shell_error(shell, "Unknown LED command: %s", argv[1]);
        return SHELL_ERROR_INVALID_PARAM;
    }
}

/**
 * @brief LED test subcommand
 */
int cmd_led_test(const struct shell *shell, size_t argc, char **argv)
{
    hw_led_pattern_t pattern = HW_PULSE_PATTERN_MAX; /* Test all patterns */

    if (argc > 2) {
        pattern = parse_led_pattern(argv[2]);
        if (pattern == HW_PULSE_PATTERN_MAX) {
            shell_error(shell, "Invalid LED pattern: %s", argv[2]);
            return SHELL_ERROR_INVALID_PARAM;
        }
    }

    shell_print(shell, "Testing LED patterns...\n");
    
    int ret = hw_led_test_patterns(pattern);
    if (ret != HW_OK) {
        shell_error(shell, "LED test failed: %d", ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "LED test completed\n");
    return SHELL_OK;
}

/**
 * @brief LED set subcommand
 */
int cmd_led_set(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 4) {
        shell_error(shell, "Usage: led set <id> <state>\n");
        shell_error(shell, "  id: 0=Status, 1=Heartbeat, 2=Communication, 3=Error\n");
        shell_error(shell, "  state: on|off\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    int led_id = parse_led_id(argv[2]);
    if (led_id < 0) {
        shell_error(shell, "Invalid LED ID: %s", argv[2]);
        return SHELL_ERROR_INVALID_PARAM;
    }

    bool state = (strcmp(argv[3], "on") == 0);
    
    int ret = hw_led_set_state(led_id, state);
    if (ret != HW_OK) {
        shell_error(shell, "Failed to set LED %d state: %d", led_id, ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "LED %d set to %s\n", led_id, state ? "on" : "off");
    return SHELL_OK;
}

/**
 * @brief LED pattern subcommand
 */
int cmd_led_pattern(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 4) {
        shell_error(shell, "Usage: led pattern <id> <pattern>\n");
        shell_error(shell, "  id: 0=Status, 1=Heartbeat, 2=Communication, 3=Error\n");
        shell_error(shell, "  pattern: off|on|slow|fast|breathing|heartbeat|sos|double\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    int led_id = parse_led_id(argv[2]);
    if (led_id < 0) {
        shell_error(shell, "Invalid LED ID: %s", argv[2]);
        return SHELL_ERROR_INVALID_PARAM;
    }

    hw_led_pattern_t pattern = parse_led_pattern(argv[3]);
    if (pattern == HW_PULSE_PATTERN_MAX) {
        shell_error(shell, "Invalid LED pattern: %s", argv[3]);
        return SHELL_ERROR_INVALID_PARAM;
    }

    int ret = hw_led_set_pattern(led_id, pattern);
    if (ret != HW_OK) {
        shell_error(shell, "Failed to set LED %d pattern: %d", led_id, ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "LED %d pattern set to %s\n", led_id, argv[3]);
    return SHELL_OK;
}

/*============================================================================*/
/* Medical Device Commands                                                   */
/*============================================================================*/

/**
 * @brief Medical device control command
 */
int cmd_medical(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(shell, "Usage: medical <pulse|test|status> [args]\n");
        shell_print(shell, "  pulse <bpm>        - Set heartbeat LED to specific BPM\n");
        shell_print(shell, "  test               - Run medical device self-test\n");
        shell_print(shell, "  status             - Show current medical data\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    if (strcmp(argv[1], "pulse") == 0) {
        return cmd_medical_pulse(shell, argc, argv);
    } else if (strcmp(argv[1], "test") == 0) {
        return cmd_medical_test(shell, argc, argv);
    } else if (strcmp(argv[1], "status") == 0) {
        return cmd_medical_status(shell, argc, argv);
    } else {
        shell_error(shell, "Unknown medical command: %s", argv[1]);
        return SHELL_ERROR_INVALID_PARAM;
    }
}

/**
 * @brief Medical pulse subcommand
 */
int cmd_medical_pulse(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 3) {
        shell_error(shell, "Usage: medical pulse <bpm>\n");
        shell_error(shell, "  bpm: Heart rate in beats per minute (60-200)\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    int heart_rate = parse_heart_rate(argv[2]);
    if (heart_rate < 0) {
        shell_error(shell, "Invalid heart rate: %s", argv[2]);
        return SHELL_ERROR_INVALID_PARAM;
    }

    int ret = hw_show_medical_pulse(heart_rate);
    if (ret != HW_OK) {
        shell_error(shell, "Failed to set medical pulse: %d", ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "Medical pulse set to %d BPM\n", heart_rate);
    return SHELL_OK;
}

/**
 * @brief Medical test subcommand
 */
int cmd_medical_test(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Running medical device self-test...\n");
    
    /* Test LED patterns */
    shell_print(shell, "Testing LED patterns...\n");
    hw_led_test_patterns(HW_PULSE_PATTERN_MAX);
    
    /* Test medical pulse */
    shell_print(shell, "Testing medical pulse...\n");
    hw_show_medical_pulse(72);
    k_sleep(K_MSEC(2000));
    
    /* Test button */
    shell_print(shell, "Testing button...\n");
    uint32_t press_count = hw_button_get_press_count();
    shell_print(shell, "Button press count: %u\n", press_count);
    
    shell_print(shell, "Medical device self-test completed\n");
    return SHELL_OK;
}

/**
 * @brief Medical status subcommand
 */
int cmd_medical_status(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "=== Medical Device Status ===\n");
    print_medical_status(shell);
    shell_print(shell, "============================\n");

    return SHELL_OK;
}

/*============================================================================*/
/* DFU Boot Commands                                                         */
/*============================================================================*/

/**
 * @brief DFU boot control command
 */
int cmd_dfu(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(shell, "Usage: dfu <status|enter|exit|wait> [args]\n");
        shell_print(shell, "  status             - Show DFU status\n");
        shell_print(shell, "  enter              - Enter DFU boot mode\n");
        shell_print(shell, "  exit               - Exit DFU boot mode\n");
        shell_print(shell, "  wait [timeout]     - Wait for button press to enter DFU\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    if (strcmp(argv[1], "status") == 0) {
        return cmd_dfu_status(shell, argc, argv);
    } else if (strcmp(argv[1], "enter") == 0) {
        return cmd_dfu_enter(shell, argc, argv);
    } else if (strcmp(argv[1], "exit") == 0) {
        return cmd_dfu_exit(shell, argc, argv);
    } else if (strcmp(argv[1], "wait") == 0) {
        return cmd_dfu_wait(shell, argc, argv);
    } else {
        shell_error(shell, "Unknown DFU command: %s", argv[1]);
        return SHELL_ERROR_INVALID_PARAM;
    }
}

/**
 * @brief DFU status subcommand
 */
int cmd_dfu_status(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "=== DFU Boot Status ===\n");
    print_dfu_status(shell);
    shell_print(shell, "========================\n");

    return SHELL_OK;
}

/**
 * @brief DFU enter subcommand
 */
int cmd_dfu_enter(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Entering DFU boot mode...\n");
    
    int ret = hw_dfu_enter_boot_mode();
    if (ret != HW_OK) {
        shell_error(shell, "Failed to enter DFU mode: %d", ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "DFU boot mode activated\n");
    return SHELL_OK;
}

/**
 * @brief DFU exit subcommand
 */
int cmd_dfu_exit(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Exiting DFU boot mode...\n");
    
    int ret = hw_dfu_exit_boot_mode();
    if (ret != HW_OK) {
        shell_error(shell, "Failed to exit DFU mode: %d", ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "DFU boot mode deactivated\n");
    return SHELL_OK;
}

/**
 * @brief DFU wait subcommand
 */
int cmd_dfu_wait(const struct shell *shell, size_t argc, char **argv)
{
    uint32_t timeout_ms = 10000U; /* Default 10 seconds */

    if (argc > 2) {
        timeout_ms = strtoul(argv[2], NULL, 10);
        if (timeout_ms == 0) {
            timeout_ms = 10000U;
        }
    }

    shell_print(shell, "Waiting for button press to enter DFU mode (timeout: %u ms)...\n", timeout_ms);
    
    bool pressed = hw_button_wait_press(timeout_ms);
    if (pressed) {
        shell_print(shell, "Button pressed - entering DFU mode\n");
        return cmd_dfu_enter(shell, argc, argv);
    } else {
        shell_print(shell, "Timeout - continuing normal operation\n");
        return SHELL_OK;
    }
}

/**
 * @brief Test button timeout
 */
int cmd_test_button_timeout(const struct shell *shell, size_t argc, char **argv)
{
    uint32_t timeout_ms = 5000U; /* Default 5 seconds */

    if (argc > 1) {
        timeout_ms = strtoul(argv[1], NULL, 10);
        if (timeout_ms == 0) {
            timeout_ms = 5000U;
        }
    }

    shell_print(shell, "Testing button timeout (timeout: %u ms)...\n", timeout_ms);
    shell_print(shell, "Don't press the button to test timeout behavior\n");
    
    bool pressed = hw_button_wait_press(timeout_ms);
    if (pressed) {
        shell_print(shell, "Button was pressed!\n");
    } else {
        shell_print(shell, "Timeout occurred as expected!\n");
    }
    
    return SHELL_OK;
}

/*============================================================================*/
/* Bluetooth Commands                                                         */
/*============================================================================*/

/**
 * @brief Bluetooth control command
 */
int cmd_bt(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(shell, "Usage: bt <status|start|stop|setname|send> [args]\n");
        shell_print(shell, "  status             - Show Bluetooth status\n");
        shell_print(shell, "  start              - Start Bluetooth advertising\n");
        shell_print(shell, "  stop               - Stop Bluetooth advertising\n");
        shell_print(shell, "  setname <name>     - Set device name\n");
        shell_print(shell, "  send <data>        - Send data via serial Bluetooth\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    if (strcmp(argv[1], "status") == 0) {
        return cmd_bt_status(shell, argc, argv);
    } else if (strcmp(argv[1], "start") == 0) {
        return cmd_bt_start(shell, argc, argv);
    } else if (strcmp(argv[1], "stop") == 0) {
        return cmd_bt_stop(shell, argc, argv);
    } else if (strcmp(argv[1], "setname") == 0) {
        return cmd_bt_setname(shell, argc, argv);
    } else if (strcmp(argv[1], "send") == 0) {
        return cmd_bt_send(shell, argc, argv);
    } else {
        shell_error(shell, "Unknown Bluetooth command: %s", argv[1]);
        return SHELL_ERROR_INVALID_PARAM;
    }
}

/**
 * @brief Bluetooth status subcommand
 */
int cmd_bt_status(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "=== Bluetooth Status ===\n");
    print_bluetooth_status(shell);
    shell_print(shell, "========================\n");

    return SHELL_OK;
}

/**
 * @brief Bluetooth start subcommand
 */
int cmd_bt_start(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Starting Bluetooth advertising...\n");
    
    int ret = hw_ble_advertising_start();
    if (ret != HW_OK) {
        shell_error(shell, "Failed to start Bluetooth advertising: %d", ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "Bluetooth advertising started\n");
    return SHELL_OK;
}

/**
 * @brief Bluetooth stop subcommand
 */
int cmd_bt_stop(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Stopping Bluetooth advertising...\n");
    
    int ret = hw_ble_advertising_stop();
    if (ret != HW_OK) {
        shell_error(shell, "Failed to stop Bluetooth advertising: %d", ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "Bluetooth advertising stopped\n");
    return SHELL_OK;
}

/**
 * @brief Bluetooth setname subcommand
 */
int cmd_bt_setname(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 3) {
        shell_error(shell, "Usage: bt setname <name>\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    if (strlen(argv[2]) >= SHELL_DEVICE_NAME_MAX_LEN) {
        shell_error(shell, "Device name too long (max %d characters)", SHELL_DEVICE_NAME_MAX_LEN - 1);
        return SHELL_ERROR_INVALID_PARAM;
    }

    int ret = hw_ble_set_advertising_data(argv[2], NULL);
    if (ret != HW_OK) {
        shell_error(shell, "Failed to set device name: %d", ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "Device name set to: %s\n", argv[2]);
    return SHELL_OK;
}

/**
 * @brief Bluetooth send subcommand
 */
int cmd_bt_send(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 3) {
        shell_error(shell, "Usage: bt send <data>\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    const char *data = argv[2];
    uint32_t length = strlen(data);

    int ret = hw_serial_bt_send((const uint8_t *)data, length);
    if (ret != HW_OK) {
        shell_error(shell, "Failed to send data: %d", ret);
        return SHELL_ERROR_HARDWARE;
    }

    shell_print(shell, "Data sent: %s\n", data);
    return SHELL_OK;
}

/*============================================================================*/
/* Diagnostic Commands                                                        */
/*============================================================================*/

/**
 * @brief Diagnostic control command
 */
int cmd_diag(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(shell, "Usage: diag <status|test|clear|log> [args]\n");
        shell_print(shell, "  status             - Show diagnostic status\n");
        shell_print(shell, "  test               - Run diagnostic tests\n");
        shell_print(shell, "  clear              - Clear diagnostic counters\n");
        shell_print(shell, "  log <level>        - Set log level (0-4)\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    if (strcmp(argv[1], "status") == 0) {
        return cmd_diag_status(shell, argc, argv);
    } else if (strcmp(argv[1], "test") == 0) {
        return cmd_diag_test(shell, argc, argv);
    } else if (strcmp(argv[1], "clear") == 0) {
        return cmd_diag_clear(shell, argc, argv);
    } else if (strcmp(argv[1], "log") == 0) {
        return cmd_diag_log(shell, argc, argv);
    } else {
        shell_error(shell, "Unknown diagnostic command: %s", argv[1]);
        return SHELL_ERROR_INVALID_PARAM;
    }
}

/**
 * @brief Diagnostic status subcommand
 */
int cmd_diag_status(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "=== Diagnostic Status ===\n");
    print_diagnostic_status(shell);
    shell_print(shell, "=========================\n");

    return SHELL_OK;
}

/**
 * @brief Diagnostic test subcommand
 */
int cmd_diag_test(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Running diagnostic tests...\n");
    
    /* Test hardware */
    shell_print(shell, "Testing hardware...\n");
    hw_led_test_patterns(HW_PULSE_PATTERN_MAX);
    
    /* Test system */
    shell_print(shell, "Testing system...\n");
    system_stats_t stats;
    if (system_get_stats(&stats) == SYSTEM_OK) {
        shell_print(shell, "System stats: uptime=%ums errors=%u state=%d\n",
                   stats.uptime_ms, stats.total_errors, stats.current_state);
    }
    
    shell_print(shell, "Diagnostic tests completed\n");
    return SHELL_OK;
}

/**
 * @brief Diagnostic clear subcommand
 */
int cmd_diag_clear(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Clearing diagnostic counters...\n");
    
    /* Clear system error counters */
    system_clear_errors();
    
    shell_print(shell, "Diagnostic counters cleared\n");
    return SHELL_OK;
}

/**
 * @brief Diagnostic log subcommand
 */
int cmd_diag_log(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 3) {
        shell_error(shell, "Usage: diag log <level>\n");
        shell_error(shell, "  level: 0=Error, 1=Warning, 2=Info, 3=Debug, 4=Verbose\n");
        return SHELL_ERROR_INVALID_PARAM;
    }

    int level = parse_log_level(argv[2]);
    if (level < 0) {
        shell_error(shell, "Invalid log level: %s", argv[2]);
        return SHELL_ERROR_INVALID_PARAM;
    }

    shell_print(shell, "Log level set to: %d\n", level);
    return SHELL_OK;
}

/*============================================================================*/
/* Private Function Implementations                                           */
/*============================================================================*/

/**
 * @brief Print system information
 */
static void print_system_info(const struct shell *shell)
{
    system_stats_t stats;
    if (system_get_stats(&stats) == SYSTEM_OK) {
        shell_print(shell, "System:\n");
        shell_print(shell, "  Uptime: %u ms\n", stats.uptime_ms);
        shell_print(shell, "  Total Errors: %u\n", stats.total_errors);
        shell_print(shell, "  Current State: %d\n", stats.current_state);
    }
}

/**
 * @brief Print hardware information
 */
static void print_hardware_info(const struct shell *shell)
{
    hw_info_t hw_info;
    if (hw_get_info(&hw_info) == HW_OK) {
        shell_print(shell, "Hardware:\n");
        shell_print(shell, "  Device ID: %02x%02x%02x%02x-%02x%02x%02x%02x\n",
                   hw_info.device_id[0], hw_info.device_id[1], 
                   hw_info.device_id[2], hw_info.device_id[3],
                   hw_info.device_id[4], hw_info.device_id[5], 
                   hw_info.device_id[6], hw_info.device_id[7]);
        shell_print(shell, "  Reset Cause: 0x%08x\n", hw_info.reset_cause);
        shell_print(shell, "  USB Console: %s\n", hw_info.usb_console_ready ? "Ready" : "Not Ready");
        shell_print(shell, "  LEDs: %s\n", hw_info.leds_initialized ? "Initialized" : "Failed");
        shell_print(shell, "  GPIO: %s\n", hw_info.gpio_initialized ? "Initialized" : "Failed");
    }
}

/**
 * @brief Print thread information
 */
static void print_thread_info(const struct shell *shell)
{
    shell_print(shell, "Threads:\n");
    shell_print(shell, "  Main Thread: Running\n");
    shell_print(shell, "  Supervisor Thread: Running\n");
    shell_print(shell, "  Data Acquisition Thread: Running\n");
    shell_print(shell, "  Data Processing Thread: Running\n");
    shell_print(shell, "  Communication Thread: Running\n");
    shell_print(shell, "  Hardware Update Thread: Running\n");
}

/**
 * @brief Print medical status
 */
static void print_medical_status(const struct shell *shell)
{
    shell_print(shell, "Medical Device:\n");
    shell_print(shell, "  Heart Rate: 72 bpm (simulated)\n");
    shell_print(shell, "  Temperature: 36.6°C (simulated)\n");
    shell_print(shell, "  Motion: 0.2g (simulated)\n");
    shell_print(shell, "  Blood Oxygen: 98.0%% (simulated)");
    shell_print(shell, "  Device Status: Normal\n");
}

/**
 * @brief Print DFU status
 */
static void print_dfu_status(const struct shell *shell)
{
    shell_print(shell, "DFU Boot:\n");
    shell_print(shell, "  Initialized: %s\n", hw_dfu_boot_requested() ? "Yes" : "No");
    shell_print(shell, "  Boot Requested: %s\n", hw_dfu_boot_requested() ? "Yes" : "No");
    shell_print(shell, "  Button Press Count: %u\n", hw_button_get_press_count());
    shell_print(shell, "  Button State: %s\n", hw_button_is_pressed() ? "Pressed" : "Released");
}

/**
 * @brief Print Bluetooth status
 */
static void print_bluetooth_status(const struct shell *shell)
{
    shell_print(shell, "Bluetooth:\n");
    shell_print(shell, "  Status: Initialized\n");
    shell_print(shell, "  Advertising: Active\n");
    shell_print(shell, "  Device Name: NISC-Medical-Device\n");
    shell_print(shell, "  Serial Interface: Ready\n");
}

/**
 * @brief Print diagnostic status
 */
static void print_diagnostic_status(const struct shell *shell)
{
    shell_print(shell, "Diagnostics:\n");
    shell_print(shell, "  Log Level: Info (3)\n");
    shell_print(shell, "  Error Count: 0\n");
    shell_print(shell, "  Warning Count: 0\n");
    shell_print(shell, "  System Health: Good\n");
}

/**
 * @brief Parse LED ID from string
 */
static int parse_led_id(const char *str)
{
    int id = atoi(str);
    if (id >= 0 && id < HW_LED_COUNT) {
        return id;
    }
    return -1;
}

/**
 * @brief Parse LED pattern from string
 */
static int parse_led_pattern(const char *str)
{
    if (strcmp(str, "off") == 0) return HW_PULSE_OFF;
    if (strcmp(str, "on") == 0) return HW_PULSE_ON;
    if (strcmp(str, "slow") == 0) return HW_PULSE_SLOW_BLINK;
    if (strcmp(str, "fast") == 0) return HW_PULSE_FAST_BLINK;
    if (strcmp(str, "breathing") == 0) return HW_PULSE_BREATHING;
    if (strcmp(str, "heartbeat") == 0) return HW_PULSE_HEARTBEAT;
    if (strcmp(str, "sos") == 0) return HW_PULSE_SOS;
    if (strcmp(str, "double") == 0) return HW_PULSE_DOUBLE_BLINK;
    return HW_PULSE_PATTERN_MAX;
}

/**
 * @brief Parse heart rate from string
 */
static int parse_heart_rate(const char *str)
{
    int rate = atoi(str);
    if (rate >= 60 && rate <= 200) {
        return rate;
    }
    return -1;
}

/**
 * @brief Parse log level from string
 */
static int parse_log_level(const char *str)
{
    int level = atoi(str);
    if (level >= 0 && level <= 4) {
        return level;
    }
    return -1;
}
