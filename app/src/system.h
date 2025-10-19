/**
 * @file system.h
 * @brief System initialization and core functionality for NISC Medical Wearable Device
 * @details This header provides system-level initialization, error handling, and core
 * services for the nRF52840-based medical wearable device. It establishes the foundation
 * for all subsystem operations and maintains system state for medical device compliance.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This module manages critical system functions and should be initialized
 * first during system startup before any other subsystems.
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include "common.h"

/*============================================================================*/
/* System Error Codes                                                         */
/*============================================================================*/

/** @defgroup SystemErrors System Error Codes
 * @brief Error codes specific to system operations
 * @details These error codes are returned by system functions to indicate
 * specific failure conditions in system initialization and operation.
 * @{
 */

/** @brief System operation completed successfully */
#define SYSTEM_OK                    0

/** @brief System initialization failed */
#define SYSTEM_ERROR_INIT           -1

/** @brief Thread creation or management failed */
#define SYSTEM_ERROR_THREAD         -2

/** @brief Memory allocation or management failed */
#define SYSTEM_ERROR_MEMORY         -3

/** @brief Device or hardware access failed */
#define SYSTEM_ERROR_DEVICE         -4

/** @} */ /* End of SystemErrors group */

/*============================================================================*/
/* System State Definitions                                                   */
/*============================================================================*/

/** @defgroup SystemStates System State Enumeration
 * @brief System operational states for medical device lifecycle management
 * @details These states track the system lifecycle from initialization through
 * normal operation to shutdown, providing clear state management for medical
 * device regulatory compliance and safety monitoring.
 * @{
 */

/**
 * @brief System operational states
 * @details Enumeration of all possible system states during device operation,
 * used for state machine management and safety monitoring.
 */
typedef enum {
    SYSTEM_STATE_UNINITIALIZED = 0,  /**< System not yet initialized */
    SYSTEM_STATE_INITIALIZING,       /**< System initialization in progress */
    SYSTEM_STATE_RUNNING,            /**< System operating normally */
    SYSTEM_STATE_ERROR,              /**< System in error state */
    SYSTEM_STATE_SHUTDOWN            /**< System shutdown initiated */
} system_state_t;

/** @} */ /* End of SystemStates group */

/*============================================================================*/
/* System Statistics Structure                                                */
/*============================================================================*/

/** @defgroup SystemStats System Statistics
 * @brief System monitoring and performance statistics
 * @details Structure for tracking system health metrics, performance
 * indicators, and operational statistics for medical device monitoring.
 * @{
 */

/**
 * @brief System statistics and monitoring data
 * @details Comprehensive system statistics for monitoring system health,
 * performance, and operational status in medical device applications.
 */
typedef struct {
    uint32_t uptime_ms;           /**< System uptime in milliseconds */
    uint32_t total_errors;        /**< Total number of errors encountered */
    uint32_t memory_usage;        /**< Current memory usage in bytes */
    system_state_t current_state; /**< Current system operational state */
} system_stats_t;

/** @} */ /* End of SystemStats group */

/*============================================================================*/
/* Public Function Declarations                                               */
/*============================================================================*/

/** @defgroup SystemAPI System API Functions
 * @brief Public API for system management and monitoring
 * @details These functions provide the public interface for system
 * initialization, monitoring, and error handling in medical device applications.
 * @{
 */

/**
 * @brief Initialize the system and all core subsystems
 * @details Performs complete system initialization including diagnostics,
 * configuration management, and core system services. This function must
 * be called first during system startup before any other operations.
 * 
 * @return SYSTEM_OK on successful initialization
 * @retval SYSTEM_ERROR_INIT if initialization fails
 * @retval SYSTEM_ERROR_MEMORY if memory allocation fails
 * @retval SYSTEM_ERROR_DEVICE if device initialization fails
 * 
 * @note This function is not thread-safe and should only be called once
 * during system startup from the main thread.
 */
int system_init(void);

/**
 * @brief Get current system operational state
 * @details Returns the current system state for monitoring and decision
 * making. This function is thread-safe and can be called from any context.
 * 
 * @return Current system state from system_state_t enumeration
 * 
 * @note This function is thread-safe and can be called from interrupt context.
 */
system_state_t system_get_state(void);

/**
 * @brief Get comprehensive system statistics
 * @details Retrieves current system statistics including uptime, error counts,
 * memory usage, and operational state for monitoring and diagnostic purposes.
 * 
 * @param[out] stats Pointer to system_stats_t structure to fill with current statistics
 * @return SYSTEM_OK on success
 * @retval SYSTEM_ERROR_INIT if stats pointer is NULL
 * 
 * @pre stats must point to valid system_stats_t structure
 * @note This function is thread-safe
 */
int system_get_stats(system_stats_t *stats);

/**
 * @brief Handle system error conditions
 * @details Processes system errors, updates error statistics, and takes
 * appropriate corrective actions based on error severity. Critical errors
 * may trigger system state changes or emergency procedures.
 * 
 * @param[in] error_code Error code indicating the type of error that occurred
 * @param[in] context Optional context string providing additional error information (may be NULL)
 * 
 * @note This function is thread-safe and can be called from any context
 * @warning Excessive errors may trigger system state change to SYSTEM_STATE_ERROR
 */
void system_handle_error(int error_code, const char *context);

/**
 * @brief Initiate controlled system shutdown
 * @details Performs orderly system shutdown including configuration saving,
 * diagnostic data preservation, and clean resource deallocation. This function
 * should be called before system power-down or reset.
 * 
 * @note This function may take several seconds to complete
 * @warning After calling this function, system state becomes SYSTEM_STATE_SHUTDOWN
 */
void system_shutdown(void);

/** @} */ /* End of SystemAPI group */

#endif /* SYSTEM_H */