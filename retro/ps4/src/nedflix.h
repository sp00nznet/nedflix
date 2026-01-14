/*
 * Nedflix for PlayStation 4
 *
 * Hardware specs (2013):
 *   - CPU: 1.6 GHz AMD Jaguar (8 cores)
 *   - RAM: 8 GB GDDR5
 *   - GPU: 800 MHz AMD Radeon (1.84 TFLOPS)
 *   - Network: Gigabit Ethernet + WiFi
 *   - Storage: 500 GB - 2 TB HDD/SSD
 *   - Output: 1080p/4K video, 7.1 audio
 *
 * PS4 is a FULL MODERN CONSOLE - completely capable.
 *
 * SDK REQUIREMENT:
 *   PlayStation 4 development REQUIRES official Sony SDK.
 *   There is NO public homebrew SDK for PS4.
 *
 *   To develop for PS4:
 *   1. Register at partners.playstation.net
 *   2. Apply for PlayStation Partners Program
 *   3. Sign NDA and agreements
 *   4. Receive development kit hardware
 *   5. Access official SDK (Orbis)
 *
 * THIS IS A STUB HEADER showing the intended API.
 * Actual implementation requires Sony Orbis SDK.
 */

#ifndef NEDFLIX_PS4_H
#define NEDFLIX_PS4_H

#include <stdbool.h>
#include <stdint.h>

/* Would include Orbis SDK headers here */
/* #include <orbis/libScePad.h> */
/* #include <orbis/libSceNet.h> */
/* etc. */

#define NEDFLIX_VERSION "1.0.0-ps4"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

#define MAX_PATH_LENGTH   1024
#define MAX_URL_LENGTH    1024
#define MAX_TITLE_LENGTH  512
#define MAX_ITEMS_VISIBLE 20
#define MAX_MEDIA_ITEMS   5000

#define HTTP_TIMEOUT_MS    60000
#define STREAM_BUFFER_SIZE (64 * 1024 * 1024)  /* 64MB */

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

/* DualShock 4 buttons */
typedef enum {
    BTN_CROSS     = (1 << 0),
    BTN_CIRCLE    = (1 << 1),
    BTN_SQUARE    = (1 << 2),
    BTN_TRIANGLE  = (1 << 3),
    BTN_OPTIONS   = (1 << 4),
    BTN_SHARE     = (1 << 5),
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
    BTN_TOUCHPAD  = (1 << 16),
    BTN_PS        = (1 << 17)
} button_mask_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    char description[2048];
    char thumbnail_url[MAX_URL_LENGTH];
    media_type_t type;
    bool is_directory;
    uint32_t duration;
    uint64_t size;
    int year;
    float rating;
    char genres[512];
    char cast[1024];
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
    char session_token[512];
    uint8_t volume;
    uint8_t library;
    bool autoplay;
    bool show_subtitles;
    uint8_t video_quality;
    char subtitle_language[8];
    char audio_language[8];
    bool enable_hdr;
    bool enable_surround;
} user_settings_t;

typedef struct {
    char title[MAX_TITLE_LENGTH];
    char url[MAX_URL_LENGTH];
    bool playing;
    bool paused;
    bool is_audio;
    uint64_t position_ms;
    uint64_t duration_ms;
    uint8_t volume;
    int buffered_percent;
    int bitrate_kbps;
    int width;
    int height;
    char codec[64];
    bool is_hdr;
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
    uint8_t l2_pressure;
    uint8_t r2_pressure;

    /* Touchpad */
    bool touch_valid;
    int touch_x;
    int touch_y;

    uint32_t frame_count;
    char error_msg[512];
    bool running;
} app_t;

extern app_t g_app;

/*
 * IMPLEMENTATION NOTE:
 *
 * All function prototypes below would be implemented using
 * Sony's Orbis SDK. Without the official SDK, this cannot
 * be built or tested.
 *
 * The Orbis SDK provides:
 *   - libScePad: Controller input
 *   - libSceNet: Network stack
 *   - libSceVideoOut: Video output
 *   - libSceAudioOut: Audio output
 *   - libSceAvPlayer: Media playback
 *   - libSceHttp: HTTP client
 *   - libSceJson: JSON parsing
 *   - libSceSaveData: Save data management
 *
 * PS4 has full hardware video decode (H.264, H.265, VP9)
 * and would be an excellent media streaming platform.
 */

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
bool audio_is_playing(void);

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
int api_get_stream_url(const char *token, const char *path, int quality, char *url, size_t len);

int config_load(user_settings_t *s);
int config_save(const user_settings_t *s);
void config_defaults(user_settings_t *s);

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#endif
