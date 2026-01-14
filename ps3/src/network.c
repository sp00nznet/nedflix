/*
 * Nedflix PS3 - Network using BSD sockets
 */

#include "nedflix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <net/net.h>
#include <net/netctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Initialize network */
int network_init(void)
{
    int ret;

    printf("Initializing network...\n");

    /* Initialize network modules */
    ret = netInitialize();
    if (ret < 0) {
        printf("netInitialize failed: %d\n", ret);
        return -1;
    }

    ret = netCtlInit();
    if (ret < 0) {
        printf("netCtlInit failed: %d\n", ret);
        return -1;
    }

    /* Get connection state */
    s32 state;
    ret = netCtlGetState(&state);
    if (ret < 0 || state != NET_CTL_STATE_IPObtained) {
        printf("Network not connected (state=%d)\n", state);
        return -1;
    }

    /* Get IP address */
    netCtlInfo info;
    ret = netCtlGetInfo(NET_CTL_INFO_IP_ADDRESS, &info);
    if (ret == 0) {
        strncpy(g_app.net.local_ip, info.ip_address, sizeof(g_app.net.local_ip) - 1);
        printf("IP Address: %s\n", g_app.net.local_ip);
    }

    g_app.net.initialized = true;
    g_app.net.connected = true;

    return 0;
}

/* Shutdown network */
void network_shutdown(void)
{
    printf("Shutting down network...\n");
    netCtlTerm();
    netDeinitialize();
    g_app.net.initialized = false;
    g_app.net.connected = false;
}

/* Parse URL into host, port, path */
static int parse_url(const char *url, char *host, int host_len, int *port, char *path, int path_len)
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
    const char *host_end = p;
    while (*host_end && *host_end != ':' && *host_end != '/') {
        host_end++;
    }

    int len = host_end - p;
    if (len >= host_len) len = host_len - 1;
    strncpy(host, p, len);
    host[len] = '\0';

    p = host_end;

    /* Check for port */
    if (*p == ':') {
        p++;
        *port = atoi(p);
        while (*p && *p != '/') p++;
    }

    /* Path */
    if (*p == '/') {
        strncpy(path, p, path_len - 1);
        path[path_len - 1] = '\0';
    } else {
        strcpy(path, "/");
    }

    return 0;
}

/* HTTP GET request */
int http_get(const char *url, char **response, size_t *len)
{
    char host[256];
    char path[512];
    int port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != 0) {
        printf("Invalid URL: %s\n", url);
        return -1;
    }

    printf("HTTP GET %s:%d%s\n", host, port, path);

    /* Create socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("socket() failed\n");
        return -1;
    }

    /* Set timeout */
    struct timeval tv;
    tv.tv_sec = HTTP_TIMEOUT_MS / 1000;
    tv.tv_usec = (HTTP_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    /* Resolve hostname - PS3 has limited DNS support */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    /* Try direct IP first */
    if (inet_aton(host, &addr.sin_addr) == 0) {
        /* Need DNS lookup - simplified for demo */
        printf("DNS lookup not fully implemented in demo\n");
        close(sock);
        return -1;
    }

    /* Connect */
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("connect() failed\n");
        close(sock);
        return -1;
    }

    /* Send request */
    char request[1024];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Nedflix-PS3/1.0\r\n"
             "Accept: */*\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host);

    if (send(sock, request, strlen(request), 0) < 0) {
        printf("send() failed\n");
        close(sock);
        return -1;
    }

    /* Receive response */
    size_t buf_size = RECV_BUFFER_SIZE;
    char *buf = malloc(buf_size);
    if (!buf) {
        close(sock);
        return -1;
    }

    size_t total = 0;
    ssize_t n;
    while ((n = recv(sock, buf + total, buf_size - total - 1, 0)) > 0) {
        total += n;
        if (total >= buf_size - 1) break;
    }
    buf[total] = '\0';

    close(sock);

    /* Skip HTTP headers */
    char *body = strstr(buf, "\r\n\r\n");
    if (body) {
        body += 4;
        size_t body_len = total - (body - buf);
        *response = malloc(body_len + 1);
        if (*response) {
            memcpy(*response, body, body_len);
            (*response)[body_len] = '\0';
            *len = body_len;
        }
    } else {
        *response = buf;
        *len = total;
        buf = NULL;
    }

    free(buf);
    return 0;
}

/* HTTP POST request */
int http_post(const char *url, const char *body, char **response, size_t *len)
{
    char host[256];
    char path[512];
    int port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != 0) {
        return -1;
    }

    printf("HTTP POST %s:%d%s\n", host, port, path);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct timeval tv;
    tv.tv_sec = HTTP_TIMEOUT_MS / 1000;
    tv.tv_usec = (HTTP_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_aton(host, &addr.sin_addr) == 0) {
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    size_t body_len = body ? strlen(body) : 0;
    char request[2048];
    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Nedflix-PS3/1.0\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host, body_len);

    send(sock, request, strlen(request), 0);
    if (body && body_len > 0) {
        send(sock, body, body_len, 0);
    }

    /* Receive response */
    size_t buf_size = RECV_BUFFER_SIZE;
    char *buf = malloc(buf_size);
    if (!buf) {
        close(sock);
        return -1;
    }

    size_t total = 0;
    ssize_t n;
    while ((n = recv(sock, buf + total, buf_size - total - 1, 0)) > 0) {
        total += n;
        if (total >= buf_size - 1) break;
    }
    buf[total] = '\0';

    close(sock);

    char *resp_body = strstr(buf, "\r\n\r\n");
    if (resp_body) {
        resp_body += 4;
        size_t resp_len = total - (resp_body - buf);
        *response = malloc(resp_len + 1);
        if (*response) {
            memcpy(*response, resp_body, resp_len);
            (*response)[resp_len] = '\0';
            *len = resp_len;
        }
    }

    free(buf);
    return 0;
}

/* Async download (simplified stub) */
int http_download_async(const char *url, const char *path, void (*callback)(int))
{
    (void)url;
    (void)path;
    if (callback) callback(-1);
    printf("Async download not implemented in demo\n");
    return -1;
}
