#include "system.h"
#include "diagnostics.h"
#include "config.h"

static system_state_t system_state = SYSTEM_STATE_UNINITIALIZED;
static system_stats_t system_statistics = {0};
static struct k_mutex system_mutex;

int system_init(void)
{
    int ret;

    printk("*** SYSTEM_INIT() CALLED ***\n");

    /* Initialize mutex */
    k_mutex_init(&system_mutex);

    k_mutex_lock(&system_mutex, K_FOREVER);
    system_state = SYSTEM_STATE_INITIALIZING;
    k_mutex_unlock(&system_mutex);

    /* Initialize diagnostics first */
    ret = diagnostics_init();
    if (ret != 0) {
        system_handle_error(SYSTEM_ERROR_INIT, "Failed to initialize diagnostics");
        return SYSTEM_ERROR_INIT;
    }

    /* Initialize configuration system */
    ret = config_init();
    if (ret != 0) {
        system_handle_error(SYSTEM_ERROR_INIT, "Failed to initialize config");
        return SYSTEM_ERROR_INIT;
    }

    /* Load configuration */
    ret = config_load();
    if (ret != 0) {
        DIAG_WARNING(DIAG_CAT_SYSTEM, "Failed to load config, using defaults");
    }

    /* Apply diagnostic level from configuration */
    uint32_t diagnostic_level;
    ret = config_get_uint32(CONFIG_KEY_DIAGNOSTIC_LEVEL, &diagnostic_level);
    if (ret == CONFIG_OK) {
        diagnostics_set_log_level((log_level_t)diagnostic_level);
        printk("*** DIAGNOSTIC LEVEL SUCCESSFULLY SET TO: %u ***\n", diagnostic_level);
        DIAG_INFO(DIAG_CAT_SYSTEM, "Applied diagnostic level: %u", diagnostic_level);
        
        /* Test all log levels immediately */
        DIAG_DEBUG(DIAG_CAT_SYSTEM, "TEST DEBUG MESSAGE - Level 0");
        DIAG_INFO(DIAG_CAT_SYSTEM, "TEST INFO MESSAGE - Level 1");
        DIAG_WARNING(DIAG_CAT_SYSTEM, "TEST WARNING MESSAGE - Level 2");
    } else {
        printk("*** FAILED TO GET DIAGNOSTIC LEVEL: ret=%d ***\n", ret);
        DIAG_WARNING(DIAG_CAT_SYSTEM, "Failed to get diagnostic level from config, using default");
    }

    /* Initialize system statistics */
    k_mutex_lock(&system_mutex, K_FOREVER);
    system_statistics.uptime_ms = 0;
    system_statistics.total_errors = 0;
    system_statistics.memory_usage = 0;
    system_statistics.current_state = SYSTEM_STATE_RUNNING;
    system_state = SYSTEM_STATE_RUNNING;
    k_mutex_unlock(&system_mutex);

    DIAG_INFO(DIAG_CAT_SYSTEM, "System initialized successfully");
    return SYSTEM_OK;
}

system_state_t system_get_state(void)
{
    system_state_t state;
    
    k_mutex_lock(&system_mutex, K_FOREVER);
    state = system_state;
    k_mutex_unlock(&system_mutex);
    
    return state;
}

int system_get_stats(system_stats_t *stats)
{
    if (stats == NULL) {
        return SYSTEM_ERROR_INIT;
    }

    k_mutex_lock(&system_mutex, K_FOREVER);
    
    /* Update uptime */
    system_statistics.uptime_ms = k_uptime_get();
    system_statistics.current_state = system_state;
    
    /* Copy statistics */
    *stats = system_statistics;
    
    k_mutex_unlock(&system_mutex);

    return SYSTEM_OK;
}

void system_handle_error(int error_code, const char *context)
{
    k_mutex_lock(&system_mutex, K_FOREVER);
    system_statistics.total_errors++;
    
    if (system_statistics.total_errors > 10 && system_state != SYSTEM_STATE_ERROR) {
        system_state = SYSTEM_STATE_ERROR;
        DIAG_CRITICAL(DIAG_CAT_SYSTEM, "Too many errors, entering error state");
    }
    k_mutex_unlock(&system_mutex);

    DIAG_ERROR(DIAG_CAT_SYSTEM, "System error %d: %s", error_code, 
               context ? context : "Unknown error");

    /* Log error details */
    diagnostics_log_error(error_code, DIAG_CAT_SYSTEM, 0, context);
}

void system_shutdown(void)
{
    DIAG_INFO(DIAG_CAT_SYSTEM, "System shutdown initiated");

    k_mutex_lock(&system_mutex, K_FOREVER);
    system_state = SYSTEM_STATE_SHUTDOWN;
    k_mutex_unlock(&system_mutex);

    /* Save configuration */
    config_save();

    /* Dump final diagnostics */
    diagnostics_dump_logs(50);

    DIAG_INFO(DIAG_CAT_SYSTEM, "System shutdown complete");
}

void system_clear_errors(void)
{
    DIAG_INFO(DIAG_CAT_SYSTEM, "Clearing system error counters");
    
    k_mutex_lock(&system_mutex, K_FOREVER);
    system_stats.total_errors = 0;
    system_stats.current_state = SYSTEM_STATE_NORMAL;
    k_mutex_unlock(&system_mutex);
    
    DIAG_INFO(DIAG_CAT_SYSTEM, "System error counters cleared");
}