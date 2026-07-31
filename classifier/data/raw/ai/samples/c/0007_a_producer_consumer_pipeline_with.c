#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define BUFFER_SIZE 8
#define ITEM_COUNT 100

typedef struct {
    int data[BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    mtx_t mutex;
    cnd_t not_empty;
    cnd_t not_full;
} BoundedQueue;

static void queue_init(BoundedQueue *queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    mtx_init(&queue->mutex, mtx_plain);
    cnd_init(&queue->not_empty);
    cnd_init(&queue->not_full);
}

static void queue_destroy(BoundedQueue *queue) {
    cnd_destroy(&queue->not_empty);
    cnd_destroy(&queue->not_full);
    mtx_destroy(&queue->mutex);
}

static void queue_push(BoundedQueue *queue, int value) {
    mtx_lock(&queue->mutex);

    while (queue->count == BUFFER_SIZE)
        cnd_wait(&queue->not_full, &queue->mutex);

    queue->data[queue->tail] = value;
    queue->tail = (queue->tail + 1) % BUFFER_SIZE;
    queue->count++;

    cnd_signal(&queue->not_empty);
    mtx_unlock(&queue->mutex);
}

static int queue_pop(BoundedQueue *queue) {
    int value;

    mtx_lock(&queue->mutex);

    while (queue->count == 0)
        cnd_wait(&queue->not_empty, &queue->mutex);

    value = queue->data[queue->head];
    queue->head = (queue->head + 1) % BUFFER_SIZE;
    queue->count--;

    cnd_signal(&queue->not_full);
    mtx_unlock(&queue->mutex);

    return value;
}

static int producer(void *arg) {
    BoundedQueue *queue = arg;

    for (int i = 1; i <= ITEM_COUNT; ++i)
        queue_push(queue, i);

    queue_push(queue, -1);
    return 0;
}

static int consumer(void *arg) {
    BoundedQueue *queue = arg;

    for (;;) {
        int value = queue_pop(queue);

        if (value == -1)
            break;

        printf("consumed: %d\n", value);
    }

    return 0;
}

int main(void) {
    BoundedQueue queue;
    thrd_t producer_thread;
    thrd_t consumer_thread;
    int producer_result;
    int consumer_result;

    queue_init(&queue);

    if (thrd_create(&producer_thread, producer, &queue) != thrd_success ||
        thrd_create(&consumer_thread, consumer, &queue) != thrd_success) {
        queue_destroy(&queue);
        return EXIT_FAILURE;
    }

    thrd_join(producer_thread, &producer_result);
    thrd_join(consumer_thread, &consumer_result);

    queue_destroy(&queue);
    return producer_result || consumer_result ? EXIT_FAILURE : EXIT_SUCCESS;
}
