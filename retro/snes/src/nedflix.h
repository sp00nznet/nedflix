/*
 * Nedflix for Super Nintendo (SNES)
 * Uses PVSnesLib
 *
 * Hardware specs (1990):
 *   - CPU: 3.58 MHz Ricoh 5A22 (65816)
 *   - RAM: 128 KB
 *   - VRAM: 64 KB
 *   - Audio: Sony SPC700 + DSP, 64KB audio RAM
 *   - Storage: Battery-backed SRAM (8KB typical)
 *
 * LIMITATIONS - Be honest:
 *   - NO network capability whatsoever
 *   - 128KB RAM cannot buffer any meaningful audio
 *   - CPU too slow for any real-time decode
 *
 * WHAT THIS PORT CAN DO:
 *   - Display pre-rendered media information
 *   - Act as a "remote control" via link cable to PC
 *   - Display album art as Mode 7 graphics
 *   - Play short pre-converted SPC audio samples
 *
 * This is primarily a COMPANION app, not standalone.
 */

#ifndef NEDFLIX_SNES_H
#define NEDFLIX_SNES_H

#include <snes.h>

#define NEDFLIX_VERSION "1.0.0-snes"

/* SNES has no network - this is a companion/display app */
#define NEDFLIX_CLIENT_MODE 0
#define NEDFLIX_COMPANION_MODE 1

#define SCREEN_WIDTH  256
#define SCREEN_HEIGHT 224

#define MAX_TITLE_LENGTH  32
#define MAX_ITEMS_VISIBLE 8
#define MAX_MEDIA_ITEMS   16

typedef enum {
    STATE_SPLASH,
    STATE_MENU,
    STATE_BROWSING,
    STATE_NOW_PLAYING,
    STATE_SETTINGS,
    STATE_LINK_WAIT
} app_state_t;

typedef enum {
    MEDIA_TYPE_FOLDER,
    MEDIA_TYPE_MUSIC,
    MEDIA_TYPE_VIDEO
} media_type_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    media_type_t type;
    u8 icon_id;
} media_item_t;

typedef struct {
    media_item_t items[MAX_MEDIA_ITEMS];
    u8 count;
    u8 selected;
    u8 scroll;
} media_list_t;

typedef struct {
    char title[MAX_TITLE_LENGTH];
    char artist[MAX_TITLE_LENGTH];
    u8 playing;
    u8 paused;
    u16 position_sec;
    u16 duration_sec;
    u8 volume;
} playback_t;

typedef struct {
    u8 volume;
    u8 theme;  /* 0=dark, 1=light */
} settings_t;

typedef struct {
    app_state_t state;
    settings_t settings;
    playback_t playback;
    media_list_t media;

    u16 buttons;
    u16 buttons_new;

    u16 frame_count;
    u8 running;
} app_t;

extern app_t g_app;

void app_init(void);
void app_run(void);

void ui_init(void);
void ui_draw_splash(void);
void ui_draw_menu(void);
void ui_draw_browser(void);
void ui_draw_playback(void);
void ui_draw_link_wait(void);

void input_update(void);
u8 input_pressed(u16 btn);
u8 input_held(u16 btn);

/* Link cable communication (if available) */
void link_init(void);
void link_update(void);
u8 link_connected(void);
void link_send_command(u8 cmd, u8 param);
u8 link_receive(u8 *data, u8 max_len);

void config_load(settings_t *s);
void config_save(settings_t *s);

/*
 * FEATURE LIMITATIONS EXPLAINED:
 *
 * 1. NO AUDIO STREAMING
 *    - SNES has 64KB audio RAM, can only hold ~4 seconds of audio
 *    - SPC700 processor runs at 1MHz, cannot decode compressed audio
 *    - Only pre-converted SPC/BRR samples can play
 *
 * 2. NO VIDEO
 *    - Mode 7 can display single 128x128 rotating image
 *    - No framebuffer - all graphics are tile-based
 *    - CPU cannot decode any video codec
 *
 * 3. NO NETWORK
 *    - SNES has no network hardware
 *    - Satellaview addon (Japan only) had satellite download, not internet
 *    - XBAND modem was dial-up for multiplayer games only
 *
 * 4. WHAT WORKS
 *    - Display now-playing info synced from external source
 *    - Act as remote control sending commands via link cable
 *    - Display album art as Mode 7 background
 *    - Play short jingles as SPC samples
 */

#endif
