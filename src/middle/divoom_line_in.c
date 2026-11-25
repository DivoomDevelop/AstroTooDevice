#include "divoom.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sched.h>
#include "divoom_line_in.h"
#include "divoom_board_io_config.h"
/* 管道模式 1：arecord|aplay 子进程，与命令行一致、声音干净；0：进程内 ALSA 双线程（同进程录播易不干净） */
#define DIVOOM_LINE_IN_USE_PIPE_CMD 1
#if defined(DIVOOM_LINE_IN_USE_PIPE_CMD) && DIVOOM_LINE_IN_USE_PIPE_CMD
#include <signal.h>
#include <sys/wait.h>
#endif

#ifndef EAGAIN
#define EAGAIN 11
#endif

// Line In 设备（ALSA 库方式，非管道模式时使用）
// 同进程内采集+播放都用 "default" 时易冲突不干净；单独录或单独播则干净。若要用 ALSA 库且同时录播都干净，
// 请在板级 /etc/asound.conf 为 Line-in 单独定义两个 PCM（采集一条链、播放一条链），例如：
//   pcm.linein_cap  { type plug; slave.pcm "hw:0,0"; }   // 采集链，可按需加 rate 等
//   pcm.linein_play { type awequal; slave.pcm "default"; }  // 播放链含 EQ，或 slave 指向 plughw
// 本工程 asound.conf 已提供 linein_cap/linein_play，默认用二者以减轻同进程录播冲突；无则改回 "default"。
// 采集：禁止用裸 "hw:0,0"（无 plug 会卷舌音）。
#ifndef DIVOOM_LINE_IN_PCM_DEVICE
#define DIVOOM_LINE_IN_PCM_DEVICE      "linein_cap"
#endif
#define LINE_IN_CAPTURE_DEVICE_FALLBACK "default"
// 播放：与采集用不同 PCM 名以减少同进程冲突。
#ifndef DIVOOM_LINE_IN_PLAYBACK_DEVICE
#define DIVOOM_LINE_IN_PLAYBACK_DEVICE "linein_play"
#endif
#define LINE_IN_PLAYBACK_DEVICE_FALLBACK "default"
#ifndef DIVOOM_LINE_IN_SAMPLE_RATE
#define DIVOOM_LINE_IN_SAMPLE_RATE     48000   /* 44.1k 下易卡顿，改回 48k 保证连续 */
#endif
/* 直通模式：不做 DC/高通/低通，仅音量+软限幅，避免 DSP 引入嘈杂失真（默认直通） */
#define DIVOOM_LINE_IN_PASSTHROUGH
/* 软低通：直通下也压一点高频嘶声，减轻杂音；不需要可注释掉 */
#define DIVOOM_LINE_IN_SOFT_LPF

/* 调试：确定杂音/颤音是录音还是播放问题（二选一，改完需重编） */
/* 1) 打开 SAVE_CAPTURE：采集 5 秒到 /tmp/linein_cap.raw，再 aplay 该文件，听是否干净 → 判断是否录音问题 */
/* #define DIVOOM_LINE_IN_DEBUG_SAVE_CAPTURE */
/* 2) 打开 PLAY_FILE：播 /tmp/linein_test.raw（S16_LE 48k 立体声 raw），听是否干净 → 判断是否播放问题 */
/* #define DIVOOM_LINE_IN_DEBUG_PLAY_FILE */
/* 3) 关闭录音端、只测播放：打开 DISABLE_CAPTURE，只播 /tmp/linein_test.raw，测播放是否干净 */
/* #define DIVOOM_LINE_IN_DISABLE_CAPTURE */
/* 4) 关闭播放端、只测录音：打开 DISABLE_PLAYBACK，只采、保存到 /tmp/linein_cap.raw(约10s)，用 aplay 播该文件听是否干净（与3二选一） */
/* #define DIVOOM_LINE_IN_DISABLE_PLAYBACK */

/* 若实测【单独录音、单独播放都干净】，但【同时录+播不干净】：说明问题在同进程内同时开 default 采集+播放
 * 导致冲突（arecord|aplay 两进程所以干净）。对策：启用下方管道模式 DIVOOM_LINE_IN_USE_PIPE_CMD 1，
 * 用 arecord|aplay 子进程，与命令行一致、听感通常更干净；或板级 asound.conf 为 Line-in 分离采集/播放 PCM。 */

#define LINE_IN_PCM_DEVICE      DIVOOM_LINE_IN_PCM_DEVICE
#define LINE_IN_PLAYBACK_DEVICE DIVOOM_LINE_IN_PLAYBACK_DEVICE
#define LINE_IN_SAMPLE_RATE     DIVOOM_LINE_IN_SAMPLE_RATE
#define LINE_IN_CHANNELS        2               // 通道数
#define LINE_IN_BUFFER_SIZE     4096            // 单次读写缓冲（≥ TRANSFER 即可）
#define LINE_IN_PERIOD_SIZE     1024            // 周期大小（捕获沿用）
/* 小传输块 1024 帧≈23ms，捕获/播放都更频繁喂数据，减轻 underrun */
#define LINE_IN_TRANSFER_FRAMES 1024
#define LINE_IN_PLAYBACK_BUFFER_FRAMES  131072 // 播放缓冲约 2.7s@48k，加大减轻 underrun/颤音
#define LINE_IN_PLAYBACK_PERIOD_FRAMES  2048   // 周期加大减少 xrun，与 transfer 块数一致
#define LINE_IN_RING_FRAMES             32768  // 环形缓冲约 740ms@48k
#define LINE_IN_RING_MIN_RESERVE        1024    // ring 预填阶段保留，避免预填抽空
/* 正常播写：有一块(1024)就写，避免等 2048 时设备已播完导致“有点声然后没声”；default 链实际 buffer 仅 7680 帧 */
#define LINE_IN_WRITE_THRESHOLD         (LINE_IN_TRANSFER_FRAMES)  // 1024
/* 预填前先等 ring 攒够，预填时一次取够不依赖边采边填，预填后 ring 仍有余量，避免“播一下就没声” */
#define LINE_IN_PREFILL_RING            (10240)  // 10 块≈210ms，预填 8192 后仍剩 2048
#define LINE_IN_PREFILL_DEVICE_FRAMES   (8192)   // 预填约 170ms
/* 每次少写几块，避免一次抽空 ring 再等捕获，缓冲更平稳，减轻颤音 */
#define LINE_IN_PLAYBACK_BURST          4
#define LINE_IN_SILENCE_PERIODS_AFTER_XRUN  1
/* xrun 后只补 1 块静音(1024 帧)，减少听觉上的“抖一下”感；设备 period 实际 960 */
#define LINE_IN_SILENCE_FRAMES_AFTER_XRUN   (LINE_IN_TRANSFER_FRAMES)

// 静音块，xrun 恢复后补静音用（至少 period 大小）
static uint8_t line_in_silence_buf[LINE_IN_PLAYBACK_PERIOD_FRAMES * 4] __attribute__((aligned(4)));

// Line In 控制结构体
typedef struct _divoom_line_in_ctrl {
    snd_pcm_t *capture_handle;          // 音频捕获句柄
    snd_pcm_t *playback_handle;         // 音频播放句柄
    uint8 audio_buffer[LINE_IN_BUFFER_SIZE * 4];  // 临时读/写缓冲
    uint8 ring_buf[LINE_IN_RING_FRAMES * 4];     // 环形缓冲，捕获->播放解耦
    unsigned int ring_rd;                         // 读位置（帧下标）
    unsigned int ring_wr;                         // 写位置（帧下标）
    unsigned int ring_cnt;                        // 当前缓冲帧数
    LineInStatus status;                 // 当前状态
    uint8 volume;                        // 音量 (0-100)
    int run_flag;                        // 运行标志
    void *lock_handle;                   // 锁句柄（保护 ring）
    void *capture_task_handle;           // 捕获线程
    void *playback_task_handle;          // 播放线程
    unsigned int actual_sample_rate;    // 采集实际采样率，播放必须一致，否则会“卷舌头”怪声
    float dc_l, dc_r;                    // DC 滤波状态
    float hp_x_l, hp_x_r, hp_y_l, hp_y_r; // 一阶高通状态
#ifdef DIVOOM_LINE_IN_SOFT_LPF
    float lp_y_l, lp_y_r;                // 一阶低通状态，压高频嘶声
#endif
} divoom_line_in_ctrl, *Pdivoom_line_in_ctrl;

static Pdivoom_line_in_ctrl pdivoom_line_in_ctrl = NULL;
#if defined(DIVOOM_LINE_IN_USE_PIPE_CMD) && DIVOOM_LINE_IN_USE_PIPE_CMD
static pid_t line_in_pipe_pid = -1;       /* arecord 子进程（进程组头） */
static pid_t line_in_pipe_pid_aplay = -1; /* aplay 子进程 */
static pthread_t line_in_pipe_thread;
static int line_in_pipe_read_fd = -1;     /* 从 arecord 读 */
static int line_in_pipe_write_fd = -1;    /* 写给 aplay */
#define LINE_IN_RELAY_BUF_SIZE 8192
#ifndef LINE_IN_MIXER_CARD
#define LINE_IN_MIXER_CARD "1"  /* mixer 回退时用；当前管道模式用软件音量 */
#endif
#endif

// 前向声明
static void divoom_line_in_cleanup(void);
static int divoom_line_in_setup_audio(void);

#if defined(DIVOOM_LINE_IN_USE_PIPE_CMD) && DIVOOM_LINE_IN_USE_PIPE_CMD
/* 软件音量：从 arecord 读 PCM，按 volume(0–100) 缩放后写给 aplay */
static void *line_in_pipe_relay_thread(void *arg)
{
    uint8_t buf[LINE_IN_RELAY_BUF_SIZE];
    int n, w, off;
    uint8_t vol;
    (void)arg;
    while (line_in_pipe_read_fd >= 0 && (n = read(line_in_pipe_read_fd, buf, sizeof(buf))) > 0) 
	{
        vol = pdivoom_line_in_ctrl ? pdivoom_line_in_ctrl->volume : 100;
        if (vol < 100) 
		{
            int16_t *p = (int16_t *)buf;
            int ns = n / 2, i;
            for (i = 0; i < ns; i++) 
			{
                int32_t s = (int32_t)p[i] * (int32_t)vol / 100;
                p[i] = (int16_t)(s < -32768 ? -32768 : (s > 32767 ? 32767 : s));
            }
        }
        for (off = 0; off < n && line_in_pipe_write_fd >= 0; off += w) 
		{
            w = write(line_in_pipe_write_fd, buf + off, (size_t)(n - off));
            if (w <= 0) break;
        }
        if (off < n) 
			break;
    }
    if (line_in_pipe_read_fd >= 0) 
	{ 
		close(line_in_pipe_read_fd); 
		line_in_pipe_read_fd = -1; 
	}
    if (line_in_pipe_write_fd >= 0) 
	{ 
		close(line_in_pipe_write_fd); 
		line_in_pipe_write_fd = -1; 
	}
    return NULL;
}
#endif

// 初始化 ALSA 捕获设备
static int divoom_line_in_init_capture(void)
{
    int err;
    snd_pcm_hw_params_t *hw_params = NULL;
    unsigned int rate = LINE_IN_SAMPLE_RATE;

    if (!pdivoom_line_in_ctrl) 
	{
        return -1;
    }

    /* 禁止采集使用 hw:0,0，否则无 plug 重采样会导致卷舌音/失真，强制用 default */
    const char *capture_dev = LINE_IN_PCM_DEVICE;
    if (strstr(capture_dev, "hw:0,0") != NULL) {
        divoom_print("Line In 采集禁止使用 hw:0,0（会导致卷舌音），改用 default\n");
        capture_dev = LINE_IN_CAPTURE_DEVICE_FALLBACK;
    }
    err = snd_pcm_open(&pdivoom_line_in_ctrl->capture_handle, capture_dev, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) 
	{
        divoom_print("Line In 采集 %s 打开失败: %s，尝试 %s\n",
            capture_dev, snd_strerror(err), LINE_IN_CAPTURE_DEVICE_FALLBACK);
        err = snd_pcm_open(&pdivoom_line_in_ctrl->capture_handle, LINE_IN_CAPTURE_DEVICE_FALLBACK, SND_PCM_STREAM_CAPTURE, 0);
        if (err < 0)
        {
            divoom_print("Line In 采集 fallback 也失败: %s\n", snd_strerror(err));
            return -1;
        }
        capture_dev = LINE_IN_CAPTURE_DEVICE_FALLBACK;
    }

    // 分配硬件参数结构
    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(pdivoom_line_in_ctrl->capture_handle, hw_params);

    // 设置硬件参数
    snd_pcm_hw_params_set_access(pdivoom_line_in_ctrl->capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pdivoom_line_in_ctrl->capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pdivoom_line_in_ctrl->capture_handle, hw_params, LINE_IN_CHANNELS);
    /* 非裸 hw 设备时开 rate_resample（default/linein_cap/plughw 等），保证采集速率稳定 */
    if (strncmp(capture_dev, "hw:", 3) != 0)
        snd_pcm_hw_params_set_rate_resample(pdivoom_line_in_ctrl->capture_handle, hw_params, 1);
    snd_pcm_hw_params_set_rate_near(pdivoom_line_in_ctrl->capture_handle, hw_params, &rate, 0);

    err = snd_pcm_hw_params(pdivoom_line_in_ctrl->capture_handle, hw_params);
    if (err < 0) 
	{
        divoom_print("无法设置 Line In 硬件参数: %s\n", snd_strerror(err));
        snd_pcm_hw_params_free(hw_params);
        return -1;
    }

    snd_pcm_hw_params_get_rate(hw_params, &rate, 0);
    pdivoom_line_in_ctrl->actual_sample_rate = rate;
    snd_pcm_hw_params_free(hw_params);
    snd_pcm_prepare(pdivoom_line_in_ctrl->capture_handle);

    divoom_print("Line In 捕获设备初始化成功 rate=%u\n", (unsigned)rate);
    return 0;
}

// 初始化 ALSA 播放设备（default 时开 rate_resample、period 先于 buffer，与 UAC 一致减失真）
static int divoom_line_in_init_playback(void)
{
    int err;
    snd_pcm_hw_params_t *hw_params = NULL;
    unsigned int rate;
    snd_pcm_uframes_t buffer_frames = LINE_IN_PLAYBACK_BUFFER_FRAMES;
    snd_pcm_uframes_t period_frames = LINE_IN_PLAYBACK_PERIOD_FRAMES;
    const char *dev_used = LINE_IN_PLAYBACK_DEVICE;

    /* 播放必须与采集同率，否则会音调/音色怪（卷舌头感） */
    rate = (pdivoom_line_in_ctrl->actual_sample_rate > 0)
        ? pdivoom_line_in_ctrl->actual_sample_rate
        : LINE_IN_SAMPLE_RATE;

    if (!pdivoom_line_in_ctrl) 
	{
        return -1;
    }

    err = snd_pcm_open(&pdivoom_line_in_ctrl->playback_handle, LINE_IN_PLAYBACK_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) 
	{
        divoom_print("Line In 播放设备 %s 打开失败: %s，尝试 fallback %s\n",
            LINE_IN_PLAYBACK_DEVICE, snd_strerror(err), LINE_IN_PLAYBACK_DEVICE_FALLBACK);
        err = snd_pcm_open(&pdivoom_line_in_ctrl->playback_handle, LINE_IN_PLAYBACK_DEVICE_FALLBACK, SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0)
        {
            divoom_print("Line In 播放 fallback %s 也失败: %s\n", LINE_IN_PLAYBACK_DEVICE_FALLBACK, snd_strerror(err));
            return -1;
        }
        dev_used = LINE_IN_PLAYBACK_DEVICE_FALLBACK;
    }

    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(pdivoom_line_in_ctrl->playback_handle, hw_params);

    snd_pcm_hw_params_set_access(pdivoom_line_in_ctrl->playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pdivoom_line_in_ctrl->playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pdivoom_line_in_ctrl->playback_handle, hw_params, LINE_IN_CHANNELS);
    /* 仅 default 链开 rate_resample；hw 直连不支持会失败 */
    if (strstr(dev_used, "default") != NULL)
        snd_pcm_hw_params_set_rate_resample(pdivoom_line_in_ctrl->playback_handle, hw_params, 1);
    snd_pcm_hw_params_set_rate_near(pdivoom_line_in_ctrl->playback_handle, hw_params, &rate, 0);
    /* 与 UAC 一致：先 period 再 buffer，部分 default 插件对顺序敏感 */
    snd_pcm_hw_params_set_period_size_near(pdivoom_line_in_ctrl->playback_handle, hw_params, &period_frames, 0);
    snd_pcm_hw_params_set_buffer_size_near(pdivoom_line_in_ctrl->playback_handle, hw_params, &buffer_frames);

    err = snd_pcm_hw_params(pdivoom_line_in_ctrl->playback_handle, hw_params);
    if (err < 0) 
	{
        divoom_print("无法设置 Line In 播放硬件参数: %s\n", snd_strerror(err));
        snd_pcm_hw_params_free(hw_params);
        return -1;
    }

    snd_pcm_hw_params_get_rate(hw_params, &rate, 0);
    snd_pcm_hw_params_get_period_size(hw_params, &period_frames, 0);
    snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_frames);
    snd_pcm_hw_params_free(hw_params);
    snd_pcm_prepare(pdivoom_line_in_ctrl->playback_handle);

    divoom_print("Line In 播放设备初始化成功 rate=%u(与采集一致) period=%lu buffer=%lu\n",
        (unsigned)rate, (unsigned long)period_frames, (unsigned long)buffer_frames);
    return 0;
}

// 设置音频设备
static int divoom_line_in_setup_audio(void)
{
#ifdef DIVOOM_LINE_IN_DISABLE_CAPTURE
    /* 关闭录音端测试播放：不打开采集，播放用 /tmp/linein_test.raw */
    pdivoom_line_in_ctrl->actual_sample_rate = LINE_IN_SAMPLE_RATE;
#else
    if (divoom_line_in_init_capture() < 0) 
	{
        return -1;
    }
#endif

#ifdef DIVOOM_LINE_IN_DISABLE_PLAYBACK
    /* 关闭播放端测试录音：不打开播放，采集保存到 /tmp/linein_cap.raw */
    return 0;
#endif

    if (divoom_line_in_init_playback() < 0) 
		{
#ifndef DIVOOM_LINE_IN_DISABLE_CAPTURE
        if (pdivoom_line_in_ctrl->capture_handle) 
		{
            snd_pcm_close(pdivoom_line_in_ctrl->capture_handle);
            pdivoom_line_in_ctrl->capture_handle = NULL;
        }
#endif
        return -1;
    }

    return 0;
}

/* DC 去除：略加强，减轻低频轰隆与嘈杂 */
#define LINE_IN_DC_COEF  0.0005f
/* 直通模式下仅做极轻 DC 去除，不碰频响，减轻杂音/底噪与喇叭削波 */
#define LINE_IN_DC_ONLY_COEF 0.0002f
#if (LINE_IN_SAMPLE_RATE == 44100)
/* 一阶高通约 120Hz、低通约 8kHz @44.1k，压嘈杂 */
#define LINE_IN_HP_ALPHA 0.9830f
#define LINE_IN_LP_ALPHA 0.35f
#else
/* 48k：高通约 120Hz、低通约 8kHz，压高频嘶声和低频轰隆 */
#define LINE_IN_HP_ALPHA 0.9843f
#define LINE_IN_LP_ALPHA 0.35f
#endif
static void divoom_line_in_remove_dc(int16_t *buffer, int frames)
{
    Pdivoom_line_in_ctrl ctrl = pdivoom_line_in_ctrl;
    if (!ctrl || !buffer) return;
    int total = frames * LINE_IN_CHANNELS;
    float dc_l = ctrl->dc_l, dc_r = ctrl->dc_r;
    float hx_l = ctrl->hp_x_l, hx_r = ctrl->hp_x_r, hy_l = ctrl->hp_y_l, hy_r = ctrl->hp_y_r;
    float a = LINE_IN_HP_ALPHA;
    int i;
    for (i = 0; i < total; i += 2) {
        float l = (float)buffer[i];
        dc_l += (l - dc_l) * LINE_IN_DC_COEF;
        l -= dc_l;
        hy_l = a * (hy_l + l - hx_l);
        hx_l = l;
        l = hy_l;
        if (i + 1 < total) {
            float r = (float)buffer[i + 1];
            dc_r += (r - dc_r) * LINE_IN_DC_COEF;
            r -= dc_r;
            hy_r = a * (hy_r + r - hx_r);
            hx_r = r;
            r = hy_r;
#ifdef DIVOOM_LINE_IN_SOFT_LPF
            { float ly = ctrl->lp_y_l, ry = ctrl->lp_y_r;
              float la = LINE_IN_LP_ALPHA;
              ly = la * ly + (1.f - la) * l; l = ly; ctrl->lp_y_l = ly;
              ry = la * ry + (1.f - la) * r; r = ry; ctrl->lp_y_r = ry; }
#endif
            buffer[i]     = (int16_t)(l > 32767.f ? 32767 : (l < -32768.f ? -32768 : l));
            buffer[i + 1] = (int16_t)(r > 32767.f ? 32767 : (r < -32768.f ? -32768 : r));
        } else {
#ifdef DIVOOM_LINE_IN_SOFT_LPF
            { float ly = ctrl->lp_y_l; ly = LINE_IN_LP_ALPHA * ly + (1.f - LINE_IN_LP_ALPHA) * l; l = ly; ctrl->lp_y_l = ly; }
#endif
            buffer[i] = (int16_t)(l > 32767.f ? 32767 : (l < -32768.f ? -32768 : l));
        }
    }
    ctrl->dc_l = dc_l; ctrl->dc_r = dc_r;
    ctrl->hp_x_l = hx_l; ctrl->hp_x_r = hx_r; ctrl->hp_y_l = hy_l; ctrl->hp_y_r = hy_r;
}

/* 直通模式下仅去除 DC 偏移（极慢跟踪），可选软低通压高频嘶声 */
static void divoom_line_in_remove_dc_only(int16_t *buffer, int frames)
{
    Pdivoom_line_in_ctrl ctrl = pdivoom_line_in_ctrl;
    if (!ctrl || !buffer) return;
    int total = frames * LINE_IN_CHANNELS;
    float dc_l = ctrl->dc_l, dc_r = ctrl->dc_r;
    const float coef = LINE_IN_DC_ONLY_COEF;
#ifdef DIVOOM_LINE_IN_SOFT_LPF
    float ly = ctrl->lp_y_l, ry = ctrl->lp_y_r;
    const float la = LINE_IN_LP_ALPHA;
#endif
    int i;
    for (i = 0; i < total; i += 2) {
        float l = (float)buffer[i];
        dc_l += (l - dc_l) * coef;
        l -= dc_l;
#ifdef DIVOOM_LINE_IN_SOFT_LPF
        ly = la * ly + (1.f - la) * l;
        l = ly;
#endif
        if (i + 1 < total) {
            float r = (float)buffer[i + 1];
            dc_r += (r - dc_r) * coef;
            r -= dc_r;
#ifdef DIVOOM_LINE_IN_SOFT_LPF
            ry = la * ry + (1.f - la) * r;
            r = ry;
#endif
            buffer[i]     = (int16_t)(l > 32767.f ? 32767 : (l < -32768.f ? -32768 : l));
            buffer[i + 1] = (int16_t)(r > 32767.f ? 32767 : (r < -32768.f ? -32768 : r));
        } else {
            buffer[i] = (int16_t)(l > 32767.f ? 32767 : (l < -32768.f ? -32768 : l));
        }
    }
    ctrl->dc_l = dc_l;
    ctrl->dc_r = dc_r;
#ifdef DIVOOM_LINE_IN_SOFT_LPF
    ctrl->lp_y_l = ly;
    ctrl->lp_y_r = ry;
#endif
}

// 应用音量调节到音频数据（可选 DC+高通+低通，再乘音量）
// 音量 100% 时完全不动数据，与 arecord|aplay 直通一致，保证声音干净
static void divoom_line_in_apply_volume(int16_t *buffer, int frames)
{
    if (!pdivoom_line_in_ctrl || !buffer) return;

#ifdef DIVOOM_LINE_IN_PASSTHROUGH
    /* 直通：仅做极轻 DC 去除（不碰频响），再音量+限幅，减轻杂音/颤音 */
    divoom_line_in_remove_dc_only(buffer, frames);
#else
    divoom_line_in_remove_dc(buffer, frames);
#endif

#ifdef DIVOOM_TIME_FRAME_SPEAKER
    /* 音量由 ALSA Soft Volume Master（default 播放链）统一调节 */
    return;
#endif

    if (pdivoom_line_in_ctrl->volume >= 100) {
        /* 100% 时仅 DC 已去，不再乘音量，声音最干净 */
        return;
    }

    int total_samples = frames * LINE_IN_CHANNELS;
    float volume_factor = (float)pdivoom_line_in_ctrl->volume / 100.0f;

    for (int i = 0; i < total_samples; i++) 
	{
        int32_t sample = (int32_t)(buffer[i] * volume_factor);
        /* 软限幅：接近满幅时压缩而非硬削波 */
        if (sample > 32700)      
			sample = 32700 + (sample - 32700) / 4;
        else if (sample < -32700) 
			sample = -32700 + (sample + 32700) / 4;
        if (sample > 32767) 
			sample = 32767;
        if (sample < -32768) 
			sample = -32768;
        buffer[i] = (int16_t)sample;
    }
}

/* 从 ring 中取出一块到 audio_buffer，返回帧数；无数据返回 0（调用前需已加锁） */
static unsigned int line_in_ring_pop_locked(Pdivoom_line_in_ctrl ctrl)
{
    const unsigned int n = LINE_IN_TRANSFER_FRAMES;
    const size_t bytes = n * 4;
    if (ctrl->ring_cnt < n) 
		return 0;
    if (ctrl->ring_rd + n <= LINE_IN_RING_FRAMES) 
	{
        memcpy(ctrl->audio_buffer, ctrl->ring_buf + ctrl->ring_rd * 4, bytes);
    } 
	else 
	{
        size_t part = (LINE_IN_RING_FRAMES - ctrl->ring_rd) * 4;
        memcpy(ctrl->audio_buffer, ctrl->ring_buf + ctrl->ring_rd * 4, part);
        memcpy((char *)ctrl->audio_buffer + part, ctrl->ring_buf, bytes - part);
    }
    ctrl->ring_rd = (ctrl->ring_rd + n) % LINE_IN_RING_FRAMES;
    ctrl->ring_cnt -= n;
    return n;
}

/* 将 audio_buffer 写入 ring，返回 1 成功，0 空间不足（调用前需已加锁） */
static int line_in_ring_push_locked(Pdivoom_line_in_ctrl ctrl)
{
    const unsigned int n = LINE_IN_TRANSFER_FRAMES;
    const size_t bytes = n * 4;
    if (ctrl->ring_cnt + n > LINE_IN_RING_FRAMES) 
		return 0;
    if (ctrl->ring_wr + n <= LINE_IN_RING_FRAMES) 
	{
        memcpy(ctrl->ring_buf + ctrl->ring_wr * 4, ctrl->audio_buffer, bytes);
    } 
	else 
	{
        size_t part = (LINE_IN_RING_FRAMES - ctrl->ring_wr) * 4;
        memcpy(ctrl->ring_buf + ctrl->ring_wr * 4, ctrl->audio_buffer, part);
        memcpy(ctrl->ring_buf, (char *)ctrl->audio_buffer + part, bytes - part);
    }
    ctrl->ring_wr = (ctrl->ring_wr + n) % LINE_IN_RING_FRAMES;
    ctrl->ring_cnt += n;
    return 1;
}

// 捕获线程：只负责 读 Line In -> 音量 -> 入 ring
static void* divoom_line_in_capture_task(void *param)
{
    int frames;
#ifdef __linux__
    struct sched_param sp;
#endif
    Pdivoom_line_in_ctrl ctrl = (Pdivoom_line_in_ctrl)param;

#ifdef __linux__
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = 79;  /* 略低于播放线程 80，与 UAC 类似 */
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) != 0)
        (void)0;
#endif
    divoom_print("Line In 捕获线程启动\n");
    while (ctrl && ctrl->run_flag)
    {
        if (ctrl->status != LINE_IN_STATUS_PLAYING) 
		{
            divoom_system_delay_ms(30);
            continue;
        }
        frames = snd_pcm_readi(ctrl->capture_handle, ctrl->audio_buffer, LINE_IN_TRANSFER_FRAMES);
        if (frames == (int)LINE_IN_TRANSFER_FRAMES) 
		{
            divoom_line_in_apply_volume((int16_t *)ctrl->audio_buffer, frames);
#if defined(DIVOOM_LINE_IN_DEBUG_SAVE_CAPTURE) || defined(DIVOOM_LINE_IN_DISABLE_PLAYBACK)
            {
                static int cap_fd = -1;
                static unsigned int cap_bytes = 0;
#if defined(DIVOOM_LINE_IN_DISABLE_PLAYBACK)
                #define LINE_IN_CAP_SAVE_MAX_BYTES (48000U * 2U * 2U * 10U)  /* 10s when playback disabled */
#else
                #define LINE_IN_CAP_SAVE_MAX_BYTES (48000U * 2U * 2U * 5U)   /* 5s debug */
#endif
                if (cap_fd < 0) {
                    cap_fd = open(DIVOOM_TMP_DIR "/linein_cap.raw", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    cap_bytes = 0;
                    if (cap_fd >= 0)
                        divoom_print("Line In: saving capture to /tmp/linein_cap.raw (playback disabled for capture test)\n");
                }
                if (cap_fd >= 0 && cap_bytes < LINE_IN_CAP_SAVE_MAX_BYTES) {
                    size_t to_write = (size_t)(LINE_IN_TRANSFER_FRAMES * 4);
                    if (cap_bytes + to_write > LINE_IN_CAP_SAVE_MAX_BYTES)
                        to_write = LINE_IN_CAP_SAVE_MAX_BYTES - cap_bytes;
                    if (write(cap_fd, ctrl->audio_buffer, to_write) == (ssize_t)to_write)
                        cap_bytes += (unsigned int)to_write;
                    if (cap_bytes >= LINE_IN_CAP_SAVE_MAX_BYTES) {
                        close(cap_fd);
                        cap_fd = -1;
                        divoom_print("Line In: saved. Run: aplay -f S16_LE -r 48000 -c 2 -t raw /tmp/linein_cap.raw\n");
                    }
                }
            }
#endif
#ifndef DIVOOM_LINE_IN_DISABLE_PLAYBACK
            if (ctrl->lock_handle) divoom_system_get_lock(ctrl->lock_handle);
            line_in_ring_push_locked(ctrl);
            if (ctrl->lock_handle) divoom_system_free_lock(ctrl->lock_handle);
#endif
        } 
		else if (frames < 0 && frames != -EAGAIN) 
		{
            if (snd_pcm_recover(ctrl->capture_handle, frames, 0) < 0)
                divoom_system_delay_ms(20);  /* 缩短恢复等待，减少丢帧导致的播放颤音 */
        }
        if (ctrl->ring_cnt + LINE_IN_TRANSFER_FRAMES > LINE_IN_RING_FRAMES)
            divoom_system_delay_ms(5);
    }
    divoom_print("Line In 捕获线程退出\n");
    return NULL;
}

// 播放线程：有数据就写设备（不依赖 poll，避免 default 链下不返回 POLLOUT 导致静音），高优先级减 underrun
static void* divoom_line_in_playback_task(void *param)
{
    int err;
    unsigned int n, cnt;
    int i, wrote_any;
    static int prefilled = 0;
#if defined(DIVOOM_LINE_IN_DEBUG_PLAY_FILE) || defined(DIVOOM_LINE_IN_DISABLE_CAPTURE)
    static FILE *debug_play_fp = NULL;
#endif
#ifdef __linux__
    struct sched_param sp;
#endif
    Pdivoom_line_in_ctrl ctrl = (Pdivoom_line_in_ctrl)param;

#ifdef __linux__
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = 85;  /* 高于 Wi-Fi/主循环，减轻 underrun */
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) != 0)
        (void)0;
#endif
    divoom_print("Line In 播放线程启动\n");
    while (ctrl && ctrl->run_flag)
    {
        if (ctrl->status != LINE_IN_STATUS_PLAYING) 
		{
            prefilled = 0;
            divoom_system_delay_ms(30);
            continue;
        }
        if (!prefilled) 
		{
#if defined(DIVOOM_LINE_IN_DEBUG_PLAY_FILE) || defined(DIVOOM_LINE_IN_DISABLE_CAPTURE)
            {
                unsigned int to_fill;
                if (!debug_play_fp)
                    debug_play_fp = divoom_port_file_open(DIVOOM_TMP_DIR "/linein_test.raw", "rb");
                if (debug_play_fp) {
                    divoom_print("Line In: playing from /tmp/linein_test.raw (capture disabled for playback test)\n");
                    for (to_fill = LINE_IN_PREFILL_DEVICE_FRAMES; to_fill >= LINE_IN_TRANSFER_FRAMES && ctrl->run_flag; ) {
                        size_t nr = divoom_port_file_read(ctrl->audio_buffer, 1, LINE_IN_TRANSFER_FRAMES * 4, debug_play_fp);
                        if (nr < (size_t)(LINE_IN_TRANSFER_FRAMES * 4)) rewind(debug_play_fp);
                        n = (nr / 4);
                        if (n > 0) {
                            err = snd_pcm_writei(ctrl->playback_handle, ctrl->audio_buffer, n);
                            if (err == (int)n) to_fill -= n;
                        }
                    }
                } else
                    divoom_print("Line In debug: open /tmp/linein_test.raw failed, put S16_LE 48k stereo raw there\n");
            }
#else
            /* 等 ring 攒够再预填，预填时一次取 8192 不依赖捕获实时跟，预填后 ring 仍剩 2048 */
            while (ctrl->run_flag && ctrl->status == LINE_IN_STATUS_PLAYING) 
			{
                if (ctrl->lock_handle) divoom_system_get_lock(ctrl->lock_handle);
                cnt = ctrl->ring_cnt;
                if (ctrl->lock_handle) divoom_system_free_lock(ctrl->lock_handle);
                if (cnt >= LINE_IN_PREFILL_RING) break;
                divoom_system_delay_ms(10);
            }
            {
                unsigned int to_fill = LINE_IN_PREFILL_DEVICE_FRAMES;
                while (ctrl->run_flag && ctrl->status == LINE_IN_STATUS_PLAYING && to_fill >= LINE_IN_TRANSFER_FRAMES)
                {
                    if (ctrl->lock_handle) divoom_system_get_lock(ctrl->lock_handle);
                    n = (ctrl->ring_cnt >= LINE_IN_TRANSFER_FRAMES) ? line_in_ring_pop_locked(ctrl) : 0;
                    if (ctrl->lock_handle) divoom_system_free_lock(ctrl->lock_handle);
                    if (n == 0) { divoom_system_delay_ms(5); continue; }
                    err = snd_pcm_writei(ctrl->playback_handle, ctrl->audio_buffer, n);
                    if (err == (int)n) to_fill -= n;
                    else if (err == -EPIPE || err == -ESTRPIPE) { (void)snd_pcm_recover(ctrl->playback_handle, err, 0); snd_pcm_prepare(ctrl->playback_handle); }
                }
            }
#endif
            prefilled = 1;
        }
        if (!ctrl->run_flag) break;

#if defined(DIVOOM_LINE_IN_DEBUG_PLAY_FILE) || defined(DIVOOM_LINE_IN_DISABLE_CAPTURE)
        if (debug_play_fp) {
                size_t nr = divoom_port_file_read(ctrl->audio_buffer, 1, LINE_IN_TRANSFER_FRAMES * 4, debug_play_fp);
                if (nr < (size_t)(LINE_IN_TRANSFER_FRAMES * 4)) rewind(debug_play_fp);
                n = (nr / 4);
                if (n > 0) {
                    err = snd_pcm_writei(ctrl->playback_handle, ctrl->audio_buffer, n);
                    if (err < 0 && err != -EAGAIN)
                    (void)snd_pcm_recover(ctrl->playback_handle, err, 0);
            }
            divoom_system_delay_ms(5);
            continue;
        }
#endif

        /* ring 有数据就取数写设备；不依赖 poll(POLLOUT)，部分 default 链下 poll 不返回会导致“播一下就没声” */
        if (ctrl->lock_handle) divoom_system_get_lock(ctrl->lock_handle);
        cnt = ctrl->ring_cnt;
        if (ctrl->lock_handle) divoom_system_free_lock(ctrl->lock_handle);
        if (cnt < LINE_IN_WRITE_THRESHOLD) {
            divoom_system_delay_ms(5);
            continue;
        }

        wrote_any = 0;
        for (i = 0; i < LINE_IN_PLAYBACK_BURST; i++) 
		{
            n = 0;
            if (ctrl->lock_handle) divoom_system_get_lock(ctrl->lock_handle);
            /* 有一块就取去写，保证持续喂设备（实际 default 链 buffer 仅 7680，等 2048 易播完就静音） */
            if (ctrl->ring_cnt >= LINE_IN_TRANSFER_FRAMES)
                n = line_in_ring_pop_locked(ctrl);
            if (ctrl->lock_handle) divoom_system_free_lock(ctrl->lock_handle);
            if (n == 0) break;
            wrote_any = 1;
            err = snd_pcm_writei(ctrl->playback_handle, ctrl->audio_buffer, n);
            if (err < 0 && err != -EAGAIN) 
			{
                if (snd_pcm_recover(ctrl->playback_handle, err, 0) == 0) 
				{
                    snd_pcm_prepare(ctrl->playback_handle);
                    /* 只补 1 块静音(1024 帧)，尽快恢复，减少颤音感 */
                    (void)snd_pcm_writei(ctrl->playback_handle, line_in_silence_buf, LINE_IN_SILENCE_FRAMES_AFTER_XRUN);
                }
                else
                {
                    /* 限速打印，避免 underrun 刷屏日志 */
                    static unsigned int xrun_log_count;
                    if ((xrun_log_count++ % 32) == 0)
                        divoom_print("Line In 播放 xrun: %s (已限速打印)\n", snd_strerror(err));
                }
            }
        }
        if (!wrote_any)
            divoom_system_delay_ms(5);
    }
    divoom_print("Line In 播放线程退出\n");
    return NULL;
}

// 清理资源
static void divoom_line_in_cleanup(void)
{
    if (!pdivoom_line_in_ctrl) 
	{
        return;
    }

#if defined(DIVOOM_LINE_IN_USE_PIPE_CMD) && DIVOOM_LINE_IN_USE_PIPE_CMD
    if (line_in_pipe_pid > 0) 
	{
        kill(-line_in_pipe_pid, SIGTERM);
        waitpid(line_in_pipe_pid, NULL, 0);
        if (line_in_pipe_pid_aplay > 0) waitpid(line_in_pipe_pid_aplay, NULL, 0);
        pthread_join(line_in_pipe_thread, NULL);
        line_in_pipe_pid = line_in_pipe_pid_aplay = -1;
        divoom_print("Line In 管道已结束\n");
    }
#endif

    if (pdivoom_line_in_ctrl->capture_handle) 
	{
        snd_pcm_drain(pdivoom_line_in_ctrl->capture_handle);
        snd_pcm_close(pdivoom_line_in_ctrl->capture_handle);
        pdivoom_line_in_ctrl->capture_handle = NULL;
    }

    if (pdivoom_line_in_ctrl->playback_handle) 
	{
        snd_pcm_drain(pdivoom_line_in_ctrl->playback_handle);
        snd_pcm_close(pdivoom_line_in_ctrl->playback_handle);
        pdivoom_line_in_ctrl->playback_handle = NULL;
    }
    if (pdivoom_line_in_ctrl->lock_handle) 
	{
        divoom_system_destroy_lock(pdivoom_line_in_ctrl->lock_handle);
        pdivoom_line_in_ctrl->lock_handle = NULL;
    }

    divoom_system_free(pdivoom_line_in_ctrl);
    pdivoom_line_in_ctrl = NULL;
}

// 初始化 Line In 模块
void divoom_line_in_init(void)
{
    if (pdivoom_line_in_ctrl) 
	{
        divoom_print("Line In 模块已初始化\n");
        return;
    }

    pdivoom_line_in_ctrl = divoom_system_malloc(sizeof(divoom_line_in_ctrl));
    if (!pdivoom_line_in_ctrl) 
	{
        divoom_print("分配 Line In 控制结构失败\n");
        return;
    }

    // 初始化结构体
    memset(pdivoom_line_in_ctrl, 0, sizeof(divoom_line_in_ctrl));
    pdivoom_line_in_ctrl->status = LINE_IN_STATUS_IDLE;
    pdivoom_line_in_ctrl->volume = 100; /* 默认 100% 直通，与 arecord|aplay 一致、声音最干净；需降低可调 divoom_line_in_set_volume */
    pdivoom_line_in_ctrl->run_flag = 1;
    pdivoom_line_in_ctrl->lock_handle = divoom_system_create_lock();
    if (!pdivoom_line_in_ctrl->lock_handle) 
	{
        divoom_print("Line In 创建锁失败\n");
        divoom_line_in_cleanup();
        return;
    }

#if defined(DIVOOM_LINE_IN_USE_PIPE_CMD) && DIVOOM_LINE_IN_USE_PIPE_CMD
    /* 管道模式：不打开 ALSA，不启线程，start 时再 fork arecord|aplay */
    divoom_print("Line In 模块初始化成功（管道模式，与命令行一致）\n");
#else
    // 初始化音频设备
    if (divoom_line_in_setup_audio() < 0) 
	{
        divoom_print("Line In 音频设备初始化失败\n");
        divoom_line_in_cleanup();
        return;
    }

    divoom_print("Line In 模块初始化成功\n");
#ifdef DIVOOM_LINE_IN_DISABLE_CAPTURE
    /* 关闭录音端：只启播放线程，播 /tmp/linein_test.raw */
    pdivoom_line_in_ctrl->playback_task_handle = divoom_system_create_task(divoom_line_in_playback_task, "linein_play", pdivoom_line_in_ctrl, DIVOOM_TASK_STACK_LINE_IN_PLAY);
#elif defined(DIVOOM_LINE_IN_DISABLE_PLAYBACK)
    /* 关闭播放端：只启采集线程，保存到 /tmp/linein_cap.raw */
    pdivoom_line_in_ctrl->capture_task_handle = divoom_system_create_task(divoom_line_in_capture_task, "linein_cap", pdivoom_line_in_ctrl, DIVOOM_TASK_STACK_LINE_IN_CAPTURE);
#else
    // 双线程：捕获线程 + 播放线程，通过 ring 解耦
    pdivoom_line_in_ctrl->capture_task_handle = divoom_system_create_task(divoom_line_in_capture_task, "linein_cap", pdivoom_line_in_ctrl, DIVOOM_TASK_STACK_LINE_IN_CAPTURE);
    pdivoom_line_in_ctrl->playback_task_handle = divoom_system_create_task(divoom_line_in_playback_task, "linein_play", pdivoom_line_in_ctrl, DIVOOM_TASK_STACK_LINE_IN_PLAY);
#endif
#endif
}

// 是否已成功初始化
int divoom_line_in_is_initialized(void)
{
    return (pdivoom_line_in_ctrl != NULL) ? 1 : 0;
}

// 反初始化 Line In 模块
void divoom_line_in_deinit(void)
{
    if (!pdivoom_line_in_ctrl) 
	{
        return;
    }

    // 停止任务
    pdivoom_line_in_ctrl->run_flag = 0;
    pdivoom_line_in_ctrl->status = LINE_IN_STATUS_STOPPED;

    // 等待任务退出
    divoom_system_delay_ms(200);

    // 清理资源
    divoom_line_in_cleanup();

    divoom_print("Line In 模块反初始化完成\n");
}

// 开始 Line In 播放
void divoom_line_in_start(void)
{
    if (!pdivoom_line_in_ctrl) 
	{
        divoom_print("Line In 模块未初始化\n");
        return;
    }

    if (pdivoom_line_in_ctrl->status == LINE_IN_STATUS_PLAYING) 
	{
        divoom_print("Line In 已在播放中\n");
        return;
    }

#if defined(DIVOOM_LINE_IN_USE_PIPE_CMD) && DIVOOM_LINE_IN_USE_PIPE_CMD
    if (line_in_pipe_pid <= 0) 
	{
        int pipe_ar[2], pipe_ap[2];
        pid_t pid_ar, pid_ap;
        if (pipe(pipe_ar) < 0 || pipe(pipe_ap) < 0) 
		{
            divoom_print("Line In 管道 pipe() 失败\n");
            return;
        }
        pid_ar = fork();
        if (pid_ar == 0) 
		{
            dup2(pipe_ar[1], STDOUT_FILENO);
            close(pipe_ar[0]); 
			close(pipe_ar[1]); 
			close(pipe_ap[0]); 
			close(pipe_ap[1]);
            setpgid(0, 0);
            execlp("arecord", "arecord", "-D", "default", "-f", "S16_LE", "-r", "48000", "-c", "2", "-t", "raw", (char *)NULL);
            _exit(127);
        }
        if (pid_ar < 0) 
		{ 
			close(pipe_ar[0]); 
			close(pipe_ar[1]); 
			close(pipe_ap[0]); 
			close(pipe_ap[1]); 
			divoom_print("Line In arecord fork 失败\n"); 
			return; 
		}
        close(pipe_ar[1]);
        pid_ap = fork();
        if (pid_ap == 0) 
		{
            dup2(pipe_ap[0], STDIN_FILENO);
            close(pipe_ap[0]); 
			close(pipe_ap[1]); 
			close(pipe_ar[0]);
            execlp("aplay", "aplay", "-D", "default", "-f", "S16_LE", "-r", "48000", "-c", "2", "-t", "raw", (char *)NULL);
            _exit(127);
        }
        if (pid_ap < 0) 
		{ 
			kill(pid_ar, SIGKILL); 
			waitpid(pid_ar, NULL, 0); 
			close(pipe_ar[0]); 
			close(pipe_ap[0]); 
			close(pipe_ap[1]); 
			divoom_print("Line In aplay fork 失败\n"); 
			return; 
		}
        setpgid(pid_ap, pid_ar);
        close(pipe_ap[0]);
        line_in_pipe_pid = pid_ar;
        line_in_pipe_pid_aplay = pid_ap;
        line_in_pipe_read_fd = pipe_ar[0];
        line_in_pipe_write_fd = pipe_ap[1];
        if (pthread_create(&line_in_pipe_thread, NULL, line_in_pipe_relay_thread, NULL) != 0) 
		{
            close(line_in_pipe_read_fd); 
			close(line_in_pipe_write_fd);
            line_in_pipe_read_fd = line_in_pipe_write_fd = -1;
            kill(-line_in_pipe_pid, SIGTERM); 
			waitpid(line_in_pipe_pid, NULL, 0); 
			waitpid(line_in_pipe_pid_aplay, NULL, 0);
            line_in_pipe_pid = line_in_pipe_pid_aplay = -1;
            divoom_print("Line In 管道 relay 线程创建失败\n");
            return;
        }
        divoom_print("Line In 管道已启动(软件音量) arecord=%d aplay=%d\n", (int)pid_ar, (int)pid_ap);
    }
#else
    /* 进程内 ALSA：播提示音前 stop 会释放设备，此处若设备未打开则重新打开 */
    if (pdivoom_line_in_ctrl->capture_handle == NULL)
    {
        if (divoom_line_in_setup_audio() < 0)
        {
            divoom_print("Line In 重新打开设备失败\n");
            return;
        }
    }
#endif

    pdivoom_line_in_ctrl->status = LINE_IN_STATUS_PLAYING;
    divoom_print("Line In 开始播放\n");
}

// 停止 Line In 播放
void divoom_line_in_stop(void)
{
    if (!pdivoom_line_in_ctrl) 
	{
        divoom_print("Line In 模块未初始化\n");
        return;
    }

#if defined(DIVOOM_LINE_IN_USE_PIPE_CMD) && DIVOOM_LINE_IN_USE_PIPE_CMD
    if (line_in_pipe_pid > 0) 
	{
        kill(-line_in_pipe_pid, SIGTERM);
        waitpid(line_in_pipe_pid, NULL, 0);
        if (line_in_pipe_pid_aplay > 0) 
			waitpid(line_in_pipe_pid_aplay, NULL, 0);
        pthread_join(line_in_pipe_thread, NULL);
        line_in_pipe_pid = -1;
        line_in_pipe_pid_aplay = -1;
        divoom_print("Line In 管道已停止\n");
    }
#else
    /* 进程内 ALSA：释放设备供提示音 TPlayer 使用，下次 start 会重新打开 */
    pdivoom_line_in_ctrl->status = LINE_IN_STATUS_STOPPED;
    divoom_system_delay_ms(120);
    if (pdivoom_line_in_ctrl->capture_handle)
    {
        snd_pcm_drain(pdivoom_line_in_ctrl->capture_handle);
        snd_pcm_close(pdivoom_line_in_ctrl->capture_handle);
        pdivoom_line_in_ctrl->capture_handle = NULL;
    }
    if (pdivoom_line_in_ctrl->playback_handle)
    {
        snd_pcm_drain(pdivoom_line_in_ctrl->playback_handle);
        snd_pcm_close(pdivoom_line_in_ctrl->playback_handle);
        pdivoom_line_in_ctrl->playback_handle = NULL;
    }
    divoom_print("Line In 已释放设备（可播提示音后再次 start 恢复）\n");
#endif

    pdivoom_line_in_ctrl->status = LINE_IN_STATUS_STOPPED;
    divoom_print("Line In 停止播放\n");
}

// 暂停 Line In 播放
void divoom_line_in_pause(void)
{
    if (!pdivoom_line_in_ctrl) 
	{
        divoom_print("Line In 模块未初始化\n");
        return;
    }

    if (pdivoom_line_in_ctrl->status != LINE_IN_STATUS_PLAYING) 
	{
        return;
    }

    pdivoom_line_in_ctrl->status = LINE_IN_STATUS_PAUSED;
    divoom_print("Line In 暂停播放\n");
}

// 恢复 Line In 播放
void divoom_line_in_resume(void)
{
    if (!pdivoom_line_in_ctrl) 
	{
        divoom_print("Line In 模块未初始化\n");
        return;
    }

    if (pdivoom_line_in_ctrl->status != LINE_IN_STATUS_PAUSED) 
	{
        return;
    }

    pdivoom_line_in_ctrl->status = LINE_IN_STATUS_PLAYING;
    divoom_print("Line In 恢复播放\n");
}

// 设置 Line In 音量 (0-100)
void divoom_line_in_set_volume(uint8 volume)
{
    if (!pdivoom_line_in_ctrl) 
	{
        divoom_print("Line In 模块未初始化\n");
        return;
    }

    if (volume > 100) 
	{
        volume = 100;
    }

    pdivoom_line_in_ctrl->volume = volume;
    divoom_print("Line In 音量设置为: %d\n", volume);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
    divoom_main_set_sys_vol((int)volume);
#endif
}

// 获取 Line In 音量
uint8 divoom_line_in_get_volume(void)
{
    if (!pdivoom_line_in_ctrl) 
	{
        return 0;
    }

    return pdivoom_line_in_ctrl->volume;
}

// 获取当前播放状态
LineInStatus divoom_line_in_get_status(void)
{
    if (!pdivoom_line_in_ctrl) 
	{
        return LINE_IN_STATUS_IDLE;
    }

    return pdivoom_line_in_ctrl->status;
}

