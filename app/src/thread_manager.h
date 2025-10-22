/**
 * @file thread_manager.h
 * @brief Thread management system for NISC Medical Wearable Device
 * @details This header provides centralized thread creation, monitoring, and lifecycle
 * management with safety features appropriate for medical device applications.
 * The thread manager ensures proper thread synchronization, watchdog monitoring,
 * and resource management for regulatory compliance.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This module is critical for system reliability and must be initialized
 * early in the system startup sequence before creating any application threads.
 */

#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "common.h"

/*============================================================================*/
/* Thread Identification                                                      */
/*============================================================================*/

/** @defgroup ThreadIDs Thread Identifiers
 * @brief Unique identifiers for all managed threads in the system
 * @details These identifiers are used to reference specific threads for
 * monitoring, control, and resource management operations.
 * @{
 */

/**
 * @brief Thread identifier enumeration
 * @details Enumeration of all threads managed by the thread manager system,
 * used for thread identification in monitoring and control operations.
 */
typedef enum {
    THREAD_ID_SUPERVISOR = 0,     /**< Safety supervisor thread (highest priority) */
    THREAD_ID_DATA_ACQUISITION,   /**< Sensor data acquisition thread */
    THREAD_ID_DATA_PROCESSING,    /**< Data processing and analysis thread */
    THREAD_ID_COMMUNICATION,      /**< External communication thread */
    THREAD_ID_DIAGNOSTICS,        /**< System diagnostics and maintenance thread */
    THREAD_ID_MAX                 /**< Maximum thread ID (used for bounds checking) */
} thread_id_t;

/** @} */ /* End of ThreadIDs group */

/*============================================================================*/
/* Thread State Management                                                     */
/*============================================================================*/

/** @defgroup ThreadStates Thread State Enumeration
 * @brief Thread operational states for lifecycle management
 * @details These states track thread lifecycle from creation through
 * termination, enabling proper monitoring and error detection.
 * @{
 */

/**
 * @brief Thread operational states
 * @details Enumeration of possible thread states during execution,
 * used for thread monitoring and lifecycle management.
 */
typedef enum {
    THREAD_STATE_STOPPED = 0,     /**< Thread is not running */
    THREAD_STATE_STARTING,        /**< Thread initialization in progress */
    THREAD_STATE_RUNNING,         /**< Thread operating normally */
    THREAD_STATE_SUSPENDED,       /**< Thread temporarily suspended */
    THREAD_STATE_ERROR            /**< Thread encountered an error condition */
} thread_state_t;

/** @} */ /* End of ThreadStates group */

/*============================================================================*/
/* Thread Priority and Resource Configuration                                 */
/*============================================================================*/

/** @defgroup ThreadConfig Thread Configuration Constants
 * @brief Thread priorities and stack size configurations
 * @details These constants define thread scheduling priorities and memory
 * allocations optimized for medical device real-time requirements.
 * @{
 */

/** @brief Supervisor thread priority (highest - safety critical) */
#define THREAD_PRIO_SUPERVISOR       1U

/** @brief Data acquisition thread priority (high - real-time data) */
#define THREAD_PRIO_DATA_ACQ         1U

/** @brief Communication thread priority (medium-high) */
#define THREAD_PRIO_COMMUNICATION    3U

/** @brief Data processing thread priority (medium) */
#define THREAD_PRIO_DATA_PROC        4U

/** @brief Diagnostics thread priority (low - background task) */
#define THREAD_PRIO_DIAGNOSTICS      5U

/** @brief Supervisor thread stack size in bytes */
#define THREAD_STACK_SUPERVISOR      1024U

/** @brief Data acquisition thread stack size in bytes */
#define THREAD_STACK_DATA_ACQ        1536U

/** @brief Communication thread stack size in bytes */
#define THREAD_STACK_COMMUNICATION   1024U

/** @brief Data processing thread stack size in bytes */
#define THREAD_STACK_DATA_PROC       1536U

/** @brief Diagnostics thread stack size in bytes */
#define THREAD_STACK_DIAGNOSTICS     512U

/** @} */ /* End of ThreadConfig group */

/*============================================================================*/
/* Thread Information and Monitoring                                          */
/*============================================================================*/

/** @defgroup ThreadMonitoring Thread Monitoring Structures
 * @brief Structures for thread monitoring and statistics
 * @details These structures provide comprehensive thread monitoring
 * capabilities for medical device safety and performance analysis.
 * @{
 */

/**
 * @brief Thread information and monitoring structure
 * @details Comprehensive information about a managed thread including
 * identification, state, performance metrics, and watchdog monitoring.
 */
typedef struct {
    thread_id_t id;                /**< Thread unique identifier */
    const char *name;              /**< Human-readable thread name */
    thread_state_t state;          /**< Current thread operational state */
    uint32_t run_count;            /**< Number of times thread has executed */
    uint32_t error_count;          /**< Number of errors encountered */
    k_timeout_t watchdog_timeout;  /**< Watchdog timeout period */
    int64_t last_heartbeat;        /**< Timestamp of last heartbeat */
} thread_info_t;

/**
 * @brief Thread entry point function type
 * @details Function signature for thread entry points managed by
 * the thread manager system.
 */
typedef void (*thread_entry_t)(void *arg1, void *arg2, void *arg3);

/** @} */ /* End of ThreadMonitoring group */

/*============================================================================*/
/* Public API Functions                                                       */
/*============================================================================*/

/** @defgroup ThreadManagerAPI Thread Manager API
 * @brief Public interface for thread management operations
 * @details These functions provide the public API for creating, monitoring,
 * and controlling threads in the medical wearable device system.
 * @{
 */

/**
 * @brief Initialize the thread management system
 * @details Sets up the thread manager infrastructure including monitoring
 * structures, watchdog timers, and resource allocation for all managed threads.
 * This function must be called before creating any managed threads.
 * 
 * @return SUCCESS on successful initialization
 * @retval ERROR_NO_MEMORY if memory allocation fails
 * @retval ERROR_INVALID_PARAM if system configuration is invalid
 * 
 * @note This function is not thread-safe and should only be called once
 * during system initialization.
 */
int thread_manager_init(void);

/**
 * @brief Create and start a managed thread
 * @details Creates a new thread with specified entry point and arguments,
 * registers it with the monitoring system, and starts execution with
 * appropriate priority and stack allocation.
 * 
 * @param[in] id Thread identifier from thread_id_t enumeration
 * @param[in] entry Thread entry point function
 * @param[in] arg1 First argument passed to thread function (may be NULL)
 * @param[in] arg2 Second argument passed to thread function (may be NULL)
 * @param[in] arg3 Third argument passed to thread function (may be NULL)
 * 
 * @return SUCCESS on successful thread creation and start
 * @retval ERROR_INVALID_PARAM if id is invalid or entry is NULL
 * @retval ERROR_NO_MEMORY if thread stack allocation fails
 * @retval ERROR_BUSY if thread with specified ID already exists
 * 
 * @pre Thread manager must be initialized via thread_manager_init()
 * @note This function is thread-safe
 */
int thread_manager_create_thread(thread_id_t id, thread_entry_t entry, 
                                void *arg1, void *arg2, void *arg3);

/**
 * @brief Get comprehensive thread information
 * @details Retrieves detailed information about a managed thread including
 * current state, performance metrics, error counts, and watchdog status.
 * 
 * @param[in] id Thread identifier
 * @param[out] info Pointer to thread_info_t structure to fill with thread information
 * 
 * @return SUCCESS on successful information retrieval
 * @retval ERROR_INVALID_PARAM if id is invalid or info is NULL
 * @retval ERROR_NOT_SUPPORTED if thread does not exist
 * 
 * @pre info must point to valid thread_info_t structure
 * @note This function is thread-safe
 */
int thread_manager_get_info(thread_id_t id, thread_info_t *info);

/**
 * @brief Update thread heartbeat (life sign)
 * @details Called by managed threads to indicate they are alive and functioning
 * properly. This updates the watchdog timer and prevents timeout detection.
 * Threads should call this function regularly during their execution.
 * 
 * @param[in] id Thread identifier of the calling thread
 * 
 * @note This function is thread-safe and should be called regularly by threads
 * @warning Failure to call this function regularly may trigger watchdog timeout
 */
void thread_manager_heartbeat(thread_id_t id);

/**
 * @brief Check all threads for watchdog timeouts
 * @details Examines all managed threads to detect those that have not sent
 * heartbeats within their configured timeout periods. This function is
 * typically called by the supervisor thread for safety monitoring.
 * 
 * @return Number of threads that have exceeded their watchdog timeout
 * @retval 0 if all threads are responding normally
 * 
 * @note This function is thread-safe
 * @note Non-zero return value indicates potential thread malfunction
 */
int thread_manager_check_watchdogs(void);

/**
 * @brief Suspend a managed thread
 * @details Temporarily suspends execution of a managed thread while
 * maintaining its monitoring and resource allocation. The thread can
 * be resumed later using thread_manager_resume_thread().
 * 
 * @param[in] id Thread identifier
 * 
 * @return SUCCESS on successful suspension
 * @retval ERROR_INVALID_PARAM if id is invalid
 * @retval ERROR_NOT_SUPPORTED if thread does not exist or cannot be suspended
 * 
 * @note This function is thread-safe
 * @warning Suspending critical threads may affect system safety
 */
int thread_manager_suspend_thread(thread_id_t id);

/**
 * @brief Resume a suspended thread
 * @details Resumes execution of a previously suspended thread, restoring
 * its normal operation and monitoring status.
 * 
 * @param[in] id Thread identifier
 * 
 * @return SUCCESS on successful resume
 * @retval ERROR_INVALID_PARAM if id is invalid
 * @retval ERROR_NOT_SUPPORTED if thread does not exist or is not suspended
 * 
 * @note This function is thread-safe
 */
int thread_manager_resume_thread(thread_id_t id);

/**
 * @brief Get human-readable thread name
 * @details Returns a string containing the human-readable name of the
 * specified thread for logging and diagnostic purposes.
 * 
 * @param[in] id Thread identifier
 * 
 * @return Pointer to null-terminated string containing thread name
 * @retval "Unknown" if thread ID is invalid or thread does not exist
 * 
 * @note Returned pointer is valid for the lifetime of the program
 * @note This function is thread-safe
 */
const char *thread_manager_get_name(thread_id_t id);

/** @} */ /* End of ThreadManagerAPI group */

#endif /* THREAD_MANAGER_H */