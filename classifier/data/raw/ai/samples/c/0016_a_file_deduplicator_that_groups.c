#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BUFFER_SIZE 65536

typedef struct {
    const char *path;
    uint64_t hash;
    unsigned long long size;
    int group;
} FileInfo;

static int file_hash(const char *path, uint64_t *hash, unsigned long long *size) {
    FILE *f = fopen(path, "rb");
    unsigned char buffer[BUFFER_SIZE];
    size_t n;
    uint64_t h = UINT64_C(14695981039346656037);
    unsigned long long total = 0;

    if (!f)
        return 0;

    while ((n = fread(buffer, 1, sizeof(buffer), f)) != 0) {
        size_t i;
        total += n;
        for (i = 0; i < n; ++i) {
            h ^= buffer[i];
            h *= UINT64_C(1099511628211);
        }
    }

    if (ferror(f)) {
        fclose(f);
        return 0;
    }

    fclose(f);
    *hash = h;
    *size = total;
    return 1;
}

static int files_equal(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb");
    FILE *fb = fopen(b, "rb");
    unsigned char ba[BUFFER_SIZE], bb[BUFFER_SIZE];
    size_t na, nb;
    int equal = 1;

    if (!fa || !fb) {
        if (fa) fclose(fa);
        if (fb) fclose(fb);
        return 0;
    }

    do {
        na = fread(ba, 1, sizeof(ba), fa);
        nb = fread(bb, 1, sizeof(bb), fb);

        if (na != nb || memcmp(ba, bb, na) != 0) {
            equal = 0;
            break;
        }
    } while (na != 0);

    if (ferror(fa) || ferror(fb))
        equal = 0;

    fclose(fa);
    fclose(fb);
    return equal;
}

int main(int argc, char **argv) {
    FileInfo *files;
    int count, i, next_group = 1;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s FILE...\n", argv[0]);
        return EXIT_FAILURE;
    }

    count = argc - 1;
    files = calloc((size_t)count, sizeof(*files));
    if (!files) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    for (i = 0; i < count; ++i) {
        files[i].path = argv[i + 1];
        if (!file_hash(files[i].path, &files[i].hash, &files[i].size)) {
            fprintf(stderr, "Cannot read: %s\n", files[i].path);
            files[i].group = -1;
        }
    }

    for (i = 0; i < count; ++i) {
        int j;

        if (files[i].group != 0)
            continue;

        files[i].group = next_group;

        for (j = i + 1; j < count; ++j) {
            if (files[j].group == 0 &&
                files[j].hash == files[i].hash &&
                files[j].size == files[i].size &&
                files_equal(files[i].path, files[j].path)) {
                files[j].group = next_group;
            }
        }

        next_group++;
    }

    for (i = 1; i < next_group; ++i) {
        int printed = 0;
        int j;

        for (j = 0; j < count; ++j) {
            if (files[j].group == i) {
                if (!printed) {
                    printf("Group %d:\n", i);
                    printed = 1;
                }
                printf("  %s\n", files[j].path);
            }
        }
    }

    free(files);
    return EXIT_SUCCESS;
}
