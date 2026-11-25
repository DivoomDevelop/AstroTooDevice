#include <alsa/asoundlib.h>
#include "divoom.h"

//#ifdef DIVOOM_USE_RK_IC
#ifdef DIVOOM_TIME_FRAME_SPEAKER
typedef struct _divoom_snd_pcm_ctrl
{
	snd_pcm_t *pcm_handle;
	snd_pcm_uframes_t frames; //一个周期有多少帧
	int channel;
	int run_flag;
	void *sem;
}divoom_snd_pcm_ctrl, *Pdivoom_snd_pcm_ctrl;

static Pdivoom_snd_pcm_ctrl pdivoom_snd_pcm_handle = NULL;

void divoom_snd_pcm_test(void);

void divoom_snd_pcm_param_init(void)
{
    int rate = 16000, channel = 1, err = 0;
	snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	snd_pcm_uframes_t period_size = 1024;  // 周期大小（帧数）
	snd_pcm_uframes_t buffer_size = 1024*4;  // 缓冲区大小（帧数）

    if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)// 打开PCM设备
		divoom_print("pcm_open error\n");

    if ((err = snd_pcm_hw_params_malloc(&params)) < 0)// 分配硬件参数对象
		divoom_print("error\n");
    
	if (snd_pcm_hw_params_any(pcm_handle, params) < 0)// 初始化硬件参数对象
		divoom_print("error\n");
    
	if (snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)// 设置访问类型
		divoom_print("error\n");

    if (snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE) < 0)// 设置音频格式
		divoom_print("error\n");

    if (snd_pcm_hw_params_set_channels(pcm_handle, params, channel) < 0)// 设置通道数
		divoom_print("error\n");
	
    if (snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0) < 0)// 设置采样率
		divoom_print("error\n");

  	snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &period_size, 0); // 设置周期大小
    snd_pcm_hw_params_set_buffer_size_near(pcm_handle, params, &buffer_size); // 设置缓冲区大小

    if (snd_pcm_hw_params(pcm_handle, params) < 0)// 应用参数
		divoom_print("error\n");
	
	// 获取帧大小（以字节为单位）
	snd_pcm_hw_params_get_period_size(params, &frames, 0);
	snd_pcm_hw_params_free(params);

	/*准备音频接口,并判断是否准备好*/
	if((err = snd_pcm_prepare(pcm_handle))<0) 
		divoom_print("can't use audio interface (%s)\n",snd_strerror(err));

	pdivoom_snd_pcm_handle->pcm_handle  = pcm_handle;
	pdivoom_snd_pcm_handle->frames = frames;
	pdivoom_snd_pcm_handle->channel = channel;
}

void *divoom_snd_pcm_task_main(void *data)
{
	int ret = 0, size = 0, i = 0;
	snd_pcm_sframes_t avail_frames;
	size = pdivoom_snd_pcm_handle->frames * pdivoom_snd_pcm_handle->channel * 2;
	char *buf = divoom_system_malloc(size);

	while (pdivoom_snd_pcm_handle->run_flag)
	{
		ret = divoom_mix_pcm_ring_get(buf,size);
		if (ret)
		{
	        snd_pcm_writei(pdivoom_snd_pcm_handle->pcm_handle, buf, pdivoom_snd_pcm_handle->frames);
		}
		else
		{
			divoom_print("get pcm error\n");
			divoom_system_delay_ms(100);
		}
	}
	divoom_system_free(buf);
	divoom_system_put_sem(pdivoom_snd_pcm_handle->sem);
	return NULL;
}	

void divoom_snd_pcm_init(void)
{
	if (pdivoom_snd_pcm_handle)
		return ;

	pdivoom_snd_pcm_handle = divoom_system_malloc(sizeof(divoom_snd_pcm_ctrl));
	divoom_snd_pcm_param_init();
	pdivoom_snd_pcm_handle->run_flag = 1;
	pdivoom_snd_pcm_handle->sem = divoom_system_create_sem(1, 0);
	divoom_system_create_task(divoom_snd_pcm_task_main, "snd_pcm", NULL, DIVOOM_TASK_STACK_SND_PCM);
}

void divoom_snd_pcm_close(void)
{
	if (pdivoom_snd_pcm_handle == NULL)
		return ;

	pdivoom_snd_pcm_handle->run_flag = 0;
	divoom_system_get_sem(pdivoom_snd_pcm_handle->sem);
	divoom_system_destroy_sem(pdivoom_snd_pcm_handle->sem);	
	// 释放资源
    snd_pcm_drain(pdivoom_snd_pcm_handle->pcm_handle);
    snd_pcm_close(pdivoom_snd_pcm_handle->pcm_handle);
	divoom_system_free(pdivoom_snd_pcm_handle);
	pdivoom_snd_pcm_handle = NULL;
}

void divoom_snd_pcm_test(void)
{
	void *fp = NULL;
	int len = 0, read_len = 0;
	int size = pdivoom_snd_pcm_handle->frames * pdivoom_snd_pcm_handle->channel * 2; // 16bit 2字节/样本
	char *buffer = (char *)divoom_system_malloc(size);

	divoom_print("wft test buf_size: %d, %d\n", size, pdivoom_snd_pcm_handle->frames);

	fp = divoom_port_file_open("/oem/tone/2_frog_sound.pcm", "rb");
	divoom_port_file_seek(fp, 0, SEEK_END);
	int file_size = divoom_port_file_tell(fp);
	divoom_port_file_seek(fp, 0, SEEK_SET);
	divoom_print("file_size: %d\n", file_size);

	while (read_len < file_size)
	{
		len = divoom_port_file_read(buffer, 1, size, fp);
		read_len += len;
		len = snd_pcm_writei(pdivoom_snd_pcm_handle->pcm_handle, buffer, pdivoom_snd_pcm_handle->frames); //按帧大小来写数据，而不是数据量大小，否则会出现啪啪音
		if (len == -EPIPE) 
		{
			// 缓冲区下溢，重置音频设备
			fprintf(stderr, "Underrun occurred, preparing device\n");
			snd_pcm_prepare(pdivoom_snd_pcm_handle->pcm_handle);
		}
		else if (len < 0) 
		{
			// 处理其他错误
			fprintf(stderr, "Error writing to PCM device: %s\n", snd_strerror(len));
		} 
		else if (len < pdivoom_snd_pcm_handle->frames) 
		{
			fprintf(stderr, "Short write, wrote %ld frames\n", len);
		}
	}

	divoom_port_file_close(fp);
	divoom_system_free(buffer);
	snd_pcm_drain(pdivoom_snd_pcm_handle->pcm_handle);
	snd_pcm_close(pdivoom_snd_pcm_handle->pcm_handle);
}
#else

#ifndef DIVOOM_SIMULATOR
void divoom_snd_pcm_init(void)
{
}

void divoom_snd_pcm_close(void)
{
}
#endif /* !DIVOOM_SIMULATOR */

#endif


