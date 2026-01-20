/*
 * Nedflix Nintendo Switch - Configuration management
 * Save data support with favorites and watch history
 */

#include "nedflix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <switch.h>

/* Save data paths */
#define SAVE_DIR         "save:/"
#define CONFIG_PATH      SAVE_DIR "nedflix.cfg"
#define FAVORITES_PATH   SAVE_DIR "favorites.dat"
#define HISTORY_PATH     SAVE_DIR "history.dat"

/* File magic headers */
#define CONFIG_MAGIC     "NDSW0002"
#define FAVORITES_MAGIC  "NDSF0001"
#define HISTORY_MAGIC    "NDSH0001"

/* History configuration */
#define MAX_HISTORY_ITEMS  100

/* Watch history entry */
typedef struct {
    char item_id[64];
    char title[128];
    uint32_t position_ms;
    uint32_t duration_ms;
    uint64_t timestamp;
    uint8_t content_type;
    char series_id[64];
    uint16_t season;
    uint16_t episode;
} history_entry_t;

/* Static storage */
static history_entry_t watch_history[MAX_HISTORY_ITEMS];
static int history_count = 0;
static char favorite_ids[MAX_FAVORITES][64];
static int favorites_count = 0;

/* Save data account */
static AccountUid account_uid;
static bool save_mounted = false;

/* Mount save data */
static int mount_save(void)
{
    if (save_mounted) return 0;

    Result rc;

    /* Get active user */
    rc = accountInitialize(AccountServiceType_Application);
    if (R_FAILED(rc)) {
        printf("accountInitialize failed: 0x%x\n", rc);
        return -1;
    }

    bool selected = false;
    rc = accountTrySelectUserWithoutInteraction(&account_uid, &selected);
    if (R_FAILED(rc) || !selected) {
        /* Show account selector */
        PselUserSelectionSettings settings;
        pselUserSelectionSettingsInit(&settings);

        rc = pselShowUserSelector(&account_uid, &settings);
        if (R_FAILED(rc)) {
            printf("User selection failed: 0x%x\n", rc);
            accountExit();
            return -1;
        }
    }

    /* Create save data if it doesn't exist */
    FsSaveDataAttribute attr;
    memset(&attr, 0, sizeof(attr));
    attr.application_id = 0;
    attr.uid = account_uid;
    attr.system_save_data_id = 0;
    attr.save_data_type = FsSaveDataType_Account;

    /* Try to mount existing save */
    rc = fsdevMountSaveData("save", 0, account_uid);
    if (R_FAILED(rc)) {
        /* Create new save data */
        s64 save_size = 0x100000;  /* 1MB */
        s64 journal_size = 0x100000;

        rc = fsCreate_SaveData(0, account_uid, save_size, journal_size, 0);
        if (R_FAILED(rc)) {
            printf("Failed to create save data: 0x%x\n", rc);
            accountExit();
            return -1;
        }

        /* Mount newly created save */
        rc = fsdevMountSaveData("save", 0, account_uid);
        if (R_FAILED(rc)) {
            printf("Failed to mount save data: 0x%x\n", rc);
            accountExit();
            return -1;
        }
    }

    save_mounted = true;
    printf("Save data mounted\n");
    return 0;
}

/* Commit save data changes */
static void commit_save(void)
{
    if (!save_mounted) return;

    Result rc = fsdevCommitDevice("save");
    if (R_FAILED(rc)) {
        printf("Warning: Failed to commit save: 0x%x\n", rc);
    }
}

/* Set default configuration */
void config_defaults(user_settings_t *s)
{
    memset(s, 0, sizeof(user_settings_t));

    /* Audio settings */
    s->volume = 80;
    s->enable_surround = false;
    s->audio_output = 0;
    strcpy(s->audio_language, "en");

    /* Video settings */
    s->video_quality = 3;  /* FHD */
    s->enable_hdr = false;
    s->aspect_ratio = 0;
    s->overscan = 0;

    /* Subtitle settings */
    s->show_subtitles = true;
    strcpy(s->subtitle_language, "en");
    s->subtitle_size = 1;
    s->subtitle_color = 0;
    s->subtitle_bg = 1;

    /* Playback settings */
    s->autoplay = true;
    s->skip_intro = false;
    s->skip_credits = false;
    s->continue_watching = true;

    /* UI settings */
    s->library = LIBRARY_MOVIES;
    s->theme = 0;
    s->animations = true;
    s->show_clock = true;

    /* Network settings */
    s->buffer_size = 2;
    s->auto_quality = true;

    /* Parental controls */
    s->parental_enabled = false;
    s->max_rating = 4;

    /* Account */
    s->remember_login = true;
    s->active_profile = 0;
}

/* Load configuration */
int config_load(user_settings_t *s)
{
    config_defaults(s);

    if (mount_save() != 0) {
        return -1;
    }

    FILE *f = fopen(CONFIG_PATH, "rb");
    if (!f) {
        printf("Config: No saved config, using defaults\n");
        return -1;
    }

    /* Read and verify magic */
    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, CONFIG_MAGIC, 8) != 0) {
        printf("Config: Invalid format\n");
        fclose(f);
        return -1;
    }

    /* Read version */
    uint32_t version;
    if (fread(&version, 1, sizeof(uint32_t), f) != sizeof(uint32_t)) {
        fclose(f);
        return -1;
    }

    /* Read settings */
    size_t read = fread(s, 1, sizeof(user_settings_t), f);
    fclose(f);

    if (read != sizeof(user_settings_t)) {
        printf("Config: Partial read, using defaults\n");
        config_defaults(s);
        return -1;
    }

    /* Validate */
    if (s->volume > 100) s->volume = 100;
    if (s->video_quality > 4) s->video_quality = 3;
    if (s->library >= LIBRARY_COUNT) s->library = LIBRARY_MOVIES;

    printf("Config: Loaded successfully\n");
    return 0;
}

/* Save configuration */
int config_save(const user_settings_t *s)
{
    if (mount_save() != 0) {
        return -1;
    }

    FILE *f = fopen(CONFIG_PATH, "wb");
    if (!f) {
        printf("Config: Cannot write\n");
        return -1;
    }

    /* Write magic */
    fwrite(CONFIG_MAGIC, 1, 8, f);

    /* Write version */
    uint32_t version = 2;
    fwrite(&version, 1, sizeof(uint32_t), f);

    /* Write settings */
    fwrite(s, 1, sizeof(user_settings_t), f);
    fclose(f);

    commit_save();
    printf("Config: Saved\n");
    return 0;
}

/* Load favorites */
int config_load_favorites(void)
{
    favorites_count = 0;
    memset(favorite_ids, 0, sizeof(favorite_ids));

    if (mount_save() != 0) {
        return -1;
    }

    FILE *f = fopen(FAVORITES_PATH, "rb");
    if (!f) {
        return 0;
    }

    /* Verify magic */
    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, FAVORITES_MAGIC, 8) != 0) {
        fclose(f);
        return -1;
    }

    /* Read count */
    uint32_t count;
    if (fread(&count, 1, sizeof(uint32_t), f) != sizeof(uint32_t)) {
        fclose(f);
        return -1;
    }

    if (count > MAX_FAVORITES) count = MAX_FAVORITES;

    /* Read favorites */
    for (uint32_t i = 0; i < count; i++) {
        if (fread(favorite_ids[i], 1, 64, f) != 64) break;
        favorites_count++;
    }

    fclose(f);
    printf("Config: Loaded %d favorites\n", favorites_count);
    return 0;
}

/* Save favorites */
int config_save_favorites(void)
{
    if (mount_save() != 0) {
        return -1;
    }

    FILE *f = fopen(FAVORITES_PATH, "wb");
    if (!f) {
        return -1;
    }

    fwrite(FAVORITES_MAGIC, 1, 8, f);

    uint32_t count = favorites_count;
    fwrite(&count, 1, sizeof(uint32_t), f);

    for (int i = 0; i < favorites_count; i++) {
        fwrite(favorite_ids[i], 1, 64, f);
    }

    fclose(f);
    commit_save();
    return 0;
}

/* Add favorite */
int config_add_favorite(const char *item_id)
{
    if (!item_id || !item_id[0]) return -1;

    /* Check if exists */
    for (int i = 0; i < favorites_count; i++) {
        if (strcmp(favorite_ids[i], item_id) == 0) {
            return 0;
        }
    }

    if (favorites_count >= MAX_FAVORITES) {
        return -1;
    }

    strncpy(favorite_ids[favorites_count], item_id, 63);
    favorite_ids[favorites_count][63] = '\0';
    favorites_count++;

    return config_save_favorites();
}

/* Remove favorite */
int config_remove_favorite(const char *item_id)
{
    if (!item_id) return -1;

    for (int i = 0; i < favorites_count; i++) {
        if (strcmp(favorite_ids[i], item_id) == 0) {
            for (int j = i; j < favorites_count - 1; j++) {
                strcpy(favorite_ids[j], favorite_ids[j + 1]);
            }
            favorites_count--;
            return config_save_favorites();
        }
    }

    return 0;
}

/* Check favorite */
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

/* Get favorite by index */
const char *config_get_favorite_id(int index)
{
    if (index < 0 || index >= favorites_count) return NULL;
    return favorite_ids[index];
}

/* Load history */
int config_load_history(void)
{
    history_count = 0;
    memset(watch_history, 0, sizeof(watch_history));

    if (mount_save() != 0) {
        return -1;
    }

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
    if (fread(&count, 1, sizeof(uint32_t), f) != sizeof(uint32_t)) {
        fclose(f);
        return -1;
    }

    if (count > MAX_HISTORY_ITEMS) count = MAX_HISTORY_ITEMS;

    for (uint32_t i = 0; i < count; i++) {
        if (fread(&watch_history[i], 1, sizeof(history_entry_t), f) != sizeof(history_entry_t)) {
            break;
        }
        history_count++;
    }

    fclose(f);
    printf("Config: Loaded %d history entries\n", history_count);
    return 0;
}

/* Save history */
int config_save_history(void)
{
    if (mount_save() != 0) {
        return -1;
    }

    FILE *f = fopen(HISTORY_PATH, "wb");
    if (!f) {
        return -1;
    }

    fwrite(HISTORY_MAGIC, 1, 8, f);

    uint32_t count = history_count;
    fwrite(&count, 1, sizeof(uint32_t), f);

    for (int i = 0; i < history_count; i++) {
        fwrite(&watch_history[i], 1, sizeof(history_entry_t), f);
    }

    fclose(f);
    commit_save();
    return 0;
}

/* Add history entry */
int config_add_history(const char *item_id, const char *title,
                       uint32_t position_ms, uint32_t duration_ms, uint8_t content_type)
{
    if (!item_id || !item_id[0]) return -1;

    /* Find existing */
    int existing = -1;
    for (int i = 0; i < history_count; i++) {
        if (strcmp(watch_history[i].item_id, item_id) == 0) {
            existing = i;
            break;
        }
    }

    history_entry_t *entry;

    if (existing >= 0) {
        /* Move to front */
        history_entry_t temp = watch_history[existing];
        for (int i = existing; i > 0; i--) {
            watch_history[i] = watch_history[i - 1];
        }
        watch_history[0] = temp;
        entry = &watch_history[0];
    } else {
        /* Add new */
        if (history_count >= MAX_HISTORY_ITEMS) {
            history_count = MAX_HISTORY_ITEMS - 1;
        }

        for (int i = history_count; i > 0; i--) {
            watch_history[i] = watch_history[i - 1];
        }

        entry = &watch_history[0];
        memset(entry, 0, sizeof(history_entry_t));
        history_count++;
    }

    strncpy(entry->item_id, item_id, 63);
    if (title) strncpy(entry->title, title, 127);
    entry->position_ms = position_ms;
    entry->duration_ms = duration_ms;
    entry->content_type = content_type;
    entry->timestamp = armGetSystemTick();

    return config_save_history();
}

/* Get resume position */
uint32_t config_get_resume_position(const char *item_id)
{
    if (!item_id) return 0;

    for (int i = 0; i < history_count; i++) {
        if (strcmp(watch_history[i].item_id, item_id) == 0) {
            if (watch_history[i].duration_ms > 0) {
                uint32_t threshold = watch_history[i].duration_ms * 95 / 100;
                if (watch_history[i].position_ms < threshold) {
                    return watch_history[i].position_ms;
                }
            }
            return 0;
        }
    }
    return 0;
}

/* Clear history */
void config_clear_history(void)
{
    history_count = 0;
    memset(watch_history, 0, sizeof(watch_history));
    config_save_history();
}

/* Get history count */
int config_get_history_count(void)
{
    return history_count;
}

/* Get history entry */
int config_get_history_entry(int index, char *item_id, char *title,
                             uint32_t *position_ms, uint32_t *duration_ms)
{
    if (index < 0 || index >= history_count) return -1;

    history_entry_t *entry = &watch_history[index];

    if (item_id) strcpy(item_id, entry->item_id);
    if (title) strcpy(title, entry->title);
    if (position_ms) *position_ms = entry->position_ms;
    if (duration_ms) *duration_ms = entry->duration_ms;

    return 0;
}

/* Clear all data */
void config_clear_all(void)
{
    if (save_mounted) {
        remove(CONFIG_PATH);
        remove(FAVORITES_PATH);
        remove(HISTORY_PATH);
        commit_save();
    }
}
