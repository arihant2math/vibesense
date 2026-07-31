#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define WAL_MAGIC 0x57414C31u
#define WAL_MAX_RECORD_SIZE (64u * 1024u * 1024u)

typedef int (*wal_replay_fn)(const void *data, uint32_t length, void *context);

/**
 * Appends a record to the write-ahead log at path.
 *
 * Returns 0 on success, or -1 on failure with errno set.
 */
int wal_append(const char *path, const void *data, uint32_t length)
{
    int fd;
    uint32_t header[3];
    uint32_t checksum = 2166136261u;
    const unsigned char *bytes = data;
    size_t written = 0;

    if (path == NULL || (data == NULL && length != 0) ||
        length > WAL_MAX_RECORD_SIZE) {
        errno = EINVAL;
        return -1;
    }

    for (uint32_t i = 0; i < length; ++i) {
        checksum ^= bytes[i];
        checksum *= 16777619u;
    }

    header[0] = WAL_MAGIC;
    header[1] = length;
    header[2] = checksum;

    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
        return -1;

    while (written < sizeof(header)) {
        ssize_t n = write(fd, (const unsigned char *)header + written,
                          sizeof(header) - written);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            close(fd);
            return -1;
        }
        written += (size_t)n;
    }

    written = 0;
    while (written < length) {
        ssize_t n = write(fd, bytes + written, length - written);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            close(fd);
            return -1;
        }
        written += (size_t)n;
    }

    if (fsync(fd) < 0) {
        close(fd);
        return -1;
    }

    if (close(fd) < 0)
        return -1;

    return 0;
}

/**
 * Replays complete, valid records from the write-ahead log at path.
 *
 * The callback is invoked once for each record. Returning nonzero from the
 * callback stops replay and causes wal_replay to return that value. A
 * truncated final record is ignored. Other malformed records cause -1 to be
 * returned with errno set.
 */
int wal_replay(const char *path, wal_replay_fn callback, void *context)
{
    int fd;
    unsigned char header[12];

    if (path == NULL || callback == NULL) {
        errno = EINVAL;
        return -1;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    for (;;) {
        size_t got = 0;

        while (got < sizeof(header)) {
            ssize_t n = read(fd, header + got, sizeof(header) - got);
            if (n == 0) {
                if (got == 0) {
                    close(fd);
                    return 0;
                }
                close(fd);
                return 0;
            }
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                close(fd);
                return -1;
            }
            got += (size_t)n;
        }

        uint32_t magic;
        uint32_t length;
        uint32_t expected_checksum;
        unsigned char *record;
        uint32_t checksum = 2166136261u;
        size_t offset = 0;

        memcpy(&magic, header, sizeof(magic));
        memcpy(&length, header + 4, sizeof(length));
        memcpy(&expected_checksum, header + 8, sizeof(expected_checksum));

        if (magic != WAL_MAGIC || length > WAL_MAX_RECORD_SIZE) {
            close(fd);
            errno = EINVAL;
            return -1;
        }

        record = malloc(length == 0 ? 1 : length);
        if (record == NULL) {
            close(fd);
            return -1;
        }

        while (offset < length) {
            ssize_t n = read(fd, record + offset, length - offset);
            if (n == 0) {
                free(record);
                close(fd);
                return 0;
            }
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                free(record);
                close(fd);
                return -1;
            }
            offset += (size_t)n;
        }

        for (uint32_t i = 0; i < length; ++i) {
            checksum ^= record[i];
            checksum *= 16777619u;
        }

        if (checksum != expected_checksum) {
            free(record);
            close(fd);
            errno = EIO;
            return -1;
        }

        int result = callback(record, length, context);
        free(record);

        if (result != 0) {
            close(fd);
            return result;
        }
    }
}
