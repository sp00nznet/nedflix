/*
 * Nedflix for Sega Dreamcast
 * Main header file
 *
 * Hardware specs (1998):
 *   - CPU: 200 MHz Hitachi SH-4
 *   - RAM: 16 MB main memory (CRITICAL CONSTRAINT!)
 *   - VRAM: 8 MB for PowerVR2
 *   - GPU: NEC PowerVR2 CLX2 (tile-based deferred rendering)
 *   - Sound: Yamaha AICA (ARM7 + 64 channels + 2MB sound RAM)
 *   - Network: Broadband Adapter (10Mbps) or 56K Modem
 *   - Storage: VMU (128KB), GD-ROM/CD-R
 *
 * Memory budget (~14MB usable after OS):
 *   - Application code: ~500KB
 *   - Network buffers: ~256KB
 *   - Audio buffer: ~1MB (streaming)
 *   - UI/Textures: ~1MB
 *   - Media list: ~256KB
 *   - Stack/heap: ~1MB
 *   - Free for streaming: ~10MB
 *
 * This port focuses on AUDIO STREAMING as the primary use case.
 * Video playback is extremely limited due to CPU/RAM constraints.
 */

#ifndef NEDFLIX_DC_H
#define NEDFLIX_DC_H

#include <kos.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Version */
#define NEDFLIX_VERSION "1.0.0-dc"

/* Build mode */
#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

/* Screen dimensions (640x480 VGA or 320x240 composite) */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* Memory constraints - be VERY conservative! */
#define MAX_PATH_LENGTH     256
#define MAX_URL_LENGTH      384
#define MAX_TITLE_LENGTH    128
#define MAX_ITEMS_VISIBLE   8
#define MAX_MEDIA_ITEMS     50   /* Limited list size to save RAM */

/* Network settings */
#define HTTP_TIMEOUT_MS     10000
#define RECV_BUFFER_SIZE    4096
#define STREAM_BUFFER_SIZE  (256 * 1024)  /* 256KB audio buffer */

/* Colors (PVR format: ARGB4444 or ARGB1555) */
#define COLOR_BLACK       0xFF000000
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_RED         0xFFE50914
#define COLOR_DARK_BG     0xFF0A0A0A
#define COLOR_MENU_BG     0xFF1A1A1A
#define COLOR_SELECTED    0xFF333333
#define COLOR_TEXT        0xFFCCCCCC
#define COLOR_TEXT_DIM    0xFF666666

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
    MEDIA_UNKNOWN,
    MEDIA_DIRECTORY,
    MEDIA_AUDIO,
    MEDIA_VIDEO
} media_type_t;

/* Library types */
typedef enum {
    LIBRARY_MUSIC,      /* Primary - works best on DC */
    LIBRARY_AUDIOBOOKS, /* Good for DC */
    LIBRARY_MOVIES,     /* Limited support */
    LIBRARY_TVSHOWS,    /* Limited support */
    LIBRARY_COUNT
} library_t;

/* Dreamcast controller buttons */
typedef enum {
    DC_BTN_C       = CONT_C,
    DC_BTN_B       = CONT_B,
    DC_BTN_A       = CONT_A,
    DC_BTN_START   = CONT_START,
    DC_BTN_UP      = CONT_DPAD_UP,
    DC_BTN_DOWN    = CONT_DPAD_DOWN,
    DC_BTN_LEFT    = CONT_DPAD_LEFT,
    DC_BTN_RIGHT   = CONT_DPAD_RIGHT,
    DC_BTN_Z       = CONT_Z,
    DC_BTN_Y       = CONT_Y,
    DC_BTN_X       = CONT_X,
    DC_BTN_D       = CONT_D,
    DC_TRIG_L      = 0x10000,  /* Left trigger (analog) */
    DC_TRIG_R      = 0x20000   /* Right trigger (analog) */
} dc_button_t;

/* Media item (compact for memory) */
typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    uint8_t type;       /* media_type_t */
    uint8_t flags;      /* is_directory, etc */
    uint16_t duration;  /* seconds, for audio */
} media_item_t;

/* Media list */
typedef struct {
    media_item_t items[MAX_MEDIA_ITEMS];
    int16_t count;
    int16_t selected;
    int16_t scroll;
    char path[MAX_PATH_LENGTH];
} media_list_t;

/* User settings (fits in VMU) */
typedef struct {
    char server_url[MAX_URL_LENGTH];
    char username[32];
    char session_token[64];
    uint8_t volume;          /* 0-100 */
    uint8_t library;         /* Current library */
    uint8_t theme;           /* 0=dark */
    uint8_t flags;           /* autoplay, shuffle, repeat */
} settings_t;

/* Playback state */
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

/* Network state */
typedef struct {
    bool initialized;
    bool connected;
    uint32_t ip_addr;
    int socket;
} network_t;

/* Global application context */
typedef struct {
    app_state_t state;
    settings_t settings;
    playback_t playback;
    media_list_t media;
    network_t net;
    library_t current_library;

    /* Input state */
    uint32_t buttons;
    uint32_t buttons_pressed;
    int8_t analog_x;
    int8_t analog_y;
    uint8_t ltrig;
    uint8_t rtrig;

    /* Timing */
    uint32_t frame_count;
    uint32_t last_input_time;

    /* Error handling */
    char error_msg[128];

    /* Running flag */
    bool running;
} app_t;

/* Global instance */
extern app_t g_app;

/*
 * Module functions
 */

/* main.c */
void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);

/* network.c */
int net_init(void);
void net_shutdown(void);
int net_http_get(const char *url, const char *auth, char **response, size_t *len);
int net_http_post(const char *url, const char *body, char **response, size_t *len);

/* ui.c */
void ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_draw_background(void);
void ui_draw_header(const char *title);
void ui_draw_text(float x, float y, const char *text, uint32_t color);
void ui_draw_text_centered(float y, const char *text, uint32_t color);
void ui_draw_menu(const char **items, int count, int selected);
void ui_draw_media_list(media_list_t *list);
void ui_draw_loading(const char *msg);
void ui_draw_error(const char *msg);
void ui_draw_playback(playback_t *pb);
void ui_draw_progress(float x, float y, float w, float h, float progress, uint32_t color);

/* input.c */
void input_init(void);
void input_update(void);
bool input_pressed(dc_button_t btn);
bool input_held(dc_button_t btn);
int input_analog_x(void);
int input_analog_y(void);

/* audio.c */
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

/* api.c */
int api_init(const char *server);
int api_login(const char *user, const char *pass, char *token, size_t len);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, char *url, size_t len);

/* config.c */
int config_load(settings_t *s);
int config_save(const settings_t *s);
void config_defaults(settings_t *s);

/* json.c - minimal parser */
typedef struct json_value json_t;
json_t *json_parse(const char *text);
void json_free(json_t *v);
const char *json_string(json_t *obj, const char *key);
int json_int(json_t *obj, const char *key, int def);
bool json_bool(json_t *obj, const char *key, bool def);
json_t *json_array(json_t *obj, const char *key);
int json_array_len(json_t *arr);
json_t *json_array_get(json_t *arr, int i);

/* Utility macros */
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(x,lo,hi) MIN(MAX(x,lo),hi)

/* Debug output */
#ifdef DEBUG
#define DBG(fmt, ...) dbglog(DBG_INFO, "[NEDFLIX] " fmt "\n", ##__VA_ARGS__)
#define ERR(fmt, ...) dbglog(DBG_ERROR, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#define ERR(fmt, ...)
#endif

#endif /* NEDFLIX_DC_H */
