/*
 * Nedflix for Xbox 360
 * Audio playback using libxenon audio
 *
 * TECHNICAL DEMO / NOVELTY PORT
 *
 * Xbox 360 audio capabilities:
 *   - XMA hardware decoder (not accessible via libxenon)
 *   - Software PCM playback available
 *   - Focus on streaming audio from network
 */

#include "nedflix.h"

/* Audio buffer configuration */
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_CHANNELS      2
#define AUDIO_BUFFER_SIZE   (32 * 1024)
#define NUM_BUFFERS         2

/* Audio state */
static struct {
    bool initialized;
    bool playing;
    bool paused;
    int volume;

    /* Double buffering */
    uint8_t *buffers[NUM_BUFFERS];
    int current_buffer;
    size_t buffer_pos;
    bool buffer_ready[NUM_BUFFERS];

    /* Playback info */
    char current_url[MAX_URL_LENGTH];
    double position;
    double duration;

    /* Network streaming */
    int socket;
    size_t bytes_received;
} g_audio;

/*
 * Initialize audio
 */
int audio_init(void)
{
    LOG("Initializing audio...");

    memset(&g_audio, 0, sizeof(g_audio));
    g_audio.volume = 100;

    /* Allocate audio buffers */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        g_audio.buffers[i] = (uint8_t *)memalign(32, AUDIO_BUFFER_SIZE);
        if (!g_audio.buffers[i]) {
            LOG_ERROR("Failed to allocate audio buffer %d", i);
            return -1;
        }
        memset(g_audio.buffers[i], 0, AUDIO_BUFFER_SIZE);
    }

    /* Initialize Xenon audio hardware */
    /* Note: libxenon audio support is limited */

    g_audio.initialized = true;
    LOG("Audio initialized");
    return 0;
}

/*
 * Shutdown audio
 */
void audio_shutdown(void)
{
    if (!g_audio.initialized) return;

    audio_stop();

    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (g_audio.buffers[i]) {
            free(g_audio.buffers[i]);
            g_audio.buffers[i] = NULL;
        }
    }

    g_audio.initialized = false;
    LOG("Audio shutdown");
}

/*
 * Play audio from URL
 */
int audio_play(const char *url)
{
    if (!g_audio.initialized || !url) return -1;

    audio_stop();

    LOG("Playing audio: %s", url);

    strncpy(g_audio.current_url, url, sizeof(g_audio.current_url) - 1);
    g_audio.position = 0.0;
    g_audio.duration = 180.0;  /* Estimate */

    /* Reset buffers */
    g_audio.buffer_ready[0] = false;
    g_audio.buffer_ready[1] = false;
    g_audio.current_buffer = 0;
    g_audio.buffer_pos = 0;

    /*
     * In a full implementation:
     * 1. Connect to streaming URL
     * 2. Parse audio format
     * 3. Start streaming to buffers
     * 4. Feed to audio hardware
     */

    g_audio.playing = true;
    g_audio.paused = false;

    return 0;
}

/*
 * Stop playback
 */
void audio_stop(void)
{
    if (!g_audio.playing) return;

    LOG("Stopping audio");

    g_audio.playing = false;
    g_audio.paused = false;
    g_audio.position = 0.0;
    g_audio.current_url[0] = '\0';

    for (int i = 0; i < NUM_BUFFERS; i++) {
        g_audio.buffer_ready[i] = false;
        if (g_audio.buffers[i]) {
            memset(g_audio.buffers[i], 0, AUDIO_BUFFER_SIZE);
        }
    }

    if (g_audio.socket > 0) {
        /* Close network socket */
        g_audio.socket = 0;
    }
}

/*
 * Pause playback
 */
void audio_pause(void)
{
    if (!g_audio.playing || g_audio.paused) return;
    LOG("Pausing audio");
    g_audio.paused = true;
}

/*
 * Resume playback
 */
void audio_resume(void)
{
    if (!g_audio.playing || !g_audio.paused) return;
    LOG("Resuming audio");
    g_audio.paused = false;
}

/*
 * Seek to position
 */
void audio_seek(double seconds)
{
    if (!g_audio.playing) return;

    seconds = CLAMP(seconds, 0.0, g_audio.duration);
    LOG("Seeking to %.1f seconds", seconds);
    g_audio.position = seconds;
}

/*
 * Set volume (0-100)
 */
void audio_set_volume(int volume)
{
    g_audio.volume = CLAMP(volume, 0, 100);
}

/*
 * Update audio streaming
 */
void audio_update(void)
{
    if (!g_audio.playing || g_audio.paused) return;

    /* Simulate playback progress */
    g_audio.position += 0.016;  /* ~60 fps */

    if (g_audio.position >= g_audio.duration) {
        g_audio.playing = false;
    }
}

/*
 * Check if playing
 */
bool audio_is_playing(void)
{
    return g_audio.playing && !g_audio.paused;
}

/*
 * Get position
 */
double audio_get_position(void)
{
    return g_audio.position;
}

/*
 * Get duration
 */
double audio_get_duration(void)
{
    return g_audio.duration;
}
