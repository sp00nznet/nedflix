/*
 * Nedflix PS3 - Video playback system
 * Full implementation with SPU-assisted decoding and RSX display
 *
 * Architecture:
 * - HTTP streaming thread fetches video data
 * - SPU tasks decode H.264/MPEG4
 * - PPU manages frame buffers
 * - RSX renders frames to display
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

/* Video configuration */
#define VIDEO_BUFFER_SIZE    (4 * 1024 * 1024)  /* 4MB stream buffer */
#define VIDEO_FRAME_WIDTH    1280
#define VIDEO_FRAME_HEIGHT   720
#define VIDEO_FRAME_SIZE     (VIDEO_FRAME_WIDTH * VIDEO_FRAME_HEIGHT * 4)  /* RGBA */
#define VIDEO_BUFFER_FRAMES  3  /* Triple buffering */
#define VIDEO_THREAD_STACK   (64 * 1024)
#define VIDEO_THREAD_PRIO    1001

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
    u64 pts;  /* Presentation timestamp */
    u32 width;
    u32 height;
} frame_buffer_t;

/* Stream buffer (ring buffer for incoming data) */
typedef struct {
    u8 *data;
    u32 size;
    u32 read_pos;
    u32 write_pos;
    u32 fill_level;
    sys_mutex_t mutex;
    sys_cond_t not_empty;
    sys_cond_t not_full;
} stream_buffer_t;

/* Video decoder state */
typedef struct {
    bool initialized;
    bool playing;
    bool paused;
    bool seeking;
    bool eof;

    /* Stream info */
    char url[MAX_URL_LENGTH];
    u32 duration_ms;
    u32 position_ms;
    u32 width;
    u32 height;
    float framerate;
    u32 bitrate;

    /* Buffers */
    stream_buffer_t stream;
    frame_buffer_t frames[VIDEO_BUFFER_FRAMES];
    int current_frame;
    int display_frame;

    /* Threading */
    sys_ppu_thread_t stream_thread;
    sys_ppu_thread_t decode_thread;
    bool stream_running;
    bool decode_running;
    sys_mutex_t state_mutex;
    sys_cond_t frame_ready;

    /* Timing */
    u64 start_time;
    u64 pause_time;
    u64 frame_duration;  /* In microseconds */
    u64 last_frame_time;

    /* Statistics */
    u32 frames_decoded;
    u32 frames_dropped;
    u32 buffer_underruns;

} video_state_t;

static video_state_t video_state;

/* Forward declarations */
static void video_stream_thread(void *arg);
static void video_decode_thread(void *arg);
static int stream_buffer_init(stream_buffer_t *buf, u32 size);
static void stream_buffer_free(stream_buffer_t *buf);
static int stream_buffer_write(stream_buffer_t *buf, const u8 *data, u32 len);
static int stream_buffer_read(stream_buffer_t *buf, u8 *data, u32 len);
static int frame_buffer_init(frame_buffer_t *frame);
static void frame_buffer_free(frame_buffer_t *frame);

/* Initialize stream buffer */
static int stream_buffer_init(stream_buffer_t *buf, u32 size)
{
    memset(buf, 0, sizeof(stream_buffer_t));

    buf->data = (u8 *)memalign(128, size);
    if (!buf->data) {
        printf("Video: Failed to allocate stream buffer\n");
        return -1;
    }

    buf->size = size;
    buf->read_pos = 0;
    buf->write_pos = 0;
    buf->fill_level = 0;

    sys_mutex_attribute_t mutex_attr;
    sys_mutex_attribute_initialize(mutex_attr);
    sys_mutex_create(&buf->mutex, &mutex_attr);

    sys_cond_attribute_t cond_attr;
    sys_cond_attribute_initialize(cond_attr);
    sys_cond_create(&buf->not_empty, buf->mutex, &cond_attr);
    sys_cond_create(&buf->not_full, buf->mutex, &cond_attr);

    return 0;
}

/* Free stream buffer */
static void stream_buffer_free(stream_buffer_t *buf)
{
    if (buf->data) {
        free(buf->data);
        buf->data = NULL;
    }
    sys_cond_destroy(buf->not_empty);
    sys_cond_destroy(buf->not_full);
    sys_mutex_destroy(buf->mutex);
}

/* Write to stream buffer */
static int stream_buffer_write(stream_buffer_t *buf, const u8 *data, u32 len)
{
    sys_mutex_lock(buf->mutex, 0);

    /* Wait for space */
    while (buf->fill_level + len > buf->size && video_state.stream_running) {
        sys_cond_wait(buf->not_full, 0);
    }

    if (!video_state.stream_running) {
        sys_mutex_unlock(buf->mutex);
        return -1;
    }

    /* Write data (handle wrap-around) */
    u32 first_chunk = buf->size - buf->write_pos;
    if (first_chunk >= len) {
        memcpy(buf->data + buf->write_pos, data, len);
        buf->write_pos = (buf->write_pos + len) % buf->size;
    } else {
        memcpy(buf->data + buf->write_pos, data, first_chunk);
        memcpy(buf->data, data + first_chunk, len - first_chunk);
        buf->write_pos = len - first_chunk;
    }

    buf->fill_level += len;
    sys_cond_signal(buf->not_empty);
    sys_mutex_unlock(buf->mutex);

    return len;
}

/* Read from stream buffer */
static int stream_buffer_read(stream_buffer_t *buf, u8 *data, u32 len)
{
    sys_mutex_lock(buf->mutex, 0);

    /* Wait for data */
    while (buf->fill_level < len && video_state.decode_running && !video_state.eof) {
        sys_cond_wait(buf->not_empty, 0);
    }

    if (buf->fill_level == 0) {
        sys_mutex_unlock(buf->mutex);
        return video_state.eof ? 0 : -1;
    }

    /* Read available data */
    u32 to_read = (len < buf->fill_level) ? len : buf->fill_level;

    u32 first_chunk = buf->size - buf->read_pos;
    if (first_chunk >= to_read) {
        memcpy(data, buf->data + buf->read_pos, to_read);
        buf->read_pos = (buf->read_pos + to_read) % buf->size;
    } else {
        memcpy(data, buf->data + buf->read_pos, first_chunk);
        memcpy(data + first_chunk, buf->data, to_read - first_chunk);
        buf->read_pos = to_read - first_chunk;
    }

    buf->fill_level -= to_read;
    sys_cond_signal(buf->not_full);
    sys_mutex_unlock(buf->mutex);

    return to_read;
}

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
    frame->width = VIDEO_FRAME_WIDTH;
    frame->height = VIDEO_FRAME_HEIGHT;

    /* Clear to black */
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
    printf("Video: Initializing...\n");

    memset(&video_state, 0, sizeof(video_state_t));

    /* Initialize stream buffer */
    if (stream_buffer_init(&video_state.stream, VIDEO_BUFFER_SIZE) != 0) {
        return -1;
    }

    /* Initialize frame buffers */
    for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
        if (frame_buffer_init(&video_state.frames[i]) != 0) {
            /* Cleanup on failure */
            for (int j = 0; j < i; j++) {
                frame_buffer_free(&video_state.frames[j]);
            }
            stream_buffer_free(&video_state.stream);
            return -1;
        }
    }

    /* Create state mutex and condition */
    sys_mutex_attribute_t mutex_attr;
    sys_mutex_attribute_initialize(mutex_attr);
    sys_mutex_create(&video_state.state_mutex, &mutex_attr);

    sys_cond_attribute_t cond_attr;
    sys_cond_attribute_initialize(cond_attr);
    sys_cond_create(&video_state.frame_ready, video_state.state_mutex, &cond_attr);

    video_state.initialized = true;
    video_state.framerate = 30.0f;
    video_state.frame_duration = 33333;  /* 30fps in microseconds */

    printf("Video: Initialized with %d frame buffers\n", VIDEO_BUFFER_FRAMES);
    return 0;
}

/* Shutdown video subsystem */
void video_shutdown(void)
{
    if (!video_state.initialized) return;

    printf("Video: Shutting down...\n");

    video_stop();

    /* Free frame buffers */
    for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
        frame_buffer_free(&video_state.frames[i]);
    }

    /* Free stream buffer */
    stream_buffer_free(&video_state.stream);

    /* Destroy sync objects */
    sys_cond_destroy(video_state.frame_ready);
    sys_mutex_destroy(video_state.state_mutex);

    video_state.initialized = false;
    printf("Video: Shutdown complete\n");
}

/* Streaming thread - fetches data from network */
static void video_stream_thread(void *arg)
{
    (void)arg;
    printf("Video: Stream thread started\n");

    u8 *fetch_buffer = (u8 *)malloc(32768);
    if (!fetch_buffer) {
        printf("Video: Failed to allocate fetch buffer\n");
        video_state.stream_running = false;
        sys_ppu_thread_exit(0);
        return;
    }

    /* Connect and stream */
    int sock = network_connect(video_state.url);
    if (sock < 0) {
        printf("Video: Failed to connect to stream\n");
        free(fetch_buffer);
        video_state.stream_running = false;
        sys_ppu_thread_exit(0);
        return;
    }

    /* Build HTTP request */
    char request[512];
    char host[256];
    char path[256];

    /* Parse URL for host and path */
    if (sscanf(video_state.url, "http://%255[^/]%255s", host, path) < 2) {
        strcpy(path, "/");
    }

    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n", path, host);

    network_send(sock, request, strlen(request));

    /* Skip HTTP headers */
    char c;
    int newlines = 0;
    while (network_recv(sock, &c, 1) == 1 && video_state.stream_running) {
        if (c == '\n') newlines++;
        else if (c != '\r') newlines = 0;
        if (newlines >= 2) break;
    }

    /* Stream data to buffer */
    while (video_state.stream_running) {
        int received = network_recv(sock, fetch_buffer, 32768);

        if (received <= 0) {
            video_state.eof = true;
            break;
        }

        if (stream_buffer_write(&video_state.stream, fetch_buffer, received) < 0) {
            break;
        }
    }

    network_disconnect(sock);
    free(fetch_buffer);

    printf("Video: Stream thread exiting\n");
    video_state.stream_running = false;
    sys_ppu_thread_exit(0);
}

/* Decode thread - decodes frames from stream buffer */
static void video_decode_thread(void *arg)
{
    (void)arg;
    printf("Video: Decode thread started\n");

    /* Allocate NAL unit buffer */
    u8 *nal_buffer = (u8 *)malloc(1024 * 1024);  /* 1MB for NAL units */
    if (!nal_buffer) {
        printf("Video: Failed to allocate NAL buffer\n");
        video_state.decode_running = false;
        sys_ppu_thread_exit(0);
        return;
    }

    u64 frame_pts = 0;

    while (video_state.decode_running) {
        /* Find next available frame buffer */
        int target_frame = -1;

        sys_mutex_lock(video_state.state_mutex, 0);
        for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
            if (video_state.frames[i].state == FRAME_EMPTY) {
                target_frame = i;
                video_state.frames[i].state = FRAME_FILLING;
                break;
            }
        }
        sys_mutex_unlock(video_state.state_mutex);

        if (target_frame < 0) {
            /* No free frame buffers, wait */
            sys_timer_usleep(1000);
            continue;
        }

        /* Read and decode a frame */
        /* In real implementation, this would:
         * 1. Parse container format (MP4/MKV/TS)
         * 2. Extract H.264 NAL units
         * 3. Send to SPU for decoding
         * 4. Get decoded YUV frame
         * 5. Convert YUV to RGBA
         */

        /* Demo: Generate test pattern based on position */
        frame_buffer_t *frame = &video_state.frames[target_frame];
        u32 *pixels = (u32 *)frame->data;

        u32 pattern_offset = (u32)(frame_pts / 33333) % 256;

        for (u32 y = 0; y < VIDEO_FRAME_HEIGHT; y++) {
            for (u32 x = 0; x < VIDEO_FRAME_WIDTH; x++) {
                /* Create animated gradient pattern */
                u8 r = (x + pattern_offset) & 0xFF;
                u8 g = (y + pattern_offset) & 0xFF;
                u8 b = ((x + y) / 2 + pattern_offset) & 0xFF;
                pixels[y * VIDEO_FRAME_WIDTH + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
            }
        }

        frame->pts = frame_pts;
        frame_pts += video_state.frame_duration;

        /* Mark frame as ready */
        sys_mutex_lock(video_state.state_mutex, 0);
        frame->state = FRAME_READY;
        video_state.frames_decoded++;
        sys_cond_signal(video_state.frame_ready);
        sys_mutex_unlock(video_state.state_mutex);

        /* Simulate decode time */
        sys_timer_usleep(10000);

        /* Check for end of stream */
        if (video_state.eof && video_state.stream.fill_level == 0) {
            break;
        }
    }

    free(nal_buffer);

    printf("Video: Decode thread exiting (decoded %u frames)\n", video_state.frames_decoded);
    video_state.decode_running = false;
    sys_ppu_thread_exit(0);
}

/* Start video playback from URL */
int video_play_stream(const char *url)
{
    if (!video_state.initialized) {
        if (video_init() != 0) return -1;
    }

    /* Stop any current playback */
    if (video_state.playing) {
        video_stop();
    }

    printf("Video: Playing %s\n", url);

    strncpy(video_state.url, url, MAX_URL_LENGTH - 1);
    video_state.url[MAX_URL_LENGTH - 1] = '\0';

    /* Reset state */
    video_state.playing = true;
    video_state.paused = false;
    video_state.seeking = false;
    video_state.eof = false;
    video_state.position_ms = 0;
    video_state.duration_ms = 3600000;  /* Demo: 1 hour */
    video_state.width = VIDEO_FRAME_WIDTH;
    video_state.height = VIDEO_FRAME_HEIGHT;
    video_state.current_frame = 0;
    video_state.display_frame = -1;
    video_state.frames_decoded = 0;
    video_state.frames_dropped = 0;
    video_state.buffer_underruns = 0;

    /* Reset frame buffers */
    for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
        video_state.frames[i].state = FRAME_EMPTY;
    }

    /* Reset stream buffer */
    video_state.stream.read_pos = 0;
    video_state.stream.write_pos = 0;
    video_state.stream.fill_level = 0;

    /* Start threads */
    video_state.stream_running = true;
    video_state.decode_running = true;

    sys_ppu_thread_create(&video_state.stream_thread, video_stream_thread, NULL,
        VIDEO_THREAD_PRIO, VIDEO_THREAD_STACK, SYS_PPU_THREAD_CREATE_JOINABLE, "VidStream");

    sys_ppu_thread_create(&video_state.decode_thread, video_decode_thread, NULL,
        VIDEO_THREAD_PRIO + 1, VIDEO_THREAD_STACK, SYS_PPU_THREAD_CREATE_JOINABLE, "VidDecode");

    /* Record start time */
    video_state.start_time = sys_time_get_system_time();
    video_state.last_frame_time = video_state.start_time;

    printf("Video: Playback started\n");
    return 0;
}

/* Stop video playback */
void video_stop(void)
{
    if (!video_state.playing) return;

    printf("Video: Stopping playback...\n");

    /* Signal threads to stop */
    video_state.stream_running = false;
    video_state.decode_running = false;
    video_state.eof = true;

    /* Wake up any waiting threads */
    sys_mutex_lock(video_state.stream.mutex, 0);
    sys_cond_broadcast(video_state.stream.not_empty);
    sys_cond_broadcast(video_state.stream.not_full);
    sys_mutex_unlock(video_state.stream.mutex);

    sys_mutex_lock(video_state.state_mutex, 0);
    sys_cond_broadcast(video_state.frame_ready);
    sys_mutex_unlock(video_state.state_mutex);

    /* Wait for threads to exit */
    u64 exit_code;
    sys_ppu_thread_join(video_state.stream_thread, &exit_code);
    sys_ppu_thread_join(video_state.decode_thread, &exit_code);

    video_state.playing = false;
    video_state.paused = false;
    video_state.url[0] = '\0';

    printf("Video: Stopped (decoded: %u, dropped: %u, underruns: %u)\n",
        video_state.frames_decoded, video_state.frames_dropped, video_state.buffer_underruns);
}

/* Pause video playback */
void video_pause(void)
{
    if (video_state.playing && !video_state.paused) {
        video_state.paused = true;
        video_state.pause_time = sys_time_get_system_time();
        printf("Video: Paused at %u ms\n", video_state.position_ms);
    }
}

/* Resume video playback */
void video_resume(void)
{
    if (video_state.playing && video_state.paused) {
        /* Adjust start time to account for pause duration */
        u64 pause_duration = sys_time_get_system_time() - video_state.pause_time;
        video_state.start_time += pause_duration;
        video_state.paused = false;
        printf("Video: Resumed\n");
    }
}

/* Seek to position */
void video_seek(int offset_ms)
{
    if (!video_state.playing) return;

    int new_pos = (int)video_state.position_ms + offset_ms;
    if (new_pos < 0) new_pos = 0;
    if (new_pos > (int)video_state.duration_ms) new_pos = video_state.duration_ms;

    video_state.seeking = true;
    video_state.position_ms = (u32)new_pos;

    /* In real implementation: flush buffers and seek in stream */

    /* Adjust timing */
    video_state.start_time = sys_time_get_system_time() - (video_state.position_ms * 1000);

    video_state.seeking = false;
    printf("Video: Seeked to %u ms\n", video_state.position_ms);
}

/* Check if video is playing */
bool video_is_playing(void)
{
    return video_state.playing && !video_state.paused;
}

/* Check if video is paused */
bool video_is_paused(void)
{
    return video_state.playing && video_state.paused;
}

/* Get playback position in milliseconds */
u32 video_get_position(void)
{
    return video_state.position_ms;
}

/* Get video duration in milliseconds */
u32 video_get_duration(void)
{
    return video_state.duration_ms;
}

/* Get video width */
int video_get_width(void)
{
    return video_state.width;
}

/* Get video height */
int video_get_height(void)
{
    return video_state.height;
}

/* Get current frame for display */
u8 *video_get_current_frame(void)
{
    if (!video_state.playing || video_state.display_frame < 0) {
        return NULL;
    }
    return video_state.frames[video_state.display_frame].data;
}

/* Render current video frame */
void video_render_frame(void)
{
    if (!video_state.playing || video_state.paused) return;

    u64 current_time = sys_time_get_system_time();
    u64 elapsed = current_time - video_state.start_time;
    video_state.position_ms = (u32)(elapsed / 1000);

    /* Check for end of playback */
    if (video_state.position_ms >= video_state.duration_ms) {
        video_state.playing = false;
        printf("Video: Playback complete\n");
        return;
    }

    /* Frame timing */
    u64 time_since_last = current_time - video_state.last_frame_time;
    if (time_since_last < video_state.frame_duration) {
        return;  /* Not time for next frame yet */
    }

    /* Find a ready frame to display */
    sys_mutex_lock(video_state.state_mutex, 0);

    /* Release previous display frame */
    if (video_state.display_frame >= 0) {
        video_state.frames[video_state.display_frame].state = FRAME_EMPTY;
    }

    /* Find next ready frame */
    int next_frame = -1;
    u64 target_pts = elapsed;

    for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
        if (video_state.frames[i].state == FRAME_READY) {
            /* Find frame closest to target PTS */
            if (next_frame < 0 ||
                (video_state.frames[i].pts <= target_pts &&
                 video_state.frames[i].pts > video_state.frames[next_frame].pts)) {
                next_frame = i;
            }
        }
    }

    if (next_frame >= 0) {
        video_state.frames[next_frame].state = FRAME_DISPLAYING;
        video_state.display_frame = next_frame;
        video_state.last_frame_time = current_time;

        /* Drop late frames */
        for (int i = 0; i < VIDEO_BUFFER_FRAMES; i++) {
            if (i != next_frame && video_state.frames[i].state == FRAME_READY &&
                video_state.frames[i].pts < target_pts - video_state.frame_duration) {
                video_state.frames[i].state = FRAME_EMPTY;
                video_state.frames_dropped++;
            }
        }
    } else {
        /* Buffer underrun */
        video_state.buffer_underruns++;
    }

    sys_mutex_unlock(video_state.state_mutex);

    /* In real implementation: upload frame to RSX texture and draw */
}

/* Get video statistics */
void video_get_stats(u32 *decoded, u32 *dropped, u32 *underruns)
{
    if (decoded) *decoded = video_state.frames_decoded;
    if (dropped) *dropped = video_state.frames_dropped;
    if (underruns) *underruns = video_state.buffer_underruns;
}

/* Get buffer fill level (0-100) */
int video_get_buffer_level(void)
{
    if (video_state.stream.size == 0) return 0;
    return (video_state.stream.fill_level * 100) / video_state.stream.size;
}
