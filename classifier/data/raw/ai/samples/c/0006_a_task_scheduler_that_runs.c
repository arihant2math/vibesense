#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

typedef void (*scheduler_callback)(void *argument);

typedef struct {
    scheduler_callback callback;
    void *argument;
    int64_t interval_nanoseconds;
    struct timespec next_run;
} scheduler_task;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    pthread_t thread;
    scheduler_task *tasks;
    size_t task_count;
    size_t task_capacity;
    int stopping;
} scheduler;

static int timespec_compare(struct timespec a, struct timespec b)
{
    if (a.tv_sec < b.tv_sec)
        return -1;
    if (a.tv_sec > b.tv_sec)
        return 1;
    if (a.tv_nsec < b.tv_nsec)
        return -1;
    if (a.tv_nsec > b.tv_nsec)
        return 1;
    return 0;
}

static struct timespec timespec_add_nanoseconds(
    struct timespec time,
    int64_t nanoseconds)
{
    time.tv_sec += nanoseconds / 1000000000;
    time.tv_nsec += nanoseconds % 1000000000;

    if (time.tv_nsec >= 1000000000) {
        time.tv_sec++;
        time.tv_nsec -= 1000000000;
    } else if (time.tv_nsec < 0) {
        time.tv_sec--;
        time.tv_nsec += 1000000000;
    }

    return time;
}

static void *scheduler_thread(void *argument)
{
    scheduler *scheduler_instance = argument;

    pthread_mutex_lock(&scheduler_instance->mutex);

    while (!scheduler_instance->stopping) {
        struct timespec now;
        size_t due_task = SIZE_MAX;
        struct timespec earliest;
        int have_earliest = 0;

        clock_gettime(CLOCK_MONOTONIC, &now);

        for (size_t i = 0; i < scheduler_instance->task_count; i++) {
            scheduler_task *task = &scheduler_instance->tasks[i];

            if (timespec_compare(task->next_run, now) <= 0) {
                due_task = i;
                break;
            }

            if (!have_earliest ||
                timespec_compare(task->next_run, earliest) < 0) {
                earliest = task->next_run;
                have_earliest = 1;
            }
        }

        if (due_task == SIZE_MAX) {
            if (have_earliest) {
                pthread_cond_timedwait(
                    &scheduler_instance->condition,
                    &scheduler_instance->mutex,
                    &earliest);
            } else {
                pthread_cond_wait(
                    &scheduler_instance->condition,
                    &scheduler_instance->mutex);
            }
            continue;
        }

        scheduler_callback callback =
            scheduler_instance->tasks[due_task].callback;
        void *callback_argument =
            scheduler_instance->tasks[due_task].argument;
        int64_t interval =
            scheduler_instance->tasks[due_task].interval_nanoseconds;

        clock_gettime(CLOCK_MONOTONIC, &now);

        do {
            scheduler_instance->tasks[due_task].next_run =
                timespec_add_nanoseconds(
                    scheduler_instance->tasks[due_task].next_run,
                    interval);
        } while (timespec_compare(
                     scheduler_instance->tasks[due_task].next_run,
                     now) <= 0);

        pthread_mutex_unlock(&scheduler_instance->mutex);
        callback(callback_argument);
        pthread_mutex_lock(&scheduler_instance->mutex);
    }

    pthread_mutex_unlock(&scheduler_instance->mutex);
    return NULL;
}

int scheduler_init(scheduler *scheduler_instance)
{
    pthread_condattr_t condition_attributes;
    int result;

    if (scheduler_instance == NULL)
        return EINVAL;

    scheduler_instance->tasks = NULL;
    scheduler_instance->task_count = 0;
    scheduler_instance->task_capacity = 0;
    scheduler_instance->stopping = 0;

    result = pthread_mutex_init(&scheduler_instance->mutex, NULL);
    if (result != 0)
        return result;

    result = pthread_condattr_init(&condition_attributes);
    if (result != 0) {
        pthread_mutex_destroy(&scheduler_instance->mutex);
        return result;
    }

    result = pthread_condattr_setclock(
        &condition_attributes,
        CLOCK_MONOTONIC);
    if (result == 0) {
        result = pthread_cond_init(
            &scheduler_instance->condition,
            &condition_attributes);
    }

    pthread_condattr_destroy(&condition_attributes);

    if (result != 0) {
        pthread_mutex_destroy(&scheduler_instance->mutex);
        return result;
    }

    result = pthread_create(
        &scheduler_instance->thread,
        NULL,
        scheduler_thread,
        scheduler_instance);

    if (result != 0) {
        pthread_cond_destroy(&scheduler_instance->condition);
        pthread_mutex_destroy(&scheduler_instance->mutex);
    }

    return result;
}

int scheduler_add(
    scheduler *scheduler_instance,
    scheduler_callback callback,
    void *argument,
    uint64_t interval_milliseconds)
{
    scheduler_task *new_tasks;
    struct timespec now;
    int result = 0;

    if (scheduler_instance == NULL ||
        callback == NULL ||
        interval_milliseconds == 0) {
        return EINVAL;
    }

    pthread_mutex_lock(&scheduler_instance->mutex);

    if (scheduler_instance->stopping) {
        result = ECANCELED;
        goto unlock;
    }

    if (scheduler_instance->task_count ==
        scheduler_instance->task_capacity) {
        size_t new_capacity =
            scheduler_instance->task_capacity == 0
                ? 8
                : scheduler_instance->task_capacity * 2;

        new_tasks = realloc(
            scheduler_instance->tasks,
            new_capacity * sizeof(*new_tasks));

        if (new_tasks == NULL) {
            result = ENOMEM;
            goto unlock;
        }

        scheduler_instance->tasks = new_tasks;
        scheduler_instance->task_capacity = new_capacity;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);

    scheduler_instance->tasks[scheduler_instance->task_count] =
        (scheduler_task) {
            .callback = callback,
            .argument = argument,
            .interval_nanoseconds =
                (int64_t) interval_milliseconds * 1000000,
            .next_run = timespec_add_nanoseconds(
                now,
                (int64_t) interval_milliseconds * 1000000)
        };

    scheduler_instance->task_count++;
    pthread_cond_signal(&scheduler_instance->condition);

unlock:
    pthread_mutex_unlock(&scheduler_instance->mutex);
    return result;
}

void scheduler_destroy(scheduler *scheduler_instance)
{
    if (scheduler_instance == NULL)
        return;

    pthread_mutex_lock(&scheduler_instance->mutex);
    scheduler_instance->stopping = 1;
    pthread_cond_broadcast(&scheduler_instance->condition);
    pthread_mutex_unlock(&scheduler_instance->mutex);

    pthread_join(scheduler_instance->thread, NULL);

    free(scheduler_instance->tasks);
    pthread_cond_destroy(&scheduler_instance->condition);
    pthread_mutex_destroy(&scheduler_instance->mutex);
}
