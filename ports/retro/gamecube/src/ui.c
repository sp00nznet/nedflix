/*
 * Nedflix for Nintendo GameCube
 * UI rendering using GX graphics
 *
 * TECHNICAL DEMO / NOVELTY PORT
 */

#include "nedflix.h"

/* GX projection matrix */
static Mtx44 projection;
static Mtx modelview;

/* Font texture (built-in console font) */
static GXTexObj font_texture;
static bool font_loaded = false;

/* Simple 8x8 bitmap font data (ASCII 32-127) */
static const unsigned char g_font_data[] = {
    /* Space (32) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* ! */
    0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00,
    /* " */
    0x6C, 0x6C, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* # */
    0x6C, 0xFE, 0x6C, 0x6C, 0xFE, 0x6C, 0x00, 0x00,
    /* $ */
    0x18, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x18, 0x00,
    /* % */
    0xC6, 0xCC, 0x18, 0x30, 0x66, 0xC6, 0x00, 0x00,
    /* & */
    0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00,
    /* ' */
    0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* ( */
    0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00,
    /* ) */
    0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00,
    /* * */
    0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00,
    /* + */
    0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00,
    /* , */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30,
    /* - */
    0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    /* . */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
    /* / */
    0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x00, 0x00,
    /* 0 */
    0x7C, 0xC6, 0xCE, 0xD6, 0xE6, 0xC6, 0x7C, 0x00,
    /* 1 */
    0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00,
    /* 2 */
    0x7C, 0xC6, 0x06, 0x1C, 0x70, 0xC0, 0xFE, 0x00,
    /* 3 */
    0x7C, 0xC6, 0x06, 0x3C, 0x06, 0xC6, 0x7C, 0x00,
    /* 4 */
    0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x0C, 0x00,
    /* 5 */
    0xFE, 0xC0, 0xFC, 0x06, 0x06, 0xC6, 0x7C, 0x00,
    /* 6 */
    0x38, 0x60, 0xC0, 0xFC, 0xC6, 0xC6, 0x7C, 0x00,
    /* 7 */
    0xFE, 0xC6, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00,
    /* 8 */
    0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C, 0x00,
    /* 9 */
    0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0x78, 0x00,
    /* : */
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00,
    /* ; */
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30,
    /* < */
    0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00,
    /* = */
    0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00,
    /* > */
    0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00,
    /* ? */
    0x7C, 0xC6, 0x0C, 0x18, 0x18, 0x00, 0x18, 0x00,
    /* @ */
    0x7C, 0xC6, 0xDE, 0xDE, 0xDC, 0xC0, 0x7C, 0x00,
    /* A */
    0x38, 0x6C, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0x00,
    /* B */
    0xFC, 0xC6, 0xC6, 0xFC, 0xC6, 0xC6, 0xFC, 0x00,
    /* C */
    0x7C, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x7C, 0x00,
    /* D */
    0xF8, 0xCC, 0xC6, 0xC6, 0xC6, 0xCC, 0xF8, 0x00,
    /* E */
    0xFE, 0xC0, 0xC0, 0xFC, 0xC0, 0xC0, 0xFE, 0x00,
    /* F */
    0xFE, 0xC0, 0xC0, 0xFC, 0xC0, 0xC0, 0xC0, 0x00,
    /* G */
    0x7C, 0xC6, 0xC0, 0xCE, 0xC6, 0xC6, 0x7E, 0x00,
    /* H */
    0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0x00,
    /* I */
    0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00,
    /* J */
    0x06, 0x06, 0x06, 0x06, 0x06, 0xC6, 0x7C, 0x00,
    /* K */
    0xC6, 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0xC6, 0x00,
    /* L */
    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFE, 0x00,
    /* M */
    0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00,
    /* N */
    0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00,
    /* O */
    0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    /* P */
    0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0xC0, 0xC0, 0x00,
    /* Q */
    0x7C, 0xC6, 0xC6, 0xC6, 0xD6, 0xDE, 0x7C, 0x06,
    /* R */
    0xFC, 0xC6, 0xC6, 0xFC, 0xD8, 0xCC, 0xC6, 0x00,
    /* S */
    0x7C, 0xC6, 0xC0, 0x7C, 0x06, 0xC6, 0x7C, 0x00,
    /* T */
    0xFE, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
    /* U */
    0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    /* V */
    0xC6, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x10, 0x00,
    /* W */
    0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00,
    /* X */
    0xC6, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0xC6, 0x00,
    /* Y */
    0xC6, 0xC6, 0x6C, 0x38, 0x18, 0x18, 0x18, 0x00,
    /* Z */
    0xFE, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFE, 0x00,
    /* [ */
    0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00,
    /* \ */
    0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00, 0x00,
    /* ] */
    0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00,
    /* ^ */
    0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00,
    /* _ */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
    /* ` */
    0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* a */
    0x00, 0x00, 0x7C, 0x06, 0x7E, 0xC6, 0x7E, 0x00,
    /* b */
    0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xFC, 0x00,
    /* c */
    0x00, 0x00, 0x7C, 0xC6, 0xC0, 0xC6, 0x7C, 0x00,
    /* d */
    0x06, 0x06, 0x7E, 0xC6, 0xC6, 0xC6, 0x7E, 0x00,
    /* e */
    0x00, 0x00, 0x7C, 0xC6, 0xFE, 0xC0, 0x7C, 0x00,
    /* f */
    0x1C, 0x36, 0x30, 0x78, 0x30, 0x30, 0x30, 0x00,
    /* g */
    0x00, 0x00, 0x7E, 0xC6, 0xC6, 0x7E, 0x06, 0x7C,
    /* h */
    0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x00,
    /* i */
    0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00,
    /* j */
    0x06, 0x00, 0x06, 0x06, 0x06, 0x06, 0xC6, 0x7C,
    /* k */
    0xC0, 0xC0, 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0x00,
    /* l */
    0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00,
    /* m */
    0x00, 0x00, 0xEC, 0xFE, 0xD6, 0xC6, 0xC6, 0x00,
    /* n */
    0x00, 0x00, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x00,
    /* o */
    0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    /* p */
    0x00, 0x00, 0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0xC0,
    /* q */
    0x00, 0x00, 0x7E, 0xC6, 0xC6, 0x7E, 0x06, 0x06,
    /* r */
    0x00, 0x00, 0xDC, 0xE6, 0xC0, 0xC0, 0xC0, 0x00,
    /* s */
    0x00, 0x00, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x00,
    /* t */
    0x30, 0x30, 0x7C, 0x30, 0x30, 0x36, 0x1C, 0x00,
    /* u */
    0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0x7E, 0x00,
    /* v */
    0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00,
    /* w */
    0x00, 0x00, 0xC6, 0xC6, 0xD6, 0xFE, 0x6C, 0x00,
    /* x */
    0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00,
    /* y */
    0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x7C,
    /* z */
    0x00, 0x00, 0xFE, 0x0C, 0x38, 0x60, 0xFE, 0x00,
    /* { */
    0x0E, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0E, 0x00,
    /* | */
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
    /* } */
    0x70, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x70, 0x00,
    /* ~ */
    0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Character width */
#define CHAR_WIDTH  8
#define CHAR_HEIGHT 8
#define FONT_SCALE  2

/*
 * Initialize UI subsystem
 */
int ui_init(void)
{
    /* Initialize GX */
    GXColor background = {0, 0, 0, 255};

    /* Setup FIFO */
    void *gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
    if (!gp_fifo) {
        return -1;
    }
    memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);

    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
    GX_SetCopyClear(background, GX_MAX_Z24);

    /* Setup viewport */
    GX_SetViewport(0, 0, g_app.rmode->fbWidth, g_app.rmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)g_app.rmode->xfbHeight / (f32)g_app.rmode->efbHeight);
    GX_SetScissor(0, 0, g_app.rmode->fbWidth, g_app.rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, g_app.rmode->fbWidth, g_app.rmode->efbHeight);
    GX_SetDispCopyDst(g_app.rmode->fbWidth, g_app.rmode->xfbHeight);
    GX_SetCopyFilter(g_app.rmode->aa, g_app.rmode->sample_pattern,
                     GX_TRUE, g_app.rmode->vfilter);

    GX_SetFieldMode(g_app.rmode->field_rendering,
                    ((g_app.rmode->viHeight == 2 * g_app.rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

    if (g_app.rmode->aa) {
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    } else {
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    }

    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(g_app.framebuffer[g_app.fb_index], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    /* Setup 2D orthographic projection */
    guOrtho(projection, 0, SCREEN_HEIGHT, 0, SCREEN_WIDTH, 0, 300);
    GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);

    /* Setup modelview */
    guMtxIdentity(modelview);
    GX_LoadPosMtxImm(modelview, GX_PNMTX0);

    /* Setup vertex format for 2D rendering */
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    /* Setup TEV for color only (no texture) */
    GX_SetNumChans(1);
    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    font_loaded = true;

    return 0;
}

/*
 * Shutdown UI subsystem
 */
void ui_shutdown(void)
{
    /* Nothing to clean up */
}

/*
 * Begin rendering frame
 */
void ui_begin_frame(void)
{
    /* Setup for 2D rendering */
    GX_SetViewport(0, 0, g_app.rmode->fbWidth, g_app.rmode->efbHeight, 0, 1);
    GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);

    guMtxIdentity(modelview);
    GX_LoadPosMtxImm(modelview, GX_PNMTX0);
}

/*
 * End rendering frame
 */
void ui_end_frame(void)
{
    /* Finish rendering */
    GX_DrawDone();

    /* Copy EFB to XFB */
    g_app.fb_index ^= 1;
    GX_CopyDisp(g_app.framebuffer[g_app.fb_index], GX_TRUE);

    /* Set next framebuffer */
    VIDEO_SetNextFramebuffer(g_app.framebuffer[g_app.fb_index]);
    VIDEO_Flush();
}

/*
 * Clear screen
 */
void ui_clear(GXColor color)
{
    GX_SetCopyClear(color, GX_MAX_Z24);
}

/*
 * Draw filled rectangle
 */
void ui_draw_rect(int x, int y, int width, int height, GXColor color)
{
    /* Setup for colored quads */
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position2f32(x, y);
        GX_Color4u8(color.r, color.g, color.b, color.a);

        GX_Position2f32(x + width, y);
        GX_Color4u8(color.r, color.g, color.b, color.a);

        GX_Position2f32(x + width, y + height);
        GX_Color4u8(color.r, color.g, color.b, color.a);

        GX_Position2f32(x, y + height);
        GX_Color4u8(color.r, color.g, color.b, color.a);
    GX_End();
}

/*
 * Draw text at position
 */
void ui_draw_text(int x, int y, const char *text, GXColor color)
{
    if (!text) return;

    int cursor_x = x;
    int cursor_y = y;

    /* Setup for colored points/quads */
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    while (*text) {
        unsigned char c = *text++;

        if (c == '\n') {
            cursor_x = x;
            cursor_y += CHAR_HEIGHT * FONT_SCALE + 2;
            continue;
        }

        if (c < 32 || c > 126) {
            c = '?';
        }

        /* Get glyph data */
        const unsigned char *glyph = &g_font_data[(c - 32) * 8];

        /* Draw character as filled pixels */
        for (int row = 0; row < CHAR_HEIGHT; row++) {
            unsigned char row_data = glyph[row];
            for (int col = 0; col < CHAR_WIDTH; col++) {
                if (row_data & (0x80 >> col)) {
                    /* Draw scaled pixel as small quad */
                    int px = cursor_x + col * FONT_SCALE;
                    int py = cursor_y + row * FONT_SCALE;

                    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
                        GX_Position2f32(px, py);
                        GX_Color4u8(color.r, color.g, color.b, color.a);

                        GX_Position2f32(px + FONT_SCALE, py);
                        GX_Color4u8(color.r, color.g, color.b, color.a);

                        GX_Position2f32(px + FONT_SCALE, py + FONT_SCALE);
                        GX_Color4u8(color.r, color.g, color.b, color.a);

                        GX_Position2f32(px, py + FONT_SCALE);
                        GX_Color4u8(color.r, color.g, color.b, color.a);
                    GX_End();
                }
            }
        }

        cursor_x += CHAR_WIDTH * FONT_SCALE;
    }
}

/*
 * Draw centered text
 */
void ui_draw_text_centered(int y, const char *text, GXColor color)
{
    if (!text) return;

    int text_width = strlen(text) * CHAR_WIDTH * FONT_SCALE;
    int x = (SCREEN_WIDTH - text_width) / 2;

    ui_draw_text(x, y, text, color);
}

/*
 * Draw header bar
 */
void ui_draw_header(const char *title)
{
    /* Header background */
    ui_draw_rect(0, 0, SCREEN_WIDTH, 50, COLOR_RED);

    /* Title */
    ui_draw_text(20, 15, title, COLOR_WHITE);

    /* Version */
    char version[32];
    snprintf(version, sizeof(version), "v%s", NEDFLIX_VERSION_STRING);
    int version_width = strlen(version) * CHAR_WIDTH * FONT_SCALE;
    ui_draw_text(SCREEN_WIDTH - version_width - 20, 15, version, COLOR_WHITE);
}

/*
 * Draw menu list
 */
void ui_draw_menu(const char **items, int count, int selected)
{
    int start_y = 80;
    int item_height = CHAR_HEIGHT * FONT_SCALE + 12;

    for (int i = 0; i < count && i < MAX_MENU_ITEMS; i++) {
        int y = start_y + i * item_height;

        /* Selection highlight */
        if (i == selected) {
            ui_draw_rect(10, y - 4, SCREEN_WIDTH - 20, item_height, COLOR_SELECTED);

            /* Selection indicator */
            ui_draw_text(20, y, ">", COLOR_RED);
        }

        /* Item text */
        ui_draw_text(40, y, items[i], i == selected ? COLOR_WHITE : COLOR_TEXT);
    }
}

/*
 * Draw file list
 */
void ui_draw_file_list(media_list_t *list)
{
    if (!list) return;

    int start_y = 70;
    int item_height = CHAR_HEIGHT * FONT_SCALE + 8;
    int visible_items = MIN(MAX_ITEMS_PER_PAGE, list->count - list->scroll_offset);

    for (int i = 0; i < visible_items; i++) {
        int index = list->scroll_offset + i;
        if (index >= list->count) break;

        media_item_t *item = &list->items[index];
        int y = start_y + i * item_height;

        /* Selection highlight */
        if (index == list->selected_index) {
            ui_draw_rect(10, y - 2, SCREEN_WIDTH - 20, item_height, COLOR_SELECTED);
        }

        /* Icon */
        const char *icon = item->is_directory ? "[D]" : "[A]";
        ui_draw_text(20, y, icon, COLOR_TEXT_DIM);

        /* Name */
        GXColor text_color = (index == list->selected_index) ? COLOR_WHITE : COLOR_TEXT;
        ui_draw_text(60, y, item->name, text_color);
    }

    /* Scroll indicators */
    if (list->scroll_offset > 0) {
        ui_draw_text_centered(55, "^ More ^", COLOR_TEXT_DIM);
    }
    if (list->scroll_offset + MAX_ITEMS_PER_PAGE < list->count) {
        ui_draw_text_centered(SCREEN_HEIGHT - 50, "v More v", COLOR_TEXT_DIM);
    }

    /* Item count */
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d/%d", list->selected_index + 1, list->count);
    int count_width = strlen(count_str) * CHAR_WIDTH * FONT_SCALE;
    ui_draw_text(SCREEN_WIDTH - count_width - 20, SCREEN_HEIGHT - 50, count_str, COLOR_TEXT_DIM);
}

/*
 * Draw progress bar
 */
void ui_draw_progress_bar(int x, int y, int width, int height, float progress, GXColor fg, GXColor bg)
{
    /* Background */
    ui_draw_rect(x, y, width, height, bg);

    /* Filled portion */
    int fill_width = (int)(width * CLAMP(progress, 0.0f, 1.0f));
    if (fill_width > 0) {
        ui_draw_rect(x, y, fill_width, height, fg);
    }
}

/*
 * Draw loading screen
 */
void ui_draw_loading(const char *message)
{
    /* Center the message */
    ui_draw_text_centered(SCREEN_HEIGHT / 2 - 20, "NEDFLIX", COLOR_RED);
    ui_draw_text_centered(SCREEN_HEIGHT / 2 + 20, message, COLOR_TEXT);

    /* Animated dots */
    static int dots = 0;
    dots = (dots + 1) % 60;
    int num_dots = (dots / 15) + 1;

    char dot_str[8] = "";
    for (int i = 0; i < num_dots && i < 4; i++) {
        strcat(dot_str, ".");
    }
    ui_draw_text_centered(SCREEN_HEIGHT / 2 + 50, dot_str, COLOR_TEXT_DIM);
}

/*
 * Draw error screen
 */
void ui_draw_error(const char *message)
{
    /* Error header */
    ui_draw_rect(0, 0, SCREEN_WIDTH, 50, COLOR_RED);
    ui_draw_text(20, 15, "Error", COLOR_WHITE);

    /* Error message */
    ui_draw_text_centered(SCREEN_HEIGHT / 2, message, COLOR_TEXT);
}

/*
 * Draw playback HUD
 */
void ui_draw_playback_hud(playback_state_t *state)
{
    if (!state) return;

    /* Background panel */
    ui_draw_rect(0, SCREEN_HEIGHT - 120, SCREEN_WIDTH, 120, COLOR_LIGHT_GRAY);

    /* Now Playing */
    ui_draw_text(20, SCREEN_HEIGHT - 110, "Now Playing:", COLOR_TEXT_DIM);
    ui_draw_text(20, SCREEN_HEIGHT - 85, state->title, COLOR_WHITE);

    /* Status */
    const char *status = state->is_paused ? "PAUSED" : (state->is_playing ? "PLAYING" : "STOPPED");
    ui_draw_text(SCREEN_WIDTH - 120, SCREEN_HEIGHT - 110, status, COLOR_RED);

    /* Progress bar */
    float progress = 0.0f;
    if (state->duration > 0) {
        progress = (float)(state->current_time / state->duration);
    }
    ui_draw_progress_bar(20, SCREEN_HEIGHT - 55, SCREEN_WIDTH - 40, 10, progress, COLOR_RED, COLOR_DARK_GRAY);

    /* Time display */
    char time_str[64];
    int cur_min = (int)(state->current_time / 60);
    int cur_sec = (int)state->current_time % 60;
    int dur_min = (int)(state->duration / 60);
    int dur_sec = (int)state->duration % 60;
    snprintf(time_str, sizeof(time_str), "%02d:%02d / %02d:%02d", cur_min, cur_sec, dur_min, dur_sec);
    ui_draw_text(20, SCREEN_HEIGHT - 35, time_str, COLOR_TEXT);

    /* Volume */
    char vol_str[32];
    snprintf(vol_str, sizeof(vol_str), "Vol: %d%%", (state->volume * 100) / 255);
    int vol_width = strlen(vol_str) * CHAR_WIDTH * FONT_SCALE;
    ui_draw_text(SCREEN_WIDTH - vol_width - 20, SCREEN_HEIGHT - 35, vol_str, COLOR_TEXT);

    /* Controls help */
    ui_draw_text_centered(SCREEN_HEIGHT - 15, "A:Play/Pause  B:Stop  D-Pad:Volume", COLOR_TEXT_DIM);
}
