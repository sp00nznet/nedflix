/*
 * Nedflix for Sega Dreamcast
 * Audio streaming using KallistiOS sound system
 *
 * The Dreamcast has:
 * - Yamaha AICA sound processor
 * - 2MB dedicated sound RAM
 * - 64 sound channels
 * - Hardware ADPCM decoding
 *
 * For streaming audio, we use double-buffering:
 * - While one buffer plays, we fill the other from network
 * - Supports MP3 decoding via libmp3 (optional)
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>

#include <dc/sound/sound.h>
#include <dc/sound/stream.h>

/* Audio buffer configuration */
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_CHANNELS      2
#define AUDIO_BUFFER_SIZE   (16 * 1024)  /* 16KB per buffer */
#define NUM_BUFFERS         2

/* Audio state */
static struct {
    bool initialized;
    bool playing;
    bool paused;
    int volume;

    /* Stream handle */
    snd_stream_hnd_t stream;

    /* Double buffer for streaming */
    uint8 *buffers[NUM_BUFFERS];
    int current_buffer;
    size_t buffer_pos;
    bool buffer_ready[NUM_BUFFERS];

    /* Playback info */
    char current_url[MAX_URL_LENGTH];
    double position;
    double duration;

    /* Network streaming state */
    int socket;
    size_t content_length;
    size_t bytes_received;

    /* Decoding state (placeholder for MP3/AAC decoder) */
    void *decoder_ctx;
} g_audio;

/* Mutex for thread safety */
static mutex_t audio_mutex = MUTEX_INITIALIZER;

/*
 * Audio stream callback (called by sound system when it needs more data)
 */
static void *audio_stream_callback(snd_stream_hnd_t hnd, int samples_req,
                                   int *samples_returned)
{
    (void)hnd;

    mutex_lock(&audio_mutex);

    if (!g_audio.playing || g_audio.paused) {
        *samples_returned = 0;
        mutex_unlock(&audio_mutex);
        return NULL;
    }

    /* Get data from current buffer */
    int buf_idx = g_audio.current_buffer;

    if (!g_audio.buffer_ready[buf_idx]) {
        /* Buffer not ready, return silence */
        *samples_returned = 0;
        mutex_unlock(&audio_mutex);
        return NULL;
    }

    /* Calculate available samples */
    int bytes_per_sample = AUDIO_CHANNELS * 2;  /* 16-bit stereo */
    int samples_avail = (AUDIO_BUFFER_SIZE - g_audio.buffer_pos) / bytes_per_sample;

    if (samples_avail > samples_req) {
        samples_avail = samples_req;
    }

    void *data = g_audio.buffers[buf_idx] + g_audio.buffer_pos;
    g_audio.buffer_pos += samples_avail * bytes_per_sample;

    /* Check if buffer is exhausted */
    if (g_audio.buffer_pos >= AUDIO_BUFFER_SIZE) {
        g_audio.buffer_ready[buf_idx] = false;
        g_audio.current_buffer = (buf_idx + 1) % NUM_BUFFERS;
        g_audio.buffer_pos = 0;
    }

    /* Update playback position */
    g_audio.position += (double)samples_avail / AUDIO_SAMPLE_RATE;

    *samples_returned = samples_avail;
    mutex_unlock(&audio_mutex);

    return data;
}

/*
 * Initialize audio subsystem
 */
int audio_init(void)
{
    LOG("Initializing audio...");

    memset(&g_audio, 0, sizeof(g_audio));
    g_audio.volume = 100;

    /* Initialize sound system */
    snd_init();

    /* Initialize streaming */
    snd_stream_init();

    /* Allocate audio buffers */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        g_audio.buffers[i] = (uint8 *)memalign(32, AUDIO_BUFFER_SIZE);
        if (!g_audio.buffers[i]) {
            LOG_ERROR("Failed to allocate audio buffer %d", i);
            return -1;
        }
        memset(g_audio.buffers[i], 0, AUDIO_BUFFER_SIZE);
    }

    /* Create stream handle */
    g_audio.stream = snd_stream_alloc(audio_stream_callback, AUDIO_BUFFER_SIZE);
    if (g_audio.stream == SND_STREAM_INVALID) {
        LOG_ERROR("Failed to allocate sound stream");
        return -1;
    }

    g_audio.initialized = true;
    LOG("Audio initialized");
    return 0;
}

/*
 * Shutdown audio subsystem
 */
void audio_shutdown(void)
{
    if (!g_audio.initialized) return;

    audio_stop();

    /* Free stream */
    if (g_audio.stream != SND_STREAM_INVALID) {
        snd_stream_destroy(g_audio.stream);
        g_audio.stream = SND_STREAM_INVALID;
    }

    /* Free buffers */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (g_audio.buffers[i]) {
            free(g_audio.buffers[i]);
            g_audio.buffers[i] = NULL;
        }
    }

    snd_stream_shutdown();
    snd_shutdown();

    g_audio.initialized = false;
    LOG("Audio shutdown");
}

/* Local WAV file state (for SD card playback via adapter) */
static struct {
    file_t handle;
    bool is_open;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t bytes_played;
    int sample_rate;
    int channels;
    int bits_per_sample;
} g_wav_state;

/*
 * Check if path is a local file (starts with /sd/ or /cd/)
 */
static bool is_local_path(const char *path)
{
    if (!path) return false;
    return (strncmp(path, "/sd/", 4) == 0 ||
            strncmp(path, "/cd/", 4) == 0 ||
            strncmp(path, "/pc/", 4) == 0 ||
            strncmp(path, "/ram/", 5) == 0);
}

/*
 * Open and parse WAV file header
 */
static int open_wav_file(const char *path)
{
    uint8_t header[44];

    g_wav_state.handle = fs_open(path, O_RDONLY);
    if (g_wav_state.handle == FILEHND_INVALID) {
        LOG_ERROR("Failed to open WAV file: %s", path);
        return -1;
    }

    /* Read RIFF header */
    if (fs_read(g_wav_state.handle, header, 44) != 44) {
        LOG_ERROR("Failed to read WAV header");
        fs_close(g_wav_state.handle);
        return -1;
    }

    /* Verify RIFF/WAVE signature */
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        LOG_ERROR("Not a valid WAV file");
        fs_close(g_wav_state.handle);
        return -1;
    }

    /* Check for PCM format (format code 1) */
    uint16_t audio_format = header[20] | (header[21] << 8);
    if (audio_format != 1) {
        LOG_ERROR("Only PCM WAV supported (format: %d)", audio_format);
        fs_close(g_wav_state.handle);
        return -1;
    }

    /* Parse format info */
    g_wav_state.channels = header[22] | (header[23] << 8);
    g_wav_state.sample_rate = header[24] | (header[25] << 8) |
                               (header[26] << 16) | (header[27] << 24);
    g_wav_state.bits_per_sample = header[34] | (header[35] << 8);

    /* Data chunk size */
    g_wav_state.data_size = header[40] | (header[41] << 8) |
                             (header[42] << 16) | (header[43] << 24);
    g_wav_state.data_offset = 44;
    g_wav_state.bytes_played = 0;
    g_wav_state.is_open = true;

    LOG("WAV: %d Hz, %d ch, %d bit, %u bytes",
        g_wav_state.sample_rate, g_wav_state.channels,
        g_wav_state.bits_per_sample, g_wav_state.data_size);

    /* Calculate duration */
    uint32_t bytes_per_sec = g_wav_state.sample_rate * g_wav_state.channels *
                              (g_wav_state.bits_per_sample / 8);
    g_audio.duration = (double)g_wav_state.data_size / bytes_per_sec;

    return 0;
}

/*
 * Fill buffer from local WAV file
 */
static int fill_buffer_local(int buf_idx)
{
    if (!g_wav_state.is_open || g_wav_state.handle == FILEHND_INVALID) {
        memset(g_audio.buffers[buf_idx], 0, AUDIO_BUFFER_SIZE);
        mutex_lock(&audio_mutex);
        g_audio.buffer_ready[buf_idx] = true;
        mutex_unlock(&audio_mutex);
        return 0;
    }

    /* Calculate how much to read */
    uint32_t remaining = g_wav_state.data_size - g_wav_state.bytes_played;
    size_t to_read = (remaining < AUDIO_BUFFER_SIZE) ? remaining : AUDIO_BUFFER_SIZE;

    if (to_read == 0) {
        /* End of file */
        memset(g_audio.buffers[buf_idx], 0, AUDIO_BUFFER_SIZE);
        g_audio.playing = false;
    } else {
        ssize_t bytes_read = fs_read(g_wav_state.handle,
                                      g_audio.buffers[buf_idx], to_read);
        if (bytes_read > 0) {
            g_wav_state.bytes_played += bytes_read;
            /* Zero-fill remainder */
            if ((size_t)bytes_read < AUDIO_BUFFER_SIZE) {
                memset(g_audio.buffers[buf_idx] + bytes_read, 0,
                       AUDIO_BUFFER_SIZE - bytes_read);
            }
        } else {
            memset(g_audio.buffers[buf_idx], 0, AUDIO_BUFFER_SIZE);
            g_audio.playing = false;
        }
    }

    mutex_lock(&audio_mutex);
    g_audio.buffer_ready[buf_idx] = true;
    mutex_unlock(&audio_mutex);

    return AUDIO_BUFFER_SIZE;
}

/*
 * Fill audio buffer from network stream
 * This should be called from the main thread periodically
 */
static int fill_buffer(int buf_idx)
{
    if (g_audio.buffer_ready[buf_idx]) {
        return 0;  /* Buffer already has data */
    }

    /* Check if we're playing a local file */
    if (g_wav_state.is_open) {
        return fill_buffer_local(buf_idx);
    }

    /*
     * Network streaming:
     * For best compatibility, the Nedflix server should transcode
     * audio to raw PCM (44100 Hz, stereo, 16-bit) before streaming.
     * This avoids the need for a software decoder on the DC.
     */
    if (g_audio.socket > 0) {
        ssize_t bytes = recv(g_audio.socket, g_audio.buffers[buf_idx],
                             AUDIO_BUFFER_SIZE, MSG_DONTWAIT);
        if (bytes > 0) {
            g_audio.bytes_received += bytes;
            if ((size_t)bytes < AUDIO_BUFFER_SIZE) {
                memset(g_audio.buffers[buf_idx] + bytes, 0,
                       AUDIO_BUFFER_SIZE - bytes);
            }
        } else if (bytes == 0) {
            /* Connection closed - end of stream */
            g_audio.playing = false;
            memset(g_audio.buffers[buf_idx], 0, AUDIO_BUFFER_SIZE);
        } else {
            /* Would block - fill with silence temporarily */
            memset(g_audio.buffers[buf_idx], 0, AUDIO_BUFFER_SIZE);
        }
    } else {
        /* No active source - silence */
        memset(g_audio.buffers[buf_idx], 0, AUDIO_BUFFER_SIZE);
    }

    mutex_lock(&audio_mutex);
    g_audio.buffer_ready[buf_idx] = true;
    mutex_unlock(&audio_mutex);

    return AUDIO_BUFFER_SIZE;
}

/*
 * Start playing audio from URL or local path
 */
int audio_play(const char *url)
{
    if (!g_audio.initialized) return -1;
    if (!url || strlen(url) == 0) return -1;

    /* Stop any current playback */
    audio_stop();

    LOG("Playing audio: %s", url);

    strncpy(g_audio.current_url, url, sizeof(g_audio.current_url) - 1);
    g_audio.position = 0.0;
    g_audio.duration = 0.0;

    /* Check if this is a local file */
    if (is_local_path(url)) {
        /* Local file playback - try to open as WAV */
        if (open_wav_file(url) != 0) {
            LOG_ERROR("Failed to open local audio file");
            return -1;
        }
    } else {
        /*
         * Network streaming:
         * 1. The Nedflix server should transcode to raw PCM
         * 2. Open streaming connection
         * 3. Start receiving audio data
         *
         * For network streams, duration comes from server metadata
         */
        g_audio.duration = 180.0;  /* Default estimate */
    }

    /* Pre-fill buffers */
    g_audio.buffer_ready[0] = false;
    g_audio.buffer_ready[1] = false;
    g_audio.current_buffer = 0;
    g_audio.buffer_pos = 0;

    fill_buffer(0);
    fill_buffer(1);

    /* Determine sample rate from source or use default */
    int sample_rate = AUDIO_SAMPLE_RATE;
    int channels = AUDIO_CHANNELS;

    if (g_wav_state.is_open) {
        sample_rate = g_wav_state.sample_rate;
        channels = g_wav_state.channels;
    }

    /* Start stream playback */
    snd_stream_start(g_audio.stream, sample_rate, channels - 1);

    /* Set initial volume */
    snd_stream_volume(g_audio.stream, g_audio.volume * 255 / 100);

    g_audio.playing = true;
    g_audio.paused = false;

    return 0;
}

/*
 * Stop playback
 */
void audio_stop(void)
{
    if (!g_audio.playing && !g_wav_state.is_open && g_audio.socket <= 0) {
        return;
    }

    LOG("Stopping audio playback");

    mutex_lock(&audio_mutex);

    snd_stream_stop(g_audio.stream);

    g_audio.playing = false;
    g_audio.paused = false;
    g_audio.position = 0.0;
    g_audio.current_url[0] = '\0';

    /* Clear buffers */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        g_audio.buffer_ready[i] = false;
        if (g_audio.buffers[i]) {
            memset(g_audio.buffers[i], 0, AUDIO_BUFFER_SIZE);
        }
    }

    mutex_unlock(&audio_mutex);

    /* Close local file if open */
    if (g_wav_state.is_open) {
        if (g_wav_state.handle != FILEHND_INVALID) {
            fs_close(g_wav_state.handle);
            g_wav_state.handle = FILEHND_INVALID;
        }
        g_wav_state.is_open = false;
        g_wav_state.bytes_played = 0;
    }

    /* Close network connection if open */
    if (g_audio.socket > 0) {
        close(g_audio.socket);
        g_audio.socket = 0;
    }

    /* Clean up decoder context */
    if (g_audio.decoder_ctx) {
        /* decoder_free(g_audio.decoder_ctx); */
        g_audio.decoder_ctx = NULL;
    }
}

/*
 * Pause playback
 */
void audio_pause(void)
{
    if (!g_audio.playing || g_audio.paused) return;

    LOG("Pausing audio");
    snd_stream_stop(g_audio.stream);
    g_audio.paused = true;
}

/*
 * Resume playback
 */
void audio_resume(void)
{
    if (!g_audio.playing || !g_audio.paused) return;

    LOG("Resuming audio");
    snd_stream_start(g_audio.stream, AUDIO_SAMPLE_RATE, AUDIO_CHANNELS - 1);
    g_audio.paused = false;
}

/*
 * Seek to position (seconds)
 */
void audio_seek(double seconds)
{
    if (!g_audio.playing) return;

    /* Clamp to valid range */
    if (seconds < 0) seconds = 0;
    if (seconds > g_audio.duration) seconds = g_audio.duration;

    LOG("Seeking audio to %.1f seconds", seconds);

    /*
     * In a real implementation:
     * 1. Calculate byte offset for seek position
     * 2. Send HTTP Range request for streaming
     * 3. Flush decoder and buffers
     * 4. Resume decoding from new position
     */

    g_audio.position = seconds;
}

/*
 * Set volume (0-100)
 */
void audio_set_volume(int volume)
{
    g_audio.volume = CLAMP(volume, 0, 100);

    if (g_audio.stream != SND_STREAM_INVALID) {
        snd_stream_volume(g_audio.stream, g_audio.volume * 255 / 100);
    }
}

/*
 * Get current volume
 */
int audio_get_volume(void)
{
    return g_audio.volume;
}

/*
 * Update audio streaming (call from main loop)
 */
void audio_update(void)
{
    if (!g_audio.playing || g_audio.paused) return;

    /* Keep buffers filled */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (!g_audio.buffer_ready[i]) {
            fill_buffer(i);
        }
    }

    /* Poll the stream to keep it running */
    snd_stream_poll(g_audio.stream);

    /* Check for end of stream */
    if (g_audio.position >= g_audio.duration) {
        LOG("Audio playback complete");
        g_audio.playing = false;
    }
}

/*
 * Check if audio is playing
 */
bool audio_is_playing(void)
{
    return g_audio.playing && !g_audio.paused;
}

/*
 * Get current playback position
 */
double audio_get_position(void)
{
    return g_audio.position;
}

/*
 * Get total duration
 */
double audio_get_duration(void)
{
    return g_audio.duration;
}

/*
 * Get current stream URL
 */
const char *audio_get_current_url(void)
{
    return g_audio.current_url;
}
