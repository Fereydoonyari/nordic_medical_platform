#include "safe_buffer.h"
#include <string.h>

int safe_buffer_init(safe_buffer_t *buffer, uint8_t *data, size_t size, bool overwrite_on_full)
{
    if (buffer == NULL || data == NULL || size == 0) {
        return BUFFER_ERROR_INVALID;
    }

    memset(buffer, 0, sizeof(safe_buffer_t));
    
    buffer->data = data;
    buffer->size = size;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->overwrite_on_full = overwrite_on_full;
    buffer->write_count = 0;
    buffer->read_count = 0;
    buffer->overflow_count = 0;

    k_mutex_init(&buffer->mutex);
    k_condvar_init(&buffer->not_empty);
    k_condvar_init(&buffer->not_full);

    return BUFFER_OK;
}

int safe_buffer_write_nb(safe_buffer_t *buffer, const void *data, size_t size, size_t *written)
{
    if (buffer == NULL || data == NULL || size == 0) {
        return BUFFER_ERROR_INVALID;
    }

    k_mutex_lock(&buffer->mutex, K_FOREVER);

    size_t available_space = buffer->size - buffer->count;
    size_t bytes_to_write = size;

    if (written) {
        *written = 0;
    }

    if (bytes_to_write > available_space) {
        if (buffer->overwrite_on_full) {
            /* Overwrite old data */
            buffer->overflow_count++;
            bytes_to_write = size; /* Write all data, overwriting as needed */
        } else {
            /* Only write what fits */
            bytes_to_write = available_space;
            if (bytes_to_write == 0) {
                k_mutex_unlock(&buffer->mutex);
                return BUFFER_ERROR_FULL;
            }
        }
    }

    const uint8_t *src = (const uint8_t *)data;
    size_t bytes_written = 0;

    while (bytes_written < bytes_to_write) {
        size_t chunk_size = bytes_to_write - bytes_written;
        size_t tail_to_end = buffer->size - buffer->tail;
        
        if (chunk_size > tail_to_end) {
            chunk_size = tail_to_end;
        }

        /* Handle overwrite mode */
        if (buffer->overwrite_on_full && buffer->count + chunk_size > buffer->size) {
            size_t overwrite_bytes = (buffer->count + chunk_size) - buffer->size;
            buffer->head = (buffer->head + overwrite_bytes) % buffer->size;
            buffer->count -= overwrite_bytes;
        }

        memcpy(&buffer->data[buffer->tail], &src[bytes_written], chunk_size);
        buffer->tail = (buffer->tail + chunk_size) % buffer->size;
        buffer->count += chunk_size;
        bytes_written += chunk_size;

        /* Ensure count doesn't exceed buffer size */
        if (buffer->count > buffer->size) {
            buffer->count = buffer->size;
        }
    }

    buffer->write_count++;
    if (written) {
        *written = bytes_written;
    }

    /* Signal waiting readers */
    k_condvar_signal(&buffer->not_empty);

    k_mutex_unlock(&buffer->mutex);
    return BUFFER_OK;
}

int safe_buffer_write(safe_buffer_t *buffer, const void *data, size_t size, 
                     k_timeout_t timeout, size_t *written)
{
    if (buffer == NULL || data == NULL || size == 0) {
        return BUFFER_ERROR_INVALID;
    }

    k_mutex_lock(&buffer->mutex, K_FOREVER);

    if (written) {
        *written = 0;
    }

    /* Wait for space if buffer is full and not in overwrite mode */
    while (!buffer->overwrite_on_full && (buffer->size - buffer->count) < size) {
        if (k_condvar_wait(&buffer->not_full, &buffer->mutex, timeout) != 0) {
            k_mutex_unlock(&buffer->mutex);
            return BUFFER_ERROR_TIMEOUT;
        }
    }

    k_mutex_unlock(&buffer->mutex);

    /* Use non-blocking write now that we have space or are in overwrite mode */
    return safe_buffer_write_nb(buffer, data, size, written);
}

int safe_buffer_read_nb(safe_buffer_t *buffer, void *data, size_t size, size_t *read_bytes)
{
    if (buffer == NULL || data == NULL || size == 0) {
        return BUFFER_ERROR_INVALID;
    }

    k_mutex_lock(&buffer->mutex, K_FOREVER);

    if (read_bytes) {
        *read_bytes = 0;
    }

    if (buffer->count == 0) {
        k_mutex_unlock(&buffer->mutex);
        return BUFFER_ERROR_EMPTY;
    }

    size_t bytes_to_read = (size < buffer->count) ? size : buffer->count;
    uint8_t *dst = (uint8_t *)data;
    size_t bytes_read = 0;

    while (bytes_read < bytes_to_read) {
        size_t chunk_size = bytes_to_read - bytes_read;
        size_t head_to_end = buffer->size - buffer->head;
        
        if (chunk_size > head_to_end) {
            chunk_size = head_to_end;
        }

        memcpy(&dst[bytes_read], &buffer->data[buffer->head], chunk_size);
        buffer->head = (buffer->head + chunk_size) % buffer->size;
        buffer->count -= chunk_size;
        bytes_read += chunk_size;
    }

    buffer->read_count++;
    if (read_bytes) {
        *read_bytes = bytes_read;
    }

    /* Signal waiting writers */
    k_condvar_signal(&buffer->not_full);

    k_mutex_unlock(&buffer->mutex);
    return BUFFER_OK;
}

int safe_buffer_read(safe_buffer_t *buffer, void *data, size_t size, 
                    k_timeout_t timeout, size_t *read_bytes)
{
    if (buffer == NULL || data == NULL || size == 0) {
        return BUFFER_ERROR_INVALID;
    }

    k_mutex_lock(&buffer->mutex, K_FOREVER);

    if (read_bytes) {
        *read_bytes = 0;
    }

    /* Wait for data if buffer is empty */
    while (buffer->count == 0) {
        if (k_condvar_wait(&buffer->not_empty, &buffer->mutex, timeout) != 0) {
            k_mutex_unlock(&buffer->mutex);
            return BUFFER_ERROR_TIMEOUT;
        }
    }

    k_mutex_unlock(&buffer->mutex);

    /* Use non-blocking read now that we have data */
    return safe_buffer_read_nb(buffer, data, size, read_bytes);
}

size_t safe_buffer_available(safe_buffer_t *buffer)
{
    if (buffer == NULL) {
        return 0;
    }

    k_mutex_lock(&buffer->mutex, K_FOREVER);
    size_t available = buffer->count;
    k_mutex_unlock(&buffer->mutex);

    return available;
}

size_t safe_buffer_free_space(safe_buffer_t *buffer)
{
    if (buffer == NULL) {
        return 0;
    }

    k_mutex_lock(&buffer->mutex, K_FOREVER);
    size_t free_space = buffer->size - buffer->count;
    k_mutex_unlock(&buffer->mutex);

    return free_space;
}

bool safe_buffer_is_empty(safe_buffer_t *buffer)
{
    return safe_buffer_available(buffer) == 0;
}

bool safe_buffer_is_full(safe_buffer_t *buffer)
{
    return safe_buffer_free_space(buffer) == 0;
}

void safe_buffer_clear(safe_buffer_t *buffer)
{
    if (buffer == NULL) {
        return;
    }

    k_mutex_lock(&buffer->mutex, K_FOREVER);
    
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    
    /* Signal all waiting threads */
    k_condvar_broadcast(&buffer->not_empty);
    k_condvar_broadcast(&buffer->not_full);
    
    k_mutex_unlock(&buffer->mutex);
}

void safe_buffer_get_stats(safe_buffer_t *buffer, uint32_t *write_count, 
                          uint32_t *read_count, uint32_t *overflow_count)
{
    if (buffer == NULL) {
        return;
    }

    k_mutex_lock(&buffer->mutex, K_FOREVER);
    
    if (write_count) {
        *write_count = buffer->write_count;
    }
    if (read_count) {
        *read_count = buffer->read_count;
    }
    if (overflow_count) {
        *overflow_count = buffer->overflow_count;
    }
    
    k_mutex_unlock(&buffer->mutex);
}