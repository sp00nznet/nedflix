/*
 * Nedflix for Nintendo GameCube
 * Main header file with common definitions
 *
 * This is a native C port of Nedflix for the Nintendo GameCube (2001)
 * using the devkitPPC toolchain and libogc library.
 *
 * TECHNICAL DEMO / NOVELTY PORT
 * This port demonstrates homebrew development on the GameCube but has
 * significant hardware limitations compared to modern platforms.
 *
 * Hardware specs:
 *   - CPU: IBM PowerPC 750CXe (Gekko) @ 485 MHz
 *   - RAM: 24 MB (16 MB main + 8 MB ARAM for audio)
 *   - GPU: ATI "Flipper" (embedded 3MB texture memory)
 *   - Storage: Memory Card (8 MB max), SD via adapter
 *   - Network: Broadband Adapter (BBA) - rare accessory
 *
 * Limitations:
 *   - No hardware video decoding
 *   - Network requires rare BBA accessory
 *   - Limited RAM for media buffering
 *   - Primary focus: Audio playback & UI demo
 *
 * Build requirements:
 *   - devkitPPC (devkitpro.org)
 *   - libogc
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
#define NEDFLIX_VERSION_MAJOR 1
#define NEDFLIX_VERSION_MINOR 0
#define NEDFLIX_VERSION_PATCH 0
#define NEDFLIX_VERSION_STRING "1.0.0-gc"

/* Screen dimensions (480i/480p NTSC) */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* UI Constants */
#define MAX_PATH_LENGTH     256
#define MAX_URL_LENGTH      256
#define MAX_TITLE_LENGTH    128
#define MAX_ITEMS_PER_PAGE  12
#define MAX_MENU_ITEMS      20

/* Color definitions (GX format: RGBA) */
#define COLOR_BLACK       (GXColor){0, 0, 0, 255}
#define COLOR_WHITE       (GXColor){255, 255, 255, 255}
#define COLOR_RED         (GXColor){229, 9, 20, 255}      /* Netflix red */
#define COLOR_DARK_GRAY   (GXColor){20, 20, 20, 255}
#define COLOR_LIGHT_GRAY  (GXColor){51, 51, 51, 255}
#define COLOR_SELECTED    (GXColor){68, 68, 68, 255}
#define COLOR_TEXT        (GXColor){229, 229, 229, 255}
#define COLOR_TEXT_DIM    (GXColor){128, 128, 128, 255}

/* Application states */
typedef enum {
    STATE_INIT,
    STATE_BROWSING,
    STATE_PLAYING,
    STATE_SETTINGS,
    STATE_ERROR
} app_state_t;

/* Library types */
typedef enum {
    LIBRARY_MUSIC,
    LIBRARY_AUDIOBOOKS,
    LIBRARY_COUNT
} library_type_t;

/* Media file types */
typedef enum {
    MEDIA_TYPE_UNKNOWN,
    MEDIA_TYPE_AUDIO,
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
    uint32_t data_size;
    uint32_t data_offset;
} audio_format_t;

/* Media item structure */
typedef struct {
    char name[MAX_TITLE_LENGTH];
    char path[MAX_PATH_LENGTH];
    media_type_t type;
    uint32_t size;
    bool is_directory;
} media_item_t;

/* Playlist/directory listing */
typedef struct {
    media_item_t *items;
    int count;
    int capacity;
    int selected_index;
    int scroll_offset;
    char current_path[MAX_PATH_LENGTH];
} media_list_t;

/* User settings */
typedef struct {
    int volume;                /* 0-255 for GameCube */
    bool shuffle;
    bool repeat;
    char last_path[MAX_PATH_LENGTH];
} user_settings_t;

/* Playback state */
typedef struct {
    char current_file[MAX_PATH_LENGTH];
    char title[MAX_TITLE_LENGTH];
    bool is_playing;
    bool is_paused;
    double current_time;       /* seconds */
    double duration;           /* seconds */
    int volume;
    audio_format_t format;
    void *audio_buffer;
    uint32_t buffer_size;
    uint32_t play_position;
    int voice;                 /* ASND voice handle */
} playback_state_t;

/* Global application context */
typedef struct {
    app_state_t state;
    user_settings_t settings;
    playback_state_t playback;
    media_list_t media_list;
    library_type_t current_library;
    char error_message[256];
    bool running;
    uint32_t buttons_pressed;
    uint32_t buttons_just_pressed;
    uint32_t buttons_prev;
    /* GX rendering state */
    GXRModeObj *rmode;
    void *framebuffer[2];
    int fb_index;
    bool first_frame;
} app_context_t;

/* Global context (defined in main.c) */
extern app_context_t g_app;

/*
 * Function declarations - implemented in separate source files
 */

/* main.c */
void app_init(void);
void app_shutdown(void);
void app_run(void);

/* ui.c */
int ui_init(void);
void ui_shutdown(void);
void ui_begin_frame(void);
void ui_end_frame(void);
void ui_clear(GXColor color);
void ui_draw_rect(int x, int y, int width, int height, GXColor color);
void ui_draw_text(int x, int y, const char *text, GXColor color);
void ui_draw_text_centered(int y, const char *text, GXColor color);
void ui_draw_header(const char *title);
void ui_draw_menu(const char **items, int count, int selected);
void ui_draw_file_list(media_list_t *list);
void ui_draw_progress_bar(int x, int y, int width, int height, float progress, GXColor fg, GXColor bg);
void ui_draw_loading(const char *message);
void ui_draw_error(const char *message);
void ui_draw_playback_hud(playback_state_t *state);

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
int audio_play(playback_state_t *state);
void audio_stop(playback_state_t *state);
void audio_pause(playback_state_t *state);
void audio_resume(playback_state_t *state);
void audio_set_volume(int volume);
void audio_update(void);
bool audio_is_playing(void);
double audio_get_position(void);
double audio_get_duration(void);

/* filesystem.c */
int fs_init(void);
void fs_shutdown(void);
int fs_list_directory(const char *path, media_list_t *list);
bool fs_file_exists(const char *path);
bool fs_is_audio_file(const char *filename);

/* config.c */
int config_load(user_settings_t *settings);
int config_save(const user_settings_t *settings);
void config_set_defaults(user_settings_t *settings);

/* Utility macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) MIN(MAX(x, lo), hi)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Debug logging (output to Gecko/USB) */
#ifdef DEBUG
#include <debug.h>
#define LOG(fmt, ...) printf("[NEDFLIX] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#define LOG_ERROR(fmt, ...)
#endif

#endif /* NEDFLIX_H */
