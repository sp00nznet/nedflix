/*
 * Nedflix Nintendo Switch - Video playback system
 * Real MPEG1 video decoding using pl_mpeg
 */

#define PL_MPEG_IMPLEMENTATION
#include "../../common/pl_mpeg.h"

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

/* Video configuration */
#define VIDEO_MAX_WIDTH     1920
#define VIDEO_MAX_HEIGHT    1080
#define NUM_VIDEO_BUFFERS   3

/* Video frame buffer */
typedef struct {
    uint32_t *data;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    bool ready;
    bool displayed;
    double pts;
} video_frame_t;

/* Video state */
static struct {
    bool initialized;
    bool playing;
    bool paused;
    bool buffering;
    bool stop_requested;
    bool eof;

    /* Stream info */
    char current_path[512];
    uint32_t position_ms;
    uint32_t duration_ms;
    int width;
    int height;
    double framerate;

    /* MPEG decoder */
    plm_t *plm;

    /* Frame buffers (triple buffering) */
    video_frame_t frames[NUM_VIDEO_BUFFERS];
    int decode_index;
    int display_index;

    /* Threading */
    Thread decode_thread;
    bool thread_running;
    Mutex frame_mutex;
    CondVar frame_cond;

    /* Subtitle overlay */
    char subtitle_text[512];
    uint64_t subtitle_start;
    uint64_t subtitle_end;
} video_state;

/* Initialize video frame buffer */
static int frame_init(video_frame_t *frame, uint32_t width, uint32_t height)
{
    size_t size = width * height * sizeof(uint32_t);
    frame->data = aligned_alloc(0x1000, size);
    if (!frame->data) return -1;

    frame->width = width;
    frame->height = height;
    frame->stride = width;
    frame->ready = false;
    frame->displayed = true;
    frame->pts = 0;

    memset(frame->data, 0, size);
    return 0;
}

/* Free video frame buffer */
static void frame_free(video_frame_t *frame)
{
    if (frame->data) {
        free(frame->data);
        frame->data = NULL;
    }
    frame->ready = false;
}

/* Decode thread function - performs real MPEG1 decoding */
static void decode_thread_func(void *arg)
{
    (void)arg;
    printf("Video decode thread started\n");

    while (video_state.thread_running && !video_state.stop_requested && !video_state.eof) {
        if (video_state.paused) {
            svcSleepThread(10000000);  /* 10ms */
            continue;
        }

        /* Get next frame buffer to decode into */
        mutexLock(&video_state.frame_mutex);

        video_frame_t *frame = &video_state.frames[video_state.decode_index];

        if (frame->ready && !frame->displayed) {
            /* Buffer full, wait for display */
            condvarWait(&video_state.frame_cond, &video_state.frame_mutex);
            mutexUnlock(&video_state.frame_mutex);
            continue;
        }

        mutexUnlock(&video_state.frame_mutex);

        /* Decode the next video frame using pl_mpeg */
        plm_frame_t *plm_frame = plm_decode_video(video_state.plm);

        if (!plm_frame) {
            /* End of video */
            video_state.eof = true;
            printf("Video decode: end of stream\n");
            break;
        }

        /* Convert YUV to RGBA and store in frame buffer */
        /* Ensure we don't overflow the buffer */
        uint32_t decode_width = plm_frame->width;
        uint32_t decode_height = plm_frame->height;

        if (decode_width > VIDEO_MAX_WIDTH) decode_width = VIDEO_MAX_WIDTH;
        if (decode_height > VIDEO_MAX_HEIGHT) decode_height = VIDEO_MAX_HEIGHT;

        plm_frame_to_rgba(plm_frame, (uint8_t *)frame->data, decode_width * 4);

        /* Mark frame as ready */
        mutexLock(&video_state.frame_mutex);

        frame->width = decode_width;
        frame->height = decode_height;
        frame->pts = plm_frame->time;
        frame->ready = true;
        frame->displayed = false;

        video_state.decode_index = (video_state.decode_index + 1) % NUM_VIDEO_BUFFERS;
        video_state.position_ms = (uint32_t)(plm_frame->time * 1000.0);

        condvarWakeAll(&video_state.frame_cond);
        mutexUnlock(&video_state.frame_mutex);

        /* Frame rate timing - sleep to match video framerate */
        if (video_state.framerate > 0) {
            uint64_t frame_time_ns = (uint64_t)(1000000000.0 / video_state.framerate);
            svcSleepThread(frame_time_ns);
        }
    }

    printf("Video decode thread ended\n");
}

/* Initialize video subsystem */
int video_init(void)
{
    printf("Initializing video subsystem...\n");

    memset(&video_state, 0, sizeof(video_state));
    video_state.framerate = 30.0;
    video_state.width = 1280;
    video_state.height = 720;

    /* Initialize frame buffers */
    for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
        if (frame_init(&video_state.frames[i], VIDEO_MAX_WIDTH, VIDEO_MAX_HEIGHT) != 0) {
            printf("Failed to allocate frame buffer %d\n", i);
            for (int j = 0; j < i; j++) {
                frame_free(&video_state.frames[j]);
            }
            return -1;
        }
    }

    /* Initialize synchronization */
    mutexInit(&video_state.frame_mutex);
    condvarInit(&video_state.frame_cond);

    video_state.initialized = true;
    printf("Video initialized\n");
    return 0;
}

/* Shutdown video */
void video_shutdown(void)
{
    if (!video_state.initialized) return;

    printf("Shutting down video...\n");

    video_stop();

    /* Free frame buffers */
    for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
        frame_free(&video_state.frames[i]);
    }

    video_state.initialized = false;
    printf("Video shutdown complete\n");
}

/* Play video from local file */
int video_play_file(const char *path)
{
    if (!video_state.initialized) {
        if (video_init() != 0) return -1;
    }

    /* Stop any current playback */
    video_stop();

    printf("Starting video file: %s\n", path);

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

    printf("Video: %dx%d @ %.2f fps, duration: %u ms\n",
           video_state.width, video_state.height,
           video_state.framerate, video_state.duration_ms);

    /* Disable audio decoding (handled by audio.c) */
    plm_set_audio_enabled(video_state.plm, FALSE);

    strncpy(video_state.current_path, path, sizeof(video_state.current_path) - 1);
    video_state.position_ms = 0;
    video_state.buffering = false;
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
    Result rc = threadCreate(&video_state.decode_thread, decode_thread_func, NULL,
                             NULL, 0x20000, 0x2B, -2);
    if (R_SUCCEEDED(rc)) {
        threadStart(&video_state.decode_thread);
    } else {
        printf("Failed to create decode thread\n");
        plm_destroy(video_state.plm);
        video_state.plm = NULL;
        return -1;
    }

    video_state.playing = true;
    video_state.paused = false;

    return 0;
}

/* Start playing video stream from URL */
int video_play_stream(const char *url)
{
    /* For streaming, we'd need to implement a custom plm_buffer
     * that reads from HTTP. For now, only local files are supported. */
    printf("Streaming not yet implemented, use local files\n");
    (void)url;
    return -1;
}

/* Stop video playback */
void video_stop(void)
{
    if (!video_state.playing && !video_state.plm) return;

    printf("Stopping video...\n");

    video_state.stop_requested = true;
    video_state.thread_running = false;

    /* Signal condition variable to wake threads */
    condvarWakeAll(&video_state.frame_cond);

    /* Wait for thread to finish */
    if (video_state.playing) {
        threadWaitForExit(&video_state.decode_thread);
        threadClose(&video_state.decode_thread);
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

    printf("Video stopped\n");
}

/* Pause video */
void video_pause(void)
{
    if (video_state.playing && !video_state.paused) {
        video_state.paused = true;
        printf("Video paused\n");
    }
}

/* Resume video */
void video_resume(void)
{
    if (video_state.playing && video_state.paused) {
        video_state.paused = false;
        printf("Video resumed\n");
    }
}

/* Seek relative to current position */
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
    mutexLock(&video_state.frame_mutex);
    for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
        video_state.frames[i].ready = false;
        video_state.frames[i].displayed = true;
    }
    video_state.position_ms = (uint32_t)new_pos;
    video_state.eof = false;
    mutexUnlock(&video_state.frame_mutex);

    printf("Video seek to %u ms\n", video_state.position_ms);
}

/* Seek to absolute position */
void video_seek_absolute(uint32_t position_ms)
{
    if (!video_state.playing || !video_state.plm) return;

    if (video_state.duration_ms > 0 && position_ms > video_state.duration_ms) {
        position_ms = video_state.duration_ms;
    }

    /* Seek in the MPEG stream */
    double seek_time = (double)position_ms / 1000.0;
    plm_seek(video_state.plm, seek_time, FALSE);

    mutexLock(&video_state.frame_mutex);
    for (int i = 0; i < NUM_VIDEO_BUFFERS; i++) {
        video_state.frames[i].ready = false;
        video_state.frames[i].displayed = true;
    }
    video_state.position_ms = position_ms;
    video_state.eof = false;
    mutexUnlock(&video_state.frame_mutex);

    printf("Video seek to %u ms\n", position_ms);
}

/* Set video track */
void video_set_track(int track)
{
    (void)track;
    /* MPEG1 typically only has one video track */
}

/* Enable/disable subtitles */
void video_set_subtitles(bool enabled)
{
    if (!enabled) {
        video_state.subtitle_text[0] = '\0';
    }
}

/* Update video (called each frame) */
void video_update(void)
{
    if (!video_state.initialized || !video_state.playing || video_state.paused) {
        return;
    }

    /* Check for end of stream */
    if (video_state.eof) {
        video_state.playing = false;
        printf("Video playback complete\n");
    }
}

/* Get current frame for display */
uint32_t *video_get_frame(uint32_t *width, uint32_t *height)
{
    if (!video_state.initialized || !video_state.playing) {
        return NULL;
    }

    mutexLock(&video_state.frame_mutex);

    /* Find a ready frame to display */
    video_frame_t *frame = &video_state.frames[video_state.display_index];

    if (frame->ready && !frame->displayed) {
        if (width) *width = frame->width;
        if (height) *height = frame->height;

        frame->displayed = true;
        video_state.display_index = (video_state.display_index + 1) % NUM_VIDEO_BUFFERS;

        condvarWakeAll(&video_state.frame_cond);
        mutexUnlock(&video_state.frame_mutex);

        return frame->data;
    }

    mutexUnlock(&video_state.frame_mutex);
    return NULL;
}

/* Check if video is playing */
bool video_is_playing(void)
{
    return video_state.playing && !video_state.paused;
}

/* Check if buffering */
bool video_is_buffering(void)
{
    return video_state.buffering;
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
void video_get_dimensions(int *width, int *height)
{
    if (width) *width = video_state.width;
    if (height) *height = video_state.height;
}

/* Get current subtitle text */
const char *video_get_subtitle(void)
{
    uint64_t now = armGetSystemTick();
    if (now >= video_state.subtitle_start && now <= video_state.subtitle_end) {
        return video_state.subtitle_text;
    }
    return NULL;
}
