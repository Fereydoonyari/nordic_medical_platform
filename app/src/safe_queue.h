#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/**
 * @file safe_queue.h
 * @brief Thread-safe queue implementation for inter-thread communication
 * @details Provides a thread-safe FIFO queue with configurable size and
 * timeout support, suitable for medical device data flow.
 */

/* Maximum queue size */
#define SAFE_QUEUE_MAX_SIZE    32

/* Queue error codes */
#define QUEUE_OK               0
#define QUEUE_ERROR_FULL      -1
#define QUEUE_ERROR_EMPTY     -2
#define QUEUE_ERROR_TIMEOUT   -3
#define QUEUE_ERROR_INVALID   -4

/* Queue item structure */
typedef struct {
    void *data;
    size_t size;
    uint32_t timestamp;
    uint32_t sequence_id;
} queue_item_t;

/* Thread-safe queue structure */
typedef struct {
    queue_item_t items[SAFE_QUEUE_MAX_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    size_t max_size;
    struct k_mutex mutex;
    struct k_condvar not_empty;
    struct k_condvar not_full;
    uint32_t next_sequence_id;
    uint32_t total_enqueued;
    uint32_t total_dequeued;
    uint32_t overrun_count;
} safe_queue_t;

/**
 * @brief Initialize a thread-safe queue
 * @param queue Pointer to queue structure
 * @param max_size Maximum number of items (must be <= SAFE_QUEUE_MAX_SIZE)
 * @return QUEUE_OK on success, error code otherwise
 */
int safe_queue_init(safe_queue_t *queue, size_t max_size);

/**
 * @brief Add item to queue (non-blocking)
 * @param queue Pointer to queue structure
 * @param data Pointer to data to enqueue
 * @param size Size of data
 * @return QUEUE_OK on success, QUEUE_ERROR_FULL if queue is full
 */
int safe_queue_enqueue_nb(safe_queue_t *queue, const void *data, size_t size);

/**
 * @brief Add item to queue (blocking with timeout)
 * @param queue Pointer to queue structure
 * @param data Pointer to data to enqueue
 * @param size Size of data
 * @param timeout Timeout for operation
 * @return QUEUE_OK on success, error code otherwise
 */
int safe_queue_enqueue(safe_queue_t *queue, const void *data, size_t size, k_timeout_t timeout);

/**
 * @brief Remove item from queue (non-blocking)
 * @param queue Pointer to queue structure
 * @param item Pointer to queue_item_t structure to fill
 * @return QUEUE_OK on success, QUEUE_ERROR_EMPTY if queue is empty
 */
int safe_queue_dequeue_nb(safe_queue_t *queue, queue_item_t *item);

/**
 * @brief Remove item from queue (blocking with timeout)
 * @param queue Pointer to queue structure
 * @param item Pointer to queue_item_t structure to fill
 * @param timeout Timeout for operation
 * @return QUEUE_OK on success, error code otherwise
 */
int safe_queue_dequeue(safe_queue_t *queue, queue_item_t *item, k_timeout_t timeout);

/**
 * @brief Get current queue size
 * @param queue Pointer to queue structure
 * @return Current number of items in queue
 */
size_t safe_queue_size(safe_queue_t *queue);

/**
 * @brief Check if queue is empty
 * @param queue Pointer to queue structure
 * @return true if empty, false otherwise
 */
bool safe_queue_is_empty(safe_queue_t *queue);

/**
 * @brief Check if queue is full
 * @param queue Pointer to queue structure
 * @return true if full, false otherwise
 */
bool safe_queue_is_full(safe_queue_t *queue);

/**
 * @brief Clear all items from queue
 * @param queue Pointer to queue structure
 */
void safe_queue_clear(safe_queue_t *queue);

/**
 * @brief Get queue statistics
 * @param queue Pointer to queue structure
 * @param total_enqueued Pointer to store total enqueued count
 * @param total_dequeued Pointer to store total dequeued count
 * @param overrun_count Pointer to store overrun count
 */
void safe_queue_get_stats(safe_queue_t *queue, uint32_t *total_enqueued, 
                         uint32_t *total_dequeued, uint32_t *overrun_count);

#endif /* SAFE_QUEUE_H */