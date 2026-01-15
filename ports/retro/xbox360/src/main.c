/*
 * Nedflix for Xbox 360
 * Main entry point
 *
 * TECHNICAL DEMO / NOVELTY PORT
 * Demonstrates homebrew on Xbox 360 using libxenon.
 * Requires JTAG/RGH modified console.
 */

#include "nedflix.h"

/* Global application context */
app_t g_app;

/* Library names */
static const char *library_names[] = {
    "Music",
    "Audiobooks",
    "Movies",
    "TV Shows"
};

/* Default paths */
static const char *library_paths[] = {
    "/Music",
    "/Audiobooks",
    "/Movies",
    "/TV Shows"
};

/*
 * State handler prototypes
 */
static void handle_state_init(void);
static void handle_state_network_init(void);
static void handle_state_connecting(void);
static void handle_state_login(void);
static void handle_state_menu(void);
static void handle_state_browsing(void);
static void handle_state_playing(void);
static void handle_state_settings(void);
static void handle_state_error(void);

/*
 * Set error state
 */
void app_set_error(const char *msg)
{
    strncpy(g_app.error_msg, msg, sizeof(g_app.error_msg) - 1);
    g_app.error_msg[sizeof(g_app.error_msg) - 1] = '\0';
    g_app.state = STATE_ERROR;
}

/*
 * Initialize the application
 */
void app_init(void)
{
    /* Initialize Xenon hardware */
    xenon_make_it_faster(XENON_SPEED_FULL);

    /* Initialize console for debug output */
    console_init();

    printf("\n");
    printf("=========================================\n");
    printf("  Nedflix for Xbox 360\n");
    printf("  Version %s\n", NEDFLIX_VERSION);
    printf("  TECHNICAL DEMO - Requires JTAG/RGH\n");
    printf("=========================================\n\n");

    /* Clear application state */
    memset(&g_app, 0, sizeof(g_app));
    g_app.state = STATE_INIT;
    g_app.running = true;
    g_app.current_library = LIBRARY_MUSIC;

    /* Set default settings */
    config_set_defaults(&g_app.settings);

    /* Initialize USB for storage and controllers */
    printf("Initializing USB...\n");
    usb_init();
    usb_do_poll();

    /* Initialize UI/Graphics */
    printf("Initializing graphics...\n");
    if (ui_init() != 0) {
        printf("ERROR: Failed to initialize UI\n");
        app_set_error("Failed to initialize graphics");
        return;
    }

    /* Initialize input */
    printf("Initializing input...\n");
    if (input_init() != 0) {
        printf("ERROR: Failed to initialize input\n");
        app_set_error("Failed to initialize input");
        return;
    }

    /* Initialize audio */
    printf("Initializing audio...\n");
    if (audio_init() != 0) {
        printf("WARNING: Audio initialization failed\n");
        /* Non-fatal */
    }

    /* Allocate media list */
    g_app.media.capacity = MAX_MEDIA_ITEMS;
    g_app.media.items = (media_item_t *)malloc(sizeof(media_item_t) * g_app.media.capacity);
    if (!g_app.media.items) {
        printf("ERROR: Failed to allocate media list\n");
        app_set_error("Out of memory");
        return;
    }
    g_app.media.count = 0;

    /* Load saved configuration */
    config_load(&g_app.settings);

    printf("Initialization complete!\n");
    printf("Press A to continue...\n\n");

    /* Wait for user input */
    while (1) {
        usb_do_poll();
        input_update();
        if (input_button_just_pressed(BTN_A)) {
            break;
        }
        mdelay(16);  /* ~60 fps */
    }

    /* Move to network initialization */
    g_app.state = STATE_NETWORK_INIT;
}

/*
 * Shutdown the application
 */
void app_shutdown(void)
{
    printf("Shutting down...\n");

    /* Stop playback */
    audio_stop();

    /* Free resources */
    if (g_app.media.items) {
        free(g_app.media.items);
        g_app.media.items = NULL;
    }

    /* Save configuration */
    config_save(&g_app.settings);

    /* Shutdown subsystems */
    audio_shutdown();
    network_shutdown();
    input_shutdown();
    ui_shutdown();

    printf("Shutdown complete\n");
}

/*
 * Main application loop
 */
void app_run(void)
{
    while (g_app.running) {
        /* Poll USB devices */
        usb_do_poll();

        /* Update input */
        input_update();

        /* Global back button handling */
        if (input_button_just_pressed(BTN_B)) {
            switch (g_app.state) {
                case STATE_PLAYING:
                    audio_stop();
                    g_app.state = STATE_BROWSING;
                    break;
                case STATE_BROWSING:
                    if (strlen(g_app.media.current_path) > 1) {
                        char *last_slash = strrchr(g_app.media.current_path, '/');
                        if (last_slash && last_slash != g_app.media.current_path) {
                            *last_slash = '\0';
                            g_app.media.selected_index = 0;
                            g_app.media.scroll_offset = 0;
                        }
                    } else {
                        g_app.state = STATE_MENU;
                    }
                    break;
                case STATE_SETTINGS:
                    g_app.state = STATE_MENU;
                    break;
                case STATE_MENU:
                    /* Could return to login or do nothing */
                    break;
                default:
                    break;
            }
        }

        /* Guide button for settings */
        if (input_button_just_pressed(BTN_GUIDE) || input_button_just_pressed(BTN_BACK)) {
            if (g_app.state == STATE_BROWSING || g_app.state == STATE_MENU) {
                g_app.state = STATE_SETTINGS;
            } else if (g_app.state == STATE_SETTINGS) {
                g_app.state = STATE_MENU;
            }
        }

        /* Begin rendering */
        ui_begin_frame();
        ui_clear(COLOR_DARK_BG);

        /* State machine */
        switch (g_app.state) {
            case STATE_INIT:
                handle_state_init();
                break;
            case STATE_NETWORK_INIT:
                handle_state_network_init();
                break;
            case STATE_CONNECTING:
                handle_state_connecting();
                break;
            case STATE_LOGIN:
                handle_state_login();
                break;
            case STATE_MENU:
                handle_state_menu();
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

        /* Update audio streaming */
        if (g_app.state == STATE_PLAYING) {
            audio_update();
        }

        /* Frame timing (~60 fps) */
        mdelay(16);
        g_app.frame_count++;
    }
}

/*
 * State handlers
 */

static void handle_state_init(void)
{
    ui_draw_loading("Initializing...");
}

static void handle_state_network_init(void)
{
    static bool initializing = false;
    static int result = -1;

    ui_draw_loading("Initializing network...");

    if (!initializing) {
        initializing = true;
        result = network_init();
    }

    if (result == 0) {
        printf("Network ready, IP: %d.%d.%d.%d\n",
               (g_app.net.ip_addr >> 24) & 0xFF,
               (g_app.net.ip_addr >> 16) & 0xFF,
               (g_app.net.ip_addr >> 8) & 0xFF,
               g_app.net.ip_addr & 0xFF);
        g_app.net.initialized = true;
        initializing = false;

        if (strlen(g_app.settings.server_url) > 0) {
            g_app.state = STATE_CONNECTING;
        } else {
            g_app.state = STATE_SETTINGS;
        }
    } else if (result < 0) {
        /* Network init failed - can still use local media */
        printf("Network initialization failed, continuing offline\n");
        initializing = false;
        g_app.state = STATE_MENU;
    }
}

static void handle_state_connecting(void)
{
    static bool connecting = false;
    static int result = 0;

    char msg[128];
    snprintf(msg, sizeof(msg), "Connecting to %s...", g_app.settings.server_url);
    ui_draw_loading(msg);

    if (!connecting) {
        connecting = true;
        result = api_init(g_app.settings.server_url);
    }

    if (result == 0) {
        connecting = false;
        if (strlen(g_app.settings.auth_token) > 0) {
            char username[64] = {0};
            if (api_get_user_info(g_app.settings.auth_token, username, sizeof(username)) == 0) {
                strncpy(g_app.settings.username, username, sizeof(g_app.settings.username) - 1);
                g_app.state = STATE_MENU;
            } else {
                g_app.state = STATE_LOGIN;
            }
        } else {
            g_app.state = STATE_LOGIN;
        }
    } else {
        connecting = false;
        char err[256];
        snprintf(err, sizeof(err), "Failed to connect to %s", g_app.settings.server_url);
        app_set_error(err);
    }
}

static void handle_state_login(void)
{
    ui_draw_header("Login");

    static int selected = 0;
    const char *menu[] = {
        "Use saved credentials",
        "Enter new credentials",
        "Change server",
        "Continue offline"
    };

    ui_draw_menu(menu, 4, selected);

    if (input_button_just_pressed(BTN_DPAD_UP)) {
        selected = (selected - 1 + 4) % 4;
    }
    if (input_button_just_pressed(BTN_DPAD_DOWN)) {
        selected = (selected + 1) % 4;
    }

    if (input_button_just_pressed(BTN_A)) {
        switch (selected) {
            case 0:  /* Use saved */
                if (strlen(g_app.settings.auth_token) > 0) {
                    g_app.state = STATE_MENU;
                }
                break;
            case 1:  /* Enter credentials - would need OSK */
                /* TODO: On-screen keyboard */
                break;
            case 2:  /* Change server */
                g_app.state = STATE_SETTINGS;
                break;
            case 3:  /* Offline */
                g_app.state = STATE_MENU;
                break;
        }
    }

    ui_draw_text(20, SCREEN_HEIGHT - 30, "A: Select   B: Back", COLOR_TEXT_DIM);
}

static void handle_state_menu(void)
{
    ui_draw_header("Nedflix");

    static int selected = 0;
    const char *menu[] = {
        "Music",
        "Audiobooks",
        "Movies",
        "TV Shows",
        "Settings",
        "Exit"
    };

    ui_draw_menu(menu, 6, selected);

    /* Show username if logged in */
    if (strlen(g_app.settings.username) > 0) {
        char user_str[128];
        snprintf(user_str, sizeof(user_str), "Logged in as: %s", g_app.settings.username);
        ui_draw_text(SCREEN_WIDTH - 300, 60, user_str, COLOR_TEXT_DIM);
    }

    if (input_button_just_pressed(BTN_DPAD_UP)) {
        selected = (selected - 1 + 6) % 6;
    }
    if (input_button_just_pressed(BTN_DPAD_DOWN)) {
        selected = (selected + 1) % 6;
    }

    if (input_button_just_pressed(BTN_A)) {
        if (selected < 4) {
            /* Library selection */
            g_app.current_library = (library_t)selected;
            strncpy(g_app.media.current_path, library_paths[selected],
                    sizeof(g_app.media.current_path) - 1);
            g_app.media.selected_index = 0;
            g_app.media.scroll_offset = 0;
            g_app.media.count = 0;

            if (g_app.net.initialized && strlen(g_app.settings.auth_token) > 0) {
                api_browse(g_app.settings.auth_token, g_app.media.current_path,
                           g_app.current_library, &g_app.media);
            }
            g_app.state = STATE_BROWSING;
        } else if (selected == 4) {
            g_app.state = STATE_SETTINGS;
        } else {
            g_app.running = false;
        }
    }

    ui_draw_text(20, SCREEN_HEIGHT - 30, "A: Select   Back: Settings", COLOR_TEXT_DIM);
}

static void handle_state_browsing(void)
{
    char header[128];
    snprintf(header, sizeof(header), "Nedflix - %s", library_names[g_app.current_library]);
    ui_draw_header(header);

    ui_draw_file_list(&g_app.media);

    /* Navigation */
    if (input_button_just_pressed(BTN_DPAD_UP)) {
        if (g_app.media.selected_index > 0) {
            g_app.media.selected_index--;
            if (g_app.media.selected_index < g_app.media.scroll_offset) {
                g_app.media.scroll_offset--;
            }
        }
    }
    if (input_button_just_pressed(BTN_DPAD_DOWN)) {
        if (g_app.media.selected_index < g_app.media.count - 1) {
            g_app.media.selected_index++;
            if (g_app.media.selected_index >= g_app.media.scroll_offset + MAX_ITEMS_VISIBLE) {
                g_app.media.scroll_offset++;
            }
        }
    }

    /* Page up/down with triggers */
    if (input_get_left_trigger() > 200) {
        g_app.media.selected_index = MAX(0, g_app.media.selected_index - MAX_ITEMS_VISIBLE);
        g_app.media.scroll_offset = MAX(0, g_app.media.scroll_offset - MAX_ITEMS_VISIBLE);
    }
    if (input_get_right_trigger() > 200) {
        g_app.media.selected_index = MIN(g_app.media.count - 1,
                                          g_app.media.selected_index + MAX_ITEMS_VISIBLE);
        if (g_app.media.selected_index >= g_app.media.scroll_offset + MAX_ITEMS_VISIBLE) {
            g_app.media.scroll_offset = g_app.media.selected_index - MAX_ITEMS_VISIBLE + 1;
        }
    }

    /* Library switching with bumpers */
    if (input_button_just_pressed(BTN_LB)) {
        g_app.current_library = (g_app.current_library - 1 + LIBRARY_COUNT) % LIBRARY_COUNT;
        strncpy(g_app.media.current_path, library_paths[g_app.current_library],
                sizeof(g_app.media.current_path) - 1);
        g_app.media.selected_index = 0;
        g_app.media.scroll_offset = 0;
        if (g_app.net.initialized) {
            api_browse(g_app.settings.auth_token, g_app.media.current_path,
                       g_app.current_library, &g_app.media);
        }
    }
    if (input_button_just_pressed(BTN_RB)) {
        g_app.current_library = (g_app.current_library + 1) % LIBRARY_COUNT;
        strncpy(g_app.media.current_path, library_paths[g_app.current_library],
                sizeof(g_app.media.current_path) - 1);
        g_app.media.selected_index = 0;
        g_app.media.scroll_offset = 0;
        if (g_app.net.initialized) {
            api_browse(g_app.settings.auth_token, g_app.media.current_path,
                       g_app.current_library, &g_app.media);
        }
    }

    /* Select item */
    if (input_button_just_pressed(BTN_A) && g_app.media.count > 0) {
        media_item_t *item = &g_app.media.items[g_app.media.selected_index];

        if (item->is_directory) {
            strncpy(g_app.media.current_path, item->path,
                    sizeof(g_app.media.current_path) - 1);
            g_app.media.selected_index = 0;
            g_app.media.scroll_offset = 0;
            if (g_app.net.initialized) {
                api_browse(g_app.settings.auth_token, g_app.media.current_path,
                           g_app.current_library, &g_app.media);
            }
        } else if (item->type == MEDIA_TYPE_AUDIO || item->type == MEDIA_TYPE_VIDEO) {
            char stream_url[MAX_URL_LENGTH];
            if (api_get_stream_url(g_app.settings.auth_token, item->path,
                                    stream_url, sizeof(stream_url)) == 0) {
                strncpy(g_app.playback.title, item->name,
                        sizeof(g_app.playback.title) - 1);
                strncpy(g_app.playback.url, stream_url,
                        sizeof(g_app.playback.url) - 1);
                g_app.playback.is_audio = (item->type == MEDIA_TYPE_AUDIO);

                if (audio_play(stream_url) == 0) {
                    g_app.state = STATE_PLAYING;
                }
            }
        }
    }

    /* Help text */
    ui_draw_text(20, SCREEN_HEIGHT - 30,
                 "A: Select   B: Back   LB/RB: Library   LT/RT: Page", COLOR_TEXT_DIM);

    if (g_app.media.count == 0) {
        ui_draw_text_centered(SCREEN_HEIGHT / 2, "No items found", COLOR_TEXT_DIM);
    }
}

static void handle_state_playing(void)
{
    /* Update playback state */
    g_app.playback.playing = audio_is_playing();
    g_app.playback.position = audio_get_position();
    g_app.playback.duration = audio_get_duration();
    g_app.playback.volume = g_app.settings.volume;

    /* Draw playback HUD */
    ui_draw_playback_hud(&g_app.playback);

    /* Play/Pause with A or X */
    if (input_button_just_pressed(BTN_A) || input_button_just_pressed(BTN_X)) {
        if (g_app.playback.paused) {
            audio_resume();
            g_app.playback.paused = false;
        } else {
            audio_pause();
            g_app.playback.paused = true;
        }
    }

    /* Volume with D-pad left/right */
    if (input_button_just_pressed(BTN_DPAD_LEFT)) {
        g_app.settings.volume = MAX(0, g_app.settings.volume - 5);
        audio_set_volume(g_app.settings.volume);
    }
    if (input_button_just_pressed(BTN_DPAD_RIGHT)) {
        g_app.settings.volume = MIN(100, g_app.settings.volume + 5);
        audio_set_volume(g_app.settings.volume);
    }

    /* Seek with triggers */
    if (input_get_left_trigger() > 100) {
        audio_seek(g_app.playback.position - 10.0);
    }
    if (input_get_right_trigger() > 100) {
        audio_seek(g_app.playback.position + 10.0);
    }

    /* Check for playback end */
    if (!g_app.playback.playing && !g_app.playback.paused &&
        g_app.playback.position >= g_app.playback.duration - 0.5) {
        g_app.state = STATE_BROWSING;

        /* Auto-play next if enabled */
        if (g_app.settings.autoplay &&
            g_app.media.selected_index < g_app.media.count - 1) {
            g_app.media.selected_index++;
        }
    }
}

static void handle_state_settings(void)
{
    ui_draw_header("Settings");

    static int selected = 0;

    char server_str[128];
    char volume_str[64];
    char autoplay_str[64];

    snprintf(server_str, sizeof(server_str), "Server: %s",
             strlen(g_app.settings.server_url) > 0 ? g_app.settings.server_url : "(not set)");
    snprintf(volume_str, sizeof(volume_str), "Volume: %d%%", g_app.settings.volume);
    snprintf(autoplay_str, sizeof(autoplay_str), "Auto-play: %s",
             g_app.settings.autoplay ? "On" : "Off");

    const char *menu[] = {
        server_str,
        volume_str,
        autoplay_str,
        "Reconnect to Server",
        "Save & Back",
        "Cancel"
    };
    int menu_count = 6;

    ui_draw_menu(menu, menu_count, selected);

    if (input_button_just_pressed(BTN_DPAD_UP)) {
        selected = (selected - 1 + menu_count) % menu_count;
    }
    if (input_button_just_pressed(BTN_DPAD_DOWN)) {
        selected = (selected + 1) % menu_count;
    }

    /* Adjust values */
    if (input_button_just_pressed(BTN_DPAD_LEFT) || input_button_just_pressed(BTN_DPAD_RIGHT)) {
        int delta = input_button_just_pressed(BTN_DPAD_RIGHT) ? 1 : -1;

        switch (selected) {
            case 1:  /* Volume */
                g_app.settings.volume = CLAMP(g_app.settings.volume + delta * 5, 0, 100);
                audio_set_volume(g_app.settings.volume);
                break;
            case 2:  /* Autoplay */
                g_app.settings.autoplay = !g_app.settings.autoplay;
                break;
        }
    }

    if (input_button_just_pressed(BTN_A)) {
        switch (selected) {
            case 0:  /* Server URL - would need OSK */
                /* TODO: On-screen keyboard */
                break;
            case 3:  /* Reconnect */
                config_save(&g_app.settings);
                api_shutdown();
                g_app.state = STATE_CONNECTING;
                break;
            case 4:  /* Save & Back */
                config_save(&g_app.settings);
                g_app.state = STATE_MENU;
                break;
            case 5:  /* Cancel */
                config_load(&g_app.settings);
                g_app.state = STATE_MENU;
                break;
        }
    }

    ui_draw_text(20, SCREEN_HEIGHT - 30, "A: Select   D-Pad: Adjust   B: Back", COLOR_TEXT_DIM);
}

static void handle_state_error(void)
{
    ui_draw_error(g_app.error_msg);

    ui_draw_text_centered(SCREEN_HEIGHT - 100, "Press A to retry, B to continue offline", COLOR_TEXT);

    if (input_button_just_pressed(BTN_A)) {
        g_app.state = STATE_NETWORK_INIT;
    }
    if (input_button_just_pressed(BTN_B)) {
        g_app.state = STATE_MENU;
    }
}

/*
 * Main entry point
 */
int main(void)
{
    app_init();
    app_run();
    app_shutdown();

    /* Reboot console */
    xenon_smc_power_reboot();

    return 0;
}
