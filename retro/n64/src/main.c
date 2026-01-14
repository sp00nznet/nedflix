/*
 * Nedflix for Nintendo 64
 * Main application - libdragon SDK
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

app_t g_app;

static const char *lib_names[] = {"Music", "Audiobooks", "Movies", "TV Shows"};
static const char *lib_paths[] = {"/Music", "/Audiobooks", "/Movies", "/TV Shows"};

static void state_init(void);
static void state_network(void);
static void state_connecting(void);
static void state_login(void);
static void state_menu(void);
static void state_browsing(void);
static void state_playing(void);
static void state_settings(void);
static void state_error(void);

void app_init(void)
{
    LOG("Nedflix for N64 v%s", NEDFLIX_VERSION);

    memset(&g_app, 0, sizeof(g_app));
    g_app.state = STATE_INIT;
    g_app.running = true;
    g_app.current_library = LIBRARY_MUSIC;

    config_defaults(&g_app.settings);
    config_load(&g_app.settings);

    ui_init();
    input_init();
    audio_init();

    g_app.state = STATE_NETWORK_INIT;
    LOG("Init complete");
}

void app_set_error(const char *msg)
{
    strncpy(g_app.error_msg, msg, sizeof(g_app.error_msg) - 1);
    g_app.state = STATE_ERROR;
    LOG_ERROR("%s", msg);
}

void app_run(void)
{
    while (g_app.running) {
        input_update();

        if (input_pressed(BTN_START) && input_held(BTN_L) && input_held(BTN_R)) {
            g_app.running = false;
            continue;
        }

        ui_begin_frame();
        ui_draw_background();

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

        ui_end_frame();
        g_app.frame_count++;
    }
}

void app_shutdown(void)
{
    LOG("Shutting down...");
    audio_stop();
    audio_shutdown();
    network_shutdown();
    ui_shutdown();
    config_save(&g_app.settings);
}

static void state_init(void)
{
    ui_draw_loading("Starting...");
}

static void state_network(void)
{
    static int phase = 0;
    static int timeout = 0;

    switch (phase) {
        case 0:
            ui_draw_loading("Detecting network...");
            phase = 1;
            timeout = 0;
            break;
        case 1:
            if (network_init() == 0) {
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
                    app_set_error("No network adapter.\nN64 requires 64DD modem\nor Randnet adapter.");
                    phase = 0;
                }
                ui_draw_loading("Initializing network...");
            }
            break;
    }
}

static void state_connecting(void)
{
    static bool started = false;

    ui_draw_loading("Connecting...");

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
            app_set_error("Cannot connect to server.");
        }
    }
}

static void state_login(void)
{
    static int selected = 0;

    ui_draw_header("Login");

    const char *options[] = {
        "Use saved credentials",
        "Browse as guest",
        "Settings",
        "Exit"
    };

    ui_draw_menu(options, 4, selected);
    ui_draw_text(20, 180, "Configure on PC first", COLOR_TEXT_DIM);

    if (input_pressed(BTN_DPAD_UP)) selected = (selected - 1 + 4) % 4;
    if (input_pressed(BTN_DPAD_DOWN)) selected = (selected + 1) % 4;

    if (input_pressed(BTN_A)) {
        switch (selected) {
            case 0:
                if (strlen(g_app.settings.session_token) > 0)
                    g_app.state = STATE_MENU;
                break;
            case 1: g_app.state = STATE_MENU; break;
            case 2: g_app.state = STATE_SETTINGS; break;
            case 3: g_app.running = false; break;
        }
    }
}

static void state_menu(void)
{
    static int selected = 0;

    ui_draw_header("Nedflix");

    const char *options[] = {
        "Music [Best]",
        "Audiobooks",
        "Movies [Limited]",
        "TV Shows [Limited]",
        "Settings"
    };

    ui_draw_menu(options, 5, selected);
    ui_draw_text(20, 200, "Audio works best on N64", COLOR_TEXT_DIM);

    if (input_pressed(BTN_DPAD_UP)) selected = (selected - 1 + 5) % 5;
    if (input_pressed(BTN_DPAD_DOWN)) selected = (selected + 1) % 5;

    if (input_pressed(BTN_A)) {
        if (selected < 4) {
            g_app.current_library = (library_t)selected;
            strncpy(g_app.media.current_path, lib_paths[selected], MAX_PATH_LENGTH - 1);
            g_app.media.count = 0;
            g_app.media.selected_index = 0;
            g_app.media.scroll_offset = 0;
#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token, g_app.media.current_path,
                       g_app.current_library, &g_app.media);
#endif
            g_app.state = STATE_BROWSING;
        } else {
            g_app.state = STATE_SETTINGS;
        }
    }

    if (input_pressed(BTN_B)) g_app.running = false;
}

static void state_browsing(void)
{
    ui_draw_header(lib_names[g_app.current_library]);
    ui_draw_media_list(&g_app.media);

    if (input_pressed(BTN_DPAD_UP) && g_app.media.selected_index > 0) {
        g_app.media.selected_index--;
        if (g_app.media.selected_index < g_app.media.scroll_offset)
            g_app.media.scroll_offset--;
    }
    if (input_pressed(BTN_DPAD_DOWN) && g_app.media.selected_index < g_app.media.count - 1) {
        g_app.media.selected_index++;
        if (g_app.media.selected_index >= g_app.media.scroll_offset + MAX_ITEMS_VISIBLE)
            g_app.media.scroll_offset++;
    }

    if (input_pressed(BTN_L)) {
        g_app.current_library = (g_app.current_library - 1 + LIBRARY_COUNT) % LIBRARY_COUNT;
        strncpy(g_app.media.current_path, lib_paths[g_app.current_library], MAX_PATH_LENGTH - 1);
        g_app.media.count = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.current_path,
                   g_app.current_library, &g_app.media);
#endif
    }
    if (input_pressed(BTN_R)) {
        g_app.current_library = (g_app.current_library + 1) % LIBRARY_COUNT;
        strncpy(g_app.media.current_path, lib_paths[g_app.current_library], MAX_PATH_LENGTH - 1);
        g_app.media.count = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.current_path,
                   g_app.current_library, &g_app.media);
#endif
    }

    if (input_pressed(BTN_A) && g_app.media.count > 0) {
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
                                   stream_url, sizeof(stream_url)) == 0) {
                strncpy(g_app.playback.title, item->name, MAX_TITLE_LENGTH - 1);
                strncpy(g_app.playback.url, stream_url, MAX_URL_LENGTH - 1);
                g_app.playback.is_audio = (item->type == MEDIA_TYPE_AUDIO);

                if (item->type == MEDIA_TYPE_AUDIO) {
                    if (audio_play_stream(stream_url) == 0) {
                        g_app.playback.playing = true;
                        g_app.state = STATE_PLAYING;
                    }
                } else {
                    app_set_error("Video not supported.\n4MB RAM too limited.");
                }
            }
#endif
        }
    }

    if (input_pressed(BTN_B)) {
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
}

static void state_playing(void)
{
    g_app.playback.position_ms = audio_get_position();
    g_app.playback.duration_ms = audio_get_duration();
    g_app.playback.playing = audio_is_playing();

    ui_draw_playback(&g_app.playback);

    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        if (g_app.playback.paused) {
            audio_resume();
            g_app.playback.paused = false;
        } else {
            audio_pause();
            g_app.playback.paused = true;
        }
    }

    if (input_pressed(BTN_B)) {
        audio_stop();
        g_app.playback.playing = false;
        g_app.state = STATE_BROWSING;
    }

    if (input_pressed(BTN_C_UP)) {
        g_app.settings.volume = MIN(100, g_app.settings.volume + 5);
        audio_set_volume(g_app.settings.volume);
    }
    if (input_pressed(BTN_C_DOWN)) {
        g_app.settings.volume = MAX(0, g_app.settings.volume - 5);
        audio_set_volume(g_app.settings.volume);
    }

    if (!g_app.playback.playing && !g_app.playback.paused &&
        g_app.playback.position_ms > 0 &&
        g_app.playback.position_ms >= g_app.playback.duration_ms - 1000) {
        g_app.state = STATE_BROWSING;
    }
}

static void state_settings(void)
{
    static int selected = 0;

    ui_draw_header("Settings");

    char vol_str[32];
    snprintf(vol_str, sizeof(vol_str), "Volume: %d%%", g_app.settings.volume);

    const char *options[] = {
        g_app.settings.server_url[0] ? g_app.settings.server_url : "Server: (not set)",
        vol_str,
        "Save to Controller Pak",
        "Back"
    };

    ui_draw_menu(options, 4, selected);

    if (input_pressed(BTN_DPAD_UP)) selected = (selected - 1 + 4) % 4;
    if (input_pressed(BTN_DPAD_DOWN)) selected = (selected + 1) % 4;

    if (selected == 1) {
        if (input_pressed(BTN_DPAD_LEFT))
            g_app.settings.volume = MAX(0, g_app.settings.volume - 5);
        if (input_pressed(BTN_DPAD_RIGHT))
            g_app.settings.volume = MIN(100, g_app.settings.volume + 5);
    }

    if (input_pressed(BTN_A)) {
        switch (selected) {
            case 2: config_save(&g_app.settings); break;
            case 3: g_app.state = STATE_MENU; break;
        }
    }

    if (input_pressed(BTN_B)) g_app.state = STATE_MENU;
}

static void state_error(void)
{
    ui_draw_error(g_app.error_msg);
    ui_draw_text_centered(200, "A:Retry B:Exit", COLOR_TEXT);

    if (input_pressed(BTN_A)) g_app.state = STATE_NETWORK_INIT;
    if (input_pressed(BTN_B)) g_app.running = false;
}

int main(void)
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    dfs_init(DFS_DEFAULT_LOCATION);
    rdpq_init();
    controller_init();
    audio_init();
    timer_init();

    app_init();
    app_run();
    app_shutdown();

    return 0;
}
