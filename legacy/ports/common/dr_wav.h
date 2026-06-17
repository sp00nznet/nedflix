/*
WAV audio loader and writer. Choice of public domain or MIT-0.

David Reid - mackron@gmail.com

GitHub: https://github.com/mackron/dr_libs

This is a single-file library. To use it, do something like the following in one .c file:
    #define DR_WAV_IMPLEMENTATION
    #include "dr_wav.h"
*/

#ifndef dr_wav_h
#define dr_wav_h

#ifdef __cplusplus
extern "C" {
#endif

#define DRWAV_STRINGIFY(x)      #x
#define DRWAV_XSTRINGIFY(x)     DRWAV_STRINGIFY(x)

#define DRWAV_VERSION_MAJOR     0
#define DRWAV_VERSION_MINOR     13
#define DRWAV_VERSION_REVISION  13
#define DRWAV_VERSION_STRING    DRWAV_XSTRINGIFY(DRWAV_VERSION_MAJOR) "." DRWAV_XSTRINGIFY(DRWAV_VERSION_MINOR) "." DRWAV_XSTRINGIFY(DRWAV_VERSION_REVISION)

#include <stddef.h>

typedef   signed char           drwav_int8;
typedef unsigned char           drwav_uint8;
typedef   signed short          drwav_int16;
typedef unsigned short          drwav_uint16;
typedef   signed int            drwav_int32;
typedef unsigned int            drwav_uint32;
#if defined(_MSC_VER) && !defined(__clang__)
    typedef   signed __int64    drwav_int64;
    typedef unsigned __int64    drwav_uint64;
#else
    #if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wlong-long"
    #endif
    typedef   signed long long  drwav_int64;
    typedef unsigned long long  drwav_uint64;
    #if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
        #pragma GCC diagnostic pop
    #endif
#endif
typedef drwav_uint8             drwav_bool8;
typedef drwav_uint32            drwav_bool32;
#define DRWAV_TRUE              1
#define DRWAV_FALSE             0

typedef void* drwav_handle;
typedef void* drwav_ptr;
typedef void (* drwav_proc)(void);

#ifndef DRWAV_API
    #define DRWAV_API
#endif
#ifndef DRWAV_PRIVATE
    #define DRWAV_PRIVATE static
#endif

typedef drwav_int32 drwav_result;
#define DRWAV_SUCCESS                        0
#define DRWAV_ERROR                         -1
#define DRWAV_INVALID_ARGS                  -2
#define DRWAV_INVALID_OPERATION             -3
#define DRWAV_OUT_OF_MEMORY                 -4
#define DRWAV_OUT_OF_RANGE                  -5
#define DRWAV_ACCESS_DENIED                 -6
#define DRWAV_DOES_NOT_EXIST                -7
#define DRWAV_ALREADY_EXISTS                -8
#define DRWAV_TOO_MANY_OPEN_FILES           -9
#define DRWAV_INVALID_FILE                  -10
#define DRWAV_TOO_BIG                       -11
#define DRWAV_PATH_TOO_LONG                 -12
#define DRWAV_NAME_TOO_LONG                 -13
#define DRWAV_NOT_DIRECTORY                 -14
#define DRWAV_IS_DIRECTORY                  -15
#define DRWAV_DIRECTORY_NOT_EMPTY           -16
#define DRWAV_END_OF_FILE                   -17
#define DRWAV_NO_SPACE                      -18
#define DRWAV_BUSY                          -19
#define DRWAV_IO_ERROR                      -20
#define DRWAV_INTERRUPT                     -21
#define DRWAV_UNAVAILABLE                   -22
#define DRWAV_ALREADY_IN_USE                -23
#define DRWAV_BAD_ADDRESS                   -24
#define DRWAV_BAD_SEEK                      -25
#define DRWAV_BAD_PIPE                      -26
#define DRWAV_DEADLOCK                      -27
#define DRWAV_TOO_MANY_LINKS                -28
#define DRWAV_NOT_IMPLEMENTED               -29
#define DRWAV_NO_MESSAGE                    -30
#define DRWAV_BAD_MESSAGE                   -31
#define DRWAV_NO_DATA_AVAILABLE             -32
#define DRWAV_INVALID_DATA                  -33
#define DRWAV_TIMEOUT                       -34
#define DRWAV_NO_NETWORK                    -35
#define DRWAV_NOT_UNIQUE                    -36
#define DRWAV_NOT_SOCKET                    -37
#define DRWAV_NO_ADDRESS                    -38
#define DRWAV_BAD_PROTOCOL                  -39
#define DRWAV_PROTOCOL_UNAVAILABLE          -40
#define DRWAV_PROTOCOL_NOT_SUPPORTED        -41
#define DRWAV_PROTOCOL_FAMILY_NOT_SUPPORTED -42
#define DRWAV_ADDRESS_FAMILY_NOT_SUPPORTED  -43
#define DRWAV_SOCKET_NOT_SUPPORTED          -44
#define DRWAV_CONNECTION_RESET              -45
#define DRWAV_ALREADY_CONNECTED             -46
#define DRWAV_NOT_CONNECTED                 -47
#define DRWAV_CONNECTION_REFUSED            -48
#define DRWAV_NO_HOST                       -49
#define DRWAV_IN_PROGRESS                   -50
#define DRWAV_CANCELLED                     -51
#define DRWAV_MEMORY_ALREADY_MAPPED         -52
#define DRWAV_AT_END                        -53

#define DR_WAVE_FORMAT_PCM          0x1
#define DR_WAVE_FORMAT_ADPCM        0x2
#define DR_WAVE_FORMAT_IEEE_FLOAT   0x3
#define DR_WAVE_FORMAT_ALAW         0x6
#define DR_WAVE_FORMAT_MULAW        0x7
#define DR_WAVE_FORMAT_DVI_ADPCM    0x11
#define DR_WAVE_FORMAT_EXTENSIBLE   0xFFFE

typedef enum {
    drwav_container_riff,
    drwav_container_w64,
    drwav_container_rf64
} drwav_container;

typedef struct {
    union {
        drwav_uint8 fourcc[4];
        drwav_uint8 guid[16];
    } id;
    drwav_uint64 sizeInBytes;
    unsigned int paddingSize;
} drwav_chunk_header;

typedef struct {
    drwav_uint16 formatTag;
    drwav_uint16 channels;
    drwav_uint32 sampleRate;
    drwav_uint32 avgBytesPerSec;
    drwav_uint16 blockAlign;
    drwav_uint16 bitsPerSample;
    drwav_uint16 extendedSize;
    drwav_uint16 validBitsPerSample;
    drwav_uint32 channelMask;
    drwav_uint8 subFormat[16];
} drwav_fmt;

typedef size_t (* drwav_read_proc)(void* pUserData, void* pBufferOut, size_t bytesToRead);
typedef size_t (* drwav_write_proc)(void* pUserData, const void* pData, size_t bytesToWrite);
typedef drwav_bool32 (* drwav_seek_proc)(void* pUserData, int offset, drwav_seek_origin origin);
typedef drwav_uint64 (* drwav_chunk_proc)(void* pChunkUserData, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pReadSeekUserData, const drwav_chunk_header* pChunkHeader, drwav_container container, const drwav_fmt* pFMT);

typedef enum {
    drwav_seek_origin_start,
    drwav_seek_origin_current
} drwav_seek_origin;

typedef struct {
    void* pUserData;
    void* (* onMalloc)(size_t sz, void* pUserData);
    void* (* onRealloc)(void* p, size_t sz, void* pUserData);
    void  (* onFree)(void* p, void* pUserData);
} drwav_allocation_callbacks;

typedef struct {
    const drwav_uint8* data;
    size_t dataSize;
    size_t currentReadPos;
} drwav__memory_stream;

typedef struct {
    void** ppData;
    size_t* pDataSize;
    size_t dataSize;
    size_t dataCapacity;
    size_t currentWritePos;
} drwav__memory_stream_write;

typedef struct {
    drwav_container container;
    drwav_uint32 format;
    drwav_uint32 channels;
    drwav_uint32 sampleRate;
    drwav_uint32 bitsPerSample;
} drwav_data_format;

typedef struct {
    drwav_read_proc onRead;
    drwav_write_proc onWrite;
    drwav_seek_proc onSeek;
    void* pUserData;
    drwav_allocation_callbacks allocationCallbacks;
    drwav_container container;
    drwav_fmt fmt;
    drwav_uint32 sampleRate;
    drwav_uint16 channels;
    drwav_uint16 bitsPerSample;
    drwav_uint16 translatedFormatTag;
    drwav_uint64 totalPCMFrameCount;
    drwav_uint64 dataChunkDataSize;
    drwav_uint64 dataChunkDataPos;
    drwav_uint64 bytesRemaining;
    drwav_uint64 readCursorInPCMFrames;
    drwav_uint64 dataChunkDataSizeTargetWrite;
    drwav_bool32 isSequentialWrite;
    drwav__memory_stream memoryStream;
    drwav__memory_stream_write memoryStreamWrite;
    struct {
        drwav_uint32 bytesRemainingInBlock;
        drwav_uint16 predictor[2];
        drwav_int32  delta[2];
        drwav_int32  cachedFrames[4];
        drwav_uint32 cachedFrameCount;
        drwav_int32  prevFrames[2][2];
    } msadpcm;
    struct {
        drwav_uint32 bytesRemainingInBlock;
        drwav_int32  predictor[2];
        drwav_int32  stepIndex[2];
        drwav_int32  cachedFrames[16];
        drwav_uint32 cachedFrameCount;
    } ima;
} drwav;

DRWAV_API drwav_bool32 drwav_init(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_ex(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, drwav_chunk_proc onChunk, void* pReadSeekUserData, void* pChunkUserData, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_with_metadata(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_write(drwav* pWav, const drwav_data_format* pFormat, drwav_write_proc onWrite, drwav_seek_proc onSeek, void* pUserData, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_write_sequential(drwav* pWav, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount, drwav_write_proc onWrite, void* pUserData, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_write_sequential_pcm_frames(drwav* pWav, const drwav_data_format* pFormat, drwav_uint64 totalPCMFrameCount, drwav_write_proc onWrite, void* pUserData, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_uint64 drwav_target_write_size_bytes(const drwav_data_format* pFormat, drwav_uint64 totalFrameCount, drwav_metadata* pMetadata, drwav_uint32 metadataCount);
DRWAV_API drwav_result drwav_uninit(drwav* pWav);
DRWAV_API drwav_uint64 drwav_read_pcm_frames(drwav* pWav, drwav_uint64 framesToRead, void* pBufferOut);
DRWAV_API drwav_uint64 drwav_read_pcm_frames_le(drwav* pWav, drwav_uint64 framesToRead, void* pBufferOut);
DRWAV_API drwav_uint64 drwav_read_pcm_frames_be(drwav* pWav, drwav_uint64 framesToRead, void* pBufferOut);
DRWAV_API drwav_bool32 drwav_seek_to_pcm_frame(drwav* pWav, drwav_uint64 targetFrameIndex);
DRWAV_API drwav_result drwav_get_cursor_in_pcm_frames(drwav* pWav, drwav_uint64* pCursor);
DRWAV_API drwav_result drwav_get_length_in_pcm_frames(drwav* pWav, drwav_uint64* pLength);
DRWAV_API drwav_uint64 drwav_write_pcm_frames(drwav* pWav, drwav_uint64 framesToWrite, const void* pData);
DRWAV_API drwav_uint64 drwav_write_pcm_frames_le(drwav* pWav, drwav_uint64 framesToWrite, const void* pData);
DRWAV_API drwav_uint64 drwav_write_pcm_frames_be(drwav* pWav, drwav_uint64 framesToWrite, const void* pData);
DRWAV_API drwav_uint64 drwav_read_pcm_frames_s16(drwav* pWav, drwav_uint64 framesToRead, drwav_int16* pBufferOut);
DRWAV_API drwav_uint64 drwav_read_pcm_frames_s32(drwav* pWav, drwav_uint64 framesToRead, drwav_int32* pBufferOut);
DRWAV_API drwav_uint64 drwav_read_pcm_frames_f32(drwav* pWav, drwav_uint64 framesToRead, float* pBufferOut);
DRWAV_API void drwav_u8_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount);
DRWAV_API void drwav_s24_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount);
DRWAV_API void drwav_s32_to_s16(drwav_int16* pOut, const drwav_int32* pIn, size_t sampleCount);
DRWAV_API void drwav_f32_to_s16(drwav_int16* pOut, const float* pIn, size_t sampleCount);
DRWAV_API void drwav_f64_to_s16(drwav_int16* pOut, const double* pIn, size_t sampleCount);
DRWAV_API void drwav_alaw_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount);
DRWAV_API void drwav_mulaw_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount);

#ifndef DR_WAV_NO_STDIO
DRWAV_API drwav_bool32 drwav_init_file(drwav* pWav, const char* filename, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_ex(drwav* pWav, const char* filename, drwav_chunk_proc onChunk, void* pChunkUserData, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_w(drwav* pWav, const wchar_t* filename, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_ex_w(drwav* pWav, const wchar_t* filename, drwav_chunk_proc onChunk, void* pChunkUserData, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_with_metadata(drwav* pWav, const char* filename, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_with_metadata_w(drwav* pWav, const wchar_t* filename, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_write(drwav* pWav, const char* filename, const drwav_data_format* pFormat, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_write_sequential(drwav* pWav, const char* filename, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_write_sequential_pcm_frames(drwav* pWav, const char* filename, const drwav_data_format* pFormat, drwav_uint64 totalPCMFrameCount, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_write_w(drwav* pWav, const wchar_t* filename, const drwav_data_format* pFormat, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_write_sequential_w(drwav* pWav, const wchar_t* filename, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_file_write_sequential_pcm_frames_w(drwav* pWav, const wchar_t* filename, const drwav_data_format* pFormat, drwav_uint64 totalPCMFrameCount, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_int16* drwav_open_file_and_read_pcm_frames_s16(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_int32* drwav_open_file_and_read_pcm_frames_s32(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API float* drwav_open_file_and_read_pcm_frames_f32(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_int16* drwav_open_file_and_read_pcm_frames_s16_w(const wchar_t* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_int32* drwav_open_file_and_read_pcm_frames_s32_w(const wchar_t* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API float* drwav_open_file_and_read_pcm_frames_f32_w(const wchar_t* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
#endif

DRWAV_API drwav_bool32 drwav_init_memory(drwav* pWav, const void* data, size_t dataSize, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_memory_ex(drwav* pWav, const void* data, size_t dataSize, drwav_chunk_proc onChunk, void* pChunkUserData, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_memory_with_metadata(drwav* pWav, const void* data, size_t dataSize, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_memory_write(drwav* pWav, void** ppData, size_t* pDataSize, const drwav_data_format* pFormat, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_memory_write_sequential(drwav* pWav, void** ppData, size_t* pDataSize, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_bool32 drwav_init_memory_write_sequential_pcm_frames(drwav* pWav, void** ppData, size_t* pDataSize, const drwav_data_format* pFormat, drwav_uint64 totalPCMFrameCount, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_int16* drwav_open_memory_and_read_pcm_frames_s16(const void* data, size_t dataSize, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_int32* drwav_open_memory_and_read_pcm_frames_s32(const void* data, size_t dataSize, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API float* drwav_open_memory_and_read_pcm_frames_f32(const void* data, size_t dataSize, unsigned int* channelsOut, unsigned int* sampleRateOut, drwav_uint64* totalFrameCountOut, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API void drwav_free(void* p, const drwav_allocation_callbacks* pAllocationCallbacks);
DRWAV_API drwav_uint16 drwav_bytes_to_u16(const drwav_uint8* data);
DRWAV_API drwav_int16 drwav_bytes_to_s16(const drwav_uint8* data);
DRWAV_API drwav_uint32 drwav_bytes_to_u32(const drwav_uint8* data);
DRWAV_API drwav_int32 drwav_bytes_to_s32(const drwav_uint8* data);
DRWAV_API drwav_uint64 drwav_bytes_to_u64(const drwav_uint8* data);
DRWAV_API drwav_int64 drwav_bytes_to_s64(const drwav_uint8* data);
DRWAV_API drwav_bool32 drwav_guid_equal(const drwav_uint8 a[16], const drwav_uint8 b[16]);
DRWAV_API drwav_bool32 drwav_fourcc_equal(const drwav_uint8* a, const char* b);

/* Metadata types for extended WAV features */
typedef struct drwav_metadata drwav_metadata;

#ifdef __cplusplus
}
#endif
#endif  /* dr_wav_h */

/* ============================================================
 * IMPLEMENTATION
 * ============================================================ */

#ifdef DR_WAV_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifndef DRWAV_ASSERT
#include <assert.h>
#define DRWAV_ASSERT(expression)           assert(expression)
#endif
#ifndef DRWAV_MALLOC
#define DRWAV_MALLOC(sz)                   malloc((sz))
#endif
#ifndef DRWAV_REALLOC
#define DRWAV_REALLOC(p, sz)               realloc((p), (sz))
#endif
#ifndef DRWAV_FREE
#define DRWAV_FREE(p)                      free((p))
#endif
#ifndef DRWAV_COPY_MEMORY
#define DRWAV_COPY_MEMORY(dst, src, sz)    memcpy((dst), (src), (sz))
#endif
#ifndef DRWAV_ZERO_MEMORY
#define DRWAV_ZERO_MEMORY(p, sz)           memset((p), 0, (sz))
#endif
#ifndef DRWAV_ZERO_OBJECT
#define DRWAV_ZERO_OBJECT(p)               DRWAV_ZERO_MEMORY((p), sizeof(*(p)))
#endif

#define drwav_countof(x)                   (sizeof(x) / sizeof(x[0]))
#define drwav_align(x, a)                  ((((x) + (a) - 1) / (a)) * (a))
#define drwav_min(a, b)                    (((a) < (b)) ? (a) : (b))
#define drwav_max(a, b)                    (((a) > (b)) ? (a) : (b))
#define drwav_clamp(x, lo, hi)             (drwav_max((lo), drwav_min((hi), (x))))
#define drwav_offset_ptr(p, offset)        (((drwav_uint8*)(p)) + (offset))

#define DRWAV_MAX_SIMD_VECTOR_SIZE         64

#define DRWAV_DATA_CHUNK_ID_RIFF           0x46464952
#define DRWAV_DATA_CHUNK_ID_WAVE           0x45564157
#define DRWAV_DATA_CHUNK_ID_FMT            0x20746D66
#define DRWAV_DATA_CHUNK_ID_DATA           0x61746164

DRWAV_PRIVATE drwav_allocation_callbacks drwav_copy_allocation_callbacks_or_defaults(const drwav_allocation_callbacks* pAllocationCallbacks)
{
    drwav_allocation_callbacks result;

    if (pAllocationCallbacks != NULL) {
        result = *pAllocationCallbacks;
    } else {
        result.pUserData = NULL;
        result.onMalloc  = NULL;
        result.onRealloc = NULL;
        result.onFree    = NULL;
    }

    return result;
}

DRWAV_PRIVATE void* drwav__malloc_from_callbacks(size_t sz, const drwav_allocation_callbacks* pAllocationCallbacks)
{
    if (pAllocationCallbacks == NULL || pAllocationCallbacks->onMalloc == NULL) {
        return DRWAV_MALLOC(sz);
    }
    return pAllocationCallbacks->onMalloc(sz, pAllocationCallbacks->pUserData);
}

DRWAV_PRIVATE void* drwav__realloc_from_callbacks(void* p, size_t sz, const drwav_allocation_callbacks* pAllocationCallbacks)
{
    if (pAllocationCallbacks == NULL || pAllocationCallbacks->onRealloc == NULL) {
        return DRWAV_REALLOC(p, sz);
    }
    return pAllocationCallbacks->onRealloc(p, sz, pAllocationCallbacks->pUserData);
}

DRWAV_PRIVATE void drwav__free_from_callbacks(void* p, const drwav_allocation_callbacks* pAllocationCallbacks)
{
    if (pAllocationCallbacks == NULL || pAllocationCallbacks->onFree == NULL) {
        DRWAV_FREE(p);
        return;
    }
    pAllocationCallbacks->onFree(p, pAllocationCallbacks->pUserData);
}

DRWAV_PRIVATE size_t drwav__on_read_memory(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    drwav__memory_stream* pMemory = (drwav__memory_stream*)pUserData;
    size_t bytesRemaining;
    DRWAV_ASSERT(pMemory != NULL);
    bytesRemaining = pMemory->dataSize - pMemory->currentReadPos;
    if (bytesToRead > bytesRemaining) {
        bytesToRead = bytesRemaining;
    }
    if (bytesToRead > 0) {
        DRWAV_COPY_MEMORY(pBufferOut, pMemory->data + pMemory->currentReadPos, bytesToRead);
        pMemory->currentReadPos += bytesToRead;
    }
    return bytesToRead;
}

DRWAV_PRIVATE drwav_bool32 drwav__on_seek_memory(void* pUserData, int offset, drwav_seek_origin origin)
{
    drwav__memory_stream* pMemory = (drwav__memory_stream*)pUserData;
    DRWAV_ASSERT(pMemory != NULL);
    if (origin == drwav_seek_origin_current) {
        if (offset > 0) {
            if (pMemory->currentReadPos + offset > pMemory->dataSize) {
                return DRWAV_FALSE;
            }
        } else {
            if (pMemory->currentReadPos < (size_t)-offset) {
                return DRWAV_FALSE;
            }
        }
        pMemory->currentReadPos += offset;
    } else {
        if ((drwav_uint32)offset <= pMemory->dataSize) {
            pMemory->currentReadPos = offset;
        } else {
            return DRWAV_FALSE;
        }
    }
    return DRWAV_TRUE;
}

DRWAV_PRIVATE drwav_uint32 drwav__read_u32(drwav* pWav)
{
    drwav_uint8 data[4];
    if (pWav->onRead(pWav->pUserData, data, 4) != 4) {
        return 0;
    }
    return drwav_bytes_to_u32(data);
}

DRWAV_PRIVATE drwav_uint16 drwav__read_u16(drwav* pWav)
{
    drwav_uint8 data[2];
    if (pWav->onRead(pWav->pUserData, data, 2) != 2) {
        return 0;
    }
    return drwav_bytes_to_u16(data);
}

DRWAV_API drwav_uint16 drwav_bytes_to_u16(const drwav_uint8* data)
{
    return ((drwav_uint16)data[0] << 0) | ((drwav_uint16)data[1] << 8);
}

DRWAV_API drwav_int16 drwav_bytes_to_s16(const drwav_uint8* data)
{
    return (drwav_int16)drwav_bytes_to_u16(data);
}

DRWAV_API drwav_uint32 drwav_bytes_to_u32(const drwav_uint8* data)
{
    return ((drwav_uint32)data[0] << 0) | ((drwav_uint32)data[1] << 8) |
           ((drwav_uint32)data[2] << 16) | ((drwav_uint32)data[3] << 24);
}

DRWAV_API drwav_int32 drwav_bytes_to_s32(const drwav_uint8* data)
{
    return (drwav_int32)drwav_bytes_to_u32(data);
}

DRWAV_API drwav_uint64 drwav_bytes_to_u64(const drwav_uint8* data)
{
    return ((drwav_uint64)data[0] << 0) | ((drwav_uint64)data[1] << 8) |
           ((drwav_uint64)data[2] << 16) | ((drwav_uint64)data[3] << 24) |
           ((drwav_uint64)data[4] << 32) | ((drwav_uint64)data[5] << 40) |
           ((drwav_uint64)data[6] << 48) | ((drwav_uint64)data[7] << 56);
}

DRWAV_API drwav_int64 drwav_bytes_to_s64(const drwav_uint8* data)
{
    return (drwav_int64)drwav_bytes_to_u64(data);
}

DRWAV_API drwav_bool32 drwav_guid_equal(const drwav_uint8 a[16], const drwav_uint8 b[16])
{
    for (int i = 0; i < 16; i++) {
        if (a[i] != b[i]) return DRWAV_FALSE;
    }
    return DRWAV_TRUE;
}

DRWAV_API drwav_bool32 drwav_fourcc_equal(const drwav_uint8* a, const char* b)
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

DRWAV_PRIVATE drwav_bool32 drwav__read_chunk_header(drwav* pWav, drwav_chunk_header* pHeader)
{
    if (pWav->onRead(pWav->pUserData, pHeader->id.fourcc, 4) != 4) {
        return DRWAV_FALSE;
    }
    drwav_uint8 sizeBytes[4];
    if (pWav->onRead(pWav->pUserData, sizeBytes, 4) != 4) {
        return DRWAV_FALSE;
    }
    pHeader->sizeInBytes = drwav_bytes_to_u32(sizeBytes);
    pHeader->paddingSize = (pHeader->sizeInBytes & 1);
    return DRWAV_TRUE;
}

DRWAV_PRIVATE drwav_bool32 drwav__seek_forward(drwav* pWav, drwav_uint64 offset)
{
    while (offset > 0) {
        int bytesToSeek = (offset > 0x7FFFFFFF) ? 0x7FFFFFFF : (int)offset;
        if (!pWav->onSeek(pWav->pUserData, bytesToSeek, drwav_seek_origin_current)) {
            return DRWAV_FALSE;
        }
        offset -= bytesToSeek;
    }
    return DRWAV_TRUE;
}

DRWAV_API drwav_bool32 drwav_init(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, const drwav_allocation_callbacks* pAllocationCallbacks)
{
    return drwav_init_ex(pWav, onRead, onSeek, NULL, pUserData, NULL, 0, pAllocationCallbacks);
}

DRWAV_API drwav_bool32 drwav_init_ex(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, drwav_chunk_proc onChunk, void* pReadSeekUserData, void* pChunkUserData, drwav_uint32 flags, const drwav_allocation_callbacks* pAllocationCallbacks)
{
    (void)flags;
    (void)onChunk;
    (void)pChunkUserData;

    if (pWav == NULL || onRead == NULL || onSeek == NULL) {
        return DRWAV_FALSE;
    }

    DRWAV_ZERO_OBJECT(pWav);
    pWav->onRead = onRead;
    pWav->onSeek = onSeek;
    pWav->pUserData = pReadSeekUserData;
    pWav->allocationCallbacks = drwav_copy_allocation_callbacks_or_defaults(pAllocationCallbacks);

    /* Read RIFF header */
    drwav_uint8 riff[4];
    if (pWav->onRead(pWav->pUserData, riff, 4) != 4) {
        return DRWAV_FALSE;
    }

    if (drwav_fourcc_equal(riff, "RIFF")) {
        pWav->container = drwav_container_riff;
    } else {
        return DRWAV_FALSE;
    }

    /* File size */
    drwav_uint32 fileSize = drwav__read_u32(pWav);
    (void)fileSize;

    /* WAVE marker */
    drwav_uint8 wave[4];
    if (pWav->onRead(pWav->pUserData, wave, 4) != 4) {
        return DRWAV_FALSE;
    }
    if (!drwav_fourcc_equal(wave, "WAVE")) {
        return DRWAV_FALSE;
    }

    /* Read chunks */
    drwav_bool32 foundFmt = DRWAV_FALSE;
    drwav_bool32 foundData = DRWAV_FALSE;

    while (!foundData) {
        drwav_chunk_header header;
        if (!drwav__read_chunk_header(pWav, &header)) {
            break;
        }

        if (drwav_fourcc_equal(header.id.fourcc, "fmt ")) {
            /* Format chunk */
            pWav->fmt.formatTag = drwav__read_u16(pWav);
            pWav->fmt.channels = drwav__read_u16(pWav);
            pWav->fmt.sampleRate = drwav__read_u32(pWav);
            pWav->fmt.avgBytesPerSec = drwav__read_u32(pWav);
            pWav->fmt.blockAlign = drwav__read_u16(pWav);
            pWav->fmt.bitsPerSample = drwav__read_u16(pWav);

            if (header.sizeInBytes > 16) {
                pWav->fmt.extendedSize = drwav__read_u16(pWav);
                drwav__seek_forward(pWav, header.sizeInBytes - 18 + header.paddingSize);
            }

            pWav->sampleRate = pWav->fmt.sampleRate;
            pWav->channels = pWav->fmt.channels;
            pWav->bitsPerSample = pWav->fmt.bitsPerSample;
            pWav->translatedFormatTag = pWav->fmt.formatTag;

            foundFmt = DRWAV_TRUE;
        }
        else if (drwav_fourcc_equal(header.id.fourcc, "data")) {
            /* Data chunk */
            if (!foundFmt) {
                return DRWAV_FALSE;
            }

            pWav->dataChunkDataSize = header.sizeInBytes;

            drwav_uint32 bytesPerFrame = pWav->channels * (pWav->bitsPerSample / 8);
            if (bytesPerFrame > 0) {
                pWav->totalPCMFrameCount = pWav->dataChunkDataSize / bytesPerFrame;
            }

            pWav->bytesRemaining = pWav->dataChunkDataSize;
            foundData = DRWAV_TRUE;
        }
        else {
            /* Skip unknown chunk */
            drwav__seek_forward(pWav, header.sizeInBytes + header.paddingSize);
        }
    }

    return foundFmt && foundData;
}

DRWAV_API drwav_bool32 drwav_init_memory(drwav* pWav, const void* data, size_t dataSize, const drwav_allocation_callbacks* pAllocationCallbacks)
{
    if (pWav == NULL || data == NULL || dataSize == 0) {
        return DRWAV_FALSE;
    }

    pWav->memoryStream.data = (const drwav_uint8*)data;
    pWav->memoryStream.dataSize = dataSize;
    pWav->memoryStream.currentReadPos = 0;

    return drwav_init(pWav, drwav__on_read_memory, drwav__on_seek_memory, &pWav->memoryStream, pAllocationCallbacks);
}

#ifndef DR_WAV_NO_STDIO
#include <stdio.h>

DRWAV_PRIVATE size_t drwav__on_read_stdio(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    return fread(pBufferOut, 1, bytesToRead, (FILE*)pUserData);
}

DRWAV_PRIVATE drwav_bool32 drwav__on_seek_stdio(void* pUserData, int offset, drwav_seek_origin origin)
{
    return fseek((FILE*)pUserData, offset, (origin == drwav_seek_origin_current) ? SEEK_CUR : SEEK_SET) == 0;
}

DRWAV_API drwav_bool32 drwav_init_file(drwav* pWav, const char* filename, const drwav_allocation_callbacks* pAllocationCallbacks)
{
    FILE* pFile = fopen(filename, "rb");
    if (pFile == NULL) {
        return DRWAV_FALSE;
    }

    drwav_bool32 result = drwav_init(pWav, drwav__on_read_stdio, drwav__on_seek_stdio, pFile, pAllocationCallbacks);
    if (!result) {
        fclose(pFile);
        return DRWAV_FALSE;
    }

    return DRWAV_TRUE;
}
#endif

DRWAV_API drwav_result drwav_uninit(drwav* pWav)
{
    if (pWav == NULL) {
        return DRWAV_INVALID_ARGS;
    }
    return DRWAV_SUCCESS;
}

DRWAV_API drwav_uint64 drwav_read_pcm_frames(drwav* pWav, drwav_uint64 framesToRead, void* pBufferOut)
{
    if (pWav == NULL || framesToRead == 0) {
        return 0;
    }

    drwav_uint32 bytesPerFrame = pWav->channels * (pWav->bitsPerSample / 8);
    drwav_uint64 bytesToRead = framesToRead * bytesPerFrame;

    if (bytesToRead > pWav->bytesRemaining) {
        bytesToRead = pWav->bytesRemaining;
    }

    size_t bytesRead = pWav->onRead(pWav->pUserData, pBufferOut, (size_t)bytesToRead);
    pWav->bytesRemaining -= bytesRead;
    pWav->readCursorInPCMFrames += bytesRead / bytesPerFrame;

    return bytesRead / bytesPerFrame;
}

DRWAV_API drwav_uint64 drwav_read_pcm_frames_s16(drwav* pWav, drwav_uint64 framesToRead, drwav_int16* pBufferOut)
{
    if (pWav == NULL || framesToRead == 0 || pBufferOut == NULL) {
        return 0;
    }

    /* For 16-bit PCM, read directly */
    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM && pWav->bitsPerSample == 16) {
        return drwav_read_pcm_frames(pWav, framesToRead, pBufferOut);
    }

    /* For other formats, convert */
    drwav_uint64 totalFramesRead = 0;
    drwav_uint32 bytesPerFrame = pWav->channels * (pWav->bitsPerSample / 8);
    drwav_uint8 tempBuffer[4096];

    while (framesToRead > 0) {
        drwav_uint64 framesToReadThisIteration = framesToRead;
        if (framesToReadThisIteration * bytesPerFrame > sizeof(tempBuffer)) {
            framesToReadThisIteration = sizeof(tempBuffer) / bytesPerFrame;
        }

        drwav_uint64 framesRead = drwav_read_pcm_frames(pWav, framesToReadThisIteration, tempBuffer);
        if (framesRead == 0) {
            break;
        }

        /* Convert to s16 */
        drwav_uint64 sampleCount = framesRead * pWav->channels;
        if (pWav->bitsPerSample == 8) {
            drwav_u8_to_s16(pBufferOut, tempBuffer, (size_t)sampleCount);
        } else if (pWav->bitsPerSample == 24) {
            drwav_s24_to_s16(pBufferOut, tempBuffer, (size_t)sampleCount);
        } else if (pWav->bitsPerSample == 32) {
            if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT) {
                drwav_f32_to_s16(pBufferOut, (const float*)tempBuffer, (size_t)sampleCount);
            } else {
                drwav_s32_to_s16(pBufferOut, (const drwav_int32*)tempBuffer, (size_t)sampleCount);
            }
        }

        pBufferOut += framesRead * pWav->channels;
        totalFramesRead += framesRead;
        framesToRead -= framesRead;
    }

    return totalFramesRead;
}

DRWAV_API drwav_bool32 drwav_seek_to_pcm_frame(drwav* pWav, drwav_uint64 targetFrameIndex)
{
    if (pWav == NULL) {
        return DRWAV_FALSE;
    }

    if (targetFrameIndex > pWav->totalPCMFrameCount) {
        targetFrameIndex = pWav->totalPCMFrameCount;
    }

    drwav_uint32 bytesPerFrame = pWav->channels * (pWav->bitsPerSample / 8);
    drwav_uint64 targetByteOffset = targetFrameIndex * bytesPerFrame;

    /* Seek from start of data chunk */
    if (!pWav->onSeek(pWav->pUserData, 0, drwav_seek_origin_start)) {
        return DRWAV_FALSE;
    }

    /* Re-read header to get to data position */
    /* Skip to data position (12 bytes RIFF header + fmt chunk + data header) */
    drwav_uint64 currentPos = 0;
    drwav_uint64 targetPos = pWav->dataChunkDataPos + targetByteOffset;

    while (currentPos < targetPos) {
        drwav_uint64 remaining = targetPos - currentPos;
        int seekAmount = (remaining > 0x7FFFFFFF) ? 0x7FFFFFFF : (int)remaining;
        if (!pWav->onSeek(pWav->pUserData, seekAmount, drwav_seek_origin_current)) {
            return DRWAV_FALSE;
        }
        currentPos += seekAmount;
    }

    pWav->readCursorInPCMFrames = targetFrameIndex;
    pWav->bytesRemaining = pWav->dataChunkDataSize - targetByteOffset;

    return DRWAV_TRUE;
}

DRWAV_API void drwav_u8_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount)
{
    for (size_t i = 0; i < sampleCount; i++) {
        pOut[i] = (drwav_int16)(((int)pIn[i] - 128) << 8);
    }
}

DRWAV_API void drwav_s24_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount)
{
    for (size_t i = 0; i < sampleCount; i++) {
        drwav_int32 sample = ((drwav_int32)pIn[i*3+2] << 24) | ((drwav_int32)pIn[i*3+1] << 16) | ((drwav_int32)pIn[i*3+0] << 8);
        pOut[i] = (drwav_int16)(sample >> 16);
    }
}

DRWAV_API void drwav_s32_to_s16(drwav_int16* pOut, const drwav_int32* pIn, size_t sampleCount)
{
    for (size_t i = 0; i < sampleCount; i++) {
        pOut[i] = (drwav_int16)(pIn[i] >> 16);
    }
}

DRWAV_API void drwav_f32_to_s16(drwav_int16* pOut, const float* pIn, size_t sampleCount)
{
    for (size_t i = 0; i < sampleCount; i++) {
        float sample = pIn[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        pOut[i] = (drwav_int16)(sample * 32767.0f);
    }
}

DRWAV_API void drwav_f64_to_s16(drwav_int16* pOut, const double* pIn, size_t sampleCount)
{
    for (size_t i = 0; i < sampleCount; i++) {
        double sample = pIn[i];
        if (sample > 1.0) sample = 1.0;
        if (sample < -1.0) sample = -1.0;
        pOut[i] = (drwav_int16)(sample * 32767.0);
    }
}

DRWAV_API void drwav_free(void* p, const drwav_allocation_callbacks* pAllocationCallbacks)
{
    drwav__free_from_callbacks(p, pAllocationCallbacks);
}

#endif /* DR_WAV_IMPLEMENTATION */
