/*
 * Nedflix for Nintendo GameCube
 * Full-featured media client using devkitPPC
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

app_t g_app;

static const char *lib_names[] = {"Music", "Audiobooks", "Movies", "TV Shows"};
static const char *lib_paths[] = {"/Music", "/Audiobooks", "/Movies", "/TV Shows"};

void app_init(void)
{
    printf("Nedflix for GameCube v%s\n", NEDFLIX_VERSION);

    memset(&g_app, 0, sizeof(g_app));
    g_app.state = STATE_INIT;
    g_app.running = true;
    g_app.current_library = LIBRARY_MUSIC;

    config_defaults(&g_app.settings);
    config_load(&g_app.settings);

    ui_init();
    input_init();
    audio_init();
    video_init();

    g_app.state = STATE_NETWORK_INIT;
    printf("Init complete, RAM free: %d KB\n", (int)(SYS_GetArena1Hi() - SYS_GetArena1Lo()) / 1024);
}

void app_set_error(const char *msg)
{
    strncpy(g_app.error_msg, msg, sizeof(g_app.error_msg) - 1);
    g_app.state = STATE_ERROR;
    printf("Error: %s\n", msg);
}

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
            if (network_init() == 0) {
                printf("Network ready: %s\n", g_app.net.local_ip);
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
                    app_set_error("Network not found.\nRequires Broadband Adapter\nor USB Gecko bridge.");
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
            app_set_error("Cannot connect to server.\nCheck Settings.");
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
    ui_draw_text(40, 380, "Configure credentials on PC then transfer via SD.", COLOR_TEXT_DIM);

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
        "Music",
        "Audiobooks",
        "Movies",
        "TV Shows",
        "Settings"
    };

    ui_draw_menu(options, 5, selected);

    /* GC is capable - show it */
    ui_draw_text(40, 400, "GameCube: Full audio + basic video support!", COLOR_TEXT_DIM);
    ui_draw_text(40, 420, "485 MHz PowerPC + 24MB RAM", COLOR_TEXT_DIM);

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
    char header[64];
    snprintf(header, sizeof(header), "%s", lib_names[g_app.current_library]);
    ui_draw_header(header);

    ui_draw_media_list(&g_app.media);

    /* Navigation */
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

    /* Fast scroll with stick */
    int sy = input_get_stick_y();
    if (sy > 50 && g_app.media.selected_index > 0) {
        g_app.media.selected_index = MAX(0, g_app.media.selected_index - 3);
        g_app.media.scroll_offset = MAX(0, g_app.media.selected_index - 2);
    }
    if (sy < -50 && g_app.media.selected_index < g_app.media.count - 1) {
        g_app.media.selected_index = MIN(g_app.media.count - 1, g_app.media.selected_index + 3);
        if (g_app.media.selected_index >= g_app.media.scroll_offset + MAX_ITEMS_VISIBLE)
            g_app.media.scroll_offset = g_app.media.selected_index - MAX_ITEMS_VISIBLE + 1;
    }

    /* Library switch */
    if (input_pressed(BTN_L)) {
        g_app.current_library = (g_app.current_library - 1 + LIBRARY_COUNT) % LIBRARY_COUNT;
        strncpy(g_app.media.current_path, lib_paths[g_app.current_library], MAX_PATH_LENGTH - 1);
        g_app.media.count = 0;
        g_app.media.selected_index = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.current_path,
                   g_app.current_library, &g_app.media);
#endif
    }
    if (input_pressed(BTN_R)) {
        g_app.current_library = (g_app.current_library + 1) % LIBRARY_COUNT;
        strncpy(g_app.media.current_path, lib_paths[g_app.current_library], MAX_PATH_LENGTH - 1);
        g_app.media.count = 0;
        g_app.media.selected_index = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.current_path,
                   g_app.current_library, &g_app.media);
#endif
    }

    /* Select item */
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
                    /* GameCube CAN do video! */
                    if (video_play_stream(stream_url) == 0) {
                        g_app.playback.playing = true;
                        g_app.playback.is_audio = false;
                        g_app.state = STATE_PLAYING;
                    } else {
                        app_set_error("Video decode failed.\nTry lower quality setting.");
                    }
                }
            }
#endif
        }
    }

    /* Go back */
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

    ui_draw_text(20, 450, "A:Select  B:Back  L/R:Library  Stick:Fast scroll", COLOR_TEXT_DIM);
}

static void state_playing(void)
{
    if (g_app.playback.is_audio) {
        g_app.playback.position_ms = audio_get_position();
        g_app.playback.duration_ms = audio_get_duration();
        g_app.playback.playing = audio_is_playing();
        audio_update();
    } else {
        g_app.playback.playing = video_is_playing();
        video_render_frame();
    }

    ui_draw_playback(&g_app.playback);

    /* Pause/resume */
    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
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

    /* Stop */
    if (input_pressed(BTN_B)) {
        if (g_app.playback.is_audio) audio_stop();
        else video_stop();
        g_app.playback.playing = false;
        g_app.state = STATE_BROWSING;
    }

    /* Volume with triggers */
    if (g_app.ltrig > 100) {
        g_app.settings.volume = MAX(0, g_app.settings.volume - 1);
        audio_set_volume(g_app.settings.volume);
    }
    if (g_app.rtrig > 100) {
        g_app.settings.volume = MIN(100, g_app.settings.volume + 1);
        audio_set_volume(g_app.settings.volume);
    }

    /* Seek with D-pad */
    if (input_held(BTN_DPAD_LEFT) && g_app.playback.is_audio) {
        /* audio_seek(-10000); */
    }
    if (input_held(BTN_DPAD_RIGHT) && g_app.playback.is_audio) {
        /* audio_seek(10000); */
    }

    /* Track end */
    if (!g_app.playback.playing && !g_app.playback.paused) {
        g_app.state = STATE_BROWSING;
    }
}

static void state_settings(void)
{
    static int selected = 0;

    ui_draw_header("Settings");

    char vol_str[32];
    snprintf(vol_str, sizeof(vol_str), "Volume: %d%%", g_app.settings.volume);

    const char *quality_names[] = {"Low (240p)", "Medium (360p)", "High (480p)"};
    char qual_str[48];
    snprintf(qual_str, sizeof(qual_str), "Video Quality: %s", quality_names[g_app.settings.video_quality]);

    const char *options[] = {
        g_app.settings.server_url[0] ? g_app.settings.server_url : "Server: (not set)",
        vol_str,
        qual_str,
        "Save to Memory Card",
        "Back"
    };

    ui_draw_menu(options, 5, selected);

    if (input_pressed(BTN_DPAD_UP)) selected = (selected - 1 + 5) % 5;
    if (input_pressed(BTN_DPAD_DOWN)) selected = (selected + 1) % 5;

    if (selected == 1) {
        if (input_pressed(BTN_DPAD_LEFT)) g_app.settings.volume = MAX(0, g_app.settings.volume - 5);
        if (input_pressed(BTN_DPAD_RIGHT)) g_app.settings.volume = MIN(100, g_app.settings.volume + 5);
    }
    if (selected == 2) {
        if (input_pressed(BTN_DPAD_LEFT)) g_app.settings.video_quality = (g_app.settings.video_quality + 2) % 3;
        if (input_pressed(BTN_DPAD_RIGHT)) g_app.settings.video_quality = (g_app.settings.video_quality + 1) % 3;
    }

    if (input_pressed(BTN_A)) {
        if (selected == 3) config_save(&g_app.settings);
        else if (selected == 4) g_app.state = STATE_MENU;
    }

    if (input_pressed(BTN_B)) g_app.state = STATE_MENU;
}

static void state_error(void)
{
    ui_draw_error(g_app.error_msg);
    ui_draw_text_centered(380, "A: Retry   B: Exit", COLOR_TEXT);

    if (input_pressed(BTN_A)) g_app.state = STATE_NETWORK_INIT;
    if (input_pressed(BTN_B)) g_app.running = false;
}

void app_run(void)
{
    while (g_app.running) {
        input_update();

        /* Global exit */
        if (input_held(BTN_START) && input_held(BTN_Z) && input_held(BTN_L) && input_held(BTN_R)) {
            g_app.running = false;
            continue;
        }

        ui_begin_frame();

        switch (g_app.state) {
            case STATE_INIT:        ui_draw_loading("Starting..."); break;
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

        VIDEO_WaitVSync();
    }
}

void app_shutdown(void)
{
    printf("Shutting down...\n");

    audio_stop();
    video_stop();
    audio_shutdown();
    video_shutdown();
    network_shutdown();
    ui_shutdown();

    config_save(&g_app.settings);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    VIDEO_Init();
    PAD_Init();
    AUDIO_Init(NULL);

    g_app.rmode = VIDEO_GetPreferredMode(NULL);
    g_app.xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(g_app.rmode));

    VIDEO_Configure(g_app.rmode);
    VIDEO_SetNextFramebuffer(g_app.xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    CON_Init(g_app.xfb, 20, 20, g_app.rmode->fbWidth, g_app.rmode->xfbHeight,
             g_app.rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    app_init();
    app_run();
    app_shutdown();

    return 0;
}
