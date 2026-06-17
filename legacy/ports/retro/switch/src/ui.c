/*
 * Nedflix Nintendo Switch - UI rendering
 * Full-featured UI with software rendering to framebuffer
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

/* Colors (RGBA8888) */
#define COLOR_BG          0xFF1A1A2E
#define COLOR_BG_DARK     0xFF0F0F1A
#define COLOR_PRIMARY     0xFFE50914
#define COLOR_SECONDARY   0xFF831010
#define COLOR_TEXT        0xFFFFFFFF
#define COLOR_TEXT_DIM    0xFF888888
#define COLOR_HIGHLIGHT   0xFF3A3A5E
#define COLOR_OVERLAY     0xC0000000
#define COLOR_SUCCESS     0xFF00AA00
#define COLOR_WARNING     0xFFFFAA00
#define COLOR_ERROR       0xFFFF0000

/* 8x16 bitmap font */
static const uint8_t font_8x16[256][16] = {
    /* Space and basic ASCII */
    [' '] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    ['!'] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x18,0,0,0,0},
    ['"'] = {0x6C,0x6C,0x6C,0,0,0,0,0,0,0,0,0,0,0,0,0},
    ['#'] = {0x6C,0x6C,0xFE,0x6C,0x6C,0x6C,0xFE,0x6C,0x6C,0,0,0,0,0,0,0},
    ['$'] = {0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0,0,0,0,0,0,0,0,0},
    ['%'] = {0xC6,0xCC,0x18,0x30,0x60,0xCC,0xC6,0,0,0,0,0,0,0,0,0},
    ['&'] = {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0,0,0,0,0,0,0,0,0},
    ['\''] = {0x18,0x18,0x30,0,0,0,0,0,0,0,0,0,0,0,0,0},
    ['('] = {0x0C,0x18,0x30,0x30,0x30,0x30,0x30,0x18,0x0C,0,0,0,0,0,0,0},
    [')'] = {0x30,0x18,0x0C,0x0C,0x0C,0x0C,0x0C,0x18,0x30,0,0,0,0,0,0,0},
    ['*'] = {0,0x66,0x3C,0xFF,0x3C,0x66,0,0,0,0,0,0,0,0,0,0},
    ['+'] = {0,0,0x18,0x18,0x7E,0x18,0x18,0,0,0,0,0,0,0,0,0},
    [','] = {0,0,0,0,0,0,0,0,0x18,0x18,0x30,0,0,0,0,0},
    ['-'] = {0,0,0,0,0,0x7E,0,0,0,0,0,0,0,0,0,0},
    ['.'] = {0,0,0,0,0,0,0,0,0x18,0x18,0,0,0,0,0,0},
    ['/'] = {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0,0,0,0,0,0,0,0,0},
    ['0'] = {0x7C,0xC6,0xCE,0xDE,0xF6,0xE6,0x7C,0,0,0,0,0,0,0,0,0},
    ['1'] = {0x18,0x38,0x78,0x18,0x18,0x18,0x7E,0,0,0,0,0,0,0,0,0},
    ['2'] = {0x7C,0xC6,0x06,0x1C,0x70,0xC6,0xFE,0,0,0,0,0,0,0,0,0},
    ['3'] = {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['4'] = {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0,0,0,0,0,0,0,0,0},
    ['5'] = {0xFE,0xC0,0xC0,0xFC,0x06,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['6'] = {0x38,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['7'] = {0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0,0,0,0,0,0,0,0,0},
    ['8'] = {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['9'] = {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0,0,0,0,0,0,0,0,0},
    [':'] = {0,0,0x18,0x18,0,0,0x18,0x18,0,0,0,0,0,0,0,0},
    [';'] = {0,0,0x18,0x18,0,0,0x18,0x18,0x30,0,0,0,0,0,0,0},
    ['<'] = {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0,0,0,0,0,0,0,0,0},
    ['='] = {0,0,0x7E,0,0,0x7E,0,0,0,0,0,0,0,0,0,0},
    ['>'] = {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0,0,0,0,0,0,0,0,0},
    ['?'] = {0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0,0,0,0,0,0,0,0,0},
    ['@'] = {0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C,0,0,0,0,0,0,0,0,0},
    ['A'] = {0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0,0,0,0,0,0,0,0,0},
    ['B'] = {0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0,0,0,0,0,0,0,0,0},
    ['C'] = {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0,0,0,0,0,0,0,0,0},
    ['D'] = {0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0,0,0,0,0,0,0,0,0},
    ['E'] = {0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0,0,0,0,0,0,0,0,0},
    ['F'] = {0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0,0,0,0,0,0,0,0,0},
    ['G'] = {0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3A,0,0,0,0,0,0,0,0,0},
    ['H'] = {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0,0,0,0,0,0,0,0,0},
    ['I'] = {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0,0,0,0,0},
    ['J'] = {0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0,0,0,0,0,0,0,0,0},
    ['K'] = {0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0,0,0,0,0,0,0,0,0},
    ['L'] = {0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0,0,0,0,0,0,0,0,0},
    ['M'] = {0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0,0,0,0,0,0,0,0,0},
    ['N'] = {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0,0,0,0,0,0,0,0,0},
    ['O'] = {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['P'] = {0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0,0,0,0,0,0,0,0,0},
    ['Q'] = {0x7C,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x0E,0,0,0,0,0,0,0,0},
    ['R'] = {0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0,0,0,0,0,0,0,0,0},
    ['S'] = {0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['T'] = {0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0,0,0,0,0},
    ['U'] = {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['V'] = {0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0,0,0,0,0,0,0,0,0},
    ['W'] = {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0,0,0,0,0,0,0,0,0},
    ['X'] = {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0,0,0,0,0,0,0,0,0},
    ['Y'] = {0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0,0,0,0,0,0,0,0,0},
    ['Z'] = {0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0,0,0,0,0,0,0,0,0},
    ['['] = {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0,0,0,0,0,0,0,0,0},
    ['\\'] = {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0,0,0,0,0,0,0,0,0},
    [']'] = {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0,0,0,0,0,0,0,0,0},
    ['^'] = {0x10,0x38,0x6C,0xC6,0,0,0,0,0,0,0,0,0,0,0,0},
    ['_'] = {0,0,0,0,0,0,0,0,0xFF,0,0,0,0,0,0,0},
    ['`'] = {0x30,0x18,0x0C,0,0,0,0,0,0,0,0,0,0,0,0,0},
    ['a'] = {0,0,0x78,0x0C,0x7C,0xCC,0x76,0,0,0,0,0,0,0,0,0},
    ['b'] = {0xE0,0x60,0x7C,0x66,0x66,0x66,0xDC,0,0,0,0,0,0,0,0,0},
    ['c'] = {0,0,0x7C,0xC6,0xC0,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['d'] = {0x1C,0x0C,0x7C,0xCC,0xCC,0xCC,0x76,0,0,0,0,0,0,0,0,0},
    ['e'] = {0,0,0x7C,0xC6,0xFE,0xC0,0x7C,0,0,0,0,0,0,0,0,0},
    ['f'] = {0x1C,0x36,0x30,0x78,0x30,0x30,0x78,0,0,0,0,0,0,0,0,0},
    ['g'] = {0,0,0x76,0xCC,0xCC,0x7C,0x0C,0xF8,0,0,0,0,0,0,0,0},
    ['h'] = {0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0,0,0,0,0,0,0,0,0},
    ['i'] = {0x18,0,0x38,0x18,0x18,0x18,0x3C,0,0,0,0,0,0,0,0,0},
    ['j'] = {0x06,0,0x06,0x06,0x06,0x66,0x66,0x3C,0,0,0,0,0,0,0,0},
    ['k'] = {0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0,0,0,0,0,0,0,0,0},
    ['l'] = {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0,0,0,0,0},
    ['m'] = {0,0,0xEC,0xFE,0xD6,0xD6,0xD6,0,0,0,0,0,0,0,0,0},
    ['n'] = {0,0,0xDC,0x66,0x66,0x66,0x66,0,0,0,0,0,0,0,0,0},
    ['o'] = {0,0,0x7C,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0,0,0,0,0},
    ['p'] = {0,0,0xDC,0x66,0x66,0x7C,0x60,0xF0,0,0,0,0,0,0,0,0},
    ['q'] = {0,0,0x76,0xCC,0xCC,0x7C,0x0C,0x1E,0,0,0,0,0,0,0,0},
    ['r'] = {0,0,0xDC,0x76,0x60,0x60,0xF0,0,0,0,0,0,0,0,0,0},
    ['s'] = {0,0,0x7E,0xC0,0x7C,0x06,0xFC,0,0,0,0,0,0,0,0,0},
    ['t'] = {0x30,0x30,0xFC,0x30,0x30,0x36,0x1C,0,0,0,0,0,0,0,0,0},
    ['u'] = {0,0,0xCC,0xCC,0xCC,0xCC,0x76,0,0,0,0,0,0,0,0,0},
    ['v'] = {0,0,0xC6,0xC6,0xC6,0x6C,0x38,0,0,0,0,0,0,0,0,0},
    ['w'] = {0,0,0xC6,0xD6,0xD6,0xFE,0x6C,0,0,0,0,0,0,0,0,0},
    ['x'] = {0,0,0xC6,0x6C,0x38,0x6C,0xC6,0,0,0,0,0,0,0,0,0},
    ['y'] = {0,0,0xC6,0xC6,0xC6,0x7E,0x06,0xFC,0,0,0,0,0,0,0,0},
    ['z'] = {0,0,0x7E,0x4C,0x18,0x32,0x7E,0,0,0,0,0,0,0,0,0},
    ['{'] = {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0,0,0,0,0,0,0,0,0},
    ['|'] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0,0,0,0,0,0,0,0,0},
    ['}'] = {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0,0,0,0,0,0,0,0,0},
    ['~'] = {0x76,0xDC,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

/* Static UI state */
static struct {
    bool initialized;
    uint32_t *framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
} ui_state;

/* Initialize UI */
void ui_init(void)
{
    memset(&ui_state, 0, sizeof(ui_state));
    ui_state.initialized = true;
    printf("UI initialized\n");
}

/* Shutdown UI */
void ui_shutdown(void)
{
    ui_state.initialized = false;
}

/* Draw filled rectangle */
static void draw_rect(int x, int y, int w, int h, uint32_t color)
{
    if (!ui_state.framebuffer) return;

    /* Clip to screen bounds */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)ui_state.width) w = ui_state.width - x;
    if (y + h > (int)ui_state.height) h = ui_state.height - y;
    if (w <= 0 || h <= 0) return;

    for (int py = y; py < y + h; py++) {
        uint32_t *row = ui_state.framebuffer + py * ui_state.stride + x;
        for (int px = 0; px < w; px++) {
            row[px] = color;
        }
    }
}

/* Draw rectangle with alpha blending */
static void draw_rect_alpha(int x, int y, int w, int h, uint32_t color)
{
    if (!ui_state.framebuffer) return;

    uint8_t alpha = (color >> 24) & 0xFF;
    if (alpha == 0) return;
    if (alpha == 255) {
        draw_rect(x, y, w, h, color);
        return;
    }

    /* Clip to screen bounds */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)ui_state.width) w = ui_state.width - x;
    if (y + h > (int)ui_state.height) h = ui_state.height - y;
    if (w <= 0 || h <= 0) return;

    uint8_t sr = (color >> 0) & 0xFF;
    uint8_t sg = (color >> 8) & 0xFF;
    uint8_t sb = (color >> 16) & 0xFF;

    for (int py = y; py < y + h; py++) {
        uint32_t *row = ui_state.framebuffer + py * ui_state.stride + x;
        for (int px = 0; px < w; px++) {
            uint32_t dst = row[px];
            uint8_t dr = (dst >> 0) & 0xFF;
            uint8_t dg = (dst >> 8) & 0xFF;
            uint8_t db = (dst >> 16) & 0xFF;

            uint8_t r = (sr * alpha + dr * (255 - alpha)) / 255;
            uint8_t g = (sg * alpha + dg * (255 - alpha)) / 255;
            uint8_t b = (sb * alpha + db * (255 - alpha)) / 255;

            row[px] = 0xFF000000 | (b << 16) | (g << 8) | r;
        }
    }
}

/* Draw rounded rectangle */
static void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color)
{
    /* Draw center */
    draw_rect(x + r, y, w - 2*r, h, color);
    /* Draw sides */
    draw_rect(x, y + r, r, h - 2*r, color);
    draw_rect(x + w - r, y + r, r, h - 2*r, color);

    /* Draw corners (simplified - just filled circles at corners) */
    /* For a proper implementation, use Bresenham circle algorithm */
}

/* Draw single character */
static void draw_char(int x, int y, char c, uint32_t color, int scale)
{
    if (!ui_state.framebuffer) return;
    if (c < 0 || c > 127) c = '?';

    const uint8_t *glyph = font_8x16[(unsigned char)c];

    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                if (scale == 1) {
                    int px = x + col;
                    int py = y + row;
                    if (px >= 0 && px < (int)ui_state.width &&
                        py >= 0 && py < (int)ui_state.height) {
                        ui_state.framebuffer[py * ui_state.stride + px] = color;
                    }
                } else {
                    draw_rect(x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

/* Draw text string */
static void draw_text(int x, int y, const char *text, uint32_t color, int scale)
{
    while (*text) {
        if (*text == '\n') {
            x = 0;
            y += 16 * scale;
        } else {
            draw_char(x, y, *text, color, scale);
            x += 8 * scale;
        }
        text++;
    }
}

/* Draw centered text */
static void draw_text_centered(int cx, int y, const char *text, uint32_t color, int scale)
{
    int len = strlen(text);
    int x = cx - (len * 8 * scale) / 2;
    draw_text(x, y, text, color, scale);
}

/* Draw progress bar */
static void draw_progress_bar(int x, int y, int w, int h, float progress, uint32_t bg, uint32_t fg)
{
    draw_rect(x, y, w, h, bg);
    int fill = (int)(w * CLAMP(progress, 0.0f, 1.0f));
    if (fill > 0) {
        draw_rect(x, y, fill, h, fg);
    }
}

/* Draw button hint */
static void draw_button_hint(int x, int y, const char *button, const char *action)
{
    /* Draw button icon/letter */
    draw_rect(x, y, 24, 24, COLOR_PRIMARY);
    draw_char(x + 8, y + 4, button[0], COLOR_TEXT, 1);

    /* Draw action text */
    draw_text(x + 30, y + 4, action, COLOR_TEXT_DIM, 1);
}

/* Clear screen */
static void clear_screen(uint32_t color)
{
    if (!ui_state.framebuffer) return;

    for (uint32_t y = 0; y < ui_state.height; y++) {
        uint32_t *row = ui_state.framebuffer + y * ui_state.stride;
        for (uint32_t x = 0; x < ui_state.width; x++) {
            row[x] = color;
        }
    }
}

/* Render splash screen */
static void render_splash(app_t *app)
{
    clear_screen(COLOR_BG_DARK);

    /* Logo */
    int logo_x = app->screen_width / 2;
    int logo_y = app->screen_height / 2 - 50;
    draw_text_centered(logo_x, logo_y, "NEDFLIX", COLOR_PRIMARY, 4);

    /* Subtitle */
    draw_text_centered(logo_x, logo_y + 80, "Nintendo Switch Edition", COLOR_TEXT_DIM, 2);

    /* Loading indicator */
    int dots = (app->state_timer / 20) % 4;
    char loading[32];
    snprintf(loading, sizeof(loading), "Loading%.*s", dots, "...");
    draw_text_centered(logo_x, app->screen_height - 100, loading, COLOR_TEXT, 1);
}

/* Render network init screen */
static void render_network_init(app_t *app)
{
    clear_screen(COLOR_BG_DARK);

    draw_text_centered(app->screen_width / 2, app->screen_height / 2 - 20,
                       "Connecting to network...", COLOR_TEXT, 2);

    /* Progress indicator */
    float progress = (float)(app->state_timer % 60) / 60.0f;
    draw_progress_bar(app->screen_width / 4, app->screen_height / 2 + 40,
                      app->screen_width / 2, 8, progress, COLOR_HIGHLIGHT, COLOR_PRIMARY);
}

/* Render login screen */
static void render_login(app_t *app)
{
    clear_screen(COLOR_BG);

    /* Title */
    draw_text_centered(app->screen_width / 2, 100, "Sign In", COLOR_TEXT, 3);

    /* Login box */
    int box_x = app->screen_width / 4;
    int box_w = app->screen_width / 2;
    int box_y = 200;

    draw_rect(box_x, box_y, box_w, 200, COLOR_HIGHLIGHT);

    draw_text(box_x + 20, box_y + 20, "Username:", COLOR_TEXT, 2);
    draw_rect(box_x + 20, box_y + 60, box_w - 40, 40, COLOR_BG_DARK);

    draw_text(box_x + 20, box_y + 120, "Password:", COLOR_TEXT, 2);
    draw_rect(box_x + 20, box_y + 160, box_w - 40, 40, COLOR_BG_DARK);

    /* Button hints */
    draw_button_hint(box_x + 20, box_y + 220, "A", "Sign In");
    draw_button_hint(box_x + 200, box_y + 220, "B", "Guest Mode");
}

/* Render main menu */
static void render_main_menu(app_t *app)
{
    clear_screen(COLOR_BG);

    /* Header */
    draw_rect(0, 0, app->screen_width, 80, COLOR_BG_DARK);
    draw_text(40, 25, "NEDFLIX", COLOR_PRIMARY, 3);

    /* User info */
    char user_text[64];
    snprintf(user_text, sizeof(user_text), "Welcome, %s",
             app->auth.logged_in ? app->auth.username : "Guest");
    draw_text(app->screen_width - 300, 30, user_text, COLOR_TEXT, 2);

    /* Menu items */
    const char *menu_items[] = {
        "Movies", "TV Shows", "Music", "Live TV", "Search", "Settings"
    };

    int menu_y = 150;
    int menu_spacing = 70;

    for (int i = 0; i < 6; i++) {
        uint32_t color = (i == app->menu_index) ? COLOR_PRIMARY : COLOR_TEXT;
        uint32_t bg = (i == app->menu_index) ? COLOR_HIGHLIGHT : COLOR_BG;

        draw_rect(100, menu_y + i * menu_spacing, app->screen_width - 200, 60, bg);
        draw_text(140, menu_y + i * menu_spacing + 15, menu_items[i], color, 2);
    }

    /* Button hints */
    draw_button_hint(100, app->screen_height - 60, "A", "Select");
    draw_button_hint(250, app->screen_height - 60, "+", "Settings");
}

/* Render browsing screen */
static void render_browsing(app_t *app)
{
    clear_screen(COLOR_BG);

    /* Header */
    draw_rect(0, 0, app->screen_width, 80, COLOR_BG_DARK);

    const char *library_names[] = {"Movies", "TV Shows", "Music", "Live TV", "Favorites"};
    draw_text(40, 25, library_names[app->current_library], COLOR_PRIMARY, 3);

    /* Content grid */
    int grid_x = 60;
    int grid_y = 120;
    int card_w = 200;
    int card_h = 300;
    int spacing = 20;

    int cols = (app->screen_width - 2 * grid_x) / (card_w + spacing);

    /* Draw placeholder cards */
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < cols; col++) {
            int x = grid_x + col * (card_w + spacing);
            int y = grid_y + row * (card_h + spacing) - app->scroll_offset;

            if (y + card_h < 80 || y > (int)app->screen_height) continue;

            bool selected = (row == app->grid_y && col == app->grid_x);
            uint32_t border_color = selected ? COLOR_PRIMARY : COLOR_HIGHLIGHT;

            /* Card background */
            draw_rect(x, y, card_w, card_h, COLOR_HIGHLIGHT);

            /* Selection border */
            if (selected) {
                draw_rect(x - 4, y - 4, card_w + 8, 4, border_color);
                draw_rect(x - 4, y + card_h, card_w + 8, 4, border_color);
                draw_rect(x - 4, y, 4, card_h, border_color);
                draw_rect(x + card_w, y, 4, card_h, border_color);
            }

            /* Placeholder title */
            char title[32];
            snprintf(title, sizeof(title), "Item %d", row * cols + col + 1);
            draw_text(x + 10, y + card_h - 30, title, COLOR_TEXT, 1);
        }
    }

    /* Button hints */
    draw_button_hint(60, app->screen_height - 60, "A", "View");
    draw_button_hint(180, app->screen_height - 60, "B", "Back");
    draw_button_hint(280, app->screen_height - 60, "X", "Favorite");
    draw_button_hint(400, app->screen_height - 60, "Y", "Search");
}

/* Render detail screen */
static void render_detail(app_t *app)
{
    clear_screen(COLOR_BG);

    /* Backdrop area */
    draw_rect(0, 0, app->screen_width, 400, COLOR_BG_DARK);

    /* Poster */
    draw_rect(60, 100, 250, 375, COLOR_HIGHLIGHT);

    /* Title and info */
    int info_x = 340;
    draw_text(info_x, 120, app->current_item.title[0] ? app->current_item.title : "Title",
              COLOR_TEXT, 3);

    /* Metadata */
    char meta[128];
    snprintf(meta, sizeof(meta), "%d • %s • %s",
             app->current_item.year ? app->current_item.year : 2024,
             "2h 15m", "HD");
    draw_text(info_x, 180, meta, COLOR_TEXT_DIM, 2);

    /* Description */
    draw_text(info_x, 240, "Lorem ipsum dolor sit amet, consectetur", COLOR_TEXT, 1);
    draw_text(info_x, 260, "adipiscing elit. Sed do eiusmod tempor", COLOR_TEXT, 1);
    draw_text(info_x, 280, "incididunt ut labore et dolore magna aliqua.", COLOR_TEXT, 1);

    /* Play button */
    draw_rect(info_x, 340, 200, 50, COLOR_PRIMARY);
    draw_text(info_x + 60, 352, "Play", COLOR_TEXT, 2);

    /* Favorite button */
    bool is_fav = config_is_favorite(app->current_item.id);
    draw_rect(info_x + 220, 340, 150, 50, is_fav ? COLOR_SECONDARY : COLOR_HIGHLIGHT);
    draw_text(info_x + 240, 352, is_fav ? "Favorited" : "Favorite", COLOR_TEXT, 1);

    /* Button hints */
    draw_button_hint(60, app->screen_height - 60, "A", "Play");
    draw_button_hint(160, app->screen_height - 60, "B", "Back");
    draw_button_hint(260, app->screen_height - 60, "X", "Toggle Favorite");
}

/* Render playback screen */
static void render_playback(app_t *app)
{
    /* Video frame would be rendered by video subsystem */
    /* Here we just draw the controls overlay */

    if (!app->show_controls) return;

    /* Semi-transparent overlay */
    draw_rect_alpha(0, 0, app->screen_width, 100, COLOR_OVERLAY);
    draw_rect_alpha(0, app->screen_height - 150, app->screen_width, 150, COLOR_OVERLAY);

    /* Title bar */
    draw_text(40, 30, app->current_item.title[0] ? app->current_item.title : "Playing",
              COLOR_TEXT, 2);

    /* Progress bar */
    int bar_y = app->screen_height - 100;
    uint32_t pos = video_get_position();
    uint32_t dur = video_get_duration();
    float progress = dur > 0 ? (float)pos / dur : 0;

    draw_progress_bar(60, bar_y, app->screen_width - 120, 8, progress,
                      COLOR_HIGHLIGHT, COLOR_PRIMARY);

    /* Time display */
    char time_str[64];
    snprintf(time_str, sizeof(time_str), "%02u:%02u / %02u:%02u",
             (pos / 60000), (pos / 1000) % 60,
             (dur / 60000), (dur / 1000) % 60);
    draw_text(60, bar_y + 20, time_str, COLOR_TEXT, 1);

    /* Playback state */
    const char *state = video_is_playing() ? "Playing" : "Paused";
    draw_text(app->screen_width / 2 - 40, bar_y + 20, state, COLOR_TEXT, 1);

    /* Volume */
    char vol_str[32];
    snprintf(vol_str, sizeof(vol_str), "Vol: %d%%", app->settings.volume);
    draw_text(app->screen_width - 200, bar_y + 20, vol_str, COLOR_TEXT, 1);

    /* Buffering indicator */
    if (video_is_buffering()) {
        draw_text_centered(app->screen_width / 2, app->screen_height / 2,
                          "Buffering...", COLOR_TEXT, 2);
    }

    /* Button hints */
    draw_button_hint(60, app->screen_height - 50, "A", "Play/Pause");
    draw_button_hint(200, app->screen_height - 50, "B", "Stop");
    draw_button_hint(320, app->screen_height - 50, "L/R", "Seek");
}

/* Render settings screen */
static void render_settings(app_t *app)
{
    clear_screen(COLOR_BG);

    /* Header */
    draw_rect(0, 0, app->screen_width, 80, COLOR_BG_DARK);
    draw_text(40, 25, "Settings", COLOR_PRIMARY, 3);

    /* Settings list */
    const char *setting_labels[] = {
        "Volume", "Video Quality", "Subtitles", "Subtitle Language",
        "Audio Language", "Auto-play Next", "Skip Intro", "HDR",
        "Parental Controls", "Clear History", "About"
    };

    int list_y = 120;
    int item_h = 60;

    for (int i = 0; i < 11; i++) {
        int y = list_y + i * item_h;
        if (y > (int)app->screen_height - 80) break;

        bool selected = (i == app->settings_index);
        uint32_t bg = selected ? COLOR_HIGHLIGHT : COLOR_BG;
        uint32_t text_color = selected ? COLOR_TEXT : COLOR_TEXT_DIM;

        draw_rect(60, y, app->screen_width - 120, item_h - 4, bg);
        draw_text(80, y + 18, setting_labels[i], text_color, 2);

        /* Value display */
        char value[32] = "";
        switch (i) {
            case 0:
                snprintf(value, sizeof(value), "%d%%", app->settings.volume);
                break;
            case 1: {
                const char *qualities[] = {"Auto", "SD", "HD", "FHD", "4K"};
                snprintf(value, sizeof(value), "%s", qualities[app->settings.video_quality]);
                break;
            }
            case 2:
                snprintf(value, sizeof(value), "%s", app->settings.show_subtitles ? "On" : "Off");
                break;
            case 5:
                snprintf(value, sizeof(value), "%s", app->settings.autoplay ? "On" : "Off");
                break;
            case 6:
                snprintf(value, sizeof(value), "%s", app->settings.skip_intro ? "On" : "Off");
                break;
            case 7:
                snprintf(value, sizeof(value), "%s", app->settings.enable_hdr ? "On" : "Off");
                break;
        }

        if (value[0]) {
            draw_text(app->screen_width - 300, y + 18, value, text_color, 2);
        }
    }

    /* Button hints */
    draw_button_hint(60, app->screen_height - 60, "D-Pad", "Navigate");
    draw_button_hint(220, app->screen_height - 60, "B", "Save & Exit");
}

/* Render error screen */
static void render_error(app_t *app)
{
    clear_screen(COLOR_BG_DARK);

    /* Error icon */
    draw_rect(app->screen_width / 2 - 40, app->screen_height / 2 - 120, 80, 80, COLOR_ERROR);
    draw_text(app->screen_width / 2 - 16, app->screen_height / 2 - 100, "!", COLOR_TEXT, 4);

    /* Error message */
    draw_text_centered(app->screen_width / 2, app->screen_height / 2,
                       app->error_msg[0] ? app->error_msg : "An error occurred",
                       COLOR_TEXT, 2);

    /* Options */
    draw_button_hint(app->screen_width / 2 - 100, app->screen_height / 2 + 80, "A", "Retry");
    draw_button_hint(app->screen_width / 2 + 20, app->screen_height / 2 + 80, "B", "Exit");
}

/* Main UI render function */
void ui_render(app_t *app)
{
    if (!app || !app->framebuffer) return;

    /* Update UI state */
    ui_state.framebuffer = app->framebuffer;
    ui_state.width = app->screen_width;
    ui_state.height = app->screen_height;
    ui_state.stride = app->screen_width;

    /* Render based on current state */
    switch (app->state) {
        case STATE_INIT:
        case STATE_SPLASH:
            render_splash(app);
            break;

        case STATE_NETWORK_INIT:
        case STATE_CONNECTING:
            render_network_init(app);
            break;

        case STATE_LOGIN:
            render_login(app);
            break;

        case STATE_PROFILE_SELECT:
            /* Similar to login */
            render_login(app);
            break;

        case STATE_MAIN_MENU:
            render_main_menu(app);
            break;

        case STATE_BROWSING:
        case STATE_SEARCH:
            render_browsing(app);
            break;

        case STATE_DETAIL:
            render_detail(app);
            break;

        case STATE_PLAYING:
        case STATE_LIVE_TV:
            render_playback(app);
            break;

        case STATE_CHANNELS:
            render_browsing(app);
            break;

        case STATE_SETTINGS:
        case STATE_ABOUT:
            render_settings(app);
            break;

        case STATE_ERROR:
            render_error(app);
            break;

        default:
            clear_screen(COLOR_BG);
            draw_text_centered(app->screen_width / 2, app->screen_height / 2,
                             "Unknown State", COLOR_ERROR, 2);
            break;
    }
}
