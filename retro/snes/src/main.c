/*
 * Nedflix for SNES
 * Companion display / remote control app
 *
 * This is NOT a streaming client - SNES cannot stream.
 * This displays information and sends commands via link cable.
 */

#include "nedflix.h"

app_t g_app;

void app_init(void)
{
    /* Initialize SNES hardware */
    consoleInit();

    /* Clear app state */
    g_app.state = STATE_SPLASH;
    g_app.running = 1;

    g_app.settings.volume = 80;
    g_app.settings.theme = 0;

    g_app.playback.playing = 0;
    g_app.playback.paused = 0;
    g_app.playback.volume = 80;

    g_app.media.count = 0;
    g_app.media.selected = 0;
    g_app.media.scroll = 0;

    config_load(&g_app.settings);
    ui_init();
    link_init();
}

static void state_splash(void)
{
    ui_draw_splash();

    if (g_app.frame_count > 120) {  /* 2 seconds */
        g_app.state = STATE_LINK_WAIT;
    }

    if (input_pressed(PAD_START) || input_pressed(PAD_A)) {
        g_app.state = STATE_LINK_WAIT;
    }
}

static void state_link_wait(void)
{
    ui_draw_link_wait();

    link_update();

    if (link_connected()) {
        g_app.state = STATE_MENU;
    }

    /* Allow offline mode for demo */
    if (input_pressed(PAD_START)) {
        /* Load demo data */
        strcpy(g_app.media.items[0].name, "Demo Music");
        g_app.media.items[0].type = MEDIA_TYPE_FOLDER;
        g_app.media.count = 1;
        g_app.state = STATE_MENU;
    }
}

static void state_menu(void)
{
    static u8 selected = 0;

    ui_draw_menu();

    if (input_pressed(PAD_UP)) {
        if (selected > 0) selected--;
    }
    if (input_pressed(PAD_DOWN)) {
        if (selected < 3) selected++;
    }

    if (input_pressed(PAD_A)) {
        switch (selected) {
            case 0:  /* Browse */
                g_app.state = STATE_BROWSING;
                break;
            case 1:  /* Now Playing */
                g_app.state = STATE_NOW_PLAYING;
                break;
            case 2:  /* Settings */
                g_app.state = STATE_SETTINGS;
                break;
            case 3:  /* Reconnect */
                g_app.state = STATE_LINK_WAIT;
                break;
        }
    }
}

static void state_browsing(void)
{
    ui_draw_browser();

    if (input_pressed(PAD_UP) && g_app.media.selected > 0) {
        g_app.media.selected--;
        if (g_app.media.selected < g_app.media.scroll)
            g_app.media.scroll--;
    }
    if (input_pressed(PAD_DOWN) && g_app.media.selected < g_app.media.count - 1) {
        g_app.media.selected++;
        if (g_app.media.selected >= g_app.media.scroll + MAX_ITEMS_VISIBLE)
            g_app.media.scroll++;
    }

    if (input_pressed(PAD_A) && g_app.media.count > 0) {
        media_item_t *item = &g_app.media.items[g_app.media.selected];

        if (item->type == MEDIA_TYPE_FOLDER) {
            /* Request folder contents via link */
            link_send_command(0x01, g_app.media.selected);
        } else {
            /* Request play via link */
            link_send_command(0x02, g_app.media.selected);
            g_app.state = STATE_NOW_PLAYING;
        }
    }

    if (input_pressed(PAD_B)) {
        g_app.state = STATE_MENU;
    }
}

static void state_now_playing(void)
{
    ui_draw_playback();

    /* Playback controls send commands to linked device */
    if (input_pressed(PAD_A) || input_pressed(PAD_START)) {
        /* Toggle pause */
        link_send_command(0x10, 0);
        g_app.playback.paused = !g_app.playback.paused;
    }

    if (input_pressed(PAD_LEFT)) {
        /* Previous track */
        link_send_command(0x11, 0);
    }

    if (input_pressed(PAD_RIGHT)) {
        /* Next track */
        link_send_command(0x12, 0);
    }

    if (input_pressed(PAD_L)) {
        /* Volume down */
        if (g_app.playback.volume > 0) {
            g_app.playback.volume -= 5;
            link_send_command(0x20, g_app.playback.volume);
        }
    }

    if (input_pressed(PAD_R)) {
        /* Volume up */
        if (g_app.playback.volume < 100) {
            g_app.playback.volume += 5;
            link_send_command(0x20, g_app.playback.volume);
        }
    }

    if (input_pressed(PAD_B)) {
        g_app.state = STATE_MENU;
    }

    if (input_pressed(PAD_Y)) {
        /* Stop playback */
        link_send_command(0x13, 0);
        g_app.playback.playing = 0;
    }
}

static void state_settings(void)
{
    static u8 selected = 0;

    /* Draw settings UI */
    consoleDrawText(8, 2, "SETTINGS");

    char vol_str[16];
    sprintf(vol_str, "Volume: %d%%", g_app.settings.volume);

    consoleDrawText(4, 6, selected == 0 ? "> " : "  ");
    consoleDrawText(6, 6, vol_str);

    consoleDrawText(4, 8, selected == 1 ? "> " : "  ");
    consoleDrawText(6, 8, g_app.settings.theme ? "Theme: Light" : "Theme: Dark");

    consoleDrawText(4, 10, selected == 2 ? "> " : "  ");
    consoleDrawText(6, 10, "Save");

    consoleDrawText(4, 12, selected == 3 ? "> " : "  ");
    consoleDrawText(6, 12, "Back");

    if (input_pressed(PAD_UP) && selected > 0) selected--;
    if (input_pressed(PAD_DOWN) && selected < 3) selected++;

    if (selected == 0) {
        if (input_pressed(PAD_LEFT) && g_app.settings.volume > 0)
            g_app.settings.volume -= 5;
        if (input_pressed(PAD_RIGHT) && g_app.settings.volume < 100)
            g_app.settings.volume += 5;
    }

    if (selected == 1 && input_pressed(PAD_A)) {
        g_app.settings.theme = !g_app.settings.theme;
    }

    if (input_pressed(PAD_A)) {
        if (selected == 2) config_save(&g_app.settings);
        if (selected == 3) g_app.state = STATE_MENU;
    }

    if (input_pressed(PAD_B)) {
        g_app.state = STATE_MENU;
    }
}

void app_run(void)
{
    while (g_app.running) {
        input_update();

        /* Update link data */
        link_update();

        /* Clear screen */
        consoleInitTextModeStuff();

        switch (g_app.state) {
            case STATE_SPLASH:      state_splash(); break;
            case STATE_LINK_WAIT:   state_link_wait(); break;
            case STATE_MENU:        state_menu(); break;
            case STATE_BROWSING:    state_browsing(); break;
            case STATE_NOW_PLAYING: state_now_playing(); break;
            case STATE_SETTINGS:    state_settings(); break;
        }

        WaitForVBlank();
        g_app.frame_count++;
    }
}

int main(void)
{
    app_init();
    app_run();
    return 0;
}
