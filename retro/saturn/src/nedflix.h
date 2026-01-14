/*
 * Nedflix for Sega Saturn
 * Uses libyaul or Jo Engine
 *
 * Hardware specs (1994):
 *   - CPU: 2x 28.6 MHz Hitachi SH-2
 *   - RAM: 2 MB main + 1.5 MB VRAM
 *   - VDP1: Sprite/polygon engine
 *   - VDP2: Tilemap/background engine
 *   - SCSP: 32 channels, 512KB sound RAM
 *   - CD-ROM: 2x speed
 *   - Storage: Internal backup RAM (32KB) + Cartridge
 *
 * Features possible:
 *   - Audio streaming (ADPCM)
 *   - Dual CPU can help with decoding
 *   - UI via VDP2 tilemaps or VDP1 sprites
 *
 * Limitations:
 *   - Complex dual-CPU architecture
 *   - No native TCP/IP
 *   - Video limited to Cinepak/MPEG cart
 */

#ifndef NEDFLIX_SATURN_H
#define NEDFLIX_SATURN_H

#include <stdint.h>
#include <stdbool.h>

/* Platform-specific includes would go here */
/* #include <yaul.h> or Jo Engine headers */

#define NEDFLIX_VERSION "1.0.0-saturn"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 224

#define MAX_PATH_LENGTH   128
#define MAX_URL_LENGTH    192
#define MAX_TITLE_LENGTH  48
#define MAX_ITEMS_VISIBLE 8
#define MAX_MEDIA_ITEMS   24

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
    BTN_A       = (1 << 0),
    BTN_B       = (1 << 1),
    BTN_C       = (1 << 2),
    BTN_X       = (1 << 3),
    BTN_Y       = (1 << 4),
    BTN_Z       = (1 << 5),
    BTN_START   = (1 << 6),
    BTN_UP      = (1 << 7),
    BTN_DOWN    = (1 << 8),
    BTN_LEFT    = (1 << 9),
    BTN_RIGHT   = (1 << 10),
    BTN_L       = (1 << 11),
    BTN_R       = (1 << 12)
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
void ui_draw_text(int x, int y, const char *text, uint16_t color);
void ui_draw_rect(int x, int y, int w, int h, uint16_t color);
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

int api_init(const char *server);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, char *url, size_t len);

int config_load(user_settings_t *s);
int config_save(user_settings_t *s);
void config_defaults(user_settings_t *s);

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* Saturn color format: ABGR1555 */
#define RGB555(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))
#define COLOR_BLACK   RGB555(0,0,0)
#define COLOR_WHITE   RGB555(255,255,255)
#define COLOR_RED     RGB555(229,9,20)
#define COLOR_GRAY    RGB555(102,102,102)

#endif
