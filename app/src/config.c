#include "config.h"
#include <string.h>

/* Configuration storage */
static config_value_t config_values[CONFIG_KEY_MAX];
static bool config_initialized = false;
static struct k_mutex config_mutex;

/* Validation functions */
static bool validate_device_id(const config_value_t *value);
static bool validate_sampling_rate(const config_value_t *value);
static bool validate_alert_thresholds(const config_value_t *value);
static bool validate_communication_interval(const config_value_t *value);
static bool validate_power_management(const config_value_t *value);
static bool validate_safety_limits(const config_value_t *value);
static bool validate_diagnostic_level(const config_value_t *value);

/* Configuration entries definition */
static const config_entry_t config_entries[CONFIG_KEY_MAX] = {
    {
        .key = CONFIG_KEY_DEVICE_ID,
        .name = "device_id",
        .type = CONFIG_TYPE_UINT32,
        .default_value = {.type = CONFIG_TYPE_UINT32, .size = sizeof(uint32_t), .value.uint32_val = 0x12345678},
        .validator = validate_device_id,
        .read_only = true,
        .requires_restart = false
    },
    {
        .key = CONFIG_KEY_SAMPLING_RATE,
        .name = "sampling_rate_hz",
        .type = CONFIG_TYPE_UINT32,
        .default_value = {.type = CONFIG_TYPE_UINT32, .size = sizeof(uint32_t), .value.uint32_val = 100},
        .validator = validate_sampling_rate,
        .read_only = false,
        .requires_restart = true
    },
    {
        .key = CONFIG_KEY_ALERT_THRESHOLDS,
        .name = "alert_thresholds",
        .type = CONFIG_TYPE_BLOB,
        .default_value = {.type = CONFIG_TYPE_BLOB, .size = 16, .value.blob_val = {80, 0, 0, 0, 100, 0, 0, 0, 150, 0, 0, 0, 95, 0, 0, 0}},
        .validator = validate_alert_thresholds,
        .read_only = false,
        .requires_restart = false
    },
    {
        .key = CONFIG_KEY_COMMUNICATION_INTERVAL,
        .name = "comm_interval_ms",
        .type = CONFIG_TYPE_UINT32,
        .default_value = {.type = CONFIG_TYPE_UINT32, .size = sizeof(uint32_t), .value.uint32_val = 5000},
        .validator = validate_communication_interval,
        .read_only = false,
        .requires_restart = false
    },
    {
        .key = CONFIG_KEY_POWER_MANAGEMENT,
        .name = "power_mgmt_enabled",
        .type = CONFIG_TYPE_BOOL,
        .default_value = {.type = CONFIG_TYPE_BOOL, .size = sizeof(bool), .value.bool_val = true},
        .validator = validate_power_management,
        .read_only = false,
        .requires_restart = true
    },
    {
        .key = CONFIG_KEY_SAFETY_LIMITS,
        .name = "safety_limits",
        .type = CONFIG_TYPE_BLOB,
        .default_value = {.type = CONFIG_TYPE_BLOB, .size = 8, .value.blob_val = {10, 0, 0, 0, 30, 0, 0, 0}}, /* Battery 10%, Signal 30% */
        .validator = validate_safety_limits,
        .read_only = false,
        .requires_restart = false
    },
    {
        .key = CONFIG_KEY_CALIBRATION_DATA,
        .name = "calibration_data",
        .type = CONFIG_TYPE_BLOB,
        .default_value = {.type = CONFIG_TYPE_BLOB, .size = 32, .value.blob_val = {0}},
        .validator = NULL, /* No validation for calibration data */
        .read_only = false,
        .requires_restart = true
    },
    {
        .key = CONFIG_KEY_DIAGNOSTIC_LEVEL,
        .name = "diagnostic_level",
        .type = CONFIG_TYPE_UINT32,
        .default_value = {.type = CONFIG_TYPE_UINT32, .size = sizeof(uint32_t), .value.uint32_val = 0}, /* LOG_LEVEL_DEBUG */
        .validator = validate_diagnostic_level,
        .read_only = false,
        .requires_restart = false
    }
};

/* Key names */
static const char *key_names[CONFIG_KEY_MAX] = {
    "device_id",
    "sampling_rate",
    "alert_thresholds", 
    "communication_interval",
    "power_management",
    "safety_limits",
    "calibration_data",
    "diagnostic_level"
};

int config_init(void)
{
    if (config_initialized) {
        return CONFIG_OK;
    }

    k_mutex_init(&config_mutex);

    /* Initialize with default values */
    for (int i = 0; i < CONFIG_KEY_MAX; i++) {
        config_values[i] = config_entries[i].default_value;
    }

    config_initialized = true;
    return CONFIG_OK;
}

int config_load(void)
{
    /* In a real implementation, this would load from persistent storage */
    /* For now, just use defaults */
    return CONFIG_OK;
}

int config_save(void)
{
    /* In a real implementation, this would save to persistent storage */
    /* For now, just return success */
    return CONFIG_OK;
}

int config_get(config_key_t key, config_value_t *value)
{
    if (!config_initialized || key >= CONFIG_KEY_MAX || value == NULL) {
        return CONFIG_ERROR_INVALID;
    }

    k_mutex_lock(&config_mutex, K_FOREVER);
    *value = config_values[key];
    k_mutex_unlock(&config_mutex);

    return CONFIG_OK;
}

int config_set(config_key_t key, const config_value_t *value)
{
    if (!config_initialized || key >= CONFIG_KEY_MAX || value == NULL) {
        return CONFIG_ERROR_INVALID;
    }

    const config_entry_t *entry = &config_entries[key];

    /* Check if read-only */
    if (entry->read_only) {
        return CONFIG_ERROR_READ_ONLY;
    }

    /* Check type compatibility */
    if (value->type != entry->type) {
        return CONFIG_ERROR_INVALID;
    }

    /* Validate value */
    if (entry->validator && !entry->validator(value)) {
        return CONFIG_ERROR_VALIDATION;
    }

    k_mutex_lock(&config_mutex, K_FOREVER);
    config_values[key] = *value;
    k_mutex_unlock(&config_mutex);

    return CONFIG_OK;
}

int config_get_uint32(config_key_t key, uint32_t *value)
{
    config_value_t config_val;
    int ret = config_get(key, &config_val);
    
    if (ret == CONFIG_OK) {
        if (config_val.type == CONFIG_TYPE_UINT32) {
            *value = config_val.value.uint32_val;
        } else {
            ret = CONFIG_ERROR_INVALID;
        }
    }
    
    return ret;
}

int config_set_uint32(config_key_t key, uint32_t value)
{
    config_value_t config_val = {
        .type = CONFIG_TYPE_UINT32,
        .size = sizeof(uint32_t),
        .value.uint32_val = value
    };
    
    return config_set(key, &config_val);
}

int config_get_float(config_key_t key, float *value)
{
    config_value_t config_val;
    int ret = config_get(key, &config_val);
    
    if (ret == CONFIG_OK) {
        if (config_val.type == CONFIG_TYPE_FLOAT) {
            *value = config_val.value.float_val;
        } else {
            ret = CONFIG_ERROR_INVALID;
        }
    }
    
    return ret;
}

int config_set_float(config_key_t key, float value)
{
    config_value_t config_val = {
        .type = CONFIG_TYPE_FLOAT,
        .size = sizeof(float),
        .value.float_val = value
    };
    
    return config_set(key, &config_val);
}

int config_get_bool(config_key_t key, bool *value)
{
    config_value_t config_val;
    int ret = config_get(key, &config_val);
    
    if (ret == CONFIG_OK) {
        if (config_val.type == CONFIG_TYPE_BOOL) {
            *value = config_val.value.bool_val;
        } else {
            ret = CONFIG_ERROR_INVALID;
        }
    }
    
    return ret;
}

int config_set_bool(config_key_t key, bool value)
{
    config_value_t config_val = {
        .type = CONFIG_TYPE_BOOL,
        .size = sizeof(bool),
        .value.bool_val = value
    };
    
    return config_set(key, &config_val);
}

int config_get_string(config_key_t key, char *value, size_t max_len)
{
    config_value_t config_val;
    int ret = config_get(key, &config_val);
    
    if (ret == CONFIG_OK) {
        if (config_val.type == CONFIG_TYPE_STRING) {
            strncpy(value, config_val.value.string_val, max_len - 1);
            value[max_len - 1] = '\0';
        } else {
            ret = CONFIG_ERROR_INVALID;
        }
    }
    
    return ret;
}

int config_set_string(config_key_t key, const char *value)
{
    if (value == NULL) {
        return CONFIG_ERROR_INVALID;
    }
    
    config_value_t config_val = {
        .type = CONFIG_TYPE_STRING,
        .size = strlen(value) + 1
    };
    
    strncpy(config_val.value.string_val, value, sizeof(config_val.value.string_val) - 1);
    config_val.value.string_val[sizeof(config_val.value.string_val) - 1] = '\0';
    
    return config_set(key, &config_val);
}

int config_reset_to_defaults(void)
{
    if (!config_initialized) {
        return CONFIG_ERROR_INVALID;
    }

    k_mutex_lock(&config_mutex, K_FOREVER);
    
    for (int i = 0; i < CONFIG_KEY_MAX; i++) {
        config_values[i] = config_entries[i].default_value;
    }
    
    k_mutex_unlock(&config_mutex);
    return CONFIG_OK;
}

int config_reset_key(config_key_t key)
{
    if (!config_initialized || key >= CONFIG_KEY_MAX) {
        return CONFIG_ERROR_INVALID;
    }

    const config_entry_t *entry = &config_entries[key];
    
    if (entry->read_only) {
        return CONFIG_ERROR_READ_ONLY;
    }

    k_mutex_lock(&config_mutex, K_FOREVER);
    config_values[key] = entry->default_value;
    k_mutex_unlock(&config_mutex);

    return CONFIG_OK;
}

int config_validate_all(config_key_t *invalid_keys, size_t max_invalid, 
                       size_t *actual_invalid)
{
    if (!config_initialized) {
        return -1;
    }

    int invalid_count = 0;
    
    if (actual_invalid) {
        *actual_invalid = 0;
    }

    for (int i = 0; i < CONFIG_KEY_MAX; i++) {
        const config_entry_t *entry = &config_entries[i];
        
        if (entry->validator && !entry->validator(&config_values[i])) {
            invalid_count++;
            
            if (invalid_keys && *actual_invalid < max_invalid) {
                invalid_keys[(*actual_invalid)++] = i;
            }
        }
    }

    return invalid_count;
}

int config_get_entry_info(config_key_t key, config_entry_t *entry)
{
    if (!config_initialized || key >= CONFIG_KEY_MAX || entry == NULL) {
        return CONFIG_ERROR_INVALID;
    }

    *entry = config_entries[key];
    return CONFIG_OK;
}

const char *config_get_key_name(config_key_t key)
{
    if (key < CONFIG_KEY_MAX) {
        return key_names[key];
    }
    return "unknown";
}

/* Validation functions */
static bool validate_device_id(const config_value_t *value)
{
    return value->value.uint32_val != 0;
}

static bool validate_sampling_rate(const config_value_t *value)
{
    return value->value.uint32_val >= 1 && value->value.uint32_val <= 1000;
}

static bool validate_alert_thresholds(const config_value_t *value)
{
    /* Basic validation - check if values are reasonable */
    return value->size >= 16; /* At least 4 uint32 values */
}

static bool validate_communication_interval(const config_value_t *value)
{
    return value->value.uint32_val >= 1000 && value->value.uint32_val <= 60000;
}

static bool validate_power_management(const config_value_t *value)
{
    /* Boolean values are always valid */
    return true;
}

static bool validate_safety_limits(const config_value_t *value)
{
    return value->size >= 8; /* At least 2 uint32 values */
}

static bool validate_diagnostic_level(const config_value_t *value)
{
    return value->value.uint32_val <= 4; /* LOG_LEVEL_CRITICAL */
}