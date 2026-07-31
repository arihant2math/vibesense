#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    void **items;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;
    bool closed;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} BlockingQueue;

int blocking_queue_init(BlockingQueue *queue, size_t capacity)
{
    if (queue == NULL || capacity == 0) {
        return -1;
    }

    queue->items = malloc(capacity * sizeof(*queue->items));
    if (queue->items == NULL) {
        return -1;
    }

    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->closed = false;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->items);
        return -1;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->items);
        return -1;
    }

    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->items);
        return -1;
    }

    return 0;
}

void blocking_queue_close(BlockingQueue *queue)
{
    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);
    queue->closed = true;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
}

void blocking_queue_destroy(BlockingQueue *queue)
{
    if (queue == NULL) {
        return;
    }

    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    pthread_mutex_destroy(&queue->mutex);
    free(queue->items);

    queue->items = NULL;
    queue->capacity = 0;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->closed = true;
}

int blocking_queue_push(BlockingQueue *queue, void *item)
{
    if (queue == NULL) {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count == queue->capacity && !queue->closed) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (queue->closed) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    queue->items[queue->tail] = item;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

int blocking_queue_pop(BlockingQueue *queue, void **item)
{
    if (queue == NULL || item == NULL) {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !queue->closed) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    if (queue->count == 0 && queue->closed) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    *item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}
