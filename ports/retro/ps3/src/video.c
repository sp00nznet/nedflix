/*
 * Nedflix PS3 - Video playback with real MPEG1 decoding
 * Uses pl_mpeg for MPEG1 video/audio decoding
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/thread.h>
#include <sys/mutex.h>
#include <sys/cond.h>
#include <sys/timer.h>
#include <sysutil/sysutil.h>

/* Include pl_mpeg implementation */
#define PL_MPEG_IMPLEMENTATION
#include "../../common/pl_mpeg.h"

/* Video configuration */
#define VIDEO_MAX_WIDTH      1920
#define VIDEO_MAX_HEIGHT     1080
#define VIDEO_FRAME_SIZE     (VIDEO_MAX_WIDTH * VIDEO_MAX_HEIGHT * 4)  /* RGBA */
#define VIDEO_BUFFER_FRAMES  3  /* Triple buffering */
#define VIDEO_THREAD_STACK   (128 * 1024)
#define VIDEO_THREAD_PRIO    1001
#define VIDEO_FILE_BUFFER    (2 * 1024 * 1024)  /* 2MB file buffer */

/* Frame buffer state */
typedef enum {
    FRAME_EMPTY = 0,
    FRAME_FILLING,
    FRAME_READY,
    FRAME_DISPLAYING
} frame_state_t;

/* Frame buffer */
typedef struct {
    u8 *data;
    frame_state_t state;
    double pts;
    u32 width;
    u32 height;
} frame_buffer_t;

/* Video decoder state */
typedef struct {
    bool initialized;
    bool playing;
    bool paused;
    bool seeking;
    bool eof;

    /* File info */
    char path[512];
    u32 duration_ms;
    u32 position_ms;
    u32 width;
    u32 height;
    float framerate;

    /* pl_mpeg decoder */
    plm_t *plm;

    /* Frame buffers */
    frame_buffer_t frames[VIDEO_BUFFER_FRAMES];
    int current_frame;
    int display_frame;

    /* Threading */
    sys_ppu_thread_t decode_thread;
    bool decode_running;
    sys_mutex_t state_mutex;
    sys_cond_t frame_ready;

    /* Timing */
    u64 start_time;
    u64 pause_time;
    u64 frame_duration;
    u64 last_frame_time;

    /* Statistics */
    u32 frames_decoded;
    u32 frames_dropped;

} video_state_t;

static video_state_t g_video;

/* Forward declarations */
static void video_decode_thread(void *arg);
static int frame_buffer_init(frame_buffer_t *frame);
static void frame_buffer_free(frame_buffer_t *frame);

/* Initialize frame buffer */
static int frame_buffer_init(frame_buffer_t *frame)
{
    frame->data = (u8 *)memalign(128, VIDEO_FRAME_SIZE);
    if (!frame->data) {
        printf("Video: Failed to allocate frame buffer\n");
        return -1;
    }

    frame->state = FRAME_EMPTY;
    frame->pts = 0;
    frame->width = 0;
    frame->height = 0;

    memset(frame->data, 0, VIDEO_FRAME_SIZE);
    return 0;
}

/* Free frame buffer */
static void frame_buffer_free(frame_buffer_t *frame)
{
    if (frame->data) {
        free(frame->data);
        frame->data = NULL;
    }
    frame->state = FRAME_EMPTY;
}

/* Initialize video subsystem */
int video_init(void)
{
    if (g_video.initialized) return 0;

    printf("Video: Initializing PS3 video decoder...\n");
    memset(&g_video, 0, sizeof(video_state_t));

    /* Initialize frame buffers */
    for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
        if (frame_buffer_init(&g_video.frames[i]) != 0) {
            for (int j = 0; j < i; j++) {
                frame_buffer_free(&g_video.frames[j]);
            }
            return -1;
        }
    }

    /* Create state mutex and condition */
    sys_mutex_attribute_t mutex_attr;
    sys_mutex_attribute_initialize(mutex_attr);
    sys_mutex_create(&g_video.state_mutex, &mutex_attr);

    sys_cond_attribute_t cond_attr;
    sys_cond_attribute_initialize(cond_attr);
    sys_cond_create(&g_video.frame_ready, g_video.state_mutex, &cond_attr);

    g_video.initialized = true;
    g_video.display_frame = -1;

    printf("Video: Initialized with %d frame buffers\n", VIDEO_BUFFER_FRAMES);
    return 0;
}

/* Shutdown video subsystem */
void video_shutdown(void)
{
    if (!g_video.initialized) return;

    printf("Video: Shutting down...\n");
    video_stop();

    for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
        frame_buffer_free(&g_video.frames[i]);
    }

    sys_cond_destroy(g_video.frame_ready);
    sys_mutex_destroy(g_video.state_mutex);

    g_video.initialized = false;
    printf("Video: Shutdown complete\n");
}

/* Decode thread - decodes frames using pl_mpeg */
static void video_decode_thread(void *arg)
{
    (void)arg;
    printf("Video: Decode thread started\n");

    while (g_video.decode_running && !g_video.eof) {
        if (g_video.paused) {
            sys_timer_usleep(10000);
            continue;
        }

        /* Find empty frame buffer */
        int target_frame = -1;

        sys_mutex_lock(g_video.state_mutex, 0);
        for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
            if (g_video.frames[i].state == FRAME_EMPTY) {
                target_frame = i;
                g_video.frames[i].state = FRAME_FILLING;
                break;
            }
        }
        sys_mutex_unlock(g_video.state_mutex);

        if (target_frame < 0) {
            sys_timer_usleep(5000);
            continue;
        }

        /* Decode next frame */
        plm_frame_t *plm_frame = plm_decode_video(g_video.plm);

        if (!plm_frame) {
            /* End of video or error */
            sys_mutex_lock(g_video.state_mutex, 0);
            g_video.frames[target_frame].state = FRAME_EMPTY;
            sys_mutex_unlock(g_video.state_mutex);

            if (plm_has_ended(g_video.plm)) {
                g_video.eof = true;
            }
            break;
        }

        /* Convert YUV to RGBA */
        frame_buffer_t *frame = &g_video.frames[target_frame];
        frame->width = plm_frame->width;
        frame->height = plm_frame->height;
        frame->pts = plm_frame->time;

        plm_frame_to_rgba(plm_frame, frame->data, frame->width * 4);

        /* Mark frame as ready */
        sys_mutex_lock(g_video.state_mutex, 0);
        frame->state = FRAME_READY;
        g_video.frames_decoded++;
        sys_cond_signal(g_video.frame_ready);
        sys_mutex_unlock(g_video.state_mutex);
    }

    printf("Video: Decode thread exiting (decoded %u frames)\n", g_video.frames_decoded);
    g_video.decode_running = false;
    sys_ppu_thread_exit(0);
}

/* Load and play video file */
int video_play_file(const char *path)
{
    if (!g_video.initialized) {
        if (video_init() != 0) return -1;
    }

    if (g_video.playing) {
        video_stop();
    }

    printf("Video: Loading %s\n", path);

    /* Open file with pl_mpeg */
    g_video.plm = plm_create_with_filename(path);
    if (!g_video.plm) {
        printf("Video: Failed to open file\n");
        return -1;
    }

    /* Probe for video info */
    if (!plm_probe(g_video.plm, 1024 * 1024)) {
        printf("Video: Failed to probe video\n");
        plm_destroy(g_video.plm);
        g_video.plm = NULL;
        return -1;
    }

    /* Get video info */
    g_video.width = plm_get_width(g_video.plm);
    g_video.height = plm_get_height(g_video.plm);
    g_video.framerate = plm_get_framerate(g_video.plm);
    g_video.duration_ms = (u32)(plm_get_duration(g_video.plm) * 1000);

    if (g_video.width == 0 || g_video.height == 0) {
        printf("Video: Invalid video dimensions\n");
        plm_destroy(g_video.plm);
        g_video.plm = NULL;
        return -1;
    }

    /* Check dimensions */
    if (g_video.width > VIDEO_MAX_WIDTH || g_video.height > VIDEO_MAX_HEIGHT) {
        printf("Video: Resolution too high (%dx%d)\n", g_video.width, g_video.height);
        plm_destroy(g_video.plm);
        g_video.plm = NULL;
        return -1;
    }

    strncpy(g_video.path, path, sizeof(g_video.path) - 1);
    g_video.frame_duration = (u64)(1000000.0 / g_video.framerate);

    printf("Video: Loaded %dx%d @ %.2f fps\n",
           g_video.width, g_video.height, g_video.framerate);

    /* Disable audio in pl_mpeg (handled separately) */
    plm_set_audio_enabled(g_video.plm, FALSE);

    /* Reset state */
    g_video.playing = true;
    g_video.paused = false;
    g_video.eof = false;
    g_video.position_ms = 0;
    g_video.current_frame = 0;
    g_video.display_frame = -1;
    g_video.frames_decoded = 0;
    g_video.frames_dropped = 0;

    /* Reset frame buffers */
    for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
        g_video.frames[i].state = FRAME_EMPTY;
    }

    /* Start decode thread */
    g_video.decode_running = true;
    sys_ppu_thread_create(&g_video.decode_thread, video_decode_thread, NULL,
        VIDEO_THREAD_PRIO, VIDEO_THREAD_STACK, SYS_PPU_THREAD_CREATE_JOINABLE, "VidDecode");

    /* Record start time */
    g_video.start_time = sys_time_get_system_time();
    g_video.last_frame_time = g_video.start_time;

    printf("Video: Playback started\n");
    return 0;
}

/* Stop video playback */
void video_stop(void)
{
    if (!g_video.playing && !g_video.plm) return;

    printf("Video: Stopping playback...\n");

    g_video.decode_running = false;
    g_video.eof = true;

    /* Wake up any waiting threads */
    sys_mutex_lock(g_video.state_mutex, 0);
    sys_cond_broadcast(g_video.frame_ready);
    sys_mutex_unlock(g_video.state_mutex);

    /* Wait for decode thread */
    if (g_video.decode_thread) {
        u64 exit_code;
        sys_ppu_thread_join(g_video.decode_thread, &exit_code);
        g_video.decode_thread = 0;
    }

    /* Clean up pl_mpeg */
    if (g_video.plm) {
        plm_destroy(g_video.plm);
        g_video.plm = NULL;
    }

    g_video.playing = false;
    g_video.paused = false;
    g_video.path[0] = '\0';

    printf("Video: Stopped (decoded: %u, dropped: %u)\n",
           g_video.frames_decoded, g_video.frames_dropped);
}

/* Pause video playback */
void video_pause(void)
{
    if (g_video.playing && !g_video.paused) {
        g_video.paused = true;
        g_video.pause_time = sys_time_get_system_time();
        printf("Video: Paused at %u ms\n", g_video.position_ms);
    }
}

/* Resume video playback */
void video_resume(void)
{
    if (g_video.playing && g_video.paused) {
        u64 pause_duration = sys_time_get_system_time() - g_video.pause_time;
        g_video.start_time += pause_duration;
        g_video.paused = false;
        printf("Video: Resumed\n");
    }
}

/* Seek to position */
void video_seek(int offset_ms)
{
    if (!g_video.playing || !g_video.plm) return;

    int new_pos = (int)g_video.position_ms + offset_ms;
    if (new_pos < 0) new_pos = 0;
    if (new_pos > (int)g_video.duration_ms) new_pos = g_video.duration_ms;

    double target_time = new_pos / 1000.0;

    g_video.seeking = true;

    /* Seek in pl_mpeg */
    if (plm_seek(g_video.plm, target_time, FALSE)) {
        g_video.position_ms = (u32)new_pos;
        g_video.start_time = sys_time_get_system_time() - (g_video.position_ms * 1000);

        /* Clear frame buffers */
        sys_mutex_lock(g_video.state_mutex, 0);
        for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
            if (g_video.frames[i].state != FRAME_DISPLAYING) {
                g_video.frames[i].state = FRAME_EMPTY;
            }
        }
        sys_mutex_unlock(g_video.state_mutex);
    }

    g_video.seeking = false;
    printf("Video: Seeked to %u ms\n", g_video.position_ms);
}

/* Seek to absolute position */
void video_seek_absolute(u32 position_ms)
{
    if (!g_video.playing) return;
    video_seek((int)position_ms - (int)g_video.position_ms);
}

/* Check if video is playing */
bool video_is_playing(void)
{
    return g_video.playing && !g_video.paused;
}

/* Check if video is paused */
bool video_is_paused(void)
{
    return g_video.playing && g_video.paused;
}

/* Get playback position in milliseconds */
u32 video_get_position(void)
{
    return g_video.position_ms;
}

/* Get video duration in milliseconds */
u32 video_get_duration(void)
{
    return g_video.duration_ms;
}

/* Get video width */
int video_get_width(void)
{
    return g_video.width;
}

/* Get video height */
int video_get_height(void)
{
    return g_video.height;
}

/* Get current frame for display */
u8 *video_get_current_frame(void)
{
    if (!g_video.playing || g_video.display_frame < 0) {
        return NULL;
    }
    return g_video.frames[g_video.display_frame].data;
}

/* Get current frame dimensions */
void video_get_frame_size(u32 *width, u32 *height)
{
    if (g_video.display_frame >= 0) {
        if (width) *width = g_video.frames[g_video.display_frame].width;
        if (height) *height = g_video.frames[g_video.display_frame].height;
    } else {
        if (width) *width = g_video.width;
        if (height) *height = g_video.height;
    }
}

/* Render current video frame - call each frame */
void video_render_frame(void)
{
    if (!g_video.playing || g_video.paused) return;

    u64 current_time = sys_time_get_system_time();
    u64 elapsed = current_time - g_video.start_time;
    g_video.position_ms = (u32)(elapsed / 1000);

    /* Check for end of playback */
    if (g_video.eof) {
        if (g_video.duration_ms > 0 && g_video.position_ms >= g_video.duration_ms) {
            g_video.playing = false;
            printf("Video: Playback complete\n");
            return;
        }
    }

    /* Frame timing */
    u64 time_since_last = current_time - g_video.last_frame_time;
    if (time_since_last < g_video.frame_duration) {
        return;
    }

    /* Find next ready frame */
    sys_mutex_lock(g_video.state_mutex, 0);

    /* Release previous display frame */
    if (g_video.display_frame >= 0) {
        g_video.frames[g_video.display_frame].state = FRAME_EMPTY;
    }

    /* Find frame with closest PTS */
    int next_frame = -1;
    double target_pts = elapsed / 1000000.0;

    for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
        if (g_video.frames[i].state == FRAME_READY) {
            if (next_frame < 0 ||
                (g_video.frames[i].pts <= target_pts &&
                 g_video.frames[i].pts > g_video.frames[next_frame].pts)) {
                next_frame = i;
            }
        }
    }

    if (next_frame >= 0) {
        g_video.frames[next_frame].state = FRAME_DISPLAYING;
        g_video.display_frame = next_frame;
        g_video.last_frame_time = current_time;

        /* Drop late frames */
        for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
            if (i != next_frame && g_video.frames[i].state == FRAME_READY) {
                double frame_deadline = g_video.frames[i].pts + (g_video.frame_duration / 1000000.0);
                if (frame_deadline < target_pts) {
                    g_video.frames[i].state = FRAME_EMPTY;
                    g_video.frames_dropped++;
                }
            }
        }
    }

    sys_mutex_unlock(g_video.state_mutex);
}

/* Get video statistics */
void video_get_stats(u32 *decoded, u32 *dropped)
{
    if (decoded) *decoded = g_video.frames_decoded;
    if (dropped) *dropped = g_video.frames_dropped;
}

/* Check if video has ended */
bool video_has_ended(void)
{
    return g_video.eof && !g_video.decode_running;
}

/* Get framerate */
float video_get_framerate(void)
{
    return g_video.framerate;
}
