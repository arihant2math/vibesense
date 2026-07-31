#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

typedef struct KVEntry {
    char *key;
    void *value;
    size_t value_size;
    uint64_t expires_at_ms;
    int has_expiry;
    struct KVEntry *next;
} KVEntry;

typedef struct {
    KVEntry **buckets;
    size_t capacity;
    size_t size;
} KVStore;

static uint64_t kv_now_ms(int *ok) {
    struct timespec ts;
    uint64_t seconds, milliseconds;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0 ||
        ts.tv_sec < 0 ||
        (uintmax_t)ts.tv_sec > UINT64_MAX / 1000ULL) {
        if (ok) *ok = 0;
        return 0;
    }

    seconds = (uint64_t)ts.tv_sec;
    milliseconds = (uint64_t)ts.tv_nsec / 1000000ULL;

    if (seconds > (UINT64_MAX - milliseconds) / 1000ULL) {
        if (ok) *ok = 0;
        return 0;
    }

    if (ok) *ok = 1;
    return seconds * 1000ULL + milliseconds;
}

static uint64_t kv_hash(const char *key) {
    uint64_t hash = 1469598103934665603ULL;

    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= 1099511628211ULL;
    }

    return hash;
}

static char *kv_strdup(const char *s) {
    size_t len;
    char *copy;

    if (!s) return NULL;
    len = strlen(s);

    if (len == SIZE_MAX) return NULL;

    copy = malloc(len + 1);
    if (!copy) return NULL;

    memcpy(copy, s, len + 1);
    return copy;
}

static void kv_entry_free(KVEntry *entry) {
    if (!entry) return;
    free(entry->key);
    free(entry->value);
    free(entry);
}

static int kv_is_expired(const KVEntry *entry, uint64_t now) {
    return entry->has_expiry && now >= entry->expires_at_ms;
}

static int kv_resize(KVStore *store, size_t new_capacity) {
    KVEntry **new_buckets;
    size_t i;

    if (!store || new_capacity == 0 ||
        new_capacity > SIZE_MAX / sizeof(*new_buckets)) {
        return 0;
    }

    new_buckets = calloc(new_capacity, sizeof(*new_buckets));
    if (!new_buckets) return 0;

    for (i = 0; i < store->capacity; ++i) {
        KVEntry *entry = store->buckets[i];

        while (entry) {
            KVEntry *next = entry->next;
            size_t index = (size_t)(kv_hash(entry->key) % new_capacity);

            entry->next = new_buckets[index];
            new_buckets[index] = entry;
            entry = next;
        }
    }

    free(store->buckets);
    store->buckets = new_buckets;
    store->capacity = new_capacity;
    return 1;
}

KVStore *kv_store_create(size_t initial_capacity) {
    KVStore *store;

    if (initial_capacity == 0)
        initial_capacity = 16;

    if (initial_capacity > SIZE_MAX / sizeof(KVEntry *))
        return NULL;

    store = calloc(1, sizeof(*store));
    if (!store) return NULL;

    store->buckets = calloc(initial_capacity, sizeof(*store->buckets));
    if (!store->buckets) {
        free(store);
        return NULL;
    }

    store->capacity = initial_capacity;
    return store;
}

void kv_store_destroy(KVStore *store) {
    size_t i;

    if (!store) return;

    for (i = 0; i < store->capacity; ++i) {
        KVEntry *entry = store->buckets[i];

        while (entry) {
            KVEntry *next = entry->next;
            kv_entry_free(entry);
            entry = next;
        }
    }

    free(store->buckets);
    free(store);
}

int kv_store_put(KVStore *store,
                 const char *key,
                 const void *value,
                 size_t value_size,
                 uint64_t ttl_ms) {
    int now_ok;
    uint64_t now;
    uint64_t expires_at = 0;
    int has_expiry;
    size_t index;
    KVEntry *entry;
    void *value_copy = NULL;
    char *key_copy = NULL;

    if (!store || !store->buckets || !key || (value_size > 0 && !value))
        return 0;

    now = kv_now_ms(&now_ok);
    if (!now_ok) return 0;

    has_expiry = ttl_ms != UINT64_MAX;
    if (has_expiry) {
        if (ttl_ms > UINT64_MAX - now)
            return 0;
        expires_at = now + ttl_ms;
    }

    index = (size_t)(kv_hash(key) % store->capacity);
    entry = store->buckets[index];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (kv_is_expired(entry, now)) {
                break;
            }

            if (value_size > 0) {
                value_copy = malloc(value_size);
                if (!value_copy) return 0;
                memcpy(value_copy, value, value_size);
            }

            free(entry->value);
            entry->value = value_copy;
            entry->value_size = value_size;
            entry->expires_at_ms = expires_at;
            entry->has_expiry = has_expiry;
            return 1;
        }

        entry = entry->next;
    }

    if (store->size == SIZE_MAX)
        return 0;

    if (store->size >= store->capacity - store->capacity / 4) {
        size_t new_capacity;

        if (store->capacity > SIZE_MAX / 2)
            return 0;

        new_capacity = store->capacity * 2;
        if (!kv_resize(store, new_capacity))
            return 0;

        index = (size_t)(kv_hash(key) % store->capacity);
    }

    key_copy = kv_strdup(key);
    if (!key_copy) return 0;

    if (value_size > 0) {
        value_copy = malloc(value_size);
        if (!value_copy) {
            free(key_copy);
            return 0;
        }
        memcpy(value_copy, value, value_size);
    }

    entry = malloc(sizeof(*entry));
    if (!entry) {
        free(key_copy);
        free(value_copy);
        return 0;
    }

    entry->key = key_copy;
    entry->value = value_copy;
    entry->value_size = value_size;
    entry->expires_at_ms = expires_at;
    entry->has_expiry = has_expiry;
    entry->next = store->buckets[index];
    store->buckets[index] = entry;
    store->size++;

    return 1;
}

int kv_store_get(KVStore *store,
                 const char *key,
                 void *out_value,
                 size_t out_size,
                 size_t *actual_size) {
    int now_ok;
    uint64_t now;
    size_t index;
    KVEntry **link;

    if (actual_size) *actual_size = 0;
    if (!store || !store->buckets || !key)
        return 0;

    now = kv_now_ms(&now_ok);
    if (!now_ok) return 0;

    index = (size_t)(kv_hash(key) % store->capacity);
    link = &store->buckets[index];

    while (*link) {
        KVEntry *entry = *link;

        if (strcmp(entry->key, key) == 0) {
            if (kv_is_expired(entry, now)) {
                *link = entry->next;
                kv_entry_free(entry);
                store->size--;
                return 0;
            }

            if (actual_size) *actual_size = entry->value_size;
            if (entry->value_size > out_size || (entry->value_size > 0 && !out_value))
                return 0;

            if (entry->value_size > 0)
                memcpy(out_value, entry->value, entry->value_size);

            return 1;
        }

        link = &entry->next;
    }

    return 0;
}

int kv_store_delete(KVStore *store, const char *key) {
    size_t index;
    KVEntry **link;

    if (!store || !store->buckets || !key)
        return 0;

    index = (size_t)(kv_hash(key) % store->capacity);
    link = &store->buckets[index];

    while (*link) {
        KVEntry *entry = *link;

        if (strcmp(entry->key, key) == 0) {
            *link = entry->next;
            kv_entry_free(entry);
            store->size--;
            return 1;
        }

        link = &entry->next;
    }

    return 0;
}

void kv_store_expire(KVStore *store) {
    int now_ok;
    uint64_t now;
    size_t i;

    if (!store || !store->buckets)
        return;

    now = kv_now_ms(&now_ok);
    if (!now_ok) return;

    for (i = 0; i < store->capacity; ++i) {
        KVEntry **link = &store->buckets[i];

        while (*link) {
            KVEntry *entry = *link;

            if (kv_is_expired(entry, now)) {
                *link = entry->next;
                kv_entry_free(entry);
                store->size--;
            } else {
                link = &entry->next;
            }
        }
    }
}
