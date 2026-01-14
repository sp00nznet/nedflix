/*
 * Nedflix for Nintendo Wii
 * Uses devkitPPC + libogc
 *
 * Hardware specs (2006):
 *   - CPU: 729 MHz IBM Broadway (PowerPC 750CL)
 *   - RAM: 24 MB MEM1 + 64 MB MEM2 = 88 MB total
 *   - GPU: 243 MHz ATI Hollywood
 *   - Audio: DSP, same as GameCube
 *   - Network: Built-in WiFi + USB Ethernet
 *   - Storage: 512 MB internal + SD card
 *
 * The Wii is VERY capable for a media client!
 * Full network, plenty of RAM, good CPU.
 */

#ifndef NEDFLIX_WII_H
#define NEDFLIX_WII_H

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogcsys.h>
#include <network.h>
#include <fat.h>
#include <stdbool.h>
#include <stdint.h>

#define NEDFLIX_VERSION "1.0.0-wii"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

#define MAX_PATH_LENGTH   512
#define MAX_URL_LENGTH    512
#define MAX_TITLE_LENGTH  256
#define MAX_ITEMS_VISIBLE 12
#define MAX_MEDIA_ITEMS   200

#define HTTP_TIMEOUT_MS    20000
#define RECV_BUFFER_SIZE   16384
#define STREAM_BUFFER_SIZE (2 * 1024 * 1024)  /* 2MB - Wii has plenty! */

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

/* Wii Remote + Classic Controller buttons */
typedef enum {
    BTN_A           = (1 << 0),
    BTN_B           = (1 << 1),
    BTN_1           = (1 << 2),
    BTN_2           = (1 << 3),
    BTN_PLUS        = (1 << 4),
    BTN_MINUS       = (1 << 5),
    BTN_HOME        = (1 << 6),
    BTN_UP          = (1 << 7),
    BTN_DOWN        = (1 << 8),
    BTN_LEFT        = (1 << 9),
    BTN_RIGHT       = (1 << 10),
    BTN_CLASSIC_X   = (1 << 11),
    BTN_CLASSIC_Y   = (1 << 12),
    BTN_CLASSIC_ZL  = (1 << 13),
    BTN_CLASSIC_ZR  = (1 << 14)
} button_mask_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    media_type_t type;
    bool is_directory;
    uint32_t duration;
    uint64_t size;
    char thumbnail_url[MAX_URL_LENGTH];
} media_item_t;

typedef struct {
    media_item_t *items;  /* Dynamic allocation - Wii has RAM */
    int count;
    int capacity;
    int selected_index;
    int scroll_offset;
    char current_path[MAX_PATH_LENGTH];
} media_list_t;

typedef struct {
    char server_url[MAX_URL_LENGTH];
    char username[64];
    char password[64];  /* Can store with Wii's storage */
    char session_token[128];
    uint8_t volume;
    uint8_t library;
    bool autoplay;
    bool show_subtitles;
    uint8_t video_quality;
    bool use_wifi;
    bool use_widescreen;
    char subtitle_language[8];
    char audio_language[8];
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
    int bitrate;
} playback_t;

typedef struct {
    bool initialized;
    bool connected;
    bool is_wifi;
    uint32_t ip_addr;
    int socket;
    char local_ip[16];
    char gateway[16];
    char netmask[16];
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

    /* Wii Remote pointer */
    int pointer_x;
    int pointer_y;
    bool pointer_valid;

    /* Nunchuk */
    int8_t nunchuk_x;
    int8_t nunchuk_y;

    uint32_t frame_count;
    char error_msg[256];
    char status_msg[128];
    bool running;

    void *xfb[2];
    int fb_index;
    GXRModeObj *rmode;
} app_t;

extern app_t g_app;

void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);
void app_set_status(const char *msg);

int network_init(void);
void network_shutdown(void);
bool network_is_available(void);
int http_get(const char *url, char **response, size_t *len);
int http_get_with_auth(const char *url, const char *token, char **response, size_t *len);
int http_post(const char *url, const char *body, char **response, size_t *len);
int http_download_file(const char *url, const char *path);

int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_draw_text(int x, int y, const char *text, u32 color);
void ui_draw_text_centered(int y, const char *text, u32 color);
void ui_draw_rect(int x, int y, int w, int h, u32 color);
void ui_draw_header(const char *title);
void ui_draw_menu(const char **options, int count, int selected);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_media_list(const media_list_t *list);
void ui_draw_playback(const playback_t *pb);
void ui_draw_pointer(int x, int y);
void ui_draw_keyboard(const char *current, int cursor);

int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_pressed(button_mask_t button);
bool input_held(button_mask_t button);

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
int audio_get_buffer_level(void);

int video_init(void);
void video_shutdown(void);
int video_play_stream(const char *url);
void video_stop(void);
void video_pause(void);
void video_resume(void);
void video_seek(int offset_ms);
bool video_is_playing(void);
void video_render_frame(void);

int api_init(const char *server);
void api_shutdown(void);
int api_login(const char *user, const char *pass, char *token, size_t len);
int api_logout(const char *token);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_search(const char *token, const char *query, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, char *url, size_t len);
int api_get_resume_position(const char *token, const char *path, uint32_t *pos);
int api_set_resume_position(const char *token, const char *path, uint32_t pos);

int config_load(user_settings_t *s);
int config_save(const user_settings_t *s);
void config_defaults(user_settings_t *s);

typedef struct json_value json_value_t;
json_value_t *json_parse(const char *text);
void json_free(json_value_t *v);
const char *json_get_string(json_value_t *obj, const char *key);
int json_get_int(json_value_t *obj, const char *key, int def);
double json_get_double(json_value_t *obj, const char *key, double def);
bool json_get_bool(json_value_t *obj, const char *key, bool def);
json_value_t *json_get_object(json_value_t *obj, const char *key);
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
#define COLOR_GREEN     0x00FF00FF
#define COLOR_BLUE      0x0066FFFF

#endif
