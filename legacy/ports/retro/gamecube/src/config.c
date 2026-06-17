/*
 * Nedflix for Nintendo GameCube
 * Configuration management with favorites and history
 *
 * Stores data on SD card at /nedflix/
 */

#include "nedflix.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* Configuration file paths on SD card */
#define CONFIG_DIR      "/nedflix"
#define CONFIG_PATH     CONFIG_DIR "/nedflix.cfg"
#define FAVORITES_PATH  CONFIG_DIR "/favorites.dat"
#define HISTORY_PATH    CONFIG_DIR "/history.dat"

/* File magic headers */
#define CONFIG_MAGIC    "NDGC0002"
#define FAVORITES_MAGIC "NDGF0001"
#define HISTORY_MAGIC   "NDGH0001"

/* Calculate checksum for config validation */
static uint32_t calc_checksum(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;

    for (size_t i = 0; i < size; i++) {
        sum += bytes[i];
        sum = (sum << 1) | (sum >> 31);
    }

    return sum;
}

/* Set default configuration values */
void config_set_defaults(user_settings_t *settings)
{
    if (!settings) return;

    memset(settings, 0, sizeof(user_settings_t));

    /* Audio */
    settings->volume = 200;  /* 0-255 for GameCube */
    settings->enable_surround = false;
    strcpy(settings->audio_language, "en");

    /* Playback */
    settings->shuffle = false;
    settings->repeat = false;
    settings->repeat_one = false;
    settings->autoplay = true;
    settings->continue_watching = true;

    /* Subtitles */
    settings->show_subtitles = true;
    strcpy(settings->subtitle_language, "en");
    settings->subtitle_size = 1;

    /* UI */
    settings->library = LIBRARY_MUSIC;
    settings->theme = 0;
    settings->show_clock = true;
    settings->animations = true;

    /* Parental */
    settings->parental_enabled = false;
    settings->max_rating = 4;
    memset(settings->parental_pin, 0, sizeof(settings->parental_pin));

    /* Misc */
    strcpy(settings->last_path, "/nedflix/music");
    settings->first_run = true;
    settings->active_profile = 0;
}

/* Load configuration from SD card */
int config_load(user_settings_t *settings)
{
    if (!settings) return -1;

    config_set_defaults(settings);

    FILE *f = fopen(CONFIG_PATH, "rb");
    if (!f) {
        LOG("Config: No saved config, using defaults");
        return -1;
    }

    /* Read and verify magic */
    char magic[8];
    if (fread(magic, 1, 8, f) != 8) {
        fclose(f);
        return -1;
    }

    if (memcmp(magic, CONFIG_MAGIC, 8) != 0) {
        /* Check for legacy format */
        if (memcmp(magic, "NDGC0001", 8) == 0 ||
            *(uint32_t*)magic == 0x4E454443) {  /* Old "NEDC" magic */
            LOG("Config: Upgrading from legacy format");
            fseek(f, 0, SEEK_SET);
            /* Read what we can from old format */
            fseek(f, 16, SEEK_SET);  /* Skip old header */
            int vol;
            if (fread(&vol, sizeof(int), 1, f) == 1) {
                settings->volume = vol;
            }
            fclose(f);
            config_save(settings);
            return 0;
        }
        LOG("Config: Invalid format");
        fclose(f);
        return -1;
    }

    /* Read version and checksum */
    uint32_t version, checksum;
    fread(&version, sizeof(uint32_t), 1, f);
    fread(&checksum, sizeof(uint32_t), 1, f);

    /* Read settings structure */
    user_settings_t loaded;
    size_t read = fread(&loaded, 1, sizeof(user_settings_t), f);
    fclose(f);

    if (read != sizeof(user_settings_t)) {
        LOG("Config: Partial read, using defaults");
        return -1;
    }

    /* Validate checksum */
    if (calc_checksum(&loaded, sizeof(user_settings_t)) != checksum) {
        LOG("Config: Checksum mismatch");
        return -1;
    }

    /* Copy loaded settings */
    memcpy(settings, &loaded, sizeof(user_settings_t));

    /* Validate ranges */
    if (settings->volume > 255) settings->volume = 200;
    if (settings->library >= LIBRARY_COUNT) settings->library = LIBRARY_MUSIC;

    settings->first_run = false;
    LOG("Config: Loaded successfully");
    return 0;
}

/* Save configuration to SD card */
int config_save(const user_settings_t *settings)
{
    if (!settings) return -1;

    /* Create directories */
    mkdir(CONFIG_DIR, 0755);

    FILE *f = fopen(CONFIG_PATH, "wb");
    if (!f) {
        LOG_ERROR("Config: Cannot write to %s", CONFIG_PATH);
        return -1;
    }

    /* Write magic */
    fwrite(CONFIG_MAGIC, 1, 8, f);

    /* Write version and checksum */
    uint32_t version = 2;
    uint32_t checksum = calc_checksum(settings, sizeof(user_settings_t));
    fwrite(&version, sizeof(uint32_t), 1, f);
    fwrite(&checksum, sizeof(uint32_t), 1, f);

    /* Write settings */
    fwrite(settings, 1, sizeof(user_settings_t), f);
    fclose(f);

    LOG("Config: Saved to %s", CONFIG_PATH);
    return 0;
}

/* Load favorites list */
int config_load_favorites(void)
{
    g_app.favorites_count = 0;
    memset(g_app.favorite_ids, 0, sizeof(g_app.favorite_ids));

    FILE *f = fopen(FAVORITES_PATH, "rb");
    if (!f) {
        return 0;  /* No favorites yet */
    }

    /* Verify magic */
    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, FAVORITES_MAGIC, 8) != 0) {
        fclose(f);
        return -1;
    }

    /* Read count */
    uint32_t count;
    if (fread(&count, sizeof(uint32_t), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (count > MAX_FAVORITES) count = MAX_FAVORITES;

    /* Read favorite IDs */
    for (uint32_t i = 0; i < count; i++) {
        if (fread(g_app.favorite_ids[i], 64, 1, f) != 1) break;
        g_app.favorites_count++;
    }

    fclose(f);
    LOG("Config: Loaded %d favorites", g_app.favorites_count);
    return 0;
}

/* Save favorites list */
int config_save_favorites(void)
{
    FILE *f = fopen(FAVORITES_PATH, "wb");
    if (!f) {
        LOG_ERROR("Config: Cannot write favorites");
        return -1;
    }

    /* Write magic */
    fwrite(FAVORITES_MAGIC, 1, 8, f);

    /* Write count */
    uint32_t count = g_app.favorites_count;
    fwrite(&count, sizeof(uint32_t), 1, f);

    /* Write favorite IDs */
    for (int i = 0; i < g_app.favorites_count; i++) {
        fwrite(g_app.favorite_ids[i], 64, 1, f);
    }

    fclose(f);
    LOG("Config: Saved %d favorites", g_app.favorites_count);
    return 0;
}

/* Add item to favorites */
int config_add_favorite(const char *item_id)
{
    if (!item_id || item_id[0] == '\0') return -1;

    /* Check if already favorited */
    for (int i = 0; i < g_app.favorites_count; i++) {
        if (strcmp(g_app.favorite_ids[i], item_id) == 0) {
            return 0;
        }
    }

    if (g_app.favorites_count >= MAX_FAVORITES) {
        LOG_ERROR("Config: Favorites list full");
        return -1;
    }

    strncpy(g_app.favorite_ids[g_app.favorites_count], item_id, 63);
    g_app.favorite_ids[g_app.favorites_count][63] = '\0';
    g_app.favorites_count++;

    return config_save_favorites();
}

/* Remove item from favorites */
int config_remove_favorite(const char *item_id)
{
    if (!item_id) return -1;

    for (int i = 0; i < g_app.favorites_count; i++) {
        if (strcmp(g_app.favorite_ids[i], item_id) == 0) {
            for (int j = i; j < g_app.favorites_count - 1; j++) {
                strcpy(g_app.favorite_ids[j], g_app.favorite_ids[j + 1]);
            }
            g_app.favorites_count--;
            return config_save_favorites();
        }
    }

    return 0;
}

/* Check if item is favorited */
bool config_is_favorite(const char *item_id)
{
    if (!item_id) return false;

    for (int i = 0; i < g_app.favorites_count; i++) {
        if (strcmp(g_app.favorite_ids[i], item_id) == 0) {
            return true;
        }
    }
    return false;
}

/* Load watch history */
int config_load_history(void)
{
    g_app.history_count = 0;
    memset(g_app.history, 0, sizeof(g_app.history));

    FILE *f = fopen(HISTORY_PATH, "rb");
    if (!f) {
        return 0;
    }

    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, HISTORY_MAGIC, 8) != 0) {
        fclose(f);
        return -1;
    }

    uint32_t count;
    if (fread(&count, sizeof(uint32_t), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (count > MAX_HISTORY) count = MAX_HISTORY;

    for (uint32_t i = 0; i < count; i++) {
        if (fread(&g_app.history[i], sizeof(history_entry_t), 1, f) != 1) {
            break;
        }
        g_app.history_count++;
    }

    fclose(f);
    LOG("Config: Loaded %d history entries", g_app.history_count);
    return 0;
}

/* Save watch history */
int config_save_history(void)
{
    FILE *f = fopen(HISTORY_PATH, "wb");
    if (!f) {
        LOG_ERROR("Config: Cannot write history");
        return -1;
    }

    fwrite(HISTORY_MAGIC, 1, 8, f);

    uint32_t count = g_app.history_count;
    fwrite(&count, sizeof(uint32_t), 1, f);

    for (int i = 0; i < g_app.history_count; i++) {
        fwrite(&g_app.history[i], sizeof(history_entry_t), 1, f);
    }

    fclose(f);
    LOG("Config: Saved %d history entries", g_app.history_count);
    return 0;
}

/* Add or update history entry */
int config_add_history(const char *id, const char *title, uint32_t position,
                       uint32_t duration, content_type_t type)
{
    if (!id || id[0] == '\0') return -1;

    int existing = -1;
    for (int i = 0; i < g_app.history_count; i++) {
        if (strcmp(g_app.history[i].id, id) == 0) {
            existing = i;
            break;
        }
    }

    history_entry_t *entry;

    if (existing >= 0) {
        history_entry_t temp = g_app.history[existing];
        for (int i = existing; i > 0; i--) {
            g_app.history[i] = g_app.history[i - 1];
        }
        g_app.history[0] = temp;
        entry = &g_app.history[0];
    } else {
        if (g_app.history_count >= MAX_HISTORY) {
            g_app.history_count = MAX_HISTORY - 1;
        }

        for (int i = g_app.history_count; i > 0; i--) {
            g_app.history[i] = g_app.history[i - 1];
        }

        entry = &g_app.history[0];
        memset(entry, 0, sizeof(history_entry_t));
        g_app.history_count++;
    }

    strncpy(entry->id, id, 63);
    entry->id[63] = '\0';

    if (title) {
        strncpy(entry->title, title, MAX_TITLE_LENGTH - 1);
        entry->title[MAX_TITLE_LENGTH - 1] = '\0';
    }

    entry->position_ms = position;
    entry->duration_ms = duration;
    entry->type = type;
    entry->timestamp = 0;

    return config_save_history();
}

/* Get resume position for an item */
uint32_t config_get_resume_position(const char *item_id)
{
    if (!item_id) return 0;

    for (int i = 0; i < g_app.history_count; i++) {
        if (strcmp(g_app.history[i].id, item_id) == 0) {
            if (g_app.history[i].duration_ms > 0) {
                uint32_t threshold = g_app.history[i].duration_ms * 95 / 100;
                if (g_app.history[i].position_ms < threshold) {
                    return g_app.history[i].position_ms;
                }
            }
            return 0;
        }
    }
    return 0;
}

/* Clear watch history */
void config_clear_history(void)
{
    g_app.history_count = 0;
    memset(g_app.history, 0, sizeof(g_app.history));
    config_save_history();
    LOG("Config: History cleared");
}
