/*
 * Nedflix for Xbox 360
 * Network stack using lwIP via libxenon
 *
 * TECHNICAL DEMO / NOVELTY PORT
 */

#include "nedflix.h"
#include <network/network.h>
#include <lwip/sockets.h>

/* Network state */
static bool g_net_initialized = false;

/*
 * Initialize network
 */
int network_init(void)
{
    LOG("Initializing network...");

    /* Initialize lwIP network stack */
    network_init_sys();

    /* Wait for DHCP */
    printf("Waiting for network (DHCP)...\n");

    int timeout = 100;  /* 10 seconds */
    while (timeout > 0) {
        network_poll();

        if (network_is_ready()) {
            struct ip_addr ip, netmask, gw;
            network_getip(&ip, &netmask, &gw);

            g_app.net.ip.addr = ip.addr;
            g_app.net.netmask.addr = netmask.addr;
            g_app.net.gateway.addr = gw.addr;
            g_app.net.ip_addr = ntohl(ip.addr);

            printf("Network ready: %d.%d.%d.%d\n",
                   ip4_addr1(&ip), ip4_addr2(&ip),
                   ip4_addr3(&ip), ip4_addr4(&ip));

            g_net_initialized = true;
            g_app.net.initialized = true;
            return 0;
        }

        mdelay(100);
        timeout--;
    }

    LOG_ERROR("Network initialization timed out");
    return -1;
}

/*
 * Shutdown network
 */
void network_shutdown(void)
{
    g_net_initialized = false;
    g_app.net.initialized = false;
}

/*
 * Check if network connected
 */
bool network_is_connected(void)
{
    return g_net_initialized && network_is_ready();
}

/*
 * Parse URL into host, port, path
 */
static int parse_url(const char *url, char *host, int host_len,
                     int *port, char *path, int path_len)
{
    const char *p = url;

    /* Skip protocol */
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;  /* Note: HTTPS not actually supported */
    }

    /* Extract host */
    const char *host_end = strchr(p, ':');
    const char *path_start = strchr(p, '/');

    if (host_end && (!path_start || host_end < path_start)) {
        int len = MIN(host_end - p, host_len - 1);
        strncpy(host, p, len);
        host[len] = '\0';
        *port = atoi(host_end + 1);
    } else if (path_start) {
        int len = MIN(path_start - p, host_len - 1);
        strncpy(host, p, len);
        host[len] = '\0';
        *port = 80;
    } else {
        strncpy(host, p, host_len - 1);
        host[host_len - 1] = '\0';
        *port = 80;
    }

    /* Extract path */
    if (path_start) {
        strncpy(path, path_start, path_len - 1);
        path[path_len - 1] = '\0';
    } else {
        strcpy(path, "/");
    }

    return 0;
}

/*
 * HTTP GET request
 */
int http_get(const char *url, char **response, size_t *len)
{
    return http_get_with_auth(url, NULL, response, len);
}

/*
 * HTTP GET with authentication
 */
int http_get_with_auth(const char *url, const char *token,
                       char **response, size_t *len)
{
    if (!url || !response || !len) return -1;

    char host[256], path[512];
    int port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != 0) {
        return -1;
    }

    /* Resolve hostname */
    struct hostent *he = gethostbyname(host);
    if (!he) {
        LOG_ERROR("Failed to resolve host: %s", host);
        return -1;
    }

    /* Create socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("Failed to create socket");
        return -1;
    }

    /* Connect */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to connect to %s:%d", host, port);
        close(sock);
        return -1;
    }

    /* Build request */
    char request[1024];
    int req_len;

    if (token) {
        req_len = snprintf(request, sizeof(request),
                           "GET %s HTTP/1.0\r\n"
                           "Host: %s\r\n"
                           "Authorization: Bearer %s\r\n"
                           "Connection: close\r\n\r\n",
                           path, host, token);
    } else {
        req_len = snprintf(request, sizeof(request),
                           "GET %s HTTP/1.0\r\n"
                           "Host: %s\r\n"
                           "Connection: close\r\n\r\n",
                           path, host);
    }

    /* Send request */
    if (send(sock, request, req_len, 0) != req_len) {
        LOG_ERROR("Failed to send request");
        close(sock);
        return -1;
    }

    /* Receive response */
    size_t buf_size = RECV_BUFFER_SIZE;
    char *buf = (char *)malloc(buf_size);
    if (!buf) {
        close(sock);
        return -1;
    }

    size_t total = 0;
    ssize_t n;
    while ((n = recv(sock, buf + total, buf_size - total - 1, 0)) > 0) {
        total += n;
        if (total >= buf_size - 1) {
            buf_size *= 2;
            char *new_buf = realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                close(sock);
                return -1;
            }
            buf = new_buf;
        }
    }
    buf[total] = '\0';

    close(sock);

    /* Skip HTTP headers */
    char *body = strstr(buf, "\r\n\r\n");
    if (body) {
        body += 4;
        size_t body_len = total - (body - buf);
        char *result = (char *)malloc(body_len + 1);
        if (result) {
            memcpy(result, body, body_len);
            result[body_len] = '\0';
            *response = result;
            *len = body_len;
        }
    }

    free(buf);
    return (*response) ? 0 : -1;
}

/*
 * HTTP POST request
 */
int http_post(const char *url, const char *body, char **response, size_t *len)
{
    if (!url || !response || !len) return -1;

    char host[256], path[512];
    int port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != 0) {
        return -1;
    }

    struct hostent *he = gethostbyname(host);
    if (!he) return -1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    size_t body_len = body ? strlen(body) : 0;

    char request[2048];
    int req_len = snprintf(request, sizeof(request),
                           "POST %s HTTP/1.0\r\n"
                           "Host: %s\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: %zu\r\n"
                           "Connection: close\r\n\r\n%s",
                           path, host, body_len, body ? body : "");

    if (send(sock, request, req_len, 0) != req_len) {
        close(sock);
        return -1;
    }

    /* Receive response */
    size_t buf_size = RECV_BUFFER_SIZE;
    char *buf = (char *)malloc(buf_size);
    if (!buf) {
        close(sock);
        return -1;
    }

    size_t total = 0;
    ssize_t n;
    while ((n = recv(sock, buf + total, buf_size - total - 1, 0)) > 0) {
        total += n;
        if (total >= buf_size - 1) {
            buf_size *= 2;
            buf = realloc(buf, buf_size);
        }
    }
    buf[total] = '\0';

    close(sock);

    char *resp_body = strstr(buf, "\r\n\r\n");
    if (resp_body) {
        resp_body += 4;
        size_t resp_len = total - (resp_body - buf);
        *response = strdup(resp_body);
        *len = resp_len;
    }

    free(buf);
    return (*response) ? 0 : -1;
}
