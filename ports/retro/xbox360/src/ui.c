/*
 * Nedflix for Xbox 360
 * UI rendering using Xenos framebuffer
 *
 * TECHNICAL DEMO / NOVELTY PORT
 *
 * Uses the Xenos GPU in framebuffer mode for simplicity.
 * A full implementation would use proper shaders.
 */

#include "nedflix.h"

/* Simple 8x8 bitmap font */
static const unsigned char g_font_data[] = {
    /* Space (32) - Z (90) characters */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* Space */
    0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00, /* ! */
    0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00, /* " */
    0x6C,0xFE,0x6C,0x6C,0xFE,0x6C,0x00,0x00, /* # */
    0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0x00, /* $ */
    0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00,0x00, /* % */
    0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00, /* & */
    0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00, /* ' */
    0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00, /* ( */
    0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00, /* ) */
    0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00, /* * */
    0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00, /* + */
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30, /* , */
    0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00, /* - */
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00, /* . */
    0x06,0x0C,0x18,0x30,0x60,0xC0,0x00,0x00, /* / */
    0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00, /* 0 */
    0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00, /* 1 */
    0x7C,0xC6,0x06,0x1C,0x70,0xC0,0xFE,0x00, /* 2 */
    0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00, /* 3 */
    0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00, /* 4 */
    0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00, /* 5 */
    0x38,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00, /* 6 */
    0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00, /* 7 */
    0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00, /* 8 */
    0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00, /* 9 */
    0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00, /* : */
    0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30, /* ; */
    0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00, /* < */
    0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00, /* = */
    0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00, /* > */
    0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00, /* ? */
    0x7C,0xC6,0xDE,0xDE,0xDC,0xC0,0x7C,0x00, /* @ */
    0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00, /* A */
    0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00, /* B */
    0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00, /* C */
    0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00, /* D */
    0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xFE,0x00, /* E */
    0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xC0,0x00, /* F */
    0x7C,0xC6,0xC0,0xCE,0xC6,0xC6,0x7E,0x00, /* G */
    0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00, /* H */
    0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00, /* I */
    0x06,0x06,0x06,0x06,0x06,0xC6,0x7C,0x00, /* J */
    0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00, /* K */
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00, /* L */
    0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00, /* M */
    0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00, /* N */
    0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00, /* O */
    0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00, /* P */
    0x7C,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x06, /* Q */
    0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6,0x00, /* R */
    0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00, /* S */
    0xFE,0x18,0x18,0x18,0x18,0x18,0x18,0x00, /* T */
    0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00, /* U */
    0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00, /* V */
    0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00, /* W */
    0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00, /* X */
    0xC6,0xC6,0x6C,0x38,0x18,0x18,0x18,0x00, /* Y */
    0xFE,0x06,0x0C,0x18,0x30,0x60,0xFE,0x00, /* Z */
    /* Lowercase a-z follow same pattern */
    0x00,0x00,0x7C,0x06,0x7E,0xC6,0x7E,0x00, /* a */
    0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xFC,0x00, /* b */
    0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00, /* c */
    0x06,0x06,0x7E,0xC6,0xC6,0xC6,0x7E,0x00, /* d */
    0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00, /* e */
    0x1C,0x36,0x30,0x78,0x30,0x30,0x30,0x00, /* f */
    0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x7C, /* g */
    0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x00, /* h */
    0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00, /* i */
    0x06,0x00,0x06,0x06,0x06,0x06,0xC6,0x7C, /* j */
    0xC0,0xC0,0xCC,0xD8,0xF0,0xD8,0xCC,0x00, /* k */
    0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00, /* l */
    0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00, /* m */
    0x00,0x00,0xFC,0xC6,0xC6,0xC6,0xC6,0x00, /* n */
    0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00, /* o */
    0x00,0x00,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0, /* p */
    0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x06, /* q */
    0x00,0x00,0xDC,0xE6,0xC0,0xC0,0xC0,0x00, /* r */
    0x00,0x00,0x7E,0xC0,0x7C,0x06,0xFC,0x00, /* s */
    0x30,0x30,0x7C,0x30,0x30,0x36,0x1C,0x00, /* t */
    0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x7E,0x00, /* u */
    0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x00, /* v */
    0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00, /* w */
    0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00, /* x */
    0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x06,0x7C, /* y */
    0x00,0x00,0xFE,0x0C,0x38,0x60,0xFE,0x00, /* z */
};

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 8
#define FONT_SCALE  2

/*
 * Initialize UI
 */
int ui_init(void)
{
    /* Initialize Xenos GPU */
    xenos_init(VIDEO_MODE_AUTO);

    g_app.xe = &__xe;

    /* Get framebuffer surface */
    g_app.fb = xe_get_framebuffer_surface(g_app.xe);
    if (!g_app.fb) {
        LOG_ERROR("Failed to get framebuffer surface");
        return -1;
    }

    /* Lock framebuffer for direct access */
    g_app.framebuffer = (uint32_t *)xe_surface_lock(g_app.xe, g_app.fb, 0, 0,
                                                      XE_LOCK_WRITE);
    if (!g_app.framebuffer) {
        LOG_ERROR("Failed to lock framebuffer");
        return -1;
    }

    LOG("UI initialized: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
    return 0;
}

/*
 * Shutdown UI
 */
void ui_shutdown(void)
{
    if (g_app.fb && g_app.framebuffer) {
        xe_surface_unlock(g_app.xe, g_app.fb);
        g_app.framebuffer = NULL;
    }
}

/*
 * Begin frame
 */
void ui_begin_frame(void)
{
    /* Nothing special needed for software rendering */
}

/*
 * End frame - flip buffers
 */
void ui_end_frame(void)
{
    /* Unlock and present */
    xe_surface_unlock(g_app.xe, g_app.fb);

    /* Resolve to display */
    xe_resolve(g_app.xe);

    /* Re-lock for next frame */
    g_app.framebuffer = (uint32_t *)xe_surface_lock(g_app.xe, g_app.fb, 0, 0,
                                                      XE_LOCK_WRITE);
}

/*
 * Clear screen
 */
void ui_clear(uint32_t color)
{
    if (!g_app.framebuffer) return;

    int pitch = g_app.fb->wpitch / 4;

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            g_app.framebuffer[y * pitch + x] = color;
        }
    }
}

/*
 * Draw filled rectangle
 */
void ui_draw_rect(int x, int y, int width, int height, uint32_t color)
{
    if (!g_app.framebuffer) return;

    int pitch = g_app.fb->wpitch / 4;

    /* Clip to screen */
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > SCREEN_WIDTH) width = SCREEN_WIDTH - x;
    if (y + height > SCREEN_HEIGHT) height = SCREEN_HEIGHT - y;

    for (int py = y; py < y + height; py++) {
        for (int px = x; px < x + width; px++) {
            g_app.framebuffer[py * pitch + px] = color;
        }
    }
}

/*
 * Draw text at position
 */
void ui_draw_text(int x, int y, const char *text, uint32_t color)
{
    if (!g_app.framebuffer || !text) return;

    int pitch = g_app.fb->wpitch / 4;
    int cursor_x = x;

    while (*text) {
        unsigned char c = *text++;

        if (c == '\n') {
            cursor_x = x;
            y += CHAR_HEIGHT * FONT_SCALE + 2;
            continue;
        }

        /* Map character to font index */
        int font_idx = -1;
        if (c >= 32 && c <= 90) {
            font_idx = c - 32;
        } else if (c >= 'a' && c <= 'z') {
            font_idx = 59 + (c - 'a');  /* After Z in font data */
        } else {
            font_idx = 0;  /* Space for unknown */
        }

        if (font_idx >= 0 && font_idx < 85) {
            const unsigned char *glyph = &g_font_data[font_idx * 8];

            for (int row = 0; row < CHAR_HEIGHT; row++) {
                unsigned char bits = glyph[row];
                for (int col = 0; col < CHAR_WIDTH; col++) {
                    if (bits & (0x80 >> col)) {
                        /* Draw scaled pixel */
                        for (int sy = 0; sy < FONT_SCALE; sy++) {
                            for (int sx = 0; sx < FONT_SCALE; sx++) {
                                int px = cursor_x + col * FONT_SCALE + sx;
                                int py = y + row * FONT_SCALE + sy;
                                if (px >= 0 && px < SCREEN_WIDTH &&
                                    py >= 0 && py < SCREEN_HEIGHT) {
                                    g_app.framebuffer[py * pitch + px] = color;
                                }
                            }
                        }
                    }
                }
            }
        }

        cursor_x += CHAR_WIDTH * FONT_SCALE;
    }
}

/*
 * Draw centered text
 */
void ui_draw_text_centered(int y, const char *text, uint32_t color)
{
    if (!text) return;
    int width = strlen(text) * CHAR_WIDTH * FONT_SCALE;
    int x = (SCREEN_WIDTH - width) / 2;
    ui_draw_text(x, y, text, color);
}

/*
 * Draw header bar
 */
void ui_draw_header(const char *title)
{
    ui_draw_rect(0, 0, SCREEN_WIDTH, 60, COLOR_RED);
    ui_draw_text(30, 20, title, COLOR_WHITE);

    char version[32];
    snprintf(version, sizeof(version), "v%s", NEDFLIX_VERSION);
    int vw = strlen(version) * CHAR_WIDTH * FONT_SCALE;
    ui_draw_text(SCREEN_WIDTH - vw - 30, 20, version, COLOR_WHITE);
}

/*
 * Draw menu list
 */
void ui_draw_menu(const char **items, int count, int selected)
{
    int start_y = 100;
    int item_height = CHAR_HEIGHT * FONT_SCALE + 16;

    for (int i = 0; i < count; i++) {
        int y = start_y + i * item_height;

        if (i == selected) {
            ui_draw_rect(20, y - 6, SCREEN_WIDTH - 40, item_height, COLOR_SELECTED);
            ui_draw_text(30, y, ">", COLOR_RED);
        }

        ui_draw_text(60, y, items[i], i == selected ? COLOR_WHITE : COLOR_TEXT);
    }
}

/*
 * Draw file list
 */
void ui_draw_file_list(media_list_t *list)
{
    if (!list) return;

    int start_y = 80;
    int item_height = CHAR_HEIGHT * FONT_SCALE + 10;
    int visible = MIN(MAX_ITEMS_VISIBLE, list->count - list->scroll_offset);

    for (int i = 0; i < visible; i++) {
        int idx = list->scroll_offset + i;
        if (idx >= list->count) break;

        media_item_t *item = &list->items[idx];
        int y = start_y + i * item_height;

        if (idx == list->selected_index) {
            ui_draw_rect(20, y - 4, SCREEN_WIDTH - 40, item_height, COLOR_SELECTED);
        }

        /* Icon */
        const char *icon = item->is_directory ? "[D]" : "[A]";
        if (item->type == MEDIA_TYPE_VIDEO) icon = "[V]";
        ui_draw_text(30, y, icon, COLOR_TEXT_DIM);

        /* Name */
        ui_draw_text(80, y, item->name,
                     idx == list->selected_index ? COLOR_WHITE : COLOR_TEXT);
    }

    /* Scroll indicators */
    if (list->scroll_offset > 0) {
        ui_draw_text_centered(65, "^ More ^", COLOR_TEXT_DIM);
    }
    if (list->scroll_offset + MAX_ITEMS_VISIBLE < list->count) {
        ui_draw_text_centered(SCREEN_HEIGHT - 60, "v More v", COLOR_TEXT_DIM);
    }

    /* Item count */
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d/%d",
             list->selected_index + 1, list->count);
    int cw = strlen(count_str) * CHAR_WIDTH * FONT_SCALE;
    ui_draw_text(SCREEN_WIDTH - cw - 30, SCREEN_HEIGHT - 60, count_str, COLOR_TEXT_DIM);
}

/*
 * Draw progress bar
 */
void ui_draw_progress_bar(int x, int y, int width, int height, float progress,
                          uint32_t fg, uint32_t bg)
{
    ui_draw_rect(x, y, width, height, bg);

    int fill = (int)(width * CLAMP(progress, 0.0f, 1.0f));
    if (fill > 0) {
        ui_draw_rect(x, y, fill, height, fg);
    }
}

/*
 * Draw playback HUD
 */
void ui_draw_playback_hud(playback_t *state)
{
    if (!state) return;

    /* Background panel */
    ui_draw_rect(0, SCREEN_HEIGHT - 140, SCREEN_WIDTH, 140, COLOR_MENU_BG);

    /* Title */
    ui_draw_text(30, SCREEN_HEIGHT - 130, "Now Playing:", COLOR_TEXT_DIM);
    ui_draw_text(30, SCREEN_HEIGHT - 100, state->title, COLOR_WHITE);

    /* Status */
    const char *status = state->paused ? "PAUSED" : (state->playing ? "PLAYING" : "STOPPED");
    ui_draw_text(SCREEN_WIDTH - 150, SCREEN_HEIGHT - 130, status, COLOR_RED);

    /* Progress bar */
    float progress = (state->duration > 0) ? (state->position / state->duration) : 0;
    ui_draw_progress_bar(30, SCREEN_HEIGHT - 65, SCREEN_WIDTH - 60, 12,
                         progress, COLOR_RED, COLOR_DARK_BG);

    /* Time */
    char time_str[64];
    int cur_m = (int)state->position / 60;
    int cur_s = (int)state->position % 60;
    int dur_m = (int)state->duration / 60;
    int dur_s = (int)state->duration % 60;
    snprintf(time_str, sizeof(time_str), "%02d:%02d / %02d:%02d",
             cur_m, cur_s, dur_m, dur_s);
    ui_draw_text(30, SCREEN_HEIGHT - 45, time_str, COLOR_TEXT);

    /* Volume */
    char vol_str[32];
    snprintf(vol_str, sizeof(vol_str), "Vol: %d%%", state->volume);
    int vw = strlen(vol_str) * CHAR_WIDTH * FONT_SCALE;
    ui_draw_text(SCREEN_WIDTH - vw - 30, SCREEN_HEIGHT - 45, vol_str, COLOR_TEXT);

    /* Controls */
    ui_draw_text_centered(SCREEN_HEIGHT - 20,
                          "A: Play/Pause   B: Stop   D-Pad: Volume   Triggers: Seek",
                          COLOR_TEXT_DIM);
}

/*
 * Draw loading screen
 */
void ui_draw_loading(const char *message)
{
    ui_draw_text_centered(SCREEN_HEIGHT / 2 - 30, "NEDFLIX", COLOR_RED);
    ui_draw_text_centered(SCREEN_HEIGHT / 2 + 20, message, COLOR_TEXT);

    /* Animated spinner */
    static int frame = 0;
    const char *spinner = "|/-\\";
    char spin[2] = { spinner[(frame++ / 10) % 4], '\0' };
    ui_draw_text_centered(SCREEN_HEIGHT / 2 + 60, spin, COLOR_TEXT_DIM);
}

/*
 * Draw error screen
 */
void ui_draw_error(const char *message)
{
    ui_draw_rect(0, 0, SCREEN_WIDTH, 60, COLOR_RED);
    ui_draw_text(30, 20, "Error", COLOR_WHITE);

    ui_draw_text_centered(SCREEN_HEIGHT / 2, message, COLOR_TEXT);
}
