/*
 * Nedflix for Game Boy Advance
 * Uses devkitARM + libgba
 *
 * Hardware specs (2001):
 *   - CPU: 16.78 MHz ARM7TDMI
 *   - RAM: 32 KB internal + 256 KB external
 *   - VRAM: 96 KB
 *   - Audio: 4 GB channels + 2 direct sound (PCM)
 *   - Display: 240x160, 15-bit color
 *   - Storage: Cartridge SRAM/Flash (32-128KB)
 *
 * GBA CAPABILITIES:
 *   - ARM CPU can decode simple audio (maybe low-bitrate)
 *   - Direct sound channels support 8-bit PCM
 *   - 256KB EWRAM can buffer some audio data
 *   - Nice color display for UI
 *
 * LIMITATIONS:
 *   - NO network (would need link cable + adapter)
 *   - Very limited audio buffer (~2 seconds at 22kHz)
 *   - No video decode capability
 *   - Battery life issues with heavy CPU use
 *
 * WHAT THIS PORT DOES:
 *   - Display now-playing info
 *   - Act as remote control via Wireless Adapter
 *   - Play short audio clips from cartridge
 *   - Show album art as bitmap
 */

#ifndef NEDFLIX_GBA_H
#define NEDFLIX_GBA_H

#include <gba.h>

#define NEDFLIX_VERSION "1.0.0-gba"

/* GBA is primarily companion mode */
#define NEDFLIX_CLIENT_MODE 0
#define NEDFLIX_COMPANION_MODE 1

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 160

#define MAX_PATH_LENGTH   64
#define MAX_TITLE_LENGTH  32
#define MAX_ITEMS_VISIBLE 6
#define MAX_MEDIA_ITEMS   24

/* Small audio buffer - GBA has limited RAM */
#define AUDIO_BUFFER_SIZE (16 * 1024)  /* 16KB */
#define AUDIO_SAMPLE_RATE 22050

typedef enum {
    STATE_SPLASH,
    STATE_MENU,
    STATE_BROWSING,
    STATE_NOW_PLAYING,
    STATE_SETTINGS,
    STATE_LINK_MODE
} app_state_t;

typedef enum {
    MEDIA_TYPE_FOLDER,
    MEDIA_TYPE_MUSIC,
    MEDIA_TYPE_PODCAST
} media_type_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    media_type_t type;
    u16 duration_sec;
} media_item_t;

typedef struct {
    media_item_t items[MAX_MEDIA_ITEMS];
    u8 count;
    u8 selected;
    u8 scroll;
    char path[MAX_PATH_LENGTH];
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
    u8 brightness;
    u8 auto_sleep;
} settings_t;

typedef struct {
    app_state_t state;
    settings_t settings;
    playback_t playback;
    media_list_t media;

    u16 keys;
    u16 keys_new;

    u32 frame_count;
    u8 running;
    u8 link_connected;
} app_t;

extern app_t g_app;

void app_init(void);
void app_run(void);

void ui_init(void);
void ui_clear(void);
void ui_draw_text(int x, int y, const char *text, u16 color);
void ui_draw_rect(int x, int y, int w, int h, u16 color);
void ui_draw_splash(void);
void ui_draw_menu(int selected);
void ui_draw_browser(void);
void ui_draw_playback(void);
void ui_draw_settings(int selected);

void input_update(void);
int input_pressed(u16 key);
int input_held(u16 key);

void audio_init(void);
void audio_shutdown(void);
int audio_play_sample(const u8 *data, u32 length);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
void audio_set_volume(int vol);
int audio_is_playing(void);

/* Link cable / Wireless Adapter functions */
void link_init(void);
void link_update(void);
int link_connected(void);
void link_send(u8 cmd, u8 param);
int link_receive(u8 *data, u8 max_len);

void config_load(settings_t *s);
void config_save(settings_t *s);

#define RGB15(r,g,b) ((r) | ((g)<<5) | ((b)<<10))
#define COLOR_BLACK   RGB15(0,0,0)
#define COLOR_WHITE   RGB15(31,31,31)
#define COLOR_RED     RGB15(28,1,2)
#define COLOR_GRAY    RGB15(16,16,16)
#define COLOR_DARK    RGB15(2,2,2)

/*
 * FEATURE LIMITATIONS:
 *
 * 1. AUDIO STREAMING - Very Limited
 *    - Direct sound supports 8-bit PCM at up to 32kHz
 *    - 256KB EWRAM limits buffer to ~11 seconds at 22kHz
 *    - ARM7 can decode low-bitrate audio but uses battery
 *    - No network = audio must come from cartridge or link
 *
 * 2. VIDEO - Not Feasible
 *    - ARM7 too slow to decode video
 *    - No hardware video decode
 *    - Display is only 240x160
 *    - RAM cannot buffer video frames
 *
 * 3. NETWORK - Hardware Required
 *    - Wireless Adapter provides local link only
 *    - Mobile Adapter GB (Japan) was slow cellular
 *    - No TCP/IP stack fits in available RAM
 *    - Can act as remote via Wireless Adapter to main device
 *
 * 4. WHAT WORKS:
 *    - Display now-playing synchronized from PC/phone
 *    - Remote control commands via Wireless Adapter
 *    - Play short audio clips stored on cartridge
 *    - Show album art as 240x160 bitmap
 */

#endif
