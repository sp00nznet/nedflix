/*
 * Nedflix for Xbox 360
 * Configuration management
 *
 * TECHNICAL DEMO / NOVELTY PORT
 *
 * Stores settings on USB drive at /nedflix/config.dat
 */

#include "nedflix.h"
#include <diskio/diskio.h>

/* Config file path (USB drive) */
#define CONFIG_PATH "uda:/nedflix/config.dat"

/* Config magic and version */
#define CONFIG_MAGIC   0x4E465833  /* "NFX3" */
#define CONFIG_VERSION 1

/* Config header */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t checksum;
} config_header_t;

/*
 * Calculate checksum
 */
static uint32_t calc_checksum(const void *data, size_t size)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t sum = 0;

    for (size_t i = 0; i < size; i++) {
        sum += p[i];
        sum = (sum << 1) | (sum >> 31);
    }

    return sum;
}

/*
 * Set default settings
 */
void config_set_defaults(user_settings_t *settings)
{
    if (!settings) return;

    memset(settings, 0, sizeof(user_settings_t));

    strcpy(settings->server_url, "http://192.168.1.100:3000");
    settings->volume = 80;
    settings->library = LIBRARY_MUSIC;
    settings->autoplay = true;
    settings->show_subtitles = false;
    strcpy(settings->subtitle_lang, "en");
    strcpy(settings->audio_lang, "en");
}

/*
 * Load configuration
 */
int config_load(user_settings_t *settings)
{
    if (!settings) return -1;

    config_set_defaults(settings);

    FILE *fp = fopen(CONFIG_PATH, "rb");
    if (!fp) {
        LOG("No config file, using defaults");
        return 0;
    }

    config_header_t header;
    if (fread(&header, 1, sizeof(header), fp) != sizeof(header)) {
        fclose(fp);
        return -1;
    }

    if (header.magic != CONFIG_MAGIC || header.version != CONFIG_VERSION) {
        LOG("Config version mismatch, using defaults");
        fclose(fp);
        return 0;
    }

    user_settings_t loaded;
    if (fread(&loaded, 1, sizeof(loaded), fp) != sizeof(loaded)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    if (calc_checksum(&loaded, sizeof(loaded)) != header.checksum) {
        LOG_ERROR("Config checksum mismatch");
        return -1;
    }

    memcpy(settings, &loaded, sizeof(user_settings_t));
    LOG("Configuration loaded");
    return 0;
}

/*
 * Save configuration
 */
int config_save(const user_settings_t *settings)
{
    if (!settings) return -1;

    /* Ensure directory exists */
    mkdir("uda:/nedflix", 0755);

    FILE *fp = fopen(CONFIG_PATH, "wb");
    if (!fp) {
        LOG_ERROR("Failed to create config file");
        return -1;
    }

    config_header_t header;
    header.magic = CONFIG_MAGIC;
    header.version = CONFIG_VERSION;
    header.size = sizeof(user_settings_t);
    header.checksum = calc_checksum(settings, sizeof(user_settings_t));

    if (fwrite(&header, 1, sizeof(header), fp) != sizeof(header)) {
        fclose(fp);
        return -1;
    }

    if (fwrite(settings, 1, sizeof(user_settings_t), fp) != sizeof(user_settings_t)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    LOG("Configuration saved");
    return 0;
}
