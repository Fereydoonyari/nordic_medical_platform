/**
 * @file diagnostics.h
 * @brief Comprehensive diagnostic and logging system for NISC Medical Wearable Device
 * @details This header provides comprehensive logging, error tracking, and diagnostic
 * capabilities required for medical device development, regulatory compliance (IEC 62304),
 * and clinical validation. The system enables real-time monitoring, post-market
 * surveillance, and quality management for medical device software.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This module implements medical device logging standards and must be
 * initialized early in the system startup sequence.
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include "common.h"

/*============================================================================*/
/* Log Level Definitions                                                      */
/*============================================================================*/

/** @defgroup LogLevels Diagnostic Log Levels
 * @brief Hierarchical log levels for message classification
 * @details These log levels provide graduated severity classification
 * for diagnostic messages, enabling appropriate filtering and response
 * in medical device applications.
 * @{
 */

/**
 * @brief Diagnostic log level enumeration
 * @details Hierarchical log levels from debug information to critical
 * system alerts, designed for medical device regulatory compliance.
 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,          /**< Detailed debugging information */
    LOG_LEVEL_INFO,               /**< Informational messages */
    LOG_LEVEL_WARNING,            /**< Warning conditions that need attention */
    LOG_LEVEL_ERROR,              /**< Error conditions that affect operation */
    LOG_LEVEL_CRITICAL            /**< Critical conditions requiring immediate action */
} log_level_t;

/** @} */ /* End of LogLevels group */

/*============================================================================*/
/* Diagnostic Categories                                                      */
/*============================================================================*/

/** @defgroup DiagCategories Diagnostic Categories
 * @brief Functional area classifications for diagnostic messages
 * @details These categories organize diagnostic messages by functional
 * area, enabling targeted analysis and troubleshooting in medical
 * device systems.
 * @{
 */

/**
 * @brief Diagnostic category enumeration
 * @details Categories for organizing diagnostic messages by functional
 * areas of the medical device system for improved analysis and debugging.
 */
typedef enum {
    DIAG_CAT_SYSTEM = 0,          /**< Core system operations and initialization */
    DIAG_CAT_SENSOR,              /**< Sensor data acquisition and processing */
    DIAG_CAT_COMMUNICATION,       /**< External communication and networking */
    DIAG_CAT_POWER,               /**< Power management and battery monitoring */
    DIAG_CAT_SAFETY,              /**< Safety-critical operations and alerts */
    DIAG_CAT_PERFORMANCE,         /**< Performance monitoring and optimization */
    DIAG_CAT_MAX                  /**< Maximum category value for bounds checking */
} diag_category_t;

/** @} */ /* End of DiagCategories group */

/*============================================================================*/
/* Diagnostic Data Structures                                                 */
/*============================================================================*/

/** @defgroup DiagStructures Diagnostic Data Structures
 * @brief Data structures for logging and error tracking
 * @details These structures provide comprehensive data organization
 * for medical device diagnostic information and regulatory compliance.
 * @{
 */

/**
 * @brief Individual log entry structure
 * @details Complete log entry with timestamp, categorization, and
 * contextual information for medical device traceability.
 */
typedef struct {
    uint32_t timestamp;           /**< System timestamp when entry was created */
    log_level_t level;            /**< Severity level of the log entry */
    diag_category_t category;     /**< Functional category of the log entry */
    uint32_t thread_id;           /**< Thread ID that generated the entry */
    char message[128];            /**< Human-readable log message */
    uint32_t data;                /**< Additional numeric data associated with entry */
} log_entry_t;

/**
 * @brief Comprehensive diagnostic statistics
 * @details System-wide diagnostic statistics for monitoring system
 * health and diagnostic system performance in medical applications.
 */
typedef struct {
    uint32_t total_entries;                            /**< Total number of log entries processed */
    uint32_t entries_by_level[LOG_LEVEL_CRITICAL + 1]; /**< Count of entries per severity level */
    uint32_t entries_by_category[DIAG_CAT_MAX];        /**< Count of entries per functional category */
    uint32_t dropped_entries;                          /**< Number of entries lost due to overflow */
    uint32_t memory_usage;                             /**< Current memory usage by diagnostic system */
} diag_stats_t;

/**
 * @brief Error tracking and analysis structure
 * @details Detailed error tracking for recurring issues and trend
 * analysis in medical device quality management.
 */
typedef struct {
    uint32_t error_code;          /**< Unique error code identifier */
    uint32_t occurrence_count;    /**< Number of times this error has occurred */
    uint32_t first_occurrence;    /**< Timestamp of first occurrence */
    uint32_t last_occurrence;     /**< Timestamp of most recent occurrence */
    diag_category_t category;     /**< Functional category where error occurred */
} error_record_t;

/** @} */ /* End of DiagStructures group */

/*============================================================================*/
/* Public API Functions                                                       */
/*============================================================================*/

/** @defgroup DiagnosticsAPI Diagnostics System API
 * @brief Public interface for diagnostic and logging operations
 * @details These functions provide comprehensive diagnostic capabilities
 * for medical device development, validation, and regulatory compliance.
 * @{
 */

/**
 * @brief Initialize the diagnostic system
 * @details Sets up the diagnostic logging infrastructure including memory
 * allocation, log buffers, error tracking systems, and default configurations.
 * This function must be called early in system initialization.
 * 
 * @return SUCCESS on successful initialization
 * @retval ERROR_NO_MEMORY if memory allocation fails
 * @retval ERROR_INVALID_PARAM if system configuration is invalid
 * 
 * @note This function is not thread-safe and should only be called once
 * @note Must be called before any other diagnostic functions
 */
int diagnostics_init(void);

/**
 * @brief Log a diagnostic message with specified level and category
 * @details Creates a new diagnostic log entry with timestamp, thread context,
 * and specified categorization. This is the primary logging interface for
 * the medical device system.
 * 
 * @param[in] level Severity level of the message
 * @param[in] category Functional category of the message
 * @param[in] format Printf-style format string for the message
 * @param[in] ... Variable arguments for format string
 * 
 * @note This function is thread-safe and can be called from any context
 * @note Message length is limited to 127 characters plus null terminator
 */
void diagnostics_log(log_level_t level, diag_category_t category, 
                    const char *format, ...);

/**
 * @brief Log an error with additional contextual data
 * @details Records a structured error entry with error code, category,
 * additional numeric data, and context string. Enables detailed error
 * tracking and trend analysis for medical device quality management.
 * 
 * @param[in] error_code Unique error code identifier
 * @param[in] category Functional category where error occurred
 * @param[in] data Additional numeric data associated with error
 * @param[in] context Optional context string describing error circumstances (may be NULL)
 * 
 * @note This function is thread-safe
 * @note Error codes should be unique within each category for proper tracking
 */
void diagnostics_log_error(uint32_t error_code, diag_category_t category, 
                          uint32_t data, const char *context);

/**
 * @brief Retrieve comprehensive diagnostic statistics
 * @details Provides detailed statistics about diagnostic system usage,
 * including entry counts by level and category, memory usage, and
 * system performance metrics for monitoring and analysis.
 * 
 * @param[out] stats Pointer to diag_stats_t structure to fill with statistics
 * 
 * @return SUCCESS on successful statistics retrieval
 * @retval ERROR_INVALID_PARAM if stats pointer is NULL
 * 
 * @pre stats must point to valid diag_stats_t structure
 * @note This function is thread-safe
 */
int diagnostics_get_stats(diag_stats_t *stats);

/**
 * @brief Retrieve error occurrence records for analysis
 * @details Returns detailed error tracking information including occurrence
 * counts, timestamps, and categorization for quality management and
 * trend analysis in medical device post-market surveillance.
 * 
 * @param[out] records Array to store error_record_t structures
 * @param[in] max_records Maximum number of records the array can hold
 * @param[out] actual_count Pointer to store actual number of records returned
 * 
 * @return SUCCESS on successful retrieval
 * @retval ERROR_INVALID_PARAM if records is NULL or actual_count is NULL
 * @retval ERROR_NOT_SUPPORTED if no error records are available
 * 
 * @pre records must point to valid array of error_record_t structures
 * @pre actual_count must point to valid uint32_t variable
 * @note This function is thread-safe
 */
int diagnostics_get_error_records(error_record_t *records, size_t max_records, 
                                 size_t *actual_count);

/**
 * @brief Clear all diagnostic logs and reset statistics
 * @details Removes all stored log entries and resets diagnostic statistics
 * to initial state. This function may be used for maintenance or when
 * storage limits are reached.
 * 
 * @warning This operation cannot be undone - all diagnostic history is lost
 * @note This function is thread-safe but may block briefly during clearing
 */
void diagnostics_clear_logs(void);

/**
 * @brief Set minimum log level for message filtering
 * @details Configures the minimum severity level for log messages to be
 * processed. Messages below this level are ignored, enabling performance
 * optimization and storage management.
 * 
 * @param[in] level Minimum log level to process and store
 * 
 * @note This function is thread-safe
 * @note Default level is LOG_LEVEL_INFO if not explicitly set
 */
void diagnostics_set_log_level(log_level_t level);

/**
 * @brief Enable or disable logging for specific categories
 * @details Controls whether messages from specific functional categories
 * are processed and stored, enabling targeted diagnostic collection.
 * 
 * @param[in] category Diagnostic category to configure
 * @param[in] enabled true to enable logging for category, false to disable
 * 
 * @note This function is thread-safe
 * @note All categories are enabled by default
 */
void diagnostics_set_category_enabled(diag_category_t category, bool enabled);

/**
 * @brief Output current logs to console for immediate review
 * @details Prints recent log entries to the console output for immediate
 * diagnostic review and debugging. Useful for development and field
 * troubleshooting of medical device issues.
 * 
 * @param[in] max_entries Maximum number of recent entries to output
 * 
 * @note This function may produce significant console output
 * @note Output format includes timestamp, level, category, and message
 */
void diagnostics_dump_logs(size_t max_entries);

/**
 * @brief Get human-readable name for diagnostic category
 * @details Returns a string representation of the diagnostic category
 * for user interfaces, reports, and debugging output.
 * 
 * @param[in] category Diagnostic category to get name for
 * 
 * @return Pointer to null-terminated string containing category name
 * @retval "Unknown" if category is invalid
 * 
 * @note Returned pointer is valid for the lifetime of the program
 * @note This function is thread-safe
 */
const char *diagnostics_get_category_name(diag_category_t category);

/**
 * @brief Get human-readable name for log level
 * @details Returns a string representation of the log level for
 * user interfaces, reports, and debugging output.
 * 
 * @param[in] level Log level to get name for
 * 
 * @return Pointer to null-terminated string containing level name
 * @retval "Unknown" if level is invalid
 * 
 * @note Returned pointer is valid for the lifetime of the program
 * @note This function is thread-safe
 */
const char *diagnostics_get_level_name(log_level_t level);

/** @} */ /* End of DiagnosticsAPI group */

/*============================================================================*/
/* Convenience Macros                                                         */
/*============================================================================*/

/** @defgroup DiagMacros Diagnostic Convenience Macros
 * @brief Simplified macros for common diagnostic operations
 * @details These macros provide a simplified interface for common logging
 * operations, reducing code verbosity while maintaining full functionality.
 * @{
 */

/**
 * @brief Log a debug message
 * @param cat Diagnostic category
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
#define DIAG_DEBUG(cat, fmt, ...) \
    diagnostics_log(LOG_LEVEL_DEBUG, cat, fmt, ##__VA_ARGS__)

/**
 * @brief Log an informational message
 * @param cat Diagnostic category
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
#define DIAG_INFO(cat, fmt, ...) \
    diagnostics_log(LOG_LEVEL_INFO, cat, fmt, ##__VA_ARGS__)

/**
 * @brief Log a warning message
 * @param cat Diagnostic category
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
#define DIAG_WARNING(cat, fmt, ...) \
    diagnostics_log(LOG_LEVEL_WARNING, cat, fmt, ##__VA_ARGS__)

/**
 * @brief Log an error message
 * @param cat Diagnostic category
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
#define DIAG_ERROR(cat, fmt, ...) \
    diagnostics_log(LOG_LEVEL_ERROR, cat, fmt, ##__VA_ARGS__)

/**
 * @brief Log a critical message
 * @param cat Diagnostic category
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
#define DIAG_CRITICAL(cat, fmt, ...) \
    diagnostics_log(LOG_LEVEL_CRITICAL, cat, fmt, ##__VA_ARGS__)

/** @} */ /* End of DiagMacros group */

#endif /* DIAGNOSTICS_H */