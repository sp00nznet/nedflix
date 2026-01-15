/*
 * Nedflix PS3 - Configuration management
 * Saves to PS3 HDD
 */

#include "nedflix.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define CONFIG_PATH "/dev_hdd0/game/NEDFLIX01/USRDIR/nedflix.cfg"
#define CONFIG_DIR  "/dev_hdd0/game/NEDFLIX01/USRDIR"

/* Set default configuration */
void config_defaults(user_settings_t *s)
{
    memset(s, 0, sizeof(user_settings_t));
    s->volume = 80;
    s->library = LIBRARY_MUSIC;
    s->autoplay = true;
    s->show_subtitles = true;
    s->video_quality = 1;  /* HD */
    strcpy(s->subtitle_language, "en");
    strcpy(s->audio_language, "en");
    s->enable_surround = false;
    s->enable_hdr = false;
}

/* Load configuration from file */
int config_load(user_settings_t *s)
{
    FILE *f = fopen(CONFIG_PATH, "rb");
    if (!f) {
        printf("Config: No saved config, using defaults\n");
        return -1;
    }

    /* Read magic header */
    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "NEDFLX01", 8) != 0) {
        printf("Config: Invalid config file\n");
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

    printf("Config: Loaded from %s\n", CONFIG_PATH);
    return 0;
}

/* Save configuration to file */
int config_save(const user_settings_t *s)
{
    /* Create directory if needed */
    mkdir(CONFIG_DIR, 0755);

    FILE *f = fopen(CONFIG_PATH, "wb");
    if (!f) {
        printf("Config: Cannot write to %s\n", CONFIG_PATH);
        return -1;
    }

    /* Write magic header */
    fwrite("NEDFLX01", 1, 8, f);

    /* Write settings */
    fwrite(s, 1, sizeof(user_settings_t), f);
    fclose(f);

    printf("Config: Saved to %s\n", CONFIG_PATH);
    return 0;
}
