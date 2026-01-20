/*
 * Nedflix for Nintendo GameCube
 * Enhanced header with full feature support
 *
 * TECHNICAL DEMO / NOVELTY PORT
 * This port demonstrates homebrew development on the GameCube.
 *
 * Hardware specs:
 *   - CPU: IBM PowerPC 750CXe (Gekko) @ 485 MHz
 *   - RAM: 24 MB (16 MB main + 8 MB ARAM for audio)
 *   - GPU: ATI "Flipper" (embedded 3MB texture memory)
 *   - Storage: Memory Card (8 MB max), SD via adapter
 *   - Network: Broadband Adapter (BBA) - rare accessory
 *
 * Build requirements:
 *   - devkitPPC (devkitpro.org)
 *   - libogc
 *   - libfat
 */

#ifndef NEDFLIX_H
#define NEDFLIX_H

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <malloc.h>
#include <fat.h>
#include <asndlib.h>

/* Version info */
#define NEDFLIX_VERSION_MAJOR 2
#define NEDFLIX_VERSION_MINOR 0
#define NEDFLIX_VERSION_PATCH 0
#define NEDFLIX_VERSION_STRING "2.0.0-gc"

/* Screen dimensions (480i/480p NTSC) */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* UI Constants */
#define MAX_PATH_LENGTH     256
#define MAX_URL_LENGTH      512
#define MAX_TITLE_LENGTH    128
#define MAX_ITEMS_PER_PAGE  12
#define MAX_MENU_ITEMS      20
#define MAX_FAVORITES       50
#define MAX_HISTORY         50
#define MAX_SEARCH_RESULTS  50

/* Audio buffer sizes */
#define AUDIO_BUFFER_SIZE   (64 * 1024)
#define ARAM_BUFFER_SIZE    (4 * 1024 * 1024)

/* Color definitions (GX format: RGBA) */
#define COLOR_BLACK       (GXColor){0, 0, 0, 255}
#define COLOR_WHITE       (GXColor){255, 255, 255, 255}
#define COLOR_RED         (GXColor){229, 9, 20, 255}
#define COLOR_DARK_RED    (GXColor){131, 16, 16, 255}
#define COLOR_DARK_GRAY   (GXColor){20, 20, 20, 255}
#define COLOR_LIGHT_GRAY  (GXColor){51, 51, 51, 255}
#define COLOR_SELECTED    (GXColor){68, 68, 68, 255}
#define COLOR_TEXT        (GXColor){229, 229, 229, 255}
#define COLOR_TEXT_DIM    (GXColor){128, 128, 128, 255}
#define COLOR_SUCCESS     (GXColor){0, 170, 0, 255}
#define COLOR_WARNING     (GXColor){255, 170, 0, 255}
#define COLOR_ERROR       (GXColor){255, 0, 0, 255}

/* Application states - expanded for full feature support */
typedef enum {
    STATE_INIT,
    STATE_SPLASH,
    STATE_MAIN_MENU,
    STATE_BROWSING,
    STATE_SEARCH,
    STATE_FAVORITES,
    STATE_HISTORY,
    STATE_DETAIL,
    STATE_PLAYING,
    STATE_SETTINGS,
    STATE_ABOUT,
    STATE_ERROR,
    STATE_SHUTDOWN
} app_state_t;

/* Library/content types */
typedef enum {
    LIBRARY_MOVIES,
    LIBRARY_TVSHOWS,
    LIBRARY_MUSIC,
    LIBRARY_AUDIOBOOKS,
    LIBRARY_FAVORITES,
    LIBRARY_COUNT
} library_type_t;

/* Content types */
typedef enum {
    CONTENT_MOVIE,
    CONTENT_EPISODE,
    CONTENT_SONG,
    CONTENT_AUDIOBOOK,
    CONTENT_CHANNEL
} content_type_t;

/* Media file types */
typedef enum {
    MEDIA_TYPE_UNKNOWN,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_DIRECTORY
} media_type_t;

/* Controller button masks (match libogc PAD defines) */
typedef enum {
    BTN_A           = PAD_BUTTON_A,
    BTN_B           = PAD_BUTTON_B,
    BTN_X           = PAD_BUTTON_X,
    BTN_Y           = PAD_BUTTON_Y,
    BTN_Z           = PAD_TRIGGER_Z,
    BTN_L           = PAD_TRIGGER_L,
    BTN_R           = PAD_TRIGGER_R,
    BTN_START       = PAD_BUTTON_START,
    BTN_DPAD_UP     = PAD_BUTTON_UP,
    BTN_DPAD_DOWN   = PAD_BUTTON_DOWN,
    BTN_DPAD_LEFT   = PAD_BUTTON_LEFT,
    BTN_DPAD_RIGHT  = PAD_BUTTON_RIGHT
} button_mask_t;

/* Audio format info */
typedef struct {
    int sample_rate;
    int channels;
    int bits_per_sample;
    int bitrate;
    uint32_t data_size;
    uint32_t data_offset;
    uint32_t total_samples;
} audio_format_t;

/* Media item structure - enhanced */
typedef struct {
    char id[64];
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    char artist[64];
    char album[64];
    char description[256];
    media_type_t type;
    content_type_t content_type;
    uint32_t size;
    uint32_t duration_ms;
    uint16_t year;
    uint8_t rating;
    bool is_directory;
    bool is_favorite;
} media_item_t;

/* Playlist/directory listing */
typedef struct {
    media_item_t *items;
    int count;
    int capacity;
    int selected_index;
    int scroll_offset;
    char current_path[MAX_PATH_LENGTH];
    char category_name[64];
} media_list_t;

/* Watch history entry */
typedef struct {
    char id[64];
    char title[MAX_TITLE_LENGTH];
    uint32_t position_ms;
    uint32_t duration_ms;
    uint64_t timestamp;
    content_type_t type;
} history_entry_t;

/* User settings - enhanced */
typedef struct {
    /* Audio */
    int volume;
    bool enable_surround;
    char audio_language[8];

    /* Playback */
    bool shuffle;
    bool repeat;
    bool repeat_one;
    bool autoplay;
    bool continue_watching;

    /* Subtitles */
    bool show_subtitles;
    char subtitle_language[8];
    int subtitle_size;

    /* UI */
    library_type_t library;
    int theme;
    bool show_clock;
    bool animations;

    /* Parental */
    bool parental_enabled;
    int max_rating;
    char parental_pin[8];

    /* Misc */
    char last_path[MAX_PATH_LENGTH];
    bool first_run;
    int active_profile;
} user_settings_t;

/* Playback state - enhanced */
typedef struct {
    char current_file[MAX_PATH_LENGTH];
    char item_id[64];
    char title[MAX_TITLE_LENGTH];
    char artist[64];
    char album[64];

    bool is_playing;
    bool is_paused;
    bool is_buffering;

    uint32_t position_ms;
    uint32_t duration_ms;
    int volume;

    audio_format_t format;
    void *audio_buffer;
    void *aram_buffer;
    uint32_t buffer_size;
    uint32_t aram_size;
    uint32_t play_position;
    int voice;

    /* For video (basic MJPEG support) */
    void *video_buffer;
    uint32_t video_width;
    uint32_t video_height;
    bool has_video;
} playback_state_t;

/* Network state */
typedef struct {
    bool initialized;
    bool connected;
    bool bba_present;
    char ip_address[16];
    char server_url[MAX_URL_LENGTH];
} network_state_t;

/* Authentication state */
typedef struct {
    bool logged_in;
    char username[64];
    char token[256];
    int profile_id;
} auth_state_t;

/* Global application context - expanded */
typedef struct {
    app_state_t state;
    app_state_t prev_state;
    uint32_t state_timer;

    user_settings_t settings;
    playback_state_t playback;
    network_state_t network;
    auth_state_t auth;

    media_list_t media_list;
    media_item_t current_item;
    library_type_t current_library;

    /* Favorites and history */
    char favorite_ids[MAX_FAVORITES][64];
    int favorites_count;
    history_entry_t history[MAX_HISTORY];
    int history_count;

    /* Search */
    char search_query[64];
    media_list_t search_results;

    /* UI state */
    int menu_index;
    int settings_index;
    int grid_x, grid_y;
    int scroll_offset;
    bool show_controls;
    int controls_timer;

    char error_message[256];
    bool running;
    bool initialized;

    /* Input state */
    uint32_t buttons_pressed;
    uint32_t buttons_just_pressed;
    uint32_t buttons_prev;
    int stick_x, stick_y;
    int cstick_x, cstick_y;

    /* GX rendering state */
    GXRModeObj *rmode;
    void *framebuffer[2];
    int fb_index;
    bool first_frame;

    /* ARAM for audio streaming */
    void *aram_base;
    uint32_t aram_size;
} app_context_t;

/* Global context (defined in main.c) */
extern app_context_t g_app;

/*
 * Function declarations
 */

/* main.c */
void app_init(void);
void app_shutdown(void);
void app_run(void);
void app_change_state(app_state_t new_state);

/* ui.c */
int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_clear(GXColor color);
void ui_draw_rect(int x, int y, int width, int height, GXColor color);
void ui_draw_rect_alpha(int x, int y, int width, int height, GXColor color, uint8_t alpha);
void ui_draw_gradient(int x, int y, int width, int height, GXColor top, GXColor bottom);
void ui_draw_text(int x, int y, const char *text, GXColor color);
void ui_draw_text_scaled(int x, int y, const char *text, GXColor color, float scale);
void ui_draw_text_centered(int y, const char *text, GXColor color);
void ui_draw_header(const char *title);
void ui_draw_footer(const char *hint);
void ui_draw_menu(const char **items, int count, int selected);
void ui_draw_grid(media_list_t *list, int cols, int selected_x, int selected_y);
void ui_draw_file_list(media_list_t *list);
void ui_draw_progress_bar(int x, int y, int width, int height, float progress, GXColor fg, GXColor bg);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_playback_hud(playback_state_t *state);
void ui_draw_splash(void);
void ui_draw_main_menu(int selected);
void ui_draw_detail(media_item_t *item);
void ui_draw_search(const char *query, media_list_t *results);
void ui_draw_favorites(media_list_t *list);
void ui_draw_history(history_entry_t *history, int count, int selected);
void ui_draw_settings(user_settings_t *settings, int selected);
void ui_draw_about(void);

/* input.c */
int input_init(void);
void input_shutdown(void);
void input_update(void);
bool input_button_pressed(uint32_t button);
bool input_button_just_pressed(uint32_t button);
int input_get_stick_x(void);
int input_get_stick_y(void);
int input_get_cstick_x(void);
int input_get_cstick_y(void);

/* audio.c */
int audio_init(void);
void audio_shutdown(void);
int audio_load_wav(const char *path, playback_state_t *state);
int audio_load_mp3(const char *path, playback_state_t *state);
int audio_load_ogg(const char *path, playback_state_t *state);
int audio_load_flac(const char *path, playback_state_t *state);
int audio_play(playback_state_t *state);
void audio_stop(playback_state_t *state);
void audio_pause(playback_state_t *state);
void audio_resume(playback_state_t *state);
void audio_seek(int offset_ms);
void audio_seek_absolute(uint32_t position_ms);
void audio_set_volume(int volume);
void audio_set_track(int track);
void audio_update(void);
bool audio_is_playing(void);
bool audio_is_buffering(void);
uint32_t audio_get_position(void);
uint32_t audio_get_duration(void);
int audio_get_buffer_percent(void);

/* video.c - basic MJPEG support */
int video_init(void);
void video_shutdown(void);
int video_load_mjpeg(const char *path, playback_state_t *state);
void video_stop(playback_state_t *state);
void video_update(void);
void *video_get_frame(uint32_t *width, uint32_t *height);
bool video_is_playing(void);

/* network.c - BBA support */
int network_init(void);
void network_shutdown(void);
bool network_is_connected(void);
int network_connect(const char *server_url);
int network_fetch_catalog(media_list_t *list, library_type_t library);
int network_search(const char *query, media_list_t *results);
int network_get_stream_url(const char *item_id, char *url, size_t url_size);

/* http.c - HTTP streaming */
int http_stream_start(const char *url);
int http_stream_read(void *buffer, size_t size);
void http_stream_stop(void);
bool http_stream_active(void);

/* filesystem.c */
int fs_init(void);
void fs_shutdown(void);
int fs_list_directory(const char *path, media_list_t *list);
bool fs_file_exists(const char *path);
bool fs_is_audio_file(const char *filename);
bool fs_is_video_file(const char *filename);
int fs_read_file(const char *path, void **data, size_t *size);
int fs_write_file(const char *path, const void *data, size_t size);

/* config.c */
int config_load(user_settings_t *settings);
int config_save(const user_settings_t *settings);
void config_set_defaults(user_settings_t *settings);
int config_load_favorites(void);
int config_save_favorites(void);
int config_add_favorite(const char *item_id);
int config_remove_favorite(const char *item_id);
bool config_is_favorite(const char *item_id);
int config_load_history(void);
int config_save_history(void);
int config_add_history(const char *id, const char *title, uint32_t position, uint32_t duration, content_type_t type);
uint32_t config_get_resume_position(const char *item_id);
void config_clear_history(void);

/* Utility macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) MIN(MAX(x, lo), hi)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Debug logging */
#ifdef DEBUG
#include <debug.h>
#define LOG(fmt, ...) printf("[NEDFLIX] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#define LOG_ERROR(fmt, ...)
#endif

#endif /* NEDFLIX_H */
