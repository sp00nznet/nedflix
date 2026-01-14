/*
 * Nedflix for Game Boy / Game Boy Color
 * Uses GBDK-2020
 *
 * Hardware specs:
 *   Game Boy (1989):
 *     - CPU: 4.19 MHz Sharp LR35902 (Z80 variant)
 *     - RAM: 8 KB
 *     - VRAM: 8 KB
 *     - Display: 160x144, 4 shades of green
 *     - Audio: 4 channels (similar to NES)
 *
 *   Game Boy Color (1998):
 *     - CPU: 8.39 MHz (double speed mode)
 *     - RAM: 32 KB
 *     - VRAM: 16 KB
 *     - Display: 160x144, 32,768 colors
 *
 * SEVERE LIMITATIONS:
 *   - 8KB RAM (32KB on GBC) - cannot buffer anything
 *   - No PCM audio - only chip tunes
 *   - 160x144 display
 *   - No network whatsoever
 *   - Z80 cannot decode any compressed format
 *
 * WHAT THIS PORT CAN DO:
 *   - Display text information
 *   - Simple menu navigation
 *   - Play chiptune music
 *   - Act as remote via Link Cable
 */

#ifndef NEDFLIX_GB_H
#define NEDFLIX_GB_H

#include <gb/gb.h>

#define NEDFLIX_VERSION "1.0.0-gb"

/* GB is display-only */
#define NEDFLIX_CLIENT_MODE 0
#define NEDFLIX_DISPLAY_ONLY 1

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 144
#define TILE_WIDTH    20
#define TILE_HEIGHT   18

#define MAX_TITLE_LENGTH  16
#define MAX_ITEMS_VISIBLE 5

typedef enum {
    STATE_SPLASH,
    STATE_MENU,
    STATE_DISPLAY,
    STATE_SETTINGS
} app_state_t;

typedef struct {
    char title[MAX_TITLE_LENGTH];
    char artist[MAX_TITLE_LENGTH];
    UINT8 playing;
    UINT8 paused;
    UINT8 volume;
} display_info_t;

typedef struct {
    UINT8 volume;
    UINT8 contrast;  /* For original GB */
} settings_t;

typedef struct {
    app_state_t state;
    settings_t settings;
    display_info_t display;

    UINT8 keys;
    UINT8 keys_new;

    UINT16 frame_count;
    UINT8 running;
    UINT8 is_gbc;
} app_t;

extern app_t g_app;

void app_init(void);
void app_run(void);

void ui_init(void);
void ui_clear(void);
void ui_print(UINT8 x, UINT8 y, const char *text);
void ui_draw_splash(void);
void ui_draw_menu(UINT8 selected);
void ui_draw_display(void);
void ui_draw_settings(UINT8 selected);

void input_update(void);
UINT8 input_pressed(UINT8 key);

void audio_init(void);
void audio_play_tone(UINT8 channel, UINT16 freq, UINT8 duration);
void audio_stop(void);

/* Link cable communication */
void link_init(void);
UINT8 link_connected(void);
void link_send(UINT8 data);
UINT8 link_receive(void);

void config_load(settings_t *s);
void config_save(settings_t *s);

/*
 * FEATURE LIMITATIONS:
 *
 * 1. AUDIO STREAMING - Impossible
 *    - No DAC for PCM audio
 *    - 4 sound channels are chip-based only
 *    - Can only play square waves, noise, and wave table
 *    - Wave table is 32 4-bit samples (not useful for music)
 *
 * 2. VIDEO - Impossible
 *    - 160x144 4-color display (or 56 colors on GBC)
 *    - All graphics are 8x8 tiles
 *    - 8KB RAM cannot store video frames
 *    - Z80 way too slow for any decode
 *
 * 3. NETWORK - Impossible
 *    - No network hardware exists for Game Boy
 *    - Mobile Adapter GB (Japan) was cellular but Japan-only
 *    - Cannot fit TCP/IP in 8KB RAM
 *
 * 4. WHAT WORKS:
 *    - Text display from external device
 *    - Remote control commands via Link Cable
 *    - Chiptune background music
 *    - Simple settings storage
 */

#endif
