/*
 * Nedflix PS3 - API client
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char api_base_url[MAX_URL_LENGTH];
static bool api_initialized = false;

/* Initialize API with server URL */
int api_init(const char *server)
{
    if (!server || strlen(server) == 0) {
        printf("API: No server URL\n");
        return -1;
    }

    strncpy(api_base_url, server, MAX_URL_LENGTH - 1);
    api_base_url[MAX_URL_LENGTH - 1] = '\0';

    /* Remove trailing slash */
    size_t len = strlen(api_base_url);
    if (len > 0 && api_base_url[len - 1] == '/') {
        api_base_url[len - 1] = '\0';
    }

    printf("API: Initialized with server %s\n", api_base_url);

    /* Test connection */
    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/health", api_base_url);

    char *response = NULL;
    size_t resp_len = 0;

    if (http_get(url, &response, &resp_len) == 0) {
        printf("API: Server connection OK\n");
        free(response);
        api_initialized = true;
        return 0;
    }

    printf("API: Server connection failed\n");
    return -1;
}

/* Shutdown API */
void api_shutdown(void)
{
    api_initialized = false;
    api_base_url[0] = '\0';
}

/* Login to server */
int api_login(const char *user, const char *pass, char *token, size_t len)
{
    if (!api_initialized) return -1;

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/auth/login", api_base_url);

    char body[512];
    snprintf(body, sizeof(body),
             "{\"username\":\"%s\",\"password\":\"%s\"}",
             user, pass);

    char *response = NULL;
    size_t resp_len = 0;

    if (http_post(url, body, &response, &resp_len) == 0 && response) {
        json_value_t *json = json_parse(response);
        if (json) {
            const char *tok = json_get_string(json, "token");
            if (tok) {
                strncpy(token, tok, len - 1);
                token[len - 1] = '\0';
                json_free(json);
                free(response);
                return 0;
            }
            json_free(json);
        }
        free(response);
    }

    return -1;
}

/* Logout */
int api_logout(const char *token)
{
    if (!api_initialized || !token) return -1;

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/auth/logout?token=%s", api_base_url, token);

    char *response = NULL;
    size_t resp_len = 0;

    http_post(url, NULL, &response, &resp_len);
    free(response);

    return 0;
}

/* Browse media directory */
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list)
{
    if (!api_initialized || !list) return -1;

    const char *lib_names[] = { "music", "audiobooks", "movies", "tvshows" };

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/browse/%s?path=%s&token=%s",
             api_base_url, lib_names[lib], path, token ? token : "");

    char *response = NULL;
    size_t resp_len = 0;

    if (http_get(url, &response, &resp_len) != 0) {
        return -1;
    }

    /* Parse JSON response */
    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    json_value_t *items = json_get_array(json, "items");
    if (!items) {
        json_free(json);
        return -1;
    }

    int count = json_array_length(items);
    if (count > MAX_MEDIA_ITEMS) count = MAX_MEDIA_ITEMS;

    /* Allocate/reallocate items array */
    if (!list->items) {
        list->items = calloc(MAX_MEDIA_ITEMS, sizeof(media_item_t));
        list->capacity = MAX_MEDIA_ITEMS;
    }

    list->count = 0;

    for (int i = 0; i < count; i++) {
        json_value_t *item = json_array_get(items, i);
        if (!item) continue;

        media_item_t *m = &list->items[list->count];
        memset(m, 0, sizeof(media_item_t));

        const char *name = json_get_string(item, "name");
        const char *item_path = json_get_string(item, "path");
        const char *type = json_get_string(item, "type");

        if (name) strncpy(m->name, name, MAX_TITLE_LENGTH - 1);
        if (item_path) strncpy(m->path, item_path, MAX_PATH_LENGTH - 1);

        m->is_directory = json_get_bool(item, "isDirectory", false);
        m->duration = json_get_int(item, "duration", 0);
        m->size = json_get_int(item, "size", 0);

        if (type) {
            if (strcmp(type, "audio") == 0) m->type = MEDIA_TYPE_AUDIO;
            else if (strcmp(type, "video") == 0) m->type = MEDIA_TYPE_VIDEO;
            else if (strcmp(type, "directory") == 0) m->type = MEDIA_TYPE_DIRECTORY;
        }

        list->count++;
    }

    json_free(json);
    return 0;
}

/* Search media */
int api_search(const char *token, const char *query, media_list_t *list)
{
    if (!api_initialized || !list) return -1;

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/search?q=%s&token=%s",
             api_base_url, query, token ? token : "");

    char *response = NULL;
    size_t resp_len = 0;

    if (http_get(url, &response, &resp_len) != 0) {
        return -1;
    }

    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    /* Similar parsing as browse */
    json_free(json);
    return 0;
}

/* Get stream URL for media item */
int api_get_stream_url(const char *token, const char *path, int quality, char *url, size_t len)
{
    if (!api_initialized) return -1;

    const char *quality_names[] = { "sd", "hd", "fhd" };
    if (quality < 0 || quality > 2) quality = 1;

    snprintf(url, len, "%s/api/stream?path=%s&quality=%s&token=%s",
             api_base_url, path, quality_names[quality], token ? token : "");

    return 0;
}

/* Get subtitles for media */
int api_get_subtitles(const char *token, const char *path, const char *lang, char **srt)
{
    if (!api_initialized) return -1;

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/subtitles?path=%s&lang=%s&token=%s",
             api_base_url, path, lang, token ? token : "");

    size_t len = 0;
    return http_get(url, srt, &len);
}

/* Get detailed media info */
int api_get_media_info(const char *token, const char *path, media_item_t *item)
{
    if (!api_initialized || !item) return -1;

    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/api/info?path=%s&token=%s",
             api_base_url, path, token ? token : "");

    char *response = NULL;
    size_t resp_len = 0;

    if (http_get(url, &response, &resp_len) != 0) {
        return -1;
    }

    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    const char *name = json_get_string(json, "name");
    const char *desc = json_get_string(json, "description");

    if (name) strncpy(item->name, name, MAX_TITLE_LENGTH - 1);
    if (desc) strncpy(item->description, desc, sizeof(item->description) - 1);

    item->duration = json_get_int(json, "duration", 0);
    item->size = json_get_int(json, "size", 0);
    item->year = json_get_int(json, "year", 0);
    item->rating = (float)json_get_double(json, "rating", 0.0);

    json_free(json);
    return 0;
}
