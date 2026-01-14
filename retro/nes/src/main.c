/*
 * Nedflix for NES
 * Display terminal / companion app
 *
 * NES cannot stream - this displays info from external source.
 */

#include "nedflix.h"
#include <string.h>

app_t g_app;

void app_init(void)
{
    g_app.state = STATE_SPLASH;
    g_app.running = 1;
    g_app.frame_count = 0;

    g_app.settings.volume = 80;
    g_app.settings.brightness = 100;

    strcpy(g_app.display.title, "No Media");
    strcpy(g_app.display.artist, "");
    g_app.display.playing = 0;
    g_app.display.paused = 0;
    g_app.display.volume = 80;

    ui_init();
    audio_init();
    config_load(&g_app.settings);
}

static void state_splash(void)
{
    ui_draw_splash();

    if (g_app.frame_count > 120) {
        g_app.state = STATE_MENU;
        ui_clear();
    }

    if (input_pressed(PAD_START) || input_pressed(PAD_A)) {
        g_app.state = STATE_MENU;
        ui_clear();
    }
}

static void state_menu(void)
{
    static unsigned char selected = 0;

    ui_draw_menu(selected);

    if (input_pressed(PAD_UP) && selected > 0) {
        selected--;
    }
    if (input_pressed(PAD_DOWN) && selected < 2) {
        selected++;
    }

    if (input_pressed(PAD_A)) {
        switch (selected) {
            case 0:  /* Now Playing Display */
                g_app.state = STATE_DISPLAY;
                ui_clear();
                break;
            case 1:  /* Settings */
                g_app.state = STATE_SETTINGS;
                ui_clear();
                break;
            case 2:  /* About */
                /* Show about info */
                break;
        }
    }
}

static void state_display(void)
{
    ui_draw_display();

    /* In a real implementation, this would receive
     * data from external source via expansion port */

    if (input_pressed(PAD_B)) {
        g_app.state = STATE_MENU;
        ui_clear();
    }

    /* Simulated playback controls - would send to external */
    if (input_pressed(PAD_A)) {
        g_app.display.paused = !g_app.display.paused;
    }
}

static void state_settings(void)
{
    static unsigned char selected = 0;

    ui_draw_settings(selected);

    if (input_pressed(PAD_UP) && selected > 0) {
        selected--;
    }
    if (input_pressed(PAD_DOWN) && selected < 2) {
        selected++;
    }

    if (selected == 0) {
        if (input_pressed(PAD_LEFT) && g_app.settings.volume > 0)
            g_app.settings.volume -= 10;
        if (input_pressed(PAD_RIGHT) && g_app.settings.volume < 100)
            g_app.settings.volume += 10;
    }

    if (input_pressed(PAD_A) && selected == 1) {
        config_save(&g_app.settings);
    }

    if (input_pressed(PAD_B) || (input_pressed(PAD_A) && selected == 2)) {
        g_app.state = STATE_MENU;
        ui_clear();
    }
}

void app_run(void)
{
    while (g_app.running) {
        input_update();

        switch (g_app.state) {
            case STATE_SPLASH:   state_splash(); break;
            case STATE_MENU:     state_menu(); break;
            case STATE_DISPLAY:  state_display(); break;
            case STATE_SETTINGS: state_settings(); break;
        }

        ppu_wait_nmi();
        g_app.frame_count++;
    }
}

void main(void)
{
    app_init();
    app_run();
}
