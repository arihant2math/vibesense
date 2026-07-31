#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    LRU_OK = 0,
    LRU_ERR_INVALID_ARGUMENT,
    LRU_ERR_OUT_OF_MEMORY,
    LRU_ERR_NOT_FOUND,
    LRU_ERR_EXISTS,
    LRU_ERR_OVERFLOW
} lru_status_t;

typedef void (*lru_value_destructor_fn)(void *value);

typedef struct lru_entry {
    char *key;
    void *value;
    struct lru_entry *prev;
    struct lru_entry *next;
    struct lru_entry *hash_next;
} lru_entry_t;

typedef struct {
    size_t capacity;
    size_t size;
    size_t bucket_count;
    lru_entry_t **buckets;
    lru_entry_t *head;
    lru_entry_t *tail;
    lru_value_destructor_fn destroy_value;
} lru_cache_t;

static uint64_t lru_hash_string(const char *key)
{
    uint64_t hash = UINT64_C(14695981039346656037);

    while (*key != '\0') {
        hash ^= (unsigned char)*key++;
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}

static size_t lru_bucket_index(const lru_cache_t *cache, const char *key)
{
    return (size_t)(lru_hash_string(key) % cache->bucket_count);
}

static lru_entry_t *lru_find_entry(
    const lru_cache_t *cache,
    const char *key,
    lru_entry_t **previous)
{
    size_t index;
    lru_entry_t *entry;
    lru_entry_t *prev = NULL;

    if (previous != NULL) {
        *previous = NULL;
    }

    index = lru_bucket_index(cache, key);
    entry = cache->buckets[index];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (previous != NULL) {
                *previous = prev;
            }
            return entry;
        }

        prev = entry;
        entry = entry->hash_next;
    }

    return NULL;
}

static void lru_unlink_list(lru_cache_t *cache, lru_entry_t *entry)
{
    if (entry->prev != NULL) {
        entry->prev->next = entry->next;
    } else {
        cache->head = entry->next;
    }

    if (entry->next != NULL) {
        entry->next->prev = entry->prev;
    } else {
        cache->tail = entry->prev;
    }

    entry->prev = NULL;
    entry->next = NULL;
}

static void lru_link_front(lru_cache_t *cache, lru_entry_t *entry)
{
    entry->prev = NULL;
    entry->next = cache->head;

    if (cache->head != NULL) {
        cache->head->prev = entry;
    } else {
        cache->tail = entry;
    }

    cache->head = entry;
}

static void lru_remove_from_hash(
    lru_cache_t *cache,
    lru_entry_t *entry)
{
    size_t index = lru_bucket_index(cache, entry->key);
    lru_entry_t *current = cache->buckets[index];
    lru_entry_t *previous = NULL;

    while (current != NULL) {
        if (current == entry) {
            if (previous != NULL) {
                previous->hash_next = current->hash_next;
            } else {
                cache->buckets[index] = current->hash_next;
            }
            current->hash_next = NULL;
            return;
        }

        previous = current;
        current = current->hash_next;
    }
}

static void lru_destroy_entry(lru_cache_t *cache, lru_entry_t *entry)
{
    if (cache->destroy_value != NULL) {
        cache->destroy_value(entry->value);
    }

    free(entry->key);
    free(entry);
}

static void lru_evict_tail(lru_cache_t *cache)
{
    lru_entry_t *entry;

    if (cache == NULL || cache->tail == NULL) {
        return;
    }

    entry = cache->tail;
    lru_unlink_list(cache, entry);
    lru_remove_from_hash(cache, entry);
    lru_destroy_entry(cache, entry);
    cache->size--;
}

lru_status_t lru_cache_create(
    size_t capacity,
    size_t bucket_count,
    lru_value_destructor_fn destroy_value,
    lru_cache_t **out_cache)
{
    lru_cache_t *cache;

    if (out_cache == NULL || capacity == 0) {
        return LRU_ERR_INVALID_ARGUMENT;
    }

    if (bucket_count == 0) {
        bucket_count = capacity;
        if (bucket_count < 16) {
            bucket_count = 16;
        }
    }

    if (bucket_count > SIZE_MAX / sizeof(*cache->buckets)) {
        return LRU_ERR_OVERFLOW;
    }

    cache = calloc(1, sizeof(*cache));
    if (cache == NULL) {
        return LRU_ERR_OUT_OF_MEMORY;
    }

    cache->buckets = calloc(bucket_count, sizeof(*cache->buckets));
    if (cache->buckets == NULL) {
        free(cache);
        return LRU_ERR_OUT_OF_MEMORY;
    }

    cache->capacity = capacity;
    cache->bucket_count = bucket_count;
    cache->destroy_value = destroy_value;
    *out_cache = cache;

    return LRU_OK;
}

lru_status_t lru_cache_get(
    lru_cache_t *cache,
    const char *key,
    void **out_value)
{
    lru_entry_t *entry;

    if (cache == NULL || key == NULL || key[0] == '\0' || out_value == NULL) {
        return LRU_ERR_INVALID_ARGUMENT;
    }

    entry = lru_find_entry(cache, key, NULL);
    if (entry == NULL) {
        *out_value = NULL;
        return LRU_ERR_NOT_FOUND;
    }

    lru_unlink_list(cache, entry);
    lru_link_front(cache, entry);
    *out_value = entry->value;

    return LRU_OK;
}

lru_status_t lru_cache_put(
    lru_cache_t *cache,
    const char *key,
    void *value)
{
    lru_entry_t *entry;
    char *key_copy;
    size_t key_length;
    size_t index;

    if (cache == NULL || key == NULL || key[0] == '\0') {
        return LRU_ERR_INVALID_ARGUMENT;
    }

    key_length = strlen(key);
    if (key_length == SIZE_MAX) {
        return LRU_ERR_OVERFLOW;
    }

    entry = lru_find_entry(cache, key, NULL);
    if (entry != NULL) {
        if (cache->destroy_value != NULL && entry->value != value) {
            cache->destroy_value(entry->value);
        }

        entry->value = value;
        lru_unlink_list(cache, entry);
        lru_link_front(cache, entry);
        return LRU_OK;
    }

    key_copy = malloc(key_length + 1);
    if (key_copy == NULL) {
        return LRU_ERR_OUT_OF_MEMORY;
    }

    memcpy(key_copy, key, key_length + 1);

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        free(key_copy);
        return LRU_ERR_OUT_OF_MEMORY;
    }

    entry->key = key_copy;
    entry->value = value;

    index = lru_bucket_index(cache, key);
    entry->hash_next = cache->buckets[index];
    cache->buckets[index] = entry;

    lru_link_front(cache, entry);
    cache->size++;

    while (cache->size > cache->capacity) {
        lru_evict_tail(cache);
    }

    return LRU_OK;
}

lru_status_t lru_cache_remove(
    lru_cache_t *cache,
    const char *key)
{
    lru_entry_t *entry;

    if (cache == NULL || key == NULL || key[0] == '\0') {
        return LRU_ERR_INVALID_ARGUMENT;
    }

    entry = lru_find_entry(cache, key, NULL);
    if (entry == NULL) {
        return LRU_ERR_NOT_FOUND;
    }

    lru_unlink_list(cache, entry);
    lru_remove_from_hash(cache, entry);
    lru_destroy_entry(cache, entry);
    cache->size--;

    return LRU_OK;
}

lru_status_t lru_cache_clear(lru_cache_t *cache)
{
    lru_entry_t *entry;
    lru_entry_t *next;

    if (cache == NULL) {
        return LRU_ERR_INVALID_ARGUMENT;
    }

    entry = cache->head;
    while (entry != NULL) {
        next = entry->next;
        lru_destroy_entry(cache, entry);
        entry = next;
    }

    memset(cache->buckets, 0,
           cache->bucket_count * sizeof(*cache->buckets));
    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;

    return LRU_OK;
}

lru_status_t lru_cache_size(
    const lru_cache_t *cache,
    size_t *out_size)
{
    if (cache == NULL || out_size == NULL) {
        return LRU_ERR_INVALID_ARGUMENT;
    }

    *out_size = cache->size;
    return LRU_OK;
}

lru_status_t lru_cache_capacity(
    const lru_cache_t *cache,
    size_t *out_capacity)
{
    if (cache == NULL || out_capacity == NULL) {
        return LRU_ERR_INVALID_ARGUMENT;
    }

    *out_capacity = cache->capacity;
    return LRU_OK;
}

void lru_cache_destroy(lru_cache_t *cache)
{
    if (cache == NULL) {
        return;
    }

    (void)lru_cache_clear(cache);
    free(cache->buckets);
    free(cache);
}
