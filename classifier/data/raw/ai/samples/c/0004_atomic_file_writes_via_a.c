#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int atomic_write_file(const char *path, const void *data, size_t len)
{
    static const char suffix[] = ".tmp.XXXXXX";
    size_t path_len;
    size_t suffix_len;
    size_t total_len;
    char *tmp_path = NULL;
    const unsigned char *buf = data;
    size_t written = 0;
    int fd = -1;
    int saved_errno;

    if (path == NULL || (data == NULL && len != 0)) {
        errno = EINVAL;
        return -1;
    }

    path_len = strlen(path);
    suffix_len = sizeof(suffix) - 1;

    if (path_len > SIZE_MAX - suffix_len ||
        path_len + suffix_len == SIZE_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }

    total_len = path_len + suffix_len + 1;
    if (total_len < path_len || total_len < suffix_len) {
        errno = EOVERFLOW;
        return -1;
    }

    tmp_path = malloc(total_len);
    if (tmp_path == NULL)
        return -1;

    memcpy(tmp_path, path, path_len);
    memcpy(tmp_path + path_len, suffix, suffix_len + 1);

    fd = mkstemp(tmp_path);
    if (fd < 0) {
        free(tmp_path);
        return -1;
    }

    while (written < len) {
        size_t remaining = len - written;
        size_t chunk = remaining;

        if (chunk > (size_t)SSIZE_MAX)
            chunk = (size_t)SSIZE_MAX;

        ssize_t n = write(fd, buf + written, chunk);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            goto fail;
        }

        if (n == 0) {
            errno = EIO;
            goto fail;
        }

        if ((size_t)n > remaining) {
            errno = EIO;
            goto fail;
        }

        written += (size_t)n;
    }

    if (fsync(fd) < 0)
        goto fail;

    if (close(fd) < 0) {
        fd = -1;
        goto fail;
    }
    fd = -1;

    if (rename(tmp_path, path) < 0)
        goto fail;

    free(tmp_path);
    return 0;

fail:
    saved_errno = errno;
    if (fd >= 0)
        close(fd);
    unlink(tmp_path);
    free(tmp_path);
    errno = saved_errno;
    return -1;
}
