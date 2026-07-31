#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define INITIAL_CAPACITY 16
#define LOAD_FACTOR_NUMERATOR 7
#define LOAD_FACTOR_DENOMINATOR 10

typedef struct {
    char *key;
    int value;
    bool occupied;
    bool deleted;
} HashEntry;

typedef struct {
    HashEntry *entries;
    size_t capacity;
    size_t size;
} HashTable;

static unsigned long hash_string(const char *key) {
    unsigned long hash = 5381;
    int c;

    while ((c = (unsigned char)*key++))
        hash = ((hash << 5) + hash) ^ (unsigned long)c;

    return hash;
}

static char *duplicate_string(const char *source) {
    size_t length = strlen(source) + 1;
    char *copy = malloc(length);

    if (copy != NULL)
        memcpy(copy, source, length);

    return copy;
}

static bool hash_table_resize(HashTable *table, size_t new_capacity) {
    HashEntry *old_entries = table->entries;
    size_t old_capacity = table->capacity;

    HashEntry *new_entries = calloc(new_capacity, sizeof(HashEntry));
    if (new_entries == NULL)
        return false;

    table->entries = new_entries;
    table->capacity = new_capacity;
    table->size = 0;

    for (size_t i = 0; i < old_capacity; ++i) {
        if (old_entries[i].occupied && !old_entries[i].deleted) {
            size_t index = hash_string(old_entries[i].key) % table->capacity;

            while (table->entries[index].occupied)
                index = (index + 1) % table->capacity;

            table->entries[index] = old_entries[i];
            table->size++;
        }
    }

    free(old_entries);
    return true;
}

HashTable *hash_table_create(void) {
    HashTable *table = malloc(sizeof(HashTable));
    if (table == NULL)
        return NULL;

    table->capacity = INITIAL_CAPACITY;
    table->size = 0;
    table->entries = calloc(table->capacity, sizeof(HashEntry));

    if (table->entries == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

void hash_table_destroy(HashTable *table) {
    if (table == NULL)
        return;

    for (size_t i = 0; i < table->capacity; ++i) {
        if (table->entries[i].occupied)
            free(table->entries[i].key);
    }

    free(table->entries);
    free(table);
}

bool hash_table_set(HashTable *table, const char *key, int value) {
    if (table == NULL || key == NULL)
        return false;

    if ((table->size + 1) * LOAD_FACTOR_DENOMINATOR >=
        table->capacity * LOAD_FACTOR_NUMERATOR) {
        if (!hash_table_resize(table, table->capacity * 2))
            return false;
    }

    size_t index = hash_string(key) % table->capacity;
    size_t first_deleted = table->capacity;

    while (table->entries[index].occupied) {
        HashEntry *entry = &table->entries[index];

        if (!entry->deleted && strcmp(entry->key, key) == 0) {
            entry->value = value;
            return true;
        }

        if (entry->deleted && first_deleted == table->capacity)
            first_deleted = index;

        index = (index + 1) % table->capacity;
    }

    if (first_deleted != table->capacity)
        index = first_deleted;

    char *key_copy = duplicate_string(key);
    if (key_copy == NULL)
        return false;

    table->entries[index].key = key_copy;
    table->entries[index].value = value;
    table->entries[index].occupied = true;
    table->entries[index].deleted = false;
    table->size++;

    return true;
}

bool hash_table_get(const HashTable *table, const char *key, int *value) {
    if (table == NULL || key == NULL)
        return false;

    size_t index = hash_string(key) % table->capacity;
    size_t start = index;

    do {
        const HashEntry *entry = &table->entries[index];

        if (!entry->occupied && !entry->deleted)
            return false;

        if (entry->occupied && !entry->deleted && strcmp(entry->key, key) == 0) {
            if (value != NULL)
                *value = entry->value;
            return true;
        }

        index = (index + 1) % table->capacity;
    } while (index != start);

    return false;
}

bool hash_table_remove(HashTable *table, const char *key) {
    if (table == NULL || key == NULL)
        return false;

    size_t index = hash_string(key) % table->capacity;
    size_t start = index;

    do {
        HashEntry *entry = &table->entries[index];

        if (!entry->occupied && !entry->deleted)
            return false;

        if (entry->occupied && !entry->deleted && strcmp(entry->key, key) == 0) {
            free(entry->key);
            entry->key = NULL;
            entry->occupied = false;
            entry->deleted = true;
            table->size--;
            return true;
        }

        index = (index + 1) % table->capacity;
    } while (index != start);

    return false;
}

int main(void) {
    HashTable *table = hash_table_create();
    if (table == NULL)
        return EXIT_FAILURE;

    hash_table_set(table, "apple", 10);
    hash_table_set(table, "banana", 20);
    hash_table_set(table, "cherry", 30);

    int value;
    if (hash_table_get(table, "banana", &value))
        printf("banana: %d\n", value);

    hash_table_remove(table, "banana");

    hash_table_destroy(table);
    return EXIT_SUCCESS;
}
