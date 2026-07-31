#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ECHO_BUFFER_SIZE 4096

typedef ssize_t (*read_fn)(int, void *, size_t);
typedef ssize_t (*write_fn)(int, const void *, size_t);

static ssize_t socket_read(int fd, void *buf, size_t count)
{
    return recv(fd, buf, count, 0);
}

static ssize_t socket_write(int fd, const void *buf, size_t count)
{
    return send(fd, buf, count, 0);
}

int echo_client(int client_fd, read_fn reader, write_fn writer)
{
    char buffer[ECHO_BUFFER_SIZE];

    for (;;) {
        ssize_t bytes_read = reader(client_fd, buffer, sizeof(buffer));

        if (bytes_read == 0)
            return 0;

        if (bytes_read < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        size_t sent = 0;
        while (sent < (size_t)bytes_read) {
            ssize_t bytes_written = writer(
                client_fd,
                buffer + sent,
                (size_t)bytes_read - sent
            );

            if (bytes_written < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            }

            if (bytes_written == 0)
                return -1;

            sent += (size_t)bytes_written;
        }
    }
}

int echo_client_socket(int client_fd)
{
    return echo_client(client_fd, socket_read, socket_write);
}

int create_echo_server(unsigned short port, int backlog)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        return -1;

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        return -1;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(server_fd, backlog) < 0) {
        close(server_fd);
        return -1;
    }

    return server_fd;
}

int run_echo_server(int server_fd)
{
    for (;;) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        echo_client_socket(client_fd);
        close(client_fd);
    }
}

#ifdef ECHO_SERVER_MAIN
int main(int argc, char **argv)
{
    unsigned short port = 12345;

    if (argc > 1) {
        char *end;
        long value = strtol(argv[1], &end, 10);
        if (*argv[1] == '\0' || *end != '\0' ||
            value < 1 || value > 65535)
            return 1;
        port = (unsigned short)value;
    }

    int server_fd = create_echo_server(port, 16);
    if (server_fd < 0)
        return 1;

    int result = run_echo_server(server_fd);
    close(server_fd);
    return result < 0 ? 1 : 0;
}
#endif
