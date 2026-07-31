#include <stdio.h>
#include <string.h>

#define MAX_PLUGINS 32
#define MAX_NAME_LENGTH 64

typedef void (*Handler)(const char *input);

typedef struct {
    char name[MAX_NAME_LENGTH];
    Handler handler;
} Plugin;

typedef struct {
    Plugin plugins[MAX_PLUGINS];
    size_t count;
} PluginRegistry;

void registry_init(PluginRegistry *registry) {
    registry->count = 0;
}

int registry_register(PluginRegistry *registry, const char *name, Handler handler) {
    if (!registry || !name || !handler || registry->count >= MAX_PLUGINS) {
        return 0;
    }

    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->plugins[i].name, name) == 0) {
            return 0;
        }
    }

    strncpy(registry->plugins[registry->count].name, name, MAX_NAME_LENGTH - 1);
    registry->plugins[registry->count].name[MAX_NAME_LENGTH - 1] = '\0';
    registry->plugins[registry->count].handler = handler;
    registry->count++;

    return 1;
}

Handler registry_resolve(const PluginRegistry *registry, const char *name) {
    if (!registry || !name) {
        return NULL;
    }

    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->plugins[i].name, name) == 0) {
            return registry->plugins[i].handler;
        }
    }

    return NULL;
}

void hello_handler(const char *input) {
    printf("Hello, %s!\n", input);
}

void uppercase_handler(const char *input) {
    printf("UPPERCASE: ");
    for (const char *p = input; *p; p++) {
        char c = *p;
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        putchar(c);
    }
    putchar('\n');
}

void reverse_handler(const char *input) {
    size_t length = strlen(input);

    printf("REVERSED: ");
    while (length > 0) {
        putchar(input[--length]);
    }
    putchar('\n');
}

int main(void) {
    PluginRegistry registry;
    registry_init(&registry);

    registry_register(&registry, "hello", hello_handler);
    registry_register(&registry, "uppercase", uppercase_handler);
    registry_register(&registry, "reverse", reverse_handler);

    const char *names[] = {"hello", "uppercase", "reverse", "missing"};
    const char *input = "plugin registry";

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        Handler handler = registry_resolve(&registry, names[i]);

        if (handler) {
            handler(input);
        } else {
            printf("No handler registered for \"%s\".\n", names[i]);
        }
    }

    return 0;
}
