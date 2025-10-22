/**
 * @file shell_commands.h
 * @brief Shell commands interface for USB console interaction
 * @details Provides interactive shell commands for device control, testing,
 * and debugging via USB console interface.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This module provides comprehensive shell commands for medical device
 * development, testing, and maintenance operations.
 */

#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H

#include <zephyr/shell/shell.h>
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* Shell Command Return Codes                                                 */
/*============================================================================*/

/** @defgroup ShellReturnCodes Shell Command Return Codes
 * @brief Return codes for shell command operations
 * @{
 */

#define SHELL_OK                      0   /**< Command executed successfully */
#define SHELL_ERROR_INVALID_PARAM     -1  /**< Invalid parameter provided */
#define SHELL_ERROR_NOT_READY         -2  /**< System not ready for command */
#define SHELL_ERROR_HARDWARE          -3  /**< Hardware operation failed */
#define SHELL_ERROR_COMMAND_FAILED    -4  /**< Command execution failed */

/** @} */ /* End of ShellReturnCodes group */

/*============================================================================*/
/* Public Function Declarations                                               */
/*============================================================================*/

/**
 * @brief Initialize shell commands
 * @details Registers all shell commands with the Zephyr shell subsystem.
 * This function must be called after hardware initialization.
 * 
 * @return SHELL_OK on success, error code on failure
 */
int shell_commands_init(void);

/*============================================================================*/
/* System Information Commands                                                */
/*============================================================================*/

/**
 * @brief Display system information
 * @details Shows comprehensive system status including hardware info,
 * uptime, error counts, and current state.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_sysinfo(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Display hardware information
 * @details Shows detailed hardware status including device ID, reset cause,
 * and peripheral status.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_hwinfo(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Display thread information
 * @details Shows current thread status, stack usage, and thread statistics.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_threadinfo(const struct shell *shell, size_t argc, char **argv);

/*============================================================================*/
/* LED Control Commands                                                       */
/*============================================================================*/

/**
 * @brief LED control command
 * @details Controls LED states and patterns for testing and status indication.
 * 
 * Usage:
 *   led test [pattern]     - Test LED patterns
 *   led set <id> <state>  - Set LED state (on/off)
 *   led pattern <id> <pattern> - Set LED pattern
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_led(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief LED test subcommand
 * @details Tests LED patterns for hardware validation.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_led_test(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief LED set subcommand
 * @details Sets LED state directly.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_led_set(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief LED pattern subcommand
 * @details Sets LED pattern for animation.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_led_pattern(const struct shell *shell, size_t argc, char **argv);

/*============================================================================*/
/* Medical Device Commands                                                    */
/*============================================================================*/

/**
 * @brief Medical device control command
 * @details Controls medical device simulation and monitoring.
 * 
 * Usage:
 *   medical pulse <bpm>    - Set heartbeat LED to specific BPM
 *   medical test           - Run medical device self-test
 *   medical status         - Show current medical data
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_medical(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Medical pulse subcommand
 * @details Sets medical pulse LED to specific heart rate.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_medical_pulse(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Medical test subcommand
 * @details Runs medical device self-test procedures.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_medical_test(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Medical status subcommand
 * @details Shows current medical sensor data.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_medical_status(const struct shell *shell, size_t argc, char **argv);

/*============================================================================*/
/* DFU Boot Commands                                                          */
/*============================================================================*/

/**
 * @brief DFU boot control command
 * @details Controls DFU boot process and firmware update operations.
 * 
 * Usage:
 *   dfu status            - Show DFU status
 *   dfu enter             - Enter DFU boot mode
 *   dfu exit              - Exit DFU boot mode
 *   dfu wait              - Wait for button press to enter DFU
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_dfu(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief DFU status subcommand
 * @details Shows current DFU boot status.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_dfu_status(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief DFU enter subcommand
 * @details Enters DFU boot mode.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_dfu_enter(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief DFU exit subcommand
 * @details Exits DFU boot mode.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_dfu_exit(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief DFU wait subcommand
 * @details Waits for button press to enter DFU mode.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_dfu_wait(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Test button timeout behavior
 * @details Tests the button timeout functionality without entering DFU mode.
 * Useful for debugging button timeout issues.
 * 
 * Usage:
 *   test_button_timeout [timeout_ms]  - Test timeout (default: 5000ms)
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return SHELL_OK on success, error code on failure
 */
int cmd_test_button_timeout(const struct shell *shell, size_t argc, char **argv);

/*============================================================================*/
/* Bluetooth Commands                                                         */
/*============================================================================*/

/**
 * @brief Bluetooth control command
 * @details Controls Bluetooth Low Energy advertising and communication.
 * 
 * Usage:
 *   bt status             - Show Bluetooth status
 *   bt start              - Start Bluetooth advertising
 *   bt stop               - Stop Bluetooth advertising
 *   bt setname <name>     - Set device name
 *   bt send <data>        - Send data via serial Bluetooth
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_bt(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Bluetooth status subcommand
 * @details Shows Bluetooth status and connection information.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_bt_status(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Bluetooth start subcommand
 * @details Starts Bluetooth advertising.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_bt_start(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Bluetooth stop subcommand
 * @details Stops Bluetooth advertising.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_bt_stop(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Bluetooth setname subcommand
 * @details Sets Bluetooth device name.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_bt_setname(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Bluetooth send subcommand
 * @details Sends data via serial Bluetooth interface.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_bt_send(const struct shell *shell, size_t argc, char **argv);

/*============================================================================*/
/* Diagnostic Commands                                                        */
/*============================================================================*/

/**
 * @brief Diagnostic control command
 * @details Controls diagnostic operations and system testing.
 * 
 * Usage:
 *   diag status           - Show diagnostic status
 *   diag test             - Run diagnostic tests
 *   diag clear            - Clear diagnostic counters
 *   diag log <level>      - Set log level
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_diag(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Diagnostic status subcommand
 * @details Shows diagnostic status and error counts.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_diag_status(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Diagnostic test subcommand
 * @details Runs diagnostic tests.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_diag_test(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Diagnostic clear subcommand
 * @details Clears diagnostic counters.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_diag_clear(const struct shell *shell, size_t argc, char **argv);

/**
 * @brief Diagnostic log subcommand
 * @details Sets diagnostic log level.
 * 
 * @param shell Shell instance
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, error code on failure
 */
int cmd_diag_log(const struct shell *shell, size_t argc, char **argv);

#endif /* SHELL_COMMANDS_H */
