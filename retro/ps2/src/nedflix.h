/*
 * Nedflix for PlayStation 2
 * Uses PS2SDK (open source)
 *
 * Hardware specs (2000):
 *   - CPU: 294.912 MHz MIPS R5900 (Emotion Engine)
 *   - RAM: 32 MB main + 4 MB video
 *   - GPU: 147.456 MHz Graphics Synthesizer
 *   - Audio: SPU2, 48 channels, 2 MB
 *   - Network: Network Adapter (optional)
 *   - Storage: Memory Card (8 MB), HDD (optional)
 *
 * PS2 is very capable:
 *   - Good network support
 *   - Plenty of RAM
 *   - Strong CPU for decode
 *   - HDD allows caching
 */

#ifndef NEDFLIX_PS2_H
#define NEDFLIX_PS2_H

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <libmc.h>
#include <stdbool.h>
#include <stdint.h>

#define NEDFLIX_VERSION "1.0.0-ps2"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

#define MAX_PATH_LENGTH   256
#define MAX_URL_LENGTH    384
#define MAX_TITLE_LENGTH  128
#define MAX_ITEMS_VISIBLE 10
#define MAX_MEDIA_ITEMS   150

#define HTTP_TIMEOUT_MS    20000
#define RECV_BUFFER_SIZE   16384
#define STREAM_BUFFER_SIZE (1 * 1024 * 1024)  /* 1MB */

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
    BTN_R2        = (1 << 13),
    BTN_L3        = (1 << 14),
    BTN_R3        = (1 << 15)
} button_mask_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    media_type_t type;
    bool is_directory;
    uint32_t duration;
    uint64_t size;
} media_item_t;

typedef struct {
    media_item_t items[MAX_MEDIA_ITEMS];
    int count;
    int selected_index;
    int scroll_offset;
    char current_path[MAX_PATH_LENGTH];
} media_list_t;

typedef struct {
    char server_url[MAX_URL_LENGTH];
    char username[32];
    char password[32];
    char session_token[64];
    uint8_t volume;
    uint8_t library;
    bool autoplay;
    bool show_subtitles;
    uint8_t video_quality;
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
    int8_t lstick_x;
    int8_t lstick_y;
    int8_t rstick_x;
    int8_t rstick_y;

    uint32_t frame_count;
    char error_msg[128];
    bool running;
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
void ui_draw_text(int x, int y, const char *text, u32 color);
void ui_draw_text_centered(int y, const char *text, u32 color);
void ui_draw_rect(int x, int y, int w, int h, u32 color);
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

int audio_init(void);
void audio_shutdown(void);
int audio_play_stream(const char *url);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
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
bool video_is_playing(void);
void video_render_frame(void);

int api_init(const char *server);
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

#define COLOR_BLACK     GS_SETREG_RGBAQ(0,0,0,0x80,0)
#define COLOR_WHITE     GS_SETREG_RGBAQ(255,255,255,0x80,0)
#define COLOR_RED       GS_SETREG_RGBAQ(229,9,20,0x80,0)
#define COLOR_TEXT      GS_SETREG_RGBAQ(204,204,204,0x80,0)
#define COLOR_TEXT_DIM  GS_SETREG_RGBAQ(102,102,102,0x80,0)

#endif
