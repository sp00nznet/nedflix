/*
 * Nedflix for Sega Dreamcast
 * Configuration persistence using VMU (Visual Memory Unit)
 *
 * The VMU has 128KB of flash memory, with ~100KB available for saves.
 * We use a small save file to store user preferences.
 */

#include "nedflix.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <dc/vmu_pkg.h>
#include <dc/fs.h>

/* VMU save file name (8 chars max) */
#define VMU_FILENAME    "NEDFLIX"
#define VMU_PATH        "/vmu/a1/" VMU_FILENAME

/* Config version for compatibility checking */
#define CONFIG_VERSION  1

/* Save file structure (must be <= 128 bytes for small VMU save) */
typedef struct {
    uint8 version;
    uint8 volume;
    uint8 autoplay;
    uint8 show_subtitles;
    uint8 theme;
    uint8 reserved[3];
    char server_url[64];
    char username[32];
    char subtitle_language[4];
    char audio_language[4];
    /* auth_token intentionally NOT saved for security */
} config_save_t;

/*
 * Set default configuration values
 */
void config_defaults(user_settings_t *settings)
{
    if (!settings) return;

    memset(settings, 0, sizeof(*settings));

    /* Default server URL */
    strncpy(settings->server_url, "http://192.168.1.100:3000",
            sizeof(settings->server_url) - 1);

    /* Default settings */
    settings->volume = 80;
    settings->autoplay = true;
    settings->show_subtitles = false;
    strncpy(settings->subtitle_language, "en", sizeof(settings->subtitle_language) - 1);
    strncpy(settings->audio_language, "en", sizeof(settings->audio_language) - 1);
    settings->theme = 0;  /* Dark */
}

/*
 * Find first available VMU
 */
static maple_device_t *find_vmu(void)
{
    /* Try each port/unit combination */
    for (int port = 0; port < 4; port++) {
        for (int unit = 1; unit <= 2; unit++) {
            maple_device_t *vmu = maple_enum_dev(port, unit);
            if (vmu && (vmu->info.functions & MAPLE_FUNC_MEMCARD)) {
                return vmu;
            }
        }
    }
    return NULL;
}

/*
 * Load configuration from VMU
 */
int config_load(user_settings_t *settings)
{
    if (!settings) return -1;

    LOG("Loading config from VMU...");

    /* Try to open the save file */
    file_t f = fs_open(VMU_PATH, O_RDONLY);
    if (f == FILEHND_INVALID) {
        /* Try other VMU slots */
        const char *paths[] = {
            "/vmu/a1/" VMU_FILENAME,
            "/vmu/a2/" VMU_FILENAME,
            "/vmu/b1/" VMU_FILENAME,
            "/vmu/b2/" VMU_FILENAME,
            "/vmu/c1/" VMU_FILENAME,
            "/vmu/c2/" VMU_FILENAME,
            "/vmu/d1/" VMU_FILENAME,
            "/vmu/d2/" VMU_FILENAME,
        };

        for (int i = 0; i < 8; i++) {
            f = fs_open(paths[i], O_RDONLY);
            if (f != FILEHND_INVALID) {
                break;
            }
        }

        if (f == FILEHND_INVALID) {
            LOG("Config file not found, using defaults");
            return -1;
        }
    }

    /* Read save data */
    config_save_t save;
    ssize_t bytes = fs_read(f, &save, sizeof(save));
    fs_close(f);

    if (bytes != sizeof(save)) {
        LOG_ERROR("Invalid config file size");
        return -1;
    }

    /* Check version */
    if (save.version != CONFIG_VERSION) {
        LOG("Config version mismatch, using defaults");
        return -1;
    }

    /* Apply settings */
    strncpy(settings->server_url, save.server_url, sizeof(settings->server_url) - 1);
    strncpy(settings->username, save.username, sizeof(settings->username) - 1);
    strncpy(settings->subtitle_language, save.subtitle_language, sizeof(settings->subtitle_language) - 1);
    strncpy(settings->audio_language, save.audio_language, sizeof(settings->audio_language) - 1);

    settings->volume = CLAMP(save.volume, 0, 100);
    settings->autoplay = save.autoplay ? true : false;
    settings->show_subtitles = save.show_subtitles ? true : false;
    settings->theme = save.theme;

    LOG("Config loaded successfully");
    return 0;
}

/*
 * Save configuration to VMU
 */
int config_save(const user_settings_t *settings)
{
    if (!settings) return -1;

    LOG("Saving config to VMU...");

    /* Find a VMU */
    maple_device_t *vmu = find_vmu();
    if (!vmu) {
        LOG_ERROR("No VMU found");
        return -1;
    }

    /* Build save structure */
    config_save_t save;
    memset(&save, 0, sizeof(save));

    save.version = CONFIG_VERSION;
    save.volume = settings->volume;
    save.autoplay = settings->autoplay ? 1 : 0;
    save.show_subtitles = settings->show_subtitles ? 1 : 0;
    save.theme = settings->theme;

    strncpy(save.server_url, settings->server_url, sizeof(save.server_url) - 1);
    strncpy(save.username, settings->username, sizeof(save.username) - 1);
    strncpy(save.subtitle_language, settings->subtitle_language, sizeof(save.subtitle_language) - 1);
    strncpy(save.audio_language, settings->audio_language, sizeof(save.audio_language) - 1);

    /* Create VMU package */
    vmu_pkg_t pkg;
    memset(&pkg, 0, sizeof(pkg));

    strcpy(pkg.desc_short, "Nedflix");
    strcpy(pkg.desc_long, "Nedflix Settings");
    strcpy(pkg.app_id, "NEDFLIX");
    pkg.icon_cnt = 0;  /* No icon for now */
    pkg.eyecatch_type = VMUPKG_EC_NONE;
    pkg.data_len = sizeof(save);
    pkg.data = (uint8 *)&save;

    /* Pack the data */
    uint8 *pkg_out;
    int pkg_size;
    if (vmu_pkg_build(&pkg, &pkg_out, &pkg_size) < 0) {
        LOG_ERROR("Failed to build VMU package");
        return -1;
    }

    /* Build VMU path */
    char path[64];
    snprintf(path, sizeof(path), "/vmu/%c%d/" VMU_FILENAME,
             'a' + vmu->port, vmu->unit);

    /* Write to VMU */
    file_t f = fs_open(path, O_WRONLY | O_CREAT);
    if (f == FILEHND_INVALID) {
        LOG_ERROR("Failed to open VMU for writing");
        free(pkg_out);
        return -1;
    }

    ssize_t written = fs_write(f, pkg_out, pkg_size);
    fs_close(f);
    free(pkg_out);

    if (written != pkg_size) {
        LOG_ERROR("Failed to write config to VMU");
        return -1;
    }

    LOG("Config saved successfully to %s", path);
    return 0;
}

/*
 * Delete configuration from VMU
 */
int config_delete(void)
{
    LOG("Deleting config from VMU...");

    const char *paths[] = {
        "/vmu/a1/" VMU_FILENAME,
        "/vmu/a2/" VMU_FILENAME,
        "/vmu/b1/" VMU_FILENAME,
        "/vmu/b2/" VMU_FILENAME,
    };

    int deleted = 0;
    for (int i = 0; i < 4; i++) {
        if (fs_unlink(paths[i]) == 0) {
            LOG("Deleted %s", paths[i]);
            deleted++;
        }
    }

    return deleted > 0 ? 0 : -1;
}
