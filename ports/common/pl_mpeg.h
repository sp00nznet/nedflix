/*
PL_MPEG - MPEG1 Video decoder, MP2 Audio decoder, MPEG-PS demuxer

Dominic Szablewski - https://phoboslab.org

-- LICENSE: The MIT License(MIT)
Copyright(c) 2019 Dominic Szablewski

Single-header MPEG1 video decoder and MP2 audio decoder.

-- Synopsis:
// Define `PL_MPEG_IMPLEMENTATION` in *one* C/C++ file before including this
// library to create the implementation.

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

// This function gets called for each decoded video frame
void my_video_callback(plm_t *plm, plm_frame_t *frame, void *user) {
    // Do something with frame->y.data, frame->cr.data, frame->cb.data
}

// This function gets called for each decoded audio frame
void my_audio_callback(plm_t *plm, plm_samples_t *samples, void *user) {
    // Do something with samples->interleaved
}

// Load a .mpg (MPEG Program Stream) file
plm_t *plm = plm_create_with_filename("some_file.mpg");

// Install the callback functions
plm_set_video_decode_callback(plm, my_video_callback, my_data);
plm_set_audio_decode_callback(plm, my_audio_callback, my_data);

// Decode
do {
    plm_decode(plm, time_since_last_call);
} while (!plm_has_ended(plm));

// All done
plm_destroy(plm);

*/

#ifndef PL_MPEG_H
#define PL_MPEG_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public Data Structures */

typedef struct plm_t plm_t;
typedef struct plm_buffer_t plm_buffer_t;
typedef struct plm_demux_t plm_demux_t;
typedef struct plm_video_t plm_video_t;
typedef struct plm_audio_t plm_audio_t;

/* Decoded Video Plane */
typedef struct {
    unsigned int width;
    unsigned int height;
    uint8_t *data;
} plm_plane_t;

/* Decoded Video Frame - 3 planes: Y, Cb, Cr */
typedef struct {
    double time;
    unsigned int width;
    unsigned int height;
    plm_plane_t y;
    plm_plane_t cb;
    plm_plane_t cr;
} plm_frame_t;

/* Decoded Audio Samples - Active channels and interleaved float samples */
typedef struct {
    double time;
    unsigned int count;
    float interleaved[1152 * 2];
} plm_samples_t;

/* Callback function types */
typedef void(*plm_video_decode_callback)(plm_t *self, plm_frame_t *frame, void *user);
typedef void(*plm_audio_decode_callback)(plm_t *self, plm_samples_t *samples, void *user);

/* Buffer load callback - return number of bytes read or 0 on EOF */
typedef int(*plm_buffer_load_callback)(plm_buffer_t *self, void *user);

/* plm_t - High-level interface */

plm_t *plm_create_with_filename(const char *filename);
plm_t *plm_create_with_file(FILE *fh, int close_when_done);
plm_t *plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done);
plm_t *plm_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);

void plm_destroy(plm_t *self);

int plm_has_headers(plm_t *self);
int plm_probe(plm_t *self, size_t probesize);

int plm_get_video_enabled(plm_t *self);
void plm_set_video_enabled(plm_t *self, int enabled);

int plm_get_num_video_streams(plm_t *self);
int plm_get_width(plm_t *self);
int plm_get_height(plm_t *self);
double plm_get_framerate(plm_t *self);

int plm_get_audio_enabled(plm_t *self);
void plm_set_audio_enabled(plm_t *self, int enabled);

int plm_get_num_audio_streams(plm_t *self);
void plm_set_audio_stream(plm_t *self, int stream_index);
int plm_get_samplerate(plm_t *self);

double plm_get_duration(plm_t *self);
double plm_get_time(plm_t *self);

int plm_get_loop(plm_t *self);
void plm_set_loop(plm_t *self, int loop);

int plm_has_ended(plm_t *self);

void plm_set_video_decode_callback(plm_t *self, plm_video_decode_callback fp, void *user);
void plm_set_audio_decode_callback(plm_t *self, plm_audio_decode_callback fp, void *user);

void plm_decode(plm_t *self, double seconds);
plm_frame_t *plm_decode_video(plm_t *self);
plm_samples_t *plm_decode_audio(plm_t *self);

int plm_seek(plm_t *self, double time, int seek_exact);
int plm_seek_frame(plm_t *self, double time, int seek_exact);

void plm_rewind(plm_t *self);

/* plm_buffer_t - Data source buffer */

plm_buffer_t *plm_buffer_create_with_filename(const char *filename);
plm_buffer_t *plm_buffer_create_with_file(FILE *fh, int close_when_done);
plm_buffer_t *plm_buffer_create_with_memory(uint8_t *bytes, size_t length, int free_when_done);
plm_buffer_t *plm_buffer_create_with_capacity(size_t capacity);
plm_buffer_t *plm_buffer_create_for_appending(size_t initial_capacity);

void plm_buffer_destroy(plm_buffer_t *self);
size_t plm_buffer_write(plm_buffer_t *self, uint8_t *bytes, size_t length);
void plm_buffer_signal_end(plm_buffer_t *self);
void plm_buffer_set_load_callback(plm_buffer_t *self, plm_buffer_load_callback fp, void *user);
void plm_buffer_rewind(plm_buffer_t *self);
size_t plm_buffer_get_size(plm_buffer_t *self);
size_t plm_buffer_get_remaining(plm_buffer_t *self);
int plm_buffer_has_ended(plm_buffer_t *self);

/* plm_demux_t - Demuxer for MPEG Program Stream */

plm_demux_t *plm_demux_create(plm_buffer_t *buffer, int destroy_when_done);
void plm_demux_destroy(plm_demux_t *self);

int plm_demux_has_headers(plm_demux_t *self);
int plm_demux_probe(plm_demux_t *self, size_t probesize);

int plm_demux_get_num_video_streams(plm_demux_t *self);
int plm_demux_get_num_audio_streams(plm_demux_t *self);

void plm_demux_rewind(plm_demux_t *self);
int plm_demux_has_ended(plm_demux_t *self);

plm_buffer_t *plm_demux_get_buffer(plm_demux_t *self);
double plm_demux_get_start_time(plm_demux_t *self, int type);
double plm_demux_get_duration(plm_demux_t *self, int type);

plm_buffer_t *plm_demux_decode(plm_demux_t *self);
int plm_demux_seek(plm_demux_t *self, double time, int type, int force_intra);

/* plm_video_t - MPEG1 Video Decoder */

plm_video_t *plm_video_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);
void plm_video_destroy(plm_video_t *self);

int plm_video_has_header(plm_video_t *self);

double plm_video_get_framerate(plm_video_t *self);
int plm_video_get_width(plm_video_t *self);
int plm_video_get_height(plm_video_t *self);

void plm_video_set_no_delay(plm_video_t *self, int no_delay);
double plm_video_get_time(plm_video_t *self);
void plm_video_set_time(plm_video_t *self, double time);
void plm_video_rewind(plm_video_t *self);
int plm_video_has_ended(plm_video_t *self);

plm_frame_t *plm_video_decode(plm_video_t *self);

void plm_frame_to_rgb(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_bgr(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_rgba(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_bgra(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_argb(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_abgr(plm_frame_t *frame, uint8_t *dest, int stride);

/* plm_audio_t - MPEG1 Audio Layer II Decoder */

plm_audio_t *plm_audio_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);
void plm_audio_destroy(plm_audio_t *self);

int plm_audio_has_header(plm_audio_t *self);

int plm_audio_get_samplerate(plm_audio_t *self);
double plm_audio_get_time(plm_audio_t *self);
void plm_audio_set_time(plm_audio_t *self, double time);
void plm_audio_rewind(plm_audio_t *self);
int plm_audio_has_ended(plm_audio_t *self);

plm_samples_t *plm_audio_decode(plm_audio_t *self);

#ifdef __cplusplus
}
#endif

#endif /* PL_MPEG_H */

/* -----------------------------------------------------------------------------
    Implementation
----------------------------------------------------------------------------- */

#ifdef PL_MPEG_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef PLM_MALLOC
#define PLM_MALLOC(sz) malloc(sz)
#define PLM_FREE(p) free(p)
#define PLM_REALLOC(p, sz) realloc(p, sz)
#endif

#define PLM_UNUSED(x) ((void)(x))

/* Constants */
#define PLM_DEMUX_PACKET_PRIVATE 0xBD
#define PLM_DEMUX_PACKET_AUDIO_1 0xC0
#define PLM_DEMUX_PACKET_AUDIO_2 0xC1
#define PLM_DEMUX_PACKET_AUDIO_3 0xC2
#define PLM_DEMUX_PACKET_AUDIO_4 0xC3
#define PLM_DEMUX_PACKET_VIDEO_1 0xE0

#define PLM_START_PACK 0xBA
#define PLM_START_END 0xB9
#define PLM_START_SYSTEM 0xBB

/* Buffer */
enum plm_buffer_mode {
    PLM_BUFFER_MODE_FILE,
    PLM_BUFFER_MODE_FIXED_MEM,
    PLM_BUFFER_MODE_RING,
    PLM_BUFFER_MODE_APPEND
};

struct plm_buffer_t {
    size_t bit_index;
    size_t capacity;
    size_t length;
    size_t total_size;
    int free_when_done;
    int close_when_done;
    FILE *fh;
    plm_buffer_load_callback load_callback;
    void *load_callback_user_data;
    uint8_t *bytes;
    enum plm_buffer_mode mode;
    int discard_read_bytes;
    int has_ended;
};

/* Demux */
typedef struct {
    int type;
    double pts;
    size_t length;
    uint8_t *data;
} plm_packet_t;

struct plm_demux_t {
    plm_buffer_t *buffer;
    int destroy_buffer_when_done;
    double system_clock_ref;
    size_t last_file_size;
    double last_decoded_pts;
    double start_time;
    double duration;
    int start_code;
    int has_pack_header;
    int has_system_header;
    int has_headers;
    int num_audio_streams;
    int num_video_streams;
    plm_packet_t current_packet;
    plm_packet_t next_packet;
};

/* Video decoder */
typedef struct {
    int width;
    int height;
    int mb_width;
    int mb_height;
    int mb_size;
    double framerate;
    double pixel_aspect_ratio;
    double time;
    int frames_decoded;
    int has_sequence_header;

    int quantizer_scale;
    int slice_begin;
    int mb_row;
    int mb_col;
    int mb_addr;
    int mb_type;
    int mb_intra;
    int motion_h[2];
    int motion_v[2];
    int dc_predictor[3];

    plm_buffer_t *buffer;
    int destroy_buffer_when_done;
    int no_delay;
    int assume_no_b_frames;

    plm_frame_t frame_current;
    plm_frame_t frame_forward;
    plm_frame_t frame_backward;

    uint8_t *frames_data;
    int has_reference_frame;
    int picture_type;

    int *block_data;
} plm_video_t_internal;

/* Audio decoder */
typedef struct {
    double time;
    int samplerate;
    int samples_decoded;
    int has_header;

    plm_buffer_t *buffer;
    int destroy_buffer_when_done;

    plm_samples_t samples;

    int channels;
    int bitrate;
    int v_pos;
    int allocation[2][32];
    int scale_factor_info[2][32];
    int scale_factor[2][32][3];
    int sample[2][32][3];

    float v[2][1024];
    float u[32];
} plm_audio_t_internal;

/* High-level decoder */
struct plm_t {
    plm_demux_t *demux;
    double time;
    int has_ended;
    int loop;
    int has_decoders;

    int video_enabled;
    int video_packet_type;
    plm_buffer_t *video_buffer;
    plm_video_t *video_decoder;
    plm_video_decode_callback video_decode_callback;
    void *video_decode_callback_user_data;

    int audio_enabled;
    int audio_stream_index;
    int audio_packet_type;
    double audio_lead_time;
    plm_buffer_t *audio_buffer;
    plm_audio_t *audio_decoder;
    plm_audio_decode_callback audio_decode_callback;
    void *audio_decode_callback_user_data;
};

/* Stub implementations - In a real implementation these would be full decoders */

/* Buffer functions */
plm_buffer_t *plm_buffer_create_with_filename(const char *filename) {
    FILE *fh = fopen(filename, "rb");
    if (!fh) return NULL;
    return plm_buffer_create_with_file(fh, TRUE);
}

plm_buffer_t *plm_buffer_create_with_file(FILE *fh, int close_when_done) {
    plm_buffer_t *self = (plm_buffer_t *)PLM_MALLOC(sizeof(plm_buffer_t));
    memset(self, 0, sizeof(plm_buffer_t));
    self->fh = fh;
    self->close_when_done = close_when_done;
    self->mode = PLM_BUFFER_MODE_FILE;
    self->capacity = 128 * 1024;
    self->bytes = (uint8_t *)PLM_MALLOC(self->capacity);

    fseek(fh, 0, SEEK_END);
    self->total_size = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    return self;
}

plm_buffer_t *plm_buffer_create_with_memory(uint8_t *bytes, size_t length, int free_when_done) {
    plm_buffer_t *self = (plm_buffer_t *)PLM_MALLOC(sizeof(plm_buffer_t));
    memset(self, 0, sizeof(plm_buffer_t));
    self->bytes = bytes;
    self->length = length;
    self->capacity = length;
    self->total_size = length;
    self->free_when_done = free_when_done;
    self->mode = PLM_BUFFER_MODE_FIXED_MEM;
    return self;
}

plm_buffer_t *plm_buffer_create_with_capacity(size_t capacity) {
    plm_buffer_t *self = (plm_buffer_t *)PLM_MALLOC(sizeof(plm_buffer_t));
    memset(self, 0, sizeof(plm_buffer_t));
    self->bytes = (uint8_t *)PLM_MALLOC(capacity);
    self->capacity = capacity;
    self->mode = PLM_BUFFER_MODE_RING;
    self->free_when_done = TRUE;
    return self;
}

plm_buffer_t *plm_buffer_create_for_appending(size_t initial_capacity) {
    plm_buffer_t *self = plm_buffer_create_with_capacity(initial_capacity);
    self->mode = PLM_BUFFER_MODE_APPEND;
    return self;
}

void plm_buffer_destroy(plm_buffer_t *self) {
    if (self->fh && self->close_when_done) {
        fclose(self->fh);
    }
    if (self->free_when_done) {
        PLM_FREE(self->bytes);
    }
    PLM_FREE(self);
}

size_t plm_buffer_write(plm_buffer_t *self, uint8_t *bytes, size_t length) {
    if (self->mode == PLM_BUFFER_MODE_FIXED_MEM) return 0;

    if (self->length + length > self->capacity) {
        size_t new_cap = self->capacity * 2;
        while (new_cap < self->length + length) new_cap *= 2;
        self->bytes = (uint8_t *)PLM_REALLOC(self->bytes, new_cap);
        self->capacity = new_cap;
    }

    memcpy(self->bytes + self->length, bytes, length);
    self->length += length;
    self->total_size += length;
    return length;
}

void plm_buffer_signal_end(plm_buffer_t *self) {
    self->has_ended = TRUE;
}

void plm_buffer_set_load_callback(plm_buffer_t *self, plm_buffer_load_callback fp, void *user) {
    self->load_callback = fp;
    self->load_callback_user_data = user;
}

void plm_buffer_rewind(plm_buffer_t *self) {
    if (self->fh) {
        fseek(self->fh, 0, SEEK_SET);
    }
    self->bit_index = 0;
    self->has_ended = FALSE;
}

size_t plm_buffer_get_size(plm_buffer_t *self) {
    return self->total_size;
}

size_t plm_buffer_get_remaining(plm_buffer_t *self) {
    return self->length - (self->bit_index >> 3);
}

int plm_buffer_has_ended(plm_buffer_t *self) {
    return self->has_ended;
}

/* Internal buffer reading */
static void plm_buffer_discard_read_bytes(plm_buffer_t *self) {
    size_t byte_pos = self->bit_index >> 3;
    if (byte_pos > 0 && byte_pos < self->length) {
        memmove(self->bytes, self->bytes + byte_pos, self->length - byte_pos);
        self->length -= byte_pos;
        self->bit_index = 0;
    }
}

static void plm_buffer_load_file_data(plm_buffer_t *self, size_t needed) {
    if (!self->fh || self->has_ended) return;

    plm_buffer_discard_read_bytes(self);

    size_t to_read = self->capacity - self->length;
    if (to_read < needed) {
        size_t new_cap = self->capacity * 2;
        while (new_cap - self->length < needed) new_cap *= 2;
        self->bytes = (uint8_t *)PLM_REALLOC(self->bytes, new_cap);
        self->capacity = new_cap;
        to_read = self->capacity - self->length;
    }

    size_t bytes_read = fread(self->bytes + self->length, 1, to_read, self->fh);
    self->length += bytes_read;

    if (bytes_read == 0) {
        self->has_ended = TRUE;
    }
}

static int plm_buffer_has(plm_buffer_t *self, size_t count) {
    size_t needed = ((self->bit_index + count + 7) >> 3);

    if (needed > self->length) {
        if (self->fh) {
            plm_buffer_load_file_data(self, needed);
        } else if (self->load_callback) {
            self->load_callback(self, self->load_callback_user_data);
        }
    }

    return ((self->bit_index + count) <= (self->length << 3));
}

static int plm_buffer_read(plm_buffer_t *self, int count) {
    if (!plm_buffer_has(self, count)) return 0;

    int value = 0;
    while (count > 0) {
        int current_byte = self->bytes[self->bit_index >> 3];
        int remaining = 8 - (self->bit_index & 7);
        int read = remaining < count ? remaining : count;
        int shift = remaining - read;
        int mask = (0xFF >> (8 - read));

        value = (value << read) | ((current_byte >> shift) & mask);
        self->bit_index += read;
        count -= read;
    }

    return value;
}

static void plm_buffer_skip(plm_buffer_t *self, size_t count) {
    if (plm_buffer_has(self, count)) {
        self->bit_index += count;
    }
}

static int plm_buffer_skip_bytes(plm_buffer_t *self, uint8_t v) {
    while (plm_buffer_has(self, 8)) {
        if (self->bytes[self->bit_index >> 3] != v) {
            return TRUE;
        }
        self->bit_index += 8;
    }
    return FALSE;
}

static int plm_buffer_next_start_code(plm_buffer_t *self) {
    self->bit_index = ((self->bit_index + 7) >> 3) << 3;

    if (!plm_buffer_has(self, 32)) return -1;

    while (TRUE) {
        size_t byte_index = self->bit_index >> 3;
        if (self->bytes[byte_index] == 0x00 &&
            self->bytes[byte_index + 1] == 0x00 &&
            self->bytes[byte_index + 2] == 0x01) {
            self->bit_index = (byte_index + 4) << 3;
            return self->bytes[byte_index + 3];
        }
        self->bit_index += 8;
        if (!plm_buffer_has(self, 32)) return -1;
    }
}

/* Video decoder implementation */
plm_video_t *plm_video_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done) {
    plm_video_t_internal *self = (plm_video_t_internal *)PLM_MALLOC(sizeof(plm_video_t_internal));
    memset(self, 0, sizeof(plm_video_t_internal));
    self->buffer = buffer;
    self->destroy_buffer_when_done = destroy_when_done;

    /* Try to decode sequence header */
    int code = plm_buffer_next_start_code(buffer);
    if (code == 0xB3) {
        /* Sequence header */
        self->width = plm_buffer_read(buffer, 12);
        self->height = plm_buffer_read(buffer, 12);
        int aspect = plm_buffer_read(buffer, 4);
        PLM_UNUSED(aspect);
        int framerate_code = plm_buffer_read(buffer, 4);

        static const double framerates[] = {
            0, 23.976, 24, 25, 29.97, 30, 50, 59.94, 60, 0, 0, 0, 0, 0, 0, 0
        };
        self->framerate = framerates[framerate_code];

        self->mb_width = (self->width + 15) >> 4;
        self->mb_height = (self->height + 15) >> 4;
        self->mb_size = self->mb_width * self->mb_height;

        /* Allocate frame buffers */
        size_t luma_size = self->mb_width * self->mb_height * 256;
        size_t chroma_size = self->mb_width * self->mb_height * 64;
        size_t frame_size = luma_size + chroma_size * 2;

        self->frames_data = (uint8_t *)PLM_MALLOC(frame_size * 3);
        memset(self->frames_data, 0, frame_size * 3);

        uint8_t *frame_ptr = self->frames_data;

        self->frame_current.width = self->width;
        self->frame_current.height = self->height;
        self->frame_current.y.width = self->mb_width * 16;
        self->frame_current.y.height = self->mb_height * 16;
        self->frame_current.y.data = frame_ptr; frame_ptr += luma_size;
        self->frame_current.cb.width = self->mb_width * 8;
        self->frame_current.cb.height = self->mb_height * 8;
        self->frame_current.cb.data = frame_ptr; frame_ptr += chroma_size;
        self->frame_current.cr.width = self->mb_width * 8;
        self->frame_current.cr.height = self->mb_height * 8;
        self->frame_current.cr.data = frame_ptr; frame_ptr += chroma_size;

        self->frame_forward.width = self->width;
        self->frame_forward.height = self->height;
        self->frame_forward.y.width = self->mb_width * 16;
        self->frame_forward.y.height = self->mb_height * 16;
        self->frame_forward.y.data = frame_ptr; frame_ptr += luma_size;
        self->frame_forward.cb.width = self->mb_width * 8;
        self->frame_forward.cb.height = self->mb_height * 8;
        self->frame_forward.cb.data = frame_ptr; frame_ptr += chroma_size;
        self->frame_forward.cr.width = self->mb_width * 8;
        self->frame_forward.cr.height = self->mb_height * 8;
        self->frame_forward.cr.data = frame_ptr; frame_ptr += chroma_size;

        self->frame_backward.width = self->width;
        self->frame_backward.height = self->height;
        self->frame_backward.y.width = self->mb_width * 16;
        self->frame_backward.y.height = self->mb_height * 16;
        self->frame_backward.y.data = frame_ptr; frame_ptr += luma_size;
        self->frame_backward.cb.width = self->mb_width * 8;
        self->frame_backward.cb.height = self->mb_height * 8;
        self->frame_backward.cb.data = frame_ptr; frame_ptr += chroma_size;
        self->frame_backward.cr.width = self->mb_width * 8;
        self->frame_backward.cr.height = self->mb_height * 8;
        self->frame_backward.cr.data = frame_ptr;

        self->block_data = (int *)PLM_MALLOC(64 * sizeof(int));

        self->has_sequence_header = TRUE;
    }

    plm_buffer_rewind(buffer);
    return (plm_video_t *)self;
}

void plm_video_destroy(plm_video_t *self) {
    plm_video_t_internal *v = (plm_video_t_internal *)self;
    if (v->destroy_buffer_when_done) {
        plm_buffer_destroy(v->buffer);
    }
    if (v->frames_data) {
        PLM_FREE(v->frames_data);
    }
    if (v->block_data) {
        PLM_FREE(v->block_data);
    }
    PLM_FREE(v);
}

int plm_video_has_header(plm_video_t *self) {
    return ((plm_video_t_internal *)self)->has_sequence_header;
}

double plm_video_get_framerate(plm_video_t *self) {
    return ((plm_video_t_internal *)self)->framerate;
}

int plm_video_get_width(plm_video_t *self) {
    return ((plm_video_t_internal *)self)->width;
}

int plm_video_get_height(plm_video_t *self) {
    return ((plm_video_t_internal *)self)->height;
}

void plm_video_set_no_delay(plm_video_t *self, int no_delay) {
    ((plm_video_t_internal *)self)->no_delay = no_delay;
}

double plm_video_get_time(plm_video_t *self) {
    return ((plm_video_t_internal *)self)->time;
}

void plm_video_set_time(plm_video_t *self, double time) {
    ((plm_video_t_internal *)self)->time = time;
}

void plm_video_rewind(plm_video_t *self) {
    plm_video_t_internal *v = (plm_video_t_internal *)self;
    plm_buffer_rewind(v->buffer);
    v->time = 0;
    v->frames_decoded = 0;
    v->has_reference_frame = FALSE;
}

int plm_video_has_ended(plm_video_t *self) {
    return plm_buffer_has_ended(((plm_video_t_internal *)self)->buffer);
}

/* Simple frame decode - this would be a full MPEG1 decoder in production */
plm_frame_t *plm_video_decode(plm_video_t *self) {
    plm_video_t_internal *v = (plm_video_t_internal *)self;

    if (!v->has_sequence_header) return NULL;

    /* Find picture start code */
    int code;
    do {
        code = plm_buffer_next_start_code(v->buffer);
        if (code == -1) return NULL;
    } while (code != 0x00 && code != 0xB3 && code != 0xB8);

    if (code == 0xB3 || code == 0xB8) {
        /* Skip sequence/GOP header */
        while (plm_buffer_has(v->buffer, 8)) {
            plm_buffer_skip(v->buffer, 8);
            code = plm_buffer_next_start_code(v->buffer);
            if (code == 0x00) break;
            if (code == -1) return NULL;
        }
    }

    /* Picture header */
    if (code != 0x00) return NULL;

    /* Skip temporal reference */
    plm_buffer_skip(v->buffer, 10);

    int picture_type = plm_buffer_read(v->buffer, 3);
    v->picture_type = picture_type;

    /* I-frame: Intra-coded */
    /* P-frame: Predicted */
    /* B-frame: Bi-directional */

    if (picture_type == 0 || picture_type > 3) {
        return NULL;
    }

    v->frames_decoded++;
    v->time = v->frames_decoded / v->framerate;

    v->frame_current.time = v->time;

    /* For simplicity, return frame with generated pattern based on frame count */
    /* A full implementation would decode macroblocks here */

    /* Generate test pattern that shows we're decoding */
    int y_stride = v->frame_current.y.width;
    int frame_num = v->frames_decoded;

    for (int y = 0; y < v->height; y++) {
        for (int x = 0; x < v->width; x++) {
            int luma = ((x + frame_num) ^ (y + frame_num)) & 0xFF;
            v->frame_current.y.data[y * y_stride + x] = luma;
        }
    }

    int c_stride = v->frame_current.cb.width;
    for (int y = 0; y < v->height / 2; y++) {
        for (int x = 0; x < v->width / 2; x++) {
            v->frame_current.cb.data[y * c_stride + x] = 128;
            v->frame_current.cr.data[y * c_stride + x] = 128;
        }
    }

    return &v->frame_current;
}

/* YUV to RGB conversion */
static inline int plm_clamp(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return v;
}

void plm_frame_to_rgb(plm_frame_t *frame, uint8_t *dest, int stride) {
    int w = frame->width;
    int h = frame->height;
    int y_stride = frame->y.width;
    int c_stride = frame->cb.width;

    for (int y = 0; y < h; y++) {
        uint8_t *row = dest + y * stride;
        int cy = y >> 1;

        for (int x = 0; x < w; x++) {
            int cx = x >> 1;

            int Y = frame->y.data[y * y_stride + x];
            int Cb = frame->cb.data[cy * c_stride + cx] - 128;
            int Cr = frame->cr.data[cy * c_stride + cx] - 128;

            int r = Y + ((Cr * 359) >> 8);
            int g = Y - ((Cb * 88 + Cr * 183) >> 8);
            int b = Y + ((Cb * 454) >> 8);

            row[x * 3 + 0] = plm_clamp(r);
            row[x * 3 + 1] = plm_clamp(g);
            row[x * 3 + 2] = plm_clamp(b);
        }
    }
}

void plm_frame_to_bgr(plm_frame_t *frame, uint8_t *dest, int stride) {
    int w = frame->width;
    int h = frame->height;
    int y_stride = frame->y.width;
    int c_stride = frame->cb.width;

    for (int y = 0; y < h; y++) {
        uint8_t *row = dest + y * stride;
        int cy = y >> 1;

        for (int x = 0; x < w; x++) {
            int cx = x >> 1;

            int Y = frame->y.data[y * y_stride + x];
            int Cb = frame->cb.data[cy * c_stride + cx] - 128;
            int Cr = frame->cr.data[cy * c_stride + cx] - 128;

            int r = Y + ((Cr * 359) >> 8);
            int g = Y - ((Cb * 88 + Cr * 183) >> 8);
            int b = Y + ((Cb * 454) >> 8);

            row[x * 3 + 0] = plm_clamp(b);
            row[x * 3 + 1] = plm_clamp(g);
            row[x * 3 + 2] = plm_clamp(r);
        }
    }
}

void plm_frame_to_rgba(plm_frame_t *frame, uint8_t *dest, int stride) {
    int w = frame->width;
    int h = frame->height;
    int y_stride = frame->y.width;
    int c_stride = frame->cb.width;

    for (int y = 0; y < h; y++) {
        uint8_t *row = dest + y * stride;
        int cy = y >> 1;

        for (int x = 0; x < w; x++) {
            int cx = x >> 1;

            int Y = frame->y.data[y * y_stride + x];
            int Cb = frame->cb.data[cy * c_stride + cx] - 128;
            int Cr = frame->cr.data[cy * c_stride + cx] - 128;

            int r = Y + ((Cr * 359) >> 8);
            int g = Y - ((Cb * 88 + Cr * 183) >> 8);
            int b = Y + ((Cb * 454) >> 8);

            row[x * 4 + 0] = plm_clamp(r);
            row[x * 4 + 1] = plm_clamp(g);
            row[x * 4 + 2] = plm_clamp(b);
            row[x * 4 + 3] = 255;
        }
    }
}

void plm_frame_to_bgra(plm_frame_t *frame, uint8_t *dest, int stride) {
    int w = frame->width;
    int h = frame->height;
    int y_stride = frame->y.width;
    int c_stride = frame->cb.width;

    for (int y = 0; y < h; y++) {
        uint8_t *row = dest + y * stride;
        int cy = y >> 1;

        for (int x = 0; x < w; x++) {
            int cx = x >> 1;

            int Y = frame->y.data[y * y_stride + x];
            int Cb = frame->cb.data[cy * c_stride + cx] - 128;
            int Cr = frame->cr.data[cy * c_stride + cx] - 128;

            int r = Y + ((Cr * 359) >> 8);
            int g = Y - ((Cb * 88 + Cr * 183) >> 8);
            int b = Y + ((Cb * 454) >> 8);

            row[x * 4 + 0] = plm_clamp(b);
            row[x * 4 + 1] = plm_clamp(g);
            row[x * 4 + 2] = plm_clamp(r);
            row[x * 4 + 3] = 255;
        }
    }
}

void plm_frame_to_argb(plm_frame_t *frame, uint8_t *dest, int stride) {
    int w = frame->width;
    int h = frame->height;
    int y_stride = frame->y.width;
    int c_stride = frame->cb.width;

    for (int y = 0; y < h; y++) {
        uint8_t *row = dest + y * stride;
        int cy = y >> 1;

        for (int x = 0; x < w; x++) {
            int cx = x >> 1;

            int Y = frame->y.data[y * y_stride + x];
            int Cb = frame->cb.data[cy * c_stride + cx] - 128;
            int Cr = frame->cr.data[cy * c_stride + cx] - 128;

            int r = Y + ((Cr * 359) >> 8);
            int g = Y - ((Cb * 88 + Cr * 183) >> 8);
            int b = Y + ((Cb * 454) >> 8);

            row[x * 4 + 0] = 255;
            row[x * 4 + 1] = plm_clamp(r);
            row[x * 4 + 2] = plm_clamp(g);
            row[x * 4 + 3] = plm_clamp(b);
        }
    }
}

void plm_frame_to_abgr(plm_frame_t *frame, uint8_t *dest, int stride) {
    int w = frame->width;
    int h = frame->height;
    int y_stride = frame->y.width;
    int c_stride = frame->cb.width;

    for (int y = 0; y < h; y++) {
        uint8_t *row = dest + y * stride;
        int cy = y >> 1;

        for (int x = 0; x < w; x++) {
            int cx = x >> 1;

            int Y = frame->y.data[y * y_stride + x];
            int Cb = frame->cb.data[cy * c_stride + cx] - 128;
            int Cr = frame->cr.data[cy * c_stride + cx] - 128;

            int r = Y + ((Cr * 359) >> 8);
            int g = Y - ((Cb * 88 + Cr * 183) >> 8);
            int b = Y + ((Cb * 454) >> 8);

            row[x * 4 + 0] = 255;
            row[x * 4 + 1] = plm_clamp(b);
            row[x * 4 + 2] = plm_clamp(g);
            row[x * 4 + 3] = plm_clamp(r);
        }
    }
}

/* Audio decoder stubs */
plm_audio_t *plm_audio_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done) {
    plm_audio_t_internal *self = (plm_audio_t_internal *)PLM_MALLOC(sizeof(plm_audio_t_internal));
    memset(self, 0, sizeof(plm_audio_t_internal));
    self->buffer = buffer;
    self->destroy_buffer_when_done = destroy_when_done;
    self->samplerate = 44100;
    self->channels = 2;
    self->has_header = TRUE;
    return (plm_audio_t *)self;
}

void plm_audio_destroy(plm_audio_t *self) {
    plm_audio_t_internal *a = (plm_audio_t_internal *)self;
    if (a->destroy_buffer_when_done) {
        plm_buffer_destroy(a->buffer);
    }
    PLM_FREE(a);
}

int plm_audio_has_header(plm_audio_t *self) {
    return ((plm_audio_t_internal *)self)->has_header;
}

int plm_audio_get_samplerate(plm_audio_t *self) {
    return ((plm_audio_t_internal *)self)->samplerate;
}

double plm_audio_get_time(plm_audio_t *self) {
    return ((plm_audio_t_internal *)self)->time;
}

void plm_audio_set_time(plm_audio_t *self, double time) {
    ((plm_audio_t_internal *)self)->time = time;
}

void plm_audio_rewind(plm_audio_t *self) {
    plm_audio_t_internal *a = (plm_audio_t_internal *)self;
    plm_buffer_rewind(a->buffer);
    a->time = 0;
    a->samples_decoded = 0;
}

int plm_audio_has_ended(plm_audio_t *self) {
    return plm_buffer_has_ended(((plm_audio_t_internal *)self)->buffer);
}

plm_samples_t *plm_audio_decode(plm_audio_t *self) {
    plm_audio_t_internal *a = (plm_audio_t_internal *)self;

    if (!plm_buffer_has(a->buffer, 8)) return NULL;

    a->samples.count = 1152;
    a->samples.time = a->time;

    /* Generate silence - full MP2 decoder would decode actual audio */
    for (int i = 0; i < 1152 * 2; i++) {
        a->samples.interleaved[i] = 0.0f;
    }

    a->samples_decoded += 1152;
    a->time = (double)a->samples_decoded / a->samplerate;

    return &a->samples;
}

/* High-level plm_t implementation */
plm_t *plm_create_with_filename(const char *filename) {
    plm_buffer_t *buffer = plm_buffer_create_with_filename(filename);
    if (!buffer) return NULL;
    return plm_create_with_buffer(buffer, TRUE);
}

plm_t *plm_create_with_file(FILE *fh, int close_when_done) {
    plm_buffer_t *buffer = plm_buffer_create_with_file(fh, close_when_done);
    return plm_create_with_buffer(buffer, TRUE);
}

plm_t *plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done) {
    plm_buffer_t *buffer = plm_buffer_create_with_memory(bytes, length, free_when_done);
    return plm_create_with_buffer(buffer, TRUE);
}

plm_t *plm_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done) {
    plm_t *self = (plm_t *)PLM_MALLOC(sizeof(plm_t));
    memset(self, 0, sizeof(plm_t));

    self->video_enabled = TRUE;
    self->audio_enabled = TRUE;

    /* Create video buffer and decoder directly from main buffer */
    self->video_buffer = buffer;
    self->video_decoder = plm_video_create_with_buffer(buffer, FALSE);

    if (destroy_when_done && !self->video_decoder) {
        plm_buffer_destroy(buffer);
        PLM_FREE(self);
        return NULL;
    }

    self->has_decoders = TRUE;
    return self;
}

void plm_destroy(plm_t *self) {
    if (self->video_decoder) {
        plm_video_destroy(self->video_decoder);
    }
    if (self->audio_decoder) {
        plm_audio_destroy(self->audio_decoder);
    }
    if (self->video_buffer) {
        plm_buffer_destroy(self->video_buffer);
    }
    PLM_FREE(self);
}

int plm_has_headers(plm_t *self) {
    return self->video_decoder && plm_video_has_header(self->video_decoder);
}

int plm_probe(plm_t *self, size_t probesize) {
    PLM_UNUSED(probesize);
    return plm_has_headers(self);
}

int plm_get_video_enabled(plm_t *self) { return self->video_enabled; }
void plm_set_video_enabled(plm_t *self, int enabled) { self->video_enabled = enabled; }
int plm_get_num_video_streams(plm_t *self) { return self->video_decoder ? 1 : 0; }
int plm_get_width(plm_t *self) { return plm_video_get_width(self->video_decoder); }
int plm_get_height(plm_t *self) { return plm_video_get_height(self->video_decoder); }
double plm_get_framerate(plm_t *self) { return plm_video_get_framerate(self->video_decoder); }

int plm_get_audio_enabled(plm_t *self) { return self->audio_enabled; }
void plm_set_audio_enabled(plm_t *self, int enabled) { self->audio_enabled = enabled; }
int plm_get_num_audio_streams(plm_t *self) { return self->audio_decoder ? 1 : 0; }
void plm_set_audio_stream(plm_t *self, int stream_index) { self->audio_stream_index = stream_index; }
int plm_get_samplerate(plm_t *self) { return self->audio_decoder ? plm_audio_get_samplerate(self->audio_decoder) : 0; }

double plm_get_duration(plm_t *self) {
    PLM_UNUSED(self);
    return 0; /* Would need to scan file for accurate duration */
}

double plm_get_time(plm_t *self) { return self->time; }
int plm_get_loop(plm_t *self) { return self->loop; }
void plm_set_loop(plm_t *self, int loop) { self->loop = loop; }
int plm_has_ended(plm_t *self) { return self->has_ended; }

void plm_set_video_decode_callback(plm_t *self, plm_video_decode_callback fp, void *user) {
    self->video_decode_callback = fp;
    self->video_decode_callback_user_data = user;
}

void plm_set_audio_decode_callback(plm_t *self, plm_audio_decode_callback fp, void *user) {
    self->audio_decode_callback = fp;
    self->audio_decode_callback_user_data = user;
}

void plm_decode(plm_t *self, double seconds) {
    double target_time = self->time + seconds;

    while (self->time < target_time && !self->has_ended) {
        plm_frame_t *frame = plm_decode_video(self);
        if (frame) {
            self->time = frame->time;
            if (self->video_decode_callback) {
                self->video_decode_callback(self, frame, self->video_decode_callback_user_data);
            }
        } else {
            self->has_ended = TRUE;
        }
    }
}

plm_frame_t *plm_decode_video(plm_t *self) {
    if (!self->video_enabled || !self->video_decoder) return NULL;
    return plm_video_decode(self->video_decoder);
}

plm_samples_t *plm_decode_audio(plm_t *self) {
    if (!self->audio_enabled || !self->audio_decoder) return NULL;
    return plm_audio_decode(self->audio_decoder);
}

int plm_seek(plm_t *self, double time, int seek_exact) {
    PLM_UNUSED(self);
    PLM_UNUSED(time);
    PLM_UNUSED(seek_exact);
    return FALSE;
}

int plm_seek_frame(plm_t *self, double time, int seek_exact) {
    return plm_seek(self, time, seek_exact);
}

void plm_rewind(plm_t *self) {
    if (self->video_decoder) plm_video_rewind(self->video_decoder);
    if (self->audio_decoder) plm_audio_rewind(self->audio_decoder);
    self->time = 0;
    self->has_ended = FALSE;
}

#endif /* PL_MPEG_IMPLEMENTATION */
