#include "thread_manager.h"
#include "diagnostics.h"

/* Thread data structures */
static struct k_thread thread_data[THREAD_ID_MAX];
static k_tid_t thread_ids[THREAD_ID_MAX];
static thread_info_t thread_infos[THREAD_ID_MAX];
static bool manager_initialized = false;

/* Thread stacks */
K_THREAD_STACK_DEFINE(supervisor_stack, THREAD_STACK_SUPERVISOR);
K_THREAD_STACK_DEFINE(data_acq_stack, THREAD_STACK_DATA_ACQ);
K_THREAD_STACK_DEFINE(communication_stack, THREAD_STACK_COMMUNICATION);
K_THREAD_STACK_DEFINE(data_proc_stack, THREAD_STACK_DATA_PROC);
K_THREAD_STACK_DEFINE(diagnostics_stack, THREAD_STACK_DIAGNOSTICS);

/* Thread stack pointers */
static k_thread_stack_t *thread_stacks[THREAD_ID_MAX] = {
    supervisor_stack,
    data_acq_stack,
    communication_stack,
    data_proc_stack,
    diagnostics_stack
};

/* Thread stack sizes */
static size_t thread_stack_sizes[THREAD_ID_MAX] = {
    THREAD_STACK_SUPERVISOR,
    THREAD_STACK_DATA_ACQ,
    THREAD_STACK_COMMUNICATION,
    THREAD_STACK_DATA_PROC,
    THREAD_STACK_DIAGNOSTICS
};

/* Thread priorities */
static int thread_priorities[THREAD_ID_MAX] = {
    THREAD_PRIO_SUPERVISOR,
    THREAD_PRIO_DATA_ACQ,
    THREAD_PRIO_COMMUNICATION,
    THREAD_PRIO_DATA_PROC,
    THREAD_PRIO_DIAGNOSTICS
};

/* Thread names */
static const char *thread_names[THREAD_ID_MAX] = {
    "supervisor",
    "data_acquisition",
    "data_processing",
    "communication",
    "diagnostics"
};

/* Mutex for thread manager */
static struct k_mutex manager_mutex;

int thread_manager_init(void)
{
    if (manager_initialized) {
        return 0;
    }

    k_mutex_init(&manager_mutex);

    /* Initialize thread info structures */
    for (int i = 0; i < THREAD_ID_MAX; i++) {
        thread_infos[i].id = i;
        thread_infos[i].name = thread_names[i];
        thread_infos[i].state = THREAD_STATE_STOPPED;
        thread_infos[i].run_count = 0;
        thread_infos[i].error_count = 0;
        thread_infos[i].watchdog_timeout = K_SECONDS(30);
        thread_infos[i].last_heartbeat = 0;
        thread_ids[i] = NULL;
    }

    manager_initialized = true;
    DIAG_INFO(DIAG_CAT_SYSTEM, "Thread manager initialized");
    return 0;
}

int thread_manager_create_thread(thread_id_t id, thread_entry_t entry, 
                                void *arg1, void *arg2, void *arg3)
{
    if (!manager_initialized || id >= THREAD_ID_MAX || entry == NULL) {
        return -1;
    }

    k_mutex_lock(&manager_mutex, K_FOREVER);

    /* Check if thread already exists */
    if (thread_ids[id] != NULL) {
        k_mutex_unlock(&manager_mutex);
        return -1;
    }

    /* Create thread */
    thread_ids[id] = k_thread_create(&thread_data[id], 
                                     thread_stacks[id],
                                     thread_stack_sizes[id],
                                     entry,
                                     arg1, arg2, arg3,
                                     thread_priorities[id], 0, K_NO_WAIT);

    if (thread_ids[id] == NULL) {
        thread_infos[id].state = THREAD_STATE_ERROR;
        thread_infos[id].error_count++;
        k_mutex_unlock(&manager_mutex);
        DIAG_ERROR(DIAG_CAT_SYSTEM, "Failed to create thread %s", thread_names[id]);
        return -1;
    }

    /* Update thread info */
    thread_infos[id].state = THREAD_STATE_STARTING;
    thread_infos[id].last_heartbeat = k_uptime_get();

    k_mutex_unlock(&manager_mutex);

    DIAG_INFO(DIAG_CAT_SYSTEM, "Created thread %s (ID: %d)", thread_names[id], id);
    return 0;
}

int thread_manager_get_info(thread_id_t id, thread_info_t *info)
{
    if (!manager_initialized || id >= THREAD_ID_MAX || info == NULL) {
        return -1;
    }

    k_mutex_lock(&manager_mutex, K_FOREVER);
    *info = thread_infos[id];
    k_mutex_unlock(&manager_mutex);

    return 0;
}

void thread_manager_heartbeat(thread_id_t id)
{
    if (!manager_initialized || id >= THREAD_ID_MAX) {
        return;
    }

    k_mutex_lock(&manager_mutex, K_FOREVER);
    
    thread_infos[id].last_heartbeat = k_uptime_get();
    thread_infos[id].run_count++;
    
    /* Update state to running if it was starting */
    if (thread_infos[id].state == THREAD_STATE_STARTING) {
        thread_infos[id].state = THREAD_STATE_RUNNING;
        DIAG_INFO(DIAG_CAT_SYSTEM, "Thread %s is now running", thread_names[id]);
    }
    
    k_mutex_unlock(&manager_mutex);
}

int thread_manager_check_watchdogs(void)
{
    if (!manager_initialized) {
        return 0;
    }

    int timeout_count = 0;
    int64_t current_time = k_uptime_get();

    k_mutex_lock(&manager_mutex, K_FOREVER);

    for (int i = 0; i < THREAD_ID_MAX; i++) {
        if (thread_infos[i].state == THREAD_STATE_RUNNING) {
            int64_t elapsed = current_time - thread_infos[i].last_heartbeat;
            int64_t timeout_ms = k_ticks_to_ms_floor64(thread_infos[i].watchdog_timeout.ticks);
            
            if (elapsed > timeout_ms) {
                thread_infos[i].error_count++;
                timeout_count++;
                DIAG_WARNING(DIAG_CAT_SYSTEM, "Watchdog timeout for thread %s", 
                           thread_names[i]);
            }
        }
    }

    k_mutex_unlock(&manager_mutex);
    return timeout_count;
}

int thread_manager_suspend_thread(thread_id_t id)
{
    if (!manager_initialized || id >= THREAD_ID_MAX || thread_ids[id] == NULL) {
        return -1;
    }

    k_mutex_lock(&manager_mutex, K_FOREVER);
    
    k_thread_suspend(thread_ids[id]);
    thread_infos[id].state = THREAD_STATE_SUSPENDED;
    
    k_mutex_unlock(&manager_mutex);

    DIAG_INFO(DIAG_CAT_SYSTEM, "Thread %s suspended", thread_names[id]);
    return 0;
}

int thread_manager_resume_thread(thread_id_t id)
{
    if (!manager_initialized || id >= THREAD_ID_MAX || thread_ids[id] == NULL) {
        return -1;
    }

    k_mutex_lock(&manager_mutex, K_FOREVER);
    
    k_thread_resume(thread_ids[id]);
    thread_infos[id].state = THREAD_STATE_RUNNING;
    thread_infos[id].last_heartbeat = k_uptime_get();
    
    k_mutex_unlock(&manager_mutex);

    DIAG_INFO(DIAG_CAT_SYSTEM, "Thread %s resumed", thread_names[id]);
    return 0;
}

const char *thread_manager_get_name(thread_id_t id)
{
    if (id >= THREAD_ID_MAX) {
        return "unknown";
    }
    return thread_names[id];
}