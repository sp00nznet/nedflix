/*
 * Nedflix N64 - API client
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>

static char server_base[MAX_URL_LENGTH];

int api_init(const char *server)
{
    strncpy(server_base, server, MAX_URL_LENGTH - 1);
    server_base[MAX_URL_LENGTH - 1] = '\0';

    /* Strip trailing slash */
    size_t len = strlen(server_base);
    if (len > 0 && server_base[len - 1] == '/')
        server_base[len - 1] = '\0';

    return 0;
}

void api_shutdown(void)
{
    server_base[0] = '\0';
}

int api_login(const char *user, const char *pass, char *token, size_t len)
{
    (void)user;
    (void)pass;

    /* Stub - would POST to /api/auth/login */
    if (token && len > 0) token[0] = '\0';
    return -1;
}

int api_browse(const char *token, const char *path, library_t lib, media_list_t *list)
{
    (void)token;
    (void)lib;

    if (!list) return -1;

    list->count = 0;
    list->selected_index = 0;
    list->scroll_offset = 0;
    strncpy(list->current_path, path, MAX_PATH_LENGTH - 1);

    /*
     * Would GET /api/browse?path=...
     * Parse JSON response into list
     */

    /* Demo data */
    strncpy(list->items[0].name, "Demo Album", MAX_TITLE_LENGTH - 1);
    strncpy(list->items[0].path, "/Music/Demo Album", MAX_PATH_LENGTH - 1);
    list->items[0].is_directory = true;
    list->items[0].type = MEDIA_TYPE_DIRECTORY;
    list->count = 1;

    return 0;
}

int api_get_stream_url(const char *token, const char *path, char *url, size_t len)
{
    if (!url || len == 0) return -1;

    /* Build stream URL */
    snprintf(url, len, "%s/api/stream?path=%s&token=%s",
             server_base, path, token ? token : "");

    return 0;
}
