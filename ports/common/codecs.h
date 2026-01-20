/*
 * Nedflix Audio Codec Library
 * Real implementations using dr_libs and stb_vorbis
 *
 * This is a unified codec layer that works on GameCube, Switch, and PS3
 */

#ifndef NEDFLIX_CODECS_H
#define NEDFLIX_CODECS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Codec types */
typedef enum {
    CODEC_UNKNOWN = 0,
    CODEC_WAV,
    CODEC_MP3,
    CODEC_FLAC,
    CODEC_OGG,
    CODEC_AAC,
    CODEC_MPEG1_VIDEO,
    CODEC_MJPEG
} codec_type_t;

/* Audio format */
typedef struct {
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
    uint32_t total_samples;
    uint32_t duration_ms;
} audio_format_t;

/* Forward declarations */
typedef struct audio_decoder audio_decoder_t;
typedef struct video_decoder video_decoder_t;

/* ============================================================
 * AUDIO DECODER API
 * ============================================================ */

/* Create decoder from file path */
audio_decoder_t *audio_decoder_open_file(const char *path);

/* Create decoder from memory buffer */
audio_decoder_t *audio_decoder_open_memory(const void *data, size_t size);

/* Close decoder and free resources */
void audio_decoder_close(audio_decoder_t *dec);

/* Get audio format info */
bool audio_decoder_get_format(audio_decoder_t *dec, audio_format_t *format);

/* Decode samples - returns number of samples decoded (per channel) */
/* Output is always interleaved 16-bit signed PCM */
int audio_decoder_read(audio_decoder_t *dec, int16_t *output, int samples_to_read);

/* Seek to sample position */
bool audio_decoder_seek(audio_decoder_t *dec, uint64_t sample_index);

/* Get current position in samples */
uint64_t audio_decoder_tell(audio_decoder_t *dec);

/* Check if at end of stream */
bool audio_decoder_eof(audio_decoder_t *dec);

/* Get codec type */
codec_type_t audio_decoder_get_type(audio_decoder_t *dec);

/* ============================================================
 * VIDEO DECODER API
 * ============================================================ */

/* Video frame */
typedef struct {
    uint8_t *y_plane;
    uint8_t *cb_plane;
    uint8_t *cr_plane;
    uint32_t *rgba;      /* Converted RGBA if requested */
    int width;
    int height;
    int y_stride;
    int c_stride;
    double timestamp;
} video_frame_t;

/* Create video decoder from file */
video_decoder_t *video_decoder_open_file(const char *path);

/* Create video decoder from memory */
video_decoder_t *video_decoder_open_memory(const void *data, size_t size);

/* Close video decoder */
void video_decoder_close(video_decoder_t *dec);

/* Get video info */
bool video_decoder_get_info(video_decoder_t *dec, int *width, int *height,
                            double *framerate, double *duration);

/* Decode next frame - returns false if no more frames */
bool video_decoder_decode(video_decoder_t *dec, video_frame_t *frame);

/* Seek to time in seconds */
bool video_decoder_seek(video_decoder_t *dec, double time);

/* Get current time */
double video_decoder_tell(video_decoder_t *dec);

/* Check if has audio track */
bool video_decoder_has_audio(video_decoder_t *dec);

/* Get audio from video file */
int video_decoder_read_audio(video_decoder_t *dec, int16_t *output, int samples);

/* ============================================================
 * UTILITY FUNCTIONS
 * ============================================================ */

/* Detect codec from file extension */
codec_type_t codec_detect_from_extension(const char *path);

/* Detect codec from file header */
codec_type_t codec_detect_from_header(const void *data, size_t size);

/* Convert codec type to string */
const char *codec_type_to_string(codec_type_t type);

#endif /* NEDFLIX_CODECS_H */
