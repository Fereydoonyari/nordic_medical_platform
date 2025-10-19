#ifndef SAFE_BUFFER_H
#define SAFE_BUFFER_H

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/**
 * @file safe_buffer.h
 * @brief Thread-safe circular buffer for continuous data streams
 * @details Provides a thread-safe circular buffer implementation optimized
 * for continuous data acquisition in medical devices.
 */

/* Buffer error codes */
#define BUFFER_OK              0
#define BUFFER_ERROR_FULL     -1
#define BUFFER_ERROR_EMPTY    -2
#define BUFFER_ERROR_TIMEOUT  -3
#define BUFFER_ERROR_INVALID  -4

/* Default buffer sizes */
#define BUFFER_SIZE_SMALL      256
#define BUFFER_SIZE_MEDIUM     1024
#define BUFFER_SIZE_LARGE      4096

/* Thread-safe circular buffer structure */
typedef struct {
    uint8_t *data;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    struct k_mutex mutex;
    struct k_condvar not_empty;
    struct k_condvar not_full;
    uint32_t write_count;
    uint32_t read_count;
    uint32_t overflow_count;
    bool overwrite_on_full;
} safe_buffer_t;

/**
 * @brief Initialize a thread-safe circular buffer
 * @param buffer Pointer to buffer structure
 * @param data Pointer to data storage area
 * @param size Size of data storage area
 * @param overwrite_on_full Whether to overwrite old data when buffer is full
 * @return BUFFER_OK on success, error code otherwise
 */
int safe_buffer_init(safe_buffer_t *buffer, uint8_t *data, size_t size, bool overwrite_on_full);

/**
 * @brief Write data to buffer (non-blocking)
 * @param buffer Pointer to buffer structure
 * @param data Pointer to data to write
 * @param size Number of bytes to write
 * @param written Pointer to store actual bytes written (optional)
 * @return BUFFER_OK on success, error code otherwise
 */
int safe_buffer_write_nb(safe_buffer_t *buffer, const void *data, size_t size, size_t *written);

/**
 * @brief Write data to buffer (blocking with timeout)
 * @param buffer Pointer to buffer structure
 * @param data Pointer to data to write
 * @param size Number of bytes to write
 * @param timeout Timeout for operation
 * @param written Pointer to store actual bytes written (optional)
 * @return BUFFER_OK on success, error code otherwise
 */
int safe_buffer_write(safe_buffer_t *buffer, const void *data, size_t size, 
                     k_timeout_t timeout, size_t *written);

/**
 * @brief Read data from buffer (non-blocking)
 * @param buffer Pointer to buffer structure
 * @param data Pointer to buffer to store read data
 * @param size Number of bytes to read
 * @param read Pointer to store actual bytes read (optional)
 * @return BUFFER_OK on success, error code otherwise
 */
int safe_buffer_read_nb(safe_buffer_t *buffer, void *data, size_t size, size_t *read);

/**
 * @brief Read data from buffer (blocking with timeout)
 * @param buffer Pointer to buffer structure
 * @param data Pointer to buffer to store read data
 * @param size Number of bytes to read
 * @param timeout Timeout for operation
 * @param read Pointer to store actual bytes read (optional)
 * @return BUFFER_OK on success, error code otherwise
 */
int safe_buffer_read(safe_buffer_t *buffer, void *data, size_t size, 
                    k_timeout_t timeout, size_t *read);

/**
 * @brief Get available data size in buffer
 * @param buffer Pointer to buffer structure
 * @return Number of bytes available to read
 */
size_t safe_buffer_available(safe_buffer_t *buffer);

/**
 * @brief Get free space in buffer
 * @param buffer Pointer to buffer structure
 * @return Number of bytes available to write
 */
size_t safe_buffer_free_space(safe_buffer_t *buffer);

/**
 * @brief Check if buffer is empty
 * @param buffer Pointer to buffer structure
 * @return true if empty, false otherwise
 */
bool safe_buffer_is_empty(safe_buffer_t *buffer);

/**
 * @brief Check if buffer is full
 * @param buffer Pointer to buffer structure
 * @return true if full, false otherwise
 */
bool safe_buffer_is_full(safe_buffer_t *buffer);

/**
 * @brief Clear buffer contents
 * @param buffer Pointer to buffer structure
 */
void safe_buffer_clear(safe_buffer_t *buffer);

/**
 * @brief Get buffer statistics
 * @param buffer Pointer to buffer structure
 * @param write_count Pointer to store total write operations
 * @param read_count Pointer to store total read operations
 * @param overflow_count Pointer to store overflow count
 */
void safe_buffer_get_stats(safe_buffer_t *buffer, uint32_t *write_count, 
                          uint32_t *read_count, uint32_t *overflow_count);

#endif /* SAFE_BUFFER_H */