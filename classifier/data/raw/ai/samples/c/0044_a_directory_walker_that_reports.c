#define _XOPEN_SOURCE 700

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    char *extension;
    uintmax_t bytes;
} ExtensionTotal;

typedef struct {
    ExtensionTotal *items;
    size_t count;
    size_t capacity;
} ExtensionTotals;

static void totals_free(ExtensionTotals *totals)
{
    size_t i;

    for (i = 0; i < totals->count; ++i)
        free(totals->items[i].extension);

    free(totals->items);
}

static int totals_add(ExtensionTotals *totals, const char *extension,
                      uintmax_t bytes)
{
    size_t i;
    ExtensionTotal *new_items;
    char *copy;

    for (i = 0; i < totals->count; ++i) {
        if (strcmp(totals->items[i].extension, extension) == 0) {
            totals->items[i].bytes += bytes;
            return 0;
        }
    }

    if (totals->count == totals->capacity) {
        size_t new_capacity = totals->capacity ? totals->capacity * 2 : 16;

        new_items = realloc(totals->items,
                            new_capacity * sizeof(*new_items));
        if (!new_items)
            return -1;

        totals->items = new_items;
        totals->capacity = new_capacity;
    }

    copy = malloc(strlen(extension) + 1);
    if (!copy)
        return -1;

    strcpy(copy, extension);
    totals->items[totals->count].extension = copy;
    totals->items[totals->count].bytes = bytes;
    ++totals->count;

    return 0;
}

static const char *file_extension(const char *path)
{
    const char *base = strrchr(path, '/');
    const char *dot;

    base = base ? base + 1 : path;
    dot = strrchr(base, '.');

    if (!dot || dot == base || dot[1] == '\0')
        return "[no extension]";

    return dot + 1;
}

static int walk_directory(const char *path, ExtensionTotals *totals)
{
    DIR *directory;
    struct dirent *entry;

    directory = opendir(path);
    if (!directory) {
        fprintf(stderr, "Cannot open '%s': %s\n", path, strerror(errno));
        return -1;
    }

    while ((entry = readdir(directory)) != NULL) {
        char *child_path;
        size_t path_length;
        size_t name_length;
        struct stat file_info;

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        path_length = strlen(path);
        name_length = strlen(entry->d_name);
        child_path = malloc(path_length + name_length + 2);
        if (!child_path) {
            closedir(directory);
            return -1;
        }

        memcpy(child_path, path, path_length);
        child_path[path_length] = '/';
        memcpy(child_path + path_length + 1, entry->d_name, name_length + 1);

        if (lstat(child_path, &file_info) != 0) {
            fprintf(stderr, "Cannot stat '%s': %s\n",
                    child_path, strerror(errno));
            free(child_path);
            continue;
        }

        if (S_ISDIR(file_info.st_mode)) {
            if (walk_directory(child_path, totals) != 0) {
                free(child_path);
                closedir(directory);
                return -1;
            }
        } else if (S_ISREG(file_info.st_mode)) {
            if (totals_add(totals, file_extension(child_path),
                           (uintmax_t)file_info.st_size) != 0) {
                free(child_path);
                closedir(directory);
                return -1;
            }
        }

        free(child_path);
    }

    closedir(directory);
    return 0;
}

/**
 * Walks a directory tree and accumulates regular-file sizes by extension.
 *
 * @param root Root directory to traverse.
 * @param totals Destination aggregate.
 * @return 0 on success, or -1 if traversal or memory allocation fails.
 */
int directory_sizes_by_extension(const char *root, ExtensionTotals *totals)
{
    if (!root || !totals) {
        errno = EINVAL;
        return -1;
    }

    totals->items = NULL;
    totals->count = 0;
    totals->capacity = 0;

    if (walk_directory(root, totals) != 0) {
        totals_free(totals);
        return -1;
    }

    return 0;
}

/**
 * Releases memory owned by an extension-size aggregate.
 *
 * @param totals Aggregate to release.
 */
void free_extension_totals(ExtensionTotals *totals)
{
    if (totals)
        totals_free(totals);
}

/**
 * Runs the directory walker and prints totals grouped by extension.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 * @return EXIT_SUCCESS on success, otherwise EXIT_FAILURE.
 */
int main(int argc, char **argv)
{
    ExtensionTotals totals;
    size_t i;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s DIRECTORY\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (directory_sizes_by_extension(argv[1], &totals) != 0)
        return EXIT_FAILURE;

    for (i = 0; i < totals.count; ++i)
        printf("%s %" PRIuMAX "\n",
               totals.items[i].extension, totals.items[i].bytes);

    free_extension_totals(&totals);
    return EXIT_SUCCESS;
}
