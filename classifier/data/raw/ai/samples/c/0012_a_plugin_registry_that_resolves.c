#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*plugin_handler)(const char *input);

typedef struct {
    char *name;
    plugin_handler handler;
} plugin_entry;

typedef struct {
    plugin_entry *entries;
    size_t count;
    size_t capacity;
} plugin_registry;

static char *plugin_strdup(const char *source) {
    size_t length;
    char *copy;

    if (source == NULL) {
        return NULL;
    }

    length = strlen(source) + 1;
    copy = malloc(length);

    if (copy != NULL) {
        memcpy(copy, source, length);
    }

    return copy;
}

void plugin_registry_init(plugin_registry *registry) {
    if (registry == NULL) {
        return;
    }

    registry->entries = NULL;
    registry->count = 0;
    registry->capacity = 0;
}

void plugin_registry_destroy(plugin_registry *registry) {
    size_t i;

    if (registry == NULL) {
        return;
    }

    for (i = 0; i < registry->count; ++i) {
        free(registry->entries[i].name);
    }

    free(registry->entries);
    registry->entries = NULL;
    registry->count = 0;
    registry->capacity = 0;
}

static int plugin_registry_grow(plugin_registry *registry) {
    size_t new_capacity;
    plugin_entry *new_entries;

    new_capacity = registry->capacity == 0 ? 8 : registry->capacity * 2;
    new_entries = realloc(
        registry->entries,
        new_capacity * sizeof(*new_entries)
    );

    if (new_entries == NULL) {
        return 0;
    }

    registry->entries = new_entries;
    registry->capacity = new_capacity;
    return 1;
}

int plugin_registry_register(
    plugin_registry *registry,
    const char *name,
    plugin_handler handler
) {
    size_t i;
    char *name_copy;

    if (registry == NULL || name == NULL || name[0] == '\0' || handler == NULL) {
        return 0;
    }

    for (i = 0; i < registry->count; ++i) {
        if (strcmp(registry->entries[i].name, name) == 0) {
            registry->entries[i].handler = handler;
            return 1;
        }
    }

    if (registry->count == registry->capacity &&
        !plugin_registry_grow(registry)) {
        return 0;
    }

    name_copy = plugin_strdup(name);
    if (name_copy == NULL) {
        return 0;
    }

    registry->entries[registry->count].name = name_copy;
    registry->entries[registry->count].handler = handler;
    registry->count++;

    return 1;
}

plugin_handler plugin_registry_resolve(
    const plugin_registry *registry,
    const char *name
) {
    size_t i;

    if (registry == NULL || name == NULL) {
        return NULL;
    }

    for (i = 0; i < registry->count; ++i) {
        if (strcmp(registry->entries[i].name, name) == 0) {
            return registry->entries[i].handler;
        }
    }

    return NULL;
}

int plugin_registry_unregister(plugin_registry *registry, const char *name) {
    size_t i;

    if (registry == NULL || name == NULL) {
        return 0;
    }

    for (i = 0; i < registry->count; ++i) {
        if (strcmp(registry->entries[i].name, name) == 0) {
            free(registry->entries[i].name);

            if (i + 1 < registry->count) {
                registry->entries[i] = registry->entries[registry->count - 1];
            }

            registry->count--;
            return 1;
        }
    }

    return 0;
}

int plugin_registry_invoke(
    const plugin_registry *registry,
    const char *name,
    const char *input
) {
    plugin_handler handler;

    handler = plugin_registry_resolve(registry, name);

    if (handler == NULL) {
        return 0;
    }

    handler(input);
    return 1;
}
