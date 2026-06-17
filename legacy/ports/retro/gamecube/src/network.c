/*
 * Nedflix for Nintendo GameCube
 * Network support via Broadband Adapter (BBA)
 *
 * Note: The BBA is a rare accessory. Most GameCube users won't have one.
 * This provides basic TCP/IP networking when available.
 */

#include "nedflix.h"
#include <network.h>
#include <errno.h>

/* Network configuration */
#define NET_TIMEOUT_MS    10000
#define NET_BUFFER_SIZE   32768
#define HTTP_PORT         80

/* Network state */
static struct {
    bool initialized;
    bool connected;
    bool bba_present;
    int socket;
    char ip_address[16];
    char server_host[128];
    int server_port;
    char server_path[256];
} net_state;

/* HTTP stream state */
static struct {
    bool active;
    int socket;
    size_t content_length;
    size_t bytes_received;
    bool chunked;
} http_state;

/* Initialize network subsystem */
int network_init(void)
{
    LOG("Initializing network...");

    memset(&net_state, 0, sizeof(net_state));
    memset(&http_state, 0, sizeof(http_state));
    net_state.socket = -1;
    http_state.socket = -1;

    /* Initialize network library */
    char local_ip[16] = {0};
    char gateway[16] = {0};
    char netmask[16] = {0};

    s32 ret = if_config(local_ip, netmask, gateway, true, 20);

    if (ret < 0) {
        LOG("Network: BBA not detected or DHCP failed (%d)", ret);
        net_state.bba_present = false;
        g_app.network.bba_present = false;
        return -1;
    }

    net_state.bba_present = true;
    net_state.initialized = true;
    strncpy(net_state.ip_address, local_ip, sizeof(net_state.ip_address) - 1);

    /* Update global state */
    g_app.network.initialized = true;
    g_app.network.bba_present = true;
    strncpy(g_app.network.ip_address, local_ip, sizeof(g_app.network.ip_address) - 1);

    LOG("Network: Initialized with IP %s", local_ip);
    return 0;
}

/* Shutdown network */
void network_shutdown(void)
{
    if (!net_state.initialized) return;

    LOG("Shutting down network...");

    http_stream_stop();

    if (net_state.socket >= 0) {
        net_close(net_state.socket);
        net_state.socket = -1;
    }

    net_state.initialized = false;
    net_state.connected = false;
    g_app.network.initialized = false;
    g_app.network.connected = false;

    LOG("Network shutdown complete");
}

/* Check if network is connected */
bool network_is_connected(void)
{
    return net_state.initialized && net_state.bba_present;
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
        /* HTTPS not supported on GameCube */
        return -1;
    } else {
        return -1;
    }

    /* Extract host */
    const char *host_end = strchr(p, '/');
    const char *port_start = strchr(p, ':');

    if (port_start && (!host_end || port_start < host_end)) {
        size_t host_len = port_start - p;
        if (host_len >= host_size) host_len = host_size - 1;
        strncpy(host, p, host_len);
        host[host_len] = '\0';
        *port = atoi(port_start + 1);
        p = host_end ? host_end : "";
    } else if (host_end) {
        size_t host_len = host_end - p;
        if (host_len >= host_size) host_len = host_size - 1;
        strncpy(host, p, host_len);
        host[host_len] = '\0';
        p = host_end;
    } else {
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

/* Connect to a server */
int network_connect(const char *server_url)
{
    if (!net_state.initialized || !net_state.bba_present) {
        return -1;
    }

    /* Parse URL */
    if (parse_url(server_url, net_state.server_host, sizeof(net_state.server_host),
                  &net_state.server_port, net_state.server_path,
                  sizeof(net_state.server_path)) != 0) {
        LOG_ERROR("Network: Invalid URL");
        return -1;
    }

    /* Resolve hostname */
    struct hostent *host = net_gethostbyname(net_state.server_host);
    if (!host) {
        LOG_ERROR("Network: Cannot resolve %s", net_state.server_host);
        return -1;
    }

    /* Create socket */
    net_state.socket = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (net_state.socket < 0) {
        LOG_ERROR("Network: Socket creation failed");
        return -1;
    }

    /* Connect */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(net_state.server_port);
    memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);

    s32 ret = net_connect(net_state.socket, (struct sockaddr *)&server_addr,
                          sizeof(server_addr));
    if (ret < 0) {
        LOG_ERROR("Network: Connection failed (%d)", ret);
        net_close(net_state.socket);
        net_state.socket = -1;
        return -1;
    }

    net_state.connected = true;
    g_app.network.connected = true;
    strncpy(g_app.network.server_url, server_url, MAX_URL_LENGTH - 1);

    LOG("Network: Connected to %s:%d", net_state.server_host, net_state.server_port);
    return 0;
}

/* Fetch catalog from server */
int network_fetch_catalog(media_list_t *list, library_type_t library)
{
    if (!net_state.connected) {
        return -1;
    }

    /* In a real implementation, this would:
     * 1. Send HTTP request to server API
     * 2. Parse JSON response
     * 3. Populate media list
     *
     * For now, return empty since we don't have a real server
     */

    list->count = 0;
    return 0;
}

/* Search for content */
int network_search(const char *query, media_list_t *results)
{
    if (!net_state.connected || !query) {
        return -1;
    }

    results->count = 0;
    return 0;
}

/* Get streaming URL for an item */
int network_get_stream_url(const char *item_id, char *url, size_t url_size)
{
    if (!net_state.connected || !item_id || !url) {
        return -1;
    }

    /* Would normally query server for stream URL */
    url[0] = '\0';
    return -1;
}

/* Start HTTP stream */
int http_stream_start(const char *url)
{
    if (!net_state.bba_present) {
        LOG_ERROR("HTTP: No network available");
        return -1;
    }

    http_stream_stop();

    LOG("Starting HTTP stream: %s", url);

    char host[128];
    int port;
    char path[256];

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != 0) {
        LOG_ERROR("HTTP: Invalid URL");
        return -1;
    }

    /* Resolve hostname */
    struct hostent *he = net_gethostbyname(host);
    if (!he) {
        LOG_ERROR("HTTP: Cannot resolve %s", host);
        return -1;
    }

    /* Create socket */
    http_state.socket = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (http_state.socket < 0) {
        LOG_ERROR("HTTP: Socket creation failed");
        return -1;
    }

    /* Connect */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (net_connect(http_state.socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("HTTP: Connection failed");
        net_close(http_state.socket);
        http_state.socket = -1;
        return -1;
    }

    /* Send HTTP request */
    char request[512];
    int len = snprintf(request, sizeof(request),
                       "GET %s HTTP/1.1\r\n"
                       "Host: %s\r\n"
                       "User-Agent: Nedflix-GC/1.0\r\n"
                       "Accept: */*\r\n"
                       "Connection: close\r\n"
                       "\r\n",
                       path, host);

    if (net_send(http_state.socket, request, len, 0) < 0) {
        LOG_ERROR("HTTP: Failed to send request");
        net_close(http_state.socket);
        http_state.socket = -1;
        return -1;
    }

    /* Read response headers */
    char header[2048];
    int header_len = 0;
    bool header_complete = false;

    while (!header_complete && header_len < sizeof(header) - 1) {
        int ret = net_recv(http_state.socket, header + header_len, 1, 0);
        if (ret <= 0) break;

        header_len++;
        header[header_len] = '\0';

        if (header_len >= 4 && strstr(header + header_len - 4, "\r\n\r\n")) {
            header_complete = true;
        }
    }

    if (!header_complete) {
        LOG_ERROR("HTTP: Failed to read headers");
        net_close(http_state.socket);
        http_state.socket = -1;
        return -1;
    }

    /* Parse status */
    int status;
    if (sscanf(header, "HTTP/%*s %d", &status) != 1 || status < 200 || status >= 300) {
        LOG_ERROR("HTTP: Bad status %d", status);
        net_close(http_state.socket);
        http_state.socket = -1;
        return -1;
    }

    /* Parse Content-Length */
    char *cl = strstr(header, "Content-Length:");
    if (cl) {
        http_state.content_length = atoi(cl + 15);
    }

    http_state.chunked = (strstr(header, "chunked") != NULL);
    http_state.bytes_received = 0;
    http_state.active = true;

    LOG("HTTP: Connected, content length: %zu", http_state.content_length);
    return 0;
}

/* Read from HTTP stream */
int http_stream_read(void *buffer, size_t size)
{
    if (!http_state.active || http_state.socket < 0) {
        return -1;
    }

    /* Check for end of content */
    if (http_state.content_length > 0 &&
        http_state.bytes_received >= http_state.content_length) {
        return 0;  /* EOF */
    }

    int ret = net_recv(http_state.socket, buffer, size, 0);
    if (ret > 0) {
        http_state.bytes_received += ret;
    } else if (ret == 0) {
        return 0;  /* Connection closed */
    } else if (ret < 0 && ret != -EAGAIN) {
        return -1;  /* Error */
    }

    return ret;
}

/* Stop HTTP stream */
void http_stream_stop(void)
{
    if (http_state.socket >= 0) {
        net_close(http_state.socket);
        http_state.socket = -1;
    }

    http_state.active = false;
    http_state.content_length = 0;
    http_state.bytes_received = 0;

    LOG("HTTP stream stopped");
}

/* Check if HTTP stream is active */
bool http_stream_active(void)
{
    return http_state.active;
}
