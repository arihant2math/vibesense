#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *items;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    bool closed;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} bounded_handoff_t;

/**
 * Initializes a bounded handoff queue with the specified capacity.
 *
 * @param handoff Queue to initialize.
 * @param capacity Maximum number of items the queue can hold.
 * @return 0 on success, or an error number on failure.
 */
int bounded_handoff_init(bounded_handoff_t *handoff, size_t capacity)
{
    int result;

    if (handoff == NULL || capacity == 0) {
        return EINVAL;
    }

    handoff->items = malloc(capacity * sizeof(*handoff->items));
    if (handoff->items == NULL) {
        return ENOMEM;
    }

    handoff->capacity = capacity;
    handoff->head = 0;
    handoff->tail = 0;
    handoff->count = 0;
    handoff->closed = false;

    result = pthread_mutex_init(&handoff->mutex, NULL);
    if (result != 0) {
        free(handoff->items);
        return result;
    }

    result = pthread_cond_init(&handoff->not_empty, NULL);
    if (result != 0) {
        pthread_mutex_destroy(&handoff->mutex);
        free(handoff->items);
        return result;
    }

    result = pthread_cond_init(&handoff->not_full, NULL);
    if (result != 0) {
        pthread_cond_destroy(&handoff->not_empty);
        pthread_mutex_destroy(&handoff->mutex);
        free(handoff->items);
        return result;
    }

    return 0;
}

/**
 * Adds an item to the queue, waiting while the queue is full.
 *
 * @param handoff Initialized bounded handoff queue.
 * @param item Item to add.
 * @return 0 on success, ECANCELED if the queue is closed, or an error number.
 */
int bounded_handoff_push(bounded_handoff_t *handoff, int item)
{
    int result = pthread_mutex_lock(&handoff->mutex);
    if (result != 0) {
        return result;
    }

    while (handoff->count == handoff->capacity && !handoff->closed) {
        result = pthread_cond_wait(&handoff->not_full, &handoff->mutex);
        if (result != 0) {
            pthread_mutex_unlock(&handoff->mutex);
            return result;
        }
    }

    if (handoff->closed) {
        pthread_mutex_unlock(&handoff->mutex);
        return ECANCELED;
    }

    handoff->items[handoff->tail] = item;
    handoff->tail = (handoff->tail + 1) % handoff->capacity;
    handoff->count++;

    result = pthread_cond_signal(&handoff->not_empty);
    pthread_mutex_unlock(&handoff->mutex);
    return result;
}

/**
 * Removes an item from the queue, waiting while the queue is empty.
 *
 * @param handoff Initialized bounded handoff queue.
 * @param item Destination for the removed item.
 * @return 0 on success, ENODATA when closed and empty, or an error number.
 */
int bounded_handoff_pop(bounded_handoff_t *handoff, int *item)
{
    int result;

    if (item == NULL) {
        return EINVAL;
    }

    result = pthread_mutex_lock(&handoff->mutex);
    if (result != 0) {
        return result;
    }

    while (handoff->count == 0 && !handoff->closed) {
        result = pthread_cond_wait(&handoff->not_empty, &handoff->mutex);
        if (result != 0) {
            pthread_mutex_unlock(&handoff->mutex);
            return result;
        }
    }

    if (handoff->count == 0 && handoff->closed) {
        pthread_mutex_unlock(&handoff->mutex);
        return ENODATA;
    }

    *item = handoff->items[handoff->head];
    handoff->head = (handoff->head + 1) % handoff->capacity;
    handoff->count--;

    result = pthread_cond_signal(&handoff->not_full);
    pthread_mutex_unlock(&handoff->mutex);
    return result;
}

/**
 * Closes the queue and wakes all waiting producers and consumers.
 *
 * Items already in the queue remain available to consumers.
 *
 * @param handoff Initialized bounded handoff queue.
 */
void bounded_handoff_close(bounded_handoff_t *handoff)
{
    pthread_mutex_lock(&handoff->mutex);
    handoff->closed = true;
    pthread_cond_broadcast(&handoff->not_empty);
    pthread_cond_broadcast(&handoff->not_full);
    pthread_mutex_unlock(&handoff->mutex);
}

/**
 * Releases all resources associated with a bounded handoff queue.
 *
 * The queue must be closed and have no active users before destruction.
 *
 * @param handoff Queue to destroy.
 */
void bounded_handoff_destroy(bounded_handoff_t *handoff)
{
    pthread_cond_destroy(&handoff->not_empty);
    pthread_cond_destroy(&handoff->not_full);
    pthread_mutex_destroy(&handoff->mutex);
    free(handoff->items);
    handoff->items = NULL;
}

/**
 * Produces a sequence of integers and places them in the bounded queue.
 *
 * @param argument Pointer to the bounded handoff queue.
 * @return NULL.
 */
void *producer_thread(void *argument)
{
    bounded_handoff_t *handoff = argument;

    for (int value = 1; value <= 100; value++) {
        if (bounded_handoff_push(handoff, value) != 0) {
            break;
        }
    }

    bounded_handoff_close(handoff);
    return NULL;
}

/**
 * Consumes and processes integers until the queue is closed and empty.
 *
 * @param argument Pointer to the bounded handoff queue.
 * @return NULL.
 */
void *consumer_thread(void *argument)
{
    bounded_handoff_t *handoff = argument;
    int value;

    while (bounded_handoff_pop(handoff, &value) == 0) {
        printf("consumed: %d\n", value);
    }

    return NULL;
}

/**
 * Runs a producer-consumer bounded handoff demonstration.
 *
 * @return EXIT_SUCCESS on success, or EXIT_FAILURE on error.
 */
int main(void)
{
    bounded_handoff_t handoff;
    pthread_t producer;
    pthread_t consumer;
    int result;

    result = bounded_handoff_init(&handoff, 8);
    if (result != 0) {
        fprintf(stderr, "queue initialization failed: %d\n", result);
        return EXIT_FAILURE;
    }

    result = pthread_create(&producer, NULL, producer_thread, &handoff);
    if (result != 0) {
        bounded_handoff_destroy(&handoff);
        return EXIT_FAILURE;
    }

    result = pthread_create(&consumer, NULL, consumer_thread, &handoff);
    if (result != 0) {
        bounded_handoff_close(&handoff);
        pthread_join(producer, NULL);
        bounded_handoff_destroy(&handoff);
        return EXIT_FAILURE;
    }

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    bounded_handoff_destroy(&handoff);

    return EXIT_SUCCESS;
}
