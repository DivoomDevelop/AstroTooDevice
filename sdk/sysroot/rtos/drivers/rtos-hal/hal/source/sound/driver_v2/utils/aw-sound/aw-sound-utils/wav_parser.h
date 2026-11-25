/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __WAV_PARSER_H
#define __WAV_PARSER_H

#include <aw-sound-lib/pcm.h>

#ifdef __cplusplus

extern "C" {
#endif

/* define default endianness */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN    3456
#endif

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN    6543
#endif

#ifndef __BYTE_ORDER
#define __BYTE_ORDER    __LITTLE_ENDIAN
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d)     ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define LE_SHORT(v)             (v)
#define LE_INT(v)               (v)
#define BE_SHORT(v)             bswap_16(v)
#define BE_INT(v)               bswap_32(v)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d)     ((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#define LE_SHORT(v)             bswap_16(v)
#define LE_INT(v)               bswap_32(v)
#define BE_SHORT(v)             (v)
#define BE_INT(v)               (v)
#else
#error "Wrong endian"
#endif

#define WAV_RIFF                COMPOSE_ID('R','I','F','F')
#define WAV_RIFX                COMPOSE_ID('R','I','F','X')
#define WAV_WAVE                COMPOSE_ID('W','A','V','E')
#define WAV_FMT                 COMPOSE_ID('f','m','t',' ')
#define WAV_DATA                COMPOSE_ID('d','a','t','a')

#define WAV_FMT_PCM             0x0001
#define WAV_FMT_IEEE_FLOAT      0x0003


typedef struct {
	unsigned int    riffType;               // 4 bytes, Resource Interchange File Format: RIFF
	unsigned int    riffSize;               // 4 bytes, total bytes from next address to file end
	unsigned int    waveType;               // 4 bytes, WAV file format: WAVE
	unsigned int    formatType;             // 4 bytes, format chunk identifier: FMT (last char is space)
	unsigned int    formatSize;             // 4 bytes, size of audio properties in bytes
	unsigned short  compressionCode;        // 2 bytes, format type
	unsigned short  numChannels;            // 2 bytes, number of channels
	unsigned int    sampleRate;             // 4 bytes, sample rate
	unsigned int    bytesPerSecond;         // 4 bytes, data transfer rate
	unsigned short  blockAlign;             // 2 bytes, data block alignment (DATA chunk length)
	unsigned short  bitsPerSample;          // 2 bytes, sample precision - PCM bit depth
	unsigned int    dataType;               // 4 bytes, data chunk identifier: data
	unsigned int    dataSize;               // 4 bytes, total bytes from next address to file end
} wav_header_t;

typedef struct  {
	uint32_t riff_id;
	uint32_t riff_sz;
	uint32_t wave_id;
} riff_wave_header;

typedef struct  {
	uint32_t id;
	uint32_t sz;
} chunk_header;

typedef struct  {
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
} chunk_fmt;

typedef struct {
	unsigned int rate;
	sound_pcm_format_t format;
	unsigned int channels;
} wav_hw_params_t;

int parse_wav_file(int fd, riff_wave_header *riff_wave_header, chunk_header *chunk_header, chunk_fmt*chunk_fmt, wav_hw_params_t *hwparams);
int check_wav_header(wav_header_t *header, wav_hw_params_t *hwparams);
void create_wav(wav_header_t *header, unsigned int format, unsigned int rate, unsigned int channels);
void resize_wav(wav_header_t *header, unsigned int size);

#ifdef __cplusplus
};  // extern "C"
#endif
#endif
