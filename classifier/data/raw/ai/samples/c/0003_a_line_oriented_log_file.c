#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

static int read_line(FILE *fp, char **buffer, size_t *capacity, size_t *length)
{
    int ch;

    if (fp == NULL || buffer == NULL || capacity == NULL || length == NULL) {
        errno = EINVAL;
        return -1;
    }

    *length = 0;

    for (;;) {
        ch = fgetc(fp);

        if (ch == EOF) {
            if (ferror(fp)) {
                return -1;
            }
            return *length == 0 ? 0 : 1;
        }

        if (*length == SIZE_MAX - 1) {
            errno = EOVERFLOW;
            return -1;
        }

        if (*length + 1 >= *capacity) {
            size_t new_capacity;
            char *new_buffer;

            if (*capacity == 0) {
                new_capacity = 128;
            } else {
                if (*capacity > SIZE_MAX / 2) {
                    errno = EOVERFLOW;
                    return -1;
                }
                new_capacity = *capacity * 2;
            }

            if (new_capacity <= *length + 1) {
                errno = EOVERFLOW;
                return -1;
            }

            new_buffer = realloc(*buffer, new_capacity);
            if (new_buffer == NULL) {
                return -1;
            }

            *buffer = new_buffer;
            *capacity = new_capacity;
        }

        (*buffer)[(*length)++] = (char)ch;

        if (ch == '\n') {
            break;
        }
    }

    (*buffer)[*length] = '\0';
    return 1;
}

static int follow_file(FILE *fp, const char *path)
{
    struct timespec delay = {0, 250000000L};
    struct stat st;
    off_t position;

    for (;;) {
        char *line = NULL;
        size_t capacity = 0;
        size_t length = 0;
        int result;

        while ((result = read_line(fp, &line, &capacity, &length)) > 0) {
            if (length > 0 && fwrite(line, 1, length, stdout) != length) {
                free(line);
                return -1;
            }
        }

        free(line);

        if (result < 0 || fflush(stdout) != 0) {
            return -1;
        }

        position = ftello(fp);
        if (position < 0) {
            return -1;
        }

        if (stat(path, &st) == 0 && st.st_size < position) {
            if (fseeko(fp, 0, SEEK_SET) != 0) {
                return -1;
            }
        }

        clearerr(fp);
        nanosleep(&delay, NULL);
    }
}

int main(int argc, char **argv)
{
    const char *path;
    int follow = 0;
    FILE *fp;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s [-f] file\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 3) {
        if (strcmp(argv[1], "-f") != 0) {
            fprintf(stderr, "usage: %s [-f] file\n", argv[0]);
            return EXIT_FAILURE;
        }
        follow = 1;
        path = argv[2];
    } else {
        path = argv[1];
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        perror(path);
        return EXIT_FAILURE;
    }

    if (follow) {
        if (follow_file(fp, path) != 0) {
            perror(path);
            fclose(fp);
            return EXIT_FAILURE;
        }
    } else {
        char *line = NULL;
        size_t capacity = 0;
        size_t length = 0;
        int result;

        while ((result = read_line(fp, &line, &capacity, &length)) > 0) {
            if (fwrite(line, 1, length, stdout) != length) {
                free(line);
                fclose(fp);
                return EXIT_FAILURE;
            }
        }

        free(line);

        if (result < 0 || fflush(stdout) != 0) {
            perror(path);
            fclose(fp);
            return EXIT_FAILURE;
        }
    }

    if (fclose(fp) != 0) {
        perror(path);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
