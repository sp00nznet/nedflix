/*
 * Nedflix for PlayStation 3
 * Fully Functional Feature-Complete Port
 *
 * Uses PSL1GHT (open source SDK) for:
 *   - RSX graphics (720p/1080p)
 *   - Cell SPE audio/video decode
 *   - BSD networking
 *   - DualShock 3 input
 *   - HDD storage
 *
 * Hardware (2006):
 *   CPU: 3.2 GHz Cell BE (1 PPE + 6 SPEs)
 *   RAM: 256 MB XDR + 256 MB GDDR3
 *   GPU: 550 MHz RSX (NVIDIA)
 *   Network: Gigabit Ethernet + WiFi
 *   Storage: HDD 20-500 GB
 *
 * Build requirements:
 *   - PSL1GHT SDK
 *   - ps3toolchain
 *   - ps3libraries
 */

#ifndef NEDFLIX_PS3_H
#define NEDFLIX_PS3_H

#include <psl1ght/lv2.h>
#include <io/pad.h>
#include <sysutil/sysutil.h>
#include <sysutil/osk.h>
#include <net/net.h>
#include <audio/audio.h>
#include <stdbool.h>
#include <stdint.h>

#define NEDFLIX_VERSION "2.0.0-ps3"
#define NEDFLIX_BUILD_DATE __DATE__

#ifndef NEDFLIX_CLIENT_MODE
#define NEDFLIX_CLIENT_MODE 1
#endif

/* Display configuration */
#define SCREEN_WIDTH_720P   1280
#define SCREEN_HEIGHT_720P  720
#define SCREEN_WIDTH_1080P  1920
#define SCREEN_HEIGHT_1080P 1080
#define SCREEN_WIDTH        SCREEN_WIDTH_720P
#define SCREEN_HEIGHT       SCREEN_HEIGHT_720P

/* Buffer sizes */
#define MAX_PATH_LENGTH     512
#define MAX_URL_LENGTH      1024
#define MAX_TITLE_LENGTH    256
#define MAX_DESCRIPTION_LEN 2048
#define MAX_ITEMS_VISIBLE   12
#define MAX_MEDIA_ITEMS     1000
#define MAX_SEARCH_RESULTS  100
#define MAX_CHANNELS        500
#define MAX_FAVORITES       100
#define MAX_HISTORY         50

/* Network configuration */
#define HTTP_TIMEOUT_MS     30000
#define RECV_BUFFER_SIZE    131072
#define STREAM_BUFFER_SIZE  (16 * 1024 * 1024)  /* 16MB streaming buffer */
#define AUDIO_BUFFER_SIZE   (2 * 1024 * 1024)   /* 2MB audio buffer */
#define VIDEO_BUFFER_SIZE   (8 * 1024 * 1024)   /* 8MB video buffer */

/* Thumbnail cache */
#define THUMB_CACHE_SIZE    50
#define THUMB_WIDTH         160
#define THUMB_HEIGHT        240

/* Font configuration */
#define FONT_CHAR_W         10
#define FONT_CHAR_H         18
#define FONT_SMALL_W        8
#define FONT_SMALL_H        14
#define FONT_LARGE_W        14
#define FONT_LARGE_H        24

/* Animation timing */
#define ANIM_DURATION_MS    200
#define SCROLL_SPEED        8
#define FADE_SPEED          16

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
    QUALITY_SD,      /* 480p */
    QUALITY_HD,      /* 720p */
    QUALITY_FHD,     /* 1080p */
    QUALITY_COUNT
} video_quality_t;

/* Audio formats */
typedef enum {
    AUDIO_FORMAT_UNKNOWN,
    AUDIO_FORMAT_PCM,
    AUDIO_FORMAT_MP3,
    AUDIO_FORMAT_AAC,
    AUDIO_FORMAT_FLAC,
    AUDIO_FORMAT_OGG
} audio_format_t;

/* Video formats */
typedef enum {
    VIDEO_FORMAT_UNKNOWN,
    VIDEO_FORMAT_H264,
    VIDEO_FORMAT_MPEG2,
    VIDEO_FORMAT_MPEG4,
    VIDEO_FORMAT_VP9
} video_format_t;

/* DualShock 3 button masks */
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

/* Media item structure */
typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    char description[MAX_DESCRIPTION_LEN];
    char thumbnail_url[MAX_URL_LENGTH];
    char genre[64];
    char director[128];
    char cast[512];
    media_type_t type;
    bool is_directory;
    uint32_t duration;      /* Duration in seconds */
    uint32_t duration_sec;  /* Alias */
    uint64_t size;          /* Size in bytes */
    uint64_t size_bytes;    /* Alias */
    int year;
    float rating;
    int season;
    int episode;
    uint32_t resume_position_sec;
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

    /* Active profile */
    user_profile_t profile;
    user_profile_t profiles[8];
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
    bool normalize_volume;
    int bass_boost;
    uint8_t audio_output;

    /* Video/Display */
    bool enable_1080p;
    bool enable_hdr;
    uint8_t aspect_ratio;
    uint8_t overscan;
    int overscan_adjust;
    int brightness;
    int contrast;

    /* Subtitles */
    uint8_t subtitle_size;
    uint8_t subtitle_color;
    uint8_t subtitle_bg;

    /* Network */
    bool prefer_wifi;
    int buffer_size_mb;
    uint8_t buffer_size;
    bool auto_quality;

    /* UI */
    uint8_t library;
    int theme;  /* 0=dark, 1=light */
    bool animations;
    bool show_clock;
    bool screensaver_enabled;
    int screensaver_timeout_min;
    bool remember_login;

    /* Parental */
    bool parental_enabled;
    char parental_pin[8];
    int max_rating;
} user_settings_t;

/* Playback state */
typedef struct {
    /* Current media */
    char title[MAX_TITLE_LENGTH];
    char url[MAX_URL_LENGTH];
    char subtitle_url[MAX_URL_LENGTH];
    char media_path[MAX_PATH_LENGTH];

    /* State */
    bool playing;
    bool paused;
    bool buffering;
    bool is_audio;
    bool is_live;

    /* Position */
    uint32_t position_ms;
    uint32_t duration_ms;
    uint32_t buffer_start_ms;
    uint32_t buffer_end_ms;

    /* Quality */
    uint8_t volume;
    float speed;
    int buffered_percent;
    int buffer_percent;
    int bitrate_kbps;

    /* Video info */
    int width;
    int height;
    float fps;
    char video_codec[32];
    char audio_codec[32];

    /* Audio tracks */
    int audio_track_count;
    int current_audio_track;
    char audio_tracks[8][64];

    /* Subtitles */
    int subtitle_count;
    int current_subtitle;
    char subtitle_tracks[8][64];
    bool subtitles_visible;

    /* Chapter support */
    int chapter_count;
    int current_chapter;
    uint32_t chapter_times[100];
    char chapter_names[100][64];
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
    char gateway[16];
    char dns[16];
    int signal_strength;
    int download_speed_kbps;
    int upload_speed_kbps;
    uint64_t bytes_downloaded;
    uint64_t bytes_uploaded;
} network_state_t;

/* Thumbnail cache entry */
typedef struct {
    char url[MAX_URL_LENGTH];
    uint32_t *pixels;
    int width;
    int height;
    bool loaded;
    uint32_t last_used;
} thumb_cache_entry_t;

/* Search state */
typedef struct {
    char query[256];
    int cursor_pos;
    bool active;
    bool has_results;
    media_list_t results;
} search_state_t;

/* On-screen keyboard state */
typedef struct {
    bool active;
    bool done;
    char input[512];
    char title[128];
    int max_length;
    oskCallbackReturnParam return_param;
    oskInputFieldInfo field_info;
    oskParam param;
} osk_state_t;

/* Animation state */
typedef struct {
    float menu_offset;
    float list_offset;
    float detail_fade;
    float overlay_alpha;
    float progress_glow;
    uint32_t last_frame_time;
} animation_t;

/* Watch history entry */
typedef struct {
    char path[MAX_PATH_LENGTH];
    char title[MAX_TITLE_LENGTH];
    uint32_t position_sec;
    uint32_t duration_sec;
    uint64_t timestamp;
} history_entry_t;

/* Main application state */
typedef struct {
    /* State machine */
    app_state_t state;
    app_state_t prev_state;
    app_state_t next_state;

    /* Core data */
    user_settings_t settings;
    playback_t playback;
    media_list_t media;
    channel_list_t channels;
    network_state_t net;
    search_state_t search;
    osk_state_t osk;
    subtitle_state_t subtitles;
    animation_t anim;

    /* Current context */
    library_t current_library;
    media_item_t *current_item;
    media_item_t detail_item;

    /* Favorites and history */
    media_item_t favorites[MAX_FAVORITES];
    int favorite_count;
    history_entry_t history[MAX_HISTORY];
    int history_count;

    /* Thumbnail cache */
    thumb_cache_entry_t thumb_cache[THUMB_CACHE_SIZE];

    /* Input state */
    uint32_t buttons_pressed;
    uint32_t buttons_just_pressed;
    uint32_t buttons_released;
    int16_t lstick_x;
    int16_t lstick_y;
    int16_t rstick_x;
    int16_t rstick_y;
    uint8_t l2_pressure;
    uint8_t r2_pressure;
    uint32_t input_repeat_timer;

    /* Display info */
    uint32_t screen_width;
    uint32_t screen_height;
    bool is_1080p;

    /* Frame timing */
    uint32_t frame_count;
    uint32_t frame_time_us;
    uint32_t fps;
    uint64_t uptime_ms;

    /* Messages */
    char error_msg[512];
    char status_msg[256];
    char toast_msg[128];
    uint32_t toast_timer;

    /* System state */
    bool running;
    bool needs_refresh;
    bool show_debug;

    /* PS3 specific */
    void *gcm_context;
    uint32_t *video_buffer;
    void *audio_port;

    /* SPE handles for media decode */
    void *spe_audio;
    void *spe_video;
} app_t;

/* Global application instance */
extern app_t g_app;

/* Core application functions */
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
void network_update_stats(void);
int http_get(const char *url, char **response, size_t *len);
int http_post(const char *url, const char *body, char **response, size_t *len);
int http_get_with_headers(const char *url, const char *headers, char **response, size_t *len);
int http_download_file(const char *url, const char *path, void (*progress)(int percent));
int http_stream_start(const char *url);
int http_stream_read(void *buffer, size_t size);
void http_stream_stop(void);

/* DNS resolution */
int dns_resolve(const char *hostname, char *ip, size_t ip_len);

/* Low-level socket functions */
int network_connect(const char *url);
int network_send(int sock, const void *data, int len);
int network_recv(int sock, void *buffer, int len);
void network_disconnect(int sock);

/* UI functions */
int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_update_animations(float dt);

/* Text rendering */
void ui_draw_text(int x, int y, const char *text, uint32_t color);
void ui_draw_text_scaled(int x, int y, const char *text, uint32_t color, int scale);
void ui_draw_text_small(int x, int y, const char *text, uint32_t color);
void ui_draw_text_large(int x, int y, const char *text, uint32_t color);
void ui_draw_text_centered(int y, const char *text, uint32_t color);
void ui_draw_text_centered_scaled(int y, const char *text, uint32_t color, int scale);
void ui_draw_text_right(int x, int y, const char *text, uint32_t color);
void ui_draw_text_wrapped(int x, int y, int max_width, const char *text, uint32_t color);
int ui_text_width(const char *text, int scale);
int ui_text_width_small(const char *text);

/* Shape rendering */
void ui_draw_rect(int x, int y, int w, int h, uint32_t color);
void ui_draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha);
void ui_draw_rect_outline(int x, int y, int w, int h, int thickness, uint32_t color);
void ui_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
void ui_draw_rect_rounded(int x, int y, int w, int h, int radius, uint32_t color);
void ui_draw_hline(int x, int y, int w, uint32_t color);
void ui_draw_vline(int x, int y, int h, uint32_t color);
void ui_draw_gradient_v(int x, int y, int w, int h, uint32_t top, uint32_t bottom);
void ui_draw_gradient_h(int x, int y, int w, int h, uint32_t left, uint32_t right);
void ui_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void ui_draw_circle(int cx, int cy, int r, uint32_t color);

/* Image rendering */
void ui_draw_image(int x, int y, int w, int h, uint32_t *pixels);
void ui_draw_image_scaled(int x, int y, int w, int h, uint32_t *pixels, int src_w, int src_h);
int ui_load_image(const char *path, uint32_t **pixels, int *w, int *h);

/* UI components */
void ui_draw_header(const char *title);
void ui_draw_footer(void);
void ui_draw_sidebar(library_t selected);
void ui_draw_menu(const char **options, int count, int selected);
void ui_draw_menu_icons(const char **options, const char **icons, int count, int selected);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_splash(int progress);
void ui_draw_search(const char *query, const char *results_hint);
void ui_draw_settings(const user_settings_t *settings, int selected_option);
void ui_draw_notification(const char *message, int duration_frames);
void ui_draw_toast(const char *message);
void ui_draw_dialog(const char *title, const char *message, const char **buttons, int count, int selected);
void ui_draw_progress_bar(int x, int y, int w, int h, int percent, uint32_t fg, uint32_t bg);
void ui_draw_spinner(int x, int y, int size);
void ui_draw_footer(const char *hints);
uint32_t ui_get_time_ms(void);
uint32_t ui_get_screen_width(void);
uint32_t ui_get_screen_height(void);

/* Media UI */
void ui_draw_media_grid(const media_list_t *list, int cols);
void ui_draw_media_list(const media_list_t *list);
void ui_draw_media_detail(const media_item_t *item);
void ui_draw_media_card(int x, int y, int w, int h, const media_item_t *item, bool selected);

/* Playback UI */
void ui_draw_playback(const playback_t *pb);
void ui_draw_playback_controls(const playback_t *pb);
void ui_draw_volume_indicator(int volume);
void ui_draw_subtitle(const char *text);

/* Channel UI */
void ui_draw_channel_list(const channel_list_t *list);
void ui_draw_channel_epg(const channel_t *channel);

/* Settings UI */
void ui_draw_settings_page(int page, int selected);

/* OSK functions */
void ui_show_osk(const char *title, char *output, int max_len);
void ui_update_osk(void);
bool ui_osk_active(void);
bool ui_osk_done(void);
void ui_close_osk(void);

/* Input functions */
int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_pressed(button_mask_t button);
bool input_held(button_mask_t button);
bool input_released(button_mask_t button);
bool input_repeat(button_mask_t button);

/* Audio functions */
int audio_init(void);
void audio_shutdown(void);
int audio_play_stream(const char *url);
int audio_play_file(const char *path);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
void audio_seek(int offset_ms);
void audio_seek_absolute(uint32_t position_ms);
void audio_set_volume(int vol);
void audio_set_track(int track);
void audio_update(void);
bool audio_is_playing(void);
bool audio_is_buffering(void);
uint32_t audio_get_position(void);
uint32_t audio_get_duration(void);
int audio_get_buffer_percent(void);

/* Video functions */
int video_init(void);
void video_shutdown(void);
int video_play_stream(const char *url);
int video_play_file(const char *path);
void video_stop(void);
void video_pause(void);
void video_resume(void);
void video_seek(int offset_ms);
void video_seek_absolute(uint32_t position_ms);
void video_set_track(int track);
void video_set_subtitle(int track);
void video_toggle_subtitles(void);
bool video_is_playing(void);
bool video_is_paused(void);
bool video_is_buffering(void);
void video_render_frame(void);
int video_get_width(void);
int video_get_height(void);
float video_get_fps(void);
uint32_t video_get_position(void);
uint32_t video_get_duration(void);
uint8_t *video_get_current_frame(void);
void video_get_stats(uint32_t *decoded, uint32_t *dropped, uint32_t *underruns);
int video_get_buffer_level(void);

/* Subtitle functions */
int subtitle_load(const char *url);
int subtitle_load_srt(const char *data);
int subtitle_load_vtt(const char *data);
void subtitle_clear(void);
const char *subtitle_get_current(uint32_t position_ms);
void subtitle_render(uint32_t position_ms);

/* API client functions */
int api_init(const char *server);
void api_shutdown(void);
int api_login(const char *user, const char *pass, char *token, size_t len);
int api_logout(const char *token);
int api_refresh_token(const char *refresh, char *token, size_t len);
int api_get_profiles(const char *token, user_profile_t *profiles, int *count);
int api_browse(const char *token, const char *path, library_t lib, media_list_t *list);
int api_browse_recent(const char *token, media_list_t *list);
int api_browse_continue(const char *token, media_list_t *list);
int api_search(const char *token, const char *query, media_list_t *list);
int api_get_stream_url(const char *token, const char *path, video_quality_t quality, char *url, size_t len);
int api_get_subtitles(const char *token, const char *path, const char *lang, char **srt);
int api_get_media_info(const char *token, const char *path, media_item_t *item);
int api_report_progress(const char *token, const char *path, uint32_t position_sec);
int api_get_channels(const char *token, channel_list_t *list);
int api_get_epg(const char *token, const char *channel_id, channel_t *channel);

/* Configuration functions */
int config_load(user_settings_t *s);
int config_save(const user_settings_t *s);
void config_defaults(user_settings_t *s);
int config_load_favorites(void);
int config_save_favorites(void);
int config_add_favorite(const char *item_id);
int config_remove_favorite(const char *item_id);
bool config_is_favorite(const char *item_id);
int config_get_favorites_count(void);
const char *config_get_favorite_id(int index);

int config_load_history(void);
int config_save_history(void);
int config_add_history(const char *item_id, const char *title, uint32_t position_ms, uint32_t duration_ms, uint8_t content_type);
uint32_t config_get_resume_position(const char *item_id);
void config_clear_history(void);
int config_get_history_count(void);
int config_get_history_entry(int index, char *item_id, char *title, uint32_t *position_ms, uint32_t *duration_ms);
void config_clear_all(void);

/* Thumbnail cache functions */
int thumb_cache_init(void);
void thumb_cache_shutdown(void);
uint32_t *thumb_cache_get(const char *url);
void thumb_cache_prefetch(const char *url);
void thumb_cache_clear(void);

/* Cell SPE functions for media decode */
int spe_init(void);
void spe_shutdown(void);
int spe_decode_audio_frame(void *input, int in_size, void *output, int *out_size);
int spe_decode_video_frame(void *input, int in_size, void *output, int *out_size);

/* JSON parser */
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

/* Utility macros */
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(x,lo,hi) MIN(MAX(x,lo),hi)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define LERP(a,b,t) ((a) + ((b) - (a)) * (t))

/* Time utilities */
uint64_t get_time_ms(void);
uint64_t get_time_us(void);
void format_duration(uint32_t ms, char *buf, size_t len);
void format_size(uint64_t bytes, char *buf, size_t len);
void format_date(uint64_t timestamp, char *buf, size_t len);

/* String utilities */
void str_trim(char *s);
void str_truncate(char *s, int max_len, const char *suffix);
int str_split(const char *s, char delim, char **parts, int max_parts);
void url_encode(const char *input, char *output, size_t output_len);

/* Colors - Nedflix theme */
#define COLOR_BLACK         0x000000FF
#define COLOR_WHITE         0xFFFFFFFF
#define COLOR_RED           0xE50914FF  /* Nedflix red */
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
#define COLOR_OVERLAY       0x00000099
#define COLOR_PROGRESS_BG   0x333333FF
#define COLOR_PROGRESS_FG   0xE50914FF
#define COLOR_BUFFER_FG     0x666666FF

/* Convert color formats */
#define RGBA_TO_XRGB(c) (((c) >> 8) | (((c) & 0xFF) << 24))
#define XRGB_TO_RGBA(c) ((((c) << 8) & 0xFFFFFF00) | (((c) >> 24) & 0xFF))
#define COLOR_ALPHA(c, a) (((c) & 0xFFFFFF00) | ((a) & 0xFF))
#define BLEND_COLORS(c1, c2, t) (\
    ((uint32_t)(LERP(((c1)>>24)&0xFF, ((c2)>>24)&0xFF, t)) << 24) | \
    ((uint32_t)(LERP(((c1)>>16)&0xFF, ((c2)>>16)&0xFF, t)) << 16) | \
    ((uint32_t)(LERP(((c1)>>8)&0xFF, ((c2)>>8)&0xFF, t)) << 8) | \
    ((uint32_t)(LERP((c1)&0xFF, (c2)&0xFF, t))))

#endif /* NEDFLIX_PS3_H */
