/*
 * Nedflix for Nintendo GameCube - Video playback system
 * Real MPEG1 video decoding using pl_mpeg
 *
 * Hardware limitations:
 *   - 24 MB RAM (16 main + 8 ARAM)
 *   - 485 MHz PowerPC (Gekko)
 *   - Max practical resolution: 640x480
 *   - Uses LWP (lightweight processes) for threading
 */

#define PL_MPEG_IMPLEMENTATION
#include "../../common/pl_mpeg.h"

#include "nedflix.h"
#include <ogc/lwp.h>
#include <ogc/mutex.h>
#include <ogc/cond.h>

/* Video configuration */
#define VIDEO_MAX_WIDTH     640
#define VIDEO_MAX_HEIGHT    480
#define NUM_VIDEO_BUFFERS   2
#define DECODE_STACK_SIZE   (32 * 1024)

/* Video frame buffer */
typedef struct {
    uint32_t *data;
    uint32_t width;
    uint32_t height;
    double pts;
    bool ready;
    bool displayed;
} video_frame_t;

/* Video state */
static struct {
    bool initialized;
    bool playing;
    bool paused;
    bool stop_requested;
    bool eof;

    /* Stream info */
    char current_path[MAX_PATH_LENGTH];
    uint32_t position_ms;
    uint32_t duration_ms;
    int width;
    int height;
    double framerate;

    /* MPEG decoder */
    plm_t *plm;

    /* Frame buffers (double buffering) */
    video_frame_t frames[NUM_VIDEO_BUFFERS];
    int decode_index;
    int display_index;

    /* Threading */
    lwp_t decode_thread;
    uint8_t *decode_stack;
    bool thread_running;
    mutex_t frame_mutex;
    cond_t frame_cond;
} video_state;

/* Initialize a frame buffer */
static int frame_init(video_frame_t *frame)
{
    size_t size = VIDEO_MAX_WIDTH * VIDEO_MAX_HEIGHT * sizeof(uint32_t);

    /* Allocate 32-byte aligned for GX */
    frame->data = memalign(32, size);
    if (!frame->data) {
        printf("Failed to allocate video frame buffer\n");
        return -1;
    }

    frame->width = 0;
    frame->height = 0;
    frame->pts = 0;
    frame->ready = false;
    frame->displayed = true;

    memset(frame->data, 0, size);
    DCFlushRange(frame->data, size);

    return 0;
}

/* Free a frame buffer */
static void frame_free(video_frame_t *frame)
{
    if (frame->data) {
        free(frame->data);
        frame->data = NULL;
    }
    frame->ready = false;
}

/* Decode thread function - performs real MPEG1 decoding */
static void *decode_thread_func(void *arg)
{
    (void)arg;
    printf("Video decode thread started\n");

    while (video_state.thread_running && !video_state.stop_requested && !video_state.eof) {
        if (video_state.paused) {
            usleep(10000);  /* 10ms */
            continue;
        }

        /* Get next frame buffer to decode into */
        LWP_MutexLock(video_state.frame_mutex);

        video_frame_t *frame = &video_state.frames[video_state.decode_index];

        if (frame->ready && !frame->displayed) {
            /* Buffer full, wait for display */
            LWP_CondWait(video_state.frame_cond, video_state.frame_mutex);
            LWP_MutexUnlock(video_state.frame_mutex);
            continue;
        }

        LWP_MutexUnlock(video_state.frame_mutex);

        /* Decode the next video frame using pl_mpeg */
        plm_frame_t *plm_frame = plm_decode_video(video_state.plm);

        if (!plm_frame) {
            /* End of video */
            video_state.eof = true;
            printf("Video decode: end of stream\n");
            break;
        }

        /* Convert YUV to RGBA */
        /* Clamp to GameCube max resolution */
        uint32_t decode_width = plm_frame->width;
        uint32_t decode_height = plm_frame->height;

        if (decode_width > VIDEO_MAX_WIDTH) decode_width = VIDEO_MAX_WIDTH;
        if (decode_height > VIDEO_MAX_HEIGHT) decode_height = VIDEO_MAX_HEIGHT;

        plm_frame_to_rgba(plm_frame, (uint8_t *)frame->data, decode_width * 4);

        /* Flush data cache for GX */
        DCFlushRange(frame->data, decode_width * decode_height * 4);

        /* Mark frame as ready */
        LWP_MutexLock(video_state.frame_mutex);

        frame->width = decode_width;
        frame->height = decode_height;
        frame->pts = plm_frame->time;
        frame->ready = true;
        frame->displayed = false;

        video_state.decode_index = (video_state.decode_index + 1) % NUM_VIDEO_BUFFERS;
        video_state.position_ms = (uint32_t)(plm_frame->time * 1000.0);

        LWP_CondSignal(video_state.frame_cond);
        LWP_MutexUnlock(video_state.frame_mutex);

        /* Frame rate timing */
        if (video_state.framerate > 0) {
            usleep((useconds_t)(1000000.0 / video_state.framerate));
        }
    }

    printf("Video decode thread ended\n");
    return NULL;
}

/* Initialize video subsystem */
int video_init(void)
{
    printf("Initializing video subsystem...\n");

    memset(&video_state, 0, sizeof(video_state));
    video_state.framerate = 24.0;  /* Standard film framerate */

    /* Initialize frame buffers */
    for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
        if (frame_init(&video_state.frames[i]) != 0) {
            printf("Failed to allocate frame buffer %d\n", i);
            for (int j = 0; j < i; j++) {
                frame_free(&video_state.frames[j]);
            }
            return -1;
        }
    }

    /* Allocate decode thread stack */
    video_state.decode_stack = memalign(32, DECODE_STACK_SIZE);
    if (!video_state.decode_stack) {
        printf("Failed to allocate decode thread stack\n");
        for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
            frame_free(&video_state.frames[i]);
        }
        return -1;
    }

    /* Initialize synchronization */
    LWP_MutexInit(&video_state.frame_mutex, FALSE);
    LWP_CondInit(&video_state.frame_cond);

    video_state.initialized = true;
    printf("Video initialized\n");
    return 0;
}

/* Shutdown video */
void video_shutdown(void)
{
    if (!video_state.initialized) return;

    printf("Shutting down video...\n");

    video_stop(&g_app.playback);

    /* Free frame buffers */
    for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
        frame_free(&video_state.frames[i]);
    }

    /* Free decode stack */
    if (video_state.decode_stack) {
        free(video_state.decode_stack);
        video_state.decode_stack = NULL;
    }

    /* Destroy synchronization primitives */
    LWP_MutexDestroy(video_state.frame_mutex);
    LWP_CondDestroy(video_state.frame_cond);

    video_state.initialized = false;
    printf("Video shutdown complete\n");
}

/* Load and play an MPEG video file */
int video_load_mjpeg(const char *path, playback_state_t *state)
{
    if (!video_state.initialized) {
        if (video_init() != 0) return -1;
    }

    /* Stop any current playback */
    video_stop(state);

    printf("Loading video: %s\n", path);

    /* Open the MPEG file with pl_mpeg */
    video_state.plm = plm_create_with_filename(path);
    if (!video_state.plm) {
        printf("Failed to open video file: %s\n", path);
        return -1;
    }

    /* Get video properties */
    video_state.width = plm_get_width(video_state.plm);
    video_state.height = plm_get_height(video_state.plm);
    video_state.framerate = plm_get_framerate(video_state.plm);
    video_state.duration_ms = (uint32_t)(plm_get_duration(video_state.plm) * 1000.0);

    /* Check if video fits in GameCube memory */
    if (video_state.width > VIDEO_MAX_WIDTH || video_state.height > VIDEO_MAX_HEIGHT) {
        printf("Warning: Video %dx%d exceeds GameCube max %dx%d\n",
               video_state.width, video_state.height,
               VIDEO_MAX_WIDTH, VIDEO_MAX_HEIGHT);
        printf("Video will be cropped\n");
    }

    printf("Video: %dx%d @ %.2f fps, duration: %u ms\n",
           video_state.width, video_state.height,
           video_state.framerate, video_state.duration_ms);

    /* Disable audio in pl_mpeg (handled by audio.c) */
    plm_set_audio_enabled(video_state.plm, FALSE);

    /* Update playback state */
    strncpy(video_state.current_path, path, sizeof(video_state.current_path) - 1);
    strncpy(state->current_file, path, sizeof(state->current_file) - 1);
    state->duration_ms = video_state.duration_ms;
    state->video_width = video_state.width;
    state->video_height = video_state.height;
    state->has_video = true;

    video_state.position_ms = 0;
    video_state.stop_requested = false;
    video_state.eof = false;
    video_state.thread_running = true;

    /* Reset frame buffers */
    video_state.decode_index = 0;
    video_state.display_index = 0;

    for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
        video_state.frames[i].ready = false;
        video_state.frames[i].displayed = true;
    }

    /* Start decode thread */
    if (LWP_CreateThread(&video_state.decode_thread, decode_thread_func, NULL,
                         video_state.decode_stack, DECODE_STACK_SIZE, 60) != 0) {
        printf("Failed to create decode thread\n");
        plm_destroy(video_state.plm);
        video_state.plm = NULL;
        return -1;
    }

    video_state.playing = true;
    video_state.paused = false;
    state->is_playing = true;

    return 0;
}

/* Stop video playback */
void video_stop(playback_state_t *state)
{
    if (!video_state.playing && !video_state.plm) return;

    printf("Stopping video...\n");

    video_state.stop_requested = true;
    video_state.thread_running = false;

    /* Signal condition variable to wake thread */
    LWP_CondSignal(video_state.frame_cond);

    /* Wait for thread to finish */
    if (video_state.playing) {
        LWP_JoinThread(video_state.decode_thread, NULL);
    }

    /* Clean up pl_mpeg decoder */
    if (video_state.plm) {
        plm_destroy(video_state.plm);
        video_state.plm = NULL;
    }

    video_state.playing = false;
    video_state.paused = false;
    video_state.position_ms = 0;
    video_state.eof = false;
    video_state.current_path[0] = '\0';

    if (state) {
        state->is_playing = false;
        state->has_video = false;
    }

    printf("Video stopped\n");
}

/* Pause video */
void video_pause(void)
{
    if (video_state.playing && !video_state.paused) {
        video_state.paused = true;
    }
}

/* Resume video */
void video_resume(void)
{
    if (video_state.playing && video_state.paused) {
        video_state.paused = false;
    }
}

/* Seek to position */
void video_seek(int offset_ms)
{
    if (!video_state.playing || !video_state.plm) return;

    int new_pos = (int)video_state.position_ms + offset_ms;
    if (new_pos < 0) new_pos = 0;
    if (video_state.duration_ms > 0 && new_pos > (int)video_state.duration_ms) {
        new_pos = video_state.duration_ms;
    }

    /* Seek in the MPEG stream */
    double seek_time = (double)new_pos / 1000.0;
    plm_seek(video_state.plm, seek_time, FALSE);

    /* Clear decode buffers */
    LWP_MutexLock(video_state.frame_mutex);
    for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
        video_state.frames[i].ready = false;
        video_state.frames[i].displayed = true;
    }
    video_state.position_ms = (uint32_t)new_pos;
    video_state.eof = false;
    LWP_MutexUnlock(video_state.frame_mutex);

    printf("Video seek to %u ms\n", video_state.position_ms);
}

/* Update video state (called each frame from main loop) */
void video_update(void)
{
    if (!video_state.initialized || !video_state.playing || video_state.paused) {
        return;
    }

    /* Check for end of stream */
    if (video_state.eof) {
        video_state.playing = false;
        g_app.playback.is_playing = false;
        printf("Video playback complete\n");
    }
}

/* Get current frame for display */
void *video_get_frame(uint32_t *width, uint32_t *height)
{
    if (!video_state.initialized || !video_state.playing) {
        return NULL;
    }

    LWP_MutexLock(video_state.frame_mutex);

    /* Find a ready frame to display */
    video_frame_t *frame = &video_state.frames[video_state.display_index];

    if (frame->ready && !frame->displayed) {
        if (width) *width = frame->width;
        if (height) *height = frame->height;

        frame->displayed = true;
        video_state.display_index = (video_state.display_index + 1) % NUM_VIDEO_BUFFERS;

        LWP_CondSignal(video_state.frame_cond);
        LWP_MutexUnlock(video_state.frame_mutex);

        return frame->data;
    }

    LWP_MutexUnlock(video_state.frame_mutex);
    return NULL;
}

/* Check if video is playing */
bool video_is_playing(void)
{
    return video_state.playing && !video_state.paused;
}

/* Check if video is paused */
bool video_is_paused(void)
{
    return video_state.paused;
}

/* Get current position in milliseconds */
uint32_t video_get_position(void)
{
    return video_state.position_ms;
}

/* Get duration in milliseconds */
uint32_t video_get_duration(void)
{
    return video_state.duration_ms;
}

/* Get video dimensions */
void video_get_dimensions(int *w, int *h)
{
    if (w) *w = video_state.width;
    if (h) *h = video_state.height;
}
