#include "safe_queue.h"
#include <string.h>

int safe_queue_init(safe_queue_t *queue, size_t max_size)
{
    if (queue == NULL || max_size == 0 || max_size > SAFE_QUEUE_MAX_SIZE) {
        return QUEUE_ERROR_INVALID;
    }

    memset(queue, 0, sizeof(safe_queue_t));
    
    queue->max_size = max_size;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->next_sequence_id = 1;
    queue->total_enqueued = 0;
    queue->total_dequeued = 0;
    queue->overrun_count = 0;

    k_mutex_init(&queue->mutex);
    k_condvar_init(&queue->not_empty);
    k_condvar_init(&queue->not_full);

    return QUEUE_OK;
}

int safe_queue_enqueue_nb(safe_queue_t *queue, const void *data, size_t size)
{
    if (queue == NULL || data == NULL || size == 0) {
        return QUEUE_ERROR_INVALID;
    }

    k_mutex_lock(&queue->mutex, K_FOREVER);

    if (queue->count >= queue->max_size) {
        queue->overrun_count++;
        k_mutex_unlock(&queue->mutex);
        return QUEUE_ERROR_FULL;
    }

    /* Add item to queue */
    queue_item_t *item = &queue->items[queue->tail];
    item->data = (void *)data;
    item->size = size;
    item->timestamp = k_uptime_get_32();
    item->sequence_id = queue->next_sequence_id++;

    queue->tail = (queue->tail + 1) % queue->max_size;
    queue->count++;
    queue->total_enqueued++;

    /* Signal waiting consumers */
    k_condvar_signal(&queue->not_empty);

    k_mutex_unlock(&queue->mutex);
    return QUEUE_OK;
}

int safe_queue_enqueue(safe_queue_t *queue, const void *data, size_t size, k_timeout_t timeout)
{
    if (queue == NULL || data == NULL || size == 0) {
        return QUEUE_ERROR_INVALID;
    }

    k_mutex_lock(&queue->mutex, K_FOREVER);

    /* Wait for space if queue is full */
    while (queue->count >= queue->max_size) {
        if (k_condvar_wait(&queue->not_full, &queue->mutex, timeout) != 0) {
            k_mutex_unlock(&queue->mutex);
            return QUEUE_ERROR_TIMEOUT;
        }
    }

    /* Add item to queue */
    queue_item_t *item = &queue->items[queue->tail];
    item->data = (void *)data;
    item->size = size;
    item->timestamp = k_uptime_get_32();
    item->sequence_id = queue->next_sequence_id++;

    queue->tail = (queue->tail + 1) % queue->max_size;
    queue->count++;
    queue->total_enqueued++;

    /* Signal waiting consumers */
    k_condvar_signal(&queue->not_empty);

    k_mutex_unlock(&queue->mutex);
    return QUEUE_OK;
}

int safe_queue_dequeue_nb(safe_queue_t *queue, queue_item_t *item)
{
    if (queue == NULL || item == NULL) {
        return QUEUE_ERROR_INVALID;
    }

    k_mutex_lock(&queue->mutex, K_FOREVER);

    if (queue->count == 0) {
        k_mutex_unlock(&queue->mutex);
        return QUEUE_ERROR_EMPTY;
    }

    /* Remove item from queue */
    *item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->max_size;
    queue->count--;
    queue->total_dequeued++;

    /* Signal waiting producers */
    k_condvar_signal(&queue->not_full);

    k_mutex_unlock(&queue->mutex);
    return QUEUE_OK;
}

int safe_queue_dequeue(safe_queue_t *queue, queue_item_t *item, k_timeout_t timeout)
{
    if (queue == NULL || item == NULL) {
        return QUEUE_ERROR_INVALID;
    }

    k_mutex_lock(&queue->mutex, K_FOREVER);

    /* Wait for item if queue is empty */
    while (queue->count == 0) {
        if (k_condvar_wait(&queue->not_empty, &queue->mutex, timeout) != 0) {
            k_mutex_unlock(&queue->mutex);
            return QUEUE_ERROR_TIMEOUT;
        }
    }

    /* Remove item from queue */
    *item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->max_size;
    queue->count--;
    queue->total_dequeued++;

    /* Signal waiting producers */
    k_condvar_signal(&queue->not_full);

    k_mutex_unlock(&queue->mutex);
    return QUEUE_OK;
}

size_t safe_queue_size(safe_queue_t *queue)
{
    if (queue == NULL) {
        return 0;
    }

    k_mutex_lock(&queue->mutex, K_FOREVER);
    size_t size = queue->count;
    k_mutex_unlock(&queue->mutex);

    return size;
}

bool safe_queue_is_empty(safe_queue_t *queue)
{
    return safe_queue_size(queue) == 0;
}

bool safe_queue_is_full(safe_queue_t *queue)
{
    if (queue == NULL) {
        return false;
    }

    k_mutex_lock(&queue->mutex, K_FOREVER);
    bool is_full = (queue->count >= queue->max_size);
    k_mutex_unlock(&queue->mutex);

    return is_full;
}

void safe_queue_clear(safe_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }

    k_mutex_lock(&queue->mutex, K_FOREVER);
    
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    
    /* Signal all waiting threads */
    k_condvar_broadcast(&queue->not_empty);
    k_condvar_broadcast(&queue->not_full);
    
    k_mutex_unlock(&queue->mutex);
}

void safe_queue_get_stats(safe_queue_t *queue, uint32_t *total_enqueued, 
                         uint32_t *total_dequeued, uint32_t *overrun_count)
{
    if (queue == NULL) {
        return;
    }

    k_mutex_lock(&queue->mutex, K_FOREVER);
    
    if (total_enqueued) {
        *total_enqueued = queue->total_enqueued;
    }
    if (total_dequeued) {
        *total_dequeued = queue->total_dequeued;
    }
    if (overrun_count) {
        *overrun_count = queue->overrun_count;
    }
    
    k_mutex_unlock(&queue->mutex);
}