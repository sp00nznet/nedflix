/*
 * Nedflix PS3 - UI rendering using RSX
 * Full implementation with bitmap font and all UI components
 */

#include "nedflix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <rsx/rsx.h>
#include <rsx/gcm_sys.h>
#include <sysutil/video.h>
#include <sys/time.h>

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

/* Animation state */
static u64 ui_start_time = 0;
static u64 ui_frame_count = 0;

/*
 * Embedded bitmap font - 8x16 ASCII characters (32-126)
 * Each character is 16 bytes (16 rows, 1 byte per row = 8 pixels)
 */
static const u8 font_bitmap[95][16] = {
    /* Space (32) */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ! (33) */
    {0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00},
    /* " (34) */
    {0x00,0x66,0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* # (35) */
    {0x00,0x00,0x6C,0x6C,0xFE,0x6C,0x6C,0x6C,0xFE,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00},
    /* $ (36) */
    {0x00,0x10,0x7C,0xD6,0xD0,0x70,0x3C,0x16,0xD6,0x7C,0x10,0x00,0x00,0x00,0x00,0x00},
    /* % (37) */
    {0x00,0x00,0x00,0xC2,0xC6,0x0C,0x18,0x30,0x66,0xC6,0x00,0x00,0x00,0x00,0x00,0x00},
    /* & (38) */
    {0x00,0x00,0x38,0x6C,0x6C,0x38,0x76,0xDC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00,0x00},
    /* ' (39) */
    {0x00,0x18,0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ( (40) */
    {0x00,0x00,0x0C,0x18,0x30,0x30,0x30,0x30,0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},
    /* ) (41) */
    {0x00,0x00,0x30,0x18,0x0C,0x0C,0x0C,0x0C,0x0C,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
    /* * (42) */
    {0x00,0x00,0x00,0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* + (43) */
    {0x00,0x00,0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* , (44) */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x00},
    /* - (45) */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* . (46) */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00},
    /* / (47) */
    {0x00,0x00,0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 0 (48) */
    {0x00,0x00,0x7C,0xC6,0xCE,0xDE,0xF6,0xE6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* 1 (49) */
    {0x00,0x00,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00,0x00,0x00},
    /* 2 (50) */
    {0x00,0x00,0x7C,0xC6,0x06,0x0C,0x18,0x30,0x60,0xC6,0xFE,0x00,0x00,0x00,0x00,0x00},
    /* 3 (51) */
    {0x00,0x00,0x7C,0xC6,0x06,0x06,0x3C,0x06,0x06,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* 4 (52) */
    {0x00,0x00,0x0C,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x1E,0x00,0x00,0x00,0x00,0x00},
    /* 5 (53) */
    {0x00,0x00,0xFE,0xC0,0xC0,0xFC,0x06,0x06,0x06,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* 6 (54) */
    {0x00,0x00,0x38,0x60,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* 7 (55) */
    {0x00,0x00,0xFE,0xC6,0x06,0x0C,0x18,0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00,0x00},
    /* 8 (56) */
    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* 9 (57) */
    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7E,0x06,0x06,0x0C,0x78,0x00,0x00,0x00,0x00,0x00},
    /* : (58) */
    {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ; (59) */
    {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
    /* < (60) */
    {0x00,0x00,0x06,0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x06,0x00,0x00,0x00,0x00,0x00},
    /* = (61) */
    {0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* > (62) */
    {0x00,0x00,0x60,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x60,0x00,0x00,0x00,0x00,0x00},
    /* ? (63) */
    {0x00,0x00,0x7C,0xC6,0xC6,0x0C,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00},
    /* @ (64) */
    {0x00,0x00,0x7C,0xC6,0xC6,0xDE,0xDE,0xDE,0xDC,0xC0,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* A (65) */
    {0x00,0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00,0x00},
    /* B (66) */
    {0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x66,0x66,0x66,0xFC,0x00,0x00,0x00,0x00,0x00},
    /* C (67) */
    {0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xC0,0xC2,0x66,0x3C,0x00,0x00,0x00,0x00,0x00},
    /* D (68) */
    {0x00,0x00,0xF8,0x6C,0x66,0x66,0x66,0x66,0x66,0x6C,0xF8,0x00,0x00,0x00,0x00,0x00},
    /* E (69) */
    {0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68,0x62,0x66,0xFE,0x00,0x00,0x00,0x00,0x00},
    /* F (70) */
    {0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68,0x60,0x60,0xF0,0x00,0x00,0x00,0x00,0x00},
    /* G (71) */
    {0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xDE,0xC6,0x66,0x3A,0x00,0x00,0x00,0x00,0x00},
    /* H (72) */
    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00,0x00},
    /* I (73) */
    {0x00,0x00,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,0x00},
    /* J (74) */
    {0x00,0x00,0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00,0x00,0x00,0x00,0x00},
    /* K (75) */
    {0x00,0x00,0xE6,0x66,0x6C,0x6C,0x78,0x6C,0x6C,0x66,0xE6,0x00,0x00,0x00,0x00,0x00},
    /* L (76) */
    {0x00,0x00,0xF0,0x60,0x60,0x60,0x60,0x60,0x62,0x66,0xFE,0x00,0x00,0x00,0x00,0x00},
    /* M (77) */
    {0x00,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00,0x00},
    /* N (78) */
    {0x00,0x00,0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00,0x00},
    /* O (79) */
    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* P (80) */
    {0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00,0x00},
    /* Q (81) */
    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x0C,0x0E,0x00,0x00,0x00,0x00},
    /* R (82) */
    {0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x6C,0x66,0x66,0xE6,0x00,0x00,0x00,0x00,0x00},
    /* S (83) */
    {0x00,0x00,0x7C,0xC6,0xC6,0x60,0x38,0x0C,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* T (84) */
    {0x00,0x00,0x7E,0x7E,0x5A,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,0x00},
    /* U (85) */
    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* V (86) */
    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00,0x00,0x00,0x00,0x00},
    /* W (87) */
    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xD6,0xFE,0xFE,0xEE,0xC6,0x00,0x00,0x00,0x00,0x00},
    /* X (88) */
    {0x00,0x00,0xC6,0xC6,0x6C,0x38,0x38,0x38,0x6C,0xC6,0xC6,0x00,0x00,0x00,0x00,0x00},
    /* Y (89) */
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,0x00},
    /* Z (90) */
    {0x00,0x00,0xFE,0xC6,0x8C,0x18,0x30,0x60,0xC2,0xC6,0xFE,0x00,0x00,0x00,0x00,0x00},
    /* [ (91) */
    {0x00,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,0x00,0x00,0x00,0x00},
    /* \ (92) */
    {0x00,0x00,0x80,0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ] (93) */
    {0x00,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,0x00,0x00,0x00,0x00},
    /* ^ (94) */
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* _ (95) */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
    /* ` (96) */
    {0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* a (97) */
    {0x00,0x00,0x00,0x00,0x00,0x78,0x0C,0x7C,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00,0x00},
    /* b (98) */
    {0x00,0x00,0xE0,0x60,0x60,0x78,0x6C,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* c (99) */
    {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xC0,0xC0,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* d (100) */
    {0x00,0x00,0x1C,0x0C,0x0C,0x3C,0x6C,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00,0x00},
    /* e (101) */
    {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xFE,0xC0,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* f (102) */
    {0x00,0x00,0x38,0x6C,0x64,0x60,0xF0,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00,0x00},
    /* g (103) */
    {0x00,0x00,0x00,0x00,0x00,0x76,0xCC,0xCC,0xCC,0x7C,0x0C,0xCC,0x78,0x00,0x00,0x00},
    /* h (104) */
    {0x00,0x00,0xE0,0x60,0x60,0x6C,0x76,0x66,0x66,0x66,0xE6,0x00,0x00,0x00,0x00,0x00},
    /* i (105) */
    {0x00,0x00,0x18,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,0x00},
    /* j (106) */
    {0x00,0x00,0x06,0x06,0x00,0x0E,0x06,0x06,0x06,0x06,0x66,0x66,0x3C,0x00,0x00,0x00},
    /* k (107) */
    {0x00,0x00,0xE0,0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00,0x00,0x00,0x00,0x00},
    /* l (108) */
    {0x00,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,0x00},
    /* m (109) */
    {0x00,0x00,0x00,0x00,0x00,0xEC,0xFE,0xD6,0xD6,0xD6,0xC6,0x00,0x00,0x00,0x00,0x00},
    /* n (110) */
    {0x00,0x00,0x00,0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00},
    /* o (111) */
    {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* p (112) */
    {0x00,0x00,0x00,0x00,0x00,0xDC,0x66,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00,0x00,0x00},
    /* q (113) */
    {0x00,0x00,0x00,0x00,0x00,0x76,0xCC,0xCC,0xCC,0x7C,0x0C,0x0C,0x1E,0x00,0x00,0x00},
    /* r (114) */
    {0x00,0x00,0x00,0x00,0x00,0xDC,0x76,0x66,0x60,0x60,0xF0,0x00,0x00,0x00,0x00,0x00},
    /* s (115) */
    {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0x70,0x1C,0xC6,0x7C,0x00,0x00,0x00,0x00,0x00},
    /* t (116) */
    {0x00,0x00,0x10,0x30,0x30,0xFC,0x30,0x30,0x30,0x36,0x1C,0x00,0x00,0x00,0x00,0x00},
    /* u (117) */
    {0x00,0x00,0x00,0x00,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00,0x00},
    /* v (118) */
    {0x00,0x00,0x00,0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00,0x00,0x00,0x00,0x00},
    /* w (119) */
    {0x00,0x00,0x00,0x00,0x00,0xC6,0xC6,0xD6,0xFE,0xFE,0x6C,0x00,0x00,0x00,0x00,0x00},
    /* x (120) */
    {0x00,0x00,0x00,0x00,0x00,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00,0x00},
    /* y (121) */
    {0x00,0x00,0x00,0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x7E,0x06,0x0C,0xF8,0x00,0x00,0x00},
    /* z (122) */
    {0x00,0x00,0x00,0x00,0x00,0xFE,0xCC,0x18,0x30,0x66,0xFE,0x00,0x00,0x00,0x00,0x00},
    /* { (123) */
    {0x00,0x00,0x0E,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00,0x00},
    /* | (124) */
    {0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00},
    /* } (125) */
    {0x00,0x00,0x70,0x18,0x18,0x18,0x0E,0x18,0x18,0x18,0x70,0x00,0x00,0x00,0x00,0x00},
    /* ~ (126) */
    {0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

#define FONT_CHAR_W 8
#define FONT_CHAR_H 16
#define FONT_FIRST_CHAR 32

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
        gcmSetDisplayBuffer(i, framebuffer_offset[i], color_pitch, screen_width, screen_height);
    }

    /* Allocate depth buffer */
    void *depth_buffer = rsxMemalign(64, depth_size);
    if (!depth_buffer) {
        printf("Failed to allocate depth buffer\n");
        return -1;
    }
    rsxAddressToOffset(depth_buffer, &depth_offset);

    /* Store context for app */
    g_app.gcm_context = rsx_context;
    g_app.video_buffer = framebuffer[0];

    /* Initialize timing */
    ui_start_time = sys_time_get_system_time();
    ui_frame_count = 0;

    printf("RSX initialized successfully\n");
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

/* Wait for RSX idle */
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

    rsxSetClearColor(rsx_context, COLOR_DARK_BG);
    rsxSetClearDepthStencil(rsx_context, 0xffffff00);
    rsxClearSurface(rsx_context, GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A | GCM_CLEAR_S | GCM_CLEAR_Z);

    rsxSetViewport(rsx_context, 0, 0, screen_width, screen_height, 0.0f, 1.0f, screen_width/2.0f, screen_height/2.0f);
    rsxSetScissor(rsx_context, 0, 0, screen_width, screen_height);

    ui_frame_count++;
}

/* End frame rendering */
void ui_end_frame(void)
{
    wait_rsx_idle();
    gcmSetFlip(rsx_context, current_buffer);
    rsxFlushBuffer(rsx_context);
    gcmSetWaitFlip(rsx_context);

    current_buffer ^= 1;
}

/* Get elapsed time in milliseconds */
u32 ui_get_time_ms(void)
{
    u64 now = sys_time_get_system_time();
    return (u32)((now - ui_start_time) / 1000);
}

/* Draw filled rectangle */
void ui_draw_rect(int x, int y, int w, int h, u32 color)
{
    u32 *fb = framebuffer[current_buffer];

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)screen_width) w = screen_width - x;
    if (y + h > (int)screen_height) h = screen_height - y;
    if (w <= 0 || h <= 0) return;

    u32 xrgb = ((color >> 8) & 0xFFFFFF);

    for (int py = y; py < y + h; py++) {
        u32 *row = fb + py * screen_width + x;
        for (int px = 0; px < w; px++) {
            row[px] = xrgb;
        }
    }
}

/* Draw rectangle with alpha blending */
void ui_draw_rect_alpha(int x, int y, int w, int h, u32 color, u8 alpha)
{
    u32 *fb = framebuffer[current_buffer];

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)screen_width) w = screen_width - x;
    if (y + h > (int)screen_height) h = screen_height - y;
    if (w <= 0 || h <= 0) return;

    u8 sr = (color >> 24) & 0xFF;
    u8 sg = (color >> 16) & 0xFF;
    u8 sb = (color >> 8) & 0xFF;

    for (int py = y; py < y + h; py++) {
        u32 *row = fb + py * screen_width + x;
        for (int px = 0; px < w; px++) {
            u32 dst = row[px];
            u8 dr = (dst >> 16) & 0xFF;
            u8 dg = (dst >> 8) & 0xFF;
            u8 db = dst & 0xFF;

            u8 r = ((sr * alpha) + (dr * (255 - alpha))) / 255;
            u8 g = ((sg * alpha) + (dg * (255 - alpha))) / 255;
            u8 b = ((sb * alpha) + (db * (255 - alpha))) / 255;

            row[px] = (r << 16) | (g << 8) | b;
        }
    }
}

/* Draw rectangle outline */
void ui_draw_rect_outline(int x, int y, int w, int h, u32 color, int thickness)
{
    ui_draw_rect(x, y, w, thickness, color);                    /* Top */
    ui_draw_rect(x, y + h - thickness, w, thickness, color);    /* Bottom */
    ui_draw_rect(x, y, thickness, h, color);                    /* Left */
    ui_draw_rect(x + w - thickness, y, thickness, h, color);    /* Right */
}

/* Draw rounded rectangle */
void ui_draw_rounded_rect(int x, int y, int w, int h, int radius, u32 color)
{
    /* Draw main rectangles */
    ui_draw_rect(x + radius, y, w - 2*radius, h, color);
    ui_draw_rect(x, y + radius, radius, h - 2*radius, color);
    ui_draw_rect(x + w - radius, y + radius, radius, h - 2*radius, color);

    /* Draw corner circles (simplified as filled rects for performance) */
    ui_draw_rect(x, y, radius, radius, color);
    ui_draw_rect(x + w - radius, y, radius, radius, color);
    ui_draw_rect(x, y + h - radius, radius, radius, color);
    ui_draw_rect(x + w - radius, y + h - radius, radius, radius, color);
}

/* Draw a single character */
static void draw_char(int x, int y, char c, u32 color, int scale)
{
    if (c < FONT_FIRST_CHAR || c > 126) return;

    u32 *fb = framebuffer[current_buffer];
    u32 xrgb = ((color >> 8) & 0xFFFFFF);
    const u8 *glyph = font_bitmap[c - FONT_FIRST_CHAR];

    for (int py = 0; py < FONT_CHAR_H; py++) {
        u8 row_bits = glyph[py];
        for (int px = 0; px < FONT_CHAR_W; px++) {
            if (row_bits & (0x80 >> px)) {
                /* Draw scaled pixel */
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        int fx = x + px * scale + sx;
                        int fy = y + py * scale + sy;
                        if (fx >= 0 && fx < (int)screen_width && fy >= 0 && fy < (int)screen_height) {
                            fb[fy * screen_width + fx] = xrgb;
                        }
                    }
                }
            }
        }
    }
}

/* Draw text string */
void ui_draw_text(int x, int y, const char *text, u32 color)
{
    ui_draw_text_scaled(x, y, text, color, 1);
}

/* Draw text with scale */
void ui_draw_text_scaled(int x, int y, const char *text, u32 color, int scale)
{
    if (!text || scale < 1) return;

    int cx = x;
    int cy = y;
    int char_w = FONT_CHAR_W * scale;
    int char_h = FONT_CHAR_H * scale;

    while (*text) {
        if (*text == '\n') {
            cx = x;
            cy += char_h + 2;
        } else {
            draw_char(cx, cy, *text, color, scale);
            cx += char_w;
        }
        text++;
    }
}

/* Draw centered text */
void ui_draw_text_centered(int y, const char *text, u32 color)
{
    ui_draw_text_centered_scaled(y, text, color, 1);
}

/* Draw centered text with scale */
void ui_draw_text_centered_scaled(int y, const char *text, u32 color, int scale)
{
    if (!text) return;

    int len = strlen(text);
    int char_w = FONT_CHAR_W * scale;
    int x = (screen_width - len * char_w) / 2;
    ui_draw_text_scaled(x, y, text, color, scale);
}

/* Draw right-aligned text */
void ui_draw_text_right(int x, int y, const char *text, u32 color)
{
    if (!text) return;
    int len = strlen(text);
    ui_draw_text(x - len * FONT_CHAR_W, y, text, color);
}

/* Calculate text width */
int ui_text_width(const char *text, int scale)
{
    if (!text) return 0;
    return strlen(text) * FONT_CHAR_W * scale;
}

/* Draw gradient rectangle */
void ui_draw_gradient_v(int x, int y, int w, int h, u32 color_top, u32 color_bottom)
{
    u32 *fb = framebuffer[current_buffer];

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)screen_width) w = screen_width - x;
    if (y + h > (int)screen_height) h = screen_height - y;
    if (w <= 0 || h <= 0) return;

    u8 r1 = (color_top >> 24) & 0xFF;
    u8 g1 = (color_top >> 16) & 0xFF;
    u8 b1 = (color_top >> 8) & 0xFF;
    u8 r2 = (color_bottom >> 24) & 0xFF;
    u8 g2 = (color_bottom >> 16) & 0xFF;
    u8 b2 = (color_bottom >> 8) & 0xFF;

    for (int py = 0; py < h; py++) {
        int t = (py * 255) / h;
        u8 r = r1 + ((r2 - r1) * t) / 255;
        u8 g = g1 + ((g2 - g1) * t) / 255;
        u8 b = b1 + ((b2 - b1) * t) / 255;
        u32 xrgb = (r << 16) | (g << 8) | b;

        u32 *row = fb + (y + py) * screen_width + x;
        for (int px = 0; px < w; px++) {
            row[px] = xrgb;
        }
    }
}

/* Draw horizontal line */
void ui_draw_hline(int x, int y, int w, u32 color)
{
    ui_draw_rect(x, y, w, 1, color);
}

/* Draw vertical line */
void ui_draw_vline(int x, int y, int h, u32 color)
{
    ui_draw_rect(x, y, 1, h, color);
}

/* Draw progress bar */
void ui_draw_progress_bar(int x, int y, int w, int h, int percent, u32 fg_color, u32 bg_color)
{
    ui_draw_rect(x, y, w, h, bg_color);

    if (percent > 0) {
        if (percent > 100) percent = 100;
        int fill_w = (w * percent) / 100;
        ui_draw_rect(x, y, fill_w, h, fg_color);
    }
}

/* Draw header bar */
void ui_draw_header(const char *title)
{
    /* Header gradient background */
    ui_draw_gradient_v(0, 0, screen_width, 80, COLOR_MENU_BG, COLOR_DARK_BG);

    /* Red accent line */
    ui_draw_rect(0, 75, screen_width, 5, COLOR_RED);

    /* Logo/Title */
    ui_draw_text_scaled(50, 25, "NEDFLIX", COLOR_RED, 2);

    /* Current section */
    if (title && title[0]) {
        ui_draw_text(250, 35, title, COLOR_WHITE);
    }

    /* Version */
    char ver[32];
    snprintf(ver, sizeof(ver), "v%s", NEDFLIX_VERSION);
    ui_draw_text_right(screen_width - 50, 35, ver, COLOR_TEXT_DIM);
}

/* Draw footer with button hints */
void ui_draw_footer(const char *hints)
{
    int footer_y = screen_height - 50;

    ui_draw_gradient_v(0, footer_y, screen_width, 50, COLOR_DARK_BG, COLOR_MENU_BG);
    ui_draw_rect(0, footer_y, screen_width, 2, COLOR_TEXT_DIM);

    if (hints) {
        ui_draw_text_centered(footer_y + 18, hints, COLOR_TEXT_DIM);
    }
}

/* Draw menu options */
void ui_draw_menu(const char **options, int count, int selected)
{
    int start_y = 150;
    int item_height = 50;

    for (int i = 0; i < count; i++) {
        int y = start_y + i * item_height;

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
    ui_draw_header("Loading");

    ui_draw_text_centered(screen_height / 2 - 20, message, COLOR_TEXT);

    /* Animated spinner */
    static const char *spinner_frames[] = { "|", "/", "-", "\\" };
    int frame = (ui_frame_count / 10) % 4;
    ui_draw_text_centered_scaled(screen_height / 2 + 40, spinner_frames[frame], COLOR_RED, 2);

    /* Progress dots */
    int dots = (ui_frame_count / 20) % 4;
    char dot_str[8] = "";
    for (int i = 0; i < dots; i++) strcat(dot_str, ".");
    ui_draw_text_centered(screen_height / 2 + 80, dot_str, COLOR_TEXT_DIM);
}

/* Draw error screen */
void ui_draw_error(const char *message)
{
    ui_draw_header("Error");

    /* Error icon (red X) */
    int icon_x = screen_width / 2 - 40;
    int icon_y = 200;
    ui_draw_rect(icon_x, icon_y, 80, 80, COLOR_RED);
    ui_draw_text_scaled(icon_x + 28, icon_y + 20, "X", COLOR_WHITE, 3);

    /* Error message */
    ui_draw_text_centered(350, message, COLOR_TEXT);

    ui_draw_footer("O: Back");
}

/* Draw splash screen */
void ui_draw_splash(int progress)
{
    /* Dark background with gradient */
    ui_draw_gradient_v(0, 0, screen_width, screen_height, 0x141414FF, 0x000000FF);

    /* Large centered logo */
    ui_draw_text_centered_scaled(screen_height / 2 - 60, "NEDFLIX", COLOR_RED, 4);

    /* Tagline */
    ui_draw_text_centered(screen_height / 2 + 20, "Stream Your Way", COLOR_TEXT_DIM);

    /* Loading bar */
    int bar_w = 400;
    int bar_x = (screen_width - bar_w) / 2;
    int bar_y = screen_height / 2 + 80;
    ui_draw_progress_bar(bar_x, bar_y, bar_w, 6, progress, COLOR_RED, COLOR_MENU_BG);

    /* Loading text */
    char loading_text[32];
    snprintf(loading_text, sizeof(loading_text), "Loading... %d%%", progress);
    ui_draw_text_centered(bar_y + 20, loading_text, COLOR_TEXT_DIM);

    /* Copyright */
    ui_draw_text_centered(screen_height - 40, "(C) 2024 Nedflix - PS3 Homebrew Edition", COLOR_TEXT_DIM);
}

/* Draw media list */
void ui_draw_media_list(const media_list_t *list)
{
    if (!list || list->count == 0) {
        ui_draw_text_centered(300, "No items found", COLOR_TEXT_DIM);
        return;
    }

    int start_y = 120;
    int item_height = 40;

    for (int i = 0; i < MAX_ITEMS_VISIBLE && (list->scroll_offset + i) < list->count; i++) {
        int idx = list->scroll_offset + i;
        const media_item_t *item = &list->items[idx];
        int y = start_y + i * item_height;

        bool selected = (idx == list->selected_index);

        if (selected) {
            ui_draw_rect(40, y - 2, screen_width - 80, item_height - 4, COLOR_SELECTED);
            ui_draw_rect(40, y - 2, 4, item_height - 4, COLOR_RED);
        }

        /* Type icon */
        const char *icon;
        u32 icon_color;
        if (item->is_directory) {
            icon = "[D]";
            icon_color = COLOR_TEXT_DIM;
        } else if (item->type == MEDIA_TYPE_AUDIO) {
            icon = "[M]";
            icon_color = 0x4CAF50FF;  /* Green */
        } else {
            icon = "[V]";
            icon_color = 0x2196F3FF;  /* Blue */
        }
        ui_draw_text(60, y + 10, icon, icon_color);

        /* Title */
        ui_draw_text(110, y + 10, item->name, selected ? COLOR_WHITE : COLOR_TEXT);

        /* Duration (if applicable) */
        if (item->duration > 0) {
            char dur[16];
            int mins = item->duration / 60;
            int secs = item->duration % 60;
            snprintf(dur, sizeof(dur), "%d:%02d", mins, secs);
            ui_draw_text_right(screen_width - 60, y + 10, dur, COLOR_TEXT_DIM);
        }
    }

    /* Scroll indicators */
    if (list->scroll_offset > 0) {
        ui_draw_text_centered(100, "^", COLOR_TEXT);
    }
    if (list->scroll_offset + MAX_ITEMS_VISIBLE < list->count) {
        ui_draw_text_centered(start_y + MAX_ITEMS_VISIBLE * item_height, "v", COLOR_TEXT);
    }

    /* Item count */
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d of %d", list->selected_index + 1, list->count);
    ui_draw_text_right(screen_width - 50, 90, count_str, COLOR_TEXT_DIM);
}

/* Draw media detail screen */
void ui_draw_media_detail(const media_item_t *item)
{
    if (!item) return;

    /* Thumbnail placeholder */
    ui_draw_rect(50, 120, 200, 280, COLOR_MENU_BG);
    ui_draw_text_centered_scaled(150, "?", COLOR_TEXT_DIM, 4);

    /* Title */
    ui_draw_text_scaled(280, 130, item->name, COLOR_WHITE, 2);

    /* Type badge */
    const char *type_str = item->type == MEDIA_TYPE_AUDIO ? "AUDIO" : "VIDEO";
    u32 badge_color = item->type == MEDIA_TYPE_AUDIO ? 0x4CAF50FF : 0x2196F3FF;
    int badge_w = ui_text_width(type_str, 1) + 16;
    ui_draw_rounded_rect(280, 180, badge_w, 24, 4, badge_color);
    ui_draw_text(288, 184, type_str, COLOR_WHITE);

    /* Description */
    ui_draw_text(280, 220, item->description, COLOR_TEXT);

    /* Metadata */
    char info[256];
    int mins = item->duration / 60;
    int secs = item->duration % 60;
    snprintf(info, sizeof(info), "Duration: %d:%02d", mins, secs);
    ui_draw_text(280, 280, info, COLOR_TEXT_DIM);

    if (item->size > 0) {
        snprintf(info, sizeof(info), "Size: %llu MB", (unsigned long long)(item->size / (1024*1024)));
        ui_draw_text(280, 310, info, COLOR_TEXT_DIM);
    }

    /* Action buttons */
    ui_draw_rounded_rect(280, 380, 120, 40, 6, COLOR_RED);
    ui_draw_text(310, 392, "PLAY", COLOR_WHITE);

    ui_draw_rounded_rect(420, 380, 140, 40, 6, COLOR_MENU_BG);
    ui_draw_text(440, 392, "ADD TO LIST", COLOR_TEXT);
}

/* Draw playback UI */
void ui_draw_playback(const playback_t *pb)
{
    if (!pb) return;

    /* Video area (black background) */
    ui_draw_rect(0, 0, screen_width, screen_height - 120, 0x000000FF);

    /* Overlay gradient at bottom */
    ui_draw_gradient_v(0, screen_height - 200, screen_width, 80, 0x00000000, 0x000000C0);

    /* Title */
    ui_draw_text(50, screen_height - 180, pb->title, COLOR_WHITE);

    /* Progress bar */
    int bar_x = 50;
    int bar_y = screen_height - 130;
    int bar_w = screen_width - 100;
    int bar_h = 8;

    /* Background */
    ui_draw_rect(bar_x, bar_y, bar_w, bar_h, COLOR_MENU_BG);

    /* Buffer indicator */
    if (pb->buffer_percent > 0 && pb->buffer_percent < 100) {
        int buf_w = (bar_w * pb->buffer_percent) / 100;
        ui_draw_rect(bar_x, bar_y, buf_w, bar_h, 0x444444FF);
    }

    /* Progress */
    if (pb->duration_ms > 0) {
        int progress_w = (bar_w * pb->position_ms) / pb->duration_ms;
        ui_draw_rect(bar_x, bar_y, progress_w, bar_h, COLOR_RED);

        /* Playhead */
        ui_draw_rect(bar_x + progress_w - 4, bar_y - 4, 8, bar_h + 8, COLOR_WHITE);
    }

    /* Time display */
    char time_str[64];
    int pos_min = pb->position_ms / 60000;
    int pos_sec = (pb->position_ms / 1000) % 60;
    int dur_min = pb->duration_ms / 60000;
    int dur_sec = (pb->duration_ms / 1000) % 60;

    snprintf(time_str, sizeof(time_str), "%02d:%02d", pos_min, pos_sec);
    ui_draw_text(bar_x, bar_y + 15, time_str, COLOR_WHITE);

    snprintf(time_str, sizeof(time_str), "%02d:%02d", dur_min, dur_sec);
    ui_draw_text_right(bar_x + bar_w, bar_y + 15, time_str, COLOR_WHITE);

    /* Playback status icon */
    int icon_x = screen_width / 2 - 20;
    int icon_y = bar_y + 15;
    if (pb->paused) {
        ui_draw_rect(icon_x, icon_y, 8, 20, COLOR_WHITE);
        ui_draw_rect(icon_x + 14, icon_y, 8, 20, COLOR_WHITE);
    } else if (pb->playing) {
        /* Play triangle (simplified) */
        ui_draw_text(icon_x, icon_y, ">", COLOR_WHITE);
    }

    /* Volume indicator */
    char vol_str[16];
    snprintf(vol_str, sizeof(vol_str), "Vol: %d%%", pb->volume);
    ui_draw_text_right(screen_width - 50, bar_y + 15, vol_str, COLOR_TEXT_DIM);

    /* Controls hint */
    ui_draw_footer("X:Play/Pause  O:Stop  D-Pad:Seek  L2/R2:Volume");
}

/* Draw settings screen */
void ui_draw_settings(const user_settings_t *settings, int selected_option)
{
    ui_draw_header("Settings");

    const char *options[] = {
        "Volume",
        "Video Quality",
        "Subtitles",
        "Subtitle Language",
        "Audio Language",
        "Autoplay Next",
        "Surround Sound",
        "HDR (if available)",
        "Clear History",
        "About"
    };
    int num_options = sizeof(options) / sizeof(options[0]);

    int start_y = 120;
    int item_height = 45;

    for (int i = 0; i < num_options; i++) {
        int y = start_y + i * item_height;
        bool selected = (i == selected_option);

        if (selected) {
            ui_draw_rect(40, y - 2, screen_width - 80, item_height - 4, COLOR_SELECTED);
            ui_draw_rect(40, y - 2, 4, item_height - 4, COLOR_RED);
        }

        ui_draw_text(70, y + 10, options[i], selected ? COLOR_WHITE : COLOR_TEXT);

        /* Current value */
        char value[64] = "";
        switch (i) {
            case 0: snprintf(value, sizeof(value), "%d%%", settings->volume); break;
            case 1: {
                const char *quality[] = {"SD", "HD 720p", "HD 1080p", "4K"};
                int qi = settings->video_quality;
                if (qi < 0 || qi > 3) qi = 1;
                strcpy(value, quality[qi]);
                break;
            }
            case 2: strcpy(value, settings->show_subtitles ? "On" : "Off"); break;
            case 3: strcpy(value, settings->subtitle_language); break;
            case 4: strcpy(value, settings->audio_language); break;
            case 5: strcpy(value, settings->autoplay ? "On" : "Off"); break;
            case 6: strcpy(value, settings->enable_surround ? "On" : "Off"); break;
            case 7: strcpy(value, settings->enable_hdr ? "On" : "Off"); break;
        }

        if (value[0]) {
            ui_draw_text_right(screen_width - 70, y + 10, value, COLOR_TEXT_DIM);
        }
    }

    ui_draw_footer("X:Change  O:Back  Start:Save");
}

/* Draw search screen with virtual keyboard */
void ui_draw_search(const char *query, const char *results_hint)
{
    ui_draw_header("Search");

    /* Search box */
    ui_draw_rect(50, 120, screen_width - 100, 50, COLOR_MENU_BG);
    ui_draw_rect_outline(50, 120, screen_width - 100, 50, COLOR_TEXT_DIM, 2);

    if (query && query[0]) {
        ui_draw_text(70, 135, query, COLOR_WHITE);
    } else {
        ui_draw_text(70, 135, "Enter search query...", COLOR_TEXT_DIM);
    }

    /* Cursor blink */
    if ((ui_frame_count / 30) % 2 == 0) {
        int cursor_x = 70 + (query ? strlen(query) * FONT_CHAR_W : 0);
        ui_draw_rect(cursor_x, 130, 2, 24, COLOR_WHITE);
    }

    /* Hint */
    if (results_hint) {
        ui_draw_text_centered(200, results_hint, COLOR_TEXT_DIM);
    }

    ui_draw_footer("X:Open Keyboard  O:Back  Start:Search");
}

/* Draw notification popup */
void ui_draw_notification(const char *message, int duration_frames)
{
    (void)duration_frames;

    int popup_w = ui_text_width(message, 1) + 40;
    int popup_h = 50;
    int popup_x = (screen_width - popup_w) / 2;
    int popup_y = screen_height - 150;

    ui_draw_rounded_rect(popup_x, popup_y, popup_w, popup_h, 8, COLOR_MENU_BG);
    ui_draw_rect_outline(popup_x, popup_y, popup_w, popup_h, COLOR_TEXT_DIM, 1);
    ui_draw_text(popup_x + 20, popup_y + 17, message, COLOR_WHITE);
}

/* Draw dialog box */
void ui_draw_dialog(const char *title, const char *message, const char **buttons, int button_count, int selected)
{
    int dialog_w = 500;
    int dialog_h = 200;
    int dialog_x = (screen_width - dialog_w) / 2;
    int dialog_y = (screen_height - dialog_h) / 2;

    /* Dim background */
    ui_draw_rect_alpha(0, 0, screen_width, screen_height, 0x000000FF, 180);

    /* Dialog box */
    ui_draw_rounded_rect(dialog_x, dialog_y, dialog_w, dialog_h, 10, COLOR_MENU_BG);
    ui_draw_rect_outline(dialog_x, dialog_y, dialog_w, dialog_h, COLOR_TEXT_DIM, 2);

    /* Title */
    ui_draw_text(dialog_x + 20, dialog_y + 20, title, COLOR_WHITE);
    ui_draw_hline(dialog_x + 20, dialog_y + 45, dialog_w - 40, COLOR_TEXT_DIM);

    /* Message */
    ui_draw_text(dialog_x + 20, dialog_y + 60, message, COLOR_TEXT);

    /* Buttons */
    int btn_y = dialog_y + dialog_h - 50;
    int btn_spacing = dialog_w / (button_count + 1);

    for (int i = 0; i < button_count; i++) {
        int btn_x = dialog_x + btn_spacing * (i + 1) - 50;
        bool is_selected = (i == selected);

        if (is_selected) {
            ui_draw_rounded_rect(btn_x, btn_y, 100, 35, 6, COLOR_RED);
        } else {
            ui_draw_rounded_rect(btn_x, btn_y, 100, 35, 6, COLOR_SELECTED);
        }

        int text_x = btn_x + (100 - ui_text_width(buttons[i], 1)) / 2;
        ui_draw_text(text_x, btn_y + 10, buttons[i], COLOR_WHITE);
    }
}

/* Draw on-screen keyboard (stub - uses system OSK) */
void ui_draw_osk(const char *title, char *output, int max_len)
{
    (void)title;
    (void)output;
    (void)max_len;
    printf("OSK requested: %s\n", title);
}

/* Draw image placeholder */
void ui_draw_image(int x, int y, int w, int h, void *data)
{
    (void)data;
    ui_draw_rect(x, y, w, h, COLOR_MENU_BG);
    ui_draw_text(x + w/2 - 8, y + h/2 - 8, "?", COLOR_TEXT_DIM);
}

/* Get screen dimensions */
u32 ui_get_screen_width(void)
{
    return screen_width;
}

u32 ui_get_screen_height(void)
{
    return screen_height;
}
