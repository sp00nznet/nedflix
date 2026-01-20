/*
 * Nedflix for Nintendo GameCube
 * Audio playback with real codec support
 * Supports: WAV, MP3 (via dr_libs)
 */

#include "nedflix.h"

/* Include dr_libs implementations */
#define DR_WAV_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#include "../../common/dr_wav.h"
#include "../../common/dr_mp3.h"

/* Audio configuration */
#define DECODE_BUFFER_SAMPLES  4096
#define DECODE_BUFFER_SIZE     (DECODE_BUFFER_SAMPLES * 2 * sizeof(int16_t))

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

    /* Codec */
    codec_t codec;
    drwav wav;
    drmp3 mp3;

    /* Format */
    uint32_t sample_rate;
    uint32_t channels;
    uint64_t total_frames;
    uint64_t current_frame;
    double duration;
    double position;

    /* Buffers (double buffer) */
    int16_t *buf_a;
    int16_t *buf_b;
    bool using_a;
    size_t buf_frames;

    /* ASND */
    int voice;
    int volume;
    bool need_fill;
} g_audio;

/* Detect codec from file */
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

/* Voice callback */
static void voice_cb(int v) {
    (void)v;
    g_audio.need_fill = true;
}

/* Decode into buffer, return frames decoded */
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

/* Initialize audio */
int audio_init(void)
{
    if (g_audio.initialized) return 0;

    memset(&g_audio, 0, sizeof(g_audio));

    ASND_Init();
    ASND_Pause(0);

    g_audio.buf_a = (int16_t*)memalign(32, DECODE_BUFFER_SIZE);
    g_audio.buf_b = (int16_t*)memalign(32, DECODE_BUFFER_SIZE);
    if (!g_audio.buf_a || !g_audio.buf_b) {
        if (g_audio.buf_a) free(g_audio.buf_a);
        if (g_audio.buf_b) free(g_audio.buf_b);
        return -1;
    }

    g_audio.buf_frames = DECODE_BUFFER_SAMPLES;
    g_audio.voice = -1;
    g_audio.volume = 255;
    g_audio.initialized = true;

    LOG("Audio initialized");
    return 0;
}

/* Shutdown */
void audio_shutdown(void)
{
    if (!g_audio.initialized) return;
    audio_stop(NULL);
    free(g_audio.buf_a);
    free(g_audio.buf_b);
    ASND_End();
    g_audio.initialized = false;
}

/* Load file */
int audio_load(const char *path)
{
    if (!g_audio.initialized && audio_init() != 0)
        return -1;

    audio_stop(NULL);

    g_audio.codec = detect_codec(path);
    if (g_audio.codec == CODEC_NONE) {
        LOG_ERROR("Unknown format: %s", path);
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
        LOG_ERROR("Failed to open: %s", path);
        g_audio.codec = CODEC_NONE;
        return -1;
    }

    g_audio.duration = (double)g_audio.total_frames / g_audio.sample_rate;
    g_audio.current_frame = 0;
    g_audio.position = 0;

    LOG("Loaded: %s (%s, %uHz, %uch, %.1fs)",
        path,
        g_audio.codec == CODEC_WAV ? "WAV" : "MP3",
        g_audio.sample_rate,
        g_audio.channels,
        g_audio.duration);

    return 0;
}

/* Play */
int audio_play(playback_state_t *state)
{
    if (g_audio.codec == CODEC_NONE) return -1;

    if (g_audio.playing) {
        if (g_audio.paused) {
            audio_resume(state);
        }
        return 0;
    }

    /* Decode first buffer */
    size_t got = decode_frames(g_audio.buf_a, g_audio.buf_frames);
    if (got == 0) return -1;
    DCFlushRange(g_audio.buf_a, got * g_audio.channels * sizeof(int16_t));

    /* Pre-fill second buffer */
    size_t got2 = decode_frames(g_audio.buf_b, g_audio.buf_frames);
    if (got2 > 0) {
        DCFlushRange(g_audio.buf_b, got2 * g_audio.channels * sizeof(int16_t));
    }

    g_audio.voice = ASND_GetFirstUnusedVoice();
    if (g_audio.voice < 0) return -1;

    int fmt = (g_audio.channels == 2) ? VOICE_STEREO_16BIT : VOICE_MONO_16BIT;

    int r = ASND_SetVoice(g_audio.voice, fmt, g_audio.sample_rate, 0,
                          g_audio.buf_a, got * g_audio.channels * sizeof(int16_t),
                          g_audio.volume, g_audio.volume, voice_cb);
    if (r != SND_OK) return -1;

    g_audio.playing = true;
    g_audio.paused = false;
    g_audio.using_a = true;
    g_audio.need_fill = false;

    if (state) {
        state->is_playing = true;
        state->is_paused = false;
    }

    return 0;
}

/* Stop */
void audio_stop(playback_state_t *state)
{
    if (g_audio.voice >= 0) {
        ASND_StopVoice(g_audio.voice);
        g_audio.voice = -1;
    }

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

    if (state) {
        state->is_playing = false;
        state->is_paused = false;
        state->current_time = 0;
    }
}

/* Pause */
void audio_pause(playback_state_t *state)
{
    if (g_audio.voice >= 0 && g_audio.playing && !g_audio.paused) {
        ASND_PauseVoice(g_audio.voice, 1);
        g_audio.paused = true;
        if (state) state->is_paused = true;
    }
}

/* Resume */
void audio_resume(playback_state_t *state)
{
    if (g_audio.voice >= 0 && g_audio.paused) {
        ASND_PauseVoice(g_audio.voice, 0);
        g_audio.paused = false;
        if (state) state->is_paused = false;
    }
}

/* Seek */
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
        g_audio.need_fill = true;
    }
}

/* Volume */
void audio_set_volume(int vol)
{
    g_audio.volume = CLAMP(vol, 0, 255);
    if (g_audio.voice >= 0) {
        ASND_ChangeVolumeVoice(g_audio.voice, g_audio.volume, g_audio.volume);
    }
}

/* Update - call every frame */
void audio_update(void)
{
    if (!g_audio.playing || g_audio.paused) return;

    /* Update position */
    if (g_audio.sample_rate > 0) {
        g_audio.position = (double)g_audio.current_frame / g_audio.sample_rate;
    }

    /* Check end */
    if (g_audio.voice >= 0 && ASND_StatusVoice(g_audio.voice) == SND_UNUSED) {
        if (g_audio.current_frame >= g_audio.total_frames) {
            g_audio.playing = false;
            return;
        }
    }

    /* Double-buffer refill */
    if (g_audio.need_fill) {
        int16_t *buf = g_audio.using_a ? g_audio.buf_b : g_audio.buf_a;
        size_t got = decode_frames(buf, g_audio.buf_frames);

        if (got > 0) {
            DCFlushRange(buf, got * g_audio.channels * sizeof(int16_t));
            if (g_audio.voice >= 0) {
                ASND_AddVoice(g_audio.voice, buf,
                              got * g_audio.channels * sizeof(int16_t));
            }
            g_audio.using_a = !g_audio.using_a;
        }
        g_audio.need_fill = false;
    }
}

/* Query functions */
bool audio_is_playing(void) { return g_audio.playing && !g_audio.paused; }
double audio_get_position(void) { return g_audio.position; }
double audio_get_duration(void) { return g_audio.duration; }

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
