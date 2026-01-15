/*
 * Nedflix for Nintendo GameCube
 * Filesystem access using libfat
 *
 * TECHNICAL DEMO / NOVELTY PORT
 *
 * Supports reading from:
 *   - SD card via SD Gecko adapter (memory card slot)
 *   - SD card via SD Media Launcher
 */

#include "nedflix.h"
#include <dirent.h>
#include <sys/stat.h>

/* Filesystem state */
static bool g_fs_initialized = false;

/* Audio file extensions we support */
static const char *audio_extensions[] = {
    ".wav", ".WAV",
    ".pcm", ".PCM",
    NULL
};

/*
 * Initialize filesystem
 */
int fs_init(void)
{
    if (g_fs_initialized) {
        return 0;
    }

    /* Initialize libfat for SD card access */
    if (!fatInitDefault()) {
        LOG_ERROR("Failed to initialize FAT filesystem");
        LOG_ERROR("Please insert SD card with media files");
        return -1;
    }

    g_fs_initialized = true;
    LOG("Filesystem initialized");

    /* Create default directories if they don't exist */
    mkdir("/nedflix", 0755);
    mkdir("/nedflix/music", 0755);
    mkdir("/nedflix/audiobooks", 0755);
    mkdir("/nedflix/config", 0755);

    return 0;
}

/*
 * Shutdown filesystem
 */
void fs_shutdown(void)
{
    /* libfat doesn't need explicit shutdown */
    g_fs_initialized = false;
}

/*
 * Check if filename has an audio extension
 */
bool fs_is_audio_file(const char *filename)
{
    if (!filename) return false;

    const char *ext = strrchr(filename, '.');
    if (!ext) return false;

    for (int i = 0; audio_extensions[i] != NULL; i++) {
        if (strcmp(ext, audio_extensions[i]) == 0) {
            return true;
        }
    }

    return false;
}

/*
 * Check if file exists
 */
bool fs_file_exists(const char *path)
{
    if (!path) return false;

    struct stat st;
    return (stat(path, &st) == 0);
}

/*
 * Compare function for sorting directory entries
 */
static int compare_media_items(const void *a, const void *b)
{
    const media_item_t *item_a = (const media_item_t *)a;
    const media_item_t *item_b = (const media_item_t *)b;

    /* Directories first */
    if (item_a->is_directory && !item_b->is_directory) return -1;
    if (!item_a->is_directory && item_b->is_directory) return 1;

    /* Then alphabetically */
    return strcasecmp(item_a->name, item_b->name);
}

/*
 * List directory contents
 */
int fs_list_directory(const char *path, media_list_t *list)
{
    if (!path || !list) {
        return -1;
    }

    /* Clear existing list */
    list->count = 0;
    list->selected_index = 0;
    list->scroll_offset = 0;

    if (!g_fs_initialized) {
        LOG_ERROR("Filesystem not initialized");
        return -1;
    }

    /* Open directory */
    DIR *dir = opendir(path);
    if (!dir) {
        LOG_ERROR("Failed to open directory: %s", path);
        return -1;
    }

    /* Read directory entries */
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && list->count < list->capacity) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path */
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        /* Get file info */
        struct stat st;
        if (stat(full_path, &st) != 0) {
            continue;
        }

        bool is_dir = S_ISDIR(st.st_mode);

        /* Filter: only directories and audio files */
        if (!is_dir && !fs_is_audio_file(entry->d_name)) {
            continue;
        }

        /* Add to list */
        media_item_t *item = &list->items[list->count];

        strncpy(item->name, entry->d_name, sizeof(item->name) - 1);
        item->name[sizeof(item->name) - 1] = '\0';

        strncpy(item->path, full_path, sizeof(item->path) - 1);
        item->path[sizeof(item->path) - 1] = '\0';

        item->is_directory = is_dir;
        item->type = is_dir ? MEDIA_TYPE_DIRECTORY : MEDIA_TYPE_AUDIO;
        item->size = (uint32_t)st.st_size;

        list->count++;
    }

    closedir(dir);

    /* Sort the list */
    if (list->count > 0) {
        qsort(list->items, list->count, sizeof(media_item_t), compare_media_items);
    }

    /* Update current path */
    strncpy(list->current_path, path, sizeof(list->current_path) - 1);
    list->current_path[sizeof(list->current_path) - 1] = '\0';

    LOG("Listed %d items in %s", list->count, path);

    return 0;
}
