/*
 * Nedflix PS3 - Audio playback with real codec support
 * Supports: WAV, MP3 (via dr_libs)
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <audio/audio.h>
#include <sys/thread.h>
#include <sys/mutex.h>

/* Include dr_libs implementations */
#define DR_WAV_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#include "../../common/dr_wav.h"
#include "../../common/dr_mp3.h"

/* Audio configuration */
#define AUDIO_SAMPLE_RATE   48000
#define AUDIO_CHANNELS      2
#define AUDIO_SAMPLES       1024
#define AUDIO_BLOCK_SIZE    (AUDIO_SAMPLES * AUDIO_CHANNELS * sizeof(int16_t))
#define NUM_AUDIO_BLOCKS    8
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
    bool stop_requested;

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

    /* Buffers */
    int16_t *decode_buffer;
    int16_t *output_buffer;
    size_t output_size;

    /* Volume */
    int volume;
    float volume_scale;

    /* PS3 audio */
    u32 audio_port;
    audioPortConfig port_config;
    sys_ppu_thread_t audio_thread;
    sys_mutex_t mutex;
    bool thread_running;
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

/* Audio output thread */
static void audio_thread_func(void *arg)
{
    (void)arg;
    printf("PS3 audio thread started\n");

    int16_t *samples = malloc(AUDIO_SAMPLES * AUDIO_CHANNELS * sizeof(int16_t));
    if (!samples) {
        printf("Failed to allocate audio samples buffer\n");
        sysThreadExit(0);
        return;
    }

    while (g_audio.thread_running && !g_audio.stop_requested) {
        if (!g_audio.playing || g_audio.paused) {
            usleep(10000);
            continue;
        }

        /* Get audio port buffer */
        sys_addr_t portbuf = 0;
        if (audioGetPortBlockTag(g_audio.audio_port, 0) == 0) {
            /* Decode samples */
            sysMutexLock(g_audio.mutex, 0);
            size_t got = decode_frames(samples, AUDIO_SAMPLES);
            sysMutexUnlock(g_audio.mutex);

            if (got == 0) {
                /* End of audio */
                g_audio.playing = false;
                break;
            }

            /* Fill with silence if not enough samples */
            if (got < AUDIO_SAMPLES) {
                memset(samples + (got * AUDIO_CHANNELS), 0,
                       (AUDIO_SAMPLES - got) * AUDIO_CHANNELS * sizeof(int16_t));
            }

            /* Apply volume */
            for (size_t i = 0; i < AUDIO_SAMPLES * AUDIO_CHANNELS; i++) {
                samples[i] = (int16_t)(samples[i] * g_audio.volume_scale);
            }

            /* Convert to float for PS3 audio output */
            audioAddData(g_audio.audio_port, (void*)samples,
                        AUDIO_SAMPLES, g_audio.volume_scale);
        }

        /* Update position */
        sysMutexLock(g_audio.mutex, 0);
        if (g_audio.sample_rate > 0) {
            g_audio.position = (double)g_audio.current_frame / g_audio.sample_rate;
        }
        sysMutexUnlock(g_audio.mutex);

        /* Small delay to prevent spinning */
        usleep(1000);
    }

    free(samples);
    printf("PS3 audio thread ended\n");
    sysThreadExit(0);
}

/* Initialize audio subsystem */
int audio_init(void)
{
    if (g_audio.initialized) return 0;

    printf("Initializing PS3 audio...\n");
    memset(&g_audio, 0, sizeof(g_audio));
    g_audio.volume = 100;
    g_audio.volume_scale = 1.0f;

    /* Allocate decode buffer */
    g_audio.decode_buffer = malloc(DECODE_BUFFER_SIZE * 2 * sizeof(int16_t));
    if (!g_audio.decode_buffer) {
        printf("Failed to allocate decode buffer\n");
        return -1;
    }

    /* Allocate output buffer */
    g_audio.output_buffer = malloc(AUDIO_BLOCK_SIZE * NUM_AUDIO_BLOCKS);
    if (!g_audio.output_buffer) {
        printf("Failed to allocate output buffer\n");
        free(g_audio.decode_buffer);
        return -1;
    }

    /* Create mutex */
    sys_mutex_attr_t mutex_attr;
    mutex_attr.attr_protocol = SYS_MUTEX_PROTOCOL_FIFO;
    mutex_attr.attr_recursive = SYS_MUTEX_ATTR_NOT_RECURSIVE;
    mutex_attr.attr_pshared = SYS_MUTEX_ATTR_PSHARED;
    mutex_attr.attr_adaptive = SYS_MUTEX_ATTR_NOT_ADAPTIVE;
    sysMutexCreate(&g_audio.mutex, &mutex_attr);

    /* Initialize PS3 audio */
    audioInit();

    /* Configure audio port */
    audioPortParam params;
    params.numChannels = AUDIO_CHANNELS;
    params.numBlocks = NUM_AUDIO_BLOCKS;
    params.attrib = 0;
    params.level = 1.0f;

    int ret = audioPortOpen(&params, &g_audio.audio_port);
    if (ret != 0) {
        printf("Failed to open audio port: %d\n", ret);
        free(g_audio.decode_buffer);
        free(g_audio.output_buffer);
        return -1;
    }

    audioGetPortConfig(g_audio.audio_port, &g_audio.port_config);

    g_audio.initialized = true;
    printf("PS3 audio initialized\n");
    return 0;
}

/* Shutdown audio */
void audio_shutdown(void)
{
    if (!g_audio.initialized) return;

    printf("Shutting down PS3 audio...\n");
    audio_stop();

    if (g_audio.audio_port) {
        audioPortStop(g_audio.audio_port);
        audioPortClose(g_audio.audio_port);
    }
    audioQuit();

    sysMutexDestroy(g_audio.mutex);

    if (g_audio.decode_buffer) {
        free(g_audio.decode_buffer);
        g_audio.decode_buffer = NULL;
    }

    if (g_audio.output_buffer) {
        free(g_audio.output_buffer);
        g_audio.output_buffer = NULL;
    }

    g_audio.initialized = false;
    printf("PS3 audio shutdown complete\n");
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

    /* Start audio port */
    audioPortStart(g_audio.audio_port);

    /* Start audio thread */
    g_audio.stop_requested = false;
    g_audio.thread_running = true;
    sysThreadCreate(&g_audio.audio_thread, audio_thread_func, NULL,
                    1500, 0x10000, THREAD_JOINABLE, "nedflix_audio");

    g_audio.playing = true;
    g_audio.paused = false;

    printf("Audio playback started\n");
    return 0;
}

/* Stop playback */
void audio_stop(void)
{
    if (!g_audio.initialized) return;

    /* Stop thread */
    g_audio.stop_requested = true;
    g_audio.thread_running = false;

    if (g_audio.audio_thread) {
        u64 retval;
        sysThreadJoin(g_audio.audio_thread, &retval);
        g_audio.audio_thread = 0;
    }

    /* Stop audio port */
    audioPortStop(g_audio.audio_port);

    /* Close codec */
    sysMutexLock(g_audio.mutex, 0);
    switch (g_audio.codec) {
        case CODEC_WAV: drwav_uninit(&g_audio.wav); break;
        case CODEC_MP3: drmp3_uninit(&g_audio.mp3); break;
        default: break;
    }
    g_audio.codec = CODEC_NONE;
    sysMutexUnlock(g_audio.mutex);

    g_audio.playing = false;
    g_audio.paused = false;
    g_audio.position = 0;
    g_audio.current_frame = 0;
    g_audio.path[0] = '\0';

    printf("Audio stopped\n");
}

/* Pause */
void audio_pause(void)
{
    if (g_audio.playing && !g_audio.paused) {
        g_audio.paused = true;
        printf("Audio paused\n");
    }
}

/* Resume */
void audio_resume(void)
{
    if (g_audio.playing && g_audio.paused) {
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

    sysMutexLock(g_audio.mutex, 0);
    bool ok = false;
    switch (g_audio.codec) {
        case CODEC_WAV: ok = drwav_seek_to_pcm_frame(&g_audio.wav, frame); break;
        case CODEC_MP3: ok = drmp3_seek_to_pcm_frame(&g_audio.mp3, frame); break;
        default: break;
    }

    if (ok) {
        g_audio.current_frame = frame;
        g_audio.position = seconds;
    }
    sysMutexUnlock(g_audio.mutex);

    if (ok) {
        printf("Audio seek to %.1fs\n", seconds);
    }
}

/* Set volume (0-100) */
void audio_set_volume(int vol)
{
    g_audio.volume = (vol < 0) ? 0 : (vol > 100) ? 100 : vol;
    g_audio.volume_scale = g_audio.volume / 100.0f;

    if (g_audio.audio_port) {
        audioSetPortLevel(g_audio.audio_port, g_audio.volume_scale);
    }
}

/* Update - call every frame */
void audio_update(void)
{
    if (!g_audio.initialized || !g_audio.playing) return;

    /* Check for end of playback */
    sysMutexLock(g_audio.mutex, 0);
    bool ended = (g_audio.current_frame >= g_audio.total_frames);
    sysMutexUnlock(g_audio.mutex);

    if (ended) {
        g_audio.playing = false;
        printf("Audio playback complete\n");
    }
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
