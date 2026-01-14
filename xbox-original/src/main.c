/*
 * Nedflix for Original Xbox
 * Main entry point
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>

#ifdef NXDK
#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>
#include <nxdk/mount.h>
#endif

/* Global application context */
app_context_t g_app;

/* Library names for display */
static const char *library_names[] = {
    "Movies",
    "TV Shows",
    "Music",
    "Audiobooks"
};

/* Library paths on server */
static const char *library_paths[] = {
    "/Movies",
    "/TV Shows",
    "/Music",
    "/Audiobooks"
};

/*
 * Forward declarations for state handlers
 */
static void handle_state_init(void);
static void handle_state_connecting(void);
static void handle_state_login(void);
static void handle_state_browsing(void);
static void handle_state_playing(void);
static void handle_state_settings(void);
static void handle_state_error(void);

/*
 * Initialize the application
 */
void app_init(void)
{
    LOG("Nedflix for Original Xbox v%s", NEDFLIX_VERSION_STRING);
    LOG("Initializing...");

    /* Clear context */
    memset(&g_app, 0, sizeof(g_app));
    g_app.state = STATE_INIT;
    g_app.running = true;
    g_app.current_library = LIBRARY_MOVIES;

    /* Load configuration */
    config_set_defaults(&g_app.settings);
    if (config_load(&g_app.settings) != 0) {
        LOG("No config found, using defaults");
    }

    /* Initialize subsystems */
    if (ui_init() != 0) {
        LOG_ERROR("Failed to initialize UI");
        g_app.state = STATE_ERROR;
        strncpy(g_app.error_message, "Failed to initialize graphics", sizeof(g_app.error_message) - 1);
        return;
    }

    if (input_init() != 0) {
        LOG_ERROR("Failed to initialize input");
        g_app.state = STATE_ERROR;
        strncpy(g_app.error_message, "Failed to initialize controller", sizeof(g_app.error_message) - 1);
        return;
    }

    if (http_init() != 0) {
        LOG_ERROR("Failed to initialize network");
        g_app.state = STATE_ERROR;
        strncpy(g_app.error_message, "Failed to initialize network", sizeof(g_app.error_message) - 1);
        return;
    }

    if (video_init() != 0) {
        LOG_ERROR("Failed to initialize video playback");
        /* Non-fatal - continue without video */
    }

    /* Allocate media list */
    g_app.media_list.capacity = 100;
    g_app.media_list.items = (media_item_t *)malloc(sizeof(media_item_t) * g_app.media_list.capacity);
    if (!g_app.media_list.items) {
        LOG_ERROR("Failed to allocate media list");
        g_app.state = STATE_ERROR;
        strncpy(g_app.error_message, "Out of memory", sizeof(g_app.error_message) - 1);
        return;
    }

    LOG("Initialization complete");

#if NEDFLIX_CLIENT_MODE
    /* Client mode - connect to server */
    if (strlen(g_app.settings.server_url) > 0) {
        g_app.state = STATE_CONNECTING;
    } else {
        g_app.state = STATE_SETTINGS;  /* Need to configure server */
    }
#else
    /* Desktop mode - browse local files */
    g_app.state = STATE_BROWSING;
    strncpy(g_app.media_list.current_path, "E:\\Media", sizeof(g_app.media_list.current_path) - 1);
#endif
}

/*
 * Shutdown the application
 */
void app_shutdown(void)
{
    LOG("Shutting down...");

    /* Stop any playback */
    video_stop();

    /* Free resources */
    if (g_app.media_list.items) {
        free(g_app.media_list.items);
        g_app.media_list.items = NULL;
    }

    /* Shutdown subsystems */
    video_shutdown();
    http_shutdown();
    input_shutdown();
    ui_shutdown();

    /* Save configuration */
    config_save(&g_app.settings);

    LOG("Shutdown complete");
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
        if (input_button_just_pressed(BTN_BACK) && g_app.state != STATE_ERROR) {
            if (g_app.state == STATE_PLAYING) {
                video_stop();
                g_app.state = STATE_BROWSING;
            } else if (g_app.state == STATE_BROWSING) {
                /* Go up a directory or show settings */
                if (strlen(g_app.media_list.current_path) > 1) {
                    /* Navigate up */
                    char *last_slash = strrchr(g_app.media_list.current_path, '/');
                    if (last_slash && last_slash != g_app.media_list.current_path) {
                        *last_slash = '\0';
                    }
                } else {
                    g_app.state = STATE_SETTINGS;
                }
            } else if (g_app.state == STATE_SETTINGS) {
                g_app.state = STATE_BROWSING;
            }
        }

        /* Start/Guide button to toggle settings */
        if (input_button_just_pressed(BTN_START) &&
            g_app.state != STATE_ERROR &&
            g_app.state != STATE_INIT) {
            if (g_app.state == STATE_SETTINGS) {
                g_app.state = STATE_BROWSING;
            } else {
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
            case STATE_CONNECTING:
                handle_state_connecting();
                break;
            case STATE_LOGIN:
                handle_state_login();
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

        /* Update video if playing */
        if (g_app.state == STATE_PLAYING) {
            video_update();
        }
    }
}

/*
 * State handlers
 */

static void handle_state_init(void)
{
    ui_draw_loading("Starting Nedflix...");
}

static void handle_state_connecting(void)
{
    static bool connecting = false;
    static int result = 0;

    ui_draw_loading("Connecting to server...");

    if (!connecting) {
        connecting = true;
        result = api_init(g_app.settings.server_url);
    }

    if (result == 0) {
        /* Connected - check if we have auth token */
        if (strlen(g_app.settings.auth_token) > 0) {
            char username[64] = {0};
            if (api_get_user_info(g_app.settings.auth_token, username, sizeof(username)) == 0) {
                strncpy(g_app.settings.username, username, sizeof(g_app.settings.username) - 1);
                g_app.state = STATE_BROWSING;
                connecting = false;
                return;
            }
        }
        g_app.state = STATE_LOGIN;
        connecting = false;
    } else {
        g_app.state = STATE_ERROR;
        snprintf(g_app.error_message, sizeof(g_app.error_message),
                 "Failed to connect to %s", g_app.settings.server_url);
        connecting = false;
    }
}

static void handle_state_login(void)
{
    /* Simple login screen - in a full implementation, this would have
     * an on-screen keyboard for entering credentials */

    ui_draw_header("Login");

    static int selected = 0;
    const char *menu_items[] = {
        "Login with saved credentials",
        "Enter username/password",
        "Change server URL",
        "Exit"
    };

    ui_draw_menu(menu_items, 4, selected);

    if (input_button_just_pressed(BTN_DPAD_UP)) {
        selected = (selected - 1 + 4) % 4;
    }
    if (input_button_just_pressed(BTN_DPAD_DOWN)) {
        selected = (selected + 1) % 4;
    }

    if (input_button_just_pressed(BTN_A)) {
        switch (selected) {
            case 0:  /* Login with saved */
                if (strlen(g_app.settings.username) > 0 &&
                    strlen(g_app.settings.auth_token) > 0) {
                    /* Use saved credentials */
                    g_app.state = STATE_BROWSING;
                }
                break;
            case 1:  /* Enter credentials - would show keyboard */
                /* TODO: Implement on-screen keyboard */
                break;
            case 2:  /* Change server */
                g_app.state = STATE_SETTINGS;
                break;
            case 3:  /* Exit */
                g_app.running = false;
                break;
        }
    }
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

    /* Switch libraries with shoulder buttons */
    if (input_button_just_pressed(BTN_LEFT_TRIGGER)) {
        g_app.current_library = (g_app.current_library - 1 + LIBRARY_COUNT) % LIBRARY_COUNT;
        g_app.media_list.count = 0;
        g_app.media_list.selected_index = 0;
        g_app.media_list.scroll_offset = 0;
        strncpy(g_app.media_list.current_path, library_paths[g_app.current_library],
                sizeof(g_app.media_list.current_path) - 1);
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.auth_token, g_app.media_list.current_path,
                   g_app.current_library, &g_app.media_list);
#endif
    }
    if (input_button_just_pressed(BTN_RIGHT_TRIGGER)) {
        g_app.current_library = (g_app.current_library + 1) % LIBRARY_COUNT;
        g_app.media_list.count = 0;
        g_app.media_list.selected_index = 0;
        g_app.media_list.scroll_offset = 0;
        strncpy(g_app.media_list.current_path, library_paths[g_app.current_library],
                sizeof(g_app.media_list.current_path) - 1);
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.auth_token, g_app.media_list.current_path,
                   g_app.current_library, &g_app.media_list);
#endif
    }

    /* Select item */
    if (input_button_just_pressed(BTN_A) && g_app.media_list.count > 0) {
        media_item_t *item = &g_app.media_list.items[g_app.media_list.selected_index];

        if (item->is_directory) {
            /* Navigate into directory */
            strncpy(g_app.media_list.current_path, item->path,
                    sizeof(g_app.media_list.current_path) - 1);
            g_app.media_list.count = 0;
            g_app.media_list.selected_index = 0;
            g_app.media_list.scroll_offset = 0;
#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.auth_token, g_app.media_list.current_path,
                       g_app.current_library, &g_app.media_list);
#endif
        } else if (item->type == MEDIA_TYPE_VIDEO || item->type == MEDIA_TYPE_AUDIO) {
            /* Play media */
            char stream_url[MAX_URL_LENGTH];
#if NEDFLIX_CLIENT_MODE
            if (api_get_stream_url(g_app.settings.auth_token, item->path,
                                   stream_url, sizeof(stream_url)) == 0) {
                strncpy(g_app.playback.current_file, item->path,
                        sizeof(g_app.playback.current_file) - 1);
                strncpy(g_app.playback.title, item->name,
                        sizeof(g_app.playback.title) - 1);
                if (video_play(stream_url) == 0) {
                    g_app.state = STATE_PLAYING;
                }
            }
#else
            /* Desktop mode - play local file */
            strncpy(g_app.playback.current_file, item->path,
                    sizeof(g_app.playback.current_file) - 1);
            strncpy(g_app.playback.title, item->name,
                    sizeof(g_app.playback.title) - 1);
            if (video_play(item->path) == 0) {
                g_app.state = STATE_PLAYING;
            }
#endif
        }
    }

    /* Draw help text */
    ui_draw_text(20, SCREEN_HEIGHT - 30, "A:Select  B:Back  LT/RT:Library  START:Settings", COLOR_TEXT_DIM);
}

static void handle_state_playing(void)
{
    /* Update playback state */
    g_app.playback.is_playing = video_is_playing();
    g_app.playback.current_time = video_get_position();
    g_app.playback.duration = video_get_duration();
    g_app.playback.volume = g_app.settings.volume;

    /* Draw playback HUD */
    ui_draw_playback_hud(&g_app.playback);

    /* Playback controls */
    if (input_button_just_pressed(BTN_A) || input_button_just_pressed(BTN_X)) {
        /* Play/Pause toggle */
        if (g_app.playback.is_paused) {
            video_resume();
            g_app.playback.is_paused = false;
        } else {
            video_pause();
            g_app.playback.is_paused = true;
        }
    }

    if (input_button_just_pressed(BTN_B)) {
        /* Stop and return to browser */
        video_stop();
        g_app.state = STATE_BROWSING;
        return;
    }

    /* Seeking with D-pad or right stick */
    if (input_button_pressed(BTN_DPAD_LEFT)) {
        video_seek(g_app.playback.current_time - 10.0);
    }
    if (input_button_pressed(BTN_DPAD_RIGHT)) {
        video_seek(g_app.playback.current_time + 10.0);
    }

    int right_x = input_get_right_stick_x();
    if (abs(right_x) > 16000) {
        double seek_speed = (double)right_x / 32768.0 * 30.0;  /* Up to 30 seconds */
        video_seek(g_app.playback.current_time + seek_speed * 0.016);  /* Per frame */
    }

    /* Volume with triggers */
    int left_trigger = input_get_left_trigger();
    int right_trigger = input_get_right_trigger();
    if (left_trigger > 128) {
        g_app.settings.volume = MAX(0, g_app.settings.volume - 1);
        video_set_volume(g_app.settings.volume);
    }
    if (right_trigger > 128) {
        g_app.settings.volume = MIN(100, g_app.settings.volume + 1);
        video_set_volume(g_app.settings.volume);
    }

    /* Fullscreen toggle with Y */
    if (input_button_just_pressed(BTN_Y)) {
        /* Toggle HUD visibility - already handled by timed auto-hide */
    }

    /* Check for playback end */
    if (!g_app.playback.is_playing && g_app.playback.current_time >= g_app.playback.duration - 0.5) {
        /* Playback finished */
        video_stop();
        g_app.state = STATE_BROWSING;

        /* Auto-play next if enabled */
        if (g_app.settings.autoplay && g_app.media_list.selected_index < g_app.media_list.count - 1) {
            g_app.media_list.selected_index++;
            /* Trigger play of next item on next frame */
        }
    }
}

static void handle_state_settings(void)
{
    ui_draw_header("Settings");

    static int selected = 0;

    /* Settings menu items */
    char server_item[128];
    char volume_item[64];
    char autoplay_item[64];
    char subtitles_item[64];

    snprintf(server_item, sizeof(server_item), "Server: %s",
             strlen(g_app.settings.server_url) > 0 ? g_app.settings.server_url : "(not set)");
    snprintf(volume_item, sizeof(volume_item), "Volume: %d%%", g_app.settings.volume);
    snprintf(autoplay_item, sizeof(autoplay_item), "Auto-play: %s",
             g_app.settings.autoplay ? "On" : "Off");
    snprintf(subtitles_item, sizeof(subtitles_item), "Subtitles: %s",
             g_app.settings.show_subtitles ? "On" : "Off");

    const char *menu_items[] = {
        server_item,
        volume_item,
        autoplay_item,
        subtitles_item,
        "Save & Exit",
        "Cancel"
    };
    int menu_count = 6;

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
            case 1:  /* Volume */
                g_app.settings.volume = CLAMP(g_app.settings.volume + delta * 5, 0, 100);
                break;
            case 2:  /* Autoplay */
                g_app.settings.autoplay = !g_app.settings.autoplay;
                break;
            case 3:  /* Subtitles */
                g_app.settings.show_subtitles = !g_app.settings.show_subtitles;
                break;
        }
    }

    /* Select action */
    if (input_button_just_pressed(BTN_A)) {
        switch (selected) {
            case 0:  /* Server URL - would show keyboard */
                /* TODO: Implement on-screen keyboard */
                break;
            case 4:  /* Save & Exit */
                config_save(&g_app.settings);
                g_app.state = STATE_BROWSING;
                break;
            case 5:  /* Cancel */
                config_load(&g_app.settings);  /* Reload saved settings */
                g_app.state = STATE_BROWSING;
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
#ifdef NXDK
int main(void)
{
    /* Initialize Xbox hardware */
    XVideoSetMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, REFRESH_DEFAULT);

    /* Mount drives */
    nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\");
    nxMountDrive('F', "\\Device\\Harddisk0\\Partition6\\");

    /* Run application */
    app_init();
    app_run();
    app_shutdown();

    return 0;
}
#else
/* Non-Xbox build for testing */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    app_init();
    app_run();
    app_shutdown();

    return 0;
}
#endif
