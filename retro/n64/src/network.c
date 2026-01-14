/*
 * Nedflix N64 - Network (stub)
 *
 * N64 had very limited network options:
 * - 64DD Randnet modem (Japan only)
 * - Homebrew solutions via USB/Serial
 * - Modern: 64drive or EverDrive USB
 */

#include "nedflix.h"
#include <string.h>

static bool net_initialized = false;

int network_init(void)
{
    LOG("Network init");

    /*
     * For real N64 homebrew networking:
     * - Use libdragon's usb.h for USB communication
     * - Or implement serial protocol via controller port
     * - Or use flashcart with USB (64drive, ED64)
     */

    net_initialized = true;
    return 0;
}

void network_shutdown(void)
{
    net_initialized = false;
}

bool network_is_available(void)
{
    return net_initialized;
}

int http_get(const char *url, char **response, size_t *len)
{
    (void)url;
    (void)response;
    (void)len;

    /*
     * HTTP would need to be tunneled through:
     * - USB serial connection to host PC
     * - Host PC acts as network proxy
     * - Custom protocol over serial
     */

    return -1;
}

int http_post(const char *url, const char *body, char **response, size_t *len)
{
    (void)url;
    (void)body;
    (void)response;
    (void)len;
    return -1;
}
