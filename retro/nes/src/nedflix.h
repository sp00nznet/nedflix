/*
 * Nedflix for Nintendo Entertainment System (NES)
 * Uses cc65/NESLib
 *
 * Hardware specs (1983):
 *   - CPU: 1.79 MHz Ricoh 2A03 (6502 variant)
 *   - RAM: 2 KB
 *   - VRAM: 2 KB
 *   - Audio: 5 channels (2 pulse, 1 triangle, 1 noise, 1 DPCM)
 *   - Storage: Battery SRAM via mapper (8KB max)
 *
 * SEVERE LIMITATIONS:
 *   - 2KB RAM - cannot store ANY meaningful data
 *   - No network capability
 *   - Audio channels cannot play sampled audio (DPCM is 1-bit, ~8KB max)
 *   - Display is 256x240 with only 4 colors per 16x16 block
 *
 * WHAT THIS PORT CAN DO:
 *   - Display static "now playing" text information
 *   - Act as display-only terminal for external system
 *   - Show simple menu interface
 *   - Play chiptune versions of theme music
 *
 * This is a DISPLAY TERMINAL only, not a media client.
 */

#ifndef NEDFLIX_NES_H
#define NEDFLIX_NES_H

#include <nes.h>
#include <neslib.h>

#define NEDFLIX_VERSION "1.0.0-nes"

/* NES is display-only */
#define NEDFLIX_CLIENT_MODE 0
#define NEDFLIX_DISPLAY_ONLY 1

#define SCREEN_WIDTH  256
#define SCREEN_HEIGHT 240
#define TILE_WIDTH    32
#define TILE_HEIGHT   30

#define MAX_TITLE_LENGTH  20
#define MAX_ITEMS_VISIBLE 6

typedef enum {
    STATE_SPLASH,
    STATE_MENU,
    STATE_DISPLAY,
    STATE_SETTINGS
} app_state_t;

typedef struct {
    char title[MAX_TITLE_LENGTH];
    char artist[MAX_TITLE_LENGTH];
    unsigned char playing;
    unsigned char paused;
    unsigned char volume;
} display_info_t;

typedef struct {
    unsigned char volume;
    unsigned char brightness;
} settings_t;

typedef struct {
    app_state_t state;
    settings_t settings;
    display_info_t display;

    unsigned char pad;
    unsigned char pad_new;

    unsigned int frame_count;
    unsigned char running;
} app_t;

extern app_t g_app;

void app_init(void);
void app_run(void);

void ui_init(void);
void ui_clear(void);
void ui_draw_text(unsigned char x, unsigned char y, const char *text);
void ui_draw_splash(void);
void ui_draw_menu(unsigned char selected);
void ui_draw_display(void);
void ui_draw_settings(unsigned char selected);

void input_update(void);
unsigned char input_pressed(unsigned char btn);

void audio_init(void);
void audio_play_jingle(unsigned char id);
void audio_stop(void);

void config_load(settings_t *s);
void config_save(settings_t *s);

/*
 * FEATURE LIMITATIONS:
 *
 * 1. NO AUDIO STREAMING - Impossible
 *    - NES has no DAC for PCM audio
 *    - DPCM channel can play ~8KB of 1-bit samples total
 *    - Memory cannot hold any meaningful audio data
 *    - Only chiptune (square/triangle waves) possible
 *
 * 2. NO VIDEO - Impossible
 *    - NES has no framebuffer
 *    - All graphics are 8x8 tiles, max 256 unique tiles
 *    - Cannot display arbitrary images
 *    - Cannot decode any video format
 *
 * 3. NO NETWORK - Impossible
 *    - NES has no network hardware
 *    - Famicom Modem (Japan) was dial-up for specific services
 *    - No TCP/IP stack possible in 2KB RAM
 *
 * 4. WHAT WORKS:
 *    - Text display of metadata from external source
 *    - Simple menu navigation
 *    - Settings storage in battery SRAM
 *    - Chiptune background music
 */

#endif
