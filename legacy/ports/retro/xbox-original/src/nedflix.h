/*
 * Nedflix for Original Xbox
 * Main header file with common definitions
 *
 * This is a native C port of Nedflix for the Original Xbox (2001)
 * using the nxdk homebrew SDK.
 *
 * Hardware specs:
 *   - CPU: 733 MHz Intel Pentium III
 *   - RAM: 64 MB
 *   - GPU: nVidia NV2A (DirectX 8.1)
 *   - Storage: 8-10GB HDD (FATX filesystem)
 *   - Network: 10/100 Ethernet
 */

#ifndef NEDFLIX_H
#define NEDFLIX_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*
 * nxdk compatibility: snprintf is not available in nxdk's C library.
 * Provide a simple implementation using vsprintf.
 * Note: This is not buffer-overflow safe like real snprintf,
 * but works for our controlled use cases.
 */
#ifdef NXDK
static inline int nxdk_snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vsprintf(buf, fmt, args);
    va_end(args);
    /* Ensure null termination */
    if (size > 0) {
        buf[size - 1] = '\0';
    }
    return ret;
}
#define snprintf nxdk_snprintf
#endif

/* Version info */
#define NEDFLIX_VERSION_MAJOR 1
#define NEDFLIX_VERSION_MINOR 0
#define NEDFLIX_VERSION_PATCH 0
#define NEDFLIX_VERSION_STRING "1.0.0"

/* Build mode - defined by Makefile */
#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1  /* 1 = client (connect to server), 0 = desktop (local media) */
#endif

/* Screen dimensions (NTSC 480i/480p) */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* UI Constants */
#define MAX_PATH_LENGTH     260
#define MAX_URL_LENGTH      512
#define MAX_TITLE_LENGTH    256
#define MAX_ITEMS_PER_PAGE  10
#define MAX_MENU_ITEMS      20

/* Network timeouts (milliseconds) */
#define HTTP_CONNECT_TIMEOUT  5000
#define HTTP_READ_TIMEOUT     30000

/* Color definitions (ARGB format for DirectX) */
#define COLOR_BLACK       0xFF000000
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_RED         0xFFE50914  /* Netflix red */
#define COLOR_DARK_GRAY   0xFF141414
#define COLOR_LIGHT_GRAY  0xFF333333
#define COLOR_SELECTED    0xFF444444
#define COLOR_TEXT        0xFFE5E5E5
#define COLOR_TEXT_DIM    0xFF808080

/* Application states */
typedef enum {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_LOGIN,
    STATE_BROWSING,
    STATE_PLAYING,
    STATE_SETTINGS,
    STATE_ERROR
} app_state_t;

/* Library types */
typedef enum {
    LIBRARY_MOVIES,
    LIBRARY_TVSHOWS,
    LIBRARY_MUSIC,
    LIBRARY_AUDIOBOOKS,
    LIBRARY_COUNT
} library_type_t;

/* Media file types */
typedef enum {
    MEDIA_TYPE_UNKNOWN,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_DIRECTORY
} media_type_t;

/* Controller button masks */
typedef enum {
    BTN_A           = 0x0001,
    BTN_B           = 0x0002,
    BTN_X           = 0x0004,
    BTN_Y           = 0x0008,
    BTN_BLACK       = 0x0010,
    BTN_WHITE       = 0x0020,
    BTN_LEFT_TRIGGER  = 0x0040,
    BTN_RIGHT_TRIGGER = 0x0080,
    BTN_DPAD_UP     = 0x0100,
    BTN_DPAD_DOWN   = 0x0200,
    BTN_DPAD_LEFT   = 0x0400,
    BTN_DPAD_RIGHT  = 0x0800,
    BTN_START       = 0x1000,
    BTN_BACK        = 0x2000,
    BTN_LEFT_THUMB  = 0x4000,
    BTN_RIGHT_THUMB = 0x8000
} button_mask_t;

/* Media item structure */
typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    media_type_t type;
    uint64_t size;
    bool is_directory;
} media_item_t;

/* Playlist/directory listing */
typedef struct {
    media_item_t *items;
    int count;
    int capacity;
    int selected_index;
    int scroll_offset;
    char current_path[MAX_PATH_LENGTH];
} media_list_t;

/* User settings */
typedef struct {
    char server_url[MAX_URL_LENGTH];
    char username[64];
    char auth_token[256];
    int volume;                /* 0-100 */
    int playback_speed;        /* 100 = 1.0x, 150 = 1.5x, etc */
    bool autoplay;
    bool show_subtitles;
    char subtitle_language[8]; /* ISO 639-1 code */
    char audio_language[8];
    int theme;                 /* 0 = dark, 1 = light */
} user_settings_t;

/* Playback state */
typedef struct {
    char current_file[MAX_PATH_LENGTH];
    char title[MAX_TITLE_LENGTH];
    bool is_playing;
    bool is_paused;
    double current_time;       /* seconds */
    double duration;           /* seconds */
    int volume;
    bool has_subtitles;
    int current_audio_track;
    int audio_track_count;
} playback_state_t;

/* Global application context */
typedef struct {
    app_state_t state;
    user_settings_t settings;
    playback_state_t playback;
    media_list_t media_list;
    library_type_t current_library;
    char error_message[256];
    bool running;
    uint32_t last_input_time;
    uint16_t buttons_pressed;
    uint16_t buttons_just_pressed;
} app_context_t;

/* Global context (defined in main.c) */
extern app_context_t g_app;

/*
 * Function declarations - implemented in separate source files
 */

/* main.c */
void app_init(void);
void app_shutdown(void);
void app_run(void);

/* http_client.c */
int http_init(void);
void http_shutdown(void);
int http_get(const char *url, char **response, size_t *response_len);
int http_post(const char *url, const char *body, char **response, size_t *response_len);
int http_get_with_auth(const char *url, const char *token, char **response, size_t *response_len);

/* json.c */
typedef struct json_value json_value_t;
json_value_t *json_parse(const char *text);
void json_free(json_value_t *value);
const char *json_get_string(json_value_t *obj, const char *key);
int json_get_int(json_value_t *obj, const char *key, int default_val);
bool json_get_bool(json_value_t *obj, const char *key, bool default_val);
json_value_t *json_get_object(json_value_t *obj, const char *key);
json_value_t *json_get_array(json_value_t *obj, const char *key);
int json_array_length(json_value_t *array);
json_value_t *json_array_get(json_value_t *array, int index);

/* ui.c */
int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_clear(uint32_t color);
void ui_draw_rect(int x, int y, int width, int height, uint32_t color);
void ui_draw_text(int x, int y, const char *text, uint32_t color);
void ui_draw_text_centered(int y, const char *text, uint32_t color);
void ui_draw_header(const char *title);
void ui_draw_menu(const char **items, int count, int selected);
void ui_draw_file_list(media_list_t *list);
void ui_draw_progress_bar(int x, int y, int width, int height, float progress, uint32_t fg, uint32_t bg);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_playback_hud(playback_state_t *state);

/* On-screen keyboard */
typedef struct {
    char *buffer;           /* Output buffer */
    int buffer_size;        /* Max buffer size */
    int cursor_pos;         /* Cursor position in buffer */
    int keyboard_row;       /* Selected row on keyboard */
    int keyboard_col;       /* Selected column on keyboard */
    bool active;            /* Is OSK currently active */
    bool confirmed;         /* User pressed confirm */
    bool cancelled;         /* User pressed cancel */
    const char *title;      /* Title to display */
} osk_state_t;

void osk_init(osk_state_t *osk, const char *title, char *buffer, int buffer_size);
void osk_update(osk_state_t *osk);
void osk_draw(osk_state_t *osk);
bool osk_is_active(osk_state_t *osk);

/* input.c */
int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_button_pressed(button_mask_t button);
bool input_button_just_pressed(button_mask_t button);
int input_get_left_stick_x(void);
int input_get_left_stick_y(void);
int input_get_right_stick_x(void);
int input_get_right_stick_y(void);
int input_get_left_trigger(void);
int input_get_right_trigger(void);

/* video.c */
int video_init(void);
void video_shutdown(void);
int video_play(const char *url);
void video_stop(void);
void video_pause(void);
void video_resume(void);
void video_seek(double seconds);
void video_set_volume(int volume);
void video_update(void);
bool video_is_playing(void);
double video_get_position(void);
double video_get_duration(void);

/* config.c */
int config_load(user_settings_t *settings);
int config_save(const user_settings_t *settings);
void config_set_defaults(user_settings_t *settings);

/* api.c - Nedflix API client */
int api_init(const char *server_url);
void api_shutdown(void);
int api_login(const char *username, const char *password, char *token_out, size_t token_len);
int api_get_user_info(const char *token, char *username_out, size_t username_len);
int api_browse(const char *token, const char *path, library_type_t library, media_list_t *list);
int api_search(const char *token, const char *query, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, char *url_out, size_t url_len);
int api_get_audio_tracks(const char *token, const char *path, int *count);
int api_save_settings(const char *token, const user_settings_t *settings);

/* Utility macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) MIN(MAX(x, lo), hi)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Debug logging */
#ifdef DEBUG
#include <stdio.h>
#define LOG(fmt, ...) printf("[NEDFLIX] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#define LOG_ERROR(fmt, ...)
#endif

#endif /* NEDFLIX_H */
