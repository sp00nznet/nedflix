/*
 * Nedflix for Nintendo 64
 * Uses libdragon SDK
 *
 * Hardware specs (1996):
 *   - CPU: 93.75 MHz NEC VR4300 (MIPS R4300i)
 *   - RAM: 4 MB (8 MB with Expansion Pak)
 *   - GPU: 62.5 MHz Reality Co-Processor (RCP)
 *   - Audio: 16-bit stereo, 48kHz max
 *   - Storage: Cartridge (4-64 MB), Controller Pak (32KB)
 */

#ifndef NEDFLIX_N64_H
#define NEDFLIX_N64_H

#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>

#define NEDFLIX_VERSION "1.0.0-n64"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

#define MAX_PATH_LENGTH   256
#define MAX_URL_LENGTH    384
#define MAX_TITLE_LENGTH  64
#define MAX_ITEMS_VISIBLE 8
#define MAX_MEDIA_ITEMS   32

#define HTTP_TIMEOUT_MS    10000
#define RECV_BUFFER_SIZE   2048
#define STREAM_BUFFER_SIZE (64 * 1024)

#define COLOR_BLACK     graphics_make_color(0, 0, 0, 255)
#define COLOR_WHITE     graphics_make_color(255, 255, 255, 255)
#define COLOR_RED       graphics_make_color(229, 9, 20, 255)
#define COLOR_DARK_BG   graphics_make_color(10, 10, 10, 255)
#define COLOR_MENU_BG   graphics_make_color(26, 26, 26, 255)
#define COLOR_SELECTED  graphics_make_color(51, 51, 51, 255)
#define COLOR_TEXT      graphics_make_color(204, 204, 204, 255)
#define COLOR_TEXT_DIM  graphics_make_color(102, 102, 102, 255)

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
    BTN_A           = (1 << 0),
    BTN_B           = (1 << 1),
    BTN_Z           = (1 << 2),
    BTN_START       = (1 << 3),
    BTN_DPAD_UP     = (1 << 4),
    BTN_DPAD_DOWN   = (1 << 5),
    BTN_DPAD_LEFT   = (1 << 6),
    BTN_DPAD_RIGHT  = (1 << 7),
    BTN_L           = (1 << 8),
    BTN_R           = (1 << 9),
    BTN_C_UP        = (1 << 10),
    BTN_C_DOWN      = (1 << 11),
    BTN_C_LEFT      = (1 << 12),
    BTN_C_RIGHT     = (1 << 13)
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
    char username[32];
    char session_token[64];
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
    bool initialized;
    bool connected;
    uint32_t ip_addr;
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
    int8_t stick_x;
    int8_t stick_y;

    uint32_t frame_count;
    char error_msg[128];
    bool running;

    display_context_t disp;
} app_t;

extern app_t g_app;

#ifdef DEBUG
#define LOG(fmt, ...) debugf("[NEDFLIX] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) debugf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#define LOG_ERROR(fmt, ...)
#endif

void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);

int network_init(void);
void network_shutdown(void);
bool network_is_available(void);
int http_get(const char *url, char **response, size_t *len);
int http_post(const char *url, const char *body, char **response, size_t *len);

int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_draw_background(void);
void ui_draw_text(int x, int y, const char *text, uint32_t color);
void ui_draw_text_centered(int y, const char *text, uint32_t color);
void ui_draw_header(const char *title);
void ui_draw_menu(const char **options, int count, int selected);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_media_list(const media_list_t *list);
void ui_draw_playback(const playback_t *pb);

int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_pressed(button_mask_t button);
bool input_held(button_mask_t button);
int input_get_stick_x(void);
int input_get_stick_y(void);

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

int api_init(const char *server);
void api_shutdown(void);
int api_login(const char *user, const char *pass, char *token, size_t len);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, char *url, size_t len);

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
#define CLAMP(x,lo,hi) MIN(MAX(x,lo),hi)

#endif
