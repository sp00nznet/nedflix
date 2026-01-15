/*
 * Nedflix for PlayStation 3
 * Technical demo using PSL1GHT SDK
 *
 * This is a proof-of-concept demonstrating:
 * - RSX graphics initialization
 * - DualShock 3 controller input
 * - Network connectivity
 * - Audio output via Cell
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/process.h>
#include <sysutil/sysutil.h>

/* Global application instance */
app_t g_app;

/* SYS_PROCESS_PARAM for PSL1GHT */
SYS_PROCESS_PARAM(1001, 0x100000);

/* Library names */
static const char *lib_names[] = {
    "Music",
    "Audiobooks",
    "Movies",
    "TV Shows"
};

/* Library paths */
static const char *lib_paths[] = {
    "/Music",
    "/Audiobooks",
    "/Movies",
    "/TV Shows"
};

/* XMB exit callback */
static void sysutil_callback(u64 status, u64 param, void *userdata)
{
    (void)param;
    (void)userdata;

    switch (status) {
        case SYSUTIL_EXIT_GAME:
            g_app.running = false;
            break;
        case SYSUTIL_DRAW_BEGIN:
        case SYSUTIL_DRAW_END:
            break;
    }
}

/* State handler prototypes */
static void state_init(void);
static void state_network(void);
static void state_connecting(void);
static void state_login(void);
static void state_menu(void);
static void state_browsing(void);
static void state_playing(void);
static void state_settings(void);
static void state_error(void);

/*
 * Initialize application
 */
void app_init(void)
{
    printf("Nedflix for PS3 v%s\n", NEDFLIX_VERSION);
    printf("PSL1GHT Technical Demo\n");
    printf("Initializing...\n");

    /* Clear state */
    memset(&g_app, 0, sizeof(g_app));
    g_app.state = STATE_INIT;
    g_app.running = true;
    g_app.current_library = LIBRARY_MUSIC;

    /* Register XMB callback */
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_callback, NULL);

    /* Load default config */
    config_defaults(&g_app.settings);
    config_load(&g_app.settings);

    /* Initialize subsystems */
    if (ui_init() != 0) {
        app_set_error("Failed to initialize graphics");
        return;
    }

    if (input_init() != 0) {
        app_set_error("Failed to initialize input");
        return;
    }

    if (audio_init() != 0) {
        printf("Warning: Audio init failed (non-fatal)\n");
    }

    /* Move to network init */
    g_app.state = STATE_NETWORK_INIT;

    printf("Initialization complete\n");
}

/*
 * Set error state
 */
void app_set_error(const char *msg)
{
    strncpy(g_app.error_msg, msg, sizeof(g_app.error_msg) - 1);
    g_app.state = STATE_ERROR;
    printf("ERROR: %s\n", msg);
}

/*
 * Set status message
 */
void app_set_status(const char *msg)
{
    strncpy(g_app.status_msg, msg, sizeof(g_app.status_msg) - 1);
    printf("STATUS: %s\n", msg);
}

/*
 * Main application loop
 */
void app_run(void)
{
    while (g_app.running) {
        /* Check XMB events */
        sysUtilCheckCallback();

        /* Update input */
        input_update();

        /* Global exit: PS + Start */
        if (input_held(BTN_PS) && input_pressed(BTN_START)) {
            printf("Exit requested via PS+Start\n");
            g_app.running = false;
            continue;
        }

        /* Begin frame */
        ui_begin_frame();

        /* Run state handler */
        switch (g_app.state) {
            case STATE_INIT:        state_init(); break;
            case STATE_NETWORK_INIT: state_network(); break;
            case STATE_CONNECTING:  state_connecting(); break;
            case STATE_LOGIN:       state_login(); break;
            case STATE_MENU:        state_menu(); break;
            case STATE_BROWSING:    state_browsing(); break;
            case STATE_PLAYING:     state_playing(); break;
            case STATE_SETTINGS:    state_settings(); break;
            case STATE_ERROR:       state_error(); break;
        }

        /* End frame */
        ui_end_frame();

        /* Update audio if playing */
        if (g_app.state == STATE_PLAYING) {
            audio_update();
        }

        g_app.frame_count++;
    }
}

/*
 * Shutdown application
 */
void app_shutdown(void)
{
    printf("Shutting down...\n");

    audio_stop();
    audio_shutdown();
    network_shutdown();
    ui_shutdown();
    input_shutdown();

    config_save(&g_app.settings);

    printf("Goodbye!\n");
}

/*
 * STATE: Init
 */
static void state_init(void)
{
    ui_draw_loading("Starting Nedflix...");
}

/*
 * STATE: Network initialization
 */
static void state_network(void)
{
    static int phase = 0;
    static int timeout = 0;

    switch (phase) {
        case 0:
            ui_draw_loading("Initializing network...");
            phase = 1;
            timeout = 0;
            break;

        case 1:
            if (network_init() == 0) {
                printf("Network initialized: %s\n", g_app.net.local_ip);
                phase = 0;
#if NEDFLIX_CLIENT_MODE
                if (strlen(g_app.settings.server_url) > 0) {
                    g_app.state = STATE_CONNECTING;
                } else {
                    g_app.state = STATE_SETTINGS;
                }
#else
                g_app.state = STATE_MENU;
#endif
            } else {
                timeout++;
                if (timeout > 300) {
                    app_set_error("Network initialization failed.\nCheck your connection.");
                    phase = 0;
                }
                ui_draw_loading("Waiting for network...");
            }
            break;
    }
}

/*
 * STATE: Connecting to server
 */
static void state_connecting(void)
{
    static bool started = false;

    ui_draw_loading("Connecting to server...");

    if (!started) {
        started = true;
        int result = api_init(g_app.settings.server_url);
        started = false;

        if (result == 0) {
            if (strlen(g_app.settings.session_token) > 0) {
                g_app.state = STATE_MENU;
            } else {
                g_app.state = STATE_LOGIN;
            }
        } else {
            app_set_error("Cannot connect to server.\nCheck settings.");
        }
    }
}

/*
 * STATE: Login screen
 */
static void state_login(void)
{
    static int selected = 0;

    ui_draw_header("Login Required");

    const char *options[] = {
        "Use saved credentials",
        "Browse as guest",
        "Settings",
        "Exit"
    };

    ui_draw_menu(options, 4, selected);

    ui_draw_text(100, 500, "Configure server URL in Settings first.", COLOR_TEXT_DIM);

    /* Navigation */
    if (input_pressed(BTN_UP)) {
        selected = (selected - 1 + 4) % 4;
    }
    if (input_pressed(BTN_DOWN)) {
        selected = (selected + 1) % 4;
    }

    if (input_pressed(BTN_CROSS)) {
        switch (selected) {
            case 0:
                if (strlen(g_app.settings.session_token) > 0) {
                    g_app.state = STATE_MENU;
                }
                break;
            case 1:
                g_app.state = STATE_MENU;
                break;
            case 2:
                g_app.state = STATE_SETTINGS;
                break;
            case 3:
                g_app.running = false;
                break;
        }
    }
}

/*
 * STATE: Main menu
 */
static void state_menu(void)
{
    static int selected = 0;

    ui_draw_header("Nedflix");

    const char *options[] = {
        "Music",
        "Audiobooks",
        "Movies         [HD Streaming]",
        "TV Shows       [HD Streaming]",
        "Settings"
    };

    ui_draw_menu(options, 5, selected);

    /* PS3 capability note */
    ui_draw_text(100, 550, "PS3: Full HD video + audio streaming supported", COLOR_TEXT_DIM);
    ui_draw_text(100, 580, "Cell SPE acceleration available", COLOR_TEXT_DIM);

    /* Navigation */
    if (input_pressed(BTN_UP)) {
        selected = (selected - 1 + 5) % 5;
    }
    if (input_pressed(BTN_DOWN)) {
        selected = (selected + 1) % 5;
    }

    if (input_pressed(BTN_CROSS)) {
        if (selected < 4) {
            g_app.current_library = (library_t)selected;
            strncpy(g_app.media.current_path, lib_paths[selected], MAX_PATH_LENGTH - 1);
            g_app.media.count = 0;
            g_app.media.selected_index = 0;
            g_app.media.scroll_offset = 0;

#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token,
                      g_app.media.current_path,
                      g_app.current_library,
                      &g_app.media);
#endif
            g_app.state = STATE_BROWSING;
        } else {
            g_app.state = STATE_SETTINGS;
        }
    }

    if (input_pressed(BTN_CIRCLE)) {
        g_app.running = false;
    }
}

/*
 * STATE: Browsing media
 */
static void state_browsing(void)
{
    char header[64];
    snprintf(header, sizeof(header), "%s", lib_names[g_app.current_library]);
    ui_draw_header(header);

    ui_draw_media_list(&g_app.media);

    /* Navigation */
    if (input_pressed(BTN_UP)) {
        if (g_app.media.selected_index > 0) {
            g_app.media.selected_index--;
            if (g_app.media.selected_index < g_app.media.scroll_offset) {
                g_app.media.scroll_offset--;
            }
        }
    }
    if (input_pressed(BTN_DOWN)) {
        if (g_app.media.selected_index < g_app.media.count - 1) {
            g_app.media.selected_index++;
            if (g_app.media.selected_index >= g_app.media.scroll_offset + MAX_ITEMS_VISIBLE) {
                g_app.media.scroll_offset++;
            }
        }
    }

    /* Library switch with L1/R1 */
    if (input_pressed(BTN_L1)) {
        g_app.current_library = (g_app.current_library - 1 + LIBRARY_COUNT) % LIBRARY_COUNT;
        strncpy(g_app.media.current_path, lib_paths[g_app.current_library], MAX_PATH_LENGTH - 1);
        g_app.media.count = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.current_path,
                  g_app.current_library, &g_app.media);
#endif
    }
    if (input_pressed(BTN_R1)) {
        g_app.current_library = (g_app.current_library + 1) % LIBRARY_COUNT;
        strncpy(g_app.media.current_path, lib_paths[g_app.current_library], MAX_PATH_LENGTH - 1);
        g_app.media.count = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.current_path,
                  g_app.current_library, &g_app.media);
#endif
    }

    /* Select item */
    if (input_pressed(BTN_CROSS) && g_app.media.count > 0) {
        media_item_t *item = &g_app.media.items[g_app.media.selected_index];

        if (item->is_directory) {
            strncpy(g_app.media.current_path, item->path, MAX_PATH_LENGTH - 1);
            g_app.media.count = 0;
            g_app.media.selected_index = 0;
            g_app.media.scroll_offset = 0;
#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token, g_app.media.current_path,
                      g_app.current_library, &g_app.media);
#endif
        } else {
            char stream_url[MAX_URL_LENGTH];
#if NEDFLIX_CLIENT_MODE
            if (api_get_stream_url(g_app.settings.session_token, item->path,
                                  g_app.settings.video_quality, stream_url, sizeof(stream_url)) == 0) {
                strncpy(g_app.playback.title, item->name, MAX_TITLE_LENGTH - 1);
                strncpy(g_app.playback.url, stream_url, MAX_URL_LENGTH - 1);
                g_app.playback.is_audio = (item->type == MEDIA_TYPE_AUDIO);

                if (item->type == MEDIA_TYPE_AUDIO) {
                    if (audio_play_stream(stream_url) == 0) {
                        g_app.playback.playing = true;
                        g_app.state = STATE_PLAYING;
                    }
                } else {
                    /* Video playback */
                    if (video_play_stream(stream_url) == 0) {
                        g_app.playback.playing = true;
                        g_app.state = STATE_PLAYING;
                    }
                }
            }
#endif
        }
    }

    /* Go back */
    if (input_pressed(BTN_CIRCLE)) {
        char *last_slash = strrchr(g_app.media.current_path, '/');
        if (last_slash && last_slash != g_app.media.current_path) {
            *last_slash = '\0';
            g_app.media.count = 0;
            g_app.media.selected_index = 0;
#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token, g_app.media.current_path,
                      g_app.current_library, &g_app.media);
#endif
        } else {
            g_app.state = STATE_MENU;
        }
    }

    ui_draw_text(50, 650, "X:Select  O:Back  L1/R1:Library", COLOR_TEXT_DIM);
}

/*
 * STATE: Playing media
 */
static void state_playing(void)
{
    /* Update playback state */
    if (g_app.playback.is_audio) {
        g_app.playback.position_ms = audio_get_position();
        g_app.playback.duration_ms = audio_get_duration();
        g_app.playback.playing = audio_is_playing();
    } else {
        video_render_frame();
    }

    /* Draw playback UI */
    ui_draw_playback(&g_app.playback);

    /* Controls */
    if (input_pressed(BTN_CROSS) || input_pressed(BTN_START)) {
        if (g_app.playback.paused) {
            if (g_app.playback.is_audio) audio_resume();
            else video_resume();
            g_app.playback.paused = false;
        } else {
            if (g_app.playback.is_audio) audio_pause();
            else video_pause();
            g_app.playback.paused = true;
        }
    }

    if (input_pressed(BTN_CIRCLE)) {
        if (g_app.playback.is_audio) audio_stop();
        else video_stop();
        g_app.playback.playing = false;
        g_app.state = STATE_BROWSING;
    }

    /* Volume with L2/R2 pressure */
    if (g_app.l2_pressure > 50) {
        g_app.settings.volume = MAX(0, g_app.settings.volume - 1);
        audio_set_volume(g_app.settings.volume);
    }
    if (g_app.r2_pressure > 50) {
        g_app.settings.volume = MIN(100, g_app.settings.volume + 1);
        audio_set_volume(g_app.settings.volume);
    }

    /* Seek with D-pad */
    if (input_held(BTN_LEFT)) {
        if (g_app.playback.is_audio) audio_seek(-10000);
        else video_seek(-10000);
    }
    if (input_held(BTN_RIGHT)) {
        if (g_app.playback.is_audio) audio_seek(10000);
        else video_seek(10000);
    }
}

/*
 * STATE: Settings
 */
static void state_settings(void)
{
    static int selected = 0;

    ui_draw_header("Settings");

    char vol_str[32];
    char quality_str[32];
    const char *qualities[] = { "SD (480p)", "HD (720p)", "Full HD (1080p)" };

    snprintf(vol_str, sizeof(vol_str), "Volume: %d%%", g_app.settings.volume);
    snprintf(quality_str, sizeof(quality_str), "Quality: %s", qualities[g_app.settings.video_quality]);

    const char *options[] = {
        g_app.settings.server_url[0] ? g_app.settings.server_url : "Server: (not set)",
        vol_str,
        quality_str,
        g_app.settings.enable_surround ? "Surround: ON" : "Surround: OFF",
        "Save settings",
        "Back"
    };

    ui_draw_menu(options, 6, selected);

    ui_draw_text(100, 550, "Use on-screen keyboard (Triangle) to edit server URL", COLOR_TEXT_DIM);

    /* Navigation */
    if (input_pressed(BTN_UP)) {
        selected = (selected - 1 + 6) % 6;
    }
    if (input_pressed(BTN_DOWN)) {
        selected = (selected + 1) % 6;
    }

    /* Adjust values */
    if (selected == 1) {
        if (input_pressed(BTN_LEFT)) {
            g_app.settings.volume = MAX(0, g_app.settings.volume - 5);
        }
        if (input_pressed(BTN_RIGHT)) {
            g_app.settings.volume = MIN(100, g_app.settings.volume + 5);
        }
    }
    if (selected == 2) {
        if (input_pressed(BTN_LEFT) || input_pressed(BTN_RIGHT)) {
            g_app.settings.video_quality = (g_app.settings.video_quality + 1) % 3;
        }
    }
    if (selected == 3) {
        if (input_pressed(BTN_CROSS)) {
            g_app.settings.enable_surround = !g_app.settings.enable_surround;
        }
    }

    /* OSK for server URL */
    if (selected == 0 && input_pressed(BTN_TRIANGLE)) {
        ui_draw_osk("Enter Server URL", g_app.settings.server_url, MAX_URL_LENGTH);
    }

    if (input_pressed(BTN_CROSS)) {
        switch (selected) {
            case 4:
                config_save(&g_app.settings);
                app_set_status("Settings saved");
                break;
            case 5:
                g_app.state = STATE_MENU;
                break;
        }
    }

    if (input_pressed(BTN_CIRCLE)) {
        g_app.state = STATE_MENU;
    }
}

/*
 * STATE: Error display
 */
static void state_error(void)
{
    ui_draw_error(g_app.error_msg);

    ui_draw_text_centered(500, "Press X to retry, O to exit", COLOR_TEXT);

    if (input_pressed(BTN_CROSS)) {
        g_app.state = STATE_NETWORK_INIT;
    }
    if (input_pressed(BTN_CIRCLE)) {
        g_app.running = false;
    }
}

/*
 * Entry point
 */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("\n=== Nedflix PS3 Technical Demo ===\n");
    printf("Using PSL1GHT SDK\n\n");

    app_init();
    app_run();
    app_shutdown();

    return 0;
}
