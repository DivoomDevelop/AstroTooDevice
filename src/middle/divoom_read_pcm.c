#include "divoom.h"

#if 0
#include "divoom_read_pcm.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdbool.h>

#define PCM_DEVICE "default"
#define CHANNELS 1
#define FRAME_SIZE 256

typedef struct _divoom_read_pcm_ctrl
{
    bool is_running;
    int g_sample_rate;
    snd_pcm_t *capture_handle;
    pcm_data_callback user_callback;
    uint16 buffer[FRAME_SIZE * CHANNELS];
}divoom_read_pcm_ctrl, *Pdivoom_read_pcm_ctrl;

Pdivoom_read_pcm_ctrl pdivoom_read_pcm_handle = NULL;
// 捕获线程函数
void *capture_thread_func(void *arg) {
    snd_pcm_sframes_t frames;

    while (1) {
        frames = snd_pcm_readi(pdivoom_read_pcm_handle->capture_handle, pdivoom_read_pcm_handle->buffer, FRAME_SIZE);
        
        if (frames < 0) {
            frames = snd_pcm_recover(pdivoom_read_pcm_handle->capture_handle, frames, 0);
            if (frames < 0) {
                printf("snd_pcm_readi failed: %s\n", snd_strerror(frames));
                break;
            }
            continue;
        }
        if (pdivoom_read_pcm_handle->user_callback) {
            pdivoom_read_pcm_handle->user_callback(pdivoom_read_pcm_handle->buffer, frames * CHANNELS*2);
        }
    }

    return NULL;
}

int divoom_pcm_init(unsigned int sample_rate) {
    int err;
    snd_pcm_hw_params_t *hw_params;

    if(pdivoom_read_pcm_handle)
    {
        return -1;
    }
    pdivoom_read_pcm_handle = divoom_system_malloc(sizeof(divoom_read_pcm_ctrl));
    pdivoom_read_pcm_handle->g_sample_rate = sample_rate;


    // 打开PCM设备
    err = snd_pcm_open(&pdivoom_read_pcm_handle->capture_handle, PCM_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        divoom_print("无法打开音频设备: %s\n", snd_strerror(err));
        return -1;
    }

    // 分配硬件参数结构体
    snd_pcm_hw_params_alloca(&hw_params);

    // 初始化硬件参数
    err = snd_pcm_hw_params_any(pdivoom_read_pcm_handle->capture_handle, hw_params);
    if (err < 0) {
        printf("无法初始化硬件参数: %s\n", snd_strerror(err));
        goto error;
    }

    // 设置访问类型
    err = snd_pcm_hw_params_set_access(pdivoom_read_pcm_handle->capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        printf("无法设置访问类型: %s\n", snd_strerror(err));
        goto error;
    }

    // 设置采样格式
    err = snd_pcm_hw_params_set_format(pdivoom_read_pcm_handle->capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        printf("无法设置格式: %s\n", snd_strerror(err));
        goto error;
    }

    // 设置采样率
    err = snd_pcm_hw_params_set_rate_near(pdivoom_read_pcm_handle->capture_handle, hw_params, &pdivoom_read_pcm_handle->g_sample_rate, 0);
    if (err < 0) {
        printf("无法设置采样率: %s\n", snd_strerror(err));
        goto error;
    }

    // 设置通道数
    err = snd_pcm_hw_params_set_channels(pdivoom_read_pcm_handle->capture_handle, hw_params, CHANNELS);
    if (err < 0) {
        printf("无法设置通道数: %s\n", snd_strerror(err));
        goto error;
    }

    // 将参数写入设备
    err = snd_pcm_hw_params(pdivoom_read_pcm_handle->capture_handle, hw_params);
    if (err < 0) {
        printf("无法设置参数: %s\n", snd_strerror(err));
        goto error;
    }

    // 准备设备
    err = snd_pcm_prepare(pdivoom_read_pcm_handle->capture_handle);
    if (err < 0) {
        printf("无法准备音频接口: %s\n", snd_strerror(err));
        goto error;
    }
    divoom_system_create_task(capture_thread_func, "pcm_capture", pdivoom_read_pcm_handle, DIVOOM_TASK_STACK_READ_PCM);
    return 0;

error:
    snd_pcm_close(pdivoom_read_pcm_handle->capture_handle);
    pdivoom_read_pcm_handle->capture_handle = NULL;
    return -1;
}

void divoom_pcm_set_callback(pcm_data_callback callback) {
    pdivoom_read_pcm_handle->user_callback = callback;
}

#if 0
void divoom_pcm_stop(void) {
    if (!is_running) {
        return;
    }

    is_running = false;
    pthread_join(capture_thread, NULL);
}

void divoom_pcm_deinit(void) {
    if (is_running) {
        divoom_pcm_stop();
    }

    if (capture_handle) {
        snd_pcm_close(capture_handle);
        capture_handle = NULL;
    }
} 
#endif

#endif



