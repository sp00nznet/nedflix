/*
 * Nedflix Audio/Video Codec Implementation
 * Real decoding using dr_libs, stb_vorbis, and pl_mpeg
 */

#include "codecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * DR_LIBS CONFIGURATION - Single header libraries
 * https://github.com/mackron/dr_libs
 * ============================================================ */

#define DR_WAV_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#define DR_FLAC_IMPLEMENTATION

/* Memory allocation overrides for constrained platforms */
#ifndef CODEC_MALLOC
#define CODEC_MALLOC(sz) malloc(sz)
#endif
#ifndef CODEC_REALLOC
#define CODEC_REALLOC(p, sz) realloc(p, sz)
#endif
#ifndef CODEC_FREE
#define CODEC_FREE(p) free(p)
#endif

#define DRWAV_MALLOC(sz, ctx) CODEC_MALLOC(sz)
#define DRWAV_REALLOC(p, sz, ctx) CODEC_REALLOC(p, sz)
#define DRWAV_FREE(p, ctx) CODEC_FREE(p)

#define DRMP3_MALLOC(sz, ctx) CODEC_MALLOC(sz)
#define DRMP3_REALLOC(p, sz, ctx) CODEC_REALLOC(p, sz)
#define DRMP3_FREE(p, ctx) CODEC_FREE(p)

#define DRFLAC_MALLOC(sz, ctx) CODEC_MALLOC(sz)
#define DRFLAC_REALLOC(p, sz, ctx) CODEC_REALLOC(p, sz)
#define DRFLAC_FREE(p, ctx) CODEC_FREE(p)

#include "dr_wav.h"
#include "dr_mp3.h"
#include "dr_flac.h"

/* ============================================================
 * STB_VORBIS - OGG Vorbis decoder
 * https://github.com/nothings/stb
 * ============================================================ */

#define STB_VORBIS_NO_STDIO 0
#define STB_VORBIS_NO_PUSHDATA_API 1
#include "stb_vorbis.h"

/* ============================================================
 * PL_MPEG - MPEG1 Video decoder
 * https://github.com/phoboslab/pl_mpeg
 * ============================================================ */

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

/* ============================================================
 * AUDIO DECODER STRUCTURE
 * ============================================================ */

struct audio_decoder {
    codec_type_t type;
    audio_format_t format;
    bool eof;

    /* File handle for streaming */
    FILE *file;
    bool owns_file;

    /* Memory buffer for memory decoding */
    const void *memory;
    size_t memory_size;

    /* Codec-specific state */
    union {
        drwav wav;
        drmp3 mp3;
        drflac *flac;
        stb_vorbis *vorbis;
    } codec;

    /* For formats that need it */
    uint64_t current_sample;
    uint64_t total_samples;
};

/* ============================================================
 * VIDEO DECODER STRUCTURE
 * ============================================================ */

struct video_decoder {
    codec_type_t type;
    plm_t *plm;
    FILE *file;

    int width;
    int height;
    double framerate;
    double duration;
    double current_time;

    /* Frame buffer for RGBA conversion */
    uint32_t *rgba_buffer;
};

/* ============================================================
 * UTILITY FUNCTIONS
 * ============================================================ */

codec_type_t codec_detect_from_extension(const char *path)
{
    if (!path) return CODEC_UNKNOWN;

    const char *ext = strrchr(path, '.');
    if (!ext) return CODEC_UNKNOWN;
    ext++;

    if (strcasecmp(ext, "wav") == 0 || strcasecmp(ext, "wave") == 0)
        return CODEC_WAV;
    if (strcasecmp(ext, "mp3") == 0)
        return CODEC_MP3;
    if (strcasecmp(ext, "flac") == 0)
        return CODEC_FLAC;
    if (strcasecmp(ext, "ogg") == 0 || strcasecmp(ext, "oga") == 0)
        return CODEC_OGG;
    if (strcasecmp(ext, "aac") == 0 || strcasecmp(ext, "m4a") == 0)
        return CODEC_AAC;
    if (strcasecmp(ext, "mpg") == 0 || strcasecmp(ext, "mpeg") == 0)
        return CODEC_MPEG1_VIDEO;
    if (strcasecmp(ext, "mjpeg") == 0 || strcasecmp(ext, "mjpg") == 0)
        return CODEC_MJPEG;

    return CODEC_UNKNOWN;
}

codec_type_t codec_detect_from_header(const void *data, size_t size)
{
    if (!data || size < 12) return CODEC_UNKNOWN;

    const uint8_t *bytes = (const uint8_t *)data;

    /* WAV: RIFF....WAVE */
    if (size >= 12 &&
        bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
        bytes[8] == 'W' && bytes[9] == 'A' && bytes[10] == 'V' && bytes[11] == 'E') {
        return CODEC_WAV;
    }

    /* MP3: ID3 tag or sync word */
    if ((bytes[0] == 'I' && bytes[1] == 'D' && bytes[2] == '3') ||
        (bytes[0] == 0xFF && (bytes[1] & 0xE0) == 0xE0)) {
        return CODEC_MP3;
    }

    /* FLAC: fLaC */
    if (bytes[0] == 'f' && bytes[1] == 'L' && bytes[2] == 'a' && bytes[3] == 'C') {
        return CODEC_FLAC;
    }

    /* OGG: OggS */
    if (bytes[0] == 'O' && bytes[1] == 'g' && bytes[2] == 'g' && bytes[3] == 'S') {
        return CODEC_OGG;
    }

    /* MPEG: 0x000001BA or 0x000001B3 */
    if (size >= 4 &&
        bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x01 &&
        (bytes[3] == 0xBA || bytes[3] == 0xB3)) {
        return CODEC_MPEG1_VIDEO;
    }

    return CODEC_UNKNOWN;
}

const char *codec_type_to_string(codec_type_t type)
{
    switch (type) {
        case CODEC_WAV:         return "WAV";
        case CODEC_MP3:         return "MP3";
        case CODEC_FLAC:        return "FLAC";
        case CODEC_OGG:         return "OGG Vorbis";
        case CODEC_AAC:         return "AAC";
        case CODEC_MPEG1_VIDEO: return "MPEG-1";
        case CODEC_MJPEG:       return "Motion JPEG";
        default:                return "Unknown";
    }
}

/* ============================================================
 * AUDIO DECODER IMPLEMENTATION
 * ============================================================ */

static bool init_wav_decoder(audio_decoder_t *dec, const char *path)
{
    if (!drwav_init_file(&dec->codec.wav, path, NULL)) {
        return false;
    }

    dec->format.sample_rate = dec->codec.wav.sampleRate;
    dec->format.channels = dec->codec.wav.channels;
    dec->format.bits_per_sample = dec->codec.wav.bitsPerSample;
    dec->format.total_samples = dec->codec.wav.totalPCMFrameCount;
    dec->format.duration_ms = (uint32_t)((dec->format.total_samples * 1000ULL) /
                                          dec->format.sample_rate);
    dec->total_samples = dec->format.total_samples;
    return true;
}

static bool init_wav_decoder_memory(audio_decoder_t *dec, const void *data, size_t size)
{
    if (!drwav_init_memory(&dec->codec.wav, data, size, NULL)) {
        return false;
    }

    dec->format.sample_rate = dec->codec.wav.sampleRate;
    dec->format.channels = dec->codec.wav.channels;
    dec->format.bits_per_sample = dec->codec.wav.bitsPerSample;
    dec->format.total_samples = dec->codec.wav.totalPCMFrameCount;
    dec->format.duration_ms = (uint32_t)((dec->format.total_samples * 1000ULL) /
                                          dec->format.sample_rate);
    dec->total_samples = dec->format.total_samples;
    return true;
}

static bool init_mp3_decoder(audio_decoder_t *dec, const char *path)
{
    if (!drmp3_init_file(&dec->codec.mp3, path, NULL)) {
        return false;
    }

    dec->format.sample_rate = dec->codec.mp3.sampleRate;
    dec->format.channels = dec->codec.mp3.channels;
    dec->format.bits_per_sample = 16;  /* drmp3 outputs 16-bit */
    dec->format.total_samples = drmp3_get_pcm_frame_count(&dec->codec.mp3);
    dec->format.duration_ms = (uint32_t)((dec->format.total_samples * 1000ULL) /
                                          dec->format.sample_rate);
    dec->total_samples = dec->format.total_samples;
    return true;
}

static bool init_mp3_decoder_memory(audio_decoder_t *dec, const void *data, size_t size)
{
    if (!drmp3_init_memory(&dec->codec.mp3, data, size, NULL)) {
        return false;
    }

    dec->format.sample_rate = dec->codec.mp3.sampleRate;
    dec->format.channels = dec->codec.mp3.channels;
    dec->format.bits_per_sample = 16;
    dec->format.total_samples = drmp3_get_pcm_frame_count(&dec->codec.mp3);
    dec->format.duration_ms = (uint32_t)((dec->format.total_samples * 1000ULL) /
                                          dec->format.sample_rate);
    dec->total_samples = dec->format.total_samples;
    return true;
}

static bool init_flac_decoder(audio_decoder_t *dec, const char *path)
{
    dec->codec.flac = drflac_open_file(path, NULL);
    if (!dec->codec.flac) {
        return false;
    }

    dec->format.sample_rate = dec->codec.flac->sampleRate;
    dec->format.channels = dec->codec.flac->channels;
    dec->format.bits_per_sample = dec->codec.flac->bitsPerSample;
    dec->format.total_samples = dec->codec.flac->totalPCMFrameCount;
    dec->format.duration_ms = (uint32_t)((dec->format.total_samples * 1000ULL) /
                                          dec->format.sample_rate);
    dec->total_samples = dec->format.total_samples;
    return true;
}

static bool init_flac_decoder_memory(audio_decoder_t *dec, const void *data, size_t size)
{
    dec->codec.flac = drflac_open_memory(data, size, NULL);
    if (!dec->codec.flac) {
        return false;
    }

    dec->format.sample_rate = dec->codec.flac->sampleRate;
    dec->format.channels = dec->codec.flac->channels;
    dec->format.bits_per_sample = dec->codec.flac->bitsPerSample;
    dec->format.total_samples = dec->codec.flac->totalPCMFrameCount;
    dec->format.duration_ms = (uint32_t)((dec->format.total_samples * 1000ULL) /
                                          dec->format.sample_rate);
    dec->total_samples = dec->format.total_samples;
    return true;
}

static bool init_vorbis_decoder(audio_decoder_t *dec, const char *path)
{
    int error;
    dec->codec.vorbis = stb_vorbis_open_filename(path, &error, NULL);
    if (!dec->codec.vorbis) {
        return false;
    }

    stb_vorbis_info info = stb_vorbis_get_info(dec->codec.vorbis);
    dec->format.sample_rate = info.sample_rate;
    dec->format.channels = info.channels;
    dec->format.bits_per_sample = 16;
    dec->format.total_samples = stb_vorbis_stream_length_in_samples(dec->codec.vorbis);
    dec->format.duration_ms = (uint32_t)((dec->format.total_samples * 1000ULL) /
                                          dec->format.sample_rate);
    dec->total_samples = dec->format.total_samples;
    return true;
}

static bool init_vorbis_decoder_memory(audio_decoder_t *dec, const void *data, size_t size)
{
    int error;
    dec->codec.vorbis = stb_vorbis_open_memory((const unsigned char *)data,
                                                (int)size, &error, NULL);
    if (!dec->codec.vorbis) {
        return false;
    }

    stb_vorbis_info info = stb_vorbis_get_info(dec->codec.vorbis);
    dec->format.sample_rate = info.sample_rate;
    dec->format.channels = info.channels;
    dec->format.bits_per_sample = 16;
    dec->format.total_samples = stb_vorbis_stream_length_in_samples(dec->codec.vorbis);
    dec->format.duration_ms = (uint32_t)((dec->format.total_samples * 1000ULL) /
                                          dec->format.sample_rate);
    dec->total_samples = dec->format.total_samples;
    return true;
}

audio_decoder_t *audio_decoder_open_file(const char *path)
{
    if (!path) return NULL;

    audio_decoder_t *dec = (audio_decoder_t *)CODEC_MALLOC(sizeof(audio_decoder_t));
    if (!dec) return NULL;

    memset(dec, 0, sizeof(audio_decoder_t));

    /* Detect codec type */
    dec->type = codec_detect_from_extension(path);

    /* If extension detection failed, try header detection */
    if (dec->type == CODEC_UNKNOWN) {
        FILE *f = fopen(path, "rb");
        if (f) {
            uint8_t header[32];
            size_t read = fread(header, 1, sizeof(header), f);
            fclose(f);
            if (read > 0) {
                dec->type = codec_detect_from_header(header, read);
            }
        }
    }

    /* Initialize codec */
    bool success = false;
    switch (dec->type) {
        case CODEC_WAV:
            success = init_wav_decoder(dec, path);
            break;
        case CODEC_MP3:
            success = init_mp3_decoder(dec, path);
            break;
        case CODEC_FLAC:
            success = init_flac_decoder(dec, path);
            break;
        case CODEC_OGG:
            success = init_vorbis_decoder(dec, path);
            break;
        default:
            break;
    }

    if (!success) {
        CODEC_FREE(dec);
        return NULL;
    }

    return dec;
}

audio_decoder_t *audio_decoder_open_memory(const void *data, size_t size)
{
    if (!data || size == 0) return NULL;

    audio_decoder_t *dec = (audio_decoder_t *)CODEC_MALLOC(sizeof(audio_decoder_t));
    if (!dec) return NULL;

    memset(dec, 0, sizeof(audio_decoder_t));
    dec->memory = data;
    dec->memory_size = size;

    /* Detect codec from header */
    dec->type = codec_detect_from_header(data, size);

    /* Initialize codec */
    bool success = false;
    switch (dec->type) {
        case CODEC_WAV:
            success = init_wav_decoder_memory(dec, data, size);
            break;
        case CODEC_MP3:
            success = init_mp3_decoder_memory(dec, data, size);
            break;
        case CODEC_FLAC:
            success = init_flac_decoder_memory(dec, data, size);
            break;
        case CODEC_OGG:
            success = init_vorbis_decoder_memory(dec, data, size);
            break;
        default:
            break;
    }

    if (!success) {
        CODEC_FREE(dec);
        return NULL;
    }

    return dec;
}

void audio_decoder_close(audio_decoder_t *dec)
{
    if (!dec) return;

    switch (dec->type) {
        case CODEC_WAV:
            drwav_uninit(&dec->codec.wav);
            break;
        case CODEC_MP3:
            drmp3_uninit(&dec->codec.mp3);
            break;
        case CODEC_FLAC:
            if (dec->codec.flac) {
                drflac_close(dec->codec.flac);
            }
            break;
        case CODEC_OGG:
            if (dec->codec.vorbis) {
                stb_vorbis_close(dec->codec.vorbis);
            }
            break;
        default:
            break;
    }

    if (dec->file && dec->owns_file) {
        fclose(dec->file);
    }

    CODEC_FREE(dec);
}

bool audio_decoder_get_format(audio_decoder_t *dec, audio_format_t *format)
{
    if (!dec || !format) return false;
    *format = dec->format;
    return true;
}

int audio_decoder_read(audio_decoder_t *dec, int16_t *output, int samples_to_read)
{
    if (!dec || !output || samples_to_read <= 0) return 0;

    int samples_read = 0;

    switch (dec->type) {
        case CODEC_WAV:
            samples_read = (int)drwav_read_pcm_frames_s16(&dec->codec.wav,
                                                          samples_to_read, output);
            break;

        case CODEC_MP3:
            samples_read = (int)drmp3_read_pcm_frames_s16(&dec->codec.mp3,
                                                          samples_to_read, output);
            break;

        case CODEC_FLAC:
            if (dec->codec.flac) {
                samples_read = (int)drflac_read_pcm_frames_s16(dec->codec.flac,
                                                               samples_to_read, output);
            }
            break;

        case CODEC_OGG:
            if (dec->codec.vorbis) {
                samples_read = stb_vorbis_get_samples_short_interleaved(
                    dec->codec.vorbis, dec->format.channels,
                    output, samples_to_read * dec->format.channels);
            }
            break;

        default:
            break;
    }

    dec->current_sample += samples_read;
    if (samples_read == 0) {
        dec->eof = true;
    }

    return samples_read;
}

bool audio_decoder_seek(audio_decoder_t *dec, uint64_t sample_index)
{
    if (!dec) return false;

    bool success = false;

    switch (dec->type) {
        case CODEC_WAV:
            success = drwav_seek_to_pcm_frame(&dec->codec.wav, sample_index);
            break;

        case CODEC_MP3:
            success = drmp3_seek_to_pcm_frame(&dec->codec.mp3, sample_index);
            break;

        case CODEC_FLAC:
            if (dec->codec.flac) {
                success = drflac_seek_to_pcm_frame(dec->codec.flac, sample_index);
            }
            break;

        case CODEC_OGG:
            if (dec->codec.vorbis) {
                success = (stb_vorbis_seek(dec->codec.vorbis,
                                           (unsigned int)sample_index) == 1);
            }
            break;

        default:
            break;
    }

    if (success) {
        dec->current_sample = sample_index;
        dec->eof = false;
    }

    return success;
}

uint64_t audio_decoder_tell(audio_decoder_t *dec)
{
    if (!dec) return 0;
    return dec->current_sample;
}

bool audio_decoder_eof(audio_decoder_t *dec)
{
    if (!dec) return true;
    return dec->eof;
}

codec_type_t audio_decoder_get_type(audio_decoder_t *dec)
{
    if (!dec) return CODEC_UNKNOWN;
    return dec->type;
}

/* ============================================================
 * VIDEO DECODER IMPLEMENTATION (MPEG-1)
 * ============================================================ */

video_decoder_t *video_decoder_open_file(const char *path)
{
    if (!path) return NULL;

    video_decoder_t *dec = (video_decoder_t *)CODEC_MALLOC(sizeof(video_decoder_t));
    if (!dec) return NULL;

    memset(dec, 0, sizeof(video_decoder_t));

    /* Open with pl_mpeg */
    dec->plm = plm_create_with_filename(path);
    if (!dec->plm) {
        CODEC_FREE(dec);
        return NULL;
    }

    dec->type = CODEC_MPEG1_VIDEO;
    dec->width = plm_get_width(dec->plm);
    dec->height = plm_get_height(dec->plm);
    dec->framerate = plm_get_framerate(dec->plm);
    dec->duration = plm_get_duration(dec->plm);

    /* Allocate RGBA buffer for conversion */
    dec->rgba_buffer = (uint32_t *)CODEC_MALLOC(dec->width * dec->height * 4);

    return dec;
}

video_decoder_t *video_decoder_open_memory(const void *data, size_t size)
{
    if (!data || size == 0) return NULL;

    video_decoder_t *dec = (video_decoder_t *)CODEC_MALLOC(sizeof(video_decoder_t));
    if (!dec) return NULL;

    memset(dec, 0, sizeof(video_decoder_t));

    /* Open with pl_mpeg from memory */
    dec->plm = plm_create_with_memory((uint8_t *)data, size, FALSE);
    if (!dec->plm) {
        CODEC_FREE(dec);
        return NULL;
    }

    dec->type = CODEC_MPEG1_VIDEO;
    dec->width = plm_get_width(dec->plm);
    dec->height = plm_get_height(dec->plm);
    dec->framerate = plm_get_framerate(dec->plm);
    dec->duration = plm_get_duration(dec->plm);

    /* Allocate RGBA buffer */
    dec->rgba_buffer = (uint32_t *)CODEC_MALLOC(dec->width * dec->height * 4);

    return dec;
}

void video_decoder_close(video_decoder_t *dec)
{
    if (!dec) return;

    if (dec->plm) {
        plm_destroy(dec->plm);
    }

    if (dec->rgba_buffer) {
        CODEC_FREE(dec->rgba_buffer);
    }

    if (dec->file) {
        fclose(dec->file);
    }

    CODEC_FREE(dec);
}

bool video_decoder_get_info(video_decoder_t *dec, int *width, int *height,
                            double *framerate, double *duration)
{
    if (!dec) return false;

    if (width) *width = dec->width;
    if (height) *height = dec->height;
    if (framerate) *framerate = dec->framerate;
    if (duration) *duration = dec->duration;

    return true;
}

bool video_decoder_decode(video_decoder_t *dec, video_frame_t *frame)
{
    if (!dec || !dec->plm || !frame) return false;

    plm_frame_t *plm_frame = plm_decode_video(dec->plm);
    if (!plm_frame) {
        return false;  /* End of video or error */
    }

    frame->y_plane = plm_frame->y.data;
    frame->cb_plane = plm_frame->cb.data;
    frame->cr_plane = plm_frame->cr.data;
    frame->width = plm_frame->width;
    frame->height = plm_frame->height;
    frame->y_stride = plm_frame->y.width;
    frame->c_stride = plm_frame->cb.width;
    frame->timestamp = plm_frame->time;

    /* Convert to RGBA if buffer allocated */
    if (dec->rgba_buffer) {
        plm_frame_to_rgba(plm_frame, (uint8_t *)dec->rgba_buffer, dec->width * 4);
        frame->rgba = dec->rgba_buffer;
    } else {
        frame->rgba = NULL;
    }

    dec->current_time = plm_frame->time;

    return true;
}

bool video_decoder_seek(video_decoder_t *dec, double time)
{
    if (!dec || !dec->plm) return false;

    plm_seek(dec->plm, time, FALSE);
    dec->current_time = time;
    return true;
}

double video_decoder_tell(video_decoder_t *dec)
{
    if (!dec) return 0.0;
    return dec->current_time;
}

bool video_decoder_has_audio(video_decoder_t *dec)
{
    if (!dec || !dec->plm) return false;
    return plm_get_num_audio_streams(dec->plm) > 0;
}

int video_decoder_read_audio(video_decoder_t *dec, int16_t *output, int samples)
{
    if (!dec || !dec->plm || !output || samples <= 0) return 0;

    plm_samples_t *plm_samples = plm_decode_audio(dec->plm);
    if (!plm_samples) return 0;

    /* Convert float samples to int16 */
    int count = plm_samples->count;
    if (count > samples) count = samples;

    for (int i = 0; i < count * 2; i++) {  /* Stereo */
        float sample = plm_samples->interleaved[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        output[i] = (int16_t)(sample * 32767.0f);
    }

    return count;
}
