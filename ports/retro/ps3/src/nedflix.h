/*
 * Nedflix for PlayStation 3
 * Uses PSL1GHT (open source SDK)
 *
 * TECHNICAL DEMO / NOVELTY PORT
 * This port demonstrates homebrew development on the PS3 using the
 * open-source PSL1GHT SDK. Not intended for production use.
 *
 * Hardware specs (2006):
 *   - CPU: 3.2 GHz Cell BE (1 PPE + 6 SPEs)
 *   - RAM: 256 MB XDR + 256 MB GDDR3
 *   - GPU: 550 MHz RSX (NVIDIA)
 *   - Network: Gigabit Ethernet + WiFi
 *   - Storage: HDD (20-500 GB)
 *
 * PS3 is EXTREMELY capable:
 *   - Cell SPEs perfect for audio/video decode
 *   - Full network stack
 *   - HD video output
 *   - Large storage
 *
 * Features in this demo:
 *   - RSX-based UI rendering (720p/1080p)
 *   - DualShock 3 controller input
 *   - BSD sockets networking
 *   - Audio streaming via Cell
 *   - Video playback (SPE-assisted decode)
 *
 * Build requirements:
 *   - PSL1GHT SDK (https://github.com/ps3dev/PSL1GHT)
 *   - ps3toolchain
 */

#ifndef NEDFLIX_PS3_H
#define NEDFLIX_PS3_H

#include <psl1ght/lv2.h>
#include <io/pad.h>
#include <sysutil/sysutil.h>
#include <net/net.h>
#include <stdbool.h>
#include <stdint.h>

#define NEDFLIX_VERSION "1.0.0-ps3"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

#define MAX_PATH_LENGTH   512
#define MAX_URL_LENGTH    512
#define MAX_TITLE_LENGTH  256
#define MAX_ITEMS_VISIBLE 15
#define MAX_MEDIA_ITEMS   500

#define HTTP_TIMEOUT_MS    30000
#define RECV_BUFFER_SIZE   65536
#define STREAM_BUFFER_SIZE (8 * 1024 * 1024)  /* 8MB - PS3 has plenty */

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

/* DualShock 3 buttons */
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
    BTN_R2        = (1 << 13),
    BTN_L3        = (1 << 14),
    BTN_R3        = (1 << 15),
    BTN_PS        = (1 << 16)
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
    uint8_t video_quality;  /* 0=SD, 1=HD, 2=Full HD */
    char subtitle_language[8];
    char audio_language[8];
    bool enable_surround;
    bool enable_hdr;
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
} playback_t;

typedef struct {
    bool initialized;
    bool connected;
    uint32_t ip_addr;
    char local_ip[16];
    int download_speed;
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
    uint8_t l2_pressure;
    uint8_t r2_pressure;

    uint32_t frame_count;
    char error_msg[256];
    char status_msg[128];
    bool running;

    /* PS3 specific */
    void *gcm_context;
    uint32_t *video_buffer;
} app_t;

extern app_t g_app;

void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);
void app_set_status(const char *msg);

int network_init(void);
void network_shutdown(void);
int http_get(const char *url, char **response, size_t *len);
int http_post(const char *url, const char *body, char **response, size_t *len);
int http_download_async(const char *url, const char *path, void (*callback)(int));

int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_draw_text(int x, int y, const char *text, uint32_t color);
void ui_draw_text_centered(int y, const char *text, uint32_t color);
void ui_draw_rect(int x, int y, int w, int h, uint32_t color);
void ui_draw_image(int x, int y, int w, int h, void *data);
void ui_draw_header(const char *title);
void ui_draw_menu(const char **options, int count, int selected);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_media_list(const media_list_t *list);
void ui_draw_media_detail(const media_item_t *item);
void ui_draw_playback(const playback_t *pb);
void ui_draw_osk(const char *title, char *output, int max_len);

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

int video_init(void);
void video_shutdown(void);
int video_play_stream(const char *url);
void video_stop(void);
void video_pause(void);
void video_resume(void);
void video_seek(int offset_ms);
bool video_is_playing(void);
void video_render_frame(void);
int video_get_width(void);
int video_get_height(void);

int api_init(const char *server);
void api_shutdown(void);
int api_login(const char *user, const char *pass, char *token, size_t len);
int api_logout(const char *token);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_search(const char *token, const char *query, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, int quality, char *url, size_t len);
int api_get_subtitles(const char *token, const char *path, const char *lang, char **srt);
int api_get_media_info(const char *token, const char *path, media_item_t *item);

int config_load(user_settings_t *s);
int config_save(const user_settings_t *s);
void config_defaults(user_settings_t *s);

/* SPE tasks for Cell processor */
int spe_decode_init(void);
void spe_decode_shutdown(void);
int spe_decode_audio(void *input, int in_size, void *output, int *out_size);
int spe_decode_video(void *input, int in_size, void *output, int *out_size);

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

#endif
