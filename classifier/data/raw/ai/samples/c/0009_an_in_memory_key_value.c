#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KV_INITIAL_CAPACITY 16U
#define KV_MAX_KEY_LENGTH (1024U * 1024U)

typedef enum {
    KV_OK = 0,
    KV_NOT_FOUND,
    KV_INVALID_ARGUMENT,
    KV_OUT_OF_MEMORY,
    KV_ALREADY_EXISTS,
    KV_OVERFLOW,
    KV_INTERNAL_ERROR
} kv_status_t;

typedef struct kv_entry {
    char *key;
    void *value;
    size_t value_size;
    uint64_t expires_at_ms;
    struct kv_entry *next;
} kv_entry_t;

typedef struct {
    kv_entry_t **buckets;
    size_t capacity;
    size_t size;
    pthread_mutex_t mutex;
} kv_store_t;

static uint64_t now_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;

    return ((uint64_t)ts.tv_sec * 1000ULL) +
           ((uint64_t)ts.tv_nsec / 1000000ULL);
}

static uint64_t hash_key(const char *key)
{
    uint64_t hash = 1469598103934665603ULL;

    while (*key != '\0') {
        hash ^= (unsigned char)*key++;
        hash *= 1099511628211ULL;
    }

    return hash;
}

static int valid_key(const char *key)
{
    size_t length;

    if (key == NULL || *key == '\0')
        return 0;

    length = strnlen(key, KV_MAX_KEY_LENGTH + 1U);
    return length > 0U && length <= KV_MAX_KEY_LENGTH;
}

static kv_status_t validate_store(kv_store_t *store)
{
    if (store == NULL || store->buckets == NULL || store->capacity == 0U)
        return KV_INVALID_ARGUMENT;

    return KV_OK;
}

static void free_entry(kv_entry_t *entry)
{
    if (entry == NULL)
        return;

    free(entry->key);
    free(entry->value);
    free(entry);
}

static void remove_expired_locked(kv_store_t *store, uint64_t now)
{
    size_t i;

    for (i = 0; i < store->capacity; ++i) {
        kv_entry_t **current = &store->buckets[i];

        while (*current != NULL) {
            kv_entry_t *entry = *current;

            if (entry->expires_at_ms != 0U &&
                entry->expires_at_ms <= now) {
                *current = entry->next;
                free_entry(entry);
                store->size--;
            } else {
                current = &entry->next;
            }
        }
    }
}

static kv_status_t resize_store_locked(kv_store_t *store)
{
    size_t new_capacity;
    kv_entry_t **new_buckets;
    size_t i;

    if (store->capacity > SIZE_MAX / 2U)
        return KV_OVERFLOW;

    new_capacity = store->capacity * 2U;
    new_buckets = calloc(new_capacity, sizeof(*new_buckets));

    if (new_buckets == NULL)
        return KV_OUT_OF_MEMORY;

    for (i = 0; i < store->capacity; ++i) {
        kv_entry_t *entry = store->buckets[i];

        while (entry != NULL) {
            kv_entry_t *next = entry->next;
            size_t index = hash_key(entry->key) % new_capacity;

            entry->next = new_buckets[index];
            new_buckets[index] = entry;
            entry = next;
        }
    }

    free(store->buckets);
    store->buckets = new_buckets;
    store->capacity = new_capacity;

    return KV_OK;
}

kv_status_t kv_store_create(kv_store_t **out_store)
{
    kv_store_t *store;
    int result;

    if (out_store == NULL)
        return KV_INVALID_ARGUMENT;

    *out_store = NULL;

    store = calloc(1, sizeof(*store));
    if (store == NULL)
        return KV_OUT_OF_MEMORY;

    store->buckets = calloc(KV_INITIAL_CAPACITY, sizeof(*store->buckets));
    if (store->buckets == NULL) {
        free(store);
        return KV_OUT_OF_MEMORY;
    }

    store->capacity = KV_INITIAL_CAPACITY;

    result = pthread_mutex_init(&store->mutex, NULL);
    if (result != 0) {
        free(store->buckets);
        free(store);
        return KV_INTERNAL_ERROR;
    }

    *out_store = store;
    return KV_OK;
}

void kv_store_destroy(kv_store_t *store)
{
    size_t i;

    if (store == NULL)
        return;

    if (pthread_mutex_lock(&store->mutex) != 0)
        return;

    for (i = 0; i < store->capacity; ++i) {
        kv_entry_t *entry = store->buckets[i];

        while (entry != NULL) {
            kv_entry_t *next = entry->next;
            free_entry(entry);
            entry = next;
        }
    }

    free(store->buckets);
    store->buckets = NULL;
    store->capacity = 0;
    store->size = 0;

    pthread_mutex_unlock(&store->mutex);
    pthread_mutex_destroy(&store->mutex);
    free(store);
}

kv_status_t kv_store_set(kv_store_t *store,
                         const char *key,
                         const void *value,
                         size_t value_size,
                         uint64_t ttl_ms)
{
    uint64_t current_time;
    uint64_t expiration = 0;
    size_t index;
    kv_entry_t *entry;
    void *value_copy = NULL;
    char *key_copy = NULL;
    kv_status_t status;

    status = validate_store(store);
    if (status != KV_OK || !valid_key(key) ||
        (value == NULL && value_size != 0U))
        return KV_INVALID_ARGUMENT;

    current_time = now_ms();
    if (current_time == 0U)
        return KV_INTERNAL_ERROR;

    if (ttl_ms != 0U) {
        if (ttl_ms > UINT64_MAX - current_time)
            return KV_OVERFLOW;
        expiration = current_time + ttl_ms;
    }

    key_copy = strdup(key);
    if (key_copy == NULL)
        return KV_OUT_OF_MEMORY;

    if (value_size != 0U) {
        value_copy = malloc(value_size);
        if (value_copy == NULL) {
            free(key_copy);
            return KV_OUT_OF_MEMORY;
        }
        memcpy(value_copy, value, value_size);
    }

    if (pthread_mutex_lock(&store->mutex) != 0) {
        free(key_copy);
        free(value_copy);
        return KV_INTERNAL_ERROR;
    }

    remove_expired_locked(store, current_time);

    index = hash_key(key) % store->capacity;
    entry = store->buckets[index];

    while (entry != NULL && strcmp(entry->key, key) != 0)
        entry = entry->next;

    if (entry != NULL) {
        free(entry->value);
        free(key_copy);
        entry->value = value_copy;
        entry->value_size = value_size;
        entry->expires_at_ms = expiration;
        pthread_mutex_unlock(&store->mutex);
        return KV_OK;
    }

    if (store->size >= store->capacity * 3U / 4U) {
        status = resize_store_locked(store);
        if (status != KV_OK) {
            pthread_mutex_unlock(&store->mutex);
            free(key_copy);
            free(value_copy);
            return status;
        }
        index = hash_key(key) % store->capacity;
    }

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        pthread_mutex_unlock(&store->mutex);
        free(key_copy);
        free(value_copy);
        return KV_OUT_OF_MEMORY;
    }

    entry->key = key_copy;
    entry->value = value_copy;
    entry->value_size = value_size;
    entry->expires_at_ms = expiration;
    entry->next = store->buckets[index];
    store->buckets[index] = entry;
    store->size++;

    pthread_mutex_unlock(&store->mutex);
    return KV_OK;
}

kv_status_t kv_store_get(kv_store_t *store,
                         const char *key,
                         void **out_value,
                         size_t *out_value_size)
{
    uint64_t current_time;
    size_t index;
    kv_entry_t *entry;

    if (validate_store(store) != KV_OK || !valid_key(key) ||
        out_value == NULL || out_value_size == NULL)
        return KV_INVALID_ARGUMENT;

    *out_value = NULL;
    *out_value_size = 0;

    current_time = now_ms();
    if (current_time == 0U)
        return KV_INTERNAL_ERROR;

    if (pthread_mutex_lock(&store->mutex) != 0)
        return KV_INTERNAL_ERROR;

    remove_expired_locked(store, current_time);
    index = hash_key(key) % store->capacity;
    entry = store->buckets[index];

    while (entry != NULL && strcmp(entry->key, key) != 0)
        entry = entry->next;

    if (entry == NULL) {
        pthread_mutex_unlock(&store->mutex);
        return KV_NOT_FOUND;
    }

    if (entry->value_size != 0U) {
        *out_value = malloc(entry->value_size);
        if (*out_value == NULL) {
            pthread_mutex_unlock(&store->mutex);
            return KV_OUT_OF_MEMORY;
        }
        memcpy(*out_value, entry->value, entry->value_size);
    }

    *out_value_size = entry->value_size;
    pthread_mutex_unlock(&store->mutex);
    return KV_OK;
}

kv_status_t kv_store_delete(kv_store_t *store, const char *key)
{
    uint64_t current_time;
    size_t index;
    kv_entry_t **current;

    if (validate_store(store) != KV_OK || !valid_key(key))
        return KV_INVALID_ARGUMENT;

    current_time = now_ms();
    if (current_time == 0U)
        return KV_INTERNAL_ERROR;

    if (pthread_mutex_lock(&store->mutex) != 0)
        return KV_INTERNAL_ERROR;

    remove_expired_locked(store, current_time);
    index = hash_key(key) % store->capacity;
    current = &store->buckets[index];

    while (*current != NULL) {
        if (strcmp((*current)->key, key) == 0) {
            kv_entry_t *entry = *current;
            *current = entry->next;
            free_entry(entry);
            store->size--;
            pthread_mutex_unlock(&store->mutex);
            return KV_OK;
        }
        current = &(*current)->next;
    }

    pthread_mutex_unlock(&store->mutex);
    return KV_NOT_FOUND;
}
