/*
 * Nedflix Nintendo Switch - Network subsystem
 * HTTP streaming client with BSD sockets
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <switch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

/* Network configuration */
#define HTTP_TIMEOUT_MS     10000
#define HTTP_BUFFER_SIZE    65536
#define MAX_HEADER_SIZE     8192
#define MAX_REDIRECTS       5

/* HTTP stream state */
static struct {
    bool initialized;
    bool connected;
    int socket;
    char host[256];
    int port;
    char path[1024];

    /* Stream info */
    size_t content_length;
    size_t bytes_received;
    bool chunked;
    bool keep_alive;

    /* Buffer for partial data */
    uint8_t *buffer;
    size_t buffer_size;
    size_t buffer_pos;
    size_t buffer_len;
} http_state;

/* Initialize network subsystem */
int network_init(void)
{
    printf("Initializing network...\n");

    memset(&http_state, 0, sizeof(http_state));
    http_state.socket = -1;

    /* Allocate buffer */
    http_state.buffer = malloc(HTTP_BUFFER_SIZE);
    if (!http_state.buffer) {
        printf("Failed to allocate network buffer\n");
        return -1;
    }
    http_state.buffer_size = HTTP_BUFFER_SIZE;

    http_state.initialized = true;
    printf("Network initialized\n");
    return 0;
}

/* Shutdown network */
void network_shutdown(void)
{
    if (!http_state.initialized) return;

    printf("Shutting down network...\n");

    http_stream_stop();

    if (http_state.buffer) {
        free(http_state.buffer);
        http_state.buffer = NULL;
    }

    http_state.initialized = false;
    printf("Network shutdown complete\n");
}

/* Parse URL into components */
static int parse_url(const char *url, char *host, size_t host_size,
                     int *port, char *path, size_t path_size)
{
    const char *p = url;

    /* Skip protocol */
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
        *port = 80;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        *port = 443;
    } else {
        return -1;
    }

    /* Extract host */
    const char *host_end = strchr(p, '/');
    const char *port_start = strchr(p, ':');

    if (port_start && (!host_end || port_start < host_end)) {
        /* Host with port */
        size_t host_len = port_start - p;
        if (host_len >= host_size) host_len = host_size - 1;
        strncpy(host, p, host_len);
        host[host_len] = '\0';

        *port = atoi(port_start + 1);
        p = host_end ? host_end : (port_start + strlen(port_start + 1) + 1);
    } else if (host_end) {
        /* Host without port */
        size_t host_len = host_end - p;
        if (host_len >= host_size) host_len = host_size - 1;
        strncpy(host, p, host_len);
        host[host_len] = '\0';
        p = host_end;
    } else {
        /* No path */
        strncpy(host, p, host_size - 1);
        host[host_size - 1] = '\0';
        strcpy(path, "/");
        return 0;
    }

    /* Extract path */
    if (*p == '/') {
        strncpy(path, p, path_size - 1);
        path[path_size - 1] = '\0';
    } else {
        strcpy(path, "/");
    }

    return 0;
}

/* Connect to host */
static int connect_to_host(const char *host, int port)
{
    struct addrinfo hints, *res, *rp;
    char port_str[16];
    int sock = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(port_str, sizeof(port_str), "%d", port);

    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(err));
        return -1;
    }

    /* Try each address */
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;

        /* Set non-blocking for timeout */
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        int ret = connect(sock, rp->ai_addr, rp->ai_addrlen);
        if (ret < 0 && errno != EINPROGRESS) {
            close(sock);
            sock = -1;
            continue;
        }

        /* Wait for connection with timeout */
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLOUT;

        ret = poll(&pfd, 1, HTTP_TIMEOUT_MS);
        if (ret <= 0) {
            close(sock);
            sock = -1;
            continue;
        }

        /* Check for connection error */
        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
        if (error != 0) {
            close(sock);
            sock = -1;
            continue;
        }

        /* Restore blocking mode */
        fcntl(sock, F_SETFL, flags);
        break;
    }

    freeaddrinfo(res);
    return sock;
}

/* Send HTTP request */
static int send_request(int sock, const char *host, const char *path,
                        size_t range_start)
{
    char request[2048];
    int len;

    if (range_start > 0) {
        len = snprintf(request, sizeof(request),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: Nedflix-Switch/1.0\r\n"
            "Accept: */*\r\n"
            "Range: bytes=%zu-\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            path, host, range_start);
    } else {
        len = snprintf(request, sizeof(request),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: Nedflix-Switch/1.0\r\n"
            "Accept: */*\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            path, host);
    }

    int sent = 0;
    while (sent < len) {
        int ret = send(sock, request + sent, len - sent, 0);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                svcSleepThread(1000000);  /* 1ms */
                continue;
            }
            return -1;
        }
        sent += ret;
    }

    return 0;
}

/* Parse HTTP response headers */
static int parse_response(int sock, int *status, size_t *content_length, bool *chunked)
{
    char header[MAX_HEADER_SIZE];
    int header_len = 0;
    bool header_complete = false;

    *status = 0;
    *content_length = 0;
    *chunked = false;

    /* Read until we get \r\n\r\n */
    while (!header_complete && header_len < MAX_HEADER_SIZE - 1) {
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, HTTP_TIMEOUT_MS);
        if (ret <= 0) return -1;

        ret = recv(sock, header + header_len, 1, 0);
        if (ret <= 0) return -1;

        header_len++;
        header[header_len] = '\0';

        /* Check for end of headers */
        if (header_len >= 4) {
            if (strstr(header + header_len - 4, "\r\n\r\n")) {
                header_complete = true;
            }
        }
    }

    if (!header_complete) return -1;

    /* Parse status line */
    if (sscanf(header, "HTTP/%*s %d", status) != 1) {
        return -1;
    }

    /* Parse Content-Length */
    char *cl = strcasestr(header, "Content-Length:");
    if (cl) {
        *content_length = strtoull(cl + 15, NULL, 10);
    }

    /* Check for chunked encoding */
    if (strcasestr(header, "Transfer-Encoding: chunked")) {
        *chunked = true;
    }

    return 0;
}

/* Start HTTP stream */
int http_stream_start(const char *url)
{
    if (!http_state.initialized) {
        if (network_init() != 0) return -1;
    }

    /* Stop any existing stream */
    http_stream_stop();

    printf("Starting HTTP stream: %s\n", url);

    /* Parse URL */
    if (parse_url(url, http_state.host, sizeof(http_state.host),
                  &http_state.port, http_state.path, sizeof(http_state.path)) != 0) {
        printf("Invalid URL\n");
        return -1;
    }

    printf("Connecting to %s:%d%s\n", http_state.host, http_state.port, http_state.path);

    /* Connect */
    http_state.socket = connect_to_host(http_state.host, http_state.port);
    if (http_state.socket < 0) {
        printf("Connection failed\n");
        return -1;
    }

    /* Send request */
    if (send_request(http_state.socket, http_state.host, http_state.path, 0) != 0) {
        printf("Failed to send request\n");
        close(http_state.socket);
        http_state.socket = -1;
        return -1;
    }

    /* Parse response */
    int status;
    if (parse_response(http_state.socket, &status, &http_state.content_length,
                       &http_state.chunked) != 0) {
        printf("Failed to parse response\n");
        close(http_state.socket);
        http_state.socket = -1;
        return -1;
    }

    printf("HTTP status: %d, length: %zu, chunked: %d\n",
           status, http_state.content_length, http_state.chunked);

    /* Handle redirects */
    if (status >= 300 && status < 400) {
        printf("Redirect not implemented\n");
        close(http_state.socket);
        http_state.socket = -1;
        return -1;
    }

    if (status < 200 || status >= 300) {
        printf("HTTP error: %d\n", status);
        close(http_state.socket);
        http_state.socket = -1;
        return -1;
    }

    http_state.connected = true;
    http_state.bytes_received = 0;
    http_state.buffer_pos = 0;
    http_state.buffer_len = 0;

    return 0;
}

/* Read from HTTP stream */
int http_stream_read(void *buffer, size_t size)
{
    if (!http_state.connected || http_state.socket < 0) {
        return -1;
    }

    /* Check for end of content */
    if (http_state.content_length > 0 &&
        http_state.bytes_received >= http_state.content_length) {
        return 0;  /* EOF */
    }

    /* Poll for data */
    struct pollfd pfd;
    pfd.fd = http_state.socket;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, 100);  /* 100ms timeout */
    if (ret < 0) {
        printf("poll error: %d\n", errno);
        return -1;
    }
    if (ret == 0) {
        return -2;  /* Timeout, no data available */
    }

    /* Read data */
    ssize_t bytes = recv(http_state.socket, buffer, size, 0);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -2;  /* Try again */
        }
        printf("recv error: %d\n", errno);
        return -1;
    }
    if (bytes == 0) {
        return 0;  /* Connection closed */
    }

    http_state.bytes_received += bytes;
    return (int)bytes;
}

/* Stop HTTP stream */
void http_stream_stop(void)
{
    if (http_state.socket >= 0) {
        close(http_state.socket);
        http_state.socket = -1;
    }

    http_state.connected = false;
    http_state.content_length = 0;
    http_state.bytes_received = 0;

    printf("HTTP stream stopped\n");
}

/* Check network connection */
bool network_is_connected(void)
{
    NifmInternetConnectionStatus status;
    Result rc = nifmGetInternetConnectionStatus(NULL, NULL, &status);
    return R_SUCCEEDED(rc) && status == NifmInternetConnectionStatus_Connected;
}

/* Get connection info */
int network_get_info(char *ip, size_t ip_size, int *signal_strength)
{
    u32 addr;
    Result rc = nifmGetCurrentIpAddress(&addr);
    if (R_FAILED(rc)) return -1;

    if (ip) {
        struct in_addr in;
        in.s_addr = addr;
        strncpy(ip, inet_ntoa(in), ip_size - 1);
        ip[ip_size - 1] = '\0';
    }

    if (signal_strength) {
        /* Not directly available, estimate from connection type */
        *signal_strength = 100;
    }

    return 0;
}
