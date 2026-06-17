/*
 * Nedflix for Xbox 360
 * Uses libxenon (Free60 open source SDK)
 *
 * TECHNICAL DEMO / NOVELTY PORT
 * This port demonstrates homebrew development on the Xbox 360 using the
 * open-source libxenon SDK from the Free60 project. Requires JTAG/RGH
 * modified console. Not intended for production use.
 *
 * Hardware specs (2005):
 *   - CPU: 3.2 GHz PowerPC tri-core (Xenon) with VMX128
 *   - RAM: 512 MB GDDR3 (unified memory)
 *   - GPU: ATI Xenos 500 MHz (unified shaders)
 *   - Network: 100 Mbps Ethernet, WiFi (S/Elite models)
 *   - Storage: HDD (20-250 GB), USB
 *
 * Xbox 360 capabilities:
 *   - Powerful tri-core CPU (6 hardware threads)
 *   - Unified memory architecture (good for streaming)
 *   - Full network stack available
 *   - HD video output (720p/1080p)
 *   - USB mass storage support
 *
 * Limitations in homebrew:
 *   - No official hardware video decode access
 *   - Limited GPU documentation (Xenos is complex)
 *   - Requires modified console (JTAG/RGH)
 *   - libxenon is minimal compared to XDK
 *
 * Features in this demo:
 *   - Framebuffer-based UI rendering
 *   - Xbox 360 controller input
 *   - lwIP network stack
 *   - Audio via xenon sound hardware
 *   - USB storage for local media
 *
 * Build requirements:
 *   - libxenon toolchain (https://github.com/Free60Project/libxenon)
 *   - devkitPPC cross-compiler
 */

#ifndef NEDFLIX_X360_H
#define NEDFLIX_X360_H

#include <xenon_soc/xenon_power.h>
#include <xenon_smc/xenon_smc.h>
#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <usb/usbmain.h>
#include <input/input.h>
#include <ppc/timebase.h>
#include <network/network.h>
#include <httpd/httpd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <malloc.h>

/* Version */
#define NEDFLIX_VERSION "1.0.0-x360"

/* Build mode */
#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

/* Screen dimensions (720p default) */
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

/* Memory limits - 512MB unified but be conservative */
#define MAX_PATH_LENGTH     512
#define MAX_URL_LENGTH      512
#define MAX_TITLE_LENGTH    256
#define MAX_ITEMS_VISIBLE   15
#define MAX_MEDIA_ITEMS     200

/* Network settings */
#define HTTP_TIMEOUT_MS     15000
#define RECV_BUFFER_SIZE    32768
#define STREAM_BUFFER_SIZE  (4 * 1024 * 1024)  /* 4MB streaming buffer */

/* Colors (ARGB8888 for framebuffer) */
#define COLOR_BLACK       0xFF000000
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_RED         0xFFE50914  /* Netflix red */
#define COLOR_DARK_BG     0xFF0A0A0A
#define COLOR_MENU_BG     0xFF1A1A1A
#define COLOR_SELECTED    0xFF333333
#define COLOR_TEXT        0xFFCCCCCC
#define COLOR_TEXT_DIM    0xFF666666
#define COLOR_GREEN       0xFF00FF00

/* Application states */
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

/* Media types */
typedef enum {
    MEDIA_TYPE_UNKNOWN,
    MEDIA_TYPE_DIRECTORY,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_VIDEO
} media_type_t;

/* Library types */
typedef enum {
    LIBRARY_MUSIC,
    LIBRARY_AUDIOBOOKS,
    LIBRARY_MOVIES,
    LIBRARY_TVSHOWS,
    LIBRARY_COUNT
} library_t;

/* Xbox 360 controller buttons */
typedef enum {
    BTN_A             = (1 << 0),
    BTN_B             = (1 << 1),
    BTN_X             = (1 << 2),
    BTN_Y             = (1 << 3),
    BTN_START         = (1 << 4),
    BTN_BACK          = (1 << 5),
    BTN_LB            = (1 << 6),
    BTN_RB            = (1 << 7),
    BTN_DPAD_UP       = (1 << 8),
    BTN_DPAD_DOWN     = (1 << 9),
    BTN_DPAD_LEFT     = (1 << 10),
    BTN_DPAD_RIGHT    = (1 << 11),
    BTN_LEFT_THUMB    = (1 << 12),
    BTN_RIGHT_THUMB   = (1 << 13),
    BTN_GUIDE         = (1 << 14)
} button_mask_t;

/* Media item */
typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    media_type_t type;
    bool is_directory;
    uint32_t size;
    uint16_t duration;  /* seconds */
} media_item_t;

/* Media list */
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
    char auth_token[128];
    uint8_t volume;          /* 0-100 */
    uint8_t library;         /* Current library */
    bool autoplay;
    bool show_subtitles;
    char subtitle_lang[8];
    char audio_lang[8];
} user_settings_t;

/* Playback state */
typedef struct {
    char title[MAX_TITLE_LENGTH];
    char url[MAX_URL_LENGTH];
    bool playing;
    bool paused;
    bool is_audio;
    double position;
    double duration;
    uint8_t volume;
} playback_t;

/* Network state */
typedef struct {
    bool initialized;
    bool connected;
    uint32_t ip_addr;
    struct ip_addr ip;
    struct ip_addr netmask;
    struct ip_addr gateway;
} network_state_t;

/* Global application context */
typedef struct {
    app_state_t state;
    user_settings_t settings;
    playback_t playback;
    media_list_t media;
    network_state_t net;
    library_t current_library;

    /* Xenos graphics */
    struct XenosDevice *xe;
    struct XenosSurface *fb;
    uint32_t *framebuffer;

    /* Input state */
    uint32_t buttons_pressed;
    uint32_t buttons_just_pressed;
    uint32_t buttons_prev;
    int16_t left_stick_x;
    int16_t left_stick_y;
    int16_t right_stick_x;
    int16_t right_stick_y;
    uint8_t left_trigger;
    uint8_t right_trigger;

    /* Timing */
    uint64_t frame_count;
    uint64_t last_time;

    /* Error handling */
    char error_msg[256];

    /* Running flag */
    bool running;
} app_t;

/* Global instance */
extern app_t g_app;

/*
 * Debug/Logging macros
 */
#ifdef DEBUG
#define LOG(fmt, ...) printf("[NEDFLIX] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#define LOG_ERROR(fmt, ...)
#endif

/*
 * Module functions
 */

/* main.c */
void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);

/* ui.c */
int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_clear(uint32_t color);
void ui_draw_rect(int x, int y, int width, int height, uint32_t color);
void ui_draw_text(int x, int y, const char *text, uint32_t color);
void ui_draw_text_centered(int y, const char *text, uint32_t color);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_header(const char *title);
void ui_draw_menu(const char **items, int count, int selected);
void ui_draw_file_list(media_list_t *list);
void ui_draw_playback_hud(playback_t *state);
void ui_draw_progress_bar(int x, int y, int width, int height, float progress, uint32_t fg, uint32_t bg);

/* input.c */
int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_button_pressed(uint32_t button);
bool input_button_just_pressed(uint32_t button);
int16_t input_get_left_stick_x(void);
int16_t input_get_left_stick_y(void);
int16_t input_get_right_stick_x(void);
int16_t input_get_right_stick_y(void);
uint8_t input_get_left_trigger(void);
uint8_t input_get_right_trigger(void);

/* network.c */
int network_init(void);
void network_shutdown(void);
bool network_is_connected(void);
int http_get(const char *url, char **response, size_t *len);
int http_post(const char *url, const char *body, char **response, size_t *len);
int http_get_with_auth(const char *url, const char *token, char **response, size_t *len);

/* audio.c */
int audio_init(void);
void audio_shutdown(void);
int audio_play(const char *url);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
void audio_seek(double seconds);
void audio_set_volume(int volume);
void audio_update(void);
bool audio_is_playing(void);
double audio_get_position(void);
double audio_get_duration(void);

/* api.c */
int api_init(const char *server_url);
void api_shutdown(void);
int api_login(const char *username, const char *password, char *token_out, size_t token_len);
int api_get_user_info(const char *token, char *username_out, size_t len);
int api_browse(const char *token, const char *path, library_t library, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, char *url_out, size_t len);

/* config.c */
int config_load(user_settings_t *settings);
int config_save(const user_settings_t *settings);
void config_set_defaults(user_settings_t *settings);

/* json.c */
typedef struct json_value json_value_t;
json_value_t *json_parse(const char *text);
void json_free(json_value_t *v);
const char *json_get_string(json_value_t *obj, const char *key);
int json_get_int(json_value_t *obj, const char *key, int def);
bool json_get_bool(json_value_t *obj, const char *key, bool def);
json_value_t *json_get_array(json_value_t *obj, const char *key);
int json_array_length(json_value_t *arr);
json_value_t *json_array_get(json_value_t *arr, int idx);

/* Utility macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) MIN(MAX(x, lo), hi)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif /* NEDFLIX_X360_H */
