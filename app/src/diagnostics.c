/**
 * @file diagnostics.c
 * @brief Simple thread-safe diagnostic system implementation
 * @details Lightweight diagnostic system that prints messages immediately
 * without complex storage, priority systems, or memory overhead.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note Simple design: thread-safe immediate printing with basic filtering
 */

#include "diagnostics.h"
#include "common.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/*============================================================================*/
/* Configuration Constants                                                    */
/*============================================================================*/

/** @brief Maximum message length */
#define MAX_MESSAGE_LENGTH    128U

/** @brief Maximum error records for tracking */
#define MAX_ERROR_RECORDS     8U

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

/** @brief Thread synchronization for console output */
static struct k_mutex print_mutex;

/** @brief Error tracking storage (minimal) */
static error_record_t error_records[MAX_ERROR_RECORDS];
static size_t error_count = 0U;

/** @brief Configuration settings */
static log_level_t min_log_level = LOG_LEVEL_INFO;
static bool category_enabled[DIAG_CAT_MAX];

/** @brief Simple statistics (no storage overhead) */
static volatile uint32_t total_messages = 0U;
static volatile uint32_t dropped_messages = 0U;

/** @brief Category name lookup table */
static const char *category_names[DIAG_CAT_MAX] = {
    "SYS", "SNS", "COM", "PWR", "SAF", "PRF"
};

/** @brief Log level name lookup table */
static const char *level_names[LOG_LEVEL_CRITICAL + 1] = {
    "DBG", "INF", "WRN", "ERR", "CRT"
};

/*============================================================================*/
/* Public Function Implementations                                           */
/*============================================================================*/

/**
 * @brief Initialize simple diagnostic system
 */
int diagnostics_init(void)
{
    /* Initialize print mutex for thread safety */
    k_mutex_init(&print_mutex);

    /* Enable all categories by default */
    for (int i = 0; i < DIAG_CAT_MAX; i++) {
        category_enabled[i] = true;
    }

    /* Clear error records */
    memset(error_records, 0, sizeof(error_records));
    error_count = 0U;
    total_messages = 0U;
    dropped_messages = 0U;

    printk("Simple diagnostics system initialized\n");
    return SUCCESS;
}

/**
 * @brief Simple thread-safe logging with immediate output
 */
void diagnostics_log(log_level_t level, diag_category_t category, 
                    const char *format, ...)
{
    /* Fast rejection for disabled messages */
    if (level > LOG_LEVEL_CRITICAL || level < 0 || 
        category >= DIAG_CAT_MAX || category < 0 ||
        level < min_log_level || 
        !category_enabled[category] || format == NULL) {
        return;
    }

    /* Format message */
    char message[MAX_MESSAGE_LENGTH];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    /* Truncate if too long */
    if (len >= (int)sizeof(message)) {
        message[sizeof(message) - 1] = '\0';
    }

    /* Thread-safe console output - but avoid blocking in ISR context */
    if (k_is_in_isr()) {
        /* In ISR context - just print directly without mutex */
        printk("[%s:%s] %s\n", level_names[level], category_names[category], message);
        total_messages++;
    } else {
        /* Normal thread context - use mutex protection */
        k_mutex_lock(&print_mutex, K_FOREVER);
        printk("[%s:%s] %s\n", level_names[level], category_names[category], message);
        total_messages++;
        k_mutex_unlock(&print_mutex);
    }
}

/**
 * @brief Simple error logging with tracking
 */
void diagnostics_log_error(uint32_t error_code, diag_category_t category, 
                          uint32_t data, const char *context)
{
    /* Validate parameters */
    if (category >= DIAG_CAT_MAX || category < 0) {
        return;
    }

    /* Track error occurrence */
    if (!k_is_in_isr()) {
        k_mutex_lock(&print_mutex, K_FOREVER);
        
        error_record_t *record = NULL;
        for (size_t i = 0; i < error_count; i++) {
            if (error_records[i].error_code == error_code && 
                error_records[i].category == category) {
                record = &error_records[i];
                break;
            }
        }

        if (record == NULL && error_count < MAX_ERROR_RECORDS) {
            record = &error_records[error_count++];
            record->error_code = error_code;
            record->category = category;
            record->occurrence_count = 0U;
            record->first_occurrence = k_uptime_get_32();
        }

        if (record != NULL) {
            record->occurrence_count++;
            record->last_occurrence = k_uptime_get_32();
        }
        
        k_mutex_unlock(&print_mutex);
    }

    /* Print error immediately */
    if (context != NULL) {
        printk("[ERR:%s] Error %u: %s (data: 0x%x)\n", 
               category_names[category], error_code, context, data);
    } else {
        printk("[ERR:%s] Error %u (data: 0x%x)\n", 
               category_names[category], error_code, data);
    }
    
    total_messages++;
}

/**
 * @brief Get simple diagnostic statistics
 */
int diagnostics_get_stats(diag_stats_t *stats)
{
    if (stats == NULL) {
        return ERROR_INVALID_PARAM;
    }

    /* Clear and fill basic stats */
    memset(stats, 0, sizeof(diag_stats_t));
    
    k_mutex_lock(&print_mutex, K_FOREVER);
    stats->total_entries = total_messages;
    stats->dropped_entries = dropped_messages;
    stats->memory_usage = sizeof(error_records); /* Minimal memory usage */
    k_mutex_unlock(&print_mutex);

    return SUCCESS;
}

/**
 * @brief Get error records for analysis
 */
int diagnostics_get_error_records(error_record_t *records, size_t max_records, 
                                 size_t *actual_count)
{
    if (records == NULL || actual_count == NULL) {
        return ERROR_INVALID_PARAM;
    }

    k_mutex_lock(&print_mutex, K_FOREVER);
    
    size_t count = (error_count < max_records) ? error_count : max_records;
    if (count > 0U) {
        memcpy(records, error_records, count * sizeof(error_record_t));
    }
    *actual_count = count;
    
    k_mutex_unlock(&print_mutex);

    return (count > 0U) ? SUCCESS : ERROR_NOT_SUPPORTED;
}

/**
 * @brief Clear error records (no log storage to clear)
 */
void diagnostics_clear_logs(void)
{
    k_mutex_lock(&print_mutex, K_FOREVER);
    
    memset(error_records, 0, sizeof(error_records));
    error_count = 0U;
    total_messages = 0U;
    dropped_messages = 0U;
    
    k_mutex_unlock(&print_mutex);

    printk("Diagnostic error records cleared\n");
}

/**
 * @brief Set minimum log level
 */
void diagnostics_set_log_level(log_level_t level)
{
    if (level <= LOG_LEVEL_CRITICAL) {
        min_log_level = level;
        printk("Log level set to %s\n", level_names[level]);
    }
}

/**
 * @brief Enable or disable logging for specific categories
 */
void diagnostics_set_category_enabled(diag_category_t category, bool enabled)
{
    if (category < DIAG_CAT_MAX) {
        category_enabled[category] = enabled;
        printk("Category %s %s\n", category_names[category], 
               enabled ? "enabled" : "disabled");
    }
}

/**
 * @brief Dump system status (no stored logs, just current state)
 */
void diagnostics_dump_logs(size_t max_entries)
{
    UNUSED(max_entries); /* Not used in simple implementation */

    k_mutex_lock(&print_mutex, K_FOREVER);

    printk("\n=== SIMPLE DIAGNOSTIC STATUS ===\n");
    printk("Total messages printed: %u\n", total_messages);
    printk("Dropped messages: %u\n", dropped_messages);
    printk("Memory usage: %zu bytes (error tracking only)\n", sizeof(error_records));
    printk("\n=== ERROR SUMMARY ===\n");
    
    for (size_t i = 0; i < error_count; i++) {
        error_record_t *record = &error_records[i];
        printk("Error %u (%s): %u occurrences, first: %u, last: %u\n",
               record->error_code, category_names[record->category],
               record->occurrence_count, record->first_occurrence,
               record->last_occurrence);
    }
    
    if (error_count == 0) {
        printk("No errors recorded\n");
    }
    
    printk("========================\n\n");

    k_mutex_unlock(&print_mutex);
}

/**
 * @brief Get category name
 */
const char *diagnostics_get_category_name(diag_category_t category)
{
    if (category < DIAG_CAT_MAX) {
        return category_names[category];
    }
    return "UNK";
}

/**
 * @brief Get log level name
 */
const char *diagnostics_get_level_name(log_level_t level)
{
    if (level <= LOG_LEVEL_CRITICAL) {
        return level_names[level];
    }
    return "UNK";
}