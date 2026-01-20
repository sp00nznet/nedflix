/*
 * Nedflix Nintendo Switch - Audio playback system
 * Full implementation with streaming and audren
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

/* Audio configuration */
#define AUDIO_SAMPLE_RATE   48000
#define AUDIO_CHANNELS      2
#define AUDIO_SAMPLE_SIZE   sizeof(int16_t)
#define AUDIO_FRAME_SIZE    (AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE)
#define AUDIO_BUFFER_FRAMES 2048
#define AUDIO_BUFFER_SIZE   (AUDIO_BUFFER_FRAMES * AUDIO_FRAME_SIZE)
#define NUM_AUDIO_BUFFERS   4

/* Ring buffer for streaming */
#define RING_BUFFER_SIZE    (256 * 1024)  /* 256KB ring buffer */

typedef struct {
    uint8_t *data;
    size_t size;
    size_t read_pos;
    size_t write_pos;
    size_t available;
    Mutex mutex;
} ring_buffer_t;

/* Audio state */
static struct {
    bool initialized;
    bool playing;
    bool paused;
    bool buffering;
    bool stop_requested;

    /* Stream info */
    char url[MAX_URL_LENGTH];
    uint32_t position_ms;
    uint32_t duration_ms;
    int sample_rate;
    int channels;
    int bitrate;

    /* Audio buffers */
    AudioDriverWaveBuf wave_bufs[NUM_AUDIO_BUFFERS];
    int16_t *sample_buffers[NUM_AUDIO_BUFFERS];
    int current_buffer;

    /* Ring buffer for streaming */
    ring_buffer_t ring;

    /* Volume */
    int volume;
    float volume_scale;

    /* Audren */
    AudioDriver drv;
    int mem_pool_id;
    void *mem_pool;
    size_t mem_pool_size;

    /* Threading */
    Thread stream_thread;
    bool thread_running;
} audio_state;

/* Initialize ring buffer */
static int ring_init(ring_buffer_t *rb, size_t size)
{
    rb->data = aligned_alloc(0x1000, size);
    if (!rb->data) return -1;

    rb->size = size;
    rb->read_pos = 0;
    rb->write_pos = 0;
    rb->available = 0;

    mutexInit(&rb->mutex);
    return 0;
}

/* Free ring buffer */
static void ring_free(ring_buffer_t *rb)
{
    if (rb->data) {
        free(rb->data);
        rb->data = NULL;
    }
}

/* Write to ring buffer */
static size_t ring_write(ring_buffer_t *rb, const void *data, size_t len)
{
    mutexLock(&rb->mutex);

    size_t free_space = rb->size - rb->available;
    if (len > free_space) len = free_space;

    const uint8_t *src = (const uint8_t*)data;
    size_t to_end = rb->size - rb->write_pos;

    if (len <= to_end) {
        memcpy(rb->data + rb->write_pos, src, len);
    } else {
        memcpy(rb->data + rb->write_pos, src, to_end);
        memcpy(rb->data, src + to_end, len - to_end);
    }

    rb->write_pos = (rb->write_pos + len) % rb->size;
    rb->available += len;

    mutexUnlock(&rb->mutex);
    return len;
}

/* Read from ring buffer */
static size_t ring_read(ring_buffer_t *rb, void *data, size_t len)
{
    mutexLock(&rb->mutex);

    if (len > rb->available) len = rb->available;

    uint8_t *dst = (uint8_t*)data;
    size_t to_end = rb->size - rb->read_pos;

    if (len <= to_end) {
        memcpy(dst, rb->data + rb->read_pos, len);
    } else {
        memcpy(dst, rb->data + rb->read_pos, to_end);
        memcpy(dst + to_end, rb->data, len - to_end);
    }

    rb->read_pos = (rb->read_pos + len) % rb->size;
    rb->available -= len;

    mutexUnlock(&rb->mutex);
    return len;
}

/* Get buffer fill percentage */
static int ring_percent(ring_buffer_t *rb)
{
    mutexLock(&rb->mutex);
    int percent = (rb->available * 100) / rb->size;
    mutexUnlock(&rb->mutex);
    return percent;
}

/* Streaming thread function */
static void stream_thread_func(void *arg)
{
    (void)arg;
    printf("Audio stream thread started\n");

    /* Start HTTP stream */
    if (http_stream_start(audio_state.url) != 0) {
        printf("Failed to start HTTP stream\n");
        audio_state.buffering = false;
        return;
    }

    uint8_t *buffer = malloc(32768);
    if (!buffer) {
        http_stream_stop();
        return;
    }

    audio_state.buffering = true;

    while (audio_state.thread_running && !audio_state.stop_requested) {
        /* Check if buffer has space */
        mutexLock(&audio_state.ring.mutex);
        size_t free_space = audio_state.ring.size - audio_state.ring.available;
        mutexUnlock(&audio_state.ring.mutex);

        if (free_space < 32768) {
            /* Buffer full, wait */
            svcSleepThread(10000000);  /* 10ms */
            continue;
        }

        /* Read from HTTP stream */
        int bytes = http_stream_read(buffer, 32768);
        if (bytes > 0) {
            ring_write(&audio_state.ring, buffer, bytes);

            /* Update buffering state */
            int percent = ring_percent(&audio_state.ring);
            if (percent > 10) {
                audio_state.buffering = false;
            }
        } else if (bytes == 0) {
            /* End of stream */
            break;
        } else {
            /* Error or timeout, retry */
            svcSleepThread(50000000);  /* 50ms */
        }
    }

    free(buffer);
    http_stream_stop();
    printf("Audio stream thread ended\n");
}

/* Fill audio buffer from ring buffer */
static void fill_audio_buffer(int buf_idx)
{
    int16_t *samples = audio_state.sample_buffers[buf_idx];
    size_t needed = AUDIO_BUFFER_SIZE;
    size_t got = ring_read(&audio_state.ring, samples, needed);

    if (got < needed) {
        /* Fill rest with silence */
        memset((uint8_t*)samples + got, 0, needed - got);
        audio_state.buffering = true;
    }

    /* Apply volume */
    for (size_t i = 0; i < AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS; i++) {
        samples[i] = (int16_t)(samples[i] * audio_state.volume_scale);
    }

    /* Flush cache for DMA */
    armDCacheFlush(samples, AUDIO_BUFFER_SIZE);
}

/* Initialize audio subsystem */
int audio_init(void)
{
    printf("Initializing audio subsystem...\n");

    memset(&audio_state, 0, sizeof(audio_state));
    audio_state.volume = 100;
    audio_state.volume_scale = 1.0f;
    audio_state.sample_rate = AUDIO_SAMPLE_RATE;
    audio_state.channels = AUDIO_CHANNELS;

    /* Initialize ring buffer */
    if (ring_init(&audio_state.ring, RING_BUFFER_SIZE) != 0) {
        printf("Failed to allocate ring buffer\n");
        return -1;
    }

    /* Allocate memory pool for audio */
    audio_state.mem_pool_size = (AUDIO_BUFFER_SIZE * NUM_AUDIO_BUFFERS + 0xFFF) & ~0xFFF;
    audio_state.mem_pool = aligned_alloc(0x1000, audio_state.mem_pool_size);
    if (!audio_state.mem_pool) {
        printf("Failed to allocate audio memory pool\n");
        ring_free(&audio_state.ring);
        return -1;
    }
    memset(audio_state.mem_pool, 0, audio_state.mem_pool_size);

    /* Set up sample buffer pointers */
    uint8_t *pool_ptr = audio_state.mem_pool;
    for (int i = 0; i < NUM_AUDIO_BUFFERS; i++) {
        audio_state.sample_buffers[i] = (int16_t*)pool_ptr;
        pool_ptr += AUDIO_BUFFER_SIZE;
    }

    /* Initialize audren */
    static const AudioRendererConfig arConfig = {
        .output_rate     = AudioRendererOutputRate_48kHz,
        .num_voices      = 4,
        .num_effects     = 0,
        .num_sinks       = 1,
        .num_mix_objs    = 1,
        .num_mix_buffers = 2,
    };

    Result rc = audrenInitialize(&arConfig);
    if (R_FAILED(rc)) {
        printf("audrenInitialize failed: 0x%x\n", rc);
        free(audio_state.mem_pool);
        ring_free(&audio_state.ring);
        return -1;
    }

    /* Create audio driver */
    rc = audrvCreate(&audio_state.drv, &arConfig, 2);
    if (R_FAILED(rc)) {
        printf("audrvCreate failed: 0x%x\n", rc);
        audrenExit();
        free(audio_state.mem_pool);
        ring_free(&audio_state.ring);
        return -1;
    }

    /* Add memory pool */
    audio_state.mem_pool_id = audrvMemPoolAdd(&audio_state.drv, audio_state.mem_pool,
                                               audio_state.mem_pool_size);
    audrvMemPoolAttach(&audio_state.drv, audio_state.mem_pool_id);

    /* Configure output sink */
    static const u8 sink_channels[] = {0, 1};
    audrvDeviceSinkAdd(&audio_state.drv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

    /* Start audio renderer */
    rc = audrvUpdate(&audio_state.drv);
    if (R_FAILED(rc)) {
        printf("audrvUpdate failed: 0x%x\n", rc);
    }

    audrenStartAudioRenderer();

    audio_state.initialized = true;
    printf("Audio initialized\n");
    return 0;
}

/* Shutdown audio */
void audio_shutdown(void)
{
    if (!audio_state.initialized) return;

    printf("Shutting down audio...\n");

    audio_stop();

    /* Cleanup audren */
    audrvClose(&audio_state.drv);
    audrenExit();

    if (audio_state.mem_pool) {
        free(audio_state.mem_pool);
        audio_state.mem_pool = NULL;
    }

    ring_free(&audio_state.ring);

    audio_state.initialized = false;
    printf("Audio shutdown complete\n");
}

/* Start playing audio stream from URL */
int audio_play_stream(const char *url)
{
    if (!audio_state.initialized) {
        if (audio_init() != 0) return -1;
    }

    /* Stop any current playback */
    audio_stop();

    printf("Starting audio stream: %s\n", url);

    strncpy(audio_state.url, url, MAX_URL_LENGTH - 1);
    audio_state.position_ms = 0;
    audio_state.duration_ms = 0;
    audio_state.buffering = true;
    audio_state.stop_requested = false;
    audio_state.thread_running = true;

    /* Clear ring buffer */
    mutexLock(&audio_state.ring.mutex);
    audio_state.ring.read_pos = 0;
    audio_state.ring.write_pos = 0;
    audio_state.ring.available = 0;
    mutexUnlock(&audio_state.ring.mutex);

    /* Initialize wave buffers */
    for (int i = 0; i < NUM_AUDIO_BUFFERS; i++) {
        audio_state.wave_bufs[i].data_raw = audio_state.sample_buffers[i];
        audio_state.wave_bufs[i].size = AUDIO_BUFFER_SIZE;
        audio_state.wave_bufs[i].start_sample_offset = 0;
        audio_state.wave_bufs[i].end_sample_offset = AUDIO_BUFFER_FRAMES;
    }

    /* Set up voice for playback */
    audrvVoiceInit(&audio_state.drv, 0, AUDIO_CHANNELS, PcmFormat_Int16, AUDIO_SAMPLE_RATE);
    audrvVoiceSetDestinationMix(&audio_state.drv, 0, AUDREN_FINAL_MIX_ID);

    /* Set channel volumes */
    audrvVoiceSetMixFactor(&audio_state.drv, 0, 1.0f, 0, 0);
    audrvVoiceSetMixFactor(&audio_state.drv, 0, 1.0f, 1, 1);

    /* Start streaming thread */
    Result rc = threadCreate(&audio_state.stream_thread, stream_thread_func, NULL,
                             NULL, 0x10000, 0x2B, -2);
    if (R_SUCCEEDED(rc)) {
        threadStart(&audio_state.stream_thread);
    }

    /* Start voice playback */
    audrvVoiceStart(&audio_state.drv, 0);

    audio_state.playing = true;
    audio_state.paused = false;

    return 0;
}

/* Play audio from local file */
int audio_play_file(const char *path)
{
    char file_url[MAX_URL_LENGTH];
    snprintf(file_url, sizeof(file_url), "file://%s", path);
    return audio_play_stream(file_url);
}

/* Stop audio playback */
void audio_stop(void)
{
    if (!audio_state.playing) return;

    printf("Stopping audio...\n");

    audio_state.stop_requested = true;
    audio_state.thread_running = false;

    /* Wait for thread to finish */
    threadWaitForExit(&audio_state.stream_thread);
    threadClose(&audio_state.stream_thread);

    /* Stop voice */
    audrvVoiceStop(&audio_state.drv, 0);

    audio_state.playing = false;
    audio_state.paused = false;
    audio_state.position_ms = 0;
    audio_state.url[0] = '\0';

    printf("Audio stopped\n");
}

/* Pause audio */
void audio_pause(void)
{
    if (audio_state.playing && !audio_state.paused) {
        audrvVoiceSetPaused(&audio_state.drv, 0, true);
        audio_state.paused = true;
        printf("Audio paused\n");
    }
}

/* Resume audio */
void audio_resume(void)
{
    if (audio_state.playing && audio_state.paused) {
        audrvVoiceSetPaused(&audio_state.drv, 0, false);
        audio_state.paused = false;
        printf("Audio resumed\n");
    }
}

/* Seek relative to current position */
void audio_seek(int offset_ms)
{
    if (!audio_state.playing) return;

    int new_pos = (int)audio_state.position_ms + offset_ms;
    if (new_pos < 0) new_pos = 0;
    if (audio_state.duration_ms > 0 && new_pos > (int)audio_state.duration_ms) {
        new_pos = audio_state.duration_ms;
    }

    audio_state.position_ms = (uint32_t)new_pos;
    printf("Audio seek to %u ms\n", audio_state.position_ms);
}

/* Seek to absolute position */
void audio_seek_absolute(uint32_t position_ms)
{
    if (!audio_state.playing) return;

    if (audio_state.duration_ms > 0 && position_ms > audio_state.duration_ms) {
        position_ms = audio_state.duration_ms;
    }

    audio_state.position_ms = position_ms;
    printf("Audio seek to %u ms\n", position_ms);
}

/* Set volume (0-100) */
void audio_set_volume(int vol)
{
    audio_state.volume = CLAMP(vol, 0, 100);
    audio_state.volume_scale = audio_state.volume / 100.0f;

    /* Update voice volume */
    if (audio_state.initialized) {
        audrvVoiceSetMixFactor(&audio_state.drv, 0, audio_state.volume_scale, 0, 0);
        audrvVoiceSetMixFactor(&audio_state.drv, 0, audio_state.volume_scale, 1, 1);
    }
}

/* Set audio track */
void audio_set_track(int track)
{
    (void)track;
    /* For future: switch audio track */
}

/* Update audio (called each frame) */
void audio_update(void)
{
    if (!audio_state.initialized || !audio_state.playing) return;

    /* Update audio driver */
    audrvUpdate(&audio_state.drv);

    /* Fill and queue buffers as needed */
    for (int i = 0; i < NUM_AUDIO_BUFFERS; i++) {
        AudioDriverWaveBuf *buf = &audio_state.wave_bufs[i];

        if (buf->state == AudioDriverWaveBufState_Free ||
            buf->state == AudioDriverWaveBufState_Done) {

            /* Fill this buffer */
            fill_audio_buffer(i);

            /* Queue it for playback */
            audrvVoiceAddWaveBuf(&audio_state.drv, 0, buf);
            audrvUpdate(&audio_state.drv);
        }
    }

    /* Update position */
    if (!audio_state.paused && !audio_state.buffering) {
        /* Approximate position based on played samples */
        uint64_t played = audrvVoiceGetPlayedSampleCount(&audio_state.drv, 0);
        audio_state.position_ms = (uint32_t)((played * 1000) / audio_state.sample_rate);
    }

    /* Check for end of stream */
    if (audio_state.duration_ms > 0 && audio_state.position_ms >= audio_state.duration_ms) {
        audio_state.playing = false;
        printf("Audio playback complete\n");
    }
}

/* Check if audio is playing */
bool audio_is_playing(void)
{
    return audio_state.playing && !audio_state.paused;
}

/* Check if buffering */
bool audio_is_buffering(void)
{
    return audio_state.buffering;
}

/* Get current position in milliseconds */
uint32_t audio_get_position(void)
{
    return audio_state.position_ms;
}

/* Get duration in milliseconds */
uint32_t audio_get_duration(void)
{
    return audio_state.duration_ms;
}

/* Get buffer fill percentage */
int audio_get_buffer_percent(void)
{
    return ring_percent(&audio_state.ring);
}
