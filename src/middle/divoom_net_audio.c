/* MPP network demux/ADEC -> existing 48-kHz audio_sink. A persistent worker
 * owns the parser/decoder. Stop cancels I/O and waits; it never frees objects
 * owned by another thread. Existing tone/sleep workers stay unchanged. */
#include "divoom.h"
#include "divoom_build_config.h"
#include "divoom_net_audio.h"
#include "divoom_net_mp3.h"
#include "divoom_net_aac.h"
#include "divoom_net_hls.h"
#include "divoom_audio_sink.h"
#include "divoom_audio_buf_pool.h"
#include "divoom_audio_policy.h"
#include <CdxParser.h>
#include <CdxTime.h>
#include <adecoder.h>
#include <speex_resampler.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define NET_AUDIO_URL_MAX 1024
#define NET_AUDIO_BS_BYTES (128 * 1024)
#define NET_AUDIO_PCM_BYTES (128 * 1024)
#define NET_AUDIO_BLOCK_FRAMES 512
#define NET_AUDIO_RETRIES 3
extern void AwStreamInit(void);
extern void AwParserInit(void);

/* 调试快照只有worker写、串口读；32位原子字段避免统计锁进入音频热路径。 */
typedef struct {
    uint32_t start_ms;
    uint32_t phase_ms;
    uint32_t phase;
    uint32_t packets;
    uint32_t input_bytes;
    uint32_t frames;
    uint32_t read_max_ms;
    uint32_t decode_sink_max_ms;
    uint32_t pcm_gap_max_ms;
    uint32_t last_pcm_ms;
    uint32_t retries;
    uint32_t read_errors;
    uint32 compressed_bytes, compressed_capacity, compressed_target, prefill_ms;
    uint32 hls_segments;
    uint32 worker_prio, producer_prio;
    uint32 read_ms, work_ms, wait_ms; /* 累计墙钟耗时，work含背压，扣wait后再评估解码。 */

} divoom_net_audio_diag_t;

typedef struct {
    /* API lock precedes source lock. Condition waits release source lock.
     * No callback runs under either lock. */
    pthread_mutex_t api_lock, source_lock;
    pthread_cond_t wake, idle;
    pthread_t worker;
    int started, requested, active;
    cdx_bool cancel;
    cdx_bool hls_cancel; /* 独立取消HLS生产者，错误退出不冒充用户STOP。 */
    unsigned int volume;
    int state;
    char url[NET_AUDIO_URL_MAX];
    CdxParserT *parser;
    CdxStreamT *stream;
    divoom_net_audio_callback callback;
    void *user;
    divoom_net_audio_diag_t diag;
} net_audio_service_t;
static net_audio_service_t net_audio = {
    .api_lock = PTHREAD_MUTEX_INITIALIZER,
    .source_lock = PTHREAD_MUTEX_INITIALIZER,
    .wake = PTHREAD_COND_INITIALIZER, .idle = PTHREAD_COND_INITIALIZER
};
typedef struct {
    AudioDecoder *decoder;
    divoom_net_mp3_t *mp3;
    divoom_net_aac *aac;
    unsigned char *packet_data;
    int packet_capacity;
    AudioStreamInfo format;
    BsInFor info;
    SpeexResamplerState *resampler;
    unsigned int rate, channels;
    int sink_open;
    uint32 cache_sample_ms; /* worker内每500ms采样，不依赖某次读调用是否跨越整点。 */
    uint64_t frames;
    int16_t output[NET_AUDIO_BLOCK_FRAMES * 2];
} net_audio_decode_t;

static uint32_t divoom_net_audio_diag_clock_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint32_t)((uint64_t)now.tv_sec * 1000u + now.tv_nsec / 1000000u);
}

static void divoom_net_audio_diag_max(uint32_t *value, uint32_t sample)
{
    if (sample > __atomic_load_n(value, __ATOMIC_RELAXED))
    {
        __atomic_store_n(value, sample, __ATOMIC_RELAXED);
    }
}

static void divoom_net_audio_diag_phase(uint32_t phase)
{
    __atomic_store_n(&net_audio.diag.phase_ms, divoom_net_audio_diag_clock_ms(), __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.phase, phase, __ATOMIC_RELEASE);
}

static void divoom_net_audio_diag_reset(void)
{
    __atomic_store_n(&net_audio.diag.start_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.phase_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.phase, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.packets, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.hls_segments, 0, __ATOMIC_RELAXED);
    struct sched_param scheduling = {0};
    int policy = 0;
    pthread_getschedparam(pthread_self(), &policy, &scheduling);
    __atomic_store_n(&net_audio.diag.worker_prio, scheduling.sched_priority, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.producer_prio, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.input_bytes, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.frames, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.read_max_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.decode_sink_max_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.pcm_gap_max_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.last_pcm_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.retries, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.read_errors, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.read_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.work_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.wait_ms, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.compressed_bytes, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.compressed_capacity, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.compressed_target, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.prefill_ms, 0, __ATOMIC_RELAXED);

    __atomic_store_n(&net_audio.diag.start_ms, divoom_net_audio_diag_clock_ms(), __ATOMIC_RELEASE);
}

static int cancelled(void)
{ return __atomic_load_n(&net_audio.cancel, __ATOMIC_ACQUIRE); }
static void set_state(int state)
{ __atomic_store_n(&net_audio.state, state, __ATOMIC_RELEASE); }
static int wait_cancel(unsigned int ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
    pthread_mutex_lock(&net_audio.source_lock);
    if (!cancelled()) pthread_cond_timedwait(&net_audio.wake, &net_audio.source_lock, &ts);
    pthread_mutex_unlock(&net_audio.source_lock);
    return cancelled() ? -1 : 0;
}
static void close_source(void)
{
    CdxParserT *parser;
    CdxStreamT *stream;
    pthread_mutex_lock(&net_audio.source_lock);
    parser = net_audio.parser; stream = net_audio.stream;
    net_audio.parser = NULL; net_audio.stream = NULL;
    pthread_mutex_unlock(&net_audio.source_lock);
    /* A created parser owns its input stream. */
    if (parser) CdxParserClose(parser);
    else if (stream) CdxStreamClose(stream);
}
static int open_sink(net_audio_decode_t *d)
{
    if (d->sink_open) return 0;
    if (divoom_audio_buf_pool_acquire(DIVOOM_AUDIO_BUS_NETWORK, "network") != 0) return -1;
    if (divoom_audio_sink_start() != 0) {
        divoom_audio_buf_pool_release(DIVOOM_AUDIO_BUS_NETWORK, "network"); return -1;
    }
    divoom_audio_sink_ch_enable(DIVOOM_AUDIO_SINK_CH_NETWORK, 1);
    if (!divoom_audio_sink_ch_ring_bound(DIVOOM_AUDIO_SINK_CH_NETWORK)) {
        divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_NETWORK); return -1;
    }
    d->sink_open = 1;
    divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_NETWORK,
        __atomic_load_n(&net_audio.volume, __ATOMIC_ACQUIRE));
    divoom_audio_sink_ch_fade_in(DIVOOM_AUDIO_SINK_CH_NETWORK, 50, 0);
    return 0;
}
static int push_pcm(net_audio_decode_t *d, unsigned int frames)
{
    unsigned int done = 0;
    if (open_sink(d) != 0) return -1;
    while (done < frames) {
        int n;
        if (cancelled()) return -1;
        /* NETWORK只有本worker写ring，混音线程只消费：有一帧空间后再push。
         * 现有sink在ring满时也返回-1（并非接口注释的0），不能把正常背压当成断网重连。
         * 不绑定ring属于真错误；等待仍使用同一取消条件，STOP/切台可以立即结束。 */
        if (divoom_audio_sink_ch_ring_blank_bytes(DIVOOM_AUDIO_SINK_CH_NETWORK) < 2 * sizeof(int16_t))
        {
            if (!divoom_audio_sink_ch_ring_bound(DIVOOM_AUDIO_SINK_CH_NETWORK))
            {
                return -1;
            }
            uint32 wait_begin = divoom_net_audio_diag_clock_ms();
            int32 wait_result = wait_cancel(10);
            __atomic_fetch_add(&net_audio.diag.wait_ms, divoom_net_audio_diag_clock_ms() - wait_begin, __ATOMIC_RELAXED);
            if (wait_result != 0)
            {
                return -1;
            }
            continue;
        }
        n = divoom_audio_sink_push(DIVOOM_AUDIO_SINK_CH_NETWORK, d->output + done * 2, frames - done);
        if (n < 0) return -1;
        if (n == 0) { if (wait_cancel(10) != 0) return -1; continue; }
        done += (unsigned int)n; d->frames += (unsigned int)n;
    }
    /* 真正越过预缓冲门限后再报告PLAYING，起播等待不能伪装成已经出声。 */
    if (!divoom_audio_sink_ch_buffering(DIVOOM_AUDIO_SINK_CH_NETWORK))
    {
        set_state(DIVOOM_NET_AUDIO_PLAYING);
#if defined(DIVOOM_PRODUCT_ASTRO_TOO)
        /* 混音器将在消费时应用音量。FFT 复制的是此处尚未缩放的 PCM。 */
        divoom_eq_noise_mic_fft_feed_radio_pcm(d->output, frames);
#endif
    }
    return 0;
}
static int convert_pcm(net_audio_decode_t *d, const int16_t *pcm, unsigned int bytes)
{
    unsigned int rate = d->format.nSampleRate, channels = d->format.nChannelNum, remain;
    if (rate < 8000 || rate > 192000 || channels < 1 || channels > 2 ||
        (d->format.nBitsPerSample != 0 && d->format.nBitsPerSample != 16) ||
        bytes % (channels * sizeof(int16_t))) return -1;
    if (d->rate != rate || d->channels != channels) {
        int err = 0;
        if (d->resampler) speex_resampler_destroy(d->resampler);
        d->resampler = NULL; d->rate = rate; d->channels = channels;
        if (rate != 48000) {
            d->resampler = speex_resampler_init_frac(channels, rate, 48000, rate, 48000, 1, &err);
            if (!d->resampler || err) return -1;
        }
    }
    remain = bytes / (channels * sizeof(int16_t));
    while (remain && !cancelled()) {
        spx_uint32_t consumed = remain, produced = NET_AUDIO_BLOCK_FRAMES;
        int err = 0;
        if (d->resampler) {
            if (channels == 1) err = speex_resampler_process_int(d->resampler, 0,
                pcm, &consumed, d->output, &produced);
            else err = speex_resampler_process_interleaved_int(d->resampler,
                pcm, &consumed, d->output, &produced);
        } else {
            consumed = produced = remain > NET_AUDIO_BLOCK_FRAMES ? NET_AUDIO_BLOCK_FRAMES : remain;
            memcpy(d->output, pcm, consumed * channels * sizeof(int16_t));
        }
        if (err || (!consumed && !produced)) return -1;
        if (channels == 1) {
            unsigned int i = produced;
            while (i--) d->output[i * 2] = d->output[i * 2 + 1] = d->output[i];
        }
        if (produced && push_pcm(d, produced) != 0) return -1;
        pcm += consumed * channels; remain -= consumed;
    }
    return cancelled() ? -1 : 0;
}
static int decode_available(net_audio_decode_t *d)
{
    unsigned int failures = 0;
    while (!cancelled()) {
        char *output = NULL;
        unsigned char *ready = NULL;
        int size = 0, ready_size = 0, ret;
        if (DecRequestPcmBuffer(d->decoder, &output) < 0) return -1;
        ret = DecodeAudioStream(d->decoder, &d->format, output, &size);
        if (ret == ERR_AUDIO_DEC_NO_BITSTREAM || ret == ERR_AUDIO_DEC_ABSEND) return 0;
        if (ret == ERR_AUDIO_DEC_EXIT || ret == ERR_AUDIO_DEC_ENDINGCHKFAIL) return -1;
        if (ret != ERR_AUDIO_DEC_NONE || size <= 0) {
            if (++failures > 32) return -1;
            continue;
        }
        failures = 0;
        DecUpdatePcmBuffer(d->decoder, size);
        if (PlybkRequestPcmBuffer(d->decoder, &ready, &ready_size) < 0 || ready_size <= 0) return -1;
        ret = convert_pcm(d, (const int16_t *)ready, (unsigned int)ready_size);
        PlybkUpdatePcmBuffer(d->decoder, ready_size);
        if (ret != 0) return -1;
    }
    return -1;
}
static int mp3_output(void *user, const int16_t *pcm, unsigned int frames,
                      unsigned int rate, unsigned int channels)
{
    net_audio_decode_t *d = user;
    d->format.nSampleRate = rate; d->format.nChannelNum = channels;
    d->format.nBitsPerSample = 16;
    return convert_pcm(d, pcm, frames * channels * sizeof(int16_t));
}
static int drain_decoder(net_audio_decode_t *d)
{
    if (d->mp3) return divoom_net_mp3_feed(d->mp3, NULL, 0, 1, mp3_output, d);
    if (d->aac)
    {
        return divoom_net_aac_feed(d->aac, NULL, 0, 1, mp3_output, d);
    }
    if (!d->decoder)
    {
        return 0;
    }
    ParserSetDecodingEOS(d->decoder);
    return decode_available(d);
}
/* 0=natural EOF, -1=transport/decode error, -2=unsupported source. */
/* 先用压缩数据保留约6秒音频；128kbps仅需96KiB，比扩大PCM更节省内存。
 * 最多等6秒，按20ms响应取消；不支持缓存查询或可寻址文件直接沿用原流程。 */
static int divoom_net_audio_prefill(CdxStreamT *stream, int32 bitrate)
{
    struct StreamCacheStateS cache = {0};
    uint32 begin, target;
    int32 previous;
    if (CdxStreamControl(stream, STREAM_CMD_GET_CACHESTATE, &cache) < 0 || cache.nCacheCapacity <= 0)
    {
        return 0;
    }
    int64_t source_bytes = CdxStreamSize(stream);
    /* Cdx以0或-1表示未知长度，仅正数才是可按有限文件处理的大小。 */
    if (source_bytes > 0)
    {
        return 0;
    }
    if (bitrate < 8000 || bitrate > 1000000)
    {
        bitrate = 128000;
    }
    target = (uint32)bitrate * 6u / 8u;
    if (target < 16384u)
    {
        target = 16384u;
    }
    if (target > 131072u)
    {
        target = 131072u;
    }
    __atomic_store_n(&net_audio.diag.compressed_target, target, __ATOMIC_RELAXED);
    previous = __atomic_load_n(&net_audio.state, __ATOMIC_ACQUIRE);
    begin = divoom_net_audio_diag_clock_ms();
    set_state(DIVOOM_NET_AUDIO_BUFFERING);
    divoom_net_audio_diag_phase(4);
    while (!cancelled())
    {
        if (CdxStreamControl(stream, STREAM_CMD_GET_CACHESTATE, &cache) < 0)
        {
            break;
        }
        __atomic_store_n(&net_audio.diag.compressed_bytes, cache.nCacheSize, __ATOMIC_RELAXED);
        __atomic_store_n(&net_audio.diag.compressed_capacity, cache.nCacheCapacity, __ATOMIC_RELAXED);
        if (cache.nCacheSize >= (int32)target || divoom_net_audio_diag_clock_ms() - begin >= 6000u ||
            CdxStreamGetIoState(stream) == CDX_IO_STATE_ERROR)
        {
            break;
        }
        if (wait_cancel(20) != 0)
        {
            break;
        }
    }
    __atomic_store_n(&net_audio.diag.prefill_ms, divoom_net_audio_diag_clock_ms() - begin, __ATOMIC_RELAXED);
    if (!cancelled())
    {
        set_state(previous);
    }
    return cancelled() ? -1 : 0;
}

/* HLS下载/解复用生产者与现有解码worker分离；只缓冲压缩ES，不增加PCM环形缓冲。
 * stream只由生产者关闭，停止方在source_lock内仅ForceStop；解码端先join再清理。 */
#define DIVOOM_NET_HLS_COMPRESSED_BYTES (128u * 1024u)
#define DIVOOM_NET_HLS_PREFILL_BYTES (64u * 1024u)
typedef struct
{
    pthread_mutex_t lock;
    pthread_t producer;
    uint32 read_pos, write_pos, filled;
    int32 done, result, abort;
    uint8 data[DIVOOM_NET_HLS_COMPRESSED_BYTES];
} divoom_net_audio_hls_pipe;

static int32 divoom_net_audio_hls_cancel(void *user)
{
    divoom_net_audio_hls_pipe *pipe = user;
    return cancelled() || __atomic_load_n(&pipe->abort, __ATOMIC_ACQUIRE);
}

static int32 divoom_net_audio_hls_wait(void *user, uint32 milliseconds)
{
    while (milliseconds && !divoom_net_audio_hls_cancel(user))
    {
        uint32 slice = milliseconds > 20 ? 20 : milliseconds;
        if (wait_cancel(slice))
        {
            return -1;
        }
        milliseconds -= slice;
    }
    return divoom_net_audio_hls_cancel(user) ? -1 : 0;
}

static int32 divoom_net_audio_hls_open(void *user, const char *url)
{
    CdxDataSourceT source;
    if (divoom_net_audio_hls_cancel(user))
    {
        return -1;
    }
    memset(&source, 0, sizeof(source));
    source.uri = (char *)url;
    int32 result = CdxStreamOpen(&source, &net_audio.source_lock, &net_audio.hls_cancel, &net_audio.stream, NULL);
    return result < 0 ? -1 : 0;
}

static int32 divoom_net_audio_hls_read(void *user, uint8 *data, uint32 bytes)
{
    if (divoom_net_audio_hls_cancel(user) || !net_audio.stream)
    {
        return -1;
    }
    uint32 begin = divoom_net_audio_diag_clock_ms();
    int32 result = CdxStreamRead(net_audio.stream, data, (int32)bytes);
    uint32 elapsed = divoom_net_audio_diag_clock_ms() - begin;
    divoom_net_audio_diag_max(&net_audio.diag.read_max_ms, elapsed);
    __atomic_fetch_add(&net_audio.diag.read_ms, elapsed, __ATOMIC_RELAXED);
    return result;
}

static void divoom_net_audio_hls_close(void *user)
{
    (void)user;
    close_source();
}

/* pipe锁内的环形复制，write=0表示消费；调用方已验证可用空间或完整记录长度。 */
static void divoom_net_audio_hls_copy(divoom_net_audio_hls_pipe *pipe, uint8 *data, uint32 bytes, uint8 write)
{
    uint32 *position = write ? &pipe->write_pos : &pipe->read_pos;
    uint32 first = DIVOOM_NET_HLS_COMPRESSED_BYTES - *position;
    if (first > bytes)
    {
        first = bytes;
    }
    if (write)
    {
        memcpy(pipe->data + *position, data, first);
        memcpy(pipe->data, data + first, bytes - first);
        pipe->filled += bytes;
    }
    else
    {
        memcpy(data, pipe->data + *position, first);
        memcpy(data + first, pipe->data, bytes - first);
        pipe->filled -= bytes;
    }
    *position = (*position + bytes) % DIVOOM_NET_HLS_COMPRESSED_BYTES;
}

static int32 divoom_net_audio_hls_emit(void *user, uint8 codec, const uint8 *data, uint32 bytes)
{
    divoom_net_audio_hls_pipe *pipe = user;
    if (bytes > 4096 || (bytes && !data))
    {
        return -1;
    }
    uint8 header[4] = {codec, 0, (uint8)bytes, (uint8)(bytes >> 8)};
    while (!divoom_net_audio_hls_cancel(user))
    {
        pthread_mutex_lock(&pipe->lock);
        if (DIVOOM_NET_HLS_COMPRESSED_BYTES - pipe->filled >= bytes + sizeof(header))
        {
            divoom_net_audio_hls_copy(pipe, header, sizeof(header), 1);
            if (bytes)
            {
                divoom_net_audio_hls_copy(pipe, (uint8 *)data, bytes, 1);
            }
            pthread_mutex_unlock(&pipe->lock);
            return 0;
        }
        pthread_mutex_unlock(&pipe->lock);
        if (divoom_net_audio_hls_wait(user, 10))
        {
            break;
        }
    }
    return -1;
}

static void *divoom_net_audio_hls_producer(void *user)
{
    struct sched_param scheduling = {0};
    int policy = 0;
    pthread_getschedparam(pthread_self(), &policy, &scheduling);
    __atomic_store_n(&net_audio.diag.producer_prio, scheduling.sched_priority, __ATOMIC_RELAXED);
    divoom_net_audio_hls_pipe *pipe = user;
    divoom_net_hls_io io = {pipe, divoom_net_audio_hls_open, divoom_net_audio_hls_read,
        divoom_net_audio_hls_close, divoom_net_audio_hls_wait, divoom_net_audio_hls_cancel,
        divoom_net_audio_hls_emit};
    int32 result = divoom_net_hls_run(net_audio.url, &io);
    pthread_mutex_lock(&pipe->lock);
    pipe->result = result;
    pipe->done = 1;
    pthread_mutex_unlock(&pipe->lock);
    return NULL;
}

static void divoom_net_audio_hls_reset_decode(net_audio_decode_t *decoder)
{
    divoom_net_mp3_destroy(decoder->mp3);
    decoder->mp3 = NULL;
    divoom_net_aac_destroy(decoder->aac);
    decoder->aac = NULL;
    if (decoder->resampler)
    {
        speex_resampler_destroy(decoder->resampler);
        decoder->resampler = NULL;
    }
    decoder->rate = decoder->channels = 0;
}

static int32 divoom_net_audio_hls_decode(net_audio_decode_t *decoder, uint8 codec,
    const uint8 *data, uint32 bytes)
{
    if (codec == DIVOOM_NET_HLS_RESET)
    {
        divoom_net_audio_hls_reset_decode(decoder);
        return 0;
    }
    if (codec == DIVOOM_NET_HLS_SEGMENT)
    {
        __atomic_fetch_add(&net_audio.diag.hls_segments, 1, __ATOMIC_RELAXED);
        return 0;
    }
    if (codec == DIVOOM_NET_HLS_MP3)
    {
        if (!decoder->mp3)
        {
            decoder->mp3 = divoom_net_mp3_create();
        }
        return decoder->mp3 ? divoom_net_mp3_feed(decoder->mp3, data, bytes, 0, mp3_output, decoder) : -1;
    }
    if (codec == DIVOOM_NET_HLS_AAC)
    {
        if (!decoder->aac)
        {
            decoder->aac = divoom_net_aac_create();
        }
        return decoder->aac ? divoom_net_aac_feed(decoder->aac, data, bytes, 0, mp3_output, decoder) : -1;
    }
    return -2;
}

/* RTOS pthread默认是空闲优先级，压缩数据充足时也可被后台业务饿死。
 * 电台解码优先于 LVGL 供数，I2S sink 优先消费 PCM；HLS 下载沿用普通业务优先级。 */
static int32 divoom_net_audio_thread_priority(pthread_attr_t *attributes, int32 priority)
{
#if defined(CONFIG_KERNEL_FREERTOS)
    struct sched_param scheduling = {0};
    scheduling.sched_priority = priority;
    return pthread_attr_setschedparam(attributes, &scheduling);
#else
    (void)attributes;
    (void)priority;
    return 0;
#endif
}

static int32 divoom_net_audio_play_hls(net_audio_decode_t *decoder)
{
    divoom_net_audio_hls_pipe *pipe = divoom_system_malloc(sizeof(*pipe));
    if (!pipe)
    {
        return -1;
    }
    if (pthread_mutex_init(&pipe->lock, NULL))
    {
        divoom_system_free(pipe);
        return -1;
    }
    pthread_mutex_lock(&net_audio.source_lock);
    if (cancelled())
    {
        pthread_mutex_unlock(&net_audio.source_lock);
        pthread_mutex_destroy(&pipe->lock);
        divoom_system_free(pipe);
        return -1;
    }
    __atomic_store_n(&net_audio.hls_cancel, 0, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&net_audio.source_lock);
    pthread_attr_t attributes;
    int32 result = pthread_attr_init(&attributes);
    if (result)
    {
        pthread_mutex_destroy(&pipe->lock);
        divoom_system_free(pipe);
        return -1;
    }
    result = pthread_attr_setstacksize(&attributes, 32 * 1024);
    if (!result)
    {
        result = divoom_net_audio_thread_priority(&attributes, DIVOOM_SYSTEM_TASK_DEFAULT_PRIO);
    }
    if (!result)
    {
        result = pthread_create(&pipe->producer, &attributes, divoom_net_audio_hls_producer, pipe);
    }
    pthread_attr_destroy(&attributes);
    if (result)
    {
        pthread_mutex_destroy(&pipe->lock);
        divoom_system_free(pipe);
        return -1;
    }
    __atomic_store_n(&net_audio.diag.compressed_capacity, DIVOOM_NET_HLS_COMPRESSED_BYTES, __ATOMIC_RELAXED);
    __atomic_store_n(&net_audio.diag.compressed_target, DIVOOM_NET_HLS_PREFILL_BYTES, __ATOMIC_RELAXED);
    uint32 begin = divoom_net_audio_diag_clock_ms();
    divoom_net_audio_diag_phase(4);
    set_state(DIVOOM_NET_AUDIO_BUFFERING);
    uint8 started = 0;
    uint8 data[4096];
    while (!cancelled())
    {
        uint8 codec = 0;
        uint32 bytes = 0;
        pthread_mutex_lock(&pipe->lock);
        uint32 filled = pipe->filled;
        __atomic_store_n(&net_audio.diag.compressed_bytes, filled, __ATOMIC_RELAXED);
        if (!started && (filled >= DIVOOM_NET_HLS_PREFILL_BYTES || pipe->done ||
            divoom_net_audio_diag_clock_ms() - begin >= 3000))
        {
            started = 1;
            __atomic_store_n(&net_audio.diag.prefill_ms, divoom_net_audio_diag_clock_ms() - begin, __ATOMIC_RELAXED);
        }
        if (started && filled >= 4)
        {
            uint8 header[4];
            divoom_net_audio_hls_copy(pipe, header, sizeof(header), 0);
            codec = header[0];
            bytes = header[2] | ((uint32)header[3] << 8);
            if (bytes > sizeof(data) || bytes > pipe->filled)
            {
                pthread_mutex_unlock(&pipe->lock);
                result = -1;
                break;
            }
            if (bytes)
            {
                divoom_net_audio_hls_copy(pipe, data, bytes, 0);
            }
        }
        else if (pipe->done && !filled)
        {
            result = pipe->result;
            pthread_mutex_unlock(&pipe->lock);
            break;
        }
        else
        {
            pthread_mutex_unlock(&pipe->lock);
            divoom_net_audio_diag_phase(2);
            if (wait_cancel(10))
            {
                result = -1;
                break;
            }
            continue;
        }
        pthread_mutex_unlock(&pipe->lock);
        divoom_net_audio_diag_phase(3);
        uint32 decode_begin = divoom_net_audio_diag_clock_ms();
        uint64_t frames_before = decoder->frames;
        result = divoom_net_audio_hls_decode(decoder, codec, data, bytes);
        if (result)
        {
            break;
        }
        uint32 now = divoom_net_audio_diag_clock_ms();
        if (decoder->frames > frames_before)
        {
            uint32 previous = __atomic_load_n(&net_audio.diag.last_pcm_ms, __ATOMIC_RELAXED);
            if (previous)
            {
                divoom_net_audio_diag_max(&net_audio.diag.pcm_gap_max_ms, now - previous);
            }
            __atomic_store_n(&net_audio.diag.last_pcm_ms, now, __ATOMIC_RELAXED);
        }
        __atomic_fetch_add(&net_audio.diag.packets, 1, __ATOMIC_RELAXED);
        __atomic_fetch_add(&net_audio.diag.input_bytes, bytes, __ATOMIC_RELAXED);
        __atomic_store_n(&net_audio.diag.frames, (uint32)decoder->frames, __ATOMIC_RELAXED);
        divoom_net_audio_diag_max(&net_audio.diag.decode_sink_max_ms, now - decode_begin);
        __atomic_fetch_add(&net_audio.diag.work_ms, now - decode_begin, __ATOMIC_RELAXED);
    }
    /* 不持pipe/source锁join；先撤销本生产者并中断其HTTP，防止满队列或网络等待拖住STOP。 */
    __atomic_store_n(&pipe->abort, 1, __ATOMIC_RELEASE);
    pthread_mutex_lock(&net_audio.source_lock);
    __atomic_store_n(&net_audio.hls_cancel, 1, __ATOMIC_RELEASE);
    if (net_audio.stream)
    {
        CdxStreamForceStop(net_audio.stream);
    }
    pthread_mutex_unlock(&net_audio.source_lock);
    pthread_join(pipe->producer, NULL);
    pthread_mutex_destroy(&pipe->lock);
    divoom_system_free(pipe);
    return cancelled() ? -1 : result;
}

static int32 divoom_net_audio_hls_url(const char *url)
{
    size_t length = strcspn(url, "?#");
    static const char suffix[] = ".m3u8";
    if (length < sizeof(suffix) - 1)
    {
        return 0;
    }
    const char *end = url + length - (sizeof(suffix) - 1);
    for (uint32 i = 0; i < sizeof(suffix) - 1; i++)
    {
        uint8 c = (uint8)end[i];
        if (c >= 'A' && c <= 'Z')
        {
            c += 'a' - 'A';
        }
        if (c != (uint8)suffix[i])
        {
            return 0;
        }
    }
    return 1;
}

static int play_source(net_audio_decode_t *d, int64_t *resume_us)
{
    CdxDataSourceT source;
    CdxMediaInfoT media;
    CdxPacketT packet;
#if defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_ADEC_AAC)
    AudioDecBufInfo buffers = { NET_AUDIO_BS_BYTES, NET_AUDIO_PCM_BYTES };
#endif
    CdxParserT *parser;
    CdxStreamT *stream;
    uint64_t frame_start = d->frames;
    int64_t pts_start;
    int ret;
    memset(&source, 0, sizeof(source)); memset(&media, 0, sizeof(media));
    source.uri = net_audio.url;
    divoom_net_audio_diag_phase(1); /* 1建连/探测，2读压缩流，3解码及sink背压。 */
    if (divoom_net_audio_hls_url(net_audio.url))
    {
        return divoom_net_audio_play_hls(d);
    }
    /* 普通HTTP仍只建一次连接；无后缀的HLS由流探测头识别，不依赖本地配置。 */
    ret = CdxStreamOpen(&source, &net_audio.source_lock, &net_audio.cancel, &net_audio.stream, NULL);
    if (ret < 0 || cancelled()) return -1;
    CdxStreamProbeDataT *probe = CdxStreamGetProbeData(net_audio.stream);
    if (probe && probe->len >= 7 && !memcmp(probe->buf, "#EXTM3U", 7))
    {
        close_source();
        return divoom_net_audio_play_hls(d);
    }
    ret = CdxParserOpen(net_audio.stream, DISABLE_VIDEO | DISABLE_SUBTITLE | NO_NEED_DURATION,
        &net_audio.source_lock, &net_audio.cancel, &net_audio.parser, NULL);
    if (ret < 0 || cancelled()) return -1;
    parser = net_audio.parser; stream = net_audio.stream;
    if (CdxParserGetMediaInfo(parser, &media) < 0 || media.programNum < 1 ||
        media.programIndex >= media.programNum || media.programIndex >= PROGRAM_LIMIT ||
        media.programIndex < 0 ||
        media.program[media.programIndex].audioNum < 1) return -2;
    d->format = media.program[media.programIndex].audio[0];
    if (d->format.eCodecFormat != AUDIO_CODEC_FORMAT_MP3 &&
        d->format.eCodecFormat != AUDIO_CODEC_FORMAT_MPEG_AAC_LC &&
        d->format.eCodecFormat != AUDIO_CODEC_FORMAT_RAAC &&
        d->format.eCodecFormat != AUDIO_CODEC_FORMAT_MPEG_AAC_LATM) return -2;
    if (*resume_us > 0 && media.bSeekable &&
        CdxParserSeekTo(parser, *resume_us, AW_SEEK_CLOSEST_SYNC) < 0) return -1;
    if (!media.bSeekable) *resume_us = 0;
    pts_start = *resume_us;
    if (divoom_net_audio_prefill(stream, d->format.nAvgBitrate) != 0)
    {
        return -1;
    }
    memset(&d->info, 0, sizeof(d->info));
    if (d->format.eCodecFormat == AUDIO_CODEC_FORMAT_MP3) {
        d->mp3 = divoom_net_mp3_create();
        if (!d->mp3) return -1;
    } else {
        /* CdxAacParser把ADTS标为RAAC，是否可解码仍由ADTS帧头验证，不接受ADIF/LATM。 */
        if (d->format.eCodecFormat == AUDIO_CODEC_FORMAT_MPEG_AAC_LC ||
            d->format.eCodecFormat == AUDIO_CODEC_FORMAT_RAAC)
        {
            d->aac = divoom_net_aac_create();
            if (!d->aac)
            {
                return -1;
            }
        }
        else
        {
#if defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_ADEC_AAC)
            d->decoder = CreateAudioDecoder();
            if (!d->decoder || InitializeAudioDecoder(d->decoder, &d->format, &d->info, &buffers) != 0) return -1;
#else
            return -2;
#endif
        }
    }
    while (!cancelled()) {
        unsigned char *bs = NULL, *ring = NULL;
        int bs_size = 0, ring_size = 0, offset = 0;
        uint32_t read_begin, decode_begin, now;
        uint64_t frames_before = d->frames;
        memset(&packet, 0, sizeof(packet));
        read_begin = divoom_net_audio_diag_clock_ms();
        divoom_net_audio_diag_phase(2);
        if (CdxParserPrefetch(parser, &packet) < 0) {
            divoom_net_audio_diag_max(&net_audio.diag.read_max_ms, divoom_net_audio_diag_clock_ms() - read_begin);
            if (CdxParserGetStatus(parser) != PSR_EOS || CdxStreamGetIoState(stream) == CDX_IO_STATE_ERROR)
            {
                if (!cancelled())
                {
                    __atomic_fetch_add(&net_audio.diag.read_errors, 1, __ATOMIC_RELAXED);
                }
                return -1;
            }
            return drain_decoder(d);
        }
        if (packet.type != CDX_MEDIA_AUDIO || packet.length <= 0 || packet.length > NET_AUDIO_BS_BYTES) return -2;
        if (d->mp3 || d->aac) {
            if (packet.length > d->packet_capacity) {
                unsigned char *next = realloc(d->packet_data, (size_t)packet.length);
                if (!next) return -1;
                d->packet_data = next; d->packet_capacity = packet.length;
            }
            bs = d->packet_data; bs_size = d->packet_capacity;
        } else if (ParserRequestBsBuffer(d->decoder, packet.length, &bs, &bs_size, &ring, &ring_size, &offset) < 0) return -1;
        packet.buf = bs; packet.buflen = bs_size; packet.ringBuf = ring; packet.ringBufLen = ring_size;
        if (CdxParserRead(parser, &packet) < 0) {
            divoom_net_audio_diag_max(&net_audio.diag.read_max_ms, divoom_net_audio_diag_clock_ms() - read_begin);
            /* Unknown-length MP3 discovers EOF in read, after prefetch. */
            if (CdxParserGetStatus(parser) != PSR_EOS || CdxStreamGetIoState(stream) == CDX_IO_STATE_ERROR)
            {
                if (!cancelled())
                {
                    __atomic_fetch_add(&net_audio.diag.read_errors, 1, __ATOMIC_RELAXED);
                }
                return -1;
            }
            return drain_decoder(d);
        }
        if (packet.length <= 0 || packet.length > bs_size + ring_size) return -1;
        now = divoom_net_audio_diag_clock_ms();
        divoom_net_audio_diag_max(&net_audio.diag.read_max_ms, now - read_begin);
        __atomic_fetch_add(&net_audio.diag.read_ms, now - read_begin, __ATOMIC_RELAXED);
        __atomic_fetch_add(&net_audio.diag.input_bytes, (uint32_t)packet.length, __ATOMIC_RELAXED);
        __atomic_fetch_add(&net_audio.diag.packets, 1, __ATOMIC_RELAXED);
        decode_begin = now;
        /* 周期采样压缩水位，保持正常播放不产生逐包打印。 */
        if (now - d->cache_sample_ms >= 500u)
        {
            struct StreamCacheStateS cache = {0};
            d->cache_sample_ms = now;
            if (CdxStreamControl(stream, STREAM_CMD_GET_CACHESTATE, &cache) == 0)
            {
                __atomic_store_n(&net_audio.diag.compressed_bytes, cache.nCacheSize, __ATOMIC_RELAXED);
                __atomic_store_n(&net_audio.diag.compressed_capacity, cache.nCacheCapacity, __ATOMIC_RELAXED);
            }
        }
        divoom_net_audio_diag_phase(3);
        if (d->mp3) {
            if (divoom_net_mp3_feed(d->mp3, bs, (size_t)packet.length, 0, mp3_output, d) < 0) return -1;
        }
        else if (d->aac)
        {
            int32 aac_result = divoom_net_aac_feed(d->aac, bs, (uint32)packet.length, 0, mp3_output, d);
            if (aac_result)
            {
                return aac_result;
            }
        }
        else {
            if (ParserUpdateBsBuffer(d->decoder, packet.length, packet.pts, offset) < 0 ||
                decode_available(d) < 0) return -1;
        }
        now = divoom_net_audio_diag_clock_ms();
        /* 该耗时包含正常sink背压，不能当作解码CPU耗时。真正欠载由sink计数核对。 */
        divoom_net_audio_diag_max(&net_audio.diag.decode_sink_max_ms, now - decode_begin);
        __atomic_fetch_add(&net_audio.diag.work_ms, now - decode_begin, __ATOMIC_RELAXED);
        __atomic_store_n(&net_audio.diag.frames, (uint32_t)d->frames, __ATOMIC_RELAXED);
        if (d->frames > frames_before)
        {
            uint32_t previous = __atomic_load_n(&net_audio.diag.last_pcm_ms, __ATOMIC_RELAXED);
            if (previous)
            {
                divoom_net_audio_diag_max(&net_audio.diag.pcm_gap_max_ms, now - previous);
            }
            __atomic_store_n(&net_audio.diag.last_pcm_ms, now, __ATOMIC_RELAXED);
        }
        /* MP3 reports a constant packet PTS until a seek. Track PCM progress
         * as well, so a rebuilt finite source does not restart at the head. */
        *resume_us = pts_start + (int64_t)((d->frames - frame_start) * 1000000 / 48000);
        if (packet.pts > *resume_us) *resume_us = packet.pts;
    }
    return -1;
}
static void cleanup_decode(net_audio_decode_t *d)
{
#if defined(DIVOOM_PRODUCT_ASTRO_TOO)
    divoom_eq_noise_mic_fft_reset_radio();
#endif
    if (d->decoder) { DestroyAudioDecoder(d->decoder); d->decoder = NULL; }
    divoom_net_mp3_destroy(d->mp3); d->mp3 = NULL;
    divoom_net_aac_destroy(d->aac); d->aac = NULL;
    free(d->packet_data); d->packet_data = NULL; d->packet_capacity = 0;
    /* Parser-owned codec metadata outlives the decoder. */
    close_source();
    if (d->resampler) { speex_resampler_destroy(d->resampler); d->resampler = NULL; }
    d->rate = d->channels = 0;
}
static void *network_worker(void *unused)
{
    (void)unused;
    AwStreamInit(); AwParserInit();
    for (;;) {
        net_audio_decode_t *d;
        divoom_net_audio_callback callback;
        void *user;
        unsigned int retries = 0;
        int64_t resume_us = 0;
        int result = -1;
        pthread_mutex_lock(&net_audio.source_lock);
        while (!net_audio.requested) pthread_cond_wait(&net_audio.wake, &net_audio.source_lock);
        net_audio.requested = 0; callback = net_audio.callback; user = net_audio.user;
        pthread_mutex_unlock(&net_audio.source_lock);
        d = calloc(1, sizeof(*d));
        if (d) {
            divoom_net_audio_diag_reset();
            divoom_audio_policy_network_active(1);
            while (!cancelled()) {
                uint64_t before = d->frames;
                set_state(retries ? DIVOOM_NET_AUDIO_RECONNECTING : DIVOOM_NET_AUDIO_CONNECTING);
                result = play_source(d, &resume_us);
                cleanup_decode(d);
                if (result == 0 || result == -2 || cancelled()) break;
                if (d->frames > before) retries = 0;
                __atomic_fetch_add(&net_audio.diag.retries, 1, __ATOMIC_RELAXED);
                if (++retries > NET_AUDIO_RETRIES) break;
                set_state(DIVOOM_NET_AUDIO_RECONNECTING);
                if (wait_cancel(500u << (retries - 1)) < 0) break;
            }
            if (d->sink_open) {
                if (result == 0 && !cancelled()) {
                    unsigned int waits = 0;
                    /* EOF放行短尾音；排空上限随实际输出队列容量计算。 */
                    unsigned int limit = divoom_audio_sink_ch_ring_capacity(DIVOOM_AUDIO_SINK_CH_NETWORK) / 1920u + 100u;
                    divoom_audio_sink_ch_drain(DIVOOM_AUDIO_SINK_CH_NETWORK);
                    while (divoom_audio_sink_ch_ring_bytes(DIVOOM_AUDIO_SINK_CH_NETWORK) && !cancelled()) {
                        if (++waits > limit) { result = -1; break; }
                        if (wait_cancel(10) < 0) break;
                    }
                }
                /* 尚未越过预缓冲门限时STOP不能把积存片段突然播出来。 */
                if (divoom_audio_sink_ch_buffering(DIVOOM_AUDIO_SINK_CH_NETWORK))
                {
                    divoom_audio_sink_ch_mute_drop(DIVOOM_AUDIO_SINK_CH_NETWORK);
                }
                divoom_audio_sink_ch_fade_out(DIVOOM_AUDIO_SINK_CH_NETWORK, 15);
                divoom_audio_sink_ch_fade_out_wait(DIVOOM_AUDIO_SINK_CH_NETWORK, 100);
                divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_NETWORK);
                divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_NETWORK);
            }
            divoom_audio_policy_network_active(0);
            free(d);
        }
        {
            divoom_net_audio_event_t event = cancelled() ? DIVOOM_NET_AUDIO_STOPPED :
                (result == 0 ? DIVOOM_NET_AUDIO_COMPLETE : DIVOOM_NET_AUDIO_ERROR);
            pthread_mutex_lock(&net_audio.source_lock);
            net_audio.active = 0; set_state(DIVOOM_NET_AUDIO_IDLE);
            divoom_net_audio_diag_phase(0);
            pthread_cond_broadcast(&net_audio.idle);
            pthread_mutex_unlock(&net_audio.source_lock);
            if (callback) callback(event, user);
        }
    }
    return NULL;
}
/* api_lock held; source_lock pins publication while ForceStop runs. Only the
 * worker closes objects after acquiring the same publication lock. */
static void stop_locked(void)
{
    __atomic_store_n(&net_audio.cancel, 1, __ATOMIC_RELEASE);
    pthread_mutex_lock(&net_audio.source_lock);
    __atomic_store_n(&net_audio.hls_cancel, 1, __ATOMIC_RELEASE);
    if (net_audio.active) set_state(DIVOOM_NET_AUDIO_STOPPING);
    pthread_cond_broadcast(&net_audio.wake);
    if (net_audio.stream) CdxStreamForceStop(net_audio.stream);
    while (net_audio.active) pthread_cond_wait(&net_audio.idle, &net_audio.source_lock);
    pthread_mutex_unlock(&net_audio.source_lock);
}
int divoom_net_audio_play(const char *url, unsigned int volume,
                         divoom_net_audio_callback callback, void *user)
{
    int ret;
    if (!url || (strncmp(url, "http://", 7) && strncmp(url, "https://", 8)) || strlen(url) >= NET_AUDIO_URL_MAX) return -1;
    /* A callback is on this worker. Never let it block behind an API caller
     * waiting for the same worker, or replace another caller's pending URL. */
    if (__atomic_load_n(&net_audio.started, __ATOMIC_ACQUIRE) &&
        pthread_equal(pthread_self(), net_audio.worker)) {
        if (pthread_mutex_trylock(&net_audio.api_lock) != 0) return -1;
        pthread_mutex_lock(&net_audio.source_lock);
        ret = net_audio.active;
        pthread_mutex_unlock(&net_audio.source_lock);
        if (ret) { pthread_mutex_unlock(&net_audio.api_lock); return -1; }
    } else {
        pthread_mutex_lock(&net_audio.api_lock);
        stop_locked();
    }
    if (!net_audio.started) {
        pthread_attr_t attr;
        ret = pthread_attr_init(&attr);
        if (ret)
        {
            pthread_mutex_unlock(&net_audio.api_lock);
            return -1;
        }
        ret = pthread_attr_setstacksize(&attr, 32 * 1024);
        if (!ret)
        {
            ret = divoom_net_audio_thread_priority(&attr, DIVOOM_NET_AUDIO_DECODE_TASK_PRIO);
        }
        if (!ret)
        {
            ret = pthread_create(&net_audio.worker, &attr, network_worker, NULL);
        }
        pthread_attr_destroy(&attr);
        if (ret) { pthread_mutex_unlock(&net_audio.api_lock); return -1; }
        __atomic_store_n(&net_audio.started, 1, __ATOMIC_RELEASE);
    }
    pthread_mutex_lock(&net_audio.source_lock);
    strcpy(net_audio.url, url);
    volume = volume > 100 ? 100 : volume;
    __atomic_store_n(&net_audio.volume, volume, __ATOMIC_RELEASE);
    divoom_audio_policy_network_set_volume(volume);
    __atomic_store_n(&net_audio.cancel, 0, __ATOMIC_RELEASE);
    net_audio.callback = callback; net_audio.user = user;
    net_audio.active = net_audio.requested = 1; set_state(DIVOOM_NET_AUDIO_CONNECTING);
    pthread_cond_signal(&net_audio.wake);
    pthread_mutex_unlock(&net_audio.source_lock);
    pthread_mutex_unlock(&net_audio.api_lock);
    return 0;
}
void divoom_net_audio_stop(void)
{
    if (__atomic_load_n(&net_audio.started, __ATOMIC_ACQUIRE) &&
        pthread_equal(pthread_self(), net_audio.worker)) {
        /* No source is owned while a final callback runs. A pending request
         * can be cancelled without waiting for this worker to execute it. */
        __atomic_store_n(&net_audio.cancel, 1, __ATOMIC_RELEASE);
        pthread_mutex_lock(&net_audio.source_lock);
        pthread_cond_broadcast(&net_audio.wake);
        pthread_mutex_unlock(&net_audio.source_lock);
        return;
    }
    pthread_mutex_lock(&net_audio.api_lock); stop_locked(); pthread_mutex_unlock(&net_audio.api_lock);
}
void divoom_net_audio_set_volume(unsigned int volume)
{
    volume = volume > 100 ? 100 : volume;
    __atomic_store_n(&net_audio.volume, volume, __ATOMIC_RELEASE);
    divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_NETWORK, volume);
    divoom_audio_policy_network_set_volume(volume);
}
divoom_net_audio_state_t divoom_net_audio_get_state(void)
{ return (divoom_net_audio_state_t)__atomic_load_n(&net_audio.state, __ATOMIC_ACQUIRE); }

#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO_CLI
#include <console.h>
/* 串口按需读取，正常播放不逐包打印，避免日志本身影响调度。 */
static void divoom_net_audio_diag_print(void)
{
    uint32_t now = divoom_net_audio_diag_clock_ms();
    uint32_t phase = __atomic_load_n(&net_audio.diag.phase, __ATOMIC_ACQUIRE);
    uint32_t phase_ms = __atomic_load_n(&net_audio.diag.phase_ms, __ATOMIC_RELAXED);
    unsigned int bytes = divoom_audio_sink_ch_ring_bytes(DIVOOM_AUDIO_SINK_CH_NETWORK);
    unsigned int capacity = divoom_audio_sink_ch_ring_capacity(DIVOOM_AUDIO_SINK_CH_NETWORK);
    printf("[net_audio_diag] state=%d phase=%u fill=%u capacity=%u buffered_ms=%u buffering=%d und=%u out=%u packets=%u input=%u frames=%u read_max_ms=%u read_pending_ms=%u decode_sink_max_ms=%u pcm_gap_max_ms=%u retries=%u read_errors=%u elapsed_ms=%u read_ms=%u work_ms=%u wait_ms=%u volume=%u compressed=%u compressed_capacity=%u compressed_target=%u prefill_ms=%u hls_segments=%u worker_prio=%u producer_prio=%u\n",
        divoom_net_audio_get_state(), phase, bytes, capacity, bytes / 192u,
        divoom_audio_sink_ch_buffering(DIVOOM_AUDIO_SINK_CH_NETWORK),
        divoom_audio_sink_whitebox_underruns(DIVOOM_AUDIO_SINK_CH_NETWORK),
        divoom_audio_sink_whitebox_out_frames(),
        __atomic_load_n(&net_audio.diag.packets, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.input_bytes, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.frames, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.read_max_ms, __ATOMIC_RELAXED), phase == 2 ? now - phase_ms : 0,
        __atomic_load_n(&net_audio.diag.decode_sink_max_ms, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.pcm_gap_max_ms, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.retries, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.read_errors, __ATOMIC_RELAXED),
        now - __atomic_load_n(&net_audio.diag.start_ms, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.read_ms, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.work_ms, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.wait_ms, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.volume, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.compressed_bytes, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.compressed_capacity, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.compressed_target, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.prefill_ms, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.hls_segments, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.worker_prio, __ATOMIC_RELAXED),
        __atomic_load_n(&net_audio.diag.producer_prio, __ATOMIC_RELAXED));
}

/* 串口只读播放器状态，播放与音量统一由系统音源控制器管理。 */
static int net_audio_cmd(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "stat"))
    {
        divoom_net_audio_diag_print();
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "state")) { printf("net_audio state=%d\n", divoom_net_audio_get_state()); return 0; }
    printf("net_audio state | stat\n");
    return -1;
}
FINSH_FUNCTION_EXPORT_CMD(net_audio_cmd, net_audio, network MP3/AAC audio);
#endif
