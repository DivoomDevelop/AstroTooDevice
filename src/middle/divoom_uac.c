#include "divoom.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include "divoom_uac.h"
#include "divoom_board_io_config.h"
// UAC 音频设备：采集为 UAC Gadget；播放用 default 走 asound 链(snddaudio2->I2S)，保证 I2S 有数据输出
#define DIVOOM_UAC_CAPTURE_DEVICE  "hw:UAC1Gadget,0"
//#define DIVOOM_UAC_PLAYBACK_DEVICE "hw:snddaudio2,0"
#define UAC_PCM_CAPTURE_DEVICE     DIVOOM_UAC_CAPTURE_DEVICE
// 播放走 default 链(snddaudio2)，与 asound 一致
#define UAC_PCM_PLAYBACK_DEVICE    "default"

//#define DIVOOM_UAC_MODE

#define UAC_SAMPLE_RATE             48000           // 采样率 (UAC 通常支持 48kHz)
#define UAC_CHANNELS                2               // 通道数
#define UAC_BUFFER_SIZE             8192            // 设备缓冲帧数
#define UAC_PERIOD_SIZE             1024            // 每周期帧数
#define UAC_RING_PERIODS            10              // 环形缓冲周期数，吸收 USB 抖动
#define UAC_RING_FRAMES             (UAC_RING_PERIODS * UAC_PERIOD_SIZE)

// UAC 控制结构体（无设备/信号检测，直接使用配置的捕获/播放设备）
typedef struct _divoom_uac_ctrl {
    snd_pcm_t *capture_handle;              // 音频捕获句柄
    snd_pcm_t *playback_handle;             // 音频播放句柄
    uint8 audio_buffer[UAC_BUFFER_SIZE * 4];  // 单次读/写缓冲
    uint8 ring_buf[UAC_RING_FRAMES * 4];   // 环形缓冲：捕获->ring->播放，解耦阻塞
    uint8 capture_buf[UAC_PERIOD_SIZE * 4]; // 捕获线程专用缓冲，避免与播放争用
    unsigned int ring_rd;                    // 读位置（帧下标）
    unsigned int ring_wr;                    // 写位置（帧下标）
    unsigned int ring_cnt;                   // 当前帧数
    UacStatus status;                        // 当前状态
    uint8 volume;                            // 音量 (0-100)
    int run_flag;                            // 捕获线程循环标志，主任务退出前清 0 以便 join 捕获
    volatile int main_task_run;              // UAC 主任务 while 条件，stop/deinit 清 0 后 join
    int main_task_created;                   // 主任务已 pthread_create 且尚未 join
    void *lock_handle;                       // 锁句柄
    pthread_t task_handle;                   // 主任务 pthread（可 join，不用 divoom_system_create_new_task）
    pthread_t capture_tid;                   // 捕获线程 id（仅填 ring，不阻塞播放）
    int capture_task_started;               // 是否已创建捕获线程
} divoom_uac_ctrl, *Pdivoom_uac_ctrl;

static Pdivoom_uac_ctrl pdivoom_uac_ctrl = NULL;

// 前向声明
static void divoom_uac_cleanup(void);
static int divoom_uac_setup_audio(void);
/* 切模式/停播时打断 ALSA 阻塞，避免 pthread_join 等数秒（readi/writei 未返回则看不到 run_flag/main_task_run） */
static void divoom_uac_pcm_drop_handles(void)
{
	if (!pdivoom_uac_ctrl)
		return;
	if (pdivoom_uac_ctrl->playback_handle)
		(void)snd_pcm_drop(pdivoom_uac_ctrl->playback_handle);
	if (pdivoom_uac_ctrl->capture_handle)
		(void)snd_pcm_drop(pdivoom_uac_ctrl->capture_handle);
}

#ifdef DIVOOM_UAC_MODE
/* setusbconfig 需同步跑完 */
static void divoom_uac_run_setusbconfig(const char *arg)
{
	if (!arg || !*arg)
		return;
	divoom_print("UAC: setusbconfig %s\n", arg);
	divoom_port_hw_usb_set_config(arg);
}
#endif

#define UAC_READY_WAIT_TIMEOUT_MS  10000   /* UAC 就绪等待超时 10s */
#define UAC_READY_POLL_INTERVAL_MS 200     /* 轮询间隔 200ms */

/* 检测 UAC 捕获设备节点是否已出现（可打开即视为就绪） */
static int divoom_uac_is_device_ready(void)
{
	snd_pcm_t *test_handle = NULL;
	int err;

	err = snd_pcm_open(&test_handle, UAC_PCM_CAPTURE_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
	if (err == 0 && test_handle) 
	{
		snd_pcm_close(test_handle);
		return 1;
	}
	return 0;
}

/*********************************************************************
  Function:        divoom_uac_init_capture
  Description:     初始化 UAC 捕获设备
  Input:           none
  Return:          0: 成功  -1: 失败
**********************************************************************/
static int divoom_uac_init_capture(void)
{
    int err;
    snd_pcm_hw_params_t *hw_params = NULL;
    unsigned int rate = UAC_SAMPLE_RATE;
    char device_name[32] = {0};

    if (!pdivoom_uac_ctrl) 
	{
        return -1;
    }

    // 使用配置的捕获设备(默认 hw:UAC1Gadget,0)，与 arecord -D hw:UAC1Gadget 一致
    snprintf(device_name, sizeof(device_name), "%s", UAC_PCM_CAPTURE_DEVICE);

    // 打开捕获设备
    err = snd_pcm_open(&pdivoom_uac_ctrl->capture_handle, device_name, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) 
	{
        divoom_print("UAC: 无法打开捕获设备 %s: %s\n",    device_name, snd_strerror(err));
        return -1;
    }

    // 分配硬件参数结构
    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(pdivoom_uac_ctrl->capture_handle, hw_params);

    // 设置硬件参数（不强制 period/buffer，用驱动默认，避免 USB 侧阻塞加重卡顿）
    snd_pcm_hw_params_set_access(pdivoom_uac_ctrl->capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pdivoom_uac_ctrl->capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pdivoom_uac_ctrl->capture_handle, hw_params, UAC_CHANNELS);
    snd_pcm_hw_params_set_rate_near(pdivoom_uac_ctrl->capture_handle, hw_params, &rate, 0);

    // 应用硬件参数
    err = snd_pcm_hw_params(pdivoom_uac_ctrl->capture_handle, hw_params);
    if (err < 0) 
	{
        divoom_print("UAC: 无法设置捕获硬件参数: %s\n", snd_strerror(err));
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pdivoom_uac_ctrl->capture_handle);
        pdivoom_uac_ctrl->capture_handle = NULL;
        return -1;
    }

    snd_pcm_hw_params_free(hw_params);
    snd_pcm_prepare(pdivoom_uac_ctrl->capture_handle);

    divoom_print("UAC: 捕获设备 %s 初始化成功\n", device_name);
    return 0;
}

/*********************************************************************
  Function:        divoom_uac_init_playback
  Description:     初始化本地播放设备
  Input:           none
  Return:          0: 成功  -1: 失败
**********************************************************************/
static int divoom_uac_init_playback(void)
{
    int err;
    snd_pcm_hw_params_t *hw_params = NULL;
    unsigned int rate = UAC_SAMPLE_RATE;
    snd_pcm_uframes_t period_frames = UAC_PERIOD_SIZE;
    snd_pcm_uframes_t buffer_frames = UAC_BUFFER_SIZE;

    if (!pdivoom_uac_ctrl) 
	{
        return -1;
    }

    // 打开播放设备
    err = snd_pcm_open(&pdivoom_uac_ctrl->playback_handle, UAC_PCM_PLAYBACK_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) 
	{
        divoom_print("UAC: 无法打开播放设备 %s: %s\n",    UAC_PCM_PLAYBACK_DEVICE, snd_strerror(err));
        return -1;
    }

    // 分配硬件参数结构
    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(pdivoom_uac_ctrl->playback_handle, hw_params);

    // 设置硬件参数
    snd_pcm_hw_params_set_access(pdivoom_uac_ctrl->playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pdivoom_uac_ctrl->playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pdivoom_uac_ctrl->playback_handle, hw_params, UAC_CHANNELS);
    snd_pcm_hw_params_set_rate_near(pdivoom_uac_ctrl->playback_handle, hw_params, &rate, 0);
    snd_pcm_hw_params_set_period_size_near(pdivoom_uac_ctrl->playback_handle, hw_params, &period_frames, 0);
    snd_pcm_hw_params_set_buffer_size_near(pdivoom_uac_ctrl->playback_handle, hw_params, &buffer_frames);

    // 应用硬件参数
    err = snd_pcm_hw_params(pdivoom_uac_ctrl->playback_handle, hw_params);
    if (err < 0) 
	{
        divoom_print("UAC: 无法设置播放硬件参数: %s\n", snd_strerror(err));
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pdivoom_uac_ctrl->playback_handle);
        pdivoom_uac_ctrl->playback_handle = NULL;
        return -1;
    }

    snd_pcm_hw_params_free(hw_params);
    snd_pcm_prepare(pdivoom_uac_ctrl->playback_handle);

    divoom_print("UAC: 播放设备初始化成功\n");
    return 0;
}

/*********************************************************************
  Function:        divoom_uac_setup_audio
  Description:     设置音频设备
  Input:           none
  Return:          0: 成功  -1: 失败
**********************************************************************/
static int divoom_uac_setup_audio(void)
{
    if (divoom_uac_init_capture() < 0) 
	{
        return -1;
    }

    if (divoom_uac_init_playback() < 0) 
	{
        if (pdivoom_uac_ctrl->capture_handle) 
		{
            snd_pcm_close(pdivoom_uac_ctrl->capture_handle);
            pdivoom_uac_ctrl->capture_handle = NULL;
        }
        return -1;
    }

    pdivoom_uac_ctrl->ring_rd = 0;
    pdivoom_uac_ctrl->ring_wr = 0;
    pdivoom_uac_ctrl->ring_cnt = 0;
    /* 预填 2 周期静音，避免开播瞬间 ring 空导致首包 underrun */
    memset(pdivoom_uac_ctrl->ring_buf, 0, UAC_PERIOD_SIZE * 4 * 2);
    pdivoom_uac_ctrl->ring_wr = UAC_PERIOD_SIZE * 2;
    pdivoom_uac_ctrl->ring_cnt = UAC_PERIOD_SIZE * 2;
    return 0;
}

/*********************************************************************
  Function:        divoom_uac_close_audio
  Description:     关闭音频设备
  Input:           none
  Return:          none
**********************************************************************/
static void divoom_uac_close_audio(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        return;
    }

    if (pdivoom_uac_ctrl->capture_handle) 
	{
        (void)snd_pcm_drop(pdivoom_uac_ctrl->capture_handle);
        snd_pcm_close(pdivoom_uac_ctrl->capture_handle);
        pdivoom_uac_ctrl->capture_handle = NULL;
    }

    if (pdivoom_uac_ctrl->playback_handle) 
	{
        (void)snd_pcm_drop(pdivoom_uac_ctrl->playback_handle);
        snd_pcm_close(pdivoom_uac_ctrl->playback_handle);
        pdivoom_uac_ctrl->playback_handle = NULL;
    }
}

/*********************************************************************
  Function:        divoom_uac_apply_volume
  Description:     应用音量调节到音频数据
  Input:           buffer - 音频数据缓冲区
                   frames - 帧数
  Return:          none
**********************************************************************/
static void divoom_uac_apply_volume(int16_t *buffer, int frames)
{
    if (!pdivoom_uac_ctrl || !buffer) 
	{
        return;
    }

    int total_samples = frames * UAC_CHANNELS;
    float volume_factor = (float)pdivoom_uac_ctrl->volume / 100.0f;

    for (int i = 0; i < total_samples; i++) 
	{
        int32_t sample = (int32_t)(buffer[i] * volume_factor);
        // 限幅处理
        if (sample > 32767) 
			sample = 32767;
        if (sample < -32768) 
			sample = -32768;
        buffer[i] = (int16_t)sample;
    }
}

/*********************************************************************
  Function:        divoom_uac_capture_task
  Description:     仅负责从 UAC 读入并写入 ring，不阻塞播放线程
  Input:           param - pdivoom_uac_ctrl
  Return:          none
**********************************************************************/
static void* divoom_uac_capture_task(void *param)
{
    Pdivoom_uac_ctrl ctrl = (Pdivoom_uac_ctrl)param;
    int n;

    while (ctrl && ctrl->run_flag) 
	{
		// 检查播放状态
        if (ctrl->status != UAC_STATUS_PLAYING) 
		{
            divoom_system_delay_ms(50);
            continue;
        }

        if (!ctrl->capture_handle) 
		{
            divoom_port_delay_ms((5000) / 1000U);
            continue;
        }
        if (ctrl->ring_cnt > UAC_RING_FRAMES - UAC_PERIOD_SIZE) 
		{
            divoom_port_delay_ms((2000) / 1000U);
            continue;
        }
        n = snd_pcm_readi(ctrl->capture_handle, ctrl->capture_buf, UAC_PERIOD_SIZE);
        if (n >= (int)UAC_PERIOD_SIZE) 
		{
            if (ctrl->lock_handle) 
				divoom_system_get_lock(ctrl->lock_handle);
            if (ctrl->ring_cnt <= UAC_RING_FRAMES - UAC_PERIOD_SIZE) 
			{
                unsigned int wr = ctrl->ring_wr;
                memcpy(ctrl->ring_buf + wr * 4, ctrl->capture_buf, UAC_PERIOD_SIZE * 4);
                ctrl->ring_wr = (wr + UAC_PERIOD_SIZE) % UAC_RING_FRAMES;
                ctrl->ring_cnt += UAC_PERIOD_SIZE;
            }
            if (ctrl->lock_handle) 
				divoom_system_free_lock(ctrl->lock_handle);
        } 
		else if (n == -EPIPE) 
		{
            snd_pcm_prepare(ctrl->capture_handle);
        } 
		else if (n == -ENODEV || n == -ESTRPIPE) 
		{
            break;
        } 
		else if (n != -EAGAIN && n < 0) 
		{
            divoom_port_delay_ms((10000) / 1000U);
        }
    }
    return NULL;
}

/*********************************************************************
  Function:        divoom_uac_main_task
  Description:     UAC 主任务线程（只负责从 ring 取数据写播放，保证不因捕获阻塞而卡顿）
  Input:           param - 参数
  Return:          none
**********************************************************************/
static void* divoom_uac_main_task(void *param)
{
    int frames;
    int err;

    divoom_print("UAC: 主任务启动\n");
	/* 提高 UAC 线程调度优先级，减少卡顿 */
	{
		struct sched_param sp;
		memset(&sp, 0, sizeof(sp));
		sp.sched_priority = 80;  /* SCHED_FIFO 1~99，越高越优先 */
		if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) != 0) {
			/* 无 CAP_SYS_NICE 时降级：提高进程 nice（数值越小越优先） */
			(void)setpriority(PRIO_PROCESS, 0, -15);
		}
	}
	struct timeval tv_start, tv_now;
	unsigned int elapsed_ms;

    while (pdivoom_uac_ctrl && pdivoom_uac_ctrl->main_task_run)
    {
        // 检查播放状态
        if (pdivoom_uac_ctrl->status != UAC_STATUS_PLAYING) 
		{
            divoom_system_delay_ms(50);
            continue;
        }

        // 检查音频句柄；若尚未打开则先切回 uac1、等设备就绪后再打开（支持 stop 后再次进入）
        if (!pdivoom_uac_ctrl->capture_handle || !pdivoom_uac_ctrl->playback_handle) 
		{
			gettimeofday(&tv_start, NULL);
			while (!divoom_uac_is_device_ready()) 
			{
				gettimeofday(&tv_now, NULL);
				elapsed_ms = (unsigned int)((tv_now.tv_sec - tv_start.tv_sec) * 1000 + (tv_now.tv_usec - tv_start.tv_usec) / 1000);
				if (elapsed_ms >= UAC_READY_WAIT_TIMEOUT_MS)
					break;
				divoom_system_delay_ms(UAC_READY_POLL_INTERVAL_MS);
			}
			divoom_uac_setup_audio();
            divoom_system_delay_ms(200);
            continue;
        }

        /* 有双句柄后只创建一次捕获线程，专门往 ring 里填数据 */
        if (!pdivoom_uac_ctrl->capture_task_started) 
		{
            pdivoom_uac_ctrl->capture_task_started = 1;
            pthread_create(&pdivoom_uac_ctrl->capture_tid, NULL, divoom_uac_capture_task, pdivoom_uac_ctrl);
        }

        /* 主线程只负责播放：从 ring 取或写静音，加锁保护 */
        if (pdivoom_uac_ctrl->lock_handle)
            divoom_system_get_lock(pdivoom_uac_ctrl->lock_handle);
        if (pdivoom_uac_ctrl->ring_cnt >= UAC_PERIOD_SIZE) 
		{
            unsigned int rd = pdivoom_uac_ctrl->ring_rd;
            memcpy(pdivoom_uac_ctrl->audio_buffer, pdivoom_uac_ctrl->ring_buf + rd * 4, UAC_PERIOD_SIZE * 4);
            pdivoom_uac_ctrl->ring_rd = (rd + UAC_PERIOD_SIZE) % UAC_RING_FRAMES;
            pdivoom_uac_ctrl->ring_cnt -= UAC_PERIOD_SIZE;
            frames = UAC_PERIOD_SIZE;
        } 
		else 
		{
            memset(pdivoom_uac_ctrl->audio_buffer, 0, UAC_PERIOD_SIZE * 4);
            frames = UAC_PERIOD_SIZE;
        }
        if (pdivoom_uac_ctrl->lock_handle)
            divoom_system_free_lock(pdivoom_uac_ctrl->lock_handle);

#ifndef DIVOOM_TIME_FRAME_SPEAKER
        if (pdivoom_uac_ctrl->volume < 100)
            divoom_uac_apply_volume((int16_t *)pdivoom_uac_ctrl->audio_buffer, frames);
#endif
        err = snd_pcm_writei(pdivoom_uac_ctrl->playback_handle, pdivoom_uac_ctrl->audio_buffer, frames);
        if (err < 0) 
		{
            if (err == -EPIPE)
                snd_pcm_prepare(pdivoom_uac_ctrl->playback_handle);
            else if (err != -EAGAIN)
                divoom_print("UAC: 播放错误: %s\n", snd_strerror(err));
        }
    }

    if (pdivoom_uac_ctrl)
        pdivoom_uac_ctrl->run_flag = 0;
    if (pdivoom_uac_ctrl && pdivoom_uac_ctrl->capture_task_started) 
	{
        pthread_join(pdivoom_uac_ctrl->capture_tid, NULL);
        pdivoom_uac_ctrl->capture_task_started = 0;
    }
    divoom_print("UAC: 主任务退出\n");
    return NULL;
}

/*********************************************************************
  Function:        divoom_uac_cleanup
  Description:     清理资源
  Input:           none
  Return:          none
**********************************************************************/
static void divoom_uac_cleanup(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        return;
    }

    divoom_uac_close_audio();
    if (pdivoom_uac_ctrl->lock_handle) 
	{
        divoom_system_destroy_lock(pdivoom_uac_ctrl->lock_handle);
        pdivoom_uac_ctrl->lock_handle = NULL;
    }
    divoom_system_free(pdivoom_uac_ctrl);
    pdivoom_uac_ctrl = NULL;
}

/*********************************************************************
  Function:        divoom_uac_init
  Description:     初始化 UAC 模块
  Input:           none
  Return:          none
**********************************************************************/
void divoom_uac_init(void)
{
    if (pdivoom_uac_ctrl) 
	{
        divoom_print("UAC: 模块已初始化\n");
        return;
    }

    pdivoom_uac_ctrl = divoom_system_malloc(sizeof(divoom_uac_ctrl));
    if (!pdivoom_uac_ctrl) 
	{
        divoom_print("UAC: 分配控制结构失败\n");
        return;
    }

    // 初始化结构体
    memset(pdivoom_uac_ctrl, 0, sizeof(divoom_uac_ctrl));
    pdivoom_uac_ctrl->status = UAC_STATUS_IDLE;
    pdivoom_uac_ctrl->volume = 80;  // 默认音量 80%
    pdivoom_uac_ctrl->run_flag = 0;
    pdivoom_uac_ctrl->main_task_run = 0;
    pdivoom_uac_ctrl->main_task_created = 0;
    pdivoom_uac_ctrl->lock_handle = divoom_system_create_lock();
    
	divoom_print("UAC: 模块初始化ok\n");
	exec_command_system("echo 3 > /proc/sys/kernel/printk");//屏蔽内核打印，因为导致uac播放卡顿
    /* 主任务在 divoom_uac_start 中创建，stop 时 join 退出，避免常驻线程状态错乱 */
}

/*********************************************************************
  Function:        divoom_uac_deinit
  Description:     反初始化 UAC 模块
  Input:           none
  Return:          none
**********************************************************************/
void divoom_uac_deinit(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        return;
    }

    pdivoom_uac_ctrl->main_task_run = 0;
    pdivoom_uac_ctrl->run_flag = 0;
    pdivoom_uac_ctrl->status = UAC_STATUS_STOPPED;
    divoom_uac_pcm_drop_handles();

    if (pdivoom_uac_ctrl->main_task_created) 
	{
        pthread_join(pdivoom_uac_ctrl->task_handle, NULL);
        pdivoom_uac_ctrl->main_task_created = 0;
    }

    divoom_uac_cleanup();

    /* 切回默认 USB 配置（若需 adb 等可改为 setusbconfig adb） */
#ifdef DIVOOM_UAC_MODE
    divoom_uac_run_setusbconfig("adb");
#endif

    divoom_print("UAC: 模块反初始化完成\n");
}

/*********************************************************************
  Function:        divoom_uac_start
  Description:     开始 UAC 播放
  Input:           none
  Return:          none
**********************************************************************/
void divoom_uac_start(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        divoom_print("UAC: 模块未初始化\n");
        return;
    }

    /* 暂停仅改状态，主线程仍在；勿 join/关设备 */
    if (pdivoom_uac_ctrl->status == UAC_STATUS_PAUSED && pdivoom_uac_ctrl->main_task_created) 
	{
        pdivoom_uac_ctrl->status = UAC_STATUS_PLAYING;
        divoom_print("UAC: 暂停态进入播放(沿用原主线程)\n");
        return;
    }

    /* 仅当 PLAYING 且双路 PCM 仍有效且主线程在跑时跳过 */
    if (pdivoom_uac_ctrl->status == UAC_STATUS_PLAYING
        && pdivoom_uac_ctrl->capture_handle
        && pdivoom_uac_ctrl->playback_handle
        && pdivoom_uac_ctrl->main_task_created) 
	{
        divoom_print("UAC: 已在播放中\n");
        return;
    }

    if (pdivoom_uac_ctrl->main_task_created) 
	{
        pdivoom_uac_ctrl->main_task_run = 0;
        pdivoom_uac_ctrl->run_flag = 0;
        divoom_uac_pcm_drop_handles();
        pthread_join(pdivoom_uac_ctrl->task_handle, NULL);
        pdivoom_uac_ctrl->main_task_created = 0;
    }

    divoom_uac_close_audio();
#ifdef DIVOOM_UAC_MODE
    divoom_uac_run_setusbconfig("uac1");
    divoom_system_delay_ms(150);
#endif

    pdivoom_uac_ctrl->run_flag = 1;
    pdivoom_uac_ctrl->main_task_run = 1;
    pdivoom_uac_ctrl->capture_task_started = 0;
    pdivoom_uac_ctrl->status = UAC_STATUS_PLAYING;

    if (pthread_create(&pdivoom_uac_ctrl->task_handle, NULL, divoom_uac_main_task, pdivoom_uac_ctrl) != 0) 
	{
        divoom_print("UAC: 主任务 pthread_create 失败\n");
        pdivoom_uac_ctrl->main_task_run = 0;
        pdivoom_uac_ctrl->run_flag = 0;
        pdivoom_uac_ctrl->status = UAC_STATUS_STOPPED;
        return;
    }
    pdivoom_uac_ctrl->main_task_created = 1;
    divoom_print("UAC: 开始播放\n");
}

/*********************************************************************
  Function:        divoom_uac_stop
  Description:     停止 UAC 播放
  Input:           none
  Return:          none
**********************************************************************/
void divoom_uac_stop(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        divoom_print("UAC: 模块未初始化\n");
        return;
    }
//	exec_command_system("echo 8 > /proc/sys/kernel/printk");//恢复打印
    pdivoom_uac_ctrl->status = UAC_STATUS_STOPPED;

    /* 先停标志并 drop，唤醒阻塞在 readi/writei 的线程，再 join（否则可能卡数秒） */
    pdivoom_uac_ctrl->main_task_run = 0;
    pdivoom_uac_ctrl->run_flag = 0;
    divoom_uac_pcm_drop_handles();
    if (pdivoom_uac_ctrl->main_task_created) 
	{
        pthread_join(pdivoom_uac_ctrl->task_handle, NULL);
        pdivoom_uac_ctrl->main_task_created = 0;
    }
    pdivoom_uac_ctrl->capture_task_started = 0;

    divoom_uac_close_audio();                 /* 关闭 UAC 句柄，再次进入时会重新打开 */
#ifdef DIVOOM_UAC_MODE
    divoom_uac_run_setusbconfig("adb");       /* 退出 UAC，切回 ADB 以便调试 */
#endif

    divoom_print("UAC: 停止播放，已关闭句柄并切换 USB 为 adb\n");
}

/*********************************************************************
  Function:        divoom_uac_pause
  Description:     暂停 UAC 播放
  Input:           none
  Return:          none
**********************************************************************/
void divoom_uac_pause(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        divoom_print("UAC: 模块未初始化\n");
        return;
    }

    if (pdivoom_uac_ctrl->status != UAC_STATUS_PLAYING) 
	{
        return;
    }

    pdivoom_uac_ctrl->status = UAC_STATUS_PAUSED;
    divoom_print("UAC: 暂停播放\n");
}

/*********************************************************************
  Function:        divoom_uac_resume
  Description:     恢复 UAC 播放
  Input:           none
  Return:          none
**********************************************************************/
void divoom_uac_resume(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        divoom_print("UAC: 模块未初始化\n");
        return;
    }

    if (pdivoom_uac_ctrl->status != UAC_STATUS_PAUSED) 
	{
        return;
    }

    pdivoom_uac_ctrl->status = UAC_STATUS_PLAYING;
    divoom_print("UAC: 恢复播放\n");
}

/*********************************************************************
  Function:        divoom_uac_set_volume
  Description:     设置 UAC 音量 (0-100)
  Input:           volume - 音量值
  Return:          none
**********************************************************************/
void divoom_uac_set_volume(uint8 volume)
{
    if (!pdivoom_uac_ctrl) 
	{
        divoom_print("UAC: 模块未初始化\n");
        return;
    }

    if (volume > 100) 
	{
        volume = 100;
    }

    pdivoom_uac_ctrl->volume = volume;
    divoom_print("UAC: 音量设置为: %d\n", volume);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
    divoom_main_set_sys_vol((int)volume);
#endif
}

/*********************************************************************
  Function:        divoom_uac_get_volume
  Description:     获取 UAC 音量
  Input:           none
  Return:          音量值 (0-100)
**********************************************************************/
uint8 divoom_uac_get_volume(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        return 0;
    }

    return pdivoom_uac_ctrl->volume;
}

/*********************************************************************
  Function:        divoom_uac_get_status
  Description:     获取当前播放状态
  Input:           none
  Return:          播放状态
**********************************************************************/
UacStatus divoom_uac_get_status(void)
{
    if (!pdivoom_uac_ctrl) 
	{
        return UAC_STATUS_IDLE;
    }

    return pdivoom_uac_ctrl->status;
}
