/*
 * Nedflix for Original Xbox
 * UI rendering with DirectX 8 / pbKit
 */

#include "nedflix.h"
#include <string.h>
#include <stdio.h>

#ifdef NXDK
#include <pbkit/pbkit.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include <xboxkrnl/xboxkrnl.h>

/* pbKit surface pointers */
static DWORD *g_fb = NULL;
static int g_fb_width = 0;
static int g_fb_height = 0;
static int g_fb_pitch = 0;

#else
/* Stub for non-Xbox builds */
static uint32_t g_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
#endif

/* Font metrics (simple 8x16 bitmap font) */
#define FONT_WIDTH  8
#define FONT_HEIGHT 16
#define FONT_CHARS  128

/* Simple 8x16 bitmap font data (ASCII 32-127) */
/* This is a minimal built-in font - a real implementation would load a font file */
static const uint8_t g_font_data[96][16] = {
    /* Space (32) - just blank */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ! (33) */
    {0x00,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00},
    /* Remaining characters would be defined here... */
    /* For brevity, using a simple placeholder that draws boxes */
};

/* Initialize UI system */
int ui_init(void)
{
    LOG("Initializing UI...");

#ifdef NXDK
    /* Initialize pbKit */
    int status = pb_init();
    if (status != 0) {
        LOG_ERROR("pbKit init failed: %d", status);
        return -1;
    }

    /* Wait for GPU */
    pb_show_front_screen();

    /* Get framebuffer info */
    g_fb_width = SCREEN_WIDTH;
    g_fb_height = SCREEN_HEIGHT;
    g_fb_pitch = g_fb_width * 4;  /* 32-bit ARGB */

    LOG("UI initialized: %dx%d", g_fb_width, g_fb_height);
#endif

    return 0;
}

/* Shutdown UI system */
void ui_shutdown(void)
{
#ifdef NXDK
    pb_kill();
#endif
    LOG("UI shutdown");
}

/* Begin frame rendering */
void ui_begin_frame(void)
{
#ifdef NXDK
    pb_wait_for_vbl();
    pb_target_back_buffer();
    pb_reset();
    pb_erase_depth_stencil_buffer(0, 0, 0xFFFFFF);
    g_fb = (DWORD *)pb_back_buffer();
#endif
}

/* End frame rendering */
void ui_end_frame(void)
{
#ifdef NXDK
    while (pb_busy());
    while (pb_finished());
#endif
}

/* Clear screen with color */
void ui_clear(uint32_t color)
{
#ifdef NXDK
    if (!g_fb) return;

    for (int y = 0; y < g_fb_height; y++) {
        DWORD *row = g_fb + (y * g_fb_width);
        for (int x = 0; x < g_fb_width; x++) {
            row[x] = color;
        }
    }
#else
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        g_framebuffer[i] = color;
    }
#endif
}

/* Draw filled rectangle */
void ui_draw_rect(int x, int y, int width, int height, uint32_t color)
{
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > SCREEN_WIDTH) width = SCREEN_WIDTH - x;
    if (y + height > SCREEN_HEIGHT) height = SCREEN_HEIGHT - y;
    if (width <= 0 || height <= 0) return;

#ifdef NXDK
    if (!g_fb) return;

    for (int py = y; py < y + height; py++) {
        DWORD *row = g_fb + (py * g_fb_width);
        for (int px = x; px < x + width; px++) {
            row[px] = color;
        }
    }
#else
    for (int py = y; py < y + height; py++) {
        for (int px = x; px < x + width; px++) {
            g_framebuffer[py * SCREEN_WIDTH + px] = color;
        }
    }
#endif
}

/* Draw single character */
static void draw_char(int x, int y, char c, uint32_t color)
{
    if (c < 32 || c > 127) c = '?';
    int char_index = c - 32;

    /* Simple box drawing for characters without full font data */
    if (char_index < 0 || char_index >= 96) return;

#ifdef NXDK
    if (!g_fb) return;

    /* Draw character from font data or simple box */
    for (int py = 0; py < FONT_HEIGHT; py++) {
        if (y + py < 0 || y + py >= g_fb_height) continue;

        DWORD *row = g_fb + ((y + py) * g_fb_width);
        uint8_t font_row = g_font_data[char_index][py];

        for (int px = 0; px < FONT_WIDTH; px++) {
            if (x + px < 0 || x + px >= g_fb_width) continue;

            /* Check if pixel is set in font (or draw simple box for visible chars) */
            bool draw = false;
            if (char_index > 0) {  /* Not space */
                if (font_row & (0x80 >> px)) {
                    draw = true;
                } else if (c >= 'A' && c <= 'Z') {
                    /* Simple capital letter box */
                    draw = (py >= 2 && py <= 13 && (px == 1 || px == 6 || py == 2 || py == 13));
                } else if (c >= 'a' && c <= 'z') {
                    /* Simple lowercase letter box */
                    draw = (py >= 5 && py <= 13 && (px == 1 || px == 6 || py == 5 || py == 13));
                } else if (c >= '0' && c <= '9') {
                    /* Simple number box */
                    draw = (py >= 2 && py <= 13 && (px == 1 || px == 6 || py == 2 || py == 13));
                } else {
                    /* Punctuation - draw a dot or line */
                    draw = (py >= 6 && py <= 10 && px >= 2 && px <= 5);
                }
            }

            if (draw) {
                row[x + px] = color;
            }
        }
    }
#else
    /* Non-Xbox: skip rendering */
    (void)x; (void)y; (void)c; (void)color;
#endif
}

/* Draw text string */
void ui_draw_text(int x, int y, const char *text, uint32_t color)
{
    if (!text) return;

    int cx = x;
    while (*text) {
        if (*text == '\n') {
            cx = x;
            y += FONT_HEIGHT;
        } else {
            draw_char(cx, y, *text, color);
            cx += FONT_WIDTH;
        }
        text++;
    }
}

/* Draw centered text */
void ui_draw_text_centered(int y, const char *text, uint32_t color)
{
    if (!text) return;
    int len = strlen(text);
    int x = (SCREEN_WIDTH - len * FONT_WIDTH) / 2;
    ui_draw_text(x, y, text, color);
}

/* Draw header bar */
void ui_draw_header(const char *title)
{
    /* Background bar */
    ui_draw_rect(0, 0, SCREEN_WIDTH, 50, COLOR_BLACK);

    /* Title */
    ui_draw_text(20, 17, title, COLOR_RED);

    /* Divider line */
    ui_draw_rect(0, 48, SCREEN_WIDTH, 2, COLOR_RED);
}

/* Draw menu list */
void ui_draw_menu(const char **items, int count, int selected)
{
    int start_y = 80;
    int item_height = 35;

    for (int i = 0; i < count; i++) {
        int y = start_y + i * item_height;

        /* Selection highlight */
        if (i == selected) {
            ui_draw_rect(20, y - 5, SCREEN_WIDTH - 40, item_height - 5, COLOR_SELECTED);
        }

        /* Item text */
        uint32_t color = (i == selected) ? COLOR_WHITE : COLOR_TEXT;
        ui_draw_text(40, y, items[i], color);
    }
}

/* Draw file/media list */
void ui_draw_file_list(media_list_t *list)
{
    int start_y = 70;
    int item_height = 35;
    int visible_items = MIN(list->count - list->scroll_offset, MAX_ITEMS_PER_PAGE);

    /* Current path */
    ui_draw_text(20, 55, list->current_path, COLOR_TEXT_DIM);

    if (list->count == 0) {
        ui_draw_text_centered(start_y + 100, "No items found", COLOR_TEXT_DIM);
        return;
    }

    for (int i = 0; i < visible_items; i++) {
        int index = list->scroll_offset + i;
        if (index >= list->count) break;

        media_item_t *item = &list->items[index];
        int y = start_y + i * item_height;

        /* Selection highlight */
        if (index == list->selected_index) {
            ui_draw_rect(20, y - 2, SCREEN_WIDTH - 40, item_height - 5, COLOR_SELECTED);
        }

        /* Icon based on type */
        const char *icon = "";
        if (item->is_directory) {
            icon = "[D] ";
        } else if (item->type == MEDIA_TYPE_VIDEO) {
            icon = "[V] ";
        } else if (item->type == MEDIA_TYPE_AUDIO) {
            icon = "[A] ";
        }

        /* Item name */
        char display[MAX_TITLE_LENGTH + 8];
        snprintf(display, sizeof(display), "%s%s", icon, item->name);

        uint32_t color = (index == list->selected_index) ? COLOR_WHITE : COLOR_TEXT;
        ui_draw_text(40, y + 5, display, color);
    }

    /* Scroll indicators */
    if (list->scroll_offset > 0) {
        ui_draw_text_centered(start_y - 15, "^ More ^", COLOR_TEXT_DIM);
    }
    if (list->scroll_offset + MAX_ITEMS_PER_PAGE < list->count) {
        ui_draw_text_centered(start_y + MAX_ITEMS_PER_PAGE * item_height, "v More v", COLOR_TEXT_DIM);
    }

    /* Item count */
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d / %d", list->selected_index + 1, list->count);
    ui_draw_text(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 60, count_str, COLOR_TEXT_DIM);
}

/* Draw progress bar */
void ui_draw_progress_bar(int x, int y, int width, int height, float progress, uint32_t fg, uint32_t bg)
{
    /* Background */
    ui_draw_rect(x, y, width, height, bg);

    /* Foreground (progress) */
    int fill_width = (int)(width * CLAMP(progress, 0.0f, 1.0f));
    if (fill_width > 0) {
        ui_draw_rect(x, y, fill_width, height, fg);
    }
}

/* Draw loading screen */
void ui_draw_loading(const char *message)
{
    ui_draw_text_centered(SCREEN_HEIGHT / 2 - 20, "NEDFLIX", COLOR_RED);
    ui_draw_text_centered(SCREEN_HEIGHT / 2 + 20, message, COLOR_TEXT);

    /* Animated dots */
    static int dots = 0;
    static int frame = 0;
    if (++frame > 30) {
        dots = (dots + 1) % 4;
        frame = 0;
    }

    char loading[16] = "Loading";
    for (int i = 0; i < dots; i++) {
        strcat(loading, ".");
    }
    ui_draw_text_centered(SCREEN_HEIGHT / 2 + 50, loading, COLOR_TEXT_DIM);
}

/* Draw error screen */
void ui_draw_error(const char *message)
{
    ui_draw_text_centered(SCREEN_HEIGHT / 2 - 40, "ERROR", COLOR_RED);
    ui_draw_text_centered(SCREEN_HEIGHT / 2, message, COLOR_TEXT);
}

/* Draw playback HUD */
void ui_draw_playback_hud(playback_state_t *state)
{
    /* Semi-transparent bottom bar */
    ui_draw_rect(0, SCREEN_HEIGHT - 80, SCREEN_WIDTH, 80, 0xC0000000);

    /* Title */
    ui_draw_text(20, SCREEN_HEIGHT - 75, state->title, COLOR_WHITE);

    /* Progress bar */
    float progress = 0.0f;
    if (state->duration > 0) {
        progress = (float)(state->current_time / state->duration);
    }
    ui_draw_progress_bar(20, SCREEN_HEIGHT - 45, SCREEN_WIDTH - 40, 8, progress, COLOR_RED, COLOR_LIGHT_GRAY);

    /* Time display */
    char time_str[64];
    int cur_min = (int)state->current_time / 60;
    int cur_sec = (int)state->current_time % 60;
    int dur_min = (int)state->duration / 60;
    int dur_sec = (int)state->duration % 60;

    snprintf(time_str, sizeof(time_str), "%02d:%02d / %02d:%02d", cur_min, cur_sec, dur_min, dur_sec);
    ui_draw_text(20, SCREEN_HEIGHT - 30, time_str, COLOR_TEXT);

    /* Volume */
    char vol_str[16];
    snprintf(vol_str, sizeof(vol_str), "Vol: %d%%", state->volume);
    ui_draw_text(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 30, vol_str, COLOR_TEXT);

    /* Pause indicator */
    if (state->is_paused) {
        ui_draw_text_centered(SCREEN_HEIGHT / 2, "|| PAUSED ||", COLOR_WHITE);
    }
}
