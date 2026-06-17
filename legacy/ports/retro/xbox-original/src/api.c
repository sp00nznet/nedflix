/*
 * Nedflix for Original Xbox
 * API client for Nedflix server communication
 */

#include "nedflix.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Server state */
static struct {
    char base_url[MAX_URL_LENGTH];
    bool initialized;
} g_api;

/*
 * URL encode a string
 */
static void url_encode(const char *src, char *dst, size_t dst_size)
{
    static const char *hex = "0123456789ABCDEF";
    size_t pos = 0;

    while (*src && pos < dst_size - 4) {
        char c = *src++;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[pos++] = c;
        } else if (c == ' ') {
            dst[pos++] = '+';
        } else {
            dst[pos++] = '%';
            dst[pos++] = hex[(c >> 4) & 0xF];
            dst[pos++] = hex[c & 0xF];
        }
    }
    dst[pos] = '\0';
}

/*
 * Build full API URL
 */
static void build_url(char *url, size_t url_size, const char *endpoint, const char *query)
{
    if (query && strlen(query) > 0) {
        snprintf(url, url_size, "%s%s?%s", g_api.base_url, endpoint, query);
    } else {
        snprintf(url, url_size, "%s%s", g_api.base_url, endpoint);
    }
}

/*
 * Initialize API client
 */
int api_init(const char *server_url)
{
    if (!server_url || strlen(server_url) == 0) {
        LOG_ERROR("Invalid server URL");
        return -1;
    }

    LOG("Initializing API client for: %s", server_url);

    strncpy(g_api.base_url, server_url, sizeof(g_api.base_url) - 1);

    /* Remove trailing slash */
    size_t len = strlen(g_api.base_url);
    if (len > 0 && g_api.base_url[len - 1] == '/') {
        g_api.base_url[len - 1] = '\0';
    }

    /* Test connection with a simple request */
    char url[MAX_URL_LENGTH];
    build_url(url, sizeof(url), "/api/user", NULL);

    char *response = NULL;
    size_t response_len = 0;
    int result = http_get(url, &response, &response_len);

    if (result == 401) {
        /* 401 Unauthorized is expected without auth - server is reachable */
        LOG("Server reachable (auth required)");
        g_api.initialized = true;
        if (response) free(response);
        return 0;
    } else if (result == 0) {
        LOG("Server reachable");
        g_api.initialized = true;
        if (response) free(response);
        return 0;
    }

    if (response) free(response);
    LOG_ERROR("Failed to connect to server: %d", result);
    return -1;
}

/*
 * Shutdown API client
 */
void api_shutdown(void)
{
    g_api.initialized = false;
    LOG("API client shutdown");
}

/*
 * Login with username and password
 */
int api_login(const char *username, const char *password, char *token_out, size_t token_len)
{
    if (!g_api.initialized) return -1;
    if (!username || !password || !token_out) return -1;

    LOG("Attempting login for user: %s", username);

    /* Build login request body */
    char body[512];
    snprintf(body, sizeof(body),
             "{\"username\":\"%s\",\"password\":\"%s\"}",
             username, password);

    char url[MAX_URL_LENGTH];
    build_url(url, sizeof(url), "/auth/local", NULL);

    char *response = NULL;
    size_t response_len = 0;
    int result = http_post(url, body, &response, &response_len);

    if (result != 0 || !response) {
        LOG_ERROR("Login request failed: %d", result);
        if (response) free(response);
        return -1;
    }

    /* Parse response */
    json_value_t *json = json_parse(response);
    free(response);

    if (!json) {
        LOG_ERROR("Failed to parse login response");
        return -1;
    }

    /* Extract token or session info */
    const char *token = json_get_string(json, "token");
    if (token) {
        strncpy(token_out, token, token_len - 1);
        token_out[token_len - 1] = '\0';
        LOG("Login successful");
        json_free(json);
        return 0;
    }

    /* Check for error message */
    const char *error = json_get_string(json, "error");
    if (error) {
        LOG_ERROR("Login failed: %s", error);
    }

    json_free(json);
    return -1;
}

/*
 * Get current user info
 */
int api_get_user_info(const char *token, char *username_out, size_t username_len)
{
    if (!g_api.initialized) return -1;
    if (!token || !username_out) return -1;

    char url[MAX_URL_LENGTH];
    build_url(url, sizeof(url), "/api/user", NULL);

    char *response = NULL;
    size_t response_len = 0;
    int result = http_get_with_auth(url, token, &response, &response_len);

    if (result != 0 || !response) {
        if (response) free(response);
        return -1;
    }

    /* Parse response */
    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    const char *username = json_get_string(json, "username");
    if (username) {
        strncpy(username_out, username, username_len - 1);
        username_out[username_len - 1] = '\0';
        json_free(json);
        return 0;
    }

    json_free(json);
    return -1;
}

/*
 * Browse directory
 */
int api_browse(const char *token, const char *path, library_type_t library, media_list_t *list)
{
    if (!g_api.initialized || !list) return -1;

    /* Clear existing list */
    list->count = 0;
    list->selected_index = 0;
    list->scroll_offset = 0;

    /* Build query parameters */
    char encoded_path[MAX_PATH_LENGTH * 3];
    url_encode(path ? path : "/", encoded_path, sizeof(encoded_path));

    char query[1024];
    snprintf(query, sizeof(query), "path=%s&limit=100", encoded_path);

    char url[MAX_URL_LENGTH];
    build_url(url, sizeof(url), "/api/browse", query);

    LOG("Browsing: %s", path);

    char *response = NULL;
    size_t response_len = 0;
    int result = http_get_with_auth(url, token, &response, &response_len);

    if (result != 0 || !response) {
        LOG_ERROR("Browse request failed: %d", result);
        if (response) free(response);
        return -1;
    }

    /* Parse response */
    json_value_t *json = json_parse(response);
    free(response);

    if (!json) {
        LOG_ERROR("Failed to parse browse response");
        return -1;
    }

    /* Extract files array */
    json_value_t *files = json_get_array(json, "files");
    if (!files) {
        json_free(json);
        return 0;  /* Empty directory */
    }

    int file_count = json_array_length(files);
    LOG("Found %d items", file_count);

    for (int i = 0; i < file_count && list->count < list->capacity; i++) {
        json_value_t *file = json_array_get(files, i);
        if (!file) continue;

        media_item_t *item = &list->items[list->count];
        memset(item, 0, sizeof(*item));

        /* Get file properties */
        const char *name = json_get_string(file, "name");
        const char *file_path = json_get_string(file, "path");
        bool is_dir = json_get_bool(file, "isDirectory", false);
        const char *type = json_get_string(file, "type");

        if (name) {
            strncpy(item->name, name, sizeof(item->name) - 1);
        }
        if (file_path) {
            strncpy(item->path, file_path, sizeof(item->path) - 1);
        }

        item->is_directory = is_dir;
        item->size = json_get_int(file, "size", 0);

        /* Determine media type */
        if (is_dir) {
            item->type = MEDIA_TYPE_DIRECTORY;
        } else if (type) {
            if (strcmp(type, "video") == 0) {
                item->type = MEDIA_TYPE_VIDEO;
            } else if (strcmp(type, "audio") == 0) {
                item->type = MEDIA_TYPE_AUDIO;
            } else {
                item->type = MEDIA_TYPE_UNKNOWN;
            }
        } else {
            /* Detect by extension */
            const char *ext = strrchr(name ? name : "", '.');
            if (ext) {
                if (strstr(".mp4.mkv.avi.mov.webm.m4v.flv.wmv", ext)) {
                    item->type = MEDIA_TYPE_VIDEO;
                } else if (strstr(".mp3.m4a.flac.wav.aac.ogg.wma.opus", ext)) {
                    item->type = MEDIA_TYPE_AUDIO;
                }
            }
        }

        list->count++;
    }

    json_free(json);
    LOG("Loaded %d items into list", list->count);
    return 0;
}

/*
 * Search media
 */
int api_search(const char *token, const char *query_str, media_list_t *list)
{
    if (!g_api.initialized || !list || !query_str) return -1;

    /* Clear existing list */
    list->count = 0;
    list->selected_index = 0;
    list->scroll_offset = 0;

    /* Build query parameters */
    char encoded_query[256];
    url_encode(query_str, encoded_query, sizeof(encoded_query));

    char query[512];
    snprintf(query, sizeof(query), "q=%s&limit=50", encoded_query);

    char url[MAX_URL_LENGTH];
    build_url(url, sizeof(url), "/api/search", query);

    LOG("Searching for: %s", query_str);

    char *response = NULL;
    size_t response_len = 0;
    int result = http_get_with_auth(url, token, &response, &response_len);

    if (result != 0 || !response) {
        if (response) free(response);
        return -1;
    }

    /* Parse response - similar to browse */
    json_value_t *json = json_parse(response);
    free(response);

    if (!json) return -1;

    json_value_t *results = json_get_array(json, "results");
    if (!results) {
        json_free(json);
        return 0;
    }

    int result_count = json_array_length(results);
    for (int i = 0; i < result_count && list->count < list->capacity; i++) {
        json_value_t *item_json = json_array_get(results, i);
        if (!item_json) continue;

        media_item_t *item = &list->items[list->count];
        memset(item, 0, sizeof(*item));

        const char *name = json_get_string(item_json, "name");
        const char *item_path = json_get_string(item_json, "path");

        if (name) strncpy(item->name, name, sizeof(item->name) - 1);
        if (item_path) strncpy(item->path, item_path, sizeof(item->path) - 1);

        item->is_directory = false;
        item->type = MEDIA_TYPE_VIDEO;  /* Assume video for search results */

        list->count++;
    }

    json_free(json);
    return 0;
}

/*
 * Get streaming URL for a file
 */
int api_get_stream_url(const char *token, const char *path, char *url_out, size_t url_len)
{
    if (!g_api.initialized || !path || !url_out) return -1;

    /* Build video streaming URL */
    char encoded_path[MAX_PATH_LENGTH * 3];
    url_encode(path, encoded_path, sizeof(encoded_path));

    /* Note: For Original Xbox, we should prefer transcoded stream for compatibility */
    snprintf(url_out, url_len, "%s/api/video-transcode?path=%s",
             g_api.base_url, encoded_path);

    LOG("Stream URL: %s", url_out);
    return 0;
}

/*
 * Get audio track count for a video
 */
int api_get_audio_tracks(const char *token, const char *path, int *count)
{
    if (!g_api.initialized || !path || !count) return -1;

    char encoded_path[MAX_PATH_LENGTH * 3];
    url_encode(path, encoded_path, sizeof(encoded_path));

    char query[1024];
    snprintf(query, sizeof(query), "path=%s", encoded_path);

    char url[MAX_URL_LENGTH];
    build_url(url, sizeof(url), "/api/audio-tracks", query);

    char *response = NULL;
    size_t response_len = 0;
    int result = http_get_with_auth(url, token, &response, &response_len);

    if (result != 0 || !response) {
        if (response) free(response);
        *count = 1;  /* Assume at least one track */
        return -1;
    }

    json_value_t *json = json_parse(response);
    free(response);

    if (!json) {
        *count = 1;
        return -1;
    }

    json_value_t *tracks = json_get_array(json, "tracks");
    if (tracks) {
        *count = json_array_length(tracks);
    } else {
        *count = 1;
    }

    json_free(json);
    return 0;
}

/*
 * Save user settings to server
 */
int api_save_settings(const char *token, const user_settings_t *settings)
{
    if (!g_api.initialized || !settings) return -1;

    /* Build settings JSON */
    char body[1024];
    snprintf(body, sizeof(body),
             "{"
             "\"volume\":%d,"
             "\"playback_speed\":%d,"
             "\"autoplay\":%s,"
             "\"subtitle_language\":\"%s\","
             "\"audio_language\":\"%s\""
             "}",
             settings->volume,
             settings->playback_speed,
             settings->autoplay ? "true" : "false",
             settings->subtitle_language,
             settings->audio_language
    );

    char url[MAX_URL_LENGTH];
    build_url(url, sizeof(url), "/api/settings", NULL);

    /* Note: Would need http_post_with_auth for this */
    /* For now, just log the attempt */
    LOG("Settings sync not implemented (would POST to %s)", url);

    return 0;
}
