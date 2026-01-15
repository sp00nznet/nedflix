/*
 * Nedflix for Nintendo GameCube
 * Audio playback using ASND library
 *
 * TECHNICAL DEMO / NOVELTY PORT
 *
 * Supports:
 *   - WAV files (PCM, 8/16-bit, mono/stereo)
 *   - Basic streaming from SD card
 *
 * Limitations:
 *   - No hardware MP3 decoding (would need software decoder)
 *   - Limited RAM for buffering (24 MB total system RAM)
 *   - Audio is buffered in ARAM (8 MB)
 */

#include "nedflix.h"

/* Audio state */
static bool g_audio_initialized = false;
static int g_current_voice = -1;
static bool g_audio_playing = false;
static bool g_audio_paused = false;
static double g_audio_position = 0.0;
static double g_audio_duration = 0.0;
static int g_audio_volume = 255;

/* Audio buffer in main RAM (for streaming) */
static uint8_t *g_audio_buffer = NULL;
static uint32_t g_buffer_size = 0;
static uint32_t g_buffer_position = 0;

/* WAV file header structure */
typedef struct {
    char riff[4];           /* "RIFF" */
    uint32_t file_size;
    char wave[4];           /* "WAVE" */
    char fmt[4];            /* "fmt " */
    uint32_t fmt_size;
    uint16_t audio_format;  /* 1 = PCM */
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} wav_header_t;

/* ASND voice callback */
static void audio_voice_callback(int voice)
{
    (void)voice;
    /* Voice finished playing */
    g_audio_playing = false;
}

/*
 * Initialize audio subsystem
 */
int audio_init(void)
{
    if (g_audio_initialized) {
        return 0;
    }

    /* Initialize ASND library */
    ASND_Init();
    ASND_Pause(0);  /* Unpause */

    g_audio_initialized = true;
    g_current_voice = -1;
    g_audio_playing = false;
    g_audio_paused = false;
    g_audio_volume = 255;

    return 0;
}

/*
 * Shutdown audio subsystem
 */
void audio_shutdown(void)
{
    if (!g_audio_initialized) {
        return;
    }

    /* Stop any playing audio */
    if (g_current_voice >= 0) {
        ASND_StopVoice(g_current_voice);
        g_current_voice = -1;
    }

    /* Free audio buffer */
    if (g_audio_buffer) {
        free(g_audio_buffer);
        g_audio_buffer = NULL;
        g_buffer_size = 0;
    }

    ASND_End();
    g_audio_initialized = false;
}

/*
 * Load WAV file into memory
 */
int audio_load_wav(const char *path, playback_state_t *state)
{
    if (!path || !state) {
        return -1;
    }

    /* Open file */
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        LOG_ERROR("Failed to open WAV file: %s", path);
        return -1;
    }

    /* Read WAV header */
    wav_header_t header;
    if (fread(&header, 1, sizeof(wav_header_t), fp) != sizeof(wav_header_t)) {
        LOG_ERROR("Failed to read WAV header");
        fclose(fp);
        return -1;
    }

    /* Verify RIFF/WAVE format */
    if (memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0) {
        LOG_ERROR("Not a valid WAV file");
        fclose(fp);
        return -1;
    }

    /* Verify PCM format */
    if (header.audio_format != 1) {
        LOG_ERROR("Only PCM WAV files supported");
        fclose(fp);
        return -1;
    }

    /* Skip to data chunk */
    char chunk_id[4];
    uint32_t chunk_size;
    uint32_t data_offset = sizeof(wav_header_t);

    /* Skip any extra fmt bytes */
    if (header.fmt_size > 16) {
        fseek(fp, header.fmt_size - 16, SEEK_CUR);
        data_offset += header.fmt_size - 16;
    }

    /* Find data chunk */
    while (fread(chunk_id, 1, 4, fp) == 4) {
        if (fread(&chunk_size, 1, 4, fp) != 4) {
            break;
        }
        data_offset += 8;

        if (memcmp(chunk_id, "data", 4) == 0) {
            /* Found data chunk */
            break;
        }

        /* Skip this chunk */
        fseek(fp, chunk_size, SEEK_CUR);
        data_offset += chunk_size;
    }

    if (memcmp(chunk_id, "data", 4) != 0) {
        LOG_ERROR("Could not find data chunk");
        fclose(fp);
        return -1;
    }

    /* Store format info */
    state->format.sample_rate = header.sample_rate;
    state->format.channels = header.num_channels;
    state->format.bits_per_sample = header.bits_per_sample;
    state->format.data_size = chunk_size;
    state->format.data_offset = data_offset;

    /* Calculate duration */
    uint32_t bytes_per_second = header.sample_rate * header.num_channels * (header.bits_per_sample / 8);
    state->duration = (double)chunk_size / bytes_per_second;

    /* Free previous buffer */
    if (g_audio_buffer) {
        free(g_audio_buffer);
        g_audio_buffer = NULL;
    }

    /* Allocate buffer for audio data */
    /* Limit to available RAM (leave some headroom) */
    g_buffer_size = MIN(chunk_size, 4 * 1024 * 1024);  /* Max 4 MB */
    g_audio_buffer = (uint8_t *)memalign(32, g_buffer_size);

    if (!g_audio_buffer) {
        LOG_ERROR("Failed to allocate audio buffer");
        fclose(fp);
        return -1;
    }

    /* Read audio data */
    size_t bytes_read = fread(g_audio_buffer, 1, g_buffer_size, fp);
    if (bytes_read == 0) {
        LOG_ERROR("Failed to read audio data");
        free(g_audio_buffer);
        g_audio_buffer = NULL;
        fclose(fp);
        return -1;
    }

    /* Flush data cache */
    DCFlushRange(g_audio_buffer, g_buffer_size);

    fclose(fp);

    /* Reset playback state */
    state->current_time = 0.0;
    state->is_playing = false;
    state->is_paused = false;
    state->audio_buffer = g_audio_buffer;
    state->buffer_size = bytes_read;
    state->play_position = 0;

    g_audio_duration = state->duration;
    g_audio_position = 0.0;
    g_buffer_position = 0;

    LOG("Loaded WAV: %d Hz, %d ch, %d bit, %.1f sec",
        header.sample_rate, header.num_channels, header.bits_per_sample, state->duration);

    return 0;
}

/*
 * Load MP3 file (stub - would need software decoder)
 */
int audio_load_mp3(const char *path, playback_state_t *state)
{
    (void)path;
    (void)state;

    /* MP3 decoding would require a software library like libmad
     * For this technical demo, we only support WAV files */
    LOG_ERROR("MP3 playback not supported on GameCube");
    return -1;
}

/*
 * Start audio playback
 */
int audio_play(playback_state_t *state)
{
    if (!g_audio_initialized || !state || !g_audio_buffer) {
        return -1;
    }

    /* Stop any current playback */
    if (g_current_voice >= 0) {
        ASND_StopVoice(g_current_voice);
    }

    /* Determine ASND format */
    int format;
    if (state->format.bits_per_sample == 16) {
        format = (state->format.channels == 2) ? VOICE_STEREO_16BIT : VOICE_MONO_16BIT;
    } else {
        format = (state->format.channels == 2) ? VOICE_STEREO_8BIT : VOICE_MONO_8BIT;
    }

    /* Set voice callback */
    g_current_voice = ASND_GetFirstUnusedVoice();
    if (g_current_voice < 0) {
        LOG_ERROR("No available audio voices");
        return -1;
    }

    /* Set volume */
    ASND_ChangeVolumeVoice(g_current_voice, g_audio_volume, g_audio_volume);

    /* Start playback */
    int result = ASND_SetVoice(
        g_current_voice,
        format,
        state->format.sample_rate,
        0,  /* Delay */
        g_audio_buffer,
        state->buffer_size,
        g_audio_volume,
        g_audio_volume,
        audio_voice_callback
    );

    if (result != SND_OK) {
        LOG_ERROR("Failed to start audio playback");
        return -1;
    }

    state->is_playing = true;
    state->is_paused = false;
    state->voice = g_current_voice;
    g_audio_playing = true;
    g_audio_paused = false;
    g_audio_position = 0.0;

    return 0;
}

/*
 * Stop audio playback
 */
void audio_stop(playback_state_t *state)
{
    if (g_current_voice >= 0) {
        ASND_StopVoice(g_current_voice);
        g_current_voice = -1;
    }

    if (state) {
        state->is_playing = false;
        state->is_paused = false;
        state->current_time = 0.0;
    }

    g_audio_playing = false;
    g_audio_paused = false;
    g_audio_position = 0.0;
}

/*
 * Pause audio playback
 */
void audio_pause(playback_state_t *state)
{
    if (g_current_voice >= 0 && g_audio_playing) {
        ASND_PauseVoice(g_current_voice, 1);
        g_audio_paused = true;

        if (state) {
            state->is_paused = true;
        }
    }
}

/*
 * Resume audio playback
 */
void audio_resume(playback_state_t *state)
{
    if (g_current_voice >= 0 && g_audio_paused) {
        ASND_PauseVoice(g_current_voice, 0);
        g_audio_paused = false;

        if (state) {
            state->is_paused = false;
        }
    }
}

/*
 * Set playback volume (0-255)
 */
void audio_set_volume(int volume)
{
    g_audio_volume = CLAMP(volume, 0, 255);

    if (g_current_voice >= 0) {
        ASND_ChangeVolumeVoice(g_current_voice, g_audio_volume, g_audio_volume);
    }
}

/*
 * Update audio state (called each frame)
 */
void audio_update(void)
{
    if (!g_audio_playing || g_audio_paused) {
        return;
    }

    /* Estimate position based on voice status */
    /* Note: ASND doesn't provide precise position tracking,
     * so we estimate based on elapsed frames */
    static uint32_t last_tick = 0;
    uint32_t current_tick = gettime();

    if (last_tick > 0) {
        double elapsed = (double)(current_tick - last_tick) / TB_TIMER_CLOCK;
        g_audio_position += elapsed;

        if (g_audio_position > g_audio_duration) {
            g_audio_position = g_audio_duration;
        }
    }
    last_tick = current_tick;

    /* Check if voice has stopped */
    if (g_current_voice >= 0 && ASND_StatusVoice(g_current_voice) == SND_UNUSED) {
        g_audio_playing = false;
        g_audio_position = g_audio_duration;
    }
}

/*
 * Check if audio is currently playing
 */
bool audio_is_playing(void)
{
    if (g_current_voice >= 0) {
        int status = ASND_StatusVoice(g_current_voice);
        return (status == SND_WORKING) && !g_audio_paused;
    }
    return false;
}

/*
 * Get current playback position in seconds
 */
double audio_get_position(void)
{
    return g_audio_position;
}

/*
 * Get total duration in seconds
 */
double audio_get_duration(void)
{
    return g_audio_duration;
}
