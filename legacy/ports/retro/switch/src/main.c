/*
 * Nedflix Nintendo Switch - Main application
 * Full-featured streaming client using libnx
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <switch.h>

/* Global application state */
static app_t app;

/* Forward declarations */
static void app_init(void);
static void app_shutdown(void);
static void app_update(void);
static void app_render(void);
static void process_input(void);
static void change_state(app_state_t new_state);

/* State handlers */
static void state_init(void);
static void state_splash(void);
static void state_network_init(void);
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
static void state_shutdown_handler(void);

/* State function table */
typedef void (*state_func_t)(void);
static const state_func_t state_handlers[] = {
    [STATE_INIT] = state_init,
    [STATE_SPLASH] = state_splash,
    [STATE_NETWORK_INIT] = state_network_init,
    [STATE_CONNECTING] = state_connecting,
    [STATE_LOGIN] = state_login,
    [STATE_PROFILE_SELECT] = state_profile_select,
    [STATE_MAIN_MENU] = state_main_menu,
    [STATE_BROWSING] = state_browsing,
    [STATE_SEARCH] = state_search,
    [STATE_DETAIL] = state_detail,
    [STATE_PLAYING] = state_playing,
    [STATE_LIVE_TV] = state_live_tv,
    [STATE_CHANNELS] = state_channels,
    [STATE_SETTINGS] = state_settings,
    [STATE_ABOUT] = state_about,
    [STATE_ERROR] = state_error,
    [STATE_SHUTDOWN] = state_shutdown_handler,
};

/* Initialize application */
static void app_init(void)
{
    printf("Nedflix Switch initializing...\n");

    memset(&app, 0, sizeof(app_t));
    app.state = STATE_INIT;
    app.running = true;
    app.docked = appletGetOperationMode() == AppletOperationMode_Console;

    /* Set resolution based on mode */
    if (app.docked) {
        app.screen_width = 1920;
        app.screen_height = 1080;
    } else {
        app.screen_width = 1280;
        app.screen_height = 720;
    }

    /* Initialize framebuffer */
    app.window = nwindowGetDefault();
    framebufferCreate(&app.fb, app.window, app.screen_width, app.screen_height,
                      PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&app.fb);

    /* Initialize input */
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&app.pad);

    /* Initialize touch */
    hidInitializeTouchScreen();

    /* Initialize subsystems */
    if (audio_init() != 0) {
        printf("Warning: Audio init failed\n");
    }

    if (video_init() != 0) {
        printf("Warning: Video init failed\n");
    }

    if (network_init() != 0) {
        printf("Warning: Network init failed\n");
    }

    /* Load configuration */
    config_load(&app.settings);
    config_load_favorites();
    config_load_history();

    /* Initialize UI */
    ui_init();

    app.initialized = true;
    printf("Nedflix Switch initialized\n");
}

/* Shutdown application */
static void app_shutdown(void)
{
    printf("Nedflix Switch shutting down...\n");

    /* Save settings */
    config_save(&app.settings);

    /* Shutdown subsystems */
    ui_shutdown();
    video_shutdown();
    audio_shutdown();
    network_shutdown();

    /* Close framebuffer */
    framebufferClose(&app.fb);

    app.initialized = false;
    printf("Nedflix Switch shutdown complete\n");
}

/* Change application state */
static void change_state(app_state_t new_state)
{
    printf("State change: %d -> %d\n", app.state, new_state);
    app.prev_state = app.state;
    app.state = new_state;
    app.state_timer = 0;

    /* State entry actions */
    switch (new_state) {
        case STATE_PLAYING:
            appletSetMediaPlaybackState(true);
            break;
        case STATE_MAIN_MENU:
        case STATE_BROWSING:
            appletSetMediaPlaybackState(false);
            break;
        default:
            break;
    }
}

/* Process controller and touch input */
static void process_input(void)
{
    /* Update pad state */
    padUpdate(&app.pad);

    u64 down = padGetButtonsDown(&app.pad);
    u64 held = padGetButtons(&app.pad);

    app.buttons_pressed = down;
    app.buttons_held = held;

    /* Analog sticks */
    HidAnalogStickState left = padGetStickPos(&app.pad, 0);
    HidAnalogStickState right = padGetStickPos(&app.pad, 1);

    app.stick_lx = left.x;
    app.stick_ly = left.y;
    app.stick_rx = right.x;
    app.stick_ry = right.y;

    /* Touch input (handheld mode) */
    if (!app.docked) {
        HidTouchScreenState touch_state = {0};
        if (hidGetTouchScreenStates(&touch_state, 1)) {
            if (touch_state.count > 0) {
                app.touch.prev_x = app.touch.x;
                app.touch.prev_y = app.touch.y;
                app.touch.x = touch_state.touches[0].x;
                app.touch.y = touch_state.touches[0].y;

                if (!app.touch.touched) {
                    app.touch.touched = true;
                    app.touch.touch_start = armGetSystemTick();
                    app.touch.prev_x = app.touch.x;
                    app.touch.prev_y = app.touch.y;
                }

                app.touch.delta_x = app.touch.x - app.touch.prev_x;
                app.touch.delta_y = app.touch.y - app.touch.prev_y;
            } else {
                app.touch.touched = false;
            }
        }
    }

    /* Global shortcuts */
    if (down & HidNpadButton_Plus) {
        if (app.state == STATE_PLAYING) {
            /* Pause/show overlay */
            video_pause();
        } else if (app.state != STATE_INIT && app.state != STATE_SHUTDOWN) {
            /* Quick settings */
            change_state(STATE_SETTINGS);
        }
    }

    if (down & HidNpadButton_Minus) {
        if (app.state == STATE_PLAYING) {
            audio_stop();
            video_stop();
            change_state(STATE_BROWSING);
        }
    }
}

/* State: Initialization */
static void state_init(void)
{
    app.state_timer++;
    if (app.state_timer > 30) {
        change_state(STATE_SPLASH);
    }
}

/* State: Splash screen */
static void state_splash(void)
{
    app.state_timer++;

    /* Show splash for 2 seconds or until button press */
    if (app.state_timer > 120 || app.buttons_pressed) {
        change_state(STATE_NETWORK_INIT);
    }
}

/* State: Network initialization */
static void state_network_init(void)
{
    NifmInternetConnectionStatus status;
    Result rc = nifmGetInternetConnectionStatus(NULL, NULL, &status);

    if (R_SUCCEEDED(rc) && status == NifmInternetConnectionStatus_Connected) {
        change_state(STATE_CONNECTING);
    } else {
        app.state_timer++;
        if (app.state_timer > 300) {
            snprintf(app.error_msg, sizeof(app.error_msg),
                    "Network connection required.\nPlease check your settings.");
            change_state(STATE_ERROR);
        }
    }
}

/* State: Connecting to server */
static void state_connecting(void)
{
    static int connect_attempt = 0;

    app.state_timer++;

    if (app.state_timer == 1) {
        connect_attempt++;
        printf("Connection attempt %d\n", connect_attempt);
    }

    /* Simulate connection (real impl would use network_connect) */
    if (app.state_timer > 60) {
        if (app.settings.remember_login && app.auth.logged_in) {
            change_state(STATE_MAIN_MENU);
        } else {
            change_state(STATE_LOGIN);
        }
    }
}

/* State: Login screen */
static void state_login(void)
{
    /* Handle login UI */
    if (app.buttons_pressed & BTN_A) {
        /* For demo: auto-login */
        app.auth.logged_in = true;
        strncpy(app.auth.username, "User", sizeof(app.auth.username) - 1);
        change_state(STATE_PROFILE_SELECT);
    }

    if (app.buttons_pressed & BTN_B) {
        /* Guest mode */
        change_state(STATE_MAIN_MENU);
    }
}

/* State: Profile selection */
static void state_profile_select(void)
{
    if (app.buttons_pressed & BTN_A) {
        change_state(STATE_MAIN_MENU);
    }

    if (app.buttons_pressed & BTN_B) {
        change_state(STATE_LOGIN);
    }
}

/* State: Main menu */
static void state_main_menu(void)
{
    /* Navigate menu items */
    if (app.buttons_pressed & BTN_DPAD_UP) {
        if (app.menu_index > 0) app.menu_index--;
    }
    if (app.buttons_pressed & BTN_DPAD_DOWN) {
        if (app.menu_index < 5) app.menu_index++;
    }

    if (app.buttons_pressed & BTN_A) {
        switch (app.menu_index) {
            case 0: /* Movies */
                app.current_library = LIBRARY_MOVIES;
                change_state(STATE_BROWSING);
                break;
            case 1: /* TV Shows */
                app.current_library = LIBRARY_TVSHOWS;
                change_state(STATE_BROWSING);
                break;
            case 2: /* Music */
                app.current_library = LIBRARY_MUSIC;
                change_state(STATE_BROWSING);
                break;
            case 3: /* Live TV */
                change_state(STATE_LIVE_TV);
                break;
            case 4: /* Search */
                change_state(STATE_SEARCH);
                break;
            case 5: /* Settings */
                change_state(STATE_SETTINGS);
                break;
        }
    }
}

/* State: Content browsing */
static void state_browsing(void)
{
    /* Grid navigation */
    if (app.buttons_pressed & BTN_DPAD_LEFT) {
        if (app.grid_x > 0) app.grid_x--;
    }
    if (app.buttons_pressed & BTN_DPAD_RIGHT) {
        if (app.grid_x < app.grid_cols - 1) app.grid_x++;
    }
    if (app.buttons_pressed & BTN_DPAD_UP) {
        if (app.grid_y > 0) {
            app.grid_y--;
        } else if (app.category_index > 0) {
            app.category_index--;
            app.grid_y = 0;
        }
    }
    if (app.buttons_pressed & BTN_DPAD_DOWN) {
        if (app.grid_y < app.grid_rows - 1) {
            app.grid_y++;
        } else if (app.category_index < app.category_count - 1) {
            app.category_index++;
            app.grid_y = 0;
        }
    }

    /* Scroll with analog stick */
    if (app.stick_ly < -20000) {
        app.scroll_offset += 8;
    } else if (app.stick_ly > 20000) {
        if (app.scroll_offset > 0) app.scroll_offset -= 8;
    }

    /* Select item */
    if (app.buttons_pressed & BTN_A) {
        change_state(STATE_DETAIL);
    }

    /* Back to main menu */
    if (app.buttons_pressed & BTN_B) {
        change_state(STATE_MAIN_MENU);
    }

    /* Quick actions */
    if (app.buttons_pressed & BTN_X) {
        /* Toggle favorite */
        if (app.current_item.id[0]) {
            if (config_is_favorite(app.current_item.id)) {
                config_remove_favorite(app.current_item.id);
            } else {
                config_add_favorite(app.current_item.id);
            }
        }
    }

    if (app.buttons_pressed & BTN_Y) {
        change_state(STATE_SEARCH);
    }
}

/* State: Search */
static void state_search(void)
{
    /* Software keyboard handling would go here */
    if (app.buttons_pressed & BTN_B) {
        change_state(app.prev_state);
    }

    if (app.buttons_pressed & BTN_A) {
        /* Execute search */
        /* In real impl: call network_search() */
    }
}

/* State: Content detail view */
static void state_detail(void)
{
    if (app.buttons_pressed & BTN_A) {
        /* Play content */
        change_state(STATE_PLAYING);
    }

    if (app.buttons_pressed & BTN_X) {
        /* Toggle favorite */
        if (config_is_favorite(app.current_item.id)) {
            config_remove_favorite(app.current_item.id);
        } else {
            config_add_favorite(app.current_item.id);
        }
    }

    if (app.buttons_pressed & BTN_B) {
        change_state(STATE_BROWSING);
    }
}

/* State: Media playback */
static void state_playing(void)
{
    /* Update playback */
    video_update();
    audio_update();

    /* Show/hide controls overlay */
    if (app.buttons_pressed) {
        app.show_controls = true;
        app.controls_timer = 0;
    }

    app.controls_timer++;
    if (app.controls_timer > 300) {
        app.show_controls = false;
    }

    /* Playback controls */
    if (app.buttons_pressed & BTN_A) {
        if (video_is_playing()) {
            video_pause();
        } else {
            video_resume();
        }
    }

    /* Seek */
    if (app.buttons_held & BTN_DPAD_RIGHT) {
        video_seek(10000);  /* Forward 10s */
    }
    if (app.buttons_held & BTN_DPAD_LEFT) {
        video_seek(-10000);  /* Back 10s */
    }

    /* Skip chapter/episode */
    if (app.buttons_pressed & BTN_R) {
        video_seek(60000);  /* Forward 1 min */
    }
    if (app.buttons_pressed & BTN_L) {
        video_seek(-60000);  /* Back 1 min */
    }

    /* Volume control */
    if (app.buttons_pressed & BTN_ZR) {
        app.settings.volume = MIN(app.settings.volume + 5, 100);
        audio_set_volume(app.settings.volume);
    }
    if (app.buttons_pressed & BTN_ZL) {
        app.settings.volume = MAX(app.settings.volume - 5, 0);
        audio_set_volume(app.settings.volume);
    }

    /* Stop and return */
    if (app.buttons_pressed & BTN_B) {
        /* Save progress */
        config_add_history(app.current_item.id, app.current_item.title,
                          video_get_position(), video_get_duration(),
                          app.current_item.type);
        video_stop();
        audio_stop();
        change_state(STATE_DETAIL);
    }

    /* Check for end of playback */
    if (!video_is_playing() && !video_is_buffering() && app.playback.position > 0) {
        /* Playback ended */
        if (app.settings.autoplay && app.current_item.type == CONTENT_EPISODE) {
            /* Auto-play next episode */
            printf("Auto-playing next episode\n");
        } else {
            change_state(STATE_DETAIL);
        }
    }
}

/* State: Live TV */
static void state_live_tv(void)
{
    /* Channel navigation */
    if (app.buttons_pressed & BTN_DPAD_UP) {
        /* Previous channel */
    }
    if (app.buttons_pressed & BTN_DPAD_DOWN) {
        /* Next channel */
    }

    if (app.buttons_pressed & BTN_A) {
        change_state(STATE_PLAYING);
    }

    if (app.buttons_pressed & BTN_B) {
        change_state(STATE_MAIN_MENU);
    }
}

/* State: Channel guide */
static void state_channels(void)
{
    if (app.buttons_pressed & BTN_B) {
        change_state(STATE_LIVE_TV);
    }
}

/* State: Settings */
static void state_settings(void)
{
    /* Navigate settings */
    if (app.buttons_pressed & BTN_DPAD_UP) {
        if (app.settings_index > 0) app.settings_index--;
    }
    if (app.buttons_pressed & BTN_DPAD_DOWN) {
        if (app.settings_index < 10) app.settings_index++;
    }

    /* Adjust values */
    if (app.buttons_pressed & BTN_DPAD_LEFT) {
        switch (app.settings_index) {
            case 0: /* Volume */
                app.settings.volume = MAX(0, app.settings.volume - 5);
                audio_set_volume(app.settings.volume);
                break;
            case 1: /* Video quality */
                if (app.settings.video_quality > 0) app.settings.video_quality--;
                break;
            case 2: /* Subtitles */
                app.settings.show_subtitles = !app.settings.show_subtitles;
                break;
        }
    }
    if (app.buttons_pressed & BTN_DPAD_RIGHT) {
        switch (app.settings_index) {
            case 0: /* Volume */
                app.settings.volume = MIN(100, app.settings.volume + 5);
                audio_set_volume(app.settings.volume);
                break;
            case 1: /* Video quality */
                if (app.settings.video_quality < 4) app.settings.video_quality++;
                break;
            case 2: /* Subtitles */
                app.settings.show_subtitles = !app.settings.show_subtitles;
                break;
        }
    }

    /* Save and exit */
    if (app.buttons_pressed & BTN_B) {
        config_save(&app.settings);
        change_state(app.prev_state);
    }

    /* About screen */
    if (app.buttons_pressed & BTN_Y) {
        change_state(STATE_ABOUT);
    }
}

/* State: About */
static void state_about(void)
{
    if (app.buttons_pressed & (BTN_A | BTN_B)) {
        change_state(STATE_SETTINGS);
    }
}

/* State: Error */
static void state_error(void)
{
    if (app.buttons_pressed & BTN_A) {
        /* Retry */
        change_state(STATE_INIT);
    }

    if (app.buttons_pressed & BTN_B) {
        /* Exit */
        app.running = false;
    }
}

/* State: Shutdown */
static void state_shutdown_handler(void)
{
    app.running = false;
}

/* Update application */
static void app_update(void)
{
    /* Check for dock/undock */
    bool now_docked = appletGetOperationMode() == AppletOperationMode_Console;
    if (now_docked != app.docked) {
        app.docked = now_docked;
        if (app.docked) {
            app.screen_width = 1920;
            app.screen_height = 1080;
        } else {
            app.screen_width = 1280;
            app.screen_height = 720;
        }
        /* Recreate framebuffer for new resolution */
        framebufferClose(&app.fb);
        framebufferCreate(&app.fb, app.window, app.screen_width, app.screen_height,
                          PIXEL_FORMAT_RGBA_8888, 2);
        framebufferMakeLinear(&app.fb);
    }

    /* Process input */
    process_input();

    /* Run state handler */
    if (app.state < sizeof(state_handlers) / sizeof(state_handlers[0])) {
        if (state_handlers[app.state]) {
            state_handlers[app.state]();
        }
    }
}

/* Render application */
static void app_render(void)
{
    /* Get framebuffer */
    u32 stride;
    app.framebuffer = (uint32_t*)framebufferBegin(&app.fb, &stride);

    if (!app.framebuffer) return;

    /* Render current state */
    ui_render(&app);

    /* Present */
    framebufferEnd(&app.fb);
}

/* Main entry point */
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /* Initialize console for debug output */
    consoleInit(NULL);
    printf("Nedflix Switch starting...\n");
    consoleExit(NULL);

    /* Initialize services */
    socketInitializeDefault();
    nifmInitialize(NifmServiceType_User);
    plInitialize(PlServiceType_User);
    romfsInit();

    /* Initialize application */
    app_init();

    /* Main loop */
    while (app.running && appletMainLoop()) {
        app_update();
        app_render();
    }

    /* Cleanup */
    app_shutdown();

    romfsExit();
    plExit();
    nifmExit();
    socketExit();

    return 0;
}

/* Get application state (for other modules) */
app_t *app_get_state(void)
{
    return &app;
}
