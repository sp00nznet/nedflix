/*
 * Nedflix for Xbox 360
 * API client for Nedflix server
 *
 * TECHNICAL DEMO / NOVELTY PORT
 */

#include "nedflix.h"

/* API state */
static char g_server_url[MAX_URL_LENGTH];
static bool g_api_initialized = false;

/*
 * Initialize API client
 */
int api_init(const char *server_url)
{
    if (!server_url || strlen(server_url) == 0) {
        return -1;
    }

    strncpy(g_server_url, server_url, sizeof(g_server_url) - 1);
    g_server_url[sizeof(g_server_url) - 1] = '\0';

    /* Test connection */
    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/health", g_server_url);

    char *response = NULL;
    size_t len = 0;

    if (http_get(url, &response, &len) == 0) {
        free(response);
        g_api_initialized = true;
        LOG("API initialized: %s", g_server_url);
        return 0;
    }

    LOG_ERROR("Failed to connect to API server");
    return -1;
}

/*
 * Shutdown API client
 */
void api_shutdown(void)
{
    g_api_initialized = false;
    g_server_url[0] = '\0';
}

/*
 * Login and get auth token
 */
int api_login(const char *username, const char *password,
              char *token_out, size_t token_len)
{
    if (!g_api_initialized || !username || !password || !token_out) {
        return -1;
    }

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/auth/login", g_server_url);

    char body[512];
    snprintf(body, sizeof(body),
             "{\"username\":\"%s\",\"password\":\"%s\"}",
             username, password);

    char *response = NULL;
    size_t len = 0;

    if (http_post(url, body, &response, &len) != 0) {
        return -1;
    }

    /* Parse token from response */
    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    const char *token = json_get_string(json, "token");
    if (token) {
        strncpy(token_out, token, token_len - 1);
        token_out[token_len - 1] = '\0';
        json_free(json);
        return 0;
    }

    json_free(json);
    return -1;
}

/*
 * Get user info
 */
int api_get_user_info(const char *token, char *username_out, size_t len)
{
    if (!g_api_initialized || !token || !username_out) {
        return -1;
    }

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/auth/me", g_server_url);

    char *response = NULL;
    size_t resp_len = 0;

    if (http_get_with_auth(url, token, &response, &resp_len) != 0) {
        return -1;
    }

    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    const char *name = json_get_string(json, "username");
    if (name) {
        strncpy(username_out, name, len - 1);
        username_out[len - 1] = '\0';
        json_free(json);
        return 0;
    }

    json_free(json);
    return -1;
}

/*
 * Browse media
 */
int api_browse(const char *token, const char *path,
               library_t library, media_list_t *list)
{
    if (!g_api_initialized || !token || !path || !list) {
        return -1;
    }

    const char *lib_names[] = {"music", "audiobooks", "movies", "tvshows"};

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/browse/%s?path=%s",
             g_server_url, lib_names[library], path);

    char *response = NULL;
    size_t resp_len = 0;

    if (http_get_with_auth(url, token, &response, &resp_len) != 0) {
        return -1;
    }

    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    json_value_t *items = json_get_array(json, "items");
    if (!items) {
        json_free(json);
        return -1;
    }

    list->count = 0;
    int item_count = json_array_length(items);

    for (int i = 0; i < item_count && list->count < list->capacity; i++) {
        json_value_t *item = json_array_get(items, i);
        if (!item) continue;

        media_item_t *media = &list->items[list->count];

        const char *name = json_get_string(item, "name");
        const char *item_path = json_get_string(item, "path");
        const char *type = json_get_string(item, "type");

        if (name && item_path) {
            strncpy(media->name, name, sizeof(media->name) - 1);
            strncpy(media->path, item_path, sizeof(media->path) - 1);

            media->is_directory = json_get_bool(item, "isDirectory", false);

            if (type) {
                if (strcmp(type, "audio") == 0) {
                    media->type = MEDIA_TYPE_AUDIO;
                } else if (strcmp(type, "video") == 0) {
                    media->type = MEDIA_TYPE_VIDEO;
                } else if (strcmp(type, "directory") == 0) {
                    media->type = MEDIA_TYPE_DIRECTORY;
                } else {
                    media->type = MEDIA_TYPE_UNKNOWN;
                }
            }

            media->duration = json_get_int(item, "duration", 0);
            media->size = json_get_int(item, "size", 0);

            list->count++;
        }
    }

    json_free(json);
    return 0;
}

/*
 * Get streaming URL for media
 */
int api_get_stream_url(const char *token, const char *path,
                       char *url_out, size_t len)
{
    if (!g_api_initialized || !token || !path || !url_out) {
        return -1;
    }

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/stream?path=%s", g_server_url, path);

    char *response = NULL;
    size_t resp_len = 0;

    if (http_get_with_auth(url, token, &response, &resp_len) != 0) {
        return -1;
    }

    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    const char *stream_url = json_get_string(json, "url");
    if (stream_url) {
        strncpy(url_out, stream_url, len - 1);
        url_out[len - 1] = '\0';
        json_free(json);
        return 0;
    }

    json_free(json);
    return -1;
}
