#include "divoom.h"

#define TAG			"EQ_MIC"
#define DIVOOM_EQ_MIC_PRINT	    divoom_print

typedef struct
{
	uint8		is_cur_mode;
	int 		pre_record_value;
	int			record_value;
	void		*noise_lock;
#if defined(DIVOOM_PRODUCT_ASTRO_TOO)
    /* PCM 写入者和应用控制任务只在交换窗口、频带时短暂持锁。 */
    void *radio_lock;
    int16 radio_pcm[DIVOOM_SPECTRUM_RADIO_WINDOW_SAMPLES];
    int32 radio_bands[DIVOOM_SPECTRUM_BAND_NUM];
    uint32 radio_epoch;
    uint32 radio_pcm_tick;
    uint32 radio_bands_tick;
    uint16 radio_pcm_count;
    uint8 radio_pcm_phase;
    uint8 radio_bands_valid;
#endif
}divoom_eq_mic_ctrl, *Pdivoom_eq_mic_ctrl;

Pdivoom_eq_mic_ctrl pdivoom_eq_mic_handle = NULL;

#if defined(DIVOOM_PRODUCT_ASTRO_TOO)
/* 频带超过 160ms 未更新即过期，避免暂停或卡流时残留上次画面。 */
#define DIVOOM_RADIO_EQ_STALE_MS 160u

void divoom_eq_noise_mic_fft_feed_radio_pcm(const int16 *pcm, int32 frames)
{
    Pdivoom_eq_mic_ctrl ctrl = pdivoom_eq_mic_handle;
    if (!ctrl || !ctrl->radio_lock || !pcm || frames <= 0 ||
        divoom_system_try_lock(ctrl->radio_lock) != 0)
    {
        return;
    }
    /* 与展示模式一样每隔一个 48kHz 帧取样，奇数长度的块保持取样相位。 */
    int32 first = ctrl->radio_pcm_phase;
    ctrl->radio_pcm_phase ^= (uint8)(frames & 1);
    if (ctrl->radio_pcm_count < DIVOOM_SPECTRUM_RADIO_WINDOW_SAMPLES)
    {
        for (int32 i = first; i < frames &&
            ctrl->radio_pcm_count < DIVOOM_SPECTRUM_RADIO_WINDOW_SAMPLES; i += 2)
        {
            ctrl->radio_pcm[ctrl->radio_pcm_count++] =
                (int16)(((int32)pcm[i * 2] + pcm[i * 2 + 1]) / 2);
        }
        if (ctrl->radio_pcm_count == DIVOOM_SPECTRUM_RADIO_WINDOW_SAMPLES)
        {
            ctrl->radio_pcm_tick = divoom_system_get_tickms();
        }
    }
    divoom_system_free_lock_direct(ctrl->radio_lock);
}

void divoom_eq_noise_mic_fft_reset_radio(void)
{
    Pdivoom_eq_mic_ctrl ctrl = pdivoom_eq_mic_handle;
    if (!ctrl || !ctrl->radio_lock)
    {
        return;
    }
    divoom_system_get_lock(ctrl->radio_lock);
    ctrl->radio_epoch++;
    ctrl->radio_pcm_count = 0;
    ctrl->radio_pcm_phase = 0;
    ctrl->radio_bands_valid = 0;
    divoom_system_free_lock(ctrl->radio_lock);
}

int32 divoom_eq_noise_mic_fft_read_radio(int32 *bands, int32 max_age_ms)
{
    Pdivoom_eq_mic_ctrl ctrl = pdivoom_eq_mic_handle;
    int32 count = 0;
    if (!ctrl || !ctrl->radio_lock || divoom_system_try_lock(ctrl->radio_lock) != 0)
    {
        return 0;
    }
    uint32 max_age = max_age_ms > 0 && max_age_ms < DIVOOM_RADIO_EQ_STALE_MS ?
        (uint32)max_age_ms : DIVOOM_RADIO_EQ_STALE_MS;
    if (ctrl->radio_bands_valid && divoom_system_get_tickms() - ctrl->radio_bands_tick <= max_age)
    {
        count = DIVOOM_SPECTRUM_BAND_NUM;
        if (bands)
        {
            memcpy(bands, ctrl->radio_bands, sizeof(ctrl->radio_bands));
        }
    }
    divoom_system_free_lock_direct(ctrl->radio_lock);
    return count;
}

void divoom_eq_noise_mic_fft_poll_radio(void)
{
    Pdivoom_eq_mic_ctrl ctrl = pdivoom_eq_mic_handle;
    int16 samples[DIVOOM_SPECTRUM_RADIO_WINDOW_SAMPLES];
    int32 bands[DIVOOM_SPECTRUM_BAND_NUM];
    uint32 epoch;
    uint32 pcm_tick;
    uint8 ready;
    if (!ctrl || !ctrl->radio_lock)
    {
        return;
    }
    divoom_system_get_lock(ctrl->radio_lock);
    epoch = ctrl->radio_epoch;
    pcm_tick = ctrl->radio_pcm_tick;
    ready = ctrl->radio_pcm_count == DIVOOM_SPECTRUM_RADIO_WINDOW_SAMPLES;
    if (ready)
    {
        memcpy(samples, ctrl->radio_pcm, sizeof(samples));
        ctrl->radio_pcm_count = 0;
    }
    divoom_system_free_lock(ctrl->radio_lock);
    if (divoom_music_ctrl_get_mode() != DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        return;
    }
    if (ready && divoom_system_get_tickms() - pcm_tick <= DIVOOM_RADIO_EQ_STALE_MS &&
        divoom_spectrum_calculate_radio_window(samples, bands))
    {
        divoom_system_get_lock(ctrl->radio_lock);
        if (ctrl->radio_epoch == epoch)
        {
            memcpy(ctrl->radio_bands, bands, sizeof(bands));
            ctrl->radio_bands_tick = pcm_tick;
            ctrl->radio_bands_valid = 1;
        }
        divoom_system_free_lock(ctrl->radio_lock);
    }
}
#endif

//拿到的mic数据进行fft运算
void divoom_eq_noise_mic_fft_eq_fft_data(uint16 *pcm_data, int len)
{
	int i, j, cur_voice = 0, tol_value = 0;
	T_S16 *voice_data;
    
	if(pdivoom_eq_mic_handle == NULL)
	{
		DIVOOM_EQ_MIC_PRINT("Must Use This Function Before Init!--%d", __LINE__);
		return;
	}
	//DIVOOM_EQ_MIC_PRINT("%s---%d\n", __func__, __LINE__);
	if(pdivoom_eq_mic_handle->is_cur_mode == 1 && !divoom_main_get_mic_switch())
	{
        /* 按样本组统计环境噪音。 */
		voice_data = (T_S16 *)pcm_data;

		for(i = 0, j = 0; i < (len >> 1); i++)
		{
			cur_voice += voice_data[i]*voice_data[i];
			tol_value += cur_voice;
			j ++;
			if(j >= 4)
			{
				divoom_system_get_lock(pdivoom_eq_mic_handle->noise_lock);
				if(cur_voice > pdivoom_eq_mic_handle->record_value)
				{
					pdivoom_eq_mic_handle->record_value = cur_voice;
				}
				divoom_system_free_lock(pdivoom_eq_mic_handle->noise_lock);
				j = 0;
				cur_voice = 0;
			}
		}
	}
}

//噪音这里不需要做fft,需要把数据每位相乘,再log
void divoom_eq_noise_mic_fft_noise_no_fft_data(uint8 *data, uint16 len)
{
	int i, j, cur_voice = 0, tol_value = 0;
	T_S16 *voice_data;
	if(pdivoom_eq_mic_handle == NULL)
	{
		DIVOOM_EQ_MIC_PRINT("Must Use This Function Before Init!--%d", __LINE__);
		return;
	}
	if(pdivoom_eq_mic_handle->is_cur_mode == 1)
	{
		voice_data = (T_S16 *)data;

		for(i = 0, j = 0; i < (len >> 1); i++)
		{
			cur_voice += voice_data[i]*voice_data[i];
			tol_value += cur_voice;
			j ++;
			if(j >= 4)
			{
				divoom_system_get_lock(pdivoom_eq_mic_handle->noise_lock);
				if(cur_voice > pdivoom_eq_mic_handle->record_value)
				{
					pdivoom_eq_mic_handle->record_value = cur_voice;
				}
				divoom_system_free_lock(pdivoom_eq_mic_handle->noise_lock);
				j = 0;
				cur_voice = 0;
			}
		}
	}
}

int divoom_eq_noise_mic_fft_get_noise_value(void)
{
#ifdef DIVOOM_ASTRO_TOO
	return divoom_btcore_uart_cmd_get_mic_noise_db();
#else
	int value = 0;
    
    if(divoom_main_get_mic_switch())
    {
        return 0;
    }
    
	if(pdivoom_eq_mic_handle && pdivoom_eq_mic_handle->record_value != 0)
	{
		divoom_system_get_lock(pdivoom_eq_mic_handle->noise_lock);
		value = 10*log10(pdivoom_eq_mic_handle->record_value) - 4;
		pdivoom_eq_mic_handle->record_value = 0;
		pdivoom_eq_mic_handle->pre_record_value = value;
		divoom_system_free_lock(pdivoom_eq_mic_handle->noise_lock);
		return value;
	}
	return pdivoom_eq_mic_handle->pre_record_value;
#endif
}

void divoom_eq_noise_mic_fft_start(void *func, DIVOOM_FFT_MIC_TYPE type)
{
	if(pdivoom_eq_mic_handle == NULL)
	{
		DIVOOM_EQ_MIC_PRINT("Must Use This Function Before Init!--%d", __LINE__);
		return;
	}
	pdivoom_eq_mic_handle->is_cur_mode = 1;
	(void)func;
}

void divoom_eq_noise_mic_fft_start_clock(void *func, DIVOOM_FFT_MIC_TYPE type)
{
	if(pdivoom_eq_mic_handle == NULL)
	{
		DIVOOM_EQ_MIC_PRINT("Must Use This Function Before Init!--%d", __LINE__);
		return;
	}
	pdivoom_eq_mic_handle->is_cur_mode = 1;
	(void)func;
}


void divoom_eq_noise_mic_fft_stop(void)
{
	if(pdivoom_eq_mic_handle == NULL)
	{
		DIVOOM_EQ_MIC_PRINT("Must Use This Function Before Init!--%d", __LINE__);
		return;
	}
	if(pdivoom_eq_mic_handle->is_cur_mode == 1)
	{
		//divoom_mic_ctrl_release();
	}
//	pdivoom_eq_mic_handle->is_cur_mode = 0;
}

void divoom_eq_noise_mic_fft_stop_clock(void)
{
	if(pdivoom_eq_mic_handle == NULL)
	{
		DIVOOM_EQ_MIC_PRINT("Must Use This Function Before Init!--%d", __LINE__);
		return;
	}
	if(pdivoom_eq_mic_handle->is_cur_mode == 1)
	{
		//divoom_mic_ctrl_release();
	}
//	pdivoom_eq_mic_handle->is_cur_mode = 0;
}

void divoom_eq_noise_mic_fft_init(void)
{
    if (pdivoom_eq_mic_handle != NULL)
    {
        DIVOOM_EQ_MIC_PRINT("Had init!");
        return;
    }

    Pdivoom_eq_mic_ctrl ctrl = divoom_system_malloc(sizeof(*ctrl));
    if (ctrl == NULL)
    {
        return;
    }
    ctrl->noise_lock = divoom_system_create_lock();
    if (ctrl->noise_lock == NULL)
    {
        divoom_system_free(ctrl);
        return;
    }
#if defined(DIVOOM_PRODUCT_ASTRO_TOO)
    ctrl->radio_lock = divoom_system_create_lock();
    if (ctrl->radio_lock == NULL)
    {
        divoom_system_destroy_lock(ctrl->noise_lock);
        divoom_system_free(ctrl);
        return;
    }
#endif

    /* 锁都创建成功后再发布控制块，电台和 PCM 回调才可开始读取。 */
    pdivoom_eq_mic_handle = ctrl;
    divoom_eq_noise_mic_fft_register_pcm_callback();
    divoom_eq_noise_mic_fft_start(NULL, DIVOOM_FFT_MIC_EQ);
}

/* Line-In 退出后重新 init PCM 时需调用，恢复 FFT 麦克风回调 */
void divoom_eq_noise_mic_fft_register_pcm_callback(void)
{
    divoom_pcm_set_callback(divoom_eq_noise_mic_fft_eq_fft_data);
}

void divoom_eq_noise_mic_fft_exit(void)
{
	if(pdivoom_eq_mic_handle == NULL)
	{
		return;
	}
#if defined(DIVOOM_PRODUCT_ASTRO_TOO)
    if (pdivoom_eq_mic_handle->radio_lock)
    {
        divoom_system_destroy_lock(pdivoom_eq_mic_handle->radio_lock);
    }
#endif
    divoom_system_destroy_lock(pdivoom_eq_mic_handle->noise_lock);
    divoom_system_free(pdivoom_eq_mic_handle);
    pdivoom_eq_mic_handle = NULL;
}



