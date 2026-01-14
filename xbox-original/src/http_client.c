/*
 * Nedflix for Original Xbox
 * HTTP Client implementation
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef NXDK
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include <nxdk/net.h>
#else
/* POSIX sockets for non-Xbox builds */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#define closesocket close
#endif

/* Buffer sizes */
#define RECV_BUFFER_SIZE  4096
#define INITIAL_RESPONSE_SIZE 8192

/* HTTP response structure */
typedef struct {
    int status_code;
    char *headers;
    char *body;
    size_t body_length;
    size_t content_length;
} http_response_t;

/* Static state */
static bool g_http_initialized = false;

/*
 * Initialize HTTP client (network stack)
 */
int http_init(void)
{
    if (g_http_initialized) {
        return 0;
    }

#ifdef NXDK
    /* Initialize Xbox network */
    if (!nxNetInit(NULL)) {
        LOG_ERROR("Failed to initialize network");
        return -1;
    }

    /* Wait for DHCP */
    LOG("Waiting for network...");
    int timeout = 10;  /* 10 seconds */
    while (!nxNetIsInitialized() && timeout > 0) {
        Sleep(1000);
        timeout--;
    }

    if (!nxNetIsInitialized()) {
        LOG_ERROR("Network initialization timeout");
        return -1;
    }

    /* Log IP address */
    struct in_addr addr;
    nxNetGetIp(&addr);
    LOG("Network initialized, IP: %s", inet_ntoa(addr));
#endif

    g_http_initialized = true;
    return 0;
}

/*
 * Shutdown HTTP client
 */
void http_shutdown(void)
{
    if (!g_http_initialized) {
        return;
    }

#ifdef NXDK
    /* Nothing to cleanup for nxdk network */
#endif

    g_http_initialized = false;
}

/*
 * Parse URL into components
 */
static int parse_url(const char *url, char *host, int *port, char *path)
{
    const char *p = url;

    /* Skip protocol */
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
        *port = 80;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        *port = 443;  /* Note: HTTPS not supported on Xbox */
        LOG("Warning: HTTPS not supported, using HTTP");
        *port = 80;
    } else {
        return -1;
    }

    /* Extract host */
    const char *host_end = strchr(p, '/');
    const char *port_start = strchr(p, ':');

    if (port_start && (!host_end || port_start < host_end)) {
        /* Host with port */
        size_t host_len = port_start - p;
        strncpy(host, p, host_len);
        host[host_len] = '\0';
        *port = atoi(port_start + 1);
        p = host_end ? host_end : p + strlen(p);
    } else if (host_end) {
        /* Host without port */
        size_t host_len = host_end - p;
        strncpy(host, p, host_len);
        host[host_len] = '\0';
        p = host_end;
    } else {
        /* Just host */
        strcpy(host, p);
        p = "/";
    }

    /* Extract path */
    if (*p == '/') {
        strcpy(path, p);
    } else {
        strcpy(path, "/");
    }

    return 0;
}

/*
 * Create HTTP request string
 */
static char *create_request(const char *method, const char *host, const char *path,
                            const char *auth_token, const char *body)
{
    size_t size = 1024 + (body ? strlen(body) : 0) + (auth_token ? strlen(auth_token) : 0);
    char *request = (char *)malloc(size);
    if (!request) return NULL;

    int offset = 0;

    /* Request line */
    offset += snprintf(request + offset, size - offset, "%s %s HTTP/1.1\r\n", method, path);

    /* Headers */
    offset += snprintf(request + offset, size - offset, "Host: %s\r\n", host);
    offset += snprintf(request + offset, size - offset, "User-Agent: Nedflix-Xbox/1.0\r\n");
    offset += snprintf(request + offset, size - offset, "Accept: application/json\r\n");
    offset += snprintf(request + offset, size - offset, "Connection: close\r\n");

    if (auth_token && strlen(auth_token) > 0) {
        offset += snprintf(request + offset, size - offset, "Authorization: Bearer %s\r\n", auth_token);
    }

    if (body) {
        offset += snprintf(request + offset, size - offset, "Content-Type: application/json\r\n");
        offset += snprintf(request + offset, size - offset, "Content-Length: %zu\r\n", strlen(body));
    }

    /* End of headers */
    offset += snprintf(request + offset, size - offset, "\r\n");

    /* Body */
    if (body) {
        offset += snprintf(request + offset, size - offset, "%s", body);
    }

    return request;
}

/*
 * Parse HTTP response
 */
static int parse_response(const char *data, size_t len, http_response_t *response)
{
    /* Find status code */
    const char *status_start = strstr(data, "HTTP/1.");
    if (!status_start) return -1;

    status_start = strchr(status_start, ' ');
    if (!status_start) return -1;
    response->status_code = atoi(status_start + 1);

    /* Find Content-Length */
    const char *cl = strstr(data, "Content-Length:");
    if (cl) {
        response->content_length = atoi(cl + 15);
    }

    /* Find body (after \r\n\r\n) */
    const char *body_start = strstr(data, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        response->body_length = len - (body_start - data);
        response->body = (char *)malloc(response->body_length + 1);
        if (response->body) {
            memcpy(response->body, body_start, response->body_length);
            response->body[response->body_length] = '\0';
        }
    }

    return 0;
}

/*
 * Perform HTTP request
 */
static int http_request(const char *method, const char *url, const char *auth_token,
                        const char *body, char **response, size_t *response_len)
{
    char host[256] = {0};
    char path[512] = {0};
    int port = 80;
    int sock = -1;
    int result = -1;

    /* Parse URL */
    if (parse_url(url, host, &port, path) != 0) {
        LOG_ERROR("Invalid URL: %s", url);
        return -1;
    }

    LOG("HTTP %s %s:%d%s", method, host, port, path);

    /* Resolve hostname */
    struct hostent *he = gethostbyname(host);
    if (!he) {
        LOG_ERROR("DNS lookup failed for %s", host);
        return -1;
    }

    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("Failed to create socket");
        return -1;
    }

    /* Set timeout */
#ifdef NXDK
    int timeout = HTTP_CONNECT_TIMEOUT;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = HTTP_CONNECT_TIMEOUT / 1000;
    tv.tv_usec = (HTTP_CONNECT_TIMEOUT % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    /* Connect */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Connection failed");
        closesocket(sock);
        return -1;
    }

    /* Create and send request */
    char *request = create_request(method, host, path, auth_token, body);
    if (!request) {
        closesocket(sock);
        return -1;
    }

    size_t request_len = strlen(request);
    if (send(sock, request, request_len, 0) != (int)request_len) {
        LOG_ERROR("Failed to send request");
        free(request);
        closesocket(sock);
        return -1;
    }
    free(request);

    /* Receive response */
    size_t buffer_size = INITIAL_RESPONSE_SIZE;
    char *buffer = (char *)malloc(buffer_size);
    if (!buffer) {
        closesocket(sock);
        return -1;
    }

    size_t total_received = 0;
    char recv_buf[RECV_BUFFER_SIZE];
    int recv_len;

    while ((recv_len = recv(sock, recv_buf, sizeof(recv_buf), 0)) > 0) {
        /* Grow buffer if needed */
        if (total_received + recv_len >= buffer_size) {
            buffer_size *= 2;
            char *new_buffer = (char *)realloc(buffer, buffer_size);
            if (!new_buffer) {
                free(buffer);
                closesocket(sock);
                return -1;
            }
            buffer = new_buffer;
        }

        memcpy(buffer + total_received, recv_buf, recv_len);
        total_received += recv_len;
    }

    closesocket(sock);

    if (total_received == 0) {
        LOG_ERROR("Empty response");
        free(buffer);
        return -1;
    }

    buffer[total_received] = '\0';

    /* Parse response */
    http_response_t resp = {0};
    if (parse_response(buffer, total_received, &resp) != 0) {
        LOG_ERROR("Failed to parse response");
        free(buffer);
        return -1;
    }

    free(buffer);

    /* Check status code */
    if (resp.status_code < 200 || resp.status_code >= 300) {
        LOG_ERROR("HTTP error: %d", resp.status_code);
        if (resp.body) free(resp.body);
        return resp.status_code;
    }

    /* Return body */
    *response = resp.body;
    *response_len = resp.body_length;

    return 0;
}

/*
 * Public HTTP functions
 */

int http_get(const char *url, char **response, size_t *response_len)
{
    return http_request("GET", url, NULL, NULL, response, response_len);
}

int http_post(const char *url, const char *body, char **response, size_t *response_len)
{
    return http_request("POST", url, NULL, body, response, response_len);
}

int http_get_with_auth(const char *url, const char *token, char **response, size_t *response_len)
{
    return http_request("GET", url, token, NULL, response, response_len);
}
