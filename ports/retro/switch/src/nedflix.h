/*
 * Nedflix for Nintendo Switch
 * Fully Feature-Complete Homebrew Port
 *
 * Uses libnx (devkitA64) for:
 *   - NVN graphics (720p docked, 1280x720 handheld)
 *   - Hardware video decode (nvdec H.264/HEVC)
 *   - Full network stack (WiFi/Ethernet)
 *   - Joy-Con/Pro Controller support
 *   - SD card and internal storage
 *   - Touch screen support
 *
 * Hardware (2017):
 *   CPU: ARM Cortex-A57 quad-core @ 1.02 GHz
 *   RAM: 4 GB LPDDR4
 *   GPU: NVIDIA Tegra X1 (Maxwell) @ 768 MHz docked
 *   Storage: 32/64 GB + microSD
 *   Network: 802.11ac WiFi, USB Ethernet
 *
 * Build requirements:
 *   - devkitA64 (devkitpro.org)
 *   - libnx
 *   - switch-curl, switch-ffmpeg (portlibs)
 */

#ifndef NEDFLIX_SWITCH_H
#define NEDFLIX_SWITCH_H

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <malloc.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

/* Version info */
#define NEDFLIX_VERSION "2.0.0-switch"
#define NEDFLIX_VERSION_MAJOR 2
#define NEDFLIX_VERSION_MINOR 0
#define NEDFLIX_VERSION_PATCH 0

/* Screen dimensions */
#define SCREEN_WIDTH_HANDHELD  1280
#define SCREEN_HEIGHT_HANDHELD 720
#define SCREEN_WIDTH_DOCKED    1920
#define SCREEN_HEIGHT_DOCKED   1080
#define SCREEN_WIDTH           1280
#define SCREEN_HEIGHT          720

/* Buffer sizes */
#define MAX_PATH_LENGTH        512
#define MAX_URL_LENGTH         2048
#define MAX_TITLE_LENGTH       256
#define MAX_DESCRIPTION_LEN    2048
#define MAX_ITEMS_VISIBLE      15
#define MAX_MEDIA_ITEMS        1000
#define MAX_SEARCH_RESULTS     100
#define MAX_CHANNELS           500
#define MAX_FAVORITES          100
#define MAX_HISTORY            100
#define MAX_PROFILES           8

/* Network configuration */
#define HTTP_TIMEOUT_MS        30000
#define RECV_BUFFER_SIZE       (256 * 1024)
#define STREAM_BUFFER_SIZE     (32 * 1024 * 1024)  /* 32MB */
#define AUDIO_BUFFER_SIZE      (4 * 1024 * 1024)   /* 4MB */
#define VIDEO_BUFFER_SIZE      (16 * 1024 * 1024)  /* 16MB */

/* Thumbnail configuration */
#define THUMB_CACHE_SIZE       100
#define THUMB_WIDTH            200
#define THUMB_HEIGHT           300

/* Animation timing */
#define ANIM_DURATION_MS       200
#define SCROLL_SPEED           12
#define FADE_SPEED             20

/* Application states */
typedef enum {
    STATE_INIT,
    STATE_SPLASH,
    STATE_NETWORK_INIT,
    STATE_CONNECTING,
    STATE_LOGIN,
    STATE_PROFILE_SELECT,
    STATE_MAIN_MENU,
    STATE_BROWSING,
    STATE_SEARCH,
    STATE_DETAIL,
    STATE_PLAYING,
    STATE_LIVE_TV,
    STATE_CHANNELS,
    STATE_SETTINGS,
    STATE_ABOUT,
    STATE_ERROR,
    STATE_SHUTDOWN
} app_state_t;

/* Media types */
typedef enum {
    MEDIA_TYPE_UNKNOWN,
    MEDIA_TYPE_DIRECTORY,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_PLAYLIST,
    MEDIA_TYPE_CHANNEL
} media_type_t;

/* Library types */
typedef enum {
    LIBRARY_HOME,
    LIBRARY_MUSIC,
    LIBRARY_AUDIOBOOKS,
    LIBRARY_MOVIES,
    LIBRARY_TVSHOWS,
    LIBRARY_LIVE_TV,
    LIBRARY_FAVORITES,
    LIBRARY_HISTORY,
    LIBRARY_SEARCH,
    LIBRARY_COUNT
} library_t;

/* Video quality levels */
typedef enum {
    QUALITY_AUTO,
    QUALITY_SD,       /* 480p */
    QUALITY_HD,       /* 720p */
    QUALITY_FHD,      /* 1080p */
    QUALITY_COUNT
} video_quality_t;

/* Audio formats */
typedef enum {
    AUDIO_FORMAT_UNKNOWN,
    AUDIO_FORMAT_PCM,
    AUDIO_FORMAT_MP3,
    AUDIO_FORMAT_AAC,
    AUDIO_FORMAT_FLAC,
    AUDIO_FORMAT_OGG,
    AUDIO_FORMAT_OPUS
} audio_format_t;

/* Video formats */
typedef enum {
    VIDEO_FORMAT_UNKNOWN,
    VIDEO_FORMAT_H264,
    VIDEO_FORMAT_HEVC,
    VIDEO_FORMAT_VP9
} video_format_t;

/* Button masks (HidNpadButton) */
typedef enum {
    BTN_A           = HidNpadButton_A,
    BTN_B           = HidNpadButton_B,
    BTN_X           = HidNpadButton_X,
    BTN_Y           = HidNpadButton_Y,
    BTN_L           = HidNpadButton_L,
    BTN_R           = HidNpadButton_R,
    BTN_ZL          = HidNpadButton_ZL,
    BTN_ZR          = HidNpadButton_ZR,
    BTN_PLUS        = HidNpadButton_Plus,
    BTN_MINUS       = HidNpadButton_Minus,
    BTN_DPAD_UP     = HidNpadButton_Up,
    BTN_DPAD_DOWN   = HidNpadButton_Down,
    BTN_DPAD_LEFT   = HidNpadButton_Left,
    BTN_DPAD_RIGHT  = HidNpadButton_Right,
    BTN_LSTICK      = HidNpadButton_StickL,
    BTN_RSTICK      = HidNpadButton_StickR,
    BTN_SL          = HidNpadButton_AnySL,
    BTN_SR          = HidNpadButton_AnySR
} button_mask_t;

/* Touch state */
typedef struct {
    bool touched;
    int32_t x;
    int32_t y;
    int32_t prev_x;
    int32_t prev_y;
    int32_t delta_x;
    int32_t delta_y;
    uint64_t touch_start;
} touch_state_t;

/* Media item structure */
typedef struct {
    char id[64];
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    char description[MAX_DESCRIPTION_LEN];
    char thumbnail_url[MAX_URL_LENGTH];
    char stream_url[MAX_URL_LENGTH];
    char genre[64];
    char director[128];
    char cast[512];
    media_type_t type;
    bool is_directory;
    uint32_t duration;
    uint64_t size;
    int year;
    float rating;
    int season;
    int episode;
    uint32_t resume_position;
    bool watched;
    bool favorite;
} media_item_t;

/* Media list container */
typedef struct {
    media_item_t *items;
    int count;
    int capacity;
    int selected_index;
    int scroll_offset;
    float scroll_anim;
    char current_path[MAX_PATH_LENGTH];
    char title[MAX_TITLE_LENGTH];
} media_list_t;

/* Live TV channel */
typedef struct {
    char id[64];
    char name[MAX_TITLE_LENGTH];
    char logo_url[MAX_URL_LENGTH];
    char stream_url[MAX_URL_LENGTH];
    char group[64];
    char current_program[MAX_TITLE_LENGTH];
    char next_program[MAX_TITLE_LENGTH];
    uint32_t current_start;
    uint32_t current_end;
    int channel_number;
    bool favorite;
} channel_t;

/* Channel list */
typedef struct {
    channel_t *channels;
    int count;
    int capacity;
    int selected_index;
    int scroll_offset;
    char current_group[64];
} channel_list_t;

/* User profile */
typedef struct {
    char id[64];
    char name[64];
    char avatar_url[MAX_URL_LENGTH];
    int avatar_index;
    bool is_admin;
    bool is_kid;
} user_profile_t;

/* User settings */
typedef struct {
    /* Connection */
    char server_url[MAX_URL_LENGTH];
    char username[64];
    char password[64];
    char session_token[256];
    char refresh_token[256];

    /* Profiles */
    user_profile_t profiles[MAX_PROFILES];
    int profile_count;
    int active_profile;

    /* Playback */
    uint8_t volume;
    uint8_t video_quality;
    bool autoplay;
    bool autoplay_next;
    bool show_subtitles;
    char subtitle_language[8];
    char audio_language[8];
    float playback_speed;
    bool skip_intro;
    bool skip_credits;
    bool continue_watching;

    /* Audio */
    bool enable_surround;
    uint8_t audio_output;

    /* Display */
    bool enable_1080p;
    bool enable_hdr;
    uint8_t aspect_ratio;
    int brightness;

    /* Subtitles */
    uint8_t subtitle_size;
    uint8_t subtitle_color;
    uint8_t subtitle_bg;

    /* Network */
    uint8_t buffer_size;
    bool auto_quality;
    bool wifi_only;

    /* UI */
    uint8_t library;
    uint8_t theme;
    bool animations;
    bool show_clock;
    bool touch_enabled;

    /* Parental */
    bool parental_enabled;
    char parental_pin[8];
    int max_rating;

    /* Remember login */
    bool remember_login;
} user_settings_t;

/* Playback state */
typedef struct {
    char title[MAX_TITLE_LENGTH];
    char url[MAX_URL_LENGTH];
    char subtitle_url[MAX_URL_LENGTH];
    char media_path[MAX_PATH_LENGTH];

    bool playing;
    bool paused;
    bool buffering;
    bool is_audio;
    bool is_live;

    uint32_t position_ms;
    uint32_t duration_ms;
    uint32_t buffer_start_ms;
    uint32_t buffer_end_ms;
    int buffer_percent;

    uint8_t volume;
    float speed;
    int bitrate_kbps;

    int width;
    int height;
    float fps;
    char video_codec[32];
    char audio_codec[32];

    int audio_track_count;
    int current_audio_track;
    char audio_tracks[8][64];

    int subtitle_count;
    int current_subtitle;
    char subtitle_tracks[8][64];
    bool subtitles_visible;
} playback_t;

/* Subtitle entry */
typedef struct {
    uint32_t start_ms;
    uint32_t end_ms;
    char text[512];
} subtitle_entry_t;

/* Subtitle state */
typedef struct {
    subtitle_entry_t *entries;
    int count;
    int capacity;
    int current_index;
    bool loaded;
} subtitle_state_t;

/* Network state */
typedef struct {
    bool initialized;
    bool connected;
    bool wifi_connected;
    uint32_t ip_addr;
    char local_ip[16];
    int signal_strength;
    int download_speed_kbps;
    uint64_t bytes_downloaded;
} network_state_t;

/* Thumbnail cache entry */
typedef struct {
    char url[MAX_URL_LENGTH];
    uint32_t *pixels;
    int width;
    int height;
    bool loaded;
    uint64_t last_used;
} thumb_cache_entry_t;

/* Search state */
typedef struct {
    char query[256];
    int cursor_pos;
    bool active;
    bool has_results;
    media_list_t results;
} search_state_t;

/* Software keyboard state */
typedef struct {
    bool active;
    bool done;
    char input[512];
    char title[128];
    int max_length;
    SwkbdConfig kbd;
} swkbd_state_t;

/* History entry */
typedef struct {
    char item_id[64];
    char title[MAX_TITLE_LENGTH];
    uint32_t position_ms;
    uint32_t duration_ms;
    uint64_t timestamp;
    uint8_t content_type;
} history_entry_t;

/* Animation state */
typedef struct {
    float menu_offset;
    float list_offset;
    float detail_fade;
    float overlay_alpha;
    uint64_t last_frame_time;
} animation_t;

/* Main application context */
typedef struct {
    app_state_t state;
    app_state_t prev_state;
    app_state_t next_state;

    user_settings_t settings;
    playback_t playback;
    media_list_t media;
    channel_list_t channels;
    network_state_t net;
    search_state_t search;
    swkbd_state_t swkbd;
    subtitle_state_t subtitles;
    animation_t anim;
    touch_state_t touch;

    library_t current_library;
    media_item_t *current_item;
    media_item_t detail_item;

    char favorites[MAX_FAVORITES][64];
    int favorite_count;
    history_entry_t history[MAX_HISTORY];
    int history_count;

    thumb_cache_entry_t thumb_cache[THUMB_CACHE_SIZE];

    uint64_t buttons_pressed;
    uint64_t buttons_just_pressed;
    uint64_t buttons_released;
    HidAnalogStickState lstick;
    HidAnalogStickState rstick;
    uint32_t input_repeat_timer;

    uint32_t screen_width;
    uint32_t screen_height;
    bool is_docked;
    bool is_handheld;

    uint32_t frame_count;
    uint64_t frame_time_us;
    uint32_t fps;
    uint64_t uptime_ms;

    char error_msg[512];
    char status_msg[256];
    char toast_msg[128];
    uint32_t toast_timer;

    bool running;
    bool needs_refresh;
    bool show_debug;

    /* Framebuffer */
    NWindow *window;
    Framebuffer fb;
    uint32_t *framebuffer;
    uint32_t fb_width;
    uint32_t fb_height;

    /* Audio output */
    AudioOutBuffer audio_buf[2];
    AudioOutBuffer *current_audio_buf;

    /* Pad state */
    PadState pad;
} app_t;

/* Global application instance */
extern app_t g_app;

/* Core functions */
void app_init(void);
void app_run(void);
void app_shutdown(void);
void app_set_error(const char *msg);
void app_set_status(const char *msg);
void app_show_toast(const char *msg, int duration_ms);
void app_transition_to(app_state_t state);

/* Network functions */
int network_init(void);
void network_shutdown(void);
void network_update(void);
bool network_is_connected(void);
int http_get(const char *url, char **response, size_t *len);
int http_post(const char *url, const char *body, char **response, size_t *len);
int http_download_file(const char *url, const char *path, void (*progress)(int));
int network_connect(const char *url);
int network_send(int sock, const void *data, int len);
int network_recv(int sock, void *buffer, int len);
void network_disconnect(int sock);

/* UI functions */
int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);

/* Text rendering */
void ui_draw_text(int x, int y, const char *text, uint32_t color);
void ui_draw_text_scaled(int x, int y, const char *text, uint32_t color, int scale);
void ui_draw_text_centered(int y, const char *text, uint32_t color);
void ui_draw_text_centered_scaled(int y, const char *text, uint32_t color, int scale);
void ui_draw_text_right(int x, int y, const char *text, uint32_t color);
int ui_text_width(const char *text, int scale);

/* Shape rendering */
void ui_draw_rect(int x, int y, int w, int h, uint32_t color);
void ui_draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha);
void ui_draw_rect_outline(int x, int y, int w, int h, int thickness, uint32_t color);
void ui_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
void ui_draw_gradient_v(int x, int y, int w, int h, uint32_t top, uint32_t bottom);
void ui_draw_gradient_h(int x, int y, int w, int h, uint32_t left, uint32_t right);
void ui_draw_hline(int x, int y, int w, uint32_t color);
void ui_draw_vline(int x, int y, int h, uint32_t color);

/* UI components */
void ui_draw_header(const char *title);
void ui_draw_footer(const char *hints);
void ui_draw_menu(const char **options, int count, int selected);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_splash(int progress);
void ui_draw_search(const char *query, const char *hint);
void ui_draw_settings(const user_settings_t *settings, int selected);
void ui_draw_notification(const char *message, int frames);
void ui_draw_dialog(const char *title, const char *msg, const char **btns, int count, int sel);
void ui_draw_progress_bar(int x, int y, int w, int h, int percent, uint32_t fg, uint32_t bg);

/* Media UI */
void ui_draw_media_list(const media_list_t *list);
void ui_draw_media_detail(const media_item_t *item);
void ui_draw_playback(const playback_t *pb);

/* Input functions */
int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_pressed(uint64_t button);
bool input_just_pressed(uint64_t button);
bool input_released(uint64_t button);
bool input_repeat(uint64_t button);
void touch_update(void);
bool touch_in_rect(int x, int y, int w, int h);

/* Audio functions */
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
int audio_get_buffer_percent(void);

/* Video functions */
int video_init(void);
void video_shutdown(void);
int video_play_stream(const char *url);
void video_stop(void);
void video_pause(void);
void video_resume(void);
void video_seek(int offset_ms);
bool video_is_playing(void);
bool video_is_paused(void);
void video_render_frame(void);
int video_get_width(void);
int video_get_height(void);
uint32_t video_get_position(void);
uint32_t video_get_duration(void);
int video_get_buffer_level(void);

/* Subtitle functions */
int subtitle_load(const char *url);
void subtitle_clear(void);
const char *subtitle_get_current(uint32_t position_ms);
void subtitle_render(uint32_t position_ms);

/* API client functions */
int api_init(const char *server);
void api_shutdown(void);
int api_login(const char *user, const char *pass, char *token, size_t len);
int api_get_profiles(const char *token, user_profile_t *profiles, int *count);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_search(const char *token, const char *query, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, video_quality_t q, char *url, size_t len);
int api_report_progress(const char *token, const char *path, uint32_t position);
int api_get_channels(const char *token, channel_list_t *list);

/* Configuration functions */
int config_load(user_settings_t *s);
int config_save(const user_settings_t *s);
void config_defaults(user_settings_t *s);
int config_load_favorites(void);
int config_save_favorites(void);
int config_add_favorite(const char *item_id);
int config_remove_favorite(const char *item_id);
bool config_is_favorite(const char *item_id);
int config_load_history(void);
int config_save_history(void);
int config_add_history(const char *id, const char *title, uint32_t pos, uint32_t dur, uint8_t type);
uint32_t config_get_resume_position(const char *item_id);
void config_clear_history(void);

/* Software keyboard */
void swkbd_show(const char *title, char *output, int max_len);
void swkbd_update(void);
bool swkbd_is_active(void);
bool swkbd_is_done(void);

/* JSON parser */
typedef struct json_value json_value_t;
json_value_t *json_parse(const char *text);
void json_free(json_value_t *v);
const char *json_get_string(json_value_t *obj, const char *key);
int json_get_int(json_value_t *obj, const char *key, int def);
bool json_get_bool(json_value_t *obj, const char *key, bool def);
json_value_t *json_get_object(json_value_t *obj, const char *key);
json_value_t *json_get_array(json_value_t *obj, const char *key);
int json_array_length(json_value_t *arr);
json_value_t *json_array_get(json_value_t *arr, int i);

/* Utility macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) MIN(MAX(x, lo), hi)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define LERP(a, b, t) ((a) + ((b) - (a)) * (t))

/* Time utilities */
uint64_t get_time_ms(void);
uint64_t get_time_us(void);
void format_duration(uint32_t ms, char *buf, size_t len);
void format_size(uint64_t bytes, char *buf, size_t len);

/* Colors - Nedflix theme (RGBA8888) */
#define COLOR_BLACK         0x000000FF
#define COLOR_WHITE         0xFFFFFFFF
#define COLOR_RED           0xE50914FF
#define COLOR_RED_DARK      0xB20710FF
#define COLOR_DARK_BG       0x0A0A0AFF
#define COLOR_MENU_BG       0x141414FF
#define COLOR_CARD_BG       0x1A1A1AFF
#define COLOR_SELECTED      0x2A2A2AFF
#define COLOR_HOVER         0x333333FF
#define COLOR_TEXT          0xE5E5E5FF
#define COLOR_TEXT_DIM      0x808080FF
#define COLOR_TEXT_BRIGHT   0xFFFFFFFF
#define COLOR_ACCENT        0xE50914FF
#define COLOR_SUCCESS       0x46D369FF
#define COLOR_WARNING       0xFFA500FF
#define COLOR_ERROR         0xFF4444FF

/* Color conversion */
#define RGBA8(r, g, b, a) (((r) << 24) | ((g) << 16) | ((b) << 8) | (a))
#define RGBA_R(c) (((c) >> 24) & 0xFF)
#define RGBA_G(c) (((c) >> 16) & 0xFF)
#define RGBA_B(c) (((c) >> 8) & 0xFF)
#define RGBA_A(c) ((c) & 0xFF)

#endif /* NEDFLIX_SWITCH_H */
