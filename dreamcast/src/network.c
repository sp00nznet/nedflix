/*
 * Nedflix for Sega Dreamcast
 * Network client using KallistiOS lwIP stack
 *
 * The Dreamcast has a 10/100 BBA (Broadband Adapter) or 56k modem.
 * This implementation uses the BBA for network streaming.
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <kos/net.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Network state */
static struct {
    bool initialized;
    bool connected;
    uint32 local_ip;
    char server_host[128];
    uint16 server_port;
} g_net;

/* HTTP response parsing state */
typedef struct {
    int status_code;
    size_t content_length;
    bool chunked;
    char *body;
    size_t body_len;
} http_response_t;

/*
 * Initialize network subsystem
 */
int network_init(void)
{
    LOG("Initializing network...");

    memset(&g_net, 0, sizeof(g_net));

    /* Initialize KOS network */
    if (net_init() < 0) {
        LOG_ERROR("Failed to initialize network stack");
        return -1;
    }

    /* Try to get IP via DHCP */
    LOG("Requesting IP address via DHCP...");

    /* Wait for network to come up (up to 10 seconds) */
    int timeout = 100;  /* 100 x 100ms = 10 seconds */
    while (timeout > 0) {
        g_net.local_ip = net_default_dev->ip_addr[0] |
                        (net_default_dev->ip_addr[1] << 8) |
                        (net_default_dev->ip_addr[2] << 16) |
                        (net_default_dev->ip_addr[3] << 24);

        if (g_net.local_ip != 0) {
            break;
        }

        thd_sleep(100);
        timeout--;
    }

    if (g_net.local_ip == 0) {
        LOG_ERROR("Failed to obtain IP address");
        return -1;
    }

    LOG("Network initialized, IP: %d.%d.%d.%d",
        net_default_dev->ip_addr[0],
        net_default_dev->ip_addr[1],
        net_default_dev->ip_addr[2],
        net_default_dev->ip_addr[3]);

    g_net.initialized = true;
    return 0;
}

/*
 * Shutdown network subsystem
 */
void network_shutdown(void)
{
    if (!g_net.initialized) return;

    net_shutdown();
    g_net.initialized = false;
    LOG("Network shutdown");
}

/*
 * Parse URL into host, port, and path components
 */
static int parse_url(const char *url, char *host, size_t host_len,
                     uint16 *port, char *path, size_t path_len)
{
    /* Skip http:// prefix */
    const char *p = url;
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    }

    /* Find host end (: or / or end) */
    const char *host_end = p;
    while (*host_end && *host_end != ':' && *host_end != '/') {
        host_end++;
    }

    /* Copy host */
    size_t hlen = host_end - p;
    if (hlen >= host_len) hlen = host_len - 1;
    strncpy(host, p, hlen);
    host[hlen] = '\0';

    /* Parse port */
    *port = 80;  /* Default HTTP port */
    if (*host_end == ':') {
        *port = (uint16)atoi(host_end + 1);
        /* Skip to path */
        while (*host_end && *host_end != '/') host_end++;
    }

    /* Copy path */
    if (*host_end == '/') {
        strncpy(path, host_end, path_len - 1);
        path[path_len - 1] = '\0';
    } else {
        strcpy(path, "/");
    }

    return 0;
}

/*
 * Resolve hostname to IP address
 */
static uint32 resolve_host(const char *hostname)
{
    /* Check if it's already an IP address */
    struct in_addr addr;
    if (inet_aton(hostname, &addr)) {
        return addr.s_addr;
    }

    /* DNS lookup */
    struct hostent *he = gethostbyname(hostname);
    if (he && he->h_addr_list[0]) {
        return *(uint32 *)he->h_addr_list[0];
    }

    LOG_ERROR("Failed to resolve: %s", hostname);
    return 0;
}

/*
 * Connect to server
 */
static int connect_to_server(const char *host, uint16 port)
{
    /* Resolve hostname */
    uint32 ip = resolve_host(host);
    if (ip == 0) {
        return -1;
    }

    /* Create socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("Failed to create socket");
        return -1;
    }

    /* Connect */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = ip;

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Failed to connect to %s:%d", host, port);
        close(sock);
        return -1;
    }

    return sock;
}

/*
 * Send HTTP request
 */
static int send_request(int sock, const char *method, const char *host,
                        const char *path, const char *auth_token,
                        const char *body)
{
    char request[1024];
    int len;

    if (body) {
        len = snprintf(request, sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "%s%s%s"
            "\r\n"
            "%s",
            method, path, host,
            (int)strlen(body),
            auth_token ? "Authorization: Bearer " : "",
            auth_token ? auth_token : "",
            auth_token ? "\r\n" : "",
            body);
    } else {
        len = snprintf(request, sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n"
            "%s%s%s"
            "\r\n",
            method, path, host,
            auth_token ? "Authorization: Bearer " : "",
            auth_token ? auth_token : "",
            auth_token ? "\r\n" : "");
    }

    /* Send request */
    int sent = 0;
    while (sent < len) {
        int n = send(sock, request + sent, len - sent, 0);
        if (n <= 0) {
            LOG_ERROR("Failed to send request");
            return -1;
        }
        sent += n;
    }

    return 0;
}

/*
 * Parse HTTP response headers
 */
static int parse_response_headers(const char *data, size_t len, http_response_t *resp)
{
    /* Find status code */
    const char *p = strstr(data, "HTTP/1.");
    if (!p) return -1;

    p = strchr(p, ' ');
    if (!p) return -1;

    resp->status_code = atoi(p + 1);

    /* Find Content-Length */
    p = strstr(data, "Content-Length:");
    if (!p) p = strstr(data, "content-length:");
    if (p) {
        p += 15;
        while (*p == ' ') p++;
        resp->content_length = atoi(p);
    }

    /* Check for chunked encoding */
    p = strstr(data, "Transfer-Encoding:");
    if (!p) p = strstr(data, "transfer-encoding:");
    if (p && strstr(p, "chunked")) {
        resp->chunked = true;
    }

    return 0;
}

/*
 * Receive HTTP response
 */
static int receive_response(int sock, char **response, size_t *response_len)
{
    /* Allocate receive buffer */
    size_t buf_size = 8192;
    char *buffer = (char *)malloc(buf_size);
    if (!buffer) {
        LOG_ERROR("Failed to allocate receive buffer");
        return -1;
    }

    size_t total = 0;
    http_response_t resp = {0};
    bool headers_done = false;
    const char *body_start = NULL;

    while (1) {
        /* Grow buffer if needed */
        if (total >= buf_size - 1) {
            size_t new_size = buf_size * 2;
            /* Limit buffer size for Dreamcast memory constraints */
            if (new_size > 256 * 1024) {
                LOG_ERROR("Response too large");
                break;
            }
            char *new_buf = (char *)realloc(buffer, new_size);
            if (!new_buf) {
                break;
            }
            buffer = new_buf;
            buf_size = new_size;
        }

        /* Receive data */
        int n = recv(sock, buffer + total, buf_size - total - 1, 0);
        if (n <= 0) {
            break;  /* Connection closed or error */
        }
        total += n;
        buffer[total] = '\0';

        /* Parse headers if not done yet */
        if (!headers_done) {
            body_start = strstr(buffer, "\r\n\r\n");
            if (body_start) {
                body_start += 4;
                parse_response_headers(buffer, body_start - buffer, &resp);
                headers_done = true;

                /* If we know content length, check if we're done */
                if (resp.content_length > 0) {
                    size_t body_received = total - (body_start - buffer);
                    if (body_received >= resp.content_length) {
                        break;
                    }
                }
            }
        } else if (resp.content_length > 0) {
            /* Check if we've received all content */
            size_t body_received = total - (body_start - buffer);
            if (body_received >= resp.content_length) {
                break;
            }
        }
    }

    /* Extract body */
    if (body_start && total > (size_t)(body_start - buffer)) {
        size_t body_len = total - (body_start - buffer);
        *response = (char *)malloc(body_len + 1);
        if (*response) {
            memcpy(*response, body_start, body_len);
            (*response)[body_len] = '\0';
            *response_len = body_len;
        }
    } else {
        *response = NULL;
        *response_len = 0;
    }

    int status = resp.status_code;
    free(buffer);

    return status;
}

/*
 * HTTP GET request
 */
int http_get(const char *url, char **response, size_t *response_len)
{
    if (!g_net.initialized) {
        LOG_ERROR("Network not initialized");
        return -1;
    }

    char host[128];
    char path[512];
    uint16 port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) < 0) {
        return -1;
    }

    int sock = connect_to_server(host, port);
    if (sock < 0) {
        return -1;
    }

    if (send_request(sock, "GET", host, path, NULL, NULL) < 0) {
        close(sock);
        return -1;
    }

    int status = receive_response(sock, response, response_len);
    close(sock);

    return (status >= 200 && status < 300) ? 0 : status;
}

/*
 * HTTP GET with authentication
 */
int http_get_with_auth(const char *url, const char *token,
                       char **response, size_t *response_len)
{
    if (!g_net.initialized) return -1;

    char host[128];
    char path[512];
    uint16 port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) < 0) {
        return -1;
    }

    int sock = connect_to_server(host, port);
    if (sock < 0) {
        return -1;
    }

    if (send_request(sock, "GET", host, path, token, NULL) < 0) {
        close(sock);
        return -1;
    }

    int status = receive_response(sock, response, response_len);
    close(sock);

    return (status >= 200 && status < 300) ? 0 : status;
}

/*
 * HTTP POST request
 */
int http_post(const char *url, const char *body,
              char **response, size_t *response_len)
{
    if (!g_net.initialized) return -1;

    char host[128];
    char path[512];
    uint16 port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) < 0) {
        return -1;
    }

    int sock = connect_to_server(host, port);
    if (sock < 0) {
        return -1;
    }

    if (send_request(sock, "POST", host, path, NULL, body) < 0) {
        close(sock);
        return -1;
    }

    int status = receive_response(sock, response, response_len);
    close(sock);

    return (status >= 200 && status < 300) ? 0 : status;
}

/*
 * HTTP POST with authentication
 */
int http_post_with_auth(const char *url, const char *token, const char *body,
                        char **response, size_t *response_len)
{
    if (!g_net.initialized) return -1;

    char host[128];
    char path[512];
    uint16 port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) < 0) {
        return -1;
    }

    int sock = connect_to_server(host, port);
    if (sock < 0) {
        return -1;
    }

    if (send_request(sock, "POST", host, path, token, body) < 0) {
        close(sock);
        return -1;
    }

    int status = receive_response(sock, response, response_len);
    close(sock);

    return (status >= 200 && status < 300) ? 0 : status;
}

/*
 * Check if network is available
 */
bool network_is_available(void)
{
    return g_net.initialized && g_net.local_ip != 0;
}
