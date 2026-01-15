/*
 * Nedflix for Original Xbox
 * Configuration persistence
 *
 * Stores settings in a simple INI-like format on the Xbox HDD.
 * Config file location: E:\UDATA\Nedflix\nedflix.cfg
 */

#include "nedflix.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef NXDK
#include <windows.h>
#include <nxdk/path.h>
#endif

/* Config file path */
#define CONFIG_DIR  "E:\\UDATA\\Nedflix"
#define CONFIG_FILE "E:\\UDATA\\Nedflix\\nedflix.cfg"

/* Config keys */
#define KEY_SERVER_URL      "server_url"
#define KEY_USERNAME        "username"
#define KEY_AUTH_TOKEN      "auth_token"
#define KEY_VOLUME          "volume"
#define KEY_PLAYBACK_SPEED  "playback_speed"
#define KEY_AUTOPLAY        "autoplay"
#define KEY_SHOW_SUBTITLES  "show_subtitles"
#define KEY_SUBTITLE_LANG   "subtitle_language"
#define KEY_AUDIO_LANG      "audio_language"
#define KEY_THEME           "theme"

/*
 * Set default configuration values
 */
void config_set_defaults(user_settings_t *settings)
{
    if (!settings) return;

    memset(settings, 0, sizeof(*settings));

    /* Default server URL - empty so client mode will prompt for IP entry */
    settings->server_url[0] = '\0';

    /* Default settings */
    settings->volume = 80;
    settings->playback_speed = 100;  /* 1.0x */
    settings->autoplay = true;
    settings->show_subtitles = false;
    strncpy(settings->subtitle_language, "en", sizeof(settings->subtitle_language) - 1);
    strncpy(settings->audio_language, "en", sizeof(settings->audio_language) - 1);
    settings->theme = 0;  /* Dark */
}

/*
 * Parse a line from config file
 */
static void parse_config_line(const char *line, user_settings_t *settings)
{
    /* Skip empty lines and comments */
    if (!line || line[0] == '\0' || line[0] == '#' || line[0] == ';') {
        return;
    }

    /* Find key=value separator */
    const char *eq = strchr(line, '=');
    if (!eq) return;

    /* Extract key */
    char key[64] = {0};
    size_t key_len = eq - line;
    if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
    strncpy(key, line, key_len);

    /* Trim key */
    while (key_len > 0 && (key[key_len-1] == ' ' || key[key_len-1] == '\t')) {
        key[--key_len] = '\0';
    }

    /* Extract value */
    const char *value = eq + 1;
    while (*value == ' ' || *value == '\t') value++;

    /* Remove trailing newline/whitespace from value */
    char val_buf[512];
    strncpy(val_buf, value, sizeof(val_buf) - 1);
    val_buf[sizeof(val_buf) - 1] = '\0';
    size_t val_len = strlen(val_buf);
    while (val_len > 0 && (val_buf[val_len-1] == '\n' || val_buf[val_len-1] == '\r' ||
           val_buf[val_len-1] == ' ' || val_buf[val_len-1] == '\t')) {
        val_buf[--val_len] = '\0';
    }

    /* Apply setting */
    if (strcmp(key, KEY_SERVER_URL) == 0) {
        strncpy(settings->server_url, val_buf, sizeof(settings->server_url) - 1);
    } else if (strcmp(key, KEY_USERNAME) == 0) {
        strncpy(settings->username, val_buf, sizeof(settings->username) - 1);
    } else if (strcmp(key, KEY_AUTH_TOKEN) == 0) {
        strncpy(settings->auth_token, val_buf, sizeof(settings->auth_token) - 1);
    } else if (strcmp(key, KEY_VOLUME) == 0) {
        settings->volume = CLAMP(atoi(val_buf), 0, 100);
    } else if (strcmp(key, KEY_PLAYBACK_SPEED) == 0) {
        settings->playback_speed = CLAMP(atoi(val_buf), 50, 200);
    } else if (strcmp(key, KEY_AUTOPLAY) == 0) {
        settings->autoplay = (strcmp(val_buf, "1") == 0 || strcmp(val_buf, "true") == 0);
    } else if (strcmp(key, KEY_SHOW_SUBTITLES) == 0) {
        settings->show_subtitles = (strcmp(val_buf, "1") == 0 || strcmp(val_buf, "true") == 0);
    } else if (strcmp(key, KEY_SUBTITLE_LANG) == 0) {
        strncpy(settings->subtitle_language, val_buf, sizeof(settings->subtitle_language) - 1);
    } else if (strcmp(key, KEY_AUDIO_LANG) == 0) {
        strncpy(settings->audio_language, val_buf, sizeof(settings->audio_language) - 1);
    } else if (strcmp(key, KEY_THEME) == 0) {
        settings->theme = atoi(val_buf);
    }
}

/*
 * Load configuration from file
 */
int config_load(user_settings_t *settings)
{
    if (!settings) return -1;

    LOG("Loading config from %s", CONFIG_FILE);

#ifdef NXDK
    /* Open config file */
    HANDLE hFile = CreateFile(CONFIG_FILE, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG("Config file not found, using defaults");
        return -1;
    }

    /* Get file size */
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > 16384) {
        CloseHandle(hFile);
        return -1;
    }

    /* Read file */
    char *buffer = (char *)malloc(fileSize + 1);
    if (!buffer) {
        CloseHandle(hFile);
        return -1;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
        free(buffer);
        CloseHandle(hFile);
        return -1;
    }
    buffer[bytesRead] = '\0';

    CloseHandle(hFile);

    /* Parse lines */
    char *line = strtok(buffer, "\n");
    while (line) {
        parse_config_line(line, settings);
        line = strtok(NULL, "\n");
    }

    free(buffer);
    LOG("Config loaded successfully");
    return 0;

#else
    /* Non-Xbox: try to read from current directory */
    FILE *fp = fopen("nedflix.cfg", "r");
    if (!fp) {
        LOG("Config file not found");
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        parse_config_line(line, settings);
    }

    fclose(fp);
    LOG("Config loaded");
    return 0;
#endif
}

/*
 * Save configuration to file
 */
int config_save(const user_settings_t *settings)
{
    if (!settings) return -1;

    LOG("Saving config to %s", CONFIG_FILE);

#ifdef NXDK
    /* Create directory if it doesn't exist */
    CreateDirectory(CONFIG_DIR, NULL);

    /* Open/create config file */
    HANDLE hFile = CreateFile(CONFIG_FILE, GENERIC_WRITE, 0,
                               NULL, CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to create config file");
        return -1;
    }

    /* Format config content */
    char buffer[2048];
    int len = snprintf(buffer, sizeof(buffer),
        "# Nedflix Configuration\n"
        "# Original Xbox Edition\n"
        "\n"
        "# Server settings\n"
        "%s=%s\n"
        "%s=%s\n"
        "%s=%s\n"
        "\n"
        "# Playback settings\n"
        "%s=%d\n"
        "%s=%d\n"
        "%s=%d\n"
        "%s=%d\n"
        "\n"
        "# Language settings\n"
        "%s=%s\n"
        "%s=%s\n"
        "\n"
        "# Appearance\n"
        "%s=%d\n",
        KEY_SERVER_URL, settings->server_url,
        KEY_USERNAME, settings->username,
        KEY_AUTH_TOKEN, settings->auth_token,
        KEY_VOLUME, settings->volume,
        KEY_PLAYBACK_SPEED, settings->playback_speed,
        KEY_AUTOPLAY, settings->autoplay ? 1 : 0,
        KEY_SHOW_SUBTITLES, settings->show_subtitles ? 1 : 0,
        KEY_SUBTITLE_LANG, settings->subtitle_language,
        KEY_AUDIO_LANG, settings->audio_language,
        KEY_THEME, settings->theme
    );

    /* Write to file */
    DWORD bytesWritten;
    BOOL result = WriteFile(hFile, buffer, len, &bytesWritten, NULL);
    CloseHandle(hFile);

    if (!result || bytesWritten != (DWORD)len) {
        LOG_ERROR("Failed to write config file");
        return -1;
    }

    LOG("Config saved successfully");
    return 0;

#else
    /* Non-Xbox: write to current directory */
    FILE *fp = fopen("nedflix.cfg", "w");
    if (!fp) {
        LOG_ERROR("Failed to create config file");
        return -1;
    }

    fprintf(fp, "# Nedflix Configuration\n");
    fprintf(fp, "%s=%s\n", KEY_SERVER_URL, settings->server_url);
    fprintf(fp, "%s=%s\n", KEY_USERNAME, settings->username);
    fprintf(fp, "%s=%s\n", KEY_AUTH_TOKEN, settings->auth_token);
    fprintf(fp, "%s=%d\n", KEY_VOLUME, settings->volume);
    fprintf(fp, "%s=%d\n", KEY_PLAYBACK_SPEED, settings->playback_speed);
    fprintf(fp, "%s=%d\n", KEY_AUTOPLAY, settings->autoplay ? 1 : 0);
    fprintf(fp, "%s=%d\n", KEY_SHOW_SUBTITLES, settings->show_subtitles ? 1 : 0);
    fprintf(fp, "%s=%s\n", KEY_SUBTITLE_LANG, settings->subtitle_language);
    fprintf(fp, "%s=%s\n", KEY_AUDIO_LANG, settings->audio_language);
    fprintf(fp, "%s=%d\n", KEY_THEME, settings->theme);

    fclose(fp);
    LOG("Config saved");
    return 0;
#endif
}
