/*
 * Nedflix for PlayStation 1 (PSX)
 * Uses PSn00bSDK (open source PS1 SDK)
 *
 * Hardware specs (1994):
 *   - CPU: 33.8688 MHz MIPS R3000A
 *   - RAM: 2 MB main, 1 MB VRAM
 *   - GPU: 2D sprite/polygon engine
 *   - SPU: 24 channels, 512KB sound RAM
 *   - CD-ROM: 2x speed (300KB/s)
 *   - Storage: Memory Card (128KB)
 *
 * Features possible:
 *   - Audio streaming (ADPCM/XA)
 *   - Basic UI with sprites
 *   - Network via Yaroze Net (rare) or serial link
 *
 * Limitations:
 *   - 2MB RAM severely limits buffering
 *   - No native TCP/IP stack
 *   - Video playback limited to MDEC (320x240)
 */

#ifndef NEDFLIX_PSX_H
#define NEDFLIX_PSX_H

#include <stdint.h>
#include <stdbool.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxspu.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>

#define NEDFLIX_VERSION "1.0.0-psx"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

#define MAX_PATH_LENGTH   128
#define MAX_URL_LENGTH    192
#define MAX_TITLE_LENGTH  48
#define MAX_ITEMS_VISIBLE 8
#define MAX_MEDIA_ITEMS   24

#define STREAM_BUFFER_SIZE (32 * 1024)

typedef enum {
    STATE_INIT,
    STATE_NETWORK_INIT,
    STATE_CONNECTING,
    STATE_LOGIN,
    STATE_MENU,
    STATE_BROWSING,
    STATE_PLAYING,
    STATE_SETTINGS,
    STATE_ERROR
} app_state_t;

typedef enum {
    MEDIA_TYPE_UNKNOWN,
    MEDIA_TYPE_DIRECTORY,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_VIDEO
} media_type_t;

typedef enum {
    LIBRARY_MUSIC,
    LIBRARY_AUDIOBOOKS,
    LIBRARY_MOVIES,
    LIBRARY_TVSHOWS,
    LIBRARY_COUNT
} library_t;

typedef enum {
    BTN_CROSS     = (1 << 0),
    BTN_CIRCLE    = (1 << 1),
    BTN_SQUARE    = (1 << 2),
    BTN_TRIANGLE  = (1 << 3),
    BTN_START     = (1 << 4),
    BTN_SELECT    = (1 << 5),
    BTN_UP        = (1 << 6),
    BTN_DOWN      = (1 << 7),
    BTN_LEFT      = (1 << 8),
    BTN_RIGHT     = (1 << 9),
    BTN_L1        = (1 << 10),
    BTN_R1        = (1 << 11),
    BTN_L2        = (1 << 12),
    BTN_R2        = (1 << 13)
} button_mask_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    media_type_t type;
    bool is_directory;
    uint16_t duration;
} media_item_t;

typedef struct {
    media_item_t items[MAX_MEDIA_ITEMS];
    int16_t count;
    int16_t selected_index;
    int16_t scroll_offset;
    char current_path[MAX_PATH_LENGTH];
} media_list_t;

typedef struct {
    char server_url[MAX_URL_LENGTH];
    char username[24];
    char session_token[48];
    uint8_t volume;
    uint8_t library;
    bool autoplay;
} user_settings_t;

typedef struct {
    char title[MAX_TITLE_LENGTH];
    char url[MAX_URL_LENGTH];
    bool playing;
    bool paused;
    bool is_audio;
    uint32_t position_ms;
    uint32_t duration_ms;
    uint8_t volume;
} playback_t;

typedef struct {
    app_state_t state;
    user_settings_t settings;
    playback_t playback;
    media_list_t media;
    library_t current_library;

    uint16_t buttons_pressed;
    uint16_t buttons_just_pressed;

    uint32_t frame_count;
    char error_msg[80];
    bool running;

    DISPENV disp;
    DRAWENV draw;
} app_t;

extern app_t g_app;

void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);

int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_draw_text(int x, int y, const char *text, uint8_t r, uint8_t g, uint8_t b);
void ui_draw_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void ui_draw_header(const char *title);
void ui_draw_menu(const char **options, int count, int selected);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_media_list(const media_list_t *list);
void ui_draw_playback(const playback_t *pb);

int input_init(void);
void input_update(void);
bool input_pressed(button_mask_t button);
bool input_held(button_mask_t button);

int audio_init(void);
void audio_shutdown(void);
int audio_play_stream(const char *url);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
void audio_set_volume(int vol);
bool audio_is_playing(void);
uint32_t audio_get_position(void);
uint32_t audio_get_duration(void);

int network_init(void);
void network_shutdown(void);
int http_get(const char *url, char **response, size_t *len);

int api_init(const char *server);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, char *url, size_t len);

int config_load(user_settings_t *s);
int config_save(const user_settings_t *s);
void config_defaults(user_settings_t *s);

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#endif
