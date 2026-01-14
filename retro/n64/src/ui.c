/*
 * Nedflix N64 - UI rendering using libdragon
 */

#include "nedflix.h"
#include <string.h>

static sprite_t *font_sprite = NULL;

int ui_init(void)
{
    LOG("UI init");
    return 0;
}

void ui_shutdown(void)
{
    if (font_sprite) {
        free(font_sprite);
        font_sprite = NULL;
    }
}

void ui_begin_frame(void)
{
    g_app.disp = display_get();
    rdpq_attach(g_app.disp, NULL);
}

void ui_end_frame(void)
{
    rdpq_detach_show();
}

void ui_draw_background(void)
{
    rdpq_set_mode_fill(COLOR_DARK_BG);
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void ui_draw_text(int x, int y, const char *text, uint32_t color)
{
    graphics_set_color(color, 0);
    graphics_draw_text(g_app.disp, x, y, text);
}

void ui_draw_text_centered(int y, const char *text, uint32_t color)
{
    int len = strlen(text);
    int x = (SCREEN_WIDTH - len * 8) / 2;
    ui_draw_text(x, y, text, color);
}

void ui_draw_header(const char *title)
{
    rdpq_set_mode_fill(COLOR_RED);
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, 24);

    ui_draw_text_centered(8, title, COLOR_WHITE);
}

void ui_draw_menu(const char **options, int count, int selected)
{
    int y = 40;
    int item_height = 20;

    for (int i = 0; i < count; i++) {
        if (i == selected) {
            rdpq_set_mode_fill(COLOR_SELECTED);
            rdpq_fill_rectangle(10, y - 2, SCREEN_WIDTH - 10, y + item_height - 2);
        }

        ui_draw_text(20, y, options[i], i == selected ? COLOR_WHITE : COLOR_TEXT);
        y += item_height;
    }
}

void ui_draw_loading(const char *message)
{
    ui_draw_text_centered(100, "NEDFLIX", COLOR_RED);
    ui_draw_text_centered(130, message, COLOR_TEXT);

    static int dots = 0;
    dots = (dots + 1) % 60;

    char loading[8] = "...";
    loading[dots / 20] = '\0';
    ui_draw_text_centered(150, loading, COLOR_TEXT_DIM);
}

void ui_draw_error(const char *message)
{
    ui_draw_text_centered(60, "ERROR", COLOR_RED);

    char line[64];
    const char *p = message;
    int y = 100;

    while (*p && y < 180) {
        const char *nl = strchr(p, '\n');
        int len = nl ? (int)(nl - p) : (int)strlen(p);
        if (len > 38) len = 38;

        strncpy(line, p, len);
        line[len] = '\0';

        ui_draw_text_centered(y, line, COLOR_TEXT);
        y += 16;

        if (nl) p = nl + 1;
        else break;
    }
}

void ui_draw_media_list(const media_list_t *list)
{
    if (list->count == 0) {
        ui_draw_text_centered(100, "No items", COLOR_TEXT_DIM);
        return;
    }

    int y = 32;
    int item_height = 18;

    for (int i = 0; i < MAX_ITEMS_VISIBLE && i + list->scroll_offset < list->count; i++) {
        int idx = i + list->scroll_offset;
        const media_item_t *item = &list->items[idx];

        if (idx == list->selected_index) {
            rdpq_set_mode_fill(COLOR_SELECTED);
            rdpq_fill_rectangle(5, y - 1, SCREEN_WIDTH - 5, y + item_height - 1);
        }

        char display[40];
        if (item->is_directory) {
            snprintf(display, sizeof(display), "[%s]", item->name);
        } else {
            snprintf(display, sizeof(display), "%s", item->name);
        }
        display[38] = '\0';

        ui_draw_text(10, y, display, idx == list->selected_index ? COLOR_WHITE : COLOR_TEXT);
        y += item_height;
    }

    if (list->count > MAX_ITEMS_VISIBLE) {
        char scroll_info[16];
        snprintf(scroll_info, sizeof(scroll_info), "%d/%d",
                 list->selected_index + 1, list->count);
        ui_draw_text(SCREEN_WIDTH - 50, SCREEN_HEIGHT - 20, scroll_info, COLOR_TEXT_DIM);
    }

    ui_draw_text(10, SCREEN_HEIGHT - 12, "A:Sel B:Back L/R:Lib", COLOR_TEXT_DIM);
}

void ui_draw_playback(const playback_t *pb)
{
    ui_draw_text_centered(40, "NOW PLAYING", COLOR_RED);

    char title_trunc[36];
    strncpy(title_trunc, pb->title, 35);
    title_trunc[35] = '\0';
    ui_draw_text_centered(70, title_trunc, COLOR_WHITE);

    uint32_t pos_sec = pb->position_ms / 1000;
    uint32_t dur_sec = pb->duration_ms / 1000;

    char time_str[32];
    snprintf(time_str, sizeof(time_str), "%02u:%02u / %02u:%02u",
             pos_sec / 60, pos_sec % 60, dur_sec / 60, dur_sec % 60);
    ui_draw_text_centered(100, time_str, COLOR_TEXT);

    int bar_width = 200;
    int bar_x = (SCREEN_WIDTH - bar_width) / 2;
    int bar_y = 125;

    rdpq_set_mode_fill(COLOR_TEXT_DIM);
    rdpq_fill_rectangle(bar_x, bar_y, bar_x + bar_width, bar_y + 8);

    if (pb->duration_ms > 0) {
        int progress = (int)((uint64_t)pb->position_ms * bar_width / pb->duration_ms);
        rdpq_set_mode_fill(COLOR_RED);
        rdpq_fill_rectangle(bar_x, bar_y, bar_x + progress, bar_y + 8);
    }

    const char *status = pb->paused ? "PAUSED" : "PLAYING";
    ui_draw_text_centered(150, status, pb->paused ? COLOR_TEXT_DIM : COLOR_WHITE);

    char vol_str[16];
    snprintf(vol_str, sizeof(vol_str), "Vol: %d%%", pb->volume);
    ui_draw_text_centered(175, vol_str, COLOR_TEXT);

    ui_draw_text_centered(210, "A:Pause B:Stop", COLOR_TEXT_DIM);
    ui_draw_text_centered(225, "C-Up/Down: Volume", COLOR_TEXT_DIM);
}
