/*
 * Nedflix for Nintendo GameCube
 * Configuration management
 *
 * TECHNICAL DEMO / NOVELTY PORT
 *
 * Stores settings on SD card at /nedflix/config/settings.dat
 */

#include "nedflix.h"

/* Config file path */
#define CONFIG_PATH "/nedflix/config/settings.dat"

/* Config file magic number for validation */
#define CONFIG_MAGIC 0x4E454443  /* "NEDC" */
#define CONFIG_VERSION 1

/* Config file header */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t checksum;
} config_header_t;

/*
 * Calculate simple checksum
 */
static uint32_t calc_checksum(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;

    for (size_t i = 0; i < size; i++) {
        sum += bytes[i];
        sum = (sum << 1) | (sum >> 31);  /* Rotate left */
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

    settings->volume = 200;          /* ~78% */
    settings->shuffle = false;
    settings->repeat = false;
    strncpy(settings->last_path, "/nedflix/music", sizeof(settings->last_path) - 1);
}

/*
 * Load configuration from SD card
 */
int config_load(user_settings_t *settings)
{
    if (!settings) {
        return -1;
    }

    /* Set defaults first */
    config_set_defaults(settings);

    /* Open config file */
    FILE *fp = fopen(CONFIG_PATH, "rb");
    if (!fp) {
        LOG("No config file found, using defaults");
        return 0;  /* Not an error - just use defaults */
    }

    /* Read header */
    config_header_t header;
    if (fread(&header, 1, sizeof(header), fp) != sizeof(header)) {
        LOG_ERROR("Failed to read config header");
        fclose(fp);
        return -1;
    }

    /* Validate header */
    if (header.magic != CONFIG_MAGIC) {
        LOG_ERROR("Invalid config file magic");
        fclose(fp);
        return -1;
    }

    if (header.version != CONFIG_VERSION) {
        LOG("Config version mismatch, using defaults");
        fclose(fp);
        return 0;
    }

    if (header.size != sizeof(user_settings_t)) {
        LOG("Config size mismatch, using defaults");
        fclose(fp);
        return 0;
    }

    /* Read settings */
    user_settings_t loaded_settings;
    if (fread(&loaded_settings, 1, sizeof(loaded_settings), fp) != sizeof(loaded_settings)) {
        LOG_ERROR("Failed to read config data");
        fclose(fp);
        return -1;
    }

    fclose(fp);

    /* Validate checksum */
    uint32_t checksum = calc_checksum(&loaded_settings, sizeof(loaded_settings));
    if (checksum != header.checksum) {
        LOG_ERROR("Config checksum mismatch");
        return -1;
    }

    /* Copy to output */
    memcpy(settings, &loaded_settings, sizeof(user_settings_t));

    LOG("Configuration loaded from %s", CONFIG_PATH);
    return 0;
}

/*
 * Save configuration to SD card
 */
int config_save(const user_settings_t *settings)
{
    if (!settings) {
        return -1;
    }

    /* Create config directory if needed */
    mkdir("/nedflix", 0755);
    mkdir("/nedflix/config", 0755);

    /* Open config file */
    FILE *fp = fopen(CONFIG_PATH, "wb");
    if (!fp) {
        LOG_ERROR("Failed to create config file");
        return -1;
    }

    /* Prepare header */
    config_header_t header;
    header.magic = CONFIG_MAGIC;
    header.version = CONFIG_VERSION;
    header.size = sizeof(user_settings_t);
    header.checksum = calc_checksum(settings, sizeof(user_settings_t));

    /* Write header */
    if (fwrite(&header, 1, sizeof(header), fp) != sizeof(header)) {
        LOG_ERROR("Failed to write config header");
        fclose(fp);
        return -1;
    }

    /* Write settings */
    if (fwrite(settings, 1, sizeof(user_settings_t), fp) != sizeof(user_settings_t)) {
        LOG_ERROR("Failed to write config data");
        fclose(fp);
        return -1;
    }

    fclose(fp);

    LOG("Configuration saved to %s", CONFIG_PATH);
    return 0;
}
