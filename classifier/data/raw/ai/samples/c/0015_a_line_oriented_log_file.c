#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef TAIL_BUFFER_SIZE
#define TAIL_BUFFER_SIZE 8192
#endif

typedef struct {
    int fd;
    char buffer[TAIL_BUFFER_SIZE];
    size_t pos;
    size_t len;
    ino_t inode;
    off_t offset;
} log_tailer;

static int tailer_open(log_tailer *t, const char *path)
{
    struct stat st;

    t->fd = open(path, O_RDONLY | O_CLOEXEC);
    if (t->fd < 0)
        return -1;

    if (fstat(t->fd, &st) < 0) {
        close(t->fd);
        t->fd = -1;
        return -1;
    }

    t->inode = st.st_ino;
    t->offset = 0;
    t->pos = 0;
    t->len = 0;
    return 0;
}

static int tailer_reopen_if_rotated(log_tailer *t, const char *path)
{
    struct stat path_st;

    if (stat(path, &path_st) < 0)
        return 0;

    if (path_st.st_ino == t->inode)
        return 0;

    close(t->fd);
    return tailer_open(t, path);
}

static ssize_t tailer_read_line(log_tailer *t, char **line, size_t *length)
{
    static char output[TAIL_BUFFER_SIZE];
    size_t used = 0;

    for (;;) {
        if (t->pos == t->len) {
            ssize_t n = read(t->fd, t->buffer, sizeof(t->buffer));

            if (n == 0)
                return 0;

            if (n < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            }

            t->pos = 0;
            t->len = (size_t)n;
        }

        while (t->pos < t->len) {
            char c = t->buffer[t->pos++];

            if (used + 1 >= sizeof(output))
                return -2;

            output[used++] = c;
            t->offset++;

            if (c == '\n') {
                output[used] = '\0';
                *line = output;
                *length = used;
                return 1;
            }
        }
    }
}

int main(int argc, char **argv)
{
    const char *path;
    log_tailer tailer;
    char *line;
    size_t length;

    if (argc != 2) {
        fprintf(stderr, "usage: %s LOG_FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    path = argv[1];
    tailer.fd = -1;

    if (tailer_open(&tailer, path) < 0) {
        perror(path);
        return EXIT_FAILURE;
    }

    for (;;) {
        int result = tailer_read_line(&tailer, &line, &length);

        if (result == 1) {
            if (fwrite(line, 1, length, stdout) != length) {
                perror("stdout");
                break;
            }
            fflush(stdout);
        } else if (result == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            break;
        } else if (result == -2) {
            fprintf(stderr, "line exceeds buffer capacity\n");
            break;
        } else {
            usleep(100000);

            if (tailer_reopen_if_rotated(&tailer, path) < 0) {
                if (errno != ENOENT)
                    perror(path);
            } else {
                struct stat st;

                if (fstat(tailer.fd, &st) == 0 &&
                    st.st_size < tailer.offset) {
                    close(tailer.fd);
                    if (tailer_open(&tailer, path) < 0 && errno != ENOENT)
                        perror(path);
                }
            }
        }
    }

    if (tailer.fd >= 0)
        close(tailer.fd);

    return EXIT_FAILURE;
}
