/*
 * Nedflix for Sega Dreamcast
 * Main application
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>

/* Global app instance */
app_t g_app;

/* Library names */
static const char *lib_names[] = {
    "Music",
    "Audiobooks",
    "Movies",
    "TV Shows"
};

/* Library base paths */
static const char *lib_paths[] = {
    "/Music",
    "/Audiobooks",
    "/Movies",
    "/TV Shows"
};

/* State handlers */
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
    DBG("Nedflix for Dreamcast v%s", NEDFLIX_VERSION);
    DBG("Initializing...");

    /* Clear state */
    memset(&g_app, 0, sizeof(g_app));
    g_app.state = STATE_INIT;
    g_app.running = true;
    g_app.current_library = LIBRARY_MUSIC;  /* Start with music - best for DC */

    /* Load or set default config */
    config_defaults(&g_app.settings);
    config_load(&g_app.settings);

    /* Initialize subsystems */
    ui_init();
    input_init();

    if (audio_init() != 0) {
        DBG("Audio init failed (non-fatal)");
    }

    /* Start network init */
    g_app.state = STATE_NETWORK_INIT;

    DBG("Init complete, free RAM: %lu bytes", (unsigned long)mallinfo().fordblks);
}

/*
 * Set error state
 */
void app_set_error(const char *msg)
{
    strncpy(g_app.error_msg, msg, sizeof(g_app.error_msg) - 1);
    g_app.state = STATE_ERROR;
    ERR("%s", msg);
}

/*
 * Main application loop
 */
void app_run(void)
{
    while (g_app.running) {
        /* Update input */
        input_update();

        /* Global exit check */
        if (input_pressed(DC_BTN_START) && input_held(DC_BTN_A) && input_held(DC_BTN_B)) {
            DBG("Exit requested");
            g_app.running = false;
            continue;
        }

        /* Begin frame */
        ui_begin_frame();
        ui_draw_background();

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
    DBG("Shutting down...");

    audio_stop();
    audio_shutdown();
    net_shutdown();
    ui_shutdown();

    config_save(&g_app.settings);

    DBG("Goodbye!");
}

/*
 * STATE: Init
 */
static void state_init(void)
{
    ui_draw_loading("Starting...");
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
            ui_draw_loading("Detecting network adapter...");
            phase = 1;
            timeout = 0;
            break;

        case 1:
            if (net_init() == 0) {
                DBG("Network initialized");
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
                if (timeout > 300) {  /* ~5 seconds at 60fps */
                    app_set_error("No network adapter found.\nBroadband Adapter required.");
                    phase = 0;
                }
                ui_draw_loading("Initializing network...");
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
            /* Check for saved session */
            if (strlen(g_app.settings.session_token) > 0) {
                g_app.state = STATE_MENU;
            } else {
                g_app.state = STATE_LOGIN;
            }
        } else {
            app_set_error("Cannot connect to server.\nCheck URL in settings.");
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

    /* For Dreamcast, we use a simplified login */
    const char *options[] = {
        "Use saved credentials",
        "Browse as guest",
        "Settings",
        "Exit"
    };

    ui_draw_menu(options, 4, selected);

    ui_draw_text(40, 380, "Dreamcast cannot display keyboard.", COLOR_TEXT_DIM);
    ui_draw_text(40, 400, "Configure server on PC first.", COLOR_TEXT_DIM);

    /* Navigation */
    if (input_pressed(DC_BTN_UP)) {
        selected = (selected - 1 + 4) % 4;
    }
    if (input_pressed(DC_BTN_DOWN)) {
        selected = (selected + 1) % 4;
    }

    if (input_pressed(DC_BTN_A)) {
        switch (selected) {
            case 0:  /* Use saved */
                if (strlen(g_app.settings.session_token) > 0) {
                    g_app.state = STATE_MENU;
                }
                break;
            case 1:  /* Guest */
                g_app.state = STATE_MENU;
                break;
            case 2:  /* Settings */
                g_app.state = STATE_SETTINGS;
                break;
            case 3:  /* Exit */
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
        "Music          [Best for Dreamcast]",
        "Audiobooks     [Recommended]",
        "Movies         [Limited support]",
        "TV Shows       [Limited support]",
        "Settings"
    };

    ui_draw_menu(options, 5, selected);

    /* Memory warning */
    ui_draw_text(40, 400, "Note: Video playback is very limited on Dreamcast.", COLOR_TEXT_DIM);
    ui_draw_text(40, 420, "Audio streaming works best!", COLOR_TEXT_DIM);

    /* Navigation */
    if (input_pressed(DC_BTN_UP)) {
        selected = (selected - 1 + 5) % 5;
    }
    if (input_pressed(DC_BTN_DOWN)) {
        selected = (selected + 1) % 5;
    }

    if (input_pressed(DC_BTN_A)) {
        if (selected < 4) {
            g_app.current_library = (library_t)selected;
            strncpy(g_app.media.path, lib_paths[selected], MAX_PATH_LENGTH - 1);
            g_app.media.count = 0;
            g_app.media.selected = 0;
            g_app.media.scroll = 0;

#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token,
                       g_app.media.path,
                       g_app.current_library,
                       &g_app.media);
#endif
            g_app.state = STATE_BROWSING;
        } else {
            g_app.state = STATE_SETTINGS;
        }
    }

    if (input_pressed(DC_BTN_B)) {
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

    /* Draw file list */
    ui_draw_media_list(&g_app.media);

    /* Navigation */
    if (input_pressed(DC_BTN_UP)) {
        if (g_app.media.selected > 0) {
            g_app.media.selected--;
            if (g_app.media.selected < g_app.media.scroll) {
                g_app.media.scroll--;
            }
        }
    }
    if (input_pressed(DC_BTN_DOWN)) {
        if (g_app.media.selected < g_app.media.count - 1) {
            g_app.media.selected++;
            if (g_app.media.selected >= g_app.media.scroll + MAX_ITEMS_VISIBLE) {
                g_app.media.scroll++;
            }
        }
    }

    /* Switch library with L/R triggers */
    if (g_app.ltrig > 200 && input_pressed(DC_BTN_LEFT)) {
        g_app.current_library = (g_app.current_library - 1 + LIBRARY_COUNT) % LIBRARY_COUNT;
        strncpy(g_app.media.path, lib_paths[g_app.current_library], MAX_PATH_LENGTH - 1);
        g_app.media.count = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.path,
                   g_app.current_library, &g_app.media);
#endif
    }
    if (g_app.rtrig > 200 && input_pressed(DC_BTN_RIGHT)) {
        g_app.current_library = (g_app.current_library + 1) % LIBRARY_COUNT;
        strncpy(g_app.media.path, lib_paths[g_app.current_library], MAX_PATH_LENGTH - 1);
        g_app.media.count = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.path,
                   g_app.current_library, &g_app.media);
#endif
    }

    /* Select item */
    if (input_pressed(DC_BTN_A) && g_app.media.count > 0) {
        media_item_t *item = &g_app.media.items[g_app.media.selected];

        if (item->flags & 1) {  /* is_directory */
            strncpy(g_app.media.path, item->path, MAX_PATH_LENGTH - 1);
            g_app.media.count = 0;
            g_app.media.selected = 0;
            g_app.media.scroll = 0;
#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token, g_app.media.path,
                       g_app.current_library, &g_app.media);
#endif
        } else {
            /* Play media */
            char stream_url[MAX_URL_LENGTH];
#if NEDFLIX_CLIENT_MODE
            if (api_get_stream_url(g_app.settings.session_token, item->path,
                                   stream_url, sizeof(stream_url)) == 0) {
                strncpy(g_app.playback.title, item->name, MAX_TITLE_LENGTH - 1);
                strncpy(g_app.playback.url, stream_url, MAX_URL_LENGTH - 1);
                g_app.playback.is_audio = (item->type == MEDIA_AUDIO);

                if (item->type == MEDIA_AUDIO) {
                    if (audio_play_stream(stream_url) == 0) {
                        g_app.playback.playing = true;
                        g_app.state = STATE_PLAYING;
                    }
                } else {
                    /* Video - show warning */
                    app_set_error("Video playback not supported.\n16MB RAM is too limited.");
                }
            }
#endif
        }
    }

    /* Go back */
    if (input_pressed(DC_BTN_B)) {
        /* Go up one directory or back to menu */
        char *last_slash = strrchr(g_app.media.path, '/');
        if (last_slash && last_slash != g_app.media.path) {
            *last_slash = '\0';
            g_app.media.count = 0;
            g_app.media.selected = 0;
#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token, g_app.media.path,
                       g_app.current_library, &g_app.media);
#endif
        } else {
            g_app.state = STATE_MENU;
        }
    }

    /* Help text */
    ui_draw_text(20, 450, "A:Select  B:Back  L+Left/R+Right:Library", COLOR_TEXT_DIM);
}

/*
 * STATE: Playing media
 */
static void state_playing(void)
{
    /* Update playback state */
    g_app.playback.position_ms = audio_get_position();
    g_app.playback.duration_ms = audio_get_duration();
    g_app.playback.playing = audio_is_playing();

    /* Draw playback UI */
    ui_draw_playback(&g_app.playback);

    /* Controls */
    if (input_pressed(DC_BTN_A) || input_pressed(DC_BTN_START)) {
        if (g_app.playback.paused) {
            audio_resume();
            g_app.playback.paused = false;
        } else {
            audio_pause();
            g_app.playback.paused = true;
        }
    }

    if (input_pressed(DC_BTN_B)) {
        audio_stop();
        g_app.playback.playing = false;
        g_app.state = STATE_BROWSING;
    }

    /* Volume control with triggers */
    if (g_app.ltrig > 100) {
        g_app.settings.volume = MAX(0, g_app.settings.volume - 1);
        audio_set_volume(g_app.settings.volume);
    }
    if (g_app.rtrig > 100) {
        g_app.settings.volume = MIN(100, g_app.settings.volume + 1);
        audio_set_volume(g_app.settings.volume);
    }

    /* Seek with D-pad (if supported) */
    if (input_held(DC_BTN_LEFT)) {
        /* Seek back - would need stream support */
    }
    if (input_held(DC_BTN_RIGHT)) {
        /* Seek forward - would need stream support */
    }

    /* Check for playback end */
    if (!g_app.playback.playing && !g_app.playback.paused &&
        g_app.playback.position_ms > 0 &&
        g_app.playback.position_ms >= g_app.playback.duration_ms - 1000) {
        /* Track finished - could auto-play next */
        g_app.state = STATE_BROWSING;
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
    snprintf(vol_str, sizeof(vol_str), "Volume: %d%%", g_app.settings.volume);

    const char *options[] = {
        g_app.settings.server_url[0] ? g_app.settings.server_url : "Server: (not set)",
        vol_str,
        "Save settings to VMU",
        "Back"
    };

    ui_draw_menu(options, 4, selected);

    ui_draw_text(40, 380, "Server URL must be configured on PC", COLOR_TEXT_DIM);
    ui_draw_text(40, 400, "then transferred via CD-R or SD adapter.", COLOR_TEXT_DIM);

    /* Navigation */
    if (input_pressed(DC_BTN_UP)) {
        selected = (selected - 1 + 4) % 4;
    }
    if (input_pressed(DC_BTN_DOWN)) {
        selected = (selected + 1) % 4;
    }

    /* Adjust volume with left/right */
    if (selected == 1) {
        if (input_pressed(DC_BTN_LEFT)) {
            g_app.settings.volume = MAX(0, g_app.settings.volume - 5);
        }
        if (input_pressed(DC_BTN_RIGHT)) {
            g_app.settings.volume = MIN(100, g_app.settings.volume + 5);
        }
    }

    if (input_pressed(DC_BTN_A)) {
        switch (selected) {
            case 0:  /* Server - can't edit on DC */
                break;
            case 1:  /* Volume - adjusted with left/right */
                break;
            case 2:  /* Save */
                if (config_save(&g_app.settings) == 0) {
                    /* Show brief confirmation */
                }
                break;
            case 3:  /* Back */
                g_app.state = STATE_MENU;
                break;
        }
    }

    if (input_pressed(DC_BTN_B)) {
        g_app.state = STATE_MENU;
    }
}

/*
 * STATE: Error display
 */
static void state_error(void)
{
    ui_draw_error(g_app.error_msg);

    ui_draw_text_centered(380, "Press A to retry, B to exit", COLOR_TEXT);

    if (input_pressed(DC_BTN_A)) {
        g_app.state = STATE_NETWORK_INIT;
    }
    if (input_pressed(DC_BTN_B)) {
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

    /* KOS initialization */
    pvr_init_defaults();

    /* Run app */
    app_init();
    app_run();
    app_shutdown();

    return 0;
}
