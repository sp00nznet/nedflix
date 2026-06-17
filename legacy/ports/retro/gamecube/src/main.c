/*
 * Nedflix for Nintendo GameCube
 * Main entry point
 *
 * TECHNICAL DEMO / NOVELTY PORT
 * Demonstrates GameCube homebrew with audio playback focus.
 */

#include "nedflix.h"

/* Global application context */
app_context_t g_app;

/* Library names for display */
static const char *library_names[] = {
    "Music",
    "Audiobooks"
};

/* Default paths on SD card */
static const char *library_paths[] = {
    "/nedflix/music",
    "/nedflix/audiobooks"
};

/*
 * Forward declarations for state handlers
 */
static void handle_state_init(void);
static void handle_state_browsing(void);
static void handle_state_playing(void);
static void handle_state_settings(void);
static void handle_state_error(void);

/*
 * Initialize the application
 */
void app_init(void)
{
    /* Initialize GameCube hardware */
    VIDEO_Init();
    PAD_Init();

    /* Get preferred video mode */
    g_app.rmode = VIDEO_GetPreferredMode(NULL);

    /* Allocate framebuffers (32-byte aligned) */
    g_app.framebuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(g_app.rmode));
    g_app.framebuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(g_app.rmode));

    /* Configure video */
    VIDEO_Configure(g_app.rmode);
    VIDEO_SetNextFramebuffer(g_app.framebuffer[0]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    if (g_app.rmode->viTVMode & VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }

    g_app.fb_index = 0;
    g_app.first_frame = true;

    /* Initialize console for debug output */
    console_init(g_app.framebuffer[0], 20, 20, g_app.rmode->fbWidth,
                 g_app.rmode->xfbHeight, g_app.rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    printf("\n\n");
    printf("=================================\n");
    printf("  Nedflix for Nintendo GameCube\n");
    printf("  Version %s\n", NEDFLIX_VERSION_STRING);
    printf("  TECHNICAL DEMO\n");
    printf("=================================\n\n");

    /* Clear context */
    memset(&g_app.state, 0, sizeof(app_state_t));
    g_app.state = STATE_INIT;
    g_app.running = true;
    g_app.current_library = LIBRARY_MUSIC;

    /* Load configuration */
    config_set_defaults(&g_app.settings);

    /* Initialize subsystems */
    printf("Initializing UI...\n");
    if (ui_init() != 0) {
        printf("ERROR: Failed to initialize UI\n");
        g_app.state = STATE_ERROR;
        strncpy(g_app.error_message, "Failed to initialize graphics", sizeof(g_app.error_message) - 1);
        return;
    }

    printf("Initializing input...\n");
    if (input_init() != 0) {
        printf("ERROR: Failed to initialize input\n");
        g_app.state = STATE_ERROR;
        strncpy(g_app.error_message, "Failed to initialize controller", sizeof(g_app.error_message) - 1);
        return;
    }

    printf("Initializing audio...\n");
    if (audio_init() != 0) {
        printf("ERROR: Failed to initialize audio\n");
        g_app.state = STATE_ERROR;
        strncpy(g_app.error_message, "Failed to initialize audio", sizeof(g_app.error_message) - 1);
        return;
    }

    printf("Initializing filesystem...\n");
    if (fs_init() != 0) {
        printf("WARNING: SD card not found\n");
        printf("Please insert an SD card with media files.\n");
        /* Non-fatal - will show error when browsing */
    }

    /* Load configuration from SD */
    config_load(&g_app.settings);

    /* Allocate media list */
    g_app.media_list.capacity = 100;
    g_app.media_list.items = (media_item_t *)malloc(sizeof(media_item_t) * g_app.media_list.capacity);
    if (!g_app.media_list.items) {
        printf("ERROR: Out of memory\n");
        g_app.state = STATE_ERROR;
        strncpy(g_app.error_message, "Out of memory", sizeof(g_app.error_message) - 1);
        return;
    }

    printf("Initialization complete!\n");
    printf("Press START to continue...\n\n");

    /* Wait for user input to continue */
    while (1) {
        PAD_ScanPads();
        if (PAD_ButtonsDown(0) & PAD_BUTTON_START) {
            break;
        }
        VIDEO_WaitVSync();
    }

    /* Start browsing */
    g_app.state = STATE_BROWSING;
    strncpy(g_app.media_list.current_path, library_paths[g_app.current_library],
            sizeof(g_app.media_list.current_path) - 1);
    fs_list_directory(g_app.media_list.current_path, &g_app.media_list);
}

/*
 * Shutdown the application
 */
void app_shutdown(void)
{
    printf("Shutting down...\n");

    /* Stop any playback */
    audio_stop(&g_app.playback);

    /* Free resources */
    if (g_app.media_list.items) {
        free(g_app.media_list.items);
        g_app.media_list.items = NULL;
    }

    /* Shutdown subsystems */
    audio_shutdown();
    fs_shutdown();
    input_shutdown();
    ui_shutdown();

    /* Save configuration */
    config_save(&g_app.settings);

    printf("Shutdown complete\n");
}

/*
 * Main application loop
 */
void app_run(void)
{
    while (g_app.running) {
        /* Update input */
        input_update();

        /* Global controls */
        if (input_button_just_pressed(BTN_B)) {
            if (g_app.state == STATE_PLAYING) {
                audio_stop(&g_app.playback);
                g_app.state = STATE_BROWSING;
            } else if (g_app.state == STATE_BROWSING) {
                /* Go up a directory or show settings */
                char *last_slash = strrchr(g_app.media_list.current_path, '/');
                if (last_slash && last_slash != g_app.media_list.current_path) {
                    *last_slash = '\0';
                    g_app.media_list.selected_index = 0;
                    g_app.media_list.scroll_offset = 0;
                    fs_list_directory(g_app.media_list.current_path, &g_app.media_list);
                } else {
                    g_app.state = STATE_SETTINGS;
                }
            } else if (g_app.state == STATE_SETTINGS) {
                g_app.state = STATE_BROWSING;
            }
        }

        /* Start button to toggle settings */
        if (input_button_just_pressed(BTN_START) && g_app.state != STATE_ERROR) {
            if (g_app.state == STATE_SETTINGS) {
                g_app.state = STATE_BROWSING;
            } else if (g_app.state != STATE_INIT) {
                g_app.state = STATE_SETTINGS;
            }
        }

        /* Begin rendering */
        ui_begin_frame();
        ui_clear(COLOR_DARK_GRAY);

        /* State-specific handling */
        switch (g_app.state) {
            case STATE_INIT:
                handle_state_init();
                break;
            case STATE_BROWSING:
                handle_state_browsing();
                break;
            case STATE_PLAYING:
                handle_state_playing();
                break;
            case STATE_SETTINGS:
                handle_state_settings();
                break;
            case STATE_ERROR:
                handle_state_error();
                break;
        }

        /* End rendering */
        ui_end_frame();

        /* Update audio if playing */
        if (g_app.state == STATE_PLAYING) {
            audio_update();
        }

        /* VSync */
        VIDEO_WaitVSync();
    }
}

/*
 * State handlers
 */

static void handle_state_init(void)
{
    ui_draw_loading("Starting Nedflix...");
}

static void handle_state_browsing(void)
{
    /* Draw header with current library */
    char header[128];
    snprintf(header, sizeof(header), "Nedflix - %s", library_names[g_app.current_library]);
    ui_draw_header(header);

    /* Draw file list */
    ui_draw_file_list(&g_app.media_list);

    /* Navigation */
    if (input_button_just_pressed(BTN_DPAD_UP)) {
        if (g_app.media_list.selected_index > 0) {
            g_app.media_list.selected_index--;
            if (g_app.media_list.selected_index < g_app.media_list.scroll_offset) {
                g_app.media_list.scroll_offset--;
            }
        }
    }
    if (input_button_just_pressed(BTN_DPAD_DOWN)) {
        if (g_app.media_list.selected_index < g_app.media_list.count - 1) {
            g_app.media_list.selected_index++;
            if (g_app.media_list.selected_index >= g_app.media_list.scroll_offset + MAX_ITEMS_PER_PAGE) {
                g_app.media_list.scroll_offset++;
            }
        }
    }

    /* Fast scroll with analog stick */
    int stick_y = input_get_stick_y();
    if (stick_y > 50) {
        g_app.media_list.selected_index = MAX(0, g_app.media_list.selected_index - 3);
        if (g_app.media_list.selected_index < g_app.media_list.scroll_offset) {
            g_app.media_list.scroll_offset = g_app.media_list.selected_index;
        }
    } else if (stick_y < -50) {
        g_app.media_list.selected_index = MIN(g_app.media_list.count - 1, g_app.media_list.selected_index + 3);
        if (g_app.media_list.selected_index >= g_app.media_list.scroll_offset + MAX_ITEMS_PER_PAGE) {
            g_app.media_list.scroll_offset = g_app.media_list.selected_index - MAX_ITEMS_PER_PAGE + 1;
        }
    }

    /* Switch libraries with L/R */
    if (input_button_just_pressed(BTN_L)) {
        g_app.current_library = (g_app.current_library - 1 + LIBRARY_COUNT) % LIBRARY_COUNT;
        g_app.media_list.selected_index = 0;
        g_app.media_list.scroll_offset = 0;
        strncpy(g_app.media_list.current_path, library_paths[g_app.current_library],
                sizeof(g_app.media_list.current_path) - 1);
        fs_list_directory(g_app.media_list.current_path, &g_app.media_list);
    }
    if (input_button_just_pressed(BTN_R)) {
        g_app.current_library = (g_app.current_library + 1) % LIBRARY_COUNT;
        g_app.media_list.selected_index = 0;
        g_app.media_list.scroll_offset = 0;
        strncpy(g_app.media_list.current_path, library_paths[g_app.current_library],
                sizeof(g_app.media_list.current_path) - 1);
        fs_list_directory(g_app.media_list.current_path, &g_app.media_list);
    }

    /* Select item */
    if (input_button_just_pressed(BTN_A) && g_app.media_list.count > 0) {
        media_item_t *item = &g_app.media_list.items[g_app.media_list.selected_index];

        if (item->is_directory) {
            /* Navigate into directory */
            strncpy(g_app.media_list.current_path, item->path,
                    sizeof(g_app.media_list.current_path) - 1);
            g_app.media_list.selected_index = 0;
            g_app.media_list.scroll_offset = 0;
            fs_list_directory(g_app.media_list.current_path, &g_app.media_list);
        } else if (item->type == MEDIA_TYPE_AUDIO) {
            /* Play audio file */
            strncpy(g_app.playback.current_file, item->path,
                    sizeof(g_app.playback.current_file) - 1);
            strncpy(g_app.playback.title, item->name,
                    sizeof(g_app.playback.title) - 1);

            int result = audio_load_wav(item->path, &g_app.playback);
            if (result != 0) {
                /* Try MP3 if WAV failed */
                result = audio_load_mp3(item->path, &g_app.playback);
            }

            if (result == 0 && audio_play(&g_app.playback) == 0) {
                g_app.state = STATE_PLAYING;
            } else {
                strncpy(g_app.error_message, "Failed to play audio file",
                        sizeof(g_app.error_message) - 1);
                /* Stay in browsing, show brief error */
            }
        }
    }

    /* Draw help text */
    ui_draw_text(20, SCREEN_HEIGHT - 30, "A:Select  B:Back  L/R:Library  START:Settings", COLOR_TEXT_DIM);

    /* Show message if no files */
    if (g_app.media_list.count == 0) {
        ui_draw_text_centered(SCREEN_HEIGHT / 2, "No audio files found", COLOR_TEXT_DIM);
        ui_draw_text_centered(SCREEN_HEIGHT / 2 + 20, "Place .wav files in /nedflix/music/", COLOR_TEXT_DIM);
    }
}

static void handle_state_playing(void)
{
    /* Update playback state */
    g_app.playback.is_playing = audio_is_playing();
    g_app.playback.current_time = audio_get_position();
    g_app.playback.volume = g_app.settings.volume;

    /* Draw playback HUD */
    ui_draw_playback_hud(&g_app.playback);

    /* Playback controls */
    if (input_button_just_pressed(BTN_A) || input_button_just_pressed(BTN_X)) {
        /* Play/Pause toggle */
        if (g_app.playback.is_paused) {
            audio_resume(&g_app.playback);
            g_app.playback.is_paused = false;
        } else {
            audio_pause(&g_app.playback);
            g_app.playback.is_paused = true;
        }
    }

    /* Volume with D-pad left/right */
    if (input_button_just_pressed(BTN_DPAD_LEFT)) {
        g_app.settings.volume = MAX(0, g_app.settings.volume - 16);
        audio_set_volume(g_app.settings.volume);
    }
    if (input_button_just_pressed(BTN_DPAD_RIGHT)) {
        g_app.settings.volume = MIN(255, g_app.settings.volume + 16);
        audio_set_volume(g_app.settings.volume);
    }

    /* Check for playback end */
    if (!g_app.playback.is_playing && !g_app.playback.is_paused) {
        /* Playback finished - go to next track if available */
        if (g_app.media_list.selected_index < g_app.media_list.count - 1) {
            g_app.media_list.selected_index++;
            media_item_t *item = &g_app.media_list.items[g_app.media_list.selected_index];

            if (item->type == MEDIA_TYPE_AUDIO && !item->is_directory) {
                strncpy(g_app.playback.current_file, item->path,
                        sizeof(g_app.playback.current_file) - 1);
                strncpy(g_app.playback.title, item->name,
                        sizeof(g_app.playback.title) - 1);

                if (audio_load_wav(item->path, &g_app.playback) == 0 ||
                    audio_load_mp3(item->path, &g_app.playback) == 0) {
                    audio_play(&g_app.playback);
                    return;
                }
            }
        }
        /* No more tracks or failed to load - return to browser */
        g_app.state = STATE_BROWSING;
    }
}

static void handle_state_settings(void)
{
    ui_draw_header("Settings");

    static int selected = 0;

    /* Settings menu items */
    char volume_item[64];
    char shuffle_item[64];
    char repeat_item[64];

    snprintf(volume_item, sizeof(volume_item), "Volume: %d%%", (g_app.settings.volume * 100) / 255);
    snprintf(shuffle_item, sizeof(shuffle_item), "Shuffle: %s", g_app.settings.shuffle ? "On" : "Off");
    snprintf(repeat_item, sizeof(repeat_item), "Repeat: %s", g_app.settings.repeat ? "On" : "Off");

    const char *menu_items[] = {
        volume_item,
        shuffle_item,
        repeat_item,
        "About",
        "Exit to Loader"
    };
    int menu_count = 5;

    ui_draw_menu(menu_items, menu_count, selected);

    /* Navigation */
    if (input_button_just_pressed(BTN_DPAD_UP)) {
        selected = (selected - 1 + menu_count) % menu_count;
    }
    if (input_button_just_pressed(BTN_DPAD_DOWN)) {
        selected = (selected + 1) % menu_count;
    }

    /* Adjust values with left/right */
    if (input_button_just_pressed(BTN_DPAD_LEFT) || input_button_just_pressed(BTN_DPAD_RIGHT)) {
        int delta = input_button_just_pressed(BTN_DPAD_RIGHT) ? 1 : -1;

        switch (selected) {
            case 0:  /* Volume */
                g_app.settings.volume = CLAMP(g_app.settings.volume + delta * 16, 0, 255);
                audio_set_volume(g_app.settings.volume);
                break;
            case 1:  /* Shuffle */
                g_app.settings.shuffle = !g_app.settings.shuffle;
                break;
            case 2:  /* Repeat */
                g_app.settings.repeat = !g_app.settings.repeat;
                break;
        }
    }

    /* Select action */
    if (input_button_just_pressed(BTN_A)) {
        switch (selected) {
            case 3:  /* About */
                /* Show about info */
                break;
            case 4:  /* Exit */
                g_app.running = false;
                break;
        }
    }

    /* Draw help */
    ui_draw_text(20, SCREEN_HEIGHT - 30, "A:Select  B:Back  D-Pad:Navigate/Adjust", COLOR_TEXT_DIM);
}

static void handle_state_error(void)
{
    ui_draw_error(g_app.error_message);

    ui_draw_text_centered(SCREEN_HEIGHT - 80, "Press A to retry, B to exit", COLOR_TEXT);

    if (input_button_just_pressed(BTN_A)) {
        g_app.state = STATE_INIT;
        app_init();
    }
    if (input_button_just_pressed(BTN_B)) {
        g_app.running = false;
    }
}

/*
 * Main entry point
 */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    app_init();
    app_run();
    app_shutdown();

    /* Return to loader */
    return 0;
}
