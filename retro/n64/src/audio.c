/*
 * Nedflix N64 - Audio streaming
 *
 * N64 audio is limited - RSP handles audio mixing
 * Network streaming requires special handling
 */

#include "nedflix.h"
#include <string.h>

static bool initialized = false;
static bool playing = false;
static bool paused = false;
static uint32_t position_ms = 0;
static uint32_t duration_ms = 0;
static int volume = 100;
static char current_url[MAX_URL_LENGTH];

static uint8_t stream_buffer[STREAM_BUFFER_SIZE];
static volatile int buffer_pos = 0;
static volatile int buffer_fill = 0;

int audio_init(void)
{
    LOG("Audio init");
    audio_init(44100, 4);
    initialized = true;
    return 0;
}

void audio_shutdown(void)
{
    if (initialized) {
        audio_close();
        initialized = false;
    }
}

int audio_play_stream(const char *url)
{
    if (!initialized) return -1;

    LOG("Playing: %s", url);

    strncpy(current_url, url, MAX_URL_LENGTH - 1);
    current_url[MAX_URL_LENGTH - 1] = '\0';

    buffer_pos = 0;
    buffer_fill = 0;
    position_ms = 0;
    duration_ms = 0;
    playing = true;
    paused = false;

    /*
     * Note: Actual network streaming would require:
     * 1. HTTP connection to server
     * 2. Streaming audio data into buffer
     * 3. Decoding (MP3/AAC) on CPU (very limited on N64)
     * 4. Feeding PCM to audio subsystem
     *
     * N64 has very limited CPU for software decoding.
     * In practice, would need pre-converted ADPCM or raw audio.
     */

    return 0;
}

void audio_stop(void)
{
    playing = false;
    paused = false;
    position_ms = 0;
    buffer_pos = 0;
    buffer_fill = 0;
    current_url[0] = '\0';
}

void audio_pause(void)
{
    paused = true;
}

void audio_resume(void)
{
    paused = false;
}

void audio_set_volume(int vol)
{
    volume = CLAMP(vol, 0, 100);
}

bool audio_is_playing(void)
{
    return playing && !paused;
}

uint32_t audio_get_position(void)
{
    return position_ms;
}

uint32_t audio_get_duration(void)
{
    return duration_ms;
}

void audio_update(void)
{
    if (!playing || paused) return;

    /* Simulate playback progress */
    position_ms += 16; /* ~60fps */

    /* In a real implementation:
     * - Check buffer level
     * - Fetch more data from network if needed
     * - Decode audio frames
     * - Submit to audio subsystem
     */
}
