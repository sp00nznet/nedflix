/*
 * Nedflix for Nintendo Switch
 * Uses libnx (devkitPro)
 *
 * Hardware specs (2017):
 *   - CPU: 1.02 GHz ARM Cortex-A57 (quad-core) + A53 (quad-core)
 *   - RAM: 4 GB
 *   - GPU: 768 MHz NVIDIA Tegra X1
 *   - Network: WiFi + Ethernet (dock)
 *   - Storage: 32 GB internal + microSD
 *   - Display: 1920x1080 (docked), 1280x720 (handheld)
 *
 * Switch is EXTREMELY capable:
 *   - Modern ARM64 CPU
 *   - Hardware video decode
 *   - Full network stack
 *   - HD audio/video output
 *
 * SDK NOTE:
 *   Official Nintendo SDK requires developer license ($500+).
 *   libnx is homebrew SDK - requires CFW (Custom Firmware).
 */

#ifndef NEDFLIX_SWITCH_H
#define NEDFLIX_SWITCH_H

#include <switch.h>
#include <stdbool.h>
#include <stdint.h>

#define NEDFLIX_VERSION "1.0.0-switch"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

#define MAX_PATH_LENGTH   512
#define MAX_URL_LENGTH    512
#define MAX_TITLE_LENGTH  256
#define MAX_ITEMS_VISIBLE 15
#define MAX_MEDIA_ITEMS   1000

#define HTTP_TIMEOUT_MS    30000
#define RECV_BUFFER_SIZE   131072
#define STREAM_BUFFER_SIZE (32 * 1024 * 1024)  /* 32MB */

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

/* Joy-Con / Pro Controller buttons */
typedef enum {
    BTN_A           = KEY_A,
    BTN_B           = KEY_B,
    BTN_X           = KEY_X,
    BTN_Y           = KEY_Y,
    BTN_PLUS        = KEY_PLUS,
    BTN_MINUS       = KEY_MINUS,
    BTN_UP          = KEY_DUP,
    BTN_DOWN        = KEY_DDOWN,
    BTN_LEFT        = KEY_DLEFT,
    BTN_RIGHT       = KEY_DRIGHT,
    BTN_L           = KEY_L,
    BTN_R           = KEY_R,
    BTN_ZL          = KEY_ZL,
    BTN_ZR          = KEY_ZR,
    BTN_LSTICK      = KEY_LSTICK,
    BTN_RSTICK      = KEY_RSTICK
} button_mask_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    char description[1024];
    char thumbnail_url[MAX_URL_LENGTH];
    media_type_t type;
    bool is_directory;
    uint32_t duration;
    uint64_t size;
    int year;
    float rating;
    char genres[256];
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
    char session_token[256];
    uint8_t volume;
    uint8_t library;
    bool autoplay;
    bool show_subtitles;
    uint8_t video_quality;  /* 0=SD, 1=720p, 2=1080p, 3=4K */
    char subtitle_language[8];
    char audio_language[8];
    bool enable_hdr;
    bool enable_surround;
    bool handheld_low_quality;
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
    char codec[32];
    bool is_docked;
} playback_t;

typedef struct {
    bool initialized;
    bool connected;
    uint32_t ip_addr;
    char local_ip[16];
    int signal_strength;
} network_state_t;

typedef struct {
    app_state_t state;
    user_settings_t settings;
    playback_t playback;
    media_list_t media;
    network_state_t net;
    library_t current_library;

    u64 buttons_pressed;
    u64 buttons_just_pressed;
    s32 lstick_x;
    s32 lstick_y;
    s32 rstick_x;
    s32 rstick_y;

    /* Touch input */
    bool touch_valid;
    s32 touch_x;
    s32 touch_y;

    uint32_t frame_count;
    char error_msg[256];
    char status_msg[128];
    bool running;
    bool is_docked;

    Framebuffer fb;
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
void ui_draw_text(int x, int y, const char *text, uint32_t color);
void ui_draw_rect(int x, int y, int w, int h, uint32_t color);
void ui_draw_image(int x, int y, int w, int h, const void *data);
void ui_draw_header(const char *title);
void ui_draw_menu(const char **options, int count, int selected);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_media_list(const media_list_t *list);
void ui_draw_playback(const playback_t *pb);
void ui_show_keyboard(const char *title, char *output, size_t max_len);

int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_pressed(u64 key);
bool input_held(u64 key);

int audio_init(void);
void audio_shutdown(void);
int audio_play_stream(const char *url);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
void audio_seek(int offset_ms);
void audio_set_volume(int vol);
void audio_update(void);
bool audio_is_playing(void);
uint32_t audio_get_position(void);
uint32_t audio_get_duration(void);

int video_init(void);
void video_shutdown(void);
int video_play_stream(const char *url);
void video_stop(void);
void video_pause(void);
void video_resume(void);
void video_seek(int offset_ms);
bool video_is_playing(void);
void video_render_frame(void);
void video_set_quality(int quality);

int api_init(const char *server);
void api_shutdown(void);
int api_login(const char *user, const char *pass, char *token, size_t len);
int api_logout(const char *token);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_search(const char *token, const char *query, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, int quality, char *url, size_t len);
int api_get_resume_position(const char *token, const char *path, uint32_t *pos);

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

#define COLOR_BLACK     0x000000FF
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_RED       0xE50914FF
#define COLOR_DARK_BG   0x0A0A0AFF
#define COLOR_MENU_BG   0x1A1A1AFF
#define COLOR_SELECTED  0x333333FF
#define COLOR_TEXT      0xCCCCCCFF
#define COLOR_TEXT_DIM  0x666666FF

/*
 * NOTE ON HOMEBREW:
 *
 * This uses libnx, the homebrew SDK.
 * To run on actual Switch hardware:
 *   1. Switch must have Custom Firmware (CFW) installed
 *   2. Build produces .nro file for Homebrew Menu
 *   3. Cannot access official Nintendo services
 *   4. Cannot be published on eShop
 *
 * For official Switch release:
 *   - Register as Nintendo developer
 *   - Apply for Nintendo Switch dev kit
 *   - Sign NDA and developer agreements
 *   - Use official Nintendo SDK
 *   - Submit for Nintendo certification
 */

#endif
