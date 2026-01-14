/*
 * Nedflix for Nintendo Wii U
 * Uses wut (Wii U Toolchain)
 *
 * Hardware specs (2012):
 *   - CPU: 1.24 GHz IBM Espresso (PowerPC tri-core)
 *   - RAM: 2 GB (1 GB for games)
 *   - GPU: 550 MHz AMD Radeon
 *   - Network: WiFi + Ethernet
 *   - Storage: 8/32 GB internal + USB
 *   - GamePad: 6.2" 854x480 touchscreen
 *
 * Wii U is VERY capable - basically a modern console:
 *   - Full network stack
 *   - HD video decode
 *   - Plenty of RAM
 *   - Second screen support
 *
 * SDK NOTE:
 *   Official SDK requires Nintendo developer license.
 *   wut is a homebrew SDK that provides similar capabilities.
 */

#ifndef NEDFLIX_WIIU_H
#define NEDFLIX_WIIU_H

#include <wut.h>
#include <coreinit/core.h>
#include <coreinit/thread.h>
#include <coreinit/screen.h>
#include <vpad/input.h>
#include <nn/ac.h>
#include <nsysnet/socket.h>
#include <stdbool.h>
#include <stdint.h>

#define NEDFLIX_VERSION "1.0.0-wiiu"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

/* TV screen */
#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

/* GamePad screen */
#define GAMEPAD_WIDTH  854
#define GAMEPAD_HEIGHT 480

#define MAX_PATH_LENGTH   512
#define MAX_URL_LENGTH    512
#define MAX_TITLE_LENGTH  256
#define MAX_ITEMS_VISIBLE 15
#define MAX_MEDIA_ITEMS   500

#define HTTP_TIMEOUT_MS    30000
#define RECV_BUFFER_SIZE   65536
#define STREAM_BUFFER_SIZE (16 * 1024 * 1024)  /* 16MB */

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

/* Wii U GamePad + Pro Controller buttons */
typedef enum {
    BTN_A           = (1 << 0),
    BTN_B           = (1 << 1),
    BTN_X           = (1 << 2),
    BTN_Y           = (1 << 3),
    BTN_PLUS        = (1 << 4),
    BTN_MINUS       = (1 << 5),
    BTN_HOME        = (1 << 6),
    BTN_UP          = (1 << 7),
    BTN_DOWN        = (1 << 8),
    BTN_LEFT        = (1 << 9),
    BTN_RIGHT       = (1 << 10),
    BTN_L           = (1 << 11),
    BTN_R           = (1 << 12),
    BTN_ZL          = (1 << 13),
    BTN_ZR          = (1 << 14),
    BTN_STICK_L     = (1 << 15),
    BTN_STICK_R     = (1 << 16)
} button_mask_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    char description[512];
    char thumbnail_url[MAX_URL_LENGTH];
    media_type_t type;
    bool is_directory;
    uint32_t duration;
    uint64_t size;
    int year;
    float rating;
} media_item_t;

typedef struct {
    media_item_t *items;
    int count;
    int capacity;
    int selected_index;
    int scroll_offset;
    char current_path[MAX_PATH_LENGTH];
} media_list_t;

typedef struct {
    char server_url[MAX_URL_LENGTH];
    char username[64];
    char password[64];
    char session_token[128];
    uint8_t volume;
    uint8_t library;
    bool autoplay;
    bool show_subtitles;
    uint8_t video_quality;  /* 0=SD, 1=720p, 2=1080p */
    char subtitle_language[8];
    char audio_language[8];
    bool use_gamepad_speaker;
    bool enable_off_tv_play;
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
    int buffered_percent;
    int bitrate_kbps;
    int width;
    int height;
} playback_t;

typedef struct {
    bool initialized;
    bool connected;
    uint32_t ip_addr;
    char local_ip[16];
} network_state_t;

typedef struct {
    app_state_t state;
    user_settings_t settings;
    playback_t playback;
    media_list_t media;
    network_state_t net;
    library_t current_library;

    uint32_t buttons_pressed;
    uint32_t buttons_just_pressed;
    int16_t lstick_x;
    int16_t lstick_y;
    int16_t rstick_x;
    int16_t rstick_y;

    /* GamePad touch */
    bool touch_valid;
    int touch_x;
    int touch_y;

    uint32_t frame_count;
    char error_msg[256];
    char status_msg[128];
    bool running;

    /* Dual screen buffers */
    void *tv_buffer;
    void *drc_buffer;
    uint32_t tv_buffer_size;
    uint32_t drc_buffer_size;
} app_t;

extern app_t g_app;

void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);

int network_init(void);
void network_shutdown(void);
int http_get(const char *url, char **response, size_t *len);
int http_post(const char *url, const char *body, char **response, size_t *len);

int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_draw_tv(void);
void ui_draw_gamepad(void);
void ui_draw_text_tv(int x, int y, const char *text, uint32_t color);
void ui_draw_text_drc(int x, int y, const char *text, uint32_t color);

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

int video_init(void);
void video_shutdown(void);
int video_play_stream(const char *url);
void video_stop(void);
void video_pause(void);
void video_resume(void);
bool video_is_playing(void);
void video_render_tv(void);
void video_render_gamepad(void);  /* Off-TV play */

int api_init(const char *server);
int api_login(const char *user, const char *pass, char *token, size_t len);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, int quality, char *url, size_t len);

int config_load(user_settings_t *s);
int config_save(const user_settings_t *s);
void config_defaults(user_settings_t *s);

typedef struct json_value json_value_t;
json_value_t *json_parse(const char *text);
void json_free(json_value_t *v);
const char *json_get_string(json_value_t *obj, const char *key);
int json_get_int(json_value_t *obj, const char *key, int def);
bool json_get_bool(json_value_t *obj, const char *key, bool def);
json_value_t *json_get_array(json_value_t *obj, const char *key);
int json_array_length(json_value_t *arr);
json_value_t *json_array_get(json_value_t *arr, int i);

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define COLOR_BLACK     0x000000FF
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_RED       0xE50914FF
#define COLOR_TEXT      0xCCCCCCFF
#define COLOR_TEXT_DIM  0x666666FF

#endif
