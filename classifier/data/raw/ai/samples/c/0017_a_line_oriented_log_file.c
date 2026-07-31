#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef int (*tailer_line_fn)(const char *line, size_t length, void *context);

typedef struct {
    char *buffer;
    size_t length;
    size_t capacity;
    size_t max_line_length;
    tailer_line_fn callback;
    void *context;
} tailer_t;

enum {
    TAILER_OK = 0,
    TAILER_ERROR = -1,
    TAILER_STOPPED = 1,
    TAILER_WOULD_BLOCK = 2,
    TAILER_EOF = 3,
    TAILER_LINE_TOO_LONG = 4
};

static int tailer_reserve(tailer_t *tailer, size_t required)
{
    size_t capacity;
    char *buffer;

    if (required <= tailer->capacity)
        return TAILER_OK;

    capacity = tailer->capacity ? tailer->capacity : 4096;
    while (capacity < required) {
        if (capacity > (size_t)-1 / 2)
            capacity = required;
        else
            capacity *= 2;
    }

    buffer = realloc(tailer->buffer, capacity);
    if (!buffer)
        return TAILER_ERROR;

    tailer->buffer = buffer;
    tailer->capacity = capacity;
    return TAILER_OK;
}

int tailer_init(tailer_t *tailer,
                size_t max_line_length,
                tailer_line_fn callback,
                void *context)
{
    if (!tailer || !callback)
        return TAILER_ERROR;

    memset(tailer, 0, sizeof(*tailer));
    tailer->max_line_length = max_line_length;
    tailer->callback = callback;
    tailer->context = context;
    return TAILER_OK;
}

void tailer_destroy(tailer_t *tailer)
{
    if (!tailer)
        return;

    free(tailer->buffer);
    memset(tailer, 0, sizeof(*tailer));
}

void tailer_reset(tailer_t *tailer)
{
    if (tailer)
        tailer->length = 0;
}

int tailer_feed(tailer_t *tailer, const void *data, size_t length)
{
    const char *bytes = data;
    size_t start = 0;
    size_t i;

    if (!tailer || (!data && length != 0))
        return TAILER_ERROR;

    if (length == 0)
        return TAILER_OK;

    if (tailer->max_line_length &&
        tailer->length > tailer->max_line_length - 1)
        return TAILER_LINE_TOO_LONG;

    if (tailer->max_line_length &&
        length > tailer->max_line_length - tailer->length)
        return TAILER_LINE_TOO_LONG;

    if (tailer_reserve(tailer, tailer->length + length) != TAILER_OK)
        return TAILER_ERROR;

    memcpy(tailer->buffer + tailer->length, bytes, length);
    tailer->length += length;

    for (i = 0; i < tailer->length; ++i) {
        size_t line_length;
        int result;

        if (tailer->buffer[i] != '\n')
            continue;

        line_length = i - start;
        if (line_length && tailer->buffer[start + line_length - 1] == '\r')
            --line_length;

        result = tailer->callback(tailer->buffer + start,
                                  line_length,
                                  tailer->context);
        if (result)
            return result;

        start = i + 1;
    }

    if (start) {
        memmove(tailer->buffer,
                tailer->buffer + start,
                tailer->length - start);
        tailer->length -= start;
    }

    return TAILER_OK;
}

int tailer_finish(tailer_t *tailer)
{
    size_t length;
    int result;

    if (!tailer)
        return TAILER_ERROR;

    if (tailer->length == 0)
        return TAILER_OK;

    length = tailer->length;
    if (length && tailer->buffer[length - 1] == '\r')
        --length;

    result = tailer->callback(tailer->buffer, length, tailer->context);
    tailer->length = 0;
    return result;
}

int tailer_poll(tailer_t *tailer, int fd)
{
    char buffer[8192];
    ssize_t count;
    int result;

    if (!tailer || fd < 0)
        return TAILER_ERROR;

    for (;;) {
        count = read(fd, buffer, sizeof(buffer));

        if (count > 0) {
            result = tailer_feed(tailer, buffer, (size_t)count);
            if (result != TAILER_OK)
                return result;
            continue;
        }

        if (count == 0) {
            result = tailer_finish(tailer);
            return result == TAILER_OK ? TAILER_EOF : result;
        }

        if (errno == EINTR)
            continue;

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return TAILER_WOULD_BLOCK;

        return TAILER_ERROR;
    }
}
