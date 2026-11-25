/* miniz.c 3.1.0 - public domain deflate/inflate, zlib-subset
   See "unlicense" statement at the end of this file.
   Rich Geldreich <richgel99@gmail.com>, last updated Oct. 13, 2013
   Implements RFC 1950: http://www.ietf.org/rfc/rfc1950.txt and RFC 1951: http://www.ietf.org/rfc/rfc1951.txt
*/
#pragma once

// #include "miniz_export.h"

#if defined(__STRICT_ANSI__)
#define MZ_FORCEINLINE
#elif defined(_MSC_VER)
#define MZ_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#define MZ_FORCEINLINE __inline__ __attribute__((__always_inline__))
#else
#define MZ_FORCEINLINE inline
#endif

/* Defines to completely disable specific portions of miniz.c:
   If all macros here are defined the only functionality remaining will be CRC-32 and adler-32. */

/* Define MINIZ_NO_STDIO to disable all usage and any functions which rely on stdio for file I/O. */
/*#define MINIZ_NO_STDIO */

/* Define MINIZ_NO_INFLATE_APIS to disable all decompression API's. */
/*#define MINIZ_NO_INFLATE_APIS */

/* Define MINIZ_NO_ZLIB_APIS to remove all ZLIB-style compression/decompression API's. */
/*#define MINIZ_NO_ZLIB_APIS */

/* Define MINIZ_NO_MALLOC to disable all calls to malloc, free, and realloc.
   Note if MINIZ_NO_MALLOC is defined then the user must always provide custom user alloc/free/realloc
   callbacks to the zlib and archive API's, and a few stand-alone helper API's which don't provide custom user
   functions (such as tdefl_compress_mem_to_heap() and tinfl_decompress_mem_to_heap()) won't work. */
/*#define MINIZ_NO_MALLOC */

#ifdef MINIZ_NO_INFLATE_APIS
#define MINIZ_NO_ARCHIVE_APIS
#endif

#include <stddef.h>

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__i386) || defined(__i486__) || defined(__i486) || defined(i386) || defined(__ia64__) || defined(__x86_64__)
/* MINIZ_X86_OR_X64_CPU is only used to help set the below macros. */
#define MINIZ_X86_OR_X64_CPU 1
#else
#define MINIZ_X86_OR_X64_CPU 0
#endif

/* Set MINIZ_LITTLE_ENDIAN only if not set */
#if !defined(MINIZ_LITTLE_ENDIAN)
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
/* Set MINIZ_LITTLE_ENDIAN to 1 if the processor is little endian. */
#define MINIZ_LITTLE_ENDIAN 1
#else
#define MINIZ_LITTLE_ENDIAN 0
#endif

#else

#if MINIZ_X86_OR_X64_CPU
#define MINIZ_LITTLE_ENDIAN 1
#else
#define MINIZ_LITTLE_ENDIAN 0
#endif

#endif
#endif

/* Using unaligned loads and stores causes errors when using UBSan */
#if defined(__has_feature)
#if __has_feature(undefined_behavior_sanitizer)
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#endif
#endif

/* Set MINIZ_USE_UNALIGNED_LOADS_AND_STORES only if not set */
#if !defined(MINIZ_USE_UNALIGNED_LOADS_AND_STORES)
#if MINIZ_X86_OR_X64_CPU
/* Set MINIZ_USE_UNALIGNED_LOADS_AND_STORES to 1 on CPU's that permit efficient integer loads and stores from unaligned addresses. */
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#define MINIZ_UNALIGNED_USE_MEMCPY
#else
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#endif
#endif

#if defined(_M_X64) || defined(_WIN64) || defined(__MINGW64__) || defined(_LP64) || defined(__LP64__) || defined(__ia64__) || defined(__x86_64__)
/* Set MINIZ_HAS_64BIT_REGISTERS to 1 if operations on 64-bit integers are reasonably fast (and don't involve compiler generated calls to helper functions). */
#define MINIZ_HAS_64BIT_REGISTERS 1
#else
#define MINIZ_HAS_64BIT_REGISTERS 0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /* ------------------- zlib-style API Definitions. */

    /* For more compatibility with zlib, miniz.c uses unsigned long for some parameters/struct members. Beware: mz_ulong can be either 32 or 64-bits! */
    typedef unsigned long mz_ulong;

    /* mz_free() internally uses the MZ_FREE() macro (which by default calls free() unless you've modified the MZ_MALLOC macro) to release a block allocated from the heap. */
     void mz_free(void *p);

    /* Heap allocation callbacks.
    Note that mz_alloc_func parameter types purposely differ from zlib's: items/size is size_t, not unsigned long. */
    typedef void *(*mz_alloc_func)(void *opaque, size_t items, size_t size);
    typedef void (*mz_free_func)(void *opaque, void *address);

#ifndef MINIZ_NO_ZLIB_APIS

    /* Flush values. For typical usage you only need MZ_NO_FLUSH and MZ_FINISH. The other values are for advanced use (refer to the zlib docs). */
    enum
    {
        MZ_NO_FLUSH = 0,
        MZ_PARTIAL_FLUSH = 1,
        MZ_SYNC_FLUSH = 2,
        MZ_FULL_FLUSH = 3,
        MZ_FINISH = 4,
        MZ_BLOCK = 5
    };

    /* Return status codes. MZ_PARAM_ERROR is non-standard. */
    enum
    {
        MZ_OK = 0,
        MZ_STREAM_END = 1,
        MZ_NEED_DICT = 2,
        MZ_ERRNO = -1,
        MZ_STREAM_ERROR = -2,
        MZ_DATA_ERROR = -3,
        MZ_MEM_ERROR = -4,
        MZ_BUF_ERROR = -5,
        MZ_VERSION_ERROR = -6,
        MZ_PARAM_ERROR = -10000
    };

/* Window bits */
#define MZ_DEFAULT_WINDOW_BITS 15

    struct mz_internal_state;

    /* Compression/decompression stream struct. */
    typedef struct mz_stream_s
    {
        const unsigned char *next_in; /* pointer to next byte to read */
        unsigned int avail_in;        /* number of bytes available at next_in */
        mz_ulong total_in;            /* total number of bytes consumed so far */

        unsigned char *next_out; /* pointer to next byte to write */
        unsigned int avail_out;  /* number of bytes that can be written to next_out */
        mz_ulong total_out;      /* total number of bytes produced so far */

        char *msg;                       /* error msg (unused) */
        struct mz_internal_state *state; /* internal state, allocated by zalloc/zfree */

        mz_alloc_func zalloc; /* optional heap allocation function (defaults to malloc) */
        mz_free_func zfree;   /* optional heap free function (defaults to free) */
        void *opaque;         /* heap alloc function user pointer */

        int data_type;     /* data_type (unused) */
        mz_ulong adler;    /* adler32 of the source or uncompressed data */
        mz_ulong reserved; /* not used */
    } mz_stream;

    typedef mz_stream *mz_streamp;

#ifndef MINIZ_NO_INFLATE_APIS

    /* Initializes a decompressor. */
      int mz_inflateInit(mz_streamp pStream);

    /* mz_inflateInit2() is like mz_inflateInit() with an additional option that controls the window size and whether or not the stream has been wrapped with a zlib header/footer: */
    /* window_bits must be MZ_DEFAULT_WINDOW_BITS (to parse zlib header/footer) or -MZ_DEFAULT_WINDOW_BITS (raw deflate). */
      int mz_inflateInit2(mz_streamp pStream, int window_bits);

    /* Quickly resets a compressor without having to reallocate anything. Same as calling mz_inflateEnd() followed by mz_inflateInit()/mz_inflateInit2(). */
      int mz_inflateReset(mz_streamp pStream);

    /* Decompresses the input stream to the output, consuming only as much of the input as needed, and writing as much to the output as possible. */
    /* Parameters: */
    /*   pStream is the stream to read from and write to. You must initialize/update the next_in, avail_in, next_out, and avail_out members. */
    /*   flush may be MZ_NO_FLUSH, MZ_SYNC_FLUSH, or MZ_FINISH. */
    /*   On the first call, if flush is MZ_FINISH it's assumed the input and output buffers are both sized large enough to decompress the entire stream in a single call (this is slightly faster). */
    /*   MZ_FINISH implies that there are no more source bytes available beside what's already in the input buffer, and that the output buffer is large enough to hold the rest of the decompressed data. */
    /* Return values: */
    /*   MZ_OK on success. Either more input is needed but not available, and/or there's more output to be written but the output buffer is full. */
    /*   MZ_STREAM_END if all needed input has been consumed and all output bytes have been written. For zlib streams, the adler-32 of the decompressed data has also been verified. */
    /*   MZ_STREAM_ERROR if the stream is bogus. */
    /*   MZ_DATA_ERROR if the deflate stream is invalid. */
    /*   MZ_PARAM_ERROR if one of the parameters is invalid. */
    /*   MZ_BUF_ERROR if no forward progress is possible because the input buffer is empty but the inflater needs more input to continue, or if the output buffer is not large enough. Call mz_inflate() again */
    /*   with more input data, or with more room in the output buffer (except when using single call decompression, described above). */
      int mz_inflate(mz_streamp pStream, int flush);

    /* Deinitializes a decompressor. */
      int mz_inflateEnd(mz_streamp pStream);

    /* Single-call decompression. */
    /* Returns MZ_OK on success, or one of the error codes from mz_inflate() on failure. */
      int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
      int mz_uncompress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong *pSource_len);
#endif /*#ifndef MINIZ_NO_INFLATE_APIS*/

/* Redefine zlib-compatible names to miniz equivalents, so miniz.c can be used as a drop-in replacement for the subset of zlib that miniz.c supports. */
/* Define MINIZ_NO_ZLIB_COMPATIBLE_NAMES to disable zlib-compatibility if you use zlib in the same project. */
#ifndef MINIZ_NO_ZLIB_COMPATIBLE_NAMES
    typedef unsigned char Byte;
    typedef unsigned int uInt;
    typedef mz_ulong uLong;
    typedef Byte Bytef;
    typedef uInt uIntf;
    typedef char charf;
    typedef int intf;
    typedef void *voidpf;
    typedef uLong uLongf;
    typedef void *voidp;
    typedef void *const voidpc;
#define Z_NULL 0
#define Z_NO_FLUSH MZ_NO_FLUSH
#define Z_PARTIAL_FLUSH MZ_PARTIAL_FLUSH
#define Z_SYNC_FLUSH MZ_SYNC_FLUSH
#define Z_FULL_FLUSH MZ_FULL_FLUSH
#define Z_FINISH MZ_FINISH
#define Z_BLOCK MZ_BLOCK
#define Z_OK MZ_OK
#define Z_STREAM_END MZ_STREAM_END
#define Z_NEED_DICT MZ_NEED_DICT
#define Z_ERRNO MZ_ERRNO
#define Z_STREAM_ERROR MZ_STREAM_ERROR
#define Z_DATA_ERROR MZ_DATA_ERROR
#define Z_MEM_ERROR MZ_MEM_ERROR
#define Z_BUF_ERROR MZ_BUF_ERROR
#define Z_VERSION_ERROR MZ_VERSION_ERROR
#define Z_PARAM_ERROR MZ_PARAM_ERROR
#define Z_DEFAULT_WINDOW_BITS MZ_DEFAULT_WINDOW_BITS
    /* See mz_alloc_func */
    typedef void *(*alloc_func)(void *opaque, size_t items, size_t size);
    /* See mz_free_func */
    typedef void (*free_func)(void *opaque, void *address);

#define internal_state mz_internal_state
#define z_stream mz_stream

#ifndef MINIZ_NO_INFLATE_APIS
    /* Compatiblity with zlib API. See called functions for documentation */
    static MZ_FORCEINLINE int inflateInit(mz_streamp pStream)
    {
        return mz_inflateInit(pStream);
    }

    static MZ_FORCEINLINE int inflateInit2(mz_streamp pStream, int window_bits)
    {
        return mz_inflateInit2(pStream, window_bits);
    }

    static MZ_FORCEINLINE int inflateReset(mz_streamp pStream)
    {
        return mz_inflateReset(pStream);
    }

    static MZ_FORCEINLINE int inflate(mz_streamp pStream, int flush)
    {
        return mz_inflate(pStream, flush);
    }

    static MZ_FORCEINLINE int inflateEnd(mz_streamp pStream)
    {
        return mz_inflateEnd(pStream);
    }

    static MZ_FORCEINLINE int uncompress(unsigned char* pDest, mz_ulong* pDest_len, const unsigned char* pSource, mz_ulong source_len)
    {
        return mz_uncompress(pDest, pDest_len, pSource, source_len);
    }

    static MZ_FORCEINLINE int uncompress2(unsigned char* pDest, mz_ulong* pDest_len, const unsigned char* pSource, mz_ulong* pSource_len)
    {
        return mz_uncompress2(pDest, pDest_len, pSource, pSource_len);
    }
#endif /*#ifndef MINIZ_NO_INFLATE_APIS*/

#define MAX_WBITS 15
#endif /* #ifndef MINIZ_NO_ZLIB_COMPATIBLE_NAMES */

#endif /* MINIZ_NO_ZLIB_APIS */

#ifdef __cplusplus
}
#endif

#include "miniz_common.h"
#include "miniz_tinfl.h"
