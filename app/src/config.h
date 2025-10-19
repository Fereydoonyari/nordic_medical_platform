#ifndef CONFIG_H
#define CONFIG_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @file config.h
 * @brief Configuration management for medical wearable device
 * @details Provides centralized configuration management with validation,
 * persistence, and runtime updates for medical device parameters.
 */

/* Configuration keys */
typedef enum {
    CONFIG_KEY_DEVICE_ID = 0,
    CONFIG_KEY_SAMPLING_RATE,
    CONFIG_KEY_ALERT_THRESHOLDS,
    CONFIG_KEY_COMMUNICATION_INTERVAL,
    CONFIG_KEY_POWER_MANAGEMENT,
    CONFIG_KEY_SAFETY_LIMITS,
    CONFIG_KEY_CALIBRATION_DATA,
    CONFIG_KEY_DIAGNOSTIC_LEVEL,
    CONFIG_KEY_MAX
} config_key_t;

/* Configuration value types */
typedef enum {
    CONFIG_TYPE_UINT32 = 0,
    CONFIG_TYPE_FLOAT,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_BLOB
} config_type_t;

/* Configuration value structure */
typedef struct {
    config_type_t type;
    size_t size;
    union {
        uint32_t uint32_val;
        float float_val;
        bool bool_val;
        char string_val[64];
        uint8_t blob_val[256];
    } value;
} config_value_t;

/* Configuration validation function type */
typedef bool (*config_validator_t)(const config_value_t *value);

/* Configuration entry structure */
typedef struct {
    config_key_t key;
    const char *name;
    config_type_t type;
    config_value_t default_value;
    config_validator_t validator;
    bool read_only;
    bool requires_restart;
} config_entry_t;

/* Configuration error codes */
#define CONFIG_OK                0
#define CONFIG_ERROR_INVALID    -1
#define CONFIG_ERROR_NOT_FOUND  -2
#define CONFIG_ERROR_READ_ONLY  -3
#define CONFIG_ERROR_VALIDATION -4
#define CONFIG_ERROR_STORAGE    -5

/**
 * @brief Initialize configuration system
 * @return CONFIG_OK on success, error code otherwise
 */
int config_init(void);

/**
 * @brief Load configuration from persistent storage
 * @return CONFIG_OK on success, error code otherwise
 */
int config_load(void);

/**
 * @brief Save configuration to persistent storage
 * @return CONFIG_OK on success, error code otherwise
 */
int config_save(void);

/**
 * @brief Get configuration value
 * @param key Configuration key
 * @param value Pointer to value structure to fill
 * @return CONFIG_OK on success, error code otherwise
 */
int config_get(config_key_t key, config_value_t *value);

/**
 * @brief Set configuration value
 * @param key Configuration key
 * @param value Pointer to value to set
 * @return CONFIG_OK on success, error code otherwise
 */
int config_set(config_key_t key, const config_value_t *value);

/**
 * @brief Get uint32 configuration value
 * @param key Configuration key
 * @param value Pointer to store value
 * @return CONFIG_OK on success, error code otherwise
 */
int config_get_uint32(config_key_t key, uint32_t *value);

/**
 * @brief Set uint32 configuration value
 * @param key Configuration key
 * @param value Value to set
 * @return CONFIG_OK on success, error code otherwise
 */
int config_set_uint32(config_key_t key, uint32_t value);

/**
 * @brief Get float configuration value
 * @param key Configuration key
 * @param value Pointer to store value
 * @return CONFIG_OK on success, error code otherwise
 */
int config_get_float(config_key_t key, float *value);

/**
 * @brief Set float configuration value
 * @param key Configuration key
 * @param value Value to set
 * @return CONFIG_OK on success, error code otherwise
 */
int config_set_float(config_key_t key, float value);

/**
 * @brief Get boolean configuration value
 * @param key Configuration key
 * @param value Pointer to store value
 * @return CONFIG_OK on success, error code otherwise
 */
int config_get_bool(config_key_t key, bool *value);

/**
 * @brief Set boolean configuration value
 * @param key Configuration key
 * @param value Value to set
 * @return CONFIG_OK on success, error code otherwise
 */
int config_set_bool(config_key_t key, bool value);

/**
 * @brief Get string configuration value
 * @param key Configuration key
 * @param value Buffer to store string
 * @param max_len Maximum length of buffer
 * @return CONFIG_OK on success, error code otherwise
 */
int config_get_string(config_key_t key, char *value, size_t max_len);

/**
 * @brief Set string configuration value
 * @param key Configuration key
 * @param value String value to set
 * @return CONFIG_OK on success, error code otherwise
 */
int config_set_string(config_key_t key, const char *value);

/**
 * @brief Reset configuration to defaults
 * @return CONFIG_OK on success, error code otherwise
 */
int config_reset_to_defaults(void);

/**
 * @brief Reset single configuration key to default
 * @param key Configuration key
 * @return CONFIG_OK on success, error code otherwise
 */
int config_reset_key(config_key_t key);

/**
 * @brief Validate all configuration values
 * @param invalid_keys Array to store invalid keys (optional)
 * @param max_invalid Maximum number of invalid keys to store
 * @param actual_invalid Pointer to store actual number of invalid keys
 * @return Number of invalid configuration entries
 */
int config_validate_all(config_key_t *invalid_keys, size_t max_invalid, 
                       size_t *actual_invalid);

/**
 * @brief Get configuration entry information
 * @param key Configuration key
 * @param entry Pointer to entry structure to fill
 * @return CONFIG_OK on success, error code otherwise
 */
int config_get_entry_info(config_key_t key, config_entry_t *entry);

/**
 * @brief Get configuration key name
 * @param key Configuration key
 * @return Key name string
 */
const char *config_get_key_name(config_key_t key);

#endif /* CONFIG_H */