/*
 * Nedflix for PlayStation 3
 * Fully Functional Feature-Complete Port
 *
 * Main application entry point and state machine
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <sys/process.h>
#include <sys/time.h>
#include <sysutil/sysutil.h>

/* Global application instance */
app_t g_app;

/* SYS_PROCESS_PARAM for PSL1GHT */
SYS_PROCESS_PARAM(1001, 0x200000);  /* 2MB stack */

/* Library display names */
static const char *lib_names[] = {
    "Home",
    "Music",
    "Audiobooks",
    "Movies",
    "TV Shows",
    "Live TV",
    "My List",
    "Continue Watching",
    "Search"
};

/* Library paths */
static const char *lib_paths[] = {
    "/",
    "/Music",
    "/Audiobooks",
    "/Movies",
    "/TV Shows",
    "/LiveTV",
    "/Favorites",
    "/History",
    "/Search"
};

/* Main menu options */
static const char *main_menu_options[] = {
    "Movies",
    "TV Shows",
    "Music",
    "Audiobooks",
    "Live TV",
    "My List",
    "Continue Watching",
    "Search",
    "Settings"
};

/* Settings pages */
static const char *settings_pages[] = {
    "Connection",
    "Playback",
    "Audio",
    "Display",
    "Network",
    "Account",
    "About"
};

/* XMB system callback */
static void sysutil_callback(u64 status, u64 param, void *userdata)
{
    (void)param;
    (void)userdata;

    switch (status) {
        case SYSUTIL_EXIT_GAME:
            printf("Exit requested via XMB\n");
            g_app.running = false;
            break;
        case SYSUTIL_DRAW_BEGIN:
        case SYSUTIL_DRAW_END:
            break;
        case SYSUTIL_OSK_LOADED:
            printf("OSK loaded\n");
            break;
        case SYSUTIL_OSK_DONE:
            g_app.osk.done = true;
            break;
        case SYSUTIL_OSK_UNLOADED:
            g_app.osk.active = false;
            break;
    }
}

/* Get current time in milliseconds */
uint64_t get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* Get current time in microseconds */
uint64_t get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* Format duration as HH:MM:SS or MM:SS */
void format_duration(uint32_t ms, char *buf, size_t len)
{
    uint32_t total_sec = ms / 1000;
    uint32_t hours = total_sec / 3600;
    uint32_t mins = (total_sec % 3600) / 60;
    uint32_t secs = total_sec % 60;

    if (hours > 0) {
        snprintf(buf, len, "%u:%02u:%02u", hours, mins, secs);
    } else {
        snprintf(buf, len, "%u:%02u", mins, secs);
    }
}

/* Format file size */
void format_size(uint64_t bytes, char *buf, size_t len)
{
    if (bytes >= 1024ULL * 1024 * 1024) {
        snprintf(buf, len, "%.1f GB", bytes / (1024.0 * 1024 * 1024));
    } else if (bytes >= 1024 * 1024) {
        snprintf(buf, len, "%.1f MB", bytes / (1024.0 * 1024));
    } else if (bytes >= 1024) {
        snprintf(buf, len, "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(buf, len, "%llu B", (unsigned long long)bytes);
    }
}

/* Format timestamp */
void format_date(uint64_t timestamp, char *buf, size_t len)
{
    time_t t = (time_t)timestamp;
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M", tm);
}

/* Trim whitespace */
void str_trim(char *s)
{
    char *start = s;
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) start++;

    char *end = s + strlen(s) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;

    memmove(s, start, end - start + 1);
    s[end - start + 1] = '\0';
}

/* Truncate string with suffix */
void str_truncate(char *s, int max_len, const char *suffix)
{
    if ((int)strlen(s) <= max_len) return;

    int suffix_len = suffix ? strlen(suffix) : 0;
    if (max_len <= suffix_len) return;

    s[max_len - suffix_len] = '\0';
    if (suffix) strcat(s, suffix);
}

/* URL encode */
void url_encode(const char *input, char *output, size_t output_len)
{
    static const char *hex = "0123456789ABCDEF";
    size_t j = 0;

    for (size_t i = 0; input[i] && j < output_len - 3; i++) {
        unsigned char c = input[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            output[j++] = c;
        } else {
            output[j++] = '%';
            output[j++] = hex[(c >> 4) & 0xF];
            output[j++] = hex[c & 0xF];
        }
    }
    output[j] = '\0';
}

/* State handler prototypes */
static void state_init(void);
static void state_splash(void);
static void state_network(void);
static void state_connecting(void);
static void state_login(void);
static void state_profile_select(void);
static void state_main_menu(void);
static void state_browsing(void);
static void state_search(void);
static void state_detail(void);
static void state_playing(void);
static void state_live_tv(void);
static void state_channels(void);
static void state_settings(void);
static void state_about(void);
static void state_error(void);

/*
 * Initialize application
 */
void app_init(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  Nedflix for PlayStation 3\n");
    printf("  Version %s\n", NEDFLIX_VERSION);
    printf("  Build: %s\n", NEDFLIX_BUILD_DATE);
    printf("========================================\n\n");

    /* Clear state */
    memset(&g_app, 0, sizeof(g_app));
    g_app.state = STATE_INIT;
    g_app.running = true;
    g_app.current_library = LIBRARY_HOME;

    /* Register XMB callback */
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_callback, NULL);

    /* Load default config first */
    config_defaults(&g_app.settings);

    /* Then try to load saved config */
    if (config_load(&g_app.settings) == 0) {
        printf("Loaded saved configuration\n");
    }

    /* Load favorites and history */
    config_load_favorites();
    config_load_history();

    /* Initialize subsystems */
    printf("Initializing graphics...\n");
    if (ui_init() != 0) {
        app_set_error("Failed to initialize graphics");
        return;
    }

    printf("Initializing input...\n");
    if (input_init() != 0) {
        app_set_error("Failed to initialize input");
        return;
    }

    printf("Initializing audio...\n");
    if (audio_init() != 0) {
        printf("Warning: Audio init failed (will retry)\n");
    }

    printf("Initializing video...\n");
    if (video_init() != 0) {
        printf("Warning: Video init failed (will retry)\n");
    }

    /* Initialize thumbnail cache */
    thumb_cache_init();

    /* Allocate media list */
    g_app.media.items = calloc(MAX_MEDIA_ITEMS, sizeof(media_item_t));
    g_app.media.capacity = MAX_MEDIA_ITEMS;

    /* Allocate channel list */
    g_app.channels.channels = calloc(MAX_CHANNELS, sizeof(channel_t));
    g_app.channels.capacity = MAX_CHANNELS;

    /* Allocate search results */
    g_app.search.results.items = calloc(MAX_SEARCH_RESULTS, sizeof(media_item_t));
    g_app.search.results.capacity = MAX_SEARCH_RESULTS;

    /* Initialize animation state */
    g_app.anim.last_frame_time = get_time_ms();

    /* Start with splash screen */
    g_app.state = STATE_SPLASH;
    g_app.uptime_ms = get_time_ms();

    printf("Initialization complete\n\n");
}

/*
 * Set error state with message
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
 * Show toast notification
 */
void app_show_toast(const char *msg, int duration_ms)
{
    strncpy(g_app.toast_msg, msg, sizeof(g_app.toast_msg) - 1);
    g_app.toast_timer = get_time_ms() + duration_ms;
}

/*
 * Transition to a new state
 */
void app_transition_to(app_state_t state)
{
    g_app.prev_state = g_app.state;
    g_app.state = state;
    g_app.anim.detail_fade = 0.0f;
    g_app.anim.list_offset = 0.0f;
}

/*
 * Update animations
 */
static void update_animations(float dt)
{
    /* Smooth scroll animation */
    float target_offset = g_app.media.scroll_offset * 50.0f;
    g_app.anim.list_offset = LERP(g_app.anim.list_offset, target_offset, dt * 10.0f);

    /* Fade animations */
    if (g_app.state == STATE_DETAIL) {
        g_app.anim.detail_fade = MIN(1.0f, g_app.anim.detail_fade + dt * 5.0f);
    } else {
        g_app.anim.detail_fade = MAX(0.0f, g_app.anim.detail_fade - dt * 5.0f);
    }

    /* Overlay fade */
    if (g_app.state == STATE_PLAYING || g_app.state == STATE_LIVE_TV) {
        g_app.anim.overlay_alpha = MAX(0.0f, g_app.anim.overlay_alpha - dt * 2.0f);
    } else {
        g_app.anim.overlay_alpha = MIN(1.0f, g_app.anim.overlay_alpha + dt * 3.0f);
    }

    /* Progress bar glow */
    g_app.anim.progress_glow += dt * 2.0f;
    if (g_app.anim.progress_glow > 6.28f) g_app.anim.progress_glow -= 6.28f;
}

/*
 * Main application loop
 */
void app_run(void)
{
    uint64_t last_time = get_time_us();

    while (g_app.running) {
        /* Calculate delta time */
        uint64_t now = get_time_us();
        g_app.frame_time_us = now - last_time;
        last_time = now;
        float dt = g_app.frame_time_us / 1000000.0f;
        if (dt > 0.1f) dt = 0.1f;  /* Cap delta time */

        /* Update FPS counter */
        static uint32_t fps_frames = 0;
        static uint64_t fps_timer = 0;
        fps_frames++;
        if (now - fps_timer >= 1000000) {
            g_app.fps = fps_frames;
            fps_frames = 0;
            fps_timer = now;
        }

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

        /* Toggle debug overlay: Select + Start */
        if (input_held(BTN_SELECT) && input_pressed(BTN_START)) {
            g_app.show_debug = !g_app.show_debug;
        }

        /* Update OSK if active */
        if (g_app.osk.active) {
            ui_update_osk();
        }

        /* Update animations */
        update_animations(dt);

        /* Begin frame */
        ui_begin_frame();

        /* Handle OSK overlay */
        if (g_app.osk.active) {
            /* Draw current state dimmed behind OSK */
            /* OSK is handled by system */
        } else {
            /* Run state handler */
            switch (g_app.state) {
                case STATE_INIT:          state_init(); break;
                case STATE_SPLASH:        state_splash(); break;
                case STATE_NETWORK_INIT:  state_network(); break;
                case STATE_CONNECTING:    state_connecting(); break;
                case STATE_LOGIN:         state_login(); break;
                case STATE_PROFILE_SELECT: state_profile_select(); break;
                case STATE_MAIN_MENU:     state_main_menu(); break;
                case STATE_BROWSING:      state_browsing(); break;
                case STATE_SEARCH:        state_search(); break;
                case STATE_DETAIL:        state_detail(); break;
                case STATE_PLAYING:       state_playing(); break;
                case STATE_LIVE_TV:       state_live_tv(); break;
                case STATE_CHANNELS:      state_channels(); break;
                case STATE_SETTINGS:      state_settings(); break;
                case STATE_ABOUT:         state_about(); break;
                case STATE_ERROR:         state_error(); break;
                default: break;
            }
        }

        /* Draw toast if active */
        if (g_app.toast_timer > get_time_ms()) {
            ui_draw_toast(g_app.toast_msg);
        }

        /* Draw debug overlay */
        if (g_app.show_debug) {
            char debug[256];
            snprintf(debug, sizeof(debug), "FPS: %u  State: %d  Mem: %uMB",
                    g_app.fps, g_app.state, 0);
            ui_draw_text(10, 10, debug, COLOR_WARNING);
        }

        /* End frame */
        ui_end_frame();

        /* Update audio/video if playing */
        if (g_app.playback.playing) {
            if (g_app.playback.is_audio) {
                audio_update();
            }
        }

        g_app.frame_count++;
        g_app.uptime_ms = get_time_ms();
    }
}

/*
 * Shutdown application
 */
void app_shutdown(void)
{
    printf("\nShutting down Nedflix...\n");

    /* Stop any playback */
    audio_stop();
    video_stop();

    /* Save state */
    printf("Saving configuration...\n");
    config_save(&g_app.settings);
    config_save_favorites();
    config_save_history();

    /* Cleanup */
    thumb_cache_shutdown();

    /* Free allocated memory */
    if (g_app.media.items) free(g_app.media.items);
    if (g_app.channels.channels) free(g_app.channels.channels);
    if (g_app.search.results.items) free(g_app.search.results.items);

    /* Shutdown subsystems */
    audio_shutdown();
    video_shutdown();
    network_shutdown();
    ui_shutdown();
    input_shutdown();

    printf("Goodbye!\n\n");
}

/* ============================================
 * STATE HANDLERS
 * ============================================ */

/*
 * STATE: Init
 */
static void state_init(void)
{
    ui_draw_loading("Starting Nedflix...", -1);
}

/*
 * STATE: Splash screen
 */
static void state_splash(void)
{
    static uint32_t splash_start = 0;
    static bool first = true;

    if (first) {
        splash_start = get_time_ms();
        first = false;
    }

    /* Draw splash background */
    ui_draw_rect(0, 0, g_app.screen_width, g_app.screen_height, COLOR_BLACK);

    /* Draw Nedflix logo (text for now) */
    int logo_y = g_app.screen_height / 2 - 50;

    /* Large "NEDFLIX" text */
    ui_draw_text_centered(logo_y, "N E D F L I X", COLOR_RED);

    /* Version */
    char version[64];
    snprintf(version, sizeof(version), "Version %s", NEDFLIX_VERSION);
    ui_draw_text_centered(logo_y + 60, version, COLOR_TEXT_DIM);

    /* Loading indicator */
    ui_draw_spinner(g_app.screen_width / 2 - 20, logo_y + 120, 40);

    /* Proceed after 2 seconds or button press */
    uint32_t elapsed = get_time_ms() - splash_start;
    if (elapsed > 2000 || input_pressed(BTN_CROSS)) {
        first = true;
        g_app.state = STATE_NETWORK_INIT;
    }
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
            ui_draw_loading("Initializing network...", 10);
            phase = 1;
            timeout = 0;
            break;

        case 1:
            ui_draw_loading("Connecting to network...", 30);
            if (network_init() == 0) {
                printf("Network initialized: %s\n", g_app.net.local_ip);
                app_set_status("Network connected");
                phase = 0;

#if NEDFLIX_CLIENT_MODE
                if (strlen(g_app.settings.server_url) > 0) {
                    g_app.state = STATE_CONNECTING;
                } else {
                    g_app.state = STATE_SETTINGS;
                    app_show_toast("Please configure server URL", 3000);
                }
#else
                g_app.state = STATE_MAIN_MENU;
#endif
            } else {
                timeout++;
                if (timeout > 300) {
                    app_set_error("Network initialization failed.\nPlease check your connection and try again.");
                    phase = 0;
                }
            }
            break;
    }
}

/*
 * STATE: Connecting to server
 */
static void state_connecting(void)
{
    static int phase = 0;
    static int timeout = 0;

    switch (phase) {
        case 0:
            ui_draw_loading("Connecting to server...", 50);
            phase = 1;
            timeout = 0;
            break;

        case 1:
            ui_draw_loading("Authenticating...", 70);

            int result = api_init(g_app.settings.server_url);
            if (result == 0) {
                phase = 0;

                /* Check for saved session */
                if (strlen(g_app.settings.session_token) > 0) {
                    /* Try to validate existing session */
                    app_set_status("Session restored");
                    g_app.state = STATE_MAIN_MENU;
                } else if (strlen(g_app.settings.username) > 0) {
                    /* Auto-login with saved credentials */
                    char token[256];
                    if (api_login(g_app.settings.username, g_app.settings.password,
                                  token, sizeof(token)) == 0) {
                        strncpy(g_app.settings.session_token, token, sizeof(g_app.settings.session_token) - 1);
                        config_save(&g_app.settings);
                        g_app.state = STATE_MAIN_MENU;
                    } else {
                        g_app.state = STATE_LOGIN;
                    }
                } else {
                    g_app.state = STATE_LOGIN;
                }
            } else {
                timeout++;
                if (timeout > 60) {
                    app_set_error("Cannot connect to server.\nPlease check your settings.");
                    phase = 0;
                }
            }
            break;
    }
}

/*
 * STATE: Login screen
 */
static void state_login(void)
{
    static int selected = 0;

    ui_draw_header("Sign In");

    /* Login form */
    int form_y = 200;
    int form_x = g_app.screen_width / 2 - 200;

    /* Username field */
    ui_draw_text(form_x, form_y, "Username:", COLOR_TEXT);
    ui_draw_rect(form_x, form_y + 25, 400, 40, selected == 0 ? COLOR_SELECTED : COLOR_CARD_BG);
    ui_draw_rect_outline(form_x, form_y + 25, 400, 40, 2, selected == 0 ? COLOR_RED : COLOR_TEXT_DIM);
    ui_draw_text(form_x + 10, form_y + 35, g_app.settings.username[0] ? g_app.settings.username : "(press X to enter)", COLOR_TEXT);

    /* Password field */
    ui_draw_text(form_x, form_y + 90, "Password:", COLOR_TEXT);
    ui_draw_rect(form_x, form_y + 115, 400, 40, selected == 1 ? COLOR_SELECTED : COLOR_CARD_BG);
    ui_draw_rect_outline(form_x, form_y + 115, 400, 40, 2, selected == 1 ? COLOR_RED : COLOR_TEXT_DIM);
    if (g_app.settings.password[0]) {
        char masked[64];
        memset(masked, '*', MIN(strlen(g_app.settings.password), 20));
        masked[MIN(strlen(g_app.settings.password), 20)] = '\0';
        ui_draw_text(form_x + 10, form_y + 125, masked, COLOR_TEXT);
    } else {
        ui_draw_text(form_x + 10, form_y + 125, "(press X to enter)", COLOR_TEXT_DIM);
    }

    /* Sign In button */
    int btn_y = form_y + 200;
    ui_draw_rect(form_x + 100, btn_y, 200, 50, selected == 2 ? COLOR_RED : COLOR_CARD_BG);
    ui_draw_text_centered(btn_y + 15, "Sign In", selected == 2 ? COLOR_WHITE : COLOR_TEXT);

    /* Browse as Guest */
    ui_draw_text_centered(btn_y + 80, selected == 3 ? "> Browse as Guest <" : "Browse as Guest", COLOR_TEXT_DIM);

    /* Settings */
    ui_draw_text_centered(btn_y + 120, selected == 4 ? "> Settings <" : "Settings", COLOR_TEXT_DIM);

    /* Server URL display */
    char server_info[128];
    snprintf(server_info, sizeof(server_info), "Server: %s",
             g_app.settings.server_url[0] ? g_app.settings.server_url : "(not configured)");
    ui_draw_text_centered(g_app.screen_height - 100, server_info, COLOR_TEXT_DIM);

    /* Navigation */
    if (input_pressed(BTN_UP)) selected = (selected - 1 + 5) % 5;
    if (input_pressed(BTN_DOWN)) selected = (selected + 1) % 5;

    /* Handle selection */
    if (input_pressed(BTN_CROSS)) {
        switch (selected) {
            case 0:  /* Username */
                ui_show_osk("Enter Username", g_app.settings.username, sizeof(g_app.settings.username));
                break;
            case 1:  /* Password */
                ui_show_osk("Enter Password", g_app.settings.password, sizeof(g_app.settings.password));
                break;
            case 2:  /* Sign In */
                if (g_app.settings.username[0] && g_app.settings.password[0]) {
                    char token[256];
                    ui_draw_loading("Signing in...", -1);
                    ui_end_frame();
                    ui_begin_frame();

                    if (api_login(g_app.settings.username, g_app.settings.password,
                                  token, sizeof(token)) == 0) {
                        strncpy(g_app.settings.session_token, token, sizeof(g_app.settings.session_token) - 1);
                        config_save(&g_app.settings);
                        app_show_toast("Signed in successfully", 2000);
                        g_app.state = STATE_MAIN_MENU;
                    } else {
                        app_show_toast("Invalid username or password", 3000);
                    }
                } else {
                    app_show_toast("Please enter username and password", 2000);
                }
                break;
            case 3:  /* Guest */
                app_show_toast("Browsing as guest", 2000);
                g_app.state = STATE_MAIN_MENU;
                break;
            case 4:  /* Settings */
                g_app.state = STATE_SETTINGS;
                break;
        }
    }

    if (input_pressed(BTN_CIRCLE)) {
        g_app.state = STATE_SETTINGS;
    }

    ui_draw_footer();
}

/*
 * STATE: Profile selection
 */
static void state_profile_select(void)
{
    static int selected = 0;

    ui_draw_header("Who's Watching?");

    int count = g_app.settings.profile_count;
    if (count == 0) count = 1;  /* At least show default */

    int grid_cols = MIN(count, 5);
    int item_size = 120;
    int spacing = 30;
    int start_x = (g_app.screen_width - (grid_cols * (item_size + spacing) - spacing)) / 2;
    int start_y = g_app.screen_height / 2 - item_size / 2;

    for (int i = 0; i < count && i < 8; i++) {
        int x = start_x + (i % grid_cols) * (item_size + spacing);
        int y = start_y + (i / grid_cols) * (item_size + spacing + 40);

        /* Profile avatar box */
        uint32_t bg_color = (i == selected) ? COLOR_SELECTED : COLOR_CARD_BG;
        ui_draw_rect(x, y, item_size, item_size, bg_color);

        if (i == selected) {
            ui_draw_rect_outline(x - 4, y - 4, item_size + 8, item_size + 8, 4, COLOR_WHITE);
        }

        /* Avatar initial */
        char initial[2] = { g_app.settings.profiles[i].name[0], '\0' };
        if (!initial[0]) initial[0] = '?';
        ui_draw_text(x + item_size / 2 - 5, y + item_size / 2 - 10, initial, COLOR_WHITE);

        /* Profile name */
        const char *name = g_app.settings.profiles[i].name[0] ? g_app.settings.profiles[i].name : "Profile";
        int name_x = x + (item_size - strlen(name) * FONT_CHAR_W) / 2;
        ui_draw_text(name_x, y + item_size + 10, name, COLOR_TEXT);
    }

    /* Navigation */
    if (input_pressed(BTN_LEFT) && selected > 0) selected--;
    if (input_pressed(BTN_RIGHT) && selected < count - 1) selected++;
    if (input_pressed(BTN_UP) && selected >= grid_cols) selected -= grid_cols;
    if (input_pressed(BTN_DOWN) && selected + grid_cols < count) selected += grid_cols;

    /* Select profile */
    if (input_pressed(BTN_CROSS)) {
        g_app.settings.active_profile = selected;
        memcpy(&g_app.settings.profile, &g_app.settings.profiles[selected], sizeof(user_profile_t));
        g_app.state = STATE_MAIN_MENU;
    }

    ui_draw_footer();
}

/*
 * STATE: Main menu
 */
static void state_main_menu(void)
{
    static int selected = 0;

    /* Draw sidebar */
    ui_draw_sidebar((library_t)(selected < 5 ? selected + 3 : selected));

    /* Draw header with profile info */
    ui_draw_header("Nedflix");

    /* Main content area */
    int content_x = 250;
    int content_y = 120;

    /* Featured content banner area */
    ui_draw_gradient_v(content_x, content_y, g_app.screen_width - content_x - 40, 200, COLOR_MENU_BG, COLOR_DARK_BG);
    ui_draw_text(content_x + 20, content_y + 20, "Welcome to Nedflix", COLOR_WHITE);
    ui_draw_text(content_x + 20, content_y + 50, "Your personal streaming service", COLOR_TEXT_DIM);

    /* Menu options in a grid */
    int menu_y = content_y + 230;
    int cols = 3;
    int card_w = 200;
    int card_h = 100;
    int spacing = 20;

    for (int i = 0; i < 9; i++) {
        int col = i % cols;
        int row = i / cols;
        int x = content_x + col * (card_w + spacing);
        int y = menu_y + row * (card_h + spacing);

        bool is_selected = (i == selected);
        uint32_t bg = is_selected ? COLOR_RED : COLOR_CARD_BG;

        ui_draw_rect(x, y, card_w, card_h, bg);

        if (is_selected) {
            ui_draw_rect_outline(x - 2, y - 2, card_w + 4, card_h + 4, 2, COLOR_WHITE);
        }

        /* Icon/label */
        ui_draw_text(x + 15, y + card_h / 2 - 10, main_menu_options[i],
                    is_selected ? COLOR_WHITE : COLOR_TEXT);
    }

    /* Navigation */
    if (input_pressed(BTN_LEFT) && selected % cols > 0) selected--;
    if (input_pressed(BTN_RIGHT) && selected % cols < cols - 1 && selected < 8) selected++;
    if (input_pressed(BTN_UP) && selected >= cols) selected -= cols;
    if (input_pressed(BTN_DOWN) && selected + cols < 9) selected += cols;

    /* Handle selection */
    if (input_pressed(BTN_CROSS)) {
        switch (selected) {
            case 0:  /* Movies */
                g_app.current_library = LIBRARY_MOVIES;
                strncpy(g_app.media.current_path, "/Movies", MAX_PATH_LENGTH);
                g_app.state = STATE_BROWSING;
                break;
            case 1:  /* TV Shows */
                g_app.current_library = LIBRARY_TVSHOWS;
                strncpy(g_app.media.current_path, "/TV Shows", MAX_PATH_LENGTH);
                g_app.state = STATE_BROWSING;
                break;
            case 2:  /* Music */
                g_app.current_library = LIBRARY_MUSIC;
                strncpy(g_app.media.current_path, "/Music", MAX_PATH_LENGTH);
                g_app.state = STATE_BROWSING;
                break;
            case 3:  /* Audiobooks */
                g_app.current_library = LIBRARY_AUDIOBOOKS;
                strncpy(g_app.media.current_path, "/Audiobooks", MAX_PATH_LENGTH);
                g_app.state = STATE_BROWSING;
                break;
            case 4:  /* Live TV */
                g_app.state = STATE_CHANNELS;
                break;
            case 5:  /* My List */
                g_app.current_library = LIBRARY_FAVORITES;
                g_app.state = STATE_BROWSING;
                break;
            case 6:  /* Continue Watching */
                g_app.current_library = LIBRARY_HISTORY;
                g_app.state = STATE_BROWSING;
                break;
            case 7:  /* Search */
                g_app.state = STATE_SEARCH;
                break;
            case 8:  /* Settings */
                g_app.state = STATE_SETTINGS;
                break;
        }

        /* Fetch content for browsing states */
        if (g_app.state == STATE_BROWSING) {
            g_app.media.count = 0;
            g_app.media.selected_index = 0;
            g_app.media.scroll_offset = 0;
#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token, g_app.media.current_path,
                      g_app.current_library, &g_app.media);
#endif
        }
    }

    if (input_pressed(BTN_TRIANGLE)) {
        g_app.state = STATE_SEARCH;
    }

    if (input_pressed(BTN_START)) {
        g_app.state = STATE_SETTINGS;
    }

    ui_draw_footer();
}

/*
 * STATE: Browsing media
 */
static void state_browsing(void)
{
    /* Draw sidebar */
    ui_draw_sidebar(g_app.current_library);

    /* Draw header with current library name */
    ui_draw_header(lib_names[g_app.current_library]);

    /* Draw breadcrumb path */
    ui_draw_text(260, 95, g_app.media.current_path, COLOR_TEXT_DIM);

    /* Main content area */
    int content_x = 250;
    int content_y = 130;

    /* Draw media grid */
    if (g_app.media.count == 0) {
        ui_draw_text_centered(g_app.screen_height / 2, "No items found", COLOR_TEXT_DIM);
        ui_draw_text_centered(g_app.screen_height / 2 + 30, "Press Circle to go back", COLOR_TEXT_DIM);
    } else {
        /* Calculate grid layout */
        int card_w = 160;
        int card_h = 240;
        int spacing = 20;
        int cols = (g_app.screen_width - content_x - 40) / (card_w + spacing);
        if (cols < 1) cols = 1;

        /* Draw visible items */
        int visible_start = g_app.media.scroll_offset;
        int visible_count = cols * ((g_app.screen_height - content_y - 80) / (card_h + spacing) + 1);

        for (int i = 0; i < visible_count && (visible_start + i) < g_app.media.count; i++) {
            int idx = visible_start + i;
            int col = i % cols;
            int row = i / cols;

            int x = content_x + col * (card_w + spacing);
            int y = content_y + row * (card_h + spacing);

            bool selected = (idx == g_app.media.selected_index);
            ui_draw_media_card(x, y, card_w, card_h, &g_app.media.items[idx], selected);
        }

        /* Scroll indicators */
        if (g_app.media.scroll_offset > 0) {
            ui_draw_text(g_app.screen_width - 80, content_y, "^", COLOR_TEXT_DIM);
        }
        if (g_app.media.scroll_offset + visible_count < g_app.media.count) {
            ui_draw_text(g_app.screen_width - 80, g_app.screen_height - 100, "v", COLOR_TEXT_DIM);
        }

        /* Item counter */
        char counter[32];
        snprintf(counter, sizeof(counter), "%d / %d", g_app.media.selected_index + 1, g_app.media.count);
        ui_draw_text(g_app.screen_width - 120, 95, counter, COLOR_TEXT_DIM);
    }

    /* Navigation */
    int cols = 4;
    if (input_pressed(BTN_UP) && g_app.media.selected_index >= cols) {
        g_app.media.selected_index -= cols;
        if (g_app.media.selected_index < g_app.media.scroll_offset) {
            g_app.media.scroll_offset = MAX(0, g_app.media.scroll_offset - cols);
        }
    }
    if (input_pressed(BTN_DOWN) && g_app.media.selected_index + cols < g_app.media.count) {
        g_app.media.selected_index += cols;
        /* Auto-scroll */
    }
    if (input_pressed(BTN_LEFT) && g_app.media.selected_index > 0) {
        g_app.media.selected_index--;
    }
    if (input_pressed(BTN_RIGHT) && g_app.media.selected_index < g_app.media.count - 1) {
        g_app.media.selected_index++;
    }

    /* Library switch with L1/R1 */
    if (input_pressed(BTN_L1)) {
        int lib = (int)g_app.current_library - 1;
        if (lib < LIBRARY_MUSIC) lib = LIBRARY_HISTORY;
        g_app.current_library = (library_t)lib;
        strncpy(g_app.media.current_path, lib_paths[g_app.current_library], MAX_PATH_LENGTH);
        g_app.media.count = 0;
        g_app.media.selected_index = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.current_path,
                  g_app.current_library, &g_app.media);
#endif
    }
    if (input_pressed(BTN_R1)) {
        int lib = (int)g_app.current_library + 1;
        if (lib > LIBRARY_HISTORY) lib = LIBRARY_MUSIC;
        g_app.current_library = (library_t)lib;
        strncpy(g_app.media.current_path, lib_paths[g_app.current_library], MAX_PATH_LENGTH);
        g_app.media.count = 0;
        g_app.media.selected_index = 0;
#if NEDFLIX_CLIENT_MODE
        api_browse(g_app.settings.session_token, g_app.media.current_path,
                  g_app.current_library, &g_app.media);
#endif
    }

    /* Select item */
    if (input_pressed(BTN_CROSS) && g_app.media.count > 0) {
        media_item_t *item = &g_app.media.items[g_app.media.selected_index];

        if (item->is_directory) {
            /* Navigate into directory */
            strncpy(g_app.media.current_path, item->path, MAX_PATH_LENGTH);
            g_app.media.count = 0;
            g_app.media.selected_index = 0;
            g_app.media.scroll_offset = 0;
#if NEDFLIX_CLIENT_MODE
            api_browse(g_app.settings.session_token, g_app.media.current_path,
                      g_app.current_library, &g_app.media);
#endif
        } else {
            /* Show detail or play directly */
            memcpy(&g_app.detail_item, item, sizeof(media_item_t));
            g_app.state = STATE_DETAIL;
        }
    }

    /* Show item detail with Triangle */
    if (input_pressed(BTN_TRIANGLE) && g_app.media.count > 0) {
        media_item_t *item = &g_app.media.items[g_app.media.selected_index];
        memcpy(&g_app.detail_item, item, sizeof(media_item_t));
        g_app.state = STATE_DETAIL;
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
            g_app.state = STATE_MAIN_MENU;
        }
    }

    /* Search shortcut */
    if (input_pressed(BTN_SELECT)) {
        g_app.state = STATE_SEARCH;
    }

    ui_draw_footer();
}

/*
 * STATE: Search
 */
static void state_search(void)
{
    ui_draw_header("Search");

    /* Search input field */
    int input_y = 150;
    ui_draw_rect(100, input_y, g_app.screen_width - 200, 50, COLOR_CARD_BG);
    ui_draw_rect_outline(100, input_y, g_app.screen_width - 200, 50, 2, COLOR_RED);

    if (g_app.search.query[0]) {
        ui_draw_text(120, input_y + 15, g_app.search.query, COLOR_WHITE);
    } else {
        ui_draw_text(120, input_y + 15, "Press X to enter search term...", COLOR_TEXT_DIM);
    }

    /* Search results */
    if (g_app.search.has_results) {
        ui_draw_text(100, 230, "Results:", COLOR_TEXT);
        /* Draw results list */
        ui_draw_media_list(&g_app.search.results);
    } else if (g_app.search.query[0]) {
        ui_draw_text_centered(g_app.screen_height / 2, "No results found", COLOR_TEXT_DIM);
    } else {
        ui_draw_text_centered(g_app.screen_height / 2, "Enter a search term to begin", COLOR_TEXT_DIM);
    }

    /* Controls */
    if (input_pressed(BTN_CROSS)) {
        if (!g_app.search.query[0]) {
            ui_show_osk("Search", g_app.search.query, sizeof(g_app.search.query));
        } else if (g_app.search.results.count > 0) {
            /* Select result */
            media_item_t *item = &g_app.search.results.items[g_app.search.results.selected_index];
            memcpy(&g_app.detail_item, item, sizeof(media_item_t));
            g_app.state = STATE_DETAIL;
        }
    }

    if (input_pressed(BTN_TRIANGLE)) {
        ui_show_osk("Search", g_app.search.query, sizeof(g_app.search.query));
    }

    /* Check if OSK returned a result */
    if (g_app.osk.done && !g_app.osk.active) {
        g_app.osk.done = false;
        if (g_app.search.query[0]) {
            /* Perform search */
            g_app.search.results.count = 0;
#if NEDFLIX_CLIENT_MODE
            api_search(g_app.settings.session_token, g_app.search.query, &g_app.search.results);
#endif
            g_app.search.has_results = (g_app.search.results.count > 0);
        }
    }

    /* Navigate results */
    if (g_app.search.has_results) {
        if (input_pressed(BTN_UP) && g_app.search.results.selected_index > 0) {
            g_app.search.results.selected_index--;
        }
        if (input_pressed(BTN_DOWN) && g_app.search.results.selected_index < g_app.search.results.count - 1) {
            g_app.search.results.selected_index++;
        }
    }

    if (input_pressed(BTN_CIRCLE)) {
        g_app.search.query[0] = '\0';
        g_app.search.has_results = false;
        g_app.state = g_app.prev_state != STATE_SEARCH ? g_app.prev_state : STATE_MAIN_MENU;
    }

    ui_draw_footer();
}

/*
 * STATE: Media detail view
 */
static void state_detail(void)
{
    media_item_t *item = &g_app.detail_item;

    /* Background with gradient */
    ui_draw_rect(0, 0, g_app.screen_width, g_app.screen_height, COLOR_DARK_BG);
    ui_draw_gradient_v(0, 0, g_app.screen_width, 300, COLOR_MENU_BG, COLOR_DARK_BG);

    /* Poster placeholder */
    int poster_x = 80;
    int poster_y = 100;
    int poster_w = 200;
    int poster_h = 300;
    ui_draw_rect(poster_x, poster_y, poster_w, poster_h, COLOR_CARD_BG);

    /* Title */
    ui_draw_text_large(poster_x + poster_w + 40, poster_y, item->name, COLOR_WHITE);

    /* Metadata */
    int info_x = poster_x + poster_w + 40;
    int info_y = poster_y + 50;

    char meta[256];
    if (item->year > 0) {
        snprintf(meta, sizeof(meta), "%d", item->year);
        ui_draw_text(info_x, info_y, meta, COLOR_TEXT_DIM);
        info_y += 25;
    }

    if (item->duration_sec > 0) {
        format_duration(item->duration_sec * 1000, meta, sizeof(meta));
        ui_draw_text(info_x, info_y, meta, COLOR_TEXT_DIM);
        info_y += 25;
    }

    if (item->rating > 0) {
        snprintf(meta, sizeof(meta), "Rating: %.1f/10", item->rating);
        ui_draw_text(info_x, info_y, meta, COLOR_TEXT_DIM);
        info_y += 25;
    }

    if (item->genre[0]) {
        ui_draw_text(info_x, info_y, item->genre, COLOR_TEXT_DIM);
        info_y += 25;
    }

    /* Description */
    info_y += 20;
    if (item->description[0]) {
        ui_draw_text_wrapped(info_x, info_y, g_app.screen_width - info_x - 80, item->description, COLOR_TEXT);
    }

    /* Action buttons */
    static int btn_selected = 0;
    int btn_y = poster_y + poster_h + 40;

    const char *buttons[] = { "Play", "Add to My List", "Back" };
    int btn_count = 3;
    int btn_w = 150;
    int btn_spacing = 20;

    for (int i = 0; i < btn_count; i++) {
        int x = poster_x + i * (btn_w + btn_spacing);
        bool selected = (i == btn_selected);

        ui_draw_rect(x, btn_y, btn_w, 45, selected ? COLOR_RED : COLOR_CARD_BG);
        ui_draw_text(x + 20, btn_y + 12, buttons[i], selected ? COLOR_WHITE : COLOR_TEXT);
    }

    /* Resume indicator */
    if (item->resume_position_sec > 0) {
        char resume[64];
        format_duration(item->resume_position_sec * 1000, resume, sizeof(resume));
        char resume_text[128];
        snprintf(resume_text, sizeof(resume_text), "Resume from %s", resume);
        ui_draw_text(poster_x, btn_y + 60, resume_text, COLOR_TEXT_DIM);
    }

    /* Navigation */
    if (input_pressed(BTN_LEFT) && btn_selected > 0) btn_selected--;
    if (input_pressed(BTN_RIGHT) && btn_selected < btn_count - 1) btn_selected++;

    /* Handle button press */
    if (input_pressed(BTN_CROSS)) {
        switch (btn_selected) {
            case 0:  /* Play */
            {
                char stream_url[MAX_URL_LENGTH];
#if NEDFLIX_CLIENT_MODE
                if (api_get_stream_url(g_app.settings.session_token, item->path,
                                       g_app.settings.video_quality, stream_url, sizeof(stream_url)) == 0) {
#else
                snprintf(stream_url, sizeof(stream_url), "file://%s", item->path);
                {
#endif
                    strncpy(g_app.playback.title, item->name, MAX_TITLE_LENGTH);
                    strncpy(g_app.playback.url, stream_url, MAX_URL_LENGTH);
                    strncpy(g_app.playback.media_path, item->path, MAX_PATH_LENGTH);
                    g_app.playback.is_audio = (item->type == MEDIA_TYPE_AUDIO);
                    g_app.playback.is_live = false;

                    /* Resume position */
                    if (item->resume_position_sec > 0) {
                        g_app.playback.position_ms = item->resume_position_sec * 1000;
                    } else {
                        g_app.playback.position_ms = 0;
                    }

                    if (item->type == MEDIA_TYPE_AUDIO) {
                        if (audio_play_stream(stream_url) == 0) {
                            g_app.playback.playing = true;
                            g_app.state = STATE_PLAYING;
                        }
                    } else {
                        if (video_play_stream(stream_url) == 0) {
                            g_app.playback.playing = true;
                            g_app.state = STATE_PLAYING;
                        }
                    }
                }
            }
            break;

            case 1:  /* Add to My List */
                item->favorite = !item->favorite;
                if (item->favorite) {
                    if (g_app.favorite_count < MAX_FAVORITES) {
                        memcpy(&g_app.favorites[g_app.favorite_count++], item, sizeof(media_item_t));
                        app_show_toast("Added to My List", 2000);
                    }
                } else {
                    /* Remove from favorites */
                    for (int i = 0; i < g_app.favorite_count; i++) {
                        if (strcmp(g_app.favorites[i].path, item->path) == 0) {
                            memmove(&g_app.favorites[i], &g_app.favorites[i + 1],
                                   (g_app.favorite_count - i - 1) * sizeof(media_item_t));
                            g_app.favorite_count--;
                            app_show_toast("Removed from My List", 2000);
                            break;
                        }
                    }
                }
                config_save_favorites();
                break;

            case 2:  /* Back */
                btn_selected = 0;
                g_app.state = STATE_BROWSING;
                break;
        }
    }

    if (input_pressed(BTN_CIRCLE)) {
        btn_selected = 0;
        g_app.state = STATE_BROWSING;
    }

    ui_draw_footer();
}

/*
 * STATE: Media playback
 */
static void state_playing(void)
{
    static bool controls_visible = true;
    static uint32_t last_input_time = 0;
    static int control_selected = 2;  /* Play/Pause */

    /* Update playback state */
    if (g_app.playback.is_audio) {
        g_app.playback.position_ms = audio_get_position();
        g_app.playback.duration_ms = audio_get_duration();
        g_app.playback.playing = audio_is_playing();
        g_app.playback.buffering = audio_is_buffering();
        g_app.playback.buffered_percent = audio_get_buffer_percent();
    } else {
        video_render_frame();
        g_app.playback.buffering = video_is_buffering();
    }

    /* Auto-hide controls after 5 seconds of no input */
    if (g_app.buttons_pressed || g_app.buttons_just_pressed) {
        last_input_time = get_time_ms();
        controls_visible = true;
    } else if (get_time_ms() - last_input_time > 5000 && g_app.playback.playing && !g_app.playback.paused) {
        controls_visible = false;
    }

    /* Draw video frame (for video) or audio visualization */
    if (!g_app.playback.is_audio) {
        /* Video frame is rendered directly by video_render_frame() */
    } else {
        /* Audio playback screen */
        ui_draw_rect(0, 0, g_app.screen_width, g_app.screen_height, COLOR_DARK_BG);
        ui_draw_text_centered(g_app.screen_height / 2 - 100, g_app.playback.title, COLOR_WHITE);
        ui_draw_text_centered(g_app.screen_height / 2 - 60, "Now Playing", COLOR_TEXT_DIM);

        /* Audio waveform visualization placeholder */
        int wave_y = g_app.screen_height / 2;
        for (int i = 0; i < 40; i++) {
            int height = 10 + (g_app.frame_count + i * 5) % 40;
            ui_draw_rect(g_app.screen_width / 2 - 200 + i * 10, wave_y - height / 2, 8, height, COLOR_RED);
        }
    }

    /* Draw subtitles if enabled */
    if (g_app.playback.subtitles_visible && g_app.subtitles.loaded) {
        const char *sub_text = subtitle_get_current(g_app.playback.position_ms);
        if (sub_text) {
            ui_draw_subtitle(sub_text);
        }
    }

    /* Draw playback controls overlay */
    if (controls_visible) {
        /* Semi-transparent overlay */
        ui_draw_rect(0, g_app.screen_height - 200, g_app.screen_width, 200, COLOR_OVERLAY);

        /* Title */
        ui_draw_text(50, g_app.screen_height - 180, g_app.playback.title, COLOR_WHITE);

        /* Progress bar */
        int bar_x = 50;
        int bar_y = g_app.screen_height - 120;
        int bar_w = g_app.screen_width - 100;
        int bar_h = 8;

        /* Buffer progress */
        ui_draw_rect(bar_x, bar_y, bar_w, bar_h, COLOR_PROGRESS_BG);

        /* Buffered range */
        if (g_app.playback.buffered_percent > 0) {
            int buf_w = (bar_w * g_app.playback.buffered_percent) / 100;
            ui_draw_rect(bar_x, bar_y, buf_w, bar_h, COLOR_BUFFER_FG);
        }

        /* Playback progress */
        if (g_app.playback.duration_ms > 0) {
            int progress_w = (bar_w * g_app.playback.position_ms) / g_app.playback.duration_ms;
            ui_draw_rect(bar_x, bar_y, progress_w, bar_h, COLOR_RED);

            /* Scrubber handle */
            ui_draw_circle(bar_x + progress_w, bar_y + bar_h / 2, 8, COLOR_WHITE);
        }

        /* Time display */
        char time_cur[16], time_dur[16];
        format_duration(g_app.playback.position_ms, time_cur, sizeof(time_cur));
        format_duration(g_app.playback.duration_ms, time_dur, sizeof(time_dur));

        ui_draw_text(bar_x, bar_y + 15, time_cur, COLOR_TEXT);
        char time_total[32];
        snprintf(time_total, sizeof(time_total), "/ %s", time_dur);
        ui_draw_text(bar_x + 80, bar_y + 15, time_total, COLOR_TEXT_DIM);

        /* Control buttons */
        int ctrl_y = g_app.screen_height - 60;
        const char *controls[] = { "|<", "<<", g_app.playback.paused ? ">" : "||", ">>", ">|", "CC", "Vol" };
        int ctrl_count = 7;
        int ctrl_w = 60;
        int ctrl_start = g_app.screen_width / 2 - (ctrl_count * ctrl_w) / 2;

        for (int i = 0; i < ctrl_count; i++) {
            int x = ctrl_start + i * ctrl_w;
            bool selected = (i == control_selected);

            if (selected) {
                ui_draw_rect(x, ctrl_y - 5, ctrl_w - 5, 40, COLOR_SELECTED);
            }
            ui_draw_text(x + 15, ctrl_y + 5, controls[i], selected ? COLOR_WHITE : COLOR_TEXT_DIM);
        }

        /* Volume indicator */
        char vol_str[16];
        snprintf(vol_str, sizeof(vol_str), "%d%%", g_app.settings.volume);
        ui_draw_text(g_app.screen_width - 100, ctrl_y + 5, vol_str, COLOR_TEXT);

        /* Buffering indicator */
        if (g_app.playback.buffering) {
            ui_draw_text_centered(g_app.screen_height / 2, "Buffering...", COLOR_WHITE);
            ui_draw_spinner(g_app.screen_width / 2 - 20, g_app.screen_height / 2 + 30, 40);
        }
    }

    /* Control navigation */
    if (controls_visible) {
        if (input_pressed(BTN_LEFT)) control_selected = MAX(0, control_selected - 1);
        if (input_pressed(BTN_RIGHT)) control_selected = MIN(6, control_selected + 1);
    }

    /* Playback controls */
    if (input_pressed(BTN_CROSS) || input_pressed(BTN_START)) {
        if (!controls_visible) {
            controls_visible = true;
            last_input_time = get_time_ms();
        } else if (control_selected == 2) {
            /* Play/Pause */
            if (g_app.playback.paused) {
                if (g_app.playback.is_audio) audio_resume();
                else video_resume();
                g_app.playback.paused = false;
            } else {
                if (g_app.playback.is_audio) audio_pause();
                else video_pause();
                g_app.playback.paused = true;
            }
        } else if (control_selected == 0) {
            /* Previous chapter / restart */
            if (g_app.playback.is_audio) audio_seek_absolute(0);
            else video_seek_absolute(0);
        } else if (control_selected == 1) {
            /* Rewind 10s */
            if (g_app.playback.is_audio) audio_seek(-10000);
            else video_seek(-10000);
        } else if (control_selected == 3) {
            /* Fast forward 10s */
            if (g_app.playback.is_audio) audio_seek(10000);
            else video_seek(10000);
        } else if (control_selected == 4) {
            /* Next chapter / skip */
        } else if (control_selected == 5) {
            /* Toggle subtitles */
            g_app.playback.subtitles_visible = !g_app.playback.subtitles_visible;
            app_show_toast(g_app.playback.subtitles_visible ? "Subtitles On" : "Subtitles Off", 1500);
        }
    }

    /* D-pad seeking */
    if (input_held(BTN_LEFT)) {
        if (g_app.playback.is_audio) audio_seek(-5000);
        else video_seek(-5000);
    }
    if (input_held(BTN_RIGHT)) {
        if (g_app.playback.is_audio) audio_seek(5000);
        else video_seek(5000);
    }

    /* Volume control with L2/R2 */
    if (g_app.l2_pressure > 30) {
        g_app.settings.volume = MAX(0, g_app.settings.volume - 1);
        audio_set_volume(g_app.settings.volume);
        controls_visible = true;
        last_input_time = get_time_ms();
    }
    if (g_app.r2_pressure > 30) {
        g_app.settings.volume = MIN(100, g_app.settings.volume + 1);
        audio_set_volume(g_app.settings.volume);
        controls_visible = true;
        last_input_time = get_time_ms();
    }

    /* Stop and go back */
    if (input_pressed(BTN_CIRCLE)) {
        /* Save progress */
        uint32_t pos_sec = g_app.playback.position_ms / 1000;
        if (pos_sec > 30) {  /* Only save if watched more than 30 seconds */
#if NEDFLIX_CLIENT_MODE
            api_report_progress(g_app.settings.session_token, g_app.playback.media_path, pos_sec);
#endif
            config_add_history(&g_app.detail_item, pos_sec);
        }

        if (g_app.playback.is_audio) audio_stop();
        else video_stop();

        g_app.playback.playing = false;
        g_app.state = STATE_DETAIL;
    }
}

/*
 * STATE: Live TV
 */
static void state_live_tv(void)
{
    static bool controls_visible = true;
    static uint32_t last_input_time = 0;

    /* Render video stream */
    video_render_frame();

    /* Auto-hide controls */
    if (g_app.buttons_pressed || g_app.buttons_just_pressed) {
        last_input_time = get_time_ms();
        controls_visible = true;
    } else if (get_time_ms() - last_input_time > 5000) {
        controls_visible = false;
    }

    /* Draw channel info overlay */
    if (controls_visible && g_app.channels.count > 0) {
        int ch_idx = g_app.channels.selected_index;
        channel_t *ch = &g_app.channels.channels[ch_idx];

        /* Channel info bar */
        ui_draw_rect(0, g_app.screen_height - 150, g_app.screen_width, 150, COLOR_OVERLAY);

        /* Channel number and name */
        char ch_str[64];
        snprintf(ch_str, sizeof(ch_str), "%d. %s", ch->channel_number, ch->name);
        ui_draw_text(50, g_app.screen_height - 130, ch_str, COLOR_WHITE);

        /* Current program */
        if (ch->current_program[0]) {
            ui_draw_text(50, g_app.screen_height - 100, ch->current_program, COLOR_TEXT);
        }

        /* Next program */
        if (ch->next_program[0]) {
            char next[256];
            snprintf(next, sizeof(next), "Next: %s", ch->next_program);
            ui_draw_text(50, g_app.screen_height - 70, next, COLOR_TEXT_DIM);
        }

        /* Controls hint */
        ui_draw_text(g_app.screen_width - 300, g_app.screen_height - 50,
                    "Up/Down: Channel  Triangle: Guide", COLOR_TEXT_DIM);
    }

    /* Channel navigation */
    if (input_pressed(BTN_UP)) {
        if (g_app.channels.selected_index > 0) {
            g_app.channels.selected_index--;
            /* Switch to new channel */
            channel_t *ch = &g_app.channels.channels[g_app.channels.selected_index];
            video_stop();
            video_play_stream(ch->stream_url);
        }
        controls_visible = true;
        last_input_time = get_time_ms();
    }

    if (input_pressed(BTN_DOWN)) {
        if (g_app.channels.selected_index < g_app.channels.count - 1) {
            g_app.channels.selected_index++;
            channel_t *ch = &g_app.channels.channels[g_app.channels.selected_index];
            video_stop();
            video_play_stream(ch->stream_url);
        }
        controls_visible = true;
        last_input_time = get_time_ms();
    }

    /* Channel list / EPG */
    if (input_pressed(BTN_TRIANGLE)) {
        g_app.state = STATE_CHANNELS;
    }

    /* Exit Live TV */
    if (input_pressed(BTN_CIRCLE)) {
        video_stop();
        g_app.playback.playing = false;
        g_app.state = STATE_CHANNELS;
    }

    /* Toggle controls */
    if (input_pressed(BTN_CROSS)) {
        controls_visible = !controls_visible;
        if (controls_visible) last_input_time = get_time_ms();
    }
}

/*
 * STATE: Channel list / EPG
 */
static void state_channels(void)
{
    ui_draw_header("Live TV");

    /* Fetch channels if not loaded */
    static bool loaded = false;
    if (!loaded) {
#if NEDFLIX_CLIENT_MODE
        api_get_channels(g_app.settings.session_token, &g_app.channels);
#endif
        loaded = true;
    }

    if (g_app.channels.count == 0) {
        ui_draw_text_centered(g_app.screen_height / 2, "No channels available", COLOR_TEXT_DIM);
        ui_draw_text_centered(g_app.screen_height / 2 + 30, "Configure IPTV in server settings", COLOR_TEXT_DIM);
    } else {
        /* Draw channel list */
        ui_draw_channel_list(&g_app.channels);
    }

    /* Navigation */
    if (input_pressed(BTN_UP) && g_app.channels.selected_index > 0) {
        g_app.channels.selected_index--;
        if (g_app.channels.selected_index < g_app.channels.scroll_offset) {
            g_app.channels.scroll_offset--;
        }
    }
    if (input_pressed(BTN_DOWN) && g_app.channels.selected_index < g_app.channels.count - 1) {
        g_app.channels.selected_index++;
        if (g_app.channels.selected_index >= g_app.channels.scroll_offset + MAX_ITEMS_VISIBLE) {
            g_app.channels.scroll_offset++;
        }
    }

    /* Watch channel */
    if (input_pressed(BTN_CROSS) && g_app.channels.count > 0) {
        channel_t *ch = &g_app.channels.channels[g_app.channels.selected_index];

        if (video_play_stream(ch->stream_url) == 0) {
            strncpy(g_app.playback.title, ch->name, MAX_TITLE_LENGTH);
            g_app.playback.playing = true;
            g_app.playback.is_live = true;
            g_app.state = STATE_LIVE_TV;
        } else {
            app_show_toast("Failed to start stream", 2000);
        }
    }

    /* Go back */
    if (input_pressed(BTN_CIRCLE)) {
        loaded = false;
        g_app.state = STATE_MAIN_MENU;
    }

    ui_draw_footer();
}

/*
 * STATE: Settings
 */
static void state_settings(void)
{
    static int page = 0;
    static int selected = 0;

    ui_draw_header("Settings");

    /* Settings tabs */
    int tab_y = 100;
    int tab_count = ARRAY_SIZE(settings_pages);

    for (int i = 0; i < tab_count; i++) {
        int x = 50 + i * 140;
        bool is_current = (i == page);

        if (is_current) {
            ui_draw_rect(x - 5, tab_y - 5, 130, 35, COLOR_RED);
        }
        ui_draw_text(x, tab_y, settings_pages[i], is_current ? COLOR_WHITE : COLOR_TEXT_DIM);
    }

    /* Draw settings for current page */
    ui_draw_settings_page(page, selected);

    /* Tab navigation with L1/R1 */
    if (input_pressed(BTN_L1) && page > 0) {
        page--;
        selected = 0;
    }
    if (input_pressed(BTN_R1) && page < tab_count - 1) {
        page++;
        selected = 0;
    }

    /* Setting navigation */
    if (input_pressed(BTN_UP) && selected > 0) selected--;
    if (input_pressed(BTN_DOWN)) selected++;

    /* Modify settings based on page and selection */
    if (input_pressed(BTN_CROSS) || input_pressed(BTN_LEFT) || input_pressed(BTN_RIGHT)) {
        bool left = input_pressed(BTN_LEFT);
        bool right = input_pressed(BTN_RIGHT);

        switch (page) {
            case 0:  /* Connection */
                if (selected == 0 && input_pressed(BTN_CROSS)) {
                    ui_show_osk("Server URL", g_app.settings.server_url, MAX_URL_LENGTH);
                }
                break;

            case 1:  /* Playback */
                if (selected == 0) {  /* Quality */
                    if (left && g_app.settings.video_quality > QUALITY_AUTO) g_app.settings.video_quality--;
                    if (right && g_app.settings.video_quality < QUALITY_FHD) g_app.settings.video_quality++;
                } else if (selected == 1) {  /* Autoplay */
                    g_app.settings.autoplay = !g_app.settings.autoplay;
                } else if (selected == 2) {  /* Subtitles */
                    g_app.settings.show_subtitles = !g_app.settings.show_subtitles;
                }
                break;

            case 2:  /* Audio */
                if (selected == 0) {  /* Volume */
                    if (left) g_app.settings.volume = MAX(0, g_app.settings.volume - 5);
                    if (right) g_app.settings.volume = MIN(100, g_app.settings.volume + 5);
                } else if (selected == 1) {  /* Surround */
                    g_app.settings.enable_surround = !g_app.settings.enable_surround;
                }
                break;

            case 3:  /* Display */
                if (selected == 0) {  /* Resolution */
                    g_app.settings.enable_1080p = !g_app.settings.enable_1080p;
                }
                break;
        }
    }

    /* Save and exit */
    if (input_pressed(BTN_START)) {
        config_save(&g_app.settings);
        app_show_toast("Settings saved", 2000);
    }

    if (input_pressed(BTN_CIRCLE)) {
        config_save(&g_app.settings);
        if (g_app.prev_state == STATE_LOGIN || g_app.prev_state == STATE_INIT) {
            g_app.state = STATE_CONNECTING;
        } else {
            g_app.state = STATE_MAIN_MENU;
        }
    }

    ui_draw_footer();
}

/*
 * STATE: About screen
 */
static void state_about(void)
{
    ui_draw_header("About Nedflix");

    int y = 150;
    int x = 100;

    ui_draw_text_large(x, y, "Nedflix for PlayStation 3", COLOR_WHITE);
    y += 50;

    char version[64];
    snprintf(version, sizeof(version), "Version %s", NEDFLIX_VERSION);
    ui_draw_text(x, y, version, COLOR_TEXT);
    y += 30;

    snprintf(version, sizeof(version), "Build Date: %s", NEDFLIX_BUILD_DATE);
    ui_draw_text(x, y, version, COLOR_TEXT_DIM);
    y += 50;

    ui_draw_text(x, y, "A personal media streaming client for PS3", COLOR_TEXT);
    y += 30;
    ui_draw_text(x, y, "Built with PSL1GHT SDK", COLOR_TEXT_DIM);
    y += 50;

    ui_draw_text(x, y, "Controls:", COLOR_WHITE);
    y += 30;
    ui_draw_text(x + 20, y, "Cross (X)  - Select / Play / Pause", COLOR_TEXT);
    y += 25;
    ui_draw_text(x + 20, y, "Circle (O) - Back / Stop", COLOR_TEXT);
    y += 25;
    ui_draw_text(x + 20, y, "Triangle   - Search / Info", COLOR_TEXT);
    y += 25;
    ui_draw_text(x + 20, y, "Square     - Add to List", COLOR_TEXT);
    y += 25;
    ui_draw_text(x + 20, y, "D-Pad      - Navigate", COLOR_TEXT);
    y += 25;
    ui_draw_text(x + 20, y, "L1/R1      - Switch Library / Tab", COLOR_TEXT);
    y += 25;
    ui_draw_text(x + 20, y, "L2/R2      - Volume Down / Up", COLOR_TEXT);
    y += 25;
    ui_draw_text(x + 20, y, "Start      - Settings", COLOR_TEXT);
    y += 25;
    ui_draw_text(x + 20, y, "PS + Start - Exit", COLOR_TEXT);

    /* Network info */
    y += 50;
    ui_draw_text(x, y, "Network:", COLOR_WHITE);
    y += 30;

    char net_info[64];
    snprintf(net_info, sizeof(net_info), "IP: %s", g_app.net.local_ip[0] ? g_app.net.local_ip : "Not connected");
    ui_draw_text(x + 20, y, net_info, COLOR_TEXT);
    y += 25;

    snprintf(net_info, sizeof(net_info), "Server: %s", g_app.settings.server_url[0] ? g_app.settings.server_url : "Not configured");
    ui_draw_text(x + 20, y, net_info, COLOR_TEXT);

    if (input_pressed(BTN_CIRCLE) || input_pressed(BTN_CROSS)) {
        g_app.state = STATE_SETTINGS;
    }

    ui_draw_footer();
}

/*
 * STATE: Error display
 */
static void state_error(void)
{
    ui_draw_header("Error");

    /* Error icon */
    ui_draw_rect(g_app.screen_width / 2 - 50, 180, 100, 100, COLOR_RED);
    ui_draw_text(g_app.screen_width / 2 - 15, 220, "!", COLOR_WHITE);

    /* Error message */
    ui_draw_text_wrapped(100, 320, g_app.screen_width - 200, g_app.error_msg, COLOR_TEXT);

    /* Options */
    ui_draw_text_centered(g_app.screen_height - 150, "Press X to retry", COLOR_TEXT);
    ui_draw_text_centered(g_app.screen_height - 120, "Press O to go to settings", COLOR_TEXT_DIM);
    ui_draw_text_centered(g_app.screen_height - 90, "Press Start to exit", COLOR_TEXT_DIM);

    if (input_pressed(BTN_CROSS)) {
        g_app.error_msg[0] = '\0';
        g_app.state = STATE_NETWORK_INIT;
    }

    if (input_pressed(BTN_CIRCLE)) {
        g_app.error_msg[0] = '\0';
        g_app.state = STATE_SETTINGS;
    }

    if (input_pressed(BTN_START)) {
        g_app.running = false;
    }

    ui_draw_footer();
}

/*
 * Entry point
 */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    app_init();
    app_run();
    app_shutdown();

    return 0;
}
