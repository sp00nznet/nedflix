/*
 * Nedflix PS3 - Configuration management
 * Full implementation with favorites and watch history
 */

#include "nedflix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>

/* File paths */
#define CONFIG_DIR       "/dev_hdd0/game/NEDFLIX01/USRDIR"
#define CONFIG_PATH      CONFIG_DIR "/nedflix.cfg"
#define FAVORITES_PATH   CONFIG_DIR "/favorites.dat"
#define HISTORY_PATH     CONFIG_DIR "/history.dat"

/* File magic headers */
#define CONFIG_MAGIC     "NEDFLX02"
#define FAVORITES_MAGIC  "NEDFAV01"
#define HISTORY_MAGIC    "NEDHIS01"

/* History configuration */
#define MAX_HISTORY_ITEMS  100

/* Watch history entry */
typedef struct {
    char item_id[64];
    char title[128];
    u32 position_ms;
    u32 duration_ms;
    u64 timestamp;
    u8 content_type;  /* 0=movie, 1=episode, 2=music, 3=channel */
    char series_id[64];
    u16 season;
    u16 episode;
} history_entry_t;

/* Static storage for history */
static history_entry_t watch_history[MAX_HISTORY_ITEMS];
static int history_count = 0;

/* Static storage for favorites */
static char favorite_ids[MAX_FAVORITES][64];
static int favorites_count = 0;

/* Ensure config directory exists */
static int ensure_config_dir(void)
{
    struct stat st;
    if (stat(CONFIG_DIR, &st) != 0) {
        /* Create directory hierarchy */
        mkdir("/dev_hdd0/game", 0755);
        mkdir("/dev_hdd0/game/NEDFLIX01", 0755);
        mkdir(CONFIG_DIR, 0755);

        if (stat(CONFIG_DIR, &st) != 0) {
            printf("Config: Failed to create directory %s\n", CONFIG_DIR);
            return -1;
        }
    }
    return 0;
}

/* Set default configuration */
void config_defaults(user_settings_t *s)
{
    memset(s, 0, sizeof(user_settings_t));

    /* Audio settings */
    s->volume = 80;
    s->enable_surround = false;
    s->audio_output = 0;  /* Auto-detect */
    strcpy(s->audio_language, "en");

    /* Video settings */
    s->video_quality = 2;  /* HD 720p */
    s->enable_hdr = false;
    s->aspect_ratio = 0;   /* Auto */
    s->overscan = 0;       /* No overscan compensation */

    /* Subtitle settings */
    s->show_subtitles = true;
    strcpy(s->subtitle_language, "en");
    s->subtitle_size = 1;    /* Medium */
    s->subtitle_color = 0;   /* White */
    s->subtitle_bg = 1;      /* Semi-transparent */

    /* Playback settings */
    s->autoplay = true;
    s->skip_intro = false;
    s->skip_credits = false;
    s->continue_watching = true;

    /* UI settings */
    s->library = LIBRARY_MOVIES;
    s->theme = 0;            /* Default theme */
    s->animations = true;
    s->show_clock = true;

    /* Network settings */
    s->buffer_size = 2;      /* Medium (8MB) */
    s->auto_quality = true;

    /* Parental controls */
    s->parental_enabled = false;
    s->max_rating = 4;       /* All content */
    memset(s->parental_pin, 0, sizeof(s->parental_pin));

    /* Account */
    s->remember_login = true;
    s->active_profile = 0;
}

/* Load configuration from file */
int config_load(user_settings_t *s)
{
    config_defaults(s);

    FILE *f = fopen(CONFIG_PATH, "rb");
    if (!f) {
        printf("Config: No saved config, using defaults\n");
        return -1;
    }

    /* Read and verify magic header */
    char magic[8];
    if (fread(magic, 1, 8, f) != 8) {
        fclose(f);
        return -1;
    }

    /* Check for current or legacy format */
    if (memcmp(magic, CONFIG_MAGIC, 8) != 0) {
        if (memcmp(magic, "NEDFLX01", 8) == 0) {
            /* Legacy format - read what we can */
            printf("Config: Upgrading from legacy format\n");
            fseek(f, 8, SEEK_SET);
            /* Read basic settings that match old format */
            fread(&s->volume, 1, sizeof(u8), f);
            fread(&s->library, 1, sizeof(u8), f);
            fread(&s->autoplay, 1, sizeof(bool), f);
            fread(&s->show_subtitles, 1, sizeof(bool), f);
            fread(&s->video_quality, 1, sizeof(u8), f);
            fread(s->subtitle_language, 1, 8, f);
            fread(s->audio_language, 1, 8, f);
            fread(&s->enable_surround, 1, sizeof(bool), f);
            fread(&s->enable_hdr, 1, sizeof(bool), f);
            fclose(f);
            /* Save in new format */
            config_save(s);
            return 0;
        }
        printf("Config: Invalid config file format\n");
        fclose(f);
        return -1;
    }

    /* Read version */
    u32 version;
    if (fread(&version, 1, sizeof(u32), f) != sizeof(u32)) {
        fclose(f);
        return -1;
    }

    /* Read settings structure */
    size_t read = fread(s, 1, sizeof(user_settings_t), f);
    fclose(f);

    if (read != sizeof(user_settings_t)) {
        printf("Config: Partial read (%zu/%zu), using defaults\n",
               read, sizeof(user_settings_t));
        config_defaults(s);
        return -1;
    }

    /* Validate ranges */
    if (s->volume > 100) s->volume = 100;
    if (s->video_quality > 4) s->video_quality = 2;
    if (s->library >= LIBRARY_COUNT) s->library = LIBRARY_MOVIES;

    printf("Config: Loaded from %s\n", CONFIG_PATH);
    return 0;
}

/* Save configuration to file */
int config_save(const user_settings_t *s)
{
    if (ensure_config_dir() != 0) {
        return -1;
    }

    FILE *f = fopen(CONFIG_PATH, "wb");
    if (!f) {
        printf("Config: Cannot write to %s\n", CONFIG_PATH);
        return -1;
    }

    /* Write magic header */
    fwrite(CONFIG_MAGIC, 1, 8, f);

    /* Write version */
    u32 version = 2;
    fwrite(&version, 1, sizeof(u32), f);

    /* Write settings */
    fwrite(s, 1, sizeof(user_settings_t), f);
    fclose(f);

    printf("Config: Saved to %s\n", CONFIG_PATH);
    return 0;
}

/* Load favorites list */
int config_load_favorites(void)
{
    favorites_count = 0;
    memset(favorite_ids, 0, sizeof(favorite_ids));

    FILE *f = fopen(FAVORITES_PATH, "rb");
    if (!f) {
        return 0;  /* No favorites yet */
    }

    /* Read and verify magic */
    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, FAVORITES_MAGIC, 8) != 0) {
        fclose(f);
        return -1;
    }

    /* Read count */
    u32 count;
    if (fread(&count, 1, sizeof(u32), f) != sizeof(u32)) {
        fclose(f);
        return -1;
    }

    if (count > MAX_FAVORITES) count = MAX_FAVORITES;

    /* Read favorite IDs */
    for (u32 i = 0; i < count; i++) {
        if (fread(favorite_ids[i], 1, 64, f) != 64) break;
        favorites_count++;
    }

    fclose(f);
    printf("Config: Loaded %d favorites\n", favorites_count);
    return 0;
}

/* Save favorites list */
int config_save_favorites(void)
{
    if (ensure_config_dir() != 0) {
        return -1;
    }

    FILE *f = fopen(FAVORITES_PATH, "wb");
    if (!f) {
        printf("Config: Cannot write favorites\n");
        return -1;
    }

    /* Write magic */
    fwrite(FAVORITES_MAGIC, 1, 8, f);

    /* Write count */
    u32 count = favorites_count;
    fwrite(&count, 1, sizeof(u32), f);

    /* Write favorite IDs */
    for (int i = 0; i < favorites_count; i++) {
        fwrite(favorite_ids[i], 1, 64, f);
    }

    fclose(f);
    printf("Config: Saved %d favorites\n", favorites_count);
    return 0;
}

/* Add to favorites */
int config_add_favorite(const char *item_id)
{
    if (!item_id || item_id[0] == '\0') return -1;

    /* Check if already favorited */
    for (int i = 0; i < favorites_count; i++) {
        if (strcmp(favorite_ids[i], item_id) == 0) {
            return 0;  /* Already exists */
        }
    }

    /* Check capacity */
    if (favorites_count >= MAX_FAVORITES) {
        printf("Config: Favorites list full\n");
        return -1;
    }

    /* Add new favorite */
    strncpy(favorite_ids[favorites_count], item_id, 63);
    favorite_ids[favorites_count][63] = '\0';
    favorites_count++;

    return config_save_favorites();
}

/* Remove from favorites */
int config_remove_favorite(const char *item_id)
{
    if (!item_id) return -1;

    for (int i = 0; i < favorites_count; i++) {
        if (strcmp(favorite_ids[i], item_id) == 0) {
            /* Shift remaining items */
            for (int j = i; j < favorites_count - 1; j++) {
                strcpy(favorite_ids[j], favorite_ids[j + 1]);
            }
            favorites_count--;
            return config_save_favorites();
        }
    }

    return 0;  /* Not found, but not an error */
}

/* Check if item is favorited */
bool config_is_favorite(const char *item_id)
{
    if (!item_id) return false;

    for (int i = 0; i < favorites_count; i++) {
        if (strcmp(favorite_ids[i], item_id) == 0) {
            return true;
        }
    }
    return false;
}

/* Get favorites count */
int config_get_favorites_count(void)
{
    return favorites_count;
}

/* Get favorite ID by index */
const char *config_get_favorite_id(int index)
{
    if (index < 0 || index >= favorites_count) return NULL;
    return favorite_ids[index];
}

/* Load watch history */
int config_load_history(void)
{
    history_count = 0;
    memset(watch_history, 0, sizeof(watch_history));

    FILE *f = fopen(HISTORY_PATH, "rb");
    if (!f) {
        return 0;  /* No history yet */
    }

    /* Read and verify magic */
    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, HISTORY_MAGIC, 8) != 0) {
        fclose(f);
        return -1;
    }

    /* Read count */
    u32 count;
    if (fread(&count, 1, sizeof(u32), f) != sizeof(u32)) {
        fclose(f);
        return -1;
    }

    if (count > MAX_HISTORY_ITEMS) count = MAX_HISTORY_ITEMS;

    /* Read history entries */
    for (u32 i = 0; i < count; i++) {
        if (fread(&watch_history[i], 1, sizeof(history_entry_t), f) != sizeof(history_entry_t)) {
            break;
        }
        history_count++;
    }

    fclose(f);
    printf("Config: Loaded %d history entries\n", history_count);
    return 0;
}

/* Save watch history */
int config_save_history(void)
{
    if (ensure_config_dir() != 0) {
        return -1;
    }

    FILE *f = fopen(HISTORY_PATH, "wb");
    if (!f) {
        printf("Config: Cannot write history\n");
        return -1;
    }

    /* Write magic */
    fwrite(HISTORY_MAGIC, 1, 8, f);

    /* Write count */
    u32 count = history_count;
    fwrite(&count, 1, sizeof(u32), f);

    /* Write history entries */
    for (int i = 0; i < history_count; i++) {
        fwrite(&watch_history[i], 1, sizeof(history_entry_t), f);
    }

    fclose(f);
    printf("Config: Saved %d history entries\n", history_count);
    return 0;
}

/* Add or update watch history entry */
int config_add_history(const char *item_id, const char *title,
                       u32 position_ms, u32 duration_ms, u8 content_type)
{
    if (!item_id || item_id[0] == '\0') return -1;

    /* Look for existing entry */
    int existing = -1;
    for (int i = 0; i < history_count; i++) {
        if (strcmp(watch_history[i].item_id, item_id) == 0) {
            existing = i;
            break;
        }
    }

    history_entry_t *entry;

    if (existing >= 0) {
        /* Update existing entry - move to front */
        history_entry_t temp = watch_history[existing];
        for (int i = existing; i > 0; i--) {
            watch_history[i] = watch_history[i - 1];
        }
        watch_history[0] = temp;
        entry = &watch_history[0];
    } else {
        /* Add new entry at front */
        if (history_count >= MAX_HISTORY_ITEMS) {
            /* Remove oldest */
            history_count = MAX_HISTORY_ITEMS - 1;
        }

        /* Shift everything down */
        for (int i = history_count; i > 0; i--) {
            watch_history[i] = watch_history[i - 1];
        }

        entry = &watch_history[0];
        memset(entry, 0, sizeof(history_entry_t));
        history_count++;
    }

    /* Update entry */
    strncpy(entry->item_id, item_id, 63);
    entry->item_id[63] = '\0';

    if (title) {
        strncpy(entry->title, title, 127);
        entry->title[127] = '\0';
    }

    entry->position_ms = position_ms;
    entry->duration_ms = duration_ms;
    entry->content_type = content_type;
    entry->timestamp = 0;  /* Would use sys_time_get_system_time() / 1000000 */

    return config_save_history();
}

/* Get resume position for an item */
u32 config_get_resume_position(const char *item_id)
{
    if (!item_id) return 0;

    for (int i = 0; i < history_count; i++) {
        if (strcmp(watch_history[i].item_id, item_id) == 0) {
            /* Only return position if not near the end (95%) */
            if (watch_history[i].duration_ms > 0) {
                u32 threshold = watch_history[i].duration_ms * 95 / 100;
                if (watch_history[i].position_ms < threshold) {
                    return watch_history[i].position_ms;
                }
            }
            return 0;  /* Finished watching */
        }
    }
    return 0;  /* Not in history */
}

/* Clear watch history */
void config_clear_history(void)
{
    history_count = 0;
    memset(watch_history, 0, sizeof(watch_history));
    config_save_history();
    printf("Config: History cleared\n");
}

/* Get history count */
int config_get_history_count(void)
{
    return history_count;
}

/* Get history entry by index (0 = most recent) */
int config_get_history_entry(int index, char *item_id, char *title,
                             u32 *position_ms, u32 *duration_ms)
{
    if (index < 0 || index >= history_count) return -1;

    history_entry_t *entry = &watch_history[index];

    if (item_id) strcpy(item_id, entry->item_id);
    if (title) strcpy(title, entry->title);
    if (position_ms) *position_ms = entry->position_ms;
    if (duration_ms) *duration_ms = entry->duration_ms;

    return 0;
}

/* Clear all saved data */
void config_clear_all(void)
{
    remove(CONFIG_PATH);
    remove(FAVORITES_PATH);
    remove(HISTORY_PATH);
    printf("Config: All data cleared\n");
}
