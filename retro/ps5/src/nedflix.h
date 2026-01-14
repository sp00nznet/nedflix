/*
 * Nedflix for PlayStation 5
 *
 * Hardware specs (2020):
 *   - CPU: 3.5 GHz AMD Zen 2 (8 cores)
 *   - RAM: 16 GB GDDR6
 *   - GPU: 2.23 GHz AMD RDNA 2 (10.28 TFLOPS)
 *   - Storage: 825 GB NVMe SSD (5.5 GB/s)
 *   - Network: Gigabit Ethernet + WiFi 6
 *   - Output: 4K/8K video, 3D audio
 *
 * PS5 is CUTTING-EDGE - the most capable platform here.
 *
 * SDK REQUIREMENT:
 *   Like PS4, PS5 development REQUIRES official Sony SDK.
 *   There is NO public homebrew SDK.
 *
 *   To develop for PS5:
 *   1. Register at partners.playstation.net
 *   2. Apply for PlayStation Partners Program
 *   3. Sign NDA and agreements
 *   4. Receive PS5 development kit hardware
 *   5. Access official Prospero SDK
 *
 * THIS IS A STUB HEADER showing the intended API.
 * Actual implementation requires Sony Prospero SDK.
 */

#ifndef NEDFLIX_PS5_H
#define NEDFLIX_PS5_H

#include <stdbool.h>
#include <stdint.h>

/* Would include Prospero SDK headers here */

#define NEDFLIX_VERSION "1.0.0-ps5"

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

#define SCREEN_WIDTH_1080P  1920
#define SCREEN_HEIGHT_1080P 1080
#define SCREEN_WIDTH_4K     3840
#define SCREEN_HEIGHT_4K    2160

#define MAX_PATH_LENGTH   2048
#define MAX_URL_LENGTH    2048
#define MAX_TITLE_LENGTH  1024
#define MAX_ITEMS_VISIBLE 25
#define MAX_MEDIA_ITEMS   10000

#define HTTP_TIMEOUT_MS    60000
#define STREAM_BUFFER_SIZE (128 * 1024 * 1024)  /* 128MB - PS5 has 16GB RAM */

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

/* DualSense buttons */
typedef enum {
    BTN_CROSS     = (1 << 0),
    BTN_CIRCLE    = (1 << 1),
    BTN_SQUARE    = (1 << 2),
    BTN_TRIANGLE  = (1 << 3),
    BTN_OPTIONS   = (1 << 4),
    BTN_CREATE    = (1 << 5),  /* Was Share on DS4 */
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
    BTN_PS        = (1 << 17),
    BTN_MUTE      = (1 << 18)  /* New on DualSense */
} button_mask_t;

typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    char description[4096];
    char thumbnail_url[MAX_URL_LENGTH];
    char backdrop_url[MAX_URL_LENGTH];
    media_type_t type;
    bool is_directory;
    uint32_t duration;
    uint64_t size;
    int year;
    float rating;
    char genres[512];
    char cast[2048];
    char director[256];
    bool has_hdr;
    bool has_dolby_vision;
    bool has_atmos;
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
    char session_token[1024];
    uint8_t volume;
    uint8_t library;
    bool autoplay;
    bool show_subtitles;
    uint8_t video_quality;  /* 0=SD, 1=HD, 2=FHD, 3=4K, 4=8K */
    char subtitle_language[8];
    char audio_language[8];
    bool enable_hdr;
    bool enable_dolby_vision;
    bool enable_atmos;
    bool enable_haptic_feedback;
    bool enable_adaptive_triggers;
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
    char video_codec[64];
    char audio_codec[64];
    bool is_hdr;
    bool is_dolby_vision;
    bool is_atmos;
} playback_t;

typedef struct {
    bool initialized;
    bool connected;
    uint32_t ip_addr;
    char local_ip[16];
    int download_speed_mbps;
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

    /* Touchpad with multi-touch */
    struct {
        bool valid;
        int x;
        int y;
    } touches[2];

    /* DualSense haptics */
    bool haptics_enabled;

    uint32_t frame_count;
    char error_msg[512];
    bool running;
    bool is_4k_output;
} app_t;

extern app_t g_app;

/*
 * PS5 CAPABILITIES:
 *
 * With official SDK, PS5 would be the best platform:
 *
 * Video:
 *   - Hardware H.264, H.265, VP9, AV1 decode
 *   - HDR10, Dolby Vision support
 *   - 4K @ 120Hz, 8K @ 60Hz output
 *
 * Audio:
 *   - 3D Tempest Audio Engine
 *   - Dolby Atmos support
 *   - DualSense speaker output
 *
 * Input:
 *   - DualSense haptic feedback
 *   - Adaptive trigger effects
 *   - Motion sensing
 *
 * Storage:
 *   - 5.5 GB/s SSD for instant loading
 *   - No buffering wait times
 *
 * Network:
 *   - WiFi 6 for fast wireless
 *   - Quick resume/reconnect
 */

void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);

int network_init(void);
void network_shutdown(void);
int http_get(const char *url, char **response, size_t *len);

int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);

int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_pressed(button_mask_t button);
bool input_held(button_mask_t button);
void input_set_haptic(int left_intensity, int right_intensity);
void input_set_trigger_effect(int trigger, int mode, int param);

int audio_init(void);
void audio_shutdown(void);
int audio_play_stream(const char *url);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
bool audio_is_playing(void);
void audio_enable_3d(bool enable);

int video_init(void);
void video_shutdown(void);
int video_play_stream(const char *url);
void video_stop(void);
void video_pause(void);
void video_resume(void);
bool video_is_playing(void);
void video_render_frame(void);
void video_enable_hdr(bool enable);

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
