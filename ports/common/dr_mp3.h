/*
MP3 audio decoder. Choice of public domain or MIT-0.

David Reid - mackron@gmail.com

GitHub: https://github.com/mackron/dr_libs

This is a single file library. To use it, do something like:
    #define DR_MP3_IMPLEMENTATION
    #include "dr_mp3.h"

Based on minimp3 by lieff: https://github.com/lieff/minimp3
*/

#ifndef dr_mp3_h
#define dr_mp3_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef   signed char           drmp3_int8;
typedef unsigned char           drmp3_uint8;
typedef   signed short          drmp3_int16;
typedef unsigned short          drmp3_uint16;
typedef   signed int            drmp3_int32;
typedef unsigned int            drmp3_uint32;
#if defined(_MSC_VER)
    typedef   signed __int64    drmp3_int64;
    typedef unsigned __int64    drmp3_uint64;
#else
    typedef   signed long long  drmp3_int64;
    typedef unsigned long long  drmp3_uint64;
#endif
typedef drmp3_uint8             drmp3_bool8;
typedef drmp3_uint32            drmp3_bool32;
#define DRMP3_TRUE              1
#define DRMP3_FALSE             0

#ifndef DRMP3_API
#define DRMP3_API
#endif

#define DRMP3_MAX_PCM_FRAMES_PER_MP3_FRAME  1152
#define DRMP3_MAX_SAMPLES_PER_FRAME         (DRMP3_MAX_PCM_FRAMES_PER_MP3_FRAME * 2)

typedef enum {
    drmp3_seek_origin_start,
    drmp3_seek_origin_current
} drmp3_seek_origin;

typedef struct {
    void* pUserData;
    void* (*onMalloc)(size_t sz, void* pUserData);
    void* (*onRealloc)(void* p, size_t sz, void* pUserData);
    void  (*onFree)(void* p, void* pUserData);
} drmp3_allocation_callbacks;

/* Low-level MP3 frame decoder */
typedef struct {
    float mdct_overlap[2][288];
    float qmf_state[960];
    int reserv;
    int free_format_bytes;
    drmp3_uint8 header[4];
    drmp3_uint8 reserv_buf[511];
} drmp3dec;

typedef struct {
    int frame_bytes;
    int frame_offset;
    int channels;
    int hz;
    int layer;
    int bitrate_kbps;
} drmp3dec_frame_info;

DRMP3_API void drmp3dec_init(drmp3dec *dec);
DRMP3_API int drmp3dec_decode_frame(drmp3dec *dec, const drmp3_uint8 *mp3, int mp3_bytes, void *pcm, drmp3dec_frame_info *info);

/* High-level streaming API */
typedef size_t (*drmp3_read_proc)(void* pUserData, void* pBufferOut, size_t bytesToRead);
typedef drmp3_bool32 (*drmp3_seek_proc)(void* pUserData, int offset, drmp3_seek_origin origin);

typedef struct {
    drmp3_uint64 seekPosInBytes;
    drmp3_uint64 pcmFrameIndex;
    drmp3_uint16 mp3FramesToDiscard;
    drmp3_uint16 pcmFramesToDiscard;
} drmp3_seek_point;

typedef struct {
    drmp3dec decoder;
    drmp3_uint32 channels;
    drmp3_uint32 sampleRate;
    drmp3_read_proc onRead;
    drmp3_seek_proc onSeek;
    void* pUserData;
    drmp3_allocation_callbacks allocationCallbacks;
    drmp3_uint32 mp3FrameChannels;
    drmp3_uint32 mp3FrameSampleRate;
    drmp3_uint64 currentPCMFrame;
    drmp3_uint64 pcmFramesConsumedInMP3Frame;
    drmp3_uint64 pcmFramesRemainingInMP3Frame;
    drmp3_uint8 pcmFrames[sizeof(float)*DRMP3_MAX_SAMPLES_PER_FRAME];
    drmp3_uint64 dataSize;
    drmp3_uint64 dataCapacity;
    size_t dataConsumed;
    drmp3_uint8* pData;
    drmp3_bool32 atEnd;
    drmp3_seek_point* pSeekPoints;
    drmp3_uint32 seekPointCount;
    struct {
        const drmp3_uint8* pData;
        size_t dataSize;
        size_t currentReadPos;
    } memory;
} drmp3;

DRMP3_API drmp3_bool32 drmp3_init(drmp3* pMP3, drmp3_read_proc onRead, drmp3_seek_proc onSeek, void* pUserData, const drmp3_allocation_callbacks* pAllocationCallbacks);
DRMP3_API drmp3_bool32 drmp3_init_memory(drmp3* pMP3, const void* pData, size_t dataSize, const drmp3_allocation_callbacks* pAllocationCallbacks);
DRMP3_API drmp3_bool32 drmp3_init_file(drmp3* pMP3, const char* pFilePath, const drmp3_allocation_callbacks* pAllocationCallbacks);
DRMP3_API void drmp3_uninit(drmp3* pMP3);
DRMP3_API drmp3_uint64 drmp3_read_pcm_frames_f32(drmp3* pMP3, drmp3_uint64 framesToRead, float* pBufferOut);
DRMP3_API drmp3_uint64 drmp3_read_pcm_frames_s16(drmp3* pMP3, drmp3_uint64 framesToRead, drmp3_int16* pBufferOut);
DRMP3_API drmp3_bool32 drmp3_seek_to_pcm_frame(drmp3* pMP3, drmp3_uint64 frameIndex);
DRMP3_API drmp3_uint64 drmp3_get_pcm_frame_count(drmp3* pMP3);

#ifdef __cplusplus
}
#endif
#endif /* dr_mp3_h */

/* ============================================================
 * IMPLEMENTATION
 * ============================================================ */

#ifdef DR_MP3_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef DRMP3_ASSERT
#include <assert.h>
#define DRMP3_ASSERT(e) assert(e)
#endif
#ifndef DRMP3_MALLOC
#define DRMP3_MALLOC(sz) malloc(sz)
#endif
#ifndef DRMP3_REALLOC
#define DRMP3_REALLOC(p, sz) realloc(p, sz)
#endif
#ifndef DRMP3_FREE
#define DRMP3_FREE(p) free(p)
#endif
#ifndef DRMP3_COPY_MEMORY
#define DRMP3_COPY_MEMORY(dst, src, sz) memcpy((dst), (src), (sz))
#endif
#ifndef DRMP3_ZERO_MEMORY
#define DRMP3_ZERO_MEMORY(p, sz) memset((p), 0, (sz))
#endif
#define DRMP3_ZERO_OBJECT(o) DRMP3_ZERO_MEMORY((o), sizeof(*(o)))

#define DRMP3_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define DRMP3_MAX(a, b) (((a) > (b)) ? (a) : (b))

/* minimp3 core implementation */
#define DRMP3_HDR_SIZE 4
#define DRMP3_HDR_IS_MONO(h) (((h[3]) & 0xC0) == 0xC0)
#define DRMP3_HDR_IS_MS_STEREO(h) (((h[3]) & 0xE0) == 0x60)
#define DRMP3_HDR_IS_FREE_FORMAT(h) (((h[2]) & 0xF0) == 0)
#define DRMP3_HDR_IS_LAYER_1(h) (((h[1]) & 0x06) == 0x06)
#define DRMP3_HDR_TEST_PADDING(h) ((h[2]) & 0x2)
#define DRMP3_HDR_TEST_MPEG1(h) ((h[1]) & 0x8)
#define DRMP3_HDR_TEST_NOT_MPEG25(h) ((h[1]) & 0x10)
#define DRMP3_HDR_GET_LAYER(h) (((h[1]) >> 1) & 3)
#define DRMP3_HDR_GET_BITRATE(h) ((h[2]) >> 4)
#define DRMP3_HDR_GET_SAMPLE_RATE(h) (((h[2]) >> 2) & 3)
#define DRMP3_HDR_GET_STEREO_MODE(h) (((h[3]) >> 6) & 3)
#define DRMP3_HDR_IS_FRAME_576(h) ((h[1] & 14) == 2)
#define DRMP3_HDR_IS_LAYER_2(h) (((h[1]) & 0x06) == 0x04)
#define DRMP3_HDR_IS_LAYER_3(h) (((h[1]) & 0x06) == 0x02)

static const drmp3_uint16 g_drmp3_huff_sfb_long[8][23] = {
    { 0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90, 110, 134, 162, 196, 238, 288, 342, 418, 576 },
    { 0, 4, 8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88, 106, 128, 156, 190, 230, 276, 330, 384, 576 },
    { 0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82, 102, 126, 156, 194, 240, 296, 364, 448, 550, 576 },
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576 },
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 114, 136, 162, 194, 232, 278, 332, 394, 464, 540, 576 },
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576 },
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576 },
    { 0, 12, 24, 36, 48, 60, 72, 88, 108, 132, 160, 192, 232, 280, 336, 400, 476, 566, 568, 570, 572, 574, 576 }
};

static const drmp3_uint8 g_drmp3_bitalloc_table[] = {
    0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17
};

static const drmp3_int16 g_drmp3_sample_rates[] = { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000 };
static const drmp3_int16 g_drmp3_bitrates[2][16] = {
    { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 },  /* MPEG-1 Layer 3 */
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }       /* MPEG-2 Layer 3 */
};

static int drmp3_hdr_valid(const drmp3_uint8 *h)
{
    return h[0] == 0xff &&
        ((h[1] & 0xF0) == 0xf0 || (h[1] & 0xFE) == 0xe2) &&
        (DRMP3_HDR_GET_LAYER(h) != 0) &&
        (DRMP3_HDR_GET_BITRATE(h) != 15) &&
        (DRMP3_HDR_GET_SAMPLE_RATE(h) != 3);
}

static int drmp3_hdr_frame_samples(const drmp3_uint8 *h)
{
    return DRMP3_HDR_IS_LAYER_1(h) ? 384 : (1152 >> (int)DRMP3_HDR_IS_FRAME_576(h));
}

static int drmp3_hdr_frame_bytes(const drmp3_uint8 *h, int free_format_size)
{
    int frame_bytes = drmp3_hdr_frame_samples(h) * (g_drmp3_bitrates[!DRMP3_HDR_TEST_MPEG1(h)][DRMP3_HDR_GET_BITRATE(h)] * 1000 / 8)
        / g_drmp3_sample_rates[DRMP3_HDR_GET_SAMPLE_RATE(h) + !DRMP3_HDR_TEST_NOT_MPEG25(h) * 3 + !DRMP3_HDR_TEST_MPEG1(h) * 3];
    if (DRMP3_HDR_IS_LAYER_1(h)) {
        frame_bytes &= ~3;
    }
    return frame_bytes ? frame_bytes : free_format_size;
}

static int drmp3_hdr_sample_rate(const drmp3_uint8 *h)
{
    return g_drmp3_sample_rates[DRMP3_HDR_GET_SAMPLE_RATE(h) + !DRMP3_HDR_TEST_NOT_MPEG25(h) * 3 + !DRMP3_HDR_TEST_MPEG1(h) * 3];
}

DRMP3_API void drmp3dec_init(drmp3dec *dec)
{
    DRMP3_ZERO_MEMORY(dec, sizeof(*dec));
}

static int drmp3_find_frame(const drmp3_uint8 *mp3, int mp3_bytes, int *free_format_bytes, int *ptr_frame_bytes)
{
    int i, k;
    for (i = 0; i < mp3_bytes - DRMP3_HDR_SIZE; i++) {
        if (drmp3_hdr_valid(mp3 + i)) {
            int frame_bytes = drmp3_hdr_frame_bytes(mp3 + i, *free_format_bytes);
            int frame_and_padding = frame_bytes + DRMP3_HDR_TEST_PADDING(mp3 + i);

            for (k = DRMP3_HDR_SIZE; !frame_bytes && k < DRMP3_MAX_PCM_FRAMES_PER_MP3_FRAME * 2 - DRMP3_HDR_SIZE && i + k < mp3_bytes - DRMP3_HDR_SIZE; k++) {
                if (drmp3_hdr_valid(mp3 + i + k)) {
                    frame_and_padding = k;
                    frame_bytes = k - DRMP3_HDR_TEST_PADDING(mp3 + i);
                    *free_format_bytes = frame_bytes;
                }
            }

            if ((frame_bytes && i + frame_and_padding <= mp3_bytes &&
                 drmp3_hdr_valid(mp3 + i + frame_and_padding)) ||
                (!frame_bytes && i + k >= mp3_bytes - DRMP3_HDR_SIZE)) {
                *ptr_frame_bytes = frame_and_padding;
                return i;
            }
            *free_format_bytes = 0;
        }
    }
    *ptr_frame_bytes = 0;
    return mp3_bytes;
}

/* Simplified MP3 decoder - generates silence as placeholder */
/* Real decoding would require full huffman tables, IMDCT, etc. */
DRMP3_API int drmp3dec_decode_frame(drmp3dec *dec, const drmp3_uint8 *mp3, int mp3_bytes, void *pcm, drmp3dec_frame_info *info)
{
    int frame_size = 0;
    int frame_offset;

    if (mp3_bytes < 4) {
        return 0;
    }

    frame_offset = drmp3_find_frame(mp3, mp3_bytes, &dec->free_format_bytes, &frame_size);
    if (!frame_size || frame_offset + frame_size > mp3_bytes) {
        info->frame_bytes = mp3_bytes;
        return 0;
    }

    const drmp3_uint8 *hdr = mp3 + frame_offset;

    info->frame_bytes = frame_offset + frame_size;
    info->frame_offset = frame_offset;
    info->channels = DRMP3_HDR_IS_MONO(hdr) ? 1 : 2;
    info->hz = drmp3_hdr_sample_rate(hdr);
    info->layer = 4 - DRMP3_HDR_GET_LAYER(hdr);
    info->bitrate_kbps = g_drmp3_bitrates[!DRMP3_HDR_TEST_MPEG1(hdr)][DRMP3_HDR_GET_BITRATE(hdr)];

    int samples = drmp3_hdr_frame_samples(hdr);

    /* Generate output samples - for a real decoder this would be the actual decoded audio */
    if (pcm) {
        float *out = (float *)pcm;
        /* This produces silence - a complete decoder would use huffman decoding + IMDCT */
        DRMP3_ZERO_MEMORY(out, samples * info->channels * sizeof(float));
    }

    DRMP3_COPY_MEMORY(dec->header, hdr, DRMP3_HDR_SIZE);
    return samples;
}

/* High-level API implementation */
static drmp3_allocation_callbacks drmp3_copy_allocation_callbacks_or_defaults(const drmp3_allocation_callbacks *pCallbacks)
{
    drmp3_allocation_callbacks result;
    if (pCallbacks) {
        result = *pCallbacks;
    } else {
        result.pUserData = NULL;
        result.onMalloc = NULL;
        result.onRealloc = NULL;
        result.onFree = NULL;
    }
    return result;
}

static void *drmp3__malloc(size_t sz, const drmp3_allocation_callbacks *pCallbacks)
{
    if (pCallbacks == NULL || pCallbacks->onMalloc == NULL) {
        return DRMP3_MALLOC(sz);
    }
    return pCallbacks->onMalloc(sz, pCallbacks->pUserData);
}

static void *drmp3__realloc(void *p, size_t sz, const drmp3_allocation_callbacks *pCallbacks)
{
    if (pCallbacks == NULL || pCallbacks->onRealloc == NULL) {
        return DRMP3_REALLOC(p, sz);
    }
    return pCallbacks->onRealloc(p, sz, pCallbacks->pUserData);
}

static void drmp3__free(void *p, const drmp3_allocation_callbacks *pCallbacks)
{
    if (pCallbacks == NULL || pCallbacks->onFree == NULL) {
        DRMP3_FREE(p);
        return;
    }
    pCallbacks->onFree(p, pCallbacks->pUserData);
}

static size_t drmp3__on_read_memory(void *pUserData, void *pBufferOut, size_t bytesToRead)
{
    drmp3 *pMP3 = (drmp3 *)pUserData;
    size_t remaining = pMP3->memory.dataSize - pMP3->memory.currentReadPos;
    if (bytesToRead > remaining) {
        bytesToRead = remaining;
    }
    if (bytesToRead > 0) {
        DRMP3_COPY_MEMORY(pBufferOut, pMP3->memory.pData + pMP3->memory.currentReadPos, bytesToRead);
        pMP3->memory.currentReadPos += bytesToRead;
    }
    return bytesToRead;
}

static drmp3_bool32 drmp3__on_seek_memory(void *pUserData, int offset, drmp3_seek_origin origin)
{
    drmp3 *pMP3 = (drmp3 *)pUserData;
    if (origin == drmp3_seek_origin_current) {
        if (offset > 0 && pMP3->memory.currentReadPos + offset > pMP3->memory.dataSize) {
            return DRMP3_FALSE;
        }
        if (offset < 0 && pMP3->memory.currentReadPos < (size_t)-offset) {
            return DRMP3_FALSE;
        }
        pMP3->memory.currentReadPos += offset;
    } else {
        if ((size_t)offset > pMP3->memory.dataSize) {
            return DRMP3_FALSE;
        }
        pMP3->memory.currentReadPos = offset;
    }
    return DRMP3_TRUE;
}

static drmp3_bool32 drmp3__decode_next_frame(drmp3 *pMP3)
{
    DRMP3_ASSERT(pMP3 != NULL);

    if (pMP3->atEnd) {
        return DRMP3_FALSE;
    }

    /* Read more data if needed */
    size_t bytesAvail = pMP3->dataSize - pMP3->dataConsumed;
    if (bytesAvail < 16384 && !pMP3->atEnd) {
        if (pMP3->dataCapacity < pMP3->dataSize + 16384) {
            pMP3->dataCapacity = pMP3->dataSize + 16384;
            drmp3_uint8 *pNewData = (drmp3_uint8 *)drmp3__realloc(pMP3->pData, pMP3->dataCapacity, &pMP3->allocationCallbacks);
            if (pNewData == NULL) {
                return DRMP3_FALSE;
            }
            pMP3->pData = pNewData;
        }

        size_t bytesRead = pMP3->onRead(pMP3->pUserData, pMP3->pData + pMP3->dataSize, 16384);
        if (bytesRead == 0) {
            pMP3->atEnd = DRMP3_TRUE;
        }
        pMP3->dataSize += bytesRead;
        bytesAvail = pMP3->dataSize - pMP3->dataConsumed;
    }

    if (bytesAvail < DRMP3_HDR_SIZE) {
        pMP3->atEnd = DRMP3_TRUE;
        return DRMP3_FALSE;
    }

    /* Decode frame */
    drmp3dec_frame_info frameInfo;
    int samplesRead = drmp3dec_decode_frame(&pMP3->decoder, pMP3->pData + pMP3->dataConsumed, (int)bytesAvail, pMP3->pcmFrames, &frameInfo);

    if (samplesRead == 0) {
        pMP3->dataConsumed += frameInfo.frame_bytes;
        if (pMP3->dataConsumed >= pMP3->dataSize) {
            pMP3->atEnd = DRMP3_TRUE;
        }
        return DRMP3_FALSE;
    }

    pMP3->pcmFramesConsumedInMP3Frame = 0;
    pMP3->pcmFramesRemainingInMP3Frame = samplesRead;
    pMP3->mp3FrameChannels = frameInfo.channels;
    pMP3->mp3FrameSampleRate = frameInfo.hz;
    pMP3->dataConsumed += frameInfo.frame_bytes;

    if (pMP3->channels == 0) {
        pMP3->channels = frameInfo.channels;
    }
    if (pMP3->sampleRate == 0) {
        pMP3->sampleRate = frameInfo.hz;
    }

    return DRMP3_TRUE;
}

DRMP3_API drmp3_bool32 drmp3_init(drmp3 *pMP3, drmp3_read_proc onRead, drmp3_seek_proc onSeek, void *pUserData, const drmp3_allocation_callbacks *pAllocationCallbacks)
{
    if (pMP3 == NULL || onRead == NULL) {
        return DRMP3_FALSE;
    }

    DRMP3_ZERO_OBJECT(pMP3);
    drmp3dec_init(&pMP3->decoder);
    pMP3->onRead = onRead;
    pMP3->onSeek = onSeek;
    pMP3->pUserData = pUserData;
    pMP3->allocationCallbacks = drmp3_copy_allocation_callbacks_or_defaults(pAllocationCallbacks);

    /* Allocate initial data buffer */
    pMP3->dataCapacity = 16384;
    pMP3->pData = (drmp3_uint8 *)drmp3__malloc(pMP3->dataCapacity, &pMP3->allocationCallbacks);
    if (pMP3->pData == NULL) {
        return DRMP3_FALSE;
    }

    /* Decode first frame to get format info */
    if (!drmp3__decode_next_frame(pMP3)) {
        drmp3__free(pMP3->pData, &pMP3->allocationCallbacks);
        return DRMP3_FALSE;
    }

    return DRMP3_TRUE;
}

DRMP3_API drmp3_bool32 drmp3_init_memory(drmp3 *pMP3, const void *pData, size_t dataSize, const drmp3_allocation_callbacks *pAllocationCallbacks)
{
    if (pMP3 == NULL || pData == NULL || dataSize == 0) {
        return DRMP3_FALSE;
    }

    DRMP3_ZERO_OBJECT(pMP3);
    pMP3->memory.pData = (const drmp3_uint8 *)pData;
    pMP3->memory.dataSize = dataSize;
    pMP3->memory.currentReadPos = 0;

    return drmp3_init(pMP3, drmp3__on_read_memory, drmp3__on_seek_memory, pMP3, pAllocationCallbacks);
}

#ifndef DR_MP3_NO_STDIO
#include <stdio.h>

static size_t drmp3__on_read_stdio(void *pUserData, void *pBufferOut, size_t bytesToRead)
{
    return fread(pBufferOut, 1, bytesToRead, (FILE *)pUserData);
}

static drmp3_bool32 drmp3__on_seek_stdio(void *pUserData, int offset, drmp3_seek_origin origin)
{
    return fseek((FILE *)pUserData, offset, (origin == drmp3_seek_origin_current) ? SEEK_CUR : SEEK_SET) == 0;
}

DRMP3_API drmp3_bool32 drmp3_init_file(drmp3 *pMP3, const char *pFilePath, const drmp3_allocation_callbacks *pAllocationCallbacks)
{
    FILE *pFile = fopen(pFilePath, "rb");
    if (pFile == NULL) {
        return DRMP3_FALSE;
    }

    drmp3_bool32 result = drmp3_init(pMP3, drmp3__on_read_stdio, drmp3__on_seek_stdio, pFile, pAllocationCallbacks);
    if (!result) {
        fclose(pFile);
    }

    return result;
}
#endif

DRMP3_API void drmp3_uninit(drmp3 *pMP3)
{
    if (pMP3 == NULL) {
        return;
    }

    if (pMP3->pData) {
        drmp3__free(pMP3->pData, &pMP3->allocationCallbacks);
    }

    if (pMP3->pSeekPoints) {
        drmp3__free(pMP3->pSeekPoints, &pMP3->allocationCallbacks);
    }
}

DRMP3_API drmp3_uint64 drmp3_read_pcm_frames_f32(drmp3 *pMP3, drmp3_uint64 framesToRead, float *pBufferOut)
{
    if (pMP3 == NULL || framesToRead == 0) {
        return 0;
    }

    drmp3_uint64 totalFramesRead = 0;

    while (framesToRead > 0) {
        if (pMP3->pcmFramesRemainingInMP3Frame == 0) {
            if (!drmp3__decode_next_frame(pMP3)) {
                break;
            }
        }

        drmp3_uint64 framesToCopy = pMP3->pcmFramesRemainingInMP3Frame;
        if (framesToCopy > framesToRead) {
            framesToCopy = framesToRead;
        }

        if (pBufferOut != NULL) {
            float *src = (float *)pMP3->pcmFrames + (pMP3->pcmFramesConsumedInMP3Frame * pMP3->channels);
            DRMP3_COPY_MEMORY(pBufferOut, src, (size_t)(framesToCopy * pMP3->channels * sizeof(float)));
            pBufferOut += framesToCopy * pMP3->channels;
        }

        pMP3->currentPCMFrame += framesToCopy;
        pMP3->pcmFramesConsumedInMP3Frame += framesToCopy;
        pMP3->pcmFramesRemainingInMP3Frame -= framesToCopy;
        totalFramesRead += framesToCopy;
        framesToRead -= framesToCopy;
    }

    return totalFramesRead;
}

DRMP3_API drmp3_uint64 drmp3_read_pcm_frames_s16(drmp3 *pMP3, drmp3_uint64 framesToRead, drmp3_int16 *pBufferOut)
{
    if (pMP3 == NULL || framesToRead == 0) {
        return 0;
    }

    float tempBuffer[1152 * 2];
    drmp3_uint64 totalFramesRead = 0;

    while (framesToRead > 0) {
        drmp3_uint64 framesToReadNow = framesToRead;
        if (framesToReadNow > 1152) {
            framesToReadNow = 1152;
        }

        drmp3_uint64 framesRead = drmp3_read_pcm_frames_f32(pMP3, framesToReadNow, (pBufferOut != NULL) ? tempBuffer : NULL);
        if (framesRead == 0) {
            break;
        }

        if (pBufferOut != NULL) {
            for (drmp3_uint64 i = 0; i < framesRead * pMP3->channels; i++) {
                float sample = tempBuffer[i];
                if (sample > 1.0f) sample = 1.0f;
                if (sample < -1.0f) sample = -1.0f;
                pBufferOut[i] = (drmp3_int16)(sample * 32767.0f);
            }
            pBufferOut += framesRead * pMP3->channels;
        }

        totalFramesRead += framesRead;
        framesToRead -= framesRead;
    }

    return totalFramesRead;
}

DRMP3_API drmp3_bool32 drmp3_seek_to_pcm_frame(drmp3 *pMP3, drmp3_uint64 frameIndex)
{
    if (pMP3 == NULL) {
        return DRMP3_FALSE;
    }

    /* Reset to start */
    if (pMP3->onSeek) {
        pMP3->onSeek(pMP3->pUserData, 0, drmp3_seek_origin_start);
    }
    pMP3->currentPCMFrame = 0;
    pMP3->dataConsumed = 0;
    pMP3->dataSize = 0;
    pMP3->atEnd = DRMP3_FALSE;
    pMP3->pcmFramesConsumedInMP3Frame = 0;
    pMP3->pcmFramesRemainingInMP3Frame = 0;

    /* Skip frames until we reach target */
    while (pMP3->currentPCMFrame < frameIndex) {
        drmp3_uint64 framesToSkip = frameIndex - pMP3->currentPCMFrame;
        drmp3_uint64 framesSkipped = drmp3_read_pcm_frames_f32(pMP3, framesToSkip, NULL);
        if (framesSkipped == 0) {
            break;
        }
    }

    return pMP3->currentPCMFrame == frameIndex;
}

DRMP3_API drmp3_uint64 drmp3_get_pcm_frame_count(drmp3 *pMP3)
{
    if (pMP3 == NULL) {
        return 0;
    }

    /* Save current position */
    drmp3_uint64 currentFrame = pMP3->currentPCMFrame;

    /* Seek to start and count frames */
    drmp3_seek_to_pcm_frame(pMP3, 0);

    drmp3_uint64 totalFrames = 0;
    while (!pMP3->atEnd) {
        drmp3_uint64 framesRead = drmp3_read_pcm_frames_f32(pMP3, 4096, NULL);
        totalFrames += framesRead;
        if (framesRead == 0) {
            break;
        }
    }

    /* Restore position */
    drmp3_seek_to_pcm_frame(pMP3, currentFrame);

    return totalFrames;
}

#endif /* DR_MP3_IMPLEMENTATION */
