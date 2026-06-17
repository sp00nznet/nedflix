/*
 * Nedflix for Sega Dreamcast
 * UI rendering using KallistiOS PVR (PowerVR2)
 *
 * The Dreamcast's PowerVR2 GPU supports:
 * - 640x480 resolution (NTSC/PAL)
 * - Hardware texture mapping
 * - Alpha blending
 * - Z-buffering
 *
 * We use the pvr library for hardware-accelerated 2D rendering.
 */

#include "nedflix.h"
#include <string.h>
#include <stdio.h>

#include <dc/pvr.h>
#include <dc/video.h>
#include <dc/biosfont.h>

/* Screen dimensions */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* UI layout constants */
#define MARGIN_X        40
#define MARGIN_Y        40
#define HEADER_HEIGHT   60
#define FOOTER_HEIGHT   40
#define LIST_ITEM_HEIGHT 28
#define MAX_VISIBLE_ITEMS 12

/* Colors (ARGB format for PVR) */
#define COLOR_BACKGROUND    0xFF1A1A2E
#define COLOR_HEADER        0xFF16213E
#define COLOR_SELECTED      0xFF0F4C75
#define COLOR_TEXT          0xFFE0E0E0
#define COLOR_TEXT_DIM      0xFF808080
#define COLOR_ACCENT        0xFF00B4D8
#define COLOR_ERROR         0xFFFF4444
#define COLOR_SUCCESS       0xFF44FF44

/* UI state */
static struct {
    bool initialized;
    int list_scroll_offset;
    char status_message[128];
    uint32 status_color;
    uint32 status_time;
} g_ui;

/*
 * Draw a filled rectangle using PVR
 */
static void draw_rect(float x, float y, float w, float h, uint32 color)
{
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    /* Extract color components */
    float a = ((color >> 24) & 0xFF) / 255.0f;
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    /* Top-left */
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y;
    vert.z = 1.0f;
    vert.argb = PVR_PACK_COLOR(a, r, g, b);
    pvr_prim(&vert, sizeof(vert));

    /* Top-right */
    vert.x = x + w;
    vert.y = y;
    pvr_prim(&vert, sizeof(vert));

    /* Bottom-left */
    vert.x = x;
    vert.y = y + h;
    pvr_prim(&vert, sizeof(vert));

    /* Bottom-right */
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x + w;
    vert.y = y + h;
    pvr_prim(&vert, sizeof(vert));
}

/*
 * Draw text using BIOS font
 * Note: KOS provides a simple BIOS font for basic text rendering
 */
static void draw_text(float x, float y, uint32 color, const char *text)
{
    /* Use BIOS font drawing */
    /* Convert color to RGB for BIOS font */
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;

    bfont_set_foreground_color(PVR_PACK_COLOR(1.0f, r/255.0f, g/255.0f, b/255.0f));
    bfont_draw_str(vram_s + ((int)y * SCREEN_WIDTH + (int)x), SCREEN_WIDTH, 1, text);
}

/*
 * Draw text centered horizontally
 */
static void draw_text_centered(float y, uint32 color, const char *text)
{
    int len = strlen(text);
    float x = (SCREEN_WIDTH - len * 12) / 2;  /* BIOS font is ~12px wide */
    draw_text(x, y, color, text);
}

/*
 * Initialize UI subsystem
 */
int ui_init(void)
{
    LOG("Initializing UI...");

    memset(&g_ui, 0, sizeof(g_ui));

    /* PVR should already be initialized in main */

    /* Set video mode */
    vid_set_mode(DM_640x480_NTSC_IL, PM_RGB565);

    g_ui.initialized = true;
    LOG("UI initialized");
    return 0;
}

/*
 * Shutdown UI subsystem
 */
void ui_shutdown(void)
{
    g_ui.initialized = false;
    LOG("UI shutdown");
}

/*
 * Set status message
 */
void ui_set_status(const char *message, uint32 color)
{
    strncpy(g_ui.status_message, message, sizeof(g_ui.status_message) - 1);
    g_ui.status_color = color;
    g_ui.status_time = timer_ms_gettime64();
}

/*
 * Draw header bar
 */
static void draw_header(const char *title, const char *subtitle)
{
    /* Header background */
    draw_rect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_HEADER);

    /* Title */
    draw_text(MARGIN_X, 15, COLOR_TEXT, title);

    /* Subtitle (smaller, dimmed) */
    if (subtitle && strlen(subtitle) > 0) {
        draw_text(MARGIN_X, 38, COLOR_TEXT_DIM, subtitle);
    }

    /* Nedflix logo/text on right */
    draw_text(SCREEN_WIDTH - MARGIN_X - 72, 20, COLOR_ACCENT, "NEDFLIX");
}

/*
 * Draw footer bar with button hints
 */
static void draw_footer(const char *hints)
{
    int y = SCREEN_HEIGHT - FOOTER_HEIGHT;

    /* Footer background */
    draw_rect(0, y, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_HEADER);

    /* Button hints */
    draw_text(MARGIN_X, y + 12, COLOR_TEXT_DIM, hints);

    /* Status message (if recent) */
    if (g_ui.status_message[0] != '\0') {
        uint64 elapsed = timer_ms_gettime64() - g_ui.status_time;
        if (elapsed < 3000) {  /* Show for 3 seconds */
            draw_text(SCREEN_WIDTH - MARGIN_X - strlen(g_ui.status_message) * 12,
                     y + 12, g_ui.status_color, g_ui.status_message);
        } else {
            g_ui.status_message[0] = '\0';
        }
    }
}

/*
 * Draw loading screen
 */
void ui_draw_loading(const char *message)
{
    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_TR_POLY);

    /* Background */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);

    /* Loading message centered */
    draw_text_centered(SCREEN_HEIGHT / 2 - 10, COLOR_TEXT, message ? message : "Loading...");

    /* Animated dots (simple animation based on time) */
    int dots = (timer_ms_gettime64() / 500) % 4;
    char dot_str[5] = "";
    for (int i = 0; i < dots; i++) {
        dot_str[i] = '.';
    }
    draw_text_centered(SCREEN_HEIGHT / 2 + 20, COLOR_ACCENT, dot_str);

    pvr_list_finish();
    pvr_scene_finish();
}

/*
 * Draw error screen
 */
void ui_draw_error(const char *message)
{
    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_TR_POLY);

    /* Background */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);

    /* Error title */
    draw_text_centered(SCREEN_HEIGHT / 2 - 40, COLOR_ERROR, "ERROR");

    /* Error message */
    draw_text_centered(SCREEN_HEIGHT / 2, COLOR_TEXT, message ? message : "An error occurred");

    /* Hint */
    draw_text_centered(SCREEN_HEIGHT / 2 + 50, COLOR_TEXT_DIM, "Press START to continue");

    pvr_list_finish();
    pvr_scene_finish();
}

/*
 * Draw login screen
 */
void ui_draw_login(int selected_field, const char *username, const char *password,
                   bool connecting)
{
    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_TR_POLY);

    /* Background */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);

    draw_header("Login", "Enter your Nedflix credentials");

    int center_x = SCREEN_WIDTH / 2;
    int start_y = 150;

    /* Username field */
    uint32 user_color = (selected_field == 0) ? COLOR_SELECTED : COLOR_HEADER;
    draw_rect(center_x - 150, start_y, 300, 40, user_color);
    draw_text(center_x - 140, start_y + 5, COLOR_TEXT_DIM, "Username:");
    draw_text(center_x - 140, start_y + 22, COLOR_TEXT,
              username && strlen(username) > 0 ? username : "_______________");

    /* Password field */
    uint32 pass_color = (selected_field == 1) ? COLOR_SELECTED : COLOR_HEADER;
    draw_rect(center_x - 150, start_y + 60, 300, 40, pass_color);
    draw_text(center_x - 140, start_y + 65, COLOR_TEXT_DIM, "Password:");

    /* Mask password */
    char masked[32];
    int plen = password ? strlen(password) : 0;
    if (plen > 0) {
        for (int i = 0; i < plen && i < 31; i++) masked[i] = '*';
        masked[plen < 31 ? plen : 31] = '\0';
    } else {
        strcpy(masked, "_______________");
    }
    draw_text(center_x - 140, start_y + 82, COLOR_TEXT, masked);

    /* Login button */
    uint32 btn_color = (selected_field == 2) ? COLOR_ACCENT : COLOR_HEADER;
    draw_rect(center_x - 60, start_y + 130, 120, 35, btn_color);
    draw_text(center_x - 30, start_y + 140, COLOR_TEXT,
              connecting ? "Connecting..." : "LOGIN");

    draw_footer("A: Select  B: Back  START: Login");

    pvr_list_finish();
    pvr_scene_finish();
}

/*
 * Draw main menu
 */
void ui_draw_main_menu(int selected, const char *username)
{
    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_TR_POLY);

    /* Background */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);

    char subtitle[64];
    snprintf(subtitle, sizeof(subtitle), "Welcome, %s", username ? username : "Guest");
    draw_header("Nedflix", subtitle);

    /* Menu items */
    const char *items[] = {
        "Browse Media",
        "Search",
        "Recent",
        "Settings",
        "Logout"
    };
    int item_count = 5;

    int start_y = 120;
    int item_height = 50;

    for (int i = 0; i < item_count; i++) {
        int y = start_y + i * item_height;
        uint32 bg_color = (i == selected) ? COLOR_SELECTED : COLOR_HEADER;

        draw_rect(MARGIN_X, y, SCREEN_WIDTH - MARGIN_X * 2, item_height - 5, bg_color);
        draw_text(MARGIN_X + 20, y + 15, COLOR_TEXT, items[i]);

        /* Selection indicator */
        if (i == selected) {
            draw_text(MARGIN_X + 5, y + 15, COLOR_ACCENT, ">");
        }
    }

    draw_footer("A: Select  B: Back");

    pvr_list_finish();
    pvr_scene_finish();
}

/*
 * Draw media browser
 */
void ui_draw_browser(const media_list_t *list, const char *current_path)
{
    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_TR_POLY);

    /* Background */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);

    draw_header("Browse", current_path ? current_path : "/");

    if (!list || list->count == 0) {
        draw_text_centered(SCREEN_HEIGHT / 2, COLOR_TEXT_DIM, "No items found");
    } else {
        int content_y = HEADER_HEIGHT + 10;
        int content_height = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20;
        int visible_items = content_height / LIST_ITEM_HEIGHT;

        /* Adjust scroll offset */
        int scroll = list->scroll_offset;
        if (list->selected_index < scroll) {
            scroll = list->selected_index;
        } else if (list->selected_index >= scroll + visible_items) {
            scroll = list->selected_index - visible_items + 1;
        }

        for (int i = 0; i < visible_items && (scroll + i) < list->count; i++) {
            int idx = scroll + i;
            const media_item_t *item = &list->items[idx];
            int y = content_y + i * LIST_ITEM_HEIGHT;

            /* Item background */
            uint32 bg_color = (idx == list->selected_index) ? COLOR_SELECTED : 0x00000000;
            if (bg_color != 0) {
                draw_rect(MARGIN_X, y, SCREEN_WIDTH - MARGIN_X * 2, LIST_ITEM_HEIGHT - 2, bg_color);
            }

            /* Icon based on type */
            const char *icon;
            switch (item->type) {
                case MEDIA_TYPE_DIRECTORY: icon = "[D]"; break;
                case MEDIA_TYPE_VIDEO:     icon = "[V]"; break;
                case MEDIA_TYPE_AUDIO:     icon = "[A]"; break;
                default:                   icon = "[?]"; break;
            }
            draw_text(MARGIN_X + 5, y + 5, COLOR_ACCENT, icon);

            /* Item name (truncate if too long) */
            char name[40];
            strncpy(name, item->name, 39);
            name[39] = '\0';
            if (strlen(item->name) > 39) {
                strcpy(name + 36, "...");
            }
            draw_text(MARGIN_X + 50, y + 5, COLOR_TEXT, name);
        }

        /* Scrollbar if needed */
        if (list->count > visible_items) {
            int sb_height = content_height;
            int sb_y = content_y;
            int thumb_height = (visible_items * sb_height) / list->count;
            int thumb_y = sb_y + (scroll * sb_height) / list->count;

            draw_rect(SCREEN_WIDTH - MARGIN_X + 5, sb_y, 5, sb_height, COLOR_HEADER);
            draw_rect(SCREEN_WIDTH - MARGIN_X + 5, thumb_y, 5, thumb_height, COLOR_ACCENT);
        }
    }

    draw_footer("A: Open  B: Back  Y: Parent Dir");

    pvr_list_finish();
    pvr_scene_finish();
}

/*
 * Draw playback screen
 */
void ui_draw_playback(const char *title, double position, double duration,
                      bool paused, int volume)
{
    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_TR_POLY);

    /* Background (dark for video) */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF000000);

    /* Title at top */
    draw_rect(0, 0, SCREEN_WIDTH, 40, 0xC0000000);
    draw_text(MARGIN_X, 10, COLOR_TEXT, title ? title : "Now Playing");

    /* Playback controls at bottom */
    draw_rect(0, SCREEN_HEIGHT - 80, SCREEN_WIDTH, 80, 0xC0000000);

    /* Progress bar */
    int bar_y = SCREEN_HEIGHT - 60;
    int bar_width = SCREEN_WIDTH - MARGIN_X * 2;
    draw_rect(MARGIN_X, bar_y, bar_width, 8, COLOR_HEADER);

    if (duration > 0) {
        int progress_width = (int)((position / duration) * bar_width);
        draw_rect(MARGIN_X, bar_y, progress_width, 8, COLOR_ACCENT);
    }

    /* Time display */
    char time_str[32];
    int pos_min = (int)(position / 60);
    int pos_sec = (int)position % 60;
    int dur_min = (int)(duration / 60);
    int dur_sec = (int)duration % 60;
    snprintf(time_str, sizeof(time_str), "%02d:%02d / %02d:%02d",
             pos_min, pos_sec, dur_min, dur_sec);
    draw_text(MARGIN_X, bar_y + 15, COLOR_TEXT, time_str);

    /* Pause indicator */
    if (paused) {
        draw_text_centered(SCREEN_HEIGHT / 2, COLOR_ACCENT, "|| PAUSED ||");
    }

    /* Volume indicator */
    char vol_str[16];
    snprintf(vol_str, sizeof(vol_str), "Vol: %d%%", volume);
    draw_text(SCREEN_WIDTH - MARGIN_X - 80, bar_y + 15, COLOR_TEXT_DIM, vol_str);

    /* Control hints */
    draw_text(MARGIN_X, SCREEN_HEIGHT - 25, COLOR_TEXT_DIM,
              "A: Play/Pause  B: Stop  L/R: Seek  Triggers: Volume");

    pvr_list_finish();
    pvr_scene_finish();
}

/*
 * Draw settings screen
 */
void ui_draw_settings(const user_settings_t *settings, int selected)
{
    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_TR_POLY);

    /* Background */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);

    draw_header("Settings", "Configure Nedflix");

    int start_y = 100;
    int item_height = 40;

    struct {
        const char *label;
        char value[32];
    } items[] = {
        {"Server URL", ""},
        {"Volume", ""},
        {"Autoplay", ""},
        {"Subtitles", ""},
        {"Theme", ""},
        {"Save Settings", ""}
    };
    int item_count = 6;

    /* Fill in values */
    if (settings) {
        snprintf(items[0].value, 32, "%s", settings->server_url);
        snprintf(items[1].value, 32, "%d%%", settings->volume);
        snprintf(items[2].value, 32, "%s", settings->autoplay ? "On" : "Off");
        snprintf(items[3].value, 32, "%s", settings->show_subtitles ? "On" : "Off");
        snprintf(items[4].value, 32, "%s", settings->theme == 0 ? "Dark" : "Light");
    }

    for (int i = 0; i < item_count; i++) {
        int y = start_y + i * item_height;
        uint32 bg_color = (i == selected) ? COLOR_SELECTED : COLOR_HEADER;

        draw_rect(MARGIN_X, y, SCREEN_WIDTH - MARGIN_X * 2, item_height - 5, bg_color);
        draw_text(MARGIN_X + 20, y + 10, COLOR_TEXT, items[i].label);

        if (strlen(items[i].value) > 0) {
            /* Truncate value if too long */
            char display_val[20];
            strncpy(display_val, items[i].value, 19);
            display_val[19] = '\0';
            draw_text(SCREEN_WIDTH - MARGIN_X - 150, y + 10, COLOR_ACCENT, display_val);
        }
    }

    draw_footer("A: Edit  B: Back  L/R: Adjust Value");

    pvr_list_finish();
    pvr_scene_finish();
}
