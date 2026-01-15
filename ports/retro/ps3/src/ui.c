/*
 * Nedflix PS3 - UI rendering using RSX
 * Technical demo implementation
 */

#include "nedflix.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <rsx/rsx.h>
#include <rsx/gcm_sys.h>
#include <sysutil/video.h>

/* RSX context and buffers */
static gcmContextData *rsx_context = NULL;
static u32 *framebuffer[2] = { NULL, NULL };
static u32 framebuffer_offset[2];
static u32 depth_offset;
static int current_buffer = 0;

static u32 screen_width = SCREEN_WIDTH;
static u32 screen_height = SCREEN_HEIGHT;
static u32 color_pitch;
static u32 depth_pitch;

/* Simple font rendering (bitmap font) */
#define FONT_CHAR_W 8
#define FONT_CHAR_H 16

/* Initialize RSX graphics */
int ui_init(void)
{
    printf("Initializing RSX graphics...\n");

    /* Get video state */
    videoState state;
    videoGetState(0, 0, &state);

    /* Get resolution info */
    videoResolution res;
    videoGetResolution(state.displayMode.resolution, &res);

    screen_width = res.width;
    screen_height = res.height;

    printf("Display: %dx%d\n", screen_width, screen_height);

    /* Configure video output */
    videoConfiguration vconfig;
    memset(&vconfig, 0, sizeof(vconfig));
    vconfig.resolution = state.displayMode.resolution;
    vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
    vconfig.pitch = screen_width * sizeof(u32);
    vconfig.aspect = state.displayMode.aspect;

    videoConfigure(0, &vconfig, NULL, 0);
    videoGetState(0, 0, &state);

    /* Initialize RSX */
    void *host_addr = memalign(1024*1024, 1024*1024);
    if (!host_addr) {
        printf("Failed to allocate RSX host memory\n");
        return -1;
    }

    rsx_context = rsxInit(0x10000, 1024*1024, host_addr);
    if (!rsx_context) {
        printf("Failed to initialize RSX\n");
        free(host_addr);
        return -1;
    }

    /* Allocate framebuffers */
    color_pitch = screen_width * sizeof(u32);
    depth_pitch = screen_width * sizeof(u32);

    u32 color_size = color_pitch * screen_height;
    u32 depth_size = depth_pitch * screen_height;

    for (int i = 0; i < 2; i++) {
        framebuffer[i] = rsxMemalign(64, color_size);
        if (!framebuffer[i]) {
            printf("Failed to allocate framebuffer %d\n", i);
            return -1;
        }
        rsxAddressToOffset(framebuffer[i], &framebuffer_offset[i]);

        /* Register for display */
        gcmSetDisplayBuffer(i, framebuffer_offset[i], color_pitch, screen_width, screen_height);
    }

    /* Allocate depth buffer */
    void *depth_buffer = rsxMemalign(64, depth_size);
    if (!depth_buffer) {
        printf("Failed to allocate depth buffer\n");
        return -1;
    }
    rsxAddressToOffset(depth_buffer, &depth_offset);

    printf("RSX initialized successfully\n");

    /* Store context for app */
    g_app.gcm_context = rsx_context;
    g_app.video_buffer = framebuffer[0];

    return 0;
}

/* Shutdown graphics */
void ui_shutdown(void)
{
    printf("Shutting down RSX...\n");

    rsxFinish(rsx_context, 1);

    for (int i = 0; i < 2; i++) {
        if (framebuffer[i]) {
            rsxFree(framebuffer[i]);
            framebuffer[i] = NULL;
        }
    }
}

/* Wait for RSX to finish */
static void wait_rsx_idle(void)
{
    rsxSetWriteBackendLabel(rsx_context, GCM_LABEL_INDEX, current_buffer);
    rsxFlushBuffer(rsx_context);

    while (*(vu32*)gcmGetLabelAddress(GCM_LABEL_INDEX) != current_buffer) {
        usleep(30);
    }

    while (gcmGetFlipStatus() != 0) {
        usleep(200);
    }
    gcmResetFlipStatus();
}

/* Begin frame rendering */
void ui_begin_frame(void)
{
    /* Set render target to current framebuffer */
    gcmSurface sf;
    memset(&sf, 0, sizeof(sf));

    sf.colorFormat = GCM_SURFACE_X8R8G8B8;
    sf.colorTarget = GCM_SURFACE_TARGET_0;
    sf.colorLocation[0] = GCM_LOCATION_RSX;
    sf.colorOffset[0] = framebuffer_offset[current_buffer];
    sf.colorPitch[0] = color_pitch;

    sf.depthFormat = GCM_SURFACE_ZETA_Z24S8;
    sf.depthLocation = GCM_LOCATION_RSX;
    sf.depthOffset = depth_offset;
    sf.depthPitch = depth_pitch;

    sf.type = GCM_SURFACE_TYPE_LINEAR;
    sf.antiAlias = GCM_SURFACE_CENTER_1;

    sf.width = screen_width;
    sf.height = screen_height;
    sf.x = 0;
    sf.y = 0;

    rsxSetSurface(rsx_context, &sf);

    /* Clear to dark background */
    rsxSetClearColor(rsx_context, COLOR_DARK_BG);
    rsxSetClearDepthStencil(rsx_context, 0xffffff00);
    rsxClearSurface(rsx_context, GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A | GCM_CLEAR_S | GCM_CLEAR_Z);

    /* Set viewport */
    rsxSetViewport(rsx_context, 0, 0, screen_width, screen_height, 0.0f, 1.0f, screen_width/2.0f, screen_height/2.0f);
    rsxSetScissor(rsx_context, 0, 0, screen_width, screen_height);
}

/* End frame rendering */
void ui_end_frame(void)
{
    /* Flip buffers */
    wait_rsx_idle();
    gcmSetFlip(rsx_context, current_buffer);
    rsxFlushBuffer(rsx_context);
    gcmSetWaitFlip(rsx_context);

    current_buffer ^= 1;
}

/* Draw a filled rectangle (software rendering to framebuffer) */
void ui_draw_rect(int x, int y, int w, int h, u32 color)
{
    u32 *fb = framebuffer[current_buffer];

    /* Clamp to screen bounds */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)screen_width) w = screen_width - x;
    if (y + h > (int)screen_height) h = screen_height - y;
    if (w <= 0 || h <= 0) return;

    /* Convert RGBA to XRGB */
    u32 xrgb = ((color >> 8) & 0xFFFFFF);

    for (int py = y; py < y + h; py++) {
        u32 *row = fb + py * screen_width + x;
        for (int px = 0; px < w; px++) {
            row[px] = xrgb;
        }
    }
}

/* Draw text character (simple bitmap) */
static void draw_char(int x, int y, char c, u32 color)
{
    /* Very simple placeholder - real impl would use proper font */
    if (c < 32 || c > 126) return;

    u32 *fb = framebuffer[current_buffer];
    u32 xrgb = ((color >> 8) & 0xFFFFFF);

    /* Draw a simple 8x16 block for each character */
    for (int py = 0; py < FONT_CHAR_H; py++) {
        if (y + py < 0 || y + py >= (int)screen_height) continue;
        for (int px = 0; px < FONT_CHAR_W; px++) {
            if (x + px < 0 || x + px >= (int)screen_width) continue;
            /* Simple pattern based on character */
            if ((c + py + px) % 3 != 0) {
                fb[(y + py) * screen_width + (x + px)] = xrgb;
            }
        }
    }
}

/* Draw text string */
void ui_draw_text(int x, int y, const char *text, u32 color)
{
    if (!text) return;

    int cx = x;
    int cy = y;

    while (*text) {
        if (*text == '\n') {
            cx = x;
            cy += FONT_CHAR_H + 2;
        } else {
            draw_char(cx, cy, *text, color);
            cx += FONT_CHAR_W;
        }
        text++;
    }
}

/* Draw centered text */
void ui_draw_text_centered(int y, const char *text, u32 color)
{
    if (!text) return;

    int len = strlen(text);
    int x = (screen_width - len * FONT_CHAR_W) / 2;
    ui_draw_text(x, y, text, color);
}

/* Draw header bar */
void ui_draw_header(const char *title)
{
    /* Header background */
    ui_draw_rect(0, 0, screen_width, 80, COLOR_MENU_BG);

    /* Netflix-style red accent */
    ui_draw_rect(0, 75, screen_width, 5, COLOR_RED);

    /* Title */
    ui_draw_text(50, 30, title, COLOR_WHITE);

    /* Version in corner */
    char ver[32];
    snprintf(ver, sizeof(ver), "v%s", NEDFLIX_VERSION);
    ui_draw_text(screen_width - 150, 30, ver, COLOR_TEXT_DIM);
}

/* Draw menu options */
void ui_draw_menu(const char **options, int count, int selected)
{
    int start_y = 150;
    int item_height = 50;

    for (int i = 0; i < count; i++) {
        int y = start_y + i * item_height;

        /* Selection highlight */
        if (i == selected) {
            ui_draw_rect(40, y - 5, screen_width - 80, item_height - 5, COLOR_SELECTED);
            ui_draw_rect(40, y - 5, 5, item_height - 5, COLOR_RED);
        }

        ui_draw_text(70, y + 10, options[i], i == selected ? COLOR_WHITE : COLOR_TEXT);
    }
}

/* Draw loading screen */
void ui_draw_loading(const char *message)
{
    ui_draw_header("Nedflix");

    /* Loading text */
    ui_draw_text_centered(screen_height / 2 - 20, message, COLOR_TEXT);

    /* Simple spinner animation */
    static int frame = 0;
    const char *spinner = "|/-\\";
    char spin_str[2] = { spinner[frame % 4], 0 };
    ui_draw_text_centered(screen_height / 2 + 30, spin_str, COLOR_RED);
    frame++;
}

/* Draw error screen */
void ui_draw_error(const char *message)
{
    ui_draw_header("Error");

    /* Error icon area */
    ui_draw_rect(screen_width/2 - 50, 200, 100, 100, COLOR_RED);
    ui_draw_text(screen_width/2 - 10, 240, "!", COLOR_WHITE);

    /* Error message */
    ui_draw_text_centered(350, message, COLOR_TEXT);
}

/* Draw media list */
void ui_draw_media_list(const media_list_t *list)
{
    if (!list || list->count == 0) {
        ui_draw_text_centered(300, "No items found", COLOR_TEXT_DIM);
        return;
    }

    int start_y = 120;
    int item_height = 35;

    for (int i = 0; i < MAX_ITEMS_VISIBLE && (list->scroll_offset + i) < list->count; i++) {
        int idx = list->scroll_offset + i;
        const media_item_t *item = &list->items[idx];
        int y = start_y + i * item_height;

        /* Selection highlight */
        if (idx == list->selected_index) {
            ui_draw_rect(40, y - 2, screen_width - 80, item_height - 2, COLOR_SELECTED);
            ui_draw_rect(40, y - 2, 4, item_height - 2, COLOR_RED);
        }

        /* Icon */
        const char *icon = item->is_directory ? "[D]" :
                          (item->type == MEDIA_TYPE_AUDIO ? "[A]" : "[V]");
        ui_draw_text(60, y + 8, icon, COLOR_TEXT_DIM);

        /* Name */
        ui_draw_text(110, y + 8, item->name, idx == list->selected_index ? COLOR_WHITE : COLOR_TEXT);
    }

    /* Scroll indicators */
    if (list->scroll_offset > 0) {
        ui_draw_text(screen_width - 60, 120, "^", COLOR_TEXT_DIM);
    }
    if (list->scroll_offset + MAX_ITEMS_VISIBLE < list->count) {
        ui_draw_text(screen_width - 60, start_y + MAX_ITEMS_VISIBLE * item_height - 20, "v", COLOR_TEXT_DIM);
    }

    /* Item count */
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d/%d", list->selected_index + 1, list->count);
    ui_draw_text(screen_width - 120, 85, count_str, COLOR_TEXT_DIM);
}

/* Draw media detail */
void ui_draw_media_detail(const media_item_t *item)
{
    if (!item) return;

    ui_draw_text(100, 150, item->name, COLOR_WHITE);
    ui_draw_text(100, 190, item->description, COLOR_TEXT);

    char info[128];
    snprintf(info, sizeof(info), "Duration: %u min | Size: %llu MB",
             item->duration / 60, (unsigned long long)(item->size / (1024*1024)));
    ui_draw_text(100, 250, info, COLOR_TEXT_DIM);
}

/* Draw playback UI */
void ui_draw_playback(const playback_t *pb)
{
    if (!pb) return;

    /* Title */
    ui_draw_text_centered(100, pb->title, COLOR_WHITE);

    /* Progress bar background */
    int bar_x = 100;
    int bar_y = screen_height - 150;
    int bar_w = screen_width - 200;
    int bar_h = 10;

    ui_draw_rect(bar_x, bar_y, bar_w, bar_h, COLOR_MENU_BG);

    /* Progress */
    if (pb->duration_ms > 0) {
        int progress_w = (bar_w * pb->position_ms) / pb->duration_ms;
        ui_draw_rect(bar_x, bar_y, progress_w, bar_h, COLOR_RED);
    }

    /* Time display */
    char time_str[64];
    int pos_min = pb->position_ms / 60000;
    int pos_sec = (pb->position_ms / 1000) % 60;
    int dur_min = pb->duration_ms / 60000;
    int dur_sec = (pb->duration_ms / 1000) % 60;

    snprintf(time_str, sizeof(time_str), "%02d:%02d / %02d:%02d", pos_min, pos_sec, dur_min, dur_sec);
    ui_draw_text_centered(bar_y + 25, time_str, COLOR_TEXT);

    /* Status */
    const char *status = pb->paused ? "PAUSED" : (pb->playing ? "PLAYING" : "STOPPED");
    ui_draw_text_centered(bar_y + 55, status, pb->paused ? COLOR_TEXT_DIM : COLOR_WHITE);

    /* Controls hint */
    ui_draw_text_centered(screen_height - 50, "X:Pause  O:Stop  L2/R2:Volume  Left/Right:Seek", COLOR_TEXT_DIM);
}

/* Draw on-screen keyboard (stub) */
void ui_draw_osk(const char *title, char *output, int max_len)
{
    (void)title;
    (void)output;
    (void)max_len;
    /* Would integrate with PS3 OSK system utility */
    printf("OSK requested: %s\n", title);
}

/* Draw image (stub) */
void ui_draw_image(int x, int y, int w, int h, void *data)
{
    (void)data;
    /* Placeholder - draw colored rect */
    ui_draw_rect(x, y, w, h, COLOR_MENU_BG);
}
