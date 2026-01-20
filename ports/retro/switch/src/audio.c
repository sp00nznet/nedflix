/*
 * Nedflix Nintendo Switch - Audio playback with real codec support
 * Supports: WAV, MP3 (via dr_libs)
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

/* Include dr_libs implementations */
#define DR_WAV_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#include "../../common/dr_wav.h"
#include "../../common/dr_mp3.h"

/* Audio configuration */
#define AUDIO_SAMPLE_RATE   48000
#define AUDIO_CHANNELS      2
#define AUDIO_SAMPLE_SIZE   sizeof(int16_t)
#define AUDIO_FRAME_SIZE    (AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE)
#define AUDIO_BUFFER_FRAMES 2048
#define AUDIO_BUFFER_SIZE   (AUDIO_BUFFER_FRAMES * AUDIO_FRAME_SIZE)
#define NUM_AUDIO_BUFFERS   4
#define DECODE_BUFFER_SIZE  4096

/* Codec types */
typedef enum {
    CODEC_NONE = 0,
    CODEC_WAV,
    CODEC_MP3
} codec_t;

/* Audio state */
static struct {
    bool initialized;
    bool playing;
    bool paused;

    /* Codec state */
    codec_t codec;
    drwav wav;
    drmp3 mp3;

    /* File info */
    char path[512];
    uint32_t sample_rate;
    uint32_t channels;
    uint64_t total_frames;
    uint64_t current_frame;
    double duration;
    double position;

    /* Audio buffers */
    AudioDriverWaveBuf wave_bufs[NUM_AUDIO_BUFFERS];
    int16_t *sample_buffers[NUM_AUDIO_BUFFERS];
    int current_buffer;
    int16_t *decode_buf;

    /* Volume */
    int volume;
    float volume_scale;

    /* Audren */
    AudioDriver drv;
    int mem_pool_id;
    void *mem_pool;
    size_t mem_pool_size;
    bool audren_init;
} g_audio;

/* Detect codec from file header */
static codec_t detect_codec(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return CODEC_NONE;

    uint8_t hdr[12];
    size_t n = fread(hdr, 1, 12, f);
    fclose(f);
    if (n < 12) return CODEC_NONE;

    /* RIFF....WAVE = WAV */
    if (hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F' &&
        hdr[8]=='W' && hdr[9]=='A' && hdr[10]=='V' && hdr[11]=='E')
        return CODEC_WAV;

    /* ID3 or 0xFF sync = MP3 */
    if ((hdr[0]=='I' && hdr[1]=='D' && hdr[2]=='3') ||
        (hdr[0]==0xFF && (hdr[1]&0xE0)==0xE0))
        return CODEC_MP3;

    return CODEC_NONE;
}

/* Decode frames into buffer */
static size_t decode_frames(int16_t *out, size_t max_frames)
{
    size_t got = 0;
    switch (g_audio.codec) {
        case CODEC_WAV:
            got = drwav_read_pcm_frames_s16(&g_audio.wav, max_frames, out);
            break;
        case CODEC_MP3:
            got = drmp3_read_pcm_frames_s16(&g_audio.mp3, max_frames, out);
            break;
        default:
            break;
    }
    g_audio.current_frame += got;
    return got;
}

/* Fill audio buffer with decoded samples */
static void fill_audio_buffer(int buf_idx)
{
    int16_t *samples = g_audio.sample_buffers[buf_idx];
    size_t frames_needed = AUDIO_BUFFER_FRAMES;
    size_t total_got = 0;

    /* Decode directly into output buffer */
    while (total_got < frames_needed && g_audio.playing) {
        size_t to_decode = frames_needed - total_got;
        size_t got = decode_frames(samples + (total_got * g_audio.channels), to_decode);

        if (got == 0) break;
        total_got += got;
    }

    /* Fill rest with silence if needed */
    if (total_got < frames_needed) {
        memset(samples + (total_got * g_audio.channels), 0,
               (frames_needed - total_got) * g_audio.channels * sizeof(int16_t));
    }

    /* Apply volume scaling */
    for (size_t i = 0; i < AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS; i++) {
        samples[i] = (int16_t)(samples[i] * g_audio.volume_scale);
    }

    /* Flush cache for DMA */
    armDCacheFlush(samples, AUDIO_BUFFER_SIZE);
}

/* Initialize audio subsystem */
int audio_init(void)
{
    if (g_audio.initialized) return 0;

    printf("Initializing Switch audio...\n");
    memset(&g_audio, 0, sizeof(g_audio));
    g_audio.volume = 100;
    g_audio.volume_scale = 1.0f;

    /* Allocate decode buffer */
    g_audio.decode_buf = (int16_t*)aligned_alloc(0x1000, DECODE_BUFFER_SIZE * 2 * sizeof(int16_t));
    if (!g_audio.decode_buf) {
        printf("Failed to allocate decode buffer\n");
        return -1;
    }

    /* Allocate memory pool for audio buffers */
    g_audio.mem_pool_size = (AUDIO_BUFFER_SIZE * NUM_AUDIO_BUFFERS + 0xFFF) & ~0xFFF;
    g_audio.mem_pool = aligned_alloc(0x1000, g_audio.mem_pool_size);
    if (!g_audio.mem_pool) {
        printf("Failed to allocate audio memory pool\n");
        free(g_audio.decode_buf);
        return -1;
    }
    memset(g_audio.mem_pool, 0, g_audio.mem_pool_size);

    /* Set up sample buffer pointers */
    uint8_t *pool_ptr = g_audio.mem_pool;
    for (int i = 0; i < NUM_AUDIO_BUFFERS; i++) {
        g_audio.sample_buffers[i] = (int16_t*)pool_ptr;
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
        free(g_audio.mem_pool);
        free(g_audio.decode_buf);
        return -1;
    }

    /* Create audio driver */
    rc = audrvCreate(&g_audio.drv, &arConfig, 2);
    if (R_FAILED(rc)) {
        printf("audrvCreate failed: 0x%x\n", rc);
        audrenExit();
        free(g_audio.mem_pool);
        free(g_audio.decode_buf);
        return -1;
    }

    /* Add memory pool */
    g_audio.mem_pool_id = audrvMemPoolAdd(&g_audio.drv, g_audio.mem_pool,
                                           g_audio.mem_pool_size);
    audrvMemPoolAttach(&g_audio.drv, g_audio.mem_pool_id);

    /* Configure output sink */
    static const u8 sink_channels[] = {0, 1};
    audrvDeviceSinkAdd(&g_audio.drv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

    audrvUpdate(&g_audio.drv);
    audrenStartAudioRenderer();

    g_audio.audren_init = true;
    g_audio.initialized = true;
    printf("Switch audio initialized\n");
    return 0;
}

/* Shutdown audio */
void audio_shutdown(void)
{
    if (!g_audio.initialized) return;

    printf("Shutting down Switch audio...\n");
    audio_stop();

    if (g_audio.audren_init) {
        audrvClose(&g_audio.drv);
        audrenExit();
        g_audio.audren_init = false;
    }

    if (g_audio.mem_pool) {
        free(g_audio.mem_pool);
        g_audio.mem_pool = NULL;
    }

    if (g_audio.decode_buf) {
        free(g_audio.decode_buf);
        g_audio.decode_buf = NULL;
    }

    g_audio.initialized = false;
    printf("Switch audio shutdown complete\n");
}

/* Load audio file */
int audio_load(const char *path)
{
    if (!g_audio.initialized && audio_init() != 0)
        return -1;

    audio_stop();

    printf("Loading audio: %s\n", path);

    g_audio.codec = detect_codec(path);
    if (g_audio.codec == CODEC_NONE) {
        printf("Unknown audio format: %s\n", path);
        return -1;
    }

    bool ok = false;
    switch (g_audio.codec) {
        case CODEC_WAV:
            if (drwav_init_file(&g_audio.wav, path, NULL)) {
                g_audio.sample_rate = g_audio.wav.sampleRate;
                g_audio.channels = g_audio.wav.channels;
                g_audio.total_frames = g_audio.wav.totalPCMFrameCount;
                ok = true;
            }
            break;
        case CODEC_MP3:
            if (drmp3_init_file(&g_audio.mp3, path, NULL)) {
                g_audio.sample_rate = g_audio.mp3.sampleRate;
                g_audio.channels = g_audio.mp3.channels;
                g_audio.total_frames = drmp3_get_pcm_frame_count(&g_audio.mp3);
                ok = true;
            }
            break;
        default:
            break;
    }

    if (!ok) {
        printf("Failed to open: %s\n", path);
        g_audio.codec = CODEC_NONE;
        return -1;
    }

    strncpy(g_audio.path, path, sizeof(g_audio.path) - 1);
    g_audio.duration = (double)g_audio.total_frames / g_audio.sample_rate;
    g_audio.current_frame = 0;
    g_audio.position = 0;

    printf("Loaded: %s (%s, %uHz, %uch, %.1fs)\n",
        path,
        g_audio.codec == CODEC_WAV ? "WAV" : "MP3",
        g_audio.sample_rate,
        g_audio.channels,
        g_audio.duration);

    return 0;
}

/* Start playback */
int audio_play(void)
{
    if (g_audio.codec == CODEC_NONE) return -1;

    if (g_audio.playing) {
        if (g_audio.paused) {
            audio_resume();
        }
        return 0;
    }

    /* Initialize wave buffers */
    for (int i = 0; i < NUM_AUDIO_BUFFERS; i++) {
        g_audio.wave_bufs[i].data_raw = g_audio.sample_buffers[i];
        g_audio.wave_bufs[i].size = AUDIO_BUFFER_SIZE;
        g_audio.wave_bufs[i].start_sample_offset = 0;
        g_audio.wave_bufs[i].end_sample_offset = AUDIO_BUFFER_FRAMES;
        g_audio.wave_bufs[i].state = AudioDriverWaveBufState_Free;
    }

    /* Set up voice */
    int voice_chans = (g_audio.channels > 2) ? 2 : g_audio.channels;
    audrvVoiceInit(&g_audio.drv, 0, voice_chans, PcmFormat_Int16, g_audio.sample_rate);
    audrvVoiceSetDestinationMix(&g_audio.drv, 0, AUDREN_FINAL_MIX_ID);
    audrvVoiceSetMixFactor(&g_audio.drv, 0, g_audio.volume_scale, 0, 0);
    if (voice_chans == 2) {
        audrvVoiceSetMixFactor(&g_audio.drv, 0, g_audio.volume_scale, 1, 1);
    }

    /* Pre-fill first two buffers */
    fill_audio_buffer(0);
    fill_audio_buffer(1);
    audrvVoiceAddWaveBuf(&g_audio.drv, 0, &g_audio.wave_bufs[0]);
    audrvVoiceAddWaveBuf(&g_audio.drv, 0, &g_audio.wave_bufs[1]);

    /* Start playback */
    audrvVoiceStart(&g_audio.drv, 0);
    audrvUpdate(&g_audio.drv);

    g_audio.playing = true;
    g_audio.paused = false;
    g_audio.current_buffer = 2;

    printf("Audio playback started\n");
    return 0;
}

/* Stop playback */
void audio_stop(void)
{
    if (g_audio.playing && g_audio.audren_init) {
        audrvVoiceStop(&g_audio.drv, 0);
        audrvUpdate(&g_audio.drv);
    }

    /* Close codec */
    switch (g_audio.codec) {
        case CODEC_WAV: drwav_uninit(&g_audio.wav); break;
        case CODEC_MP3: drmp3_uninit(&g_audio.mp3); break;
        default: break;
    }

    g_audio.playing = false;
    g_audio.paused = false;
    g_audio.codec = CODEC_NONE;
    g_audio.position = 0;
    g_audio.current_frame = 0;
    g_audio.path[0] = '\0';

    printf("Audio stopped\n");
}

/* Pause */
void audio_pause(void)
{
    if (g_audio.playing && !g_audio.paused) {
        audrvVoiceSetPaused(&g_audio.drv, 0, true);
        audrvUpdate(&g_audio.drv);
        g_audio.paused = true;
        printf("Audio paused\n");
    }
}

/* Resume */
void audio_resume(void)
{
    if (g_audio.playing && g_audio.paused) {
        audrvVoiceSetPaused(&g_audio.drv, 0, false);
        audrvUpdate(&g_audio.drv);
        g_audio.paused = false;
        printf("Audio resumed\n");
    }
}

/* Seek to position in seconds */
void audio_seek(double seconds)
{
    if (!g_audio.playing || g_audio.sample_rate == 0) return;

    if (seconds < 0) seconds = 0;
    if (seconds > g_audio.duration) seconds = g_audio.duration;

    uint64_t frame = (uint64_t)(seconds * g_audio.sample_rate);
    bool ok = false;

    switch (g_audio.codec) {
        case CODEC_WAV: ok = drwav_seek_to_pcm_frame(&g_audio.wav, frame); break;
        case CODEC_MP3: ok = drmp3_seek_to_pcm_frame(&g_audio.mp3, frame); break;
        default: break;
    }

    if (ok) {
        g_audio.current_frame = frame;
        g_audio.position = seconds;
        printf("Audio seek to %.1fs\n", seconds);
    }
}

/* Set volume (0-100) */
void audio_set_volume(int vol)
{
    g_audio.volume = (vol < 0) ? 0 : (vol > 100) ? 100 : vol;
    g_audio.volume_scale = g_audio.volume / 100.0f;

    if (g_audio.audren_init && g_audio.playing) {
        audrvVoiceSetMixFactor(&g_audio.drv, 0, g_audio.volume_scale, 0, 0);
        audrvVoiceSetMixFactor(&g_audio.drv, 0, g_audio.volume_scale, 1, 1);
        audrvUpdate(&g_audio.drv);
    }
}

/* Update - call every frame */
void audio_update(void)
{
    if (!g_audio.initialized || !g_audio.playing || g_audio.paused) return;

    /* Update position */
    if (g_audio.sample_rate > 0) {
        g_audio.position = (double)g_audio.current_frame / g_audio.sample_rate;
    }

    /* Check for end of playback */
    if (g_audio.current_frame >= g_audio.total_frames) {
        g_audio.playing = false;
        printf("Audio playback complete\n");
        return;
    }

    /* Update audio driver */
    audrvUpdate(&g_audio.drv);

    /* Refill buffers as needed */
    for (int i = 0; i < NUM_AUDIO_BUFFERS; i++) {
        AudioDriverWaveBuf *buf = &g_audio.wave_bufs[i];

        if (buf->state == AudioDriverWaveBufState_Free ||
            buf->state == AudioDriverWaveBufState_Done) {

            fill_audio_buffer(i);
            audrvVoiceAddWaveBuf(&g_audio.drv, 0, buf);
        }
    }

    audrvUpdate(&g_audio.drv);
}

/* Query functions */
bool audio_is_playing(void) { return g_audio.playing && !g_audio.paused; }
double audio_get_position(void) { return g_audio.position; }
double audio_get_duration(void) { return g_audio.duration; }
int audio_get_volume(void) { return g_audio.volume; }

/* Compatibility wrappers */
int audio_load_wav(const char *path, playback_state_t *state)
{
    int r = audio_load(path);
    if (r == 0 && state) {
        state->duration = g_audio.duration;
        state->current_time = 0;
        state->is_playing = false;
        state->is_paused = false;
        state->format.sample_rate = g_audio.sample_rate;
        state->format.channels = g_audio.channels;
        state->format.bits_per_sample = 16;
    }
    return r;
}

int audio_load_mp3(const char *path, playback_state_t *state)
{
    return audio_load_wav(path, state);
}

int audio_play_ex(playback_state_t *state)
{
    int r = audio_play();
    if (r == 0 && state) {
        state->is_playing = true;
        state->is_paused = false;
    }
    return r;
}

void audio_stop_ex(playback_state_t *state)
{
    audio_stop();
    if (state) {
        state->is_playing = false;
        state->is_paused = false;
        state->current_time = 0;
    }
}

void audio_pause_ex(playback_state_t *state)
{
    audio_pause();
    if (state) state->is_paused = true;
}

void audio_resume_ex(playback_state_t *state)
{
    audio_resume();
    if (state) state->is_paused = false;
}
