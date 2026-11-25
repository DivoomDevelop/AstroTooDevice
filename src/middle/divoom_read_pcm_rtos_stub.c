#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom.h"
#include "divoom_read_pcm.h"

#if DIVOOM_PIXEL_EQ_ENABLE
#include <string.h>

#define DIVOOM_PCM_STUB_DEFAULT_RATE   32000u
#define DIVOOM_PCM_STUB_TICK_MS        16u
/* 32kHz @ 16ms => 512 samples；与 Tina 侧 FRAME_SIZE*2 字节语义一致 */
#define DIVOOM_PCM_STUB_MAX_SAMPLES    512u

typedef struct _divoom_read_pcm_ctrl
{
	uint8 is_running;
	int g_sample_rate;
	pcm_data_callback user_callback;
} divoom_read_pcm_ctrl, *Pdivoom_read_pcm_ctrl;

static Pdivoom_read_pcm_ctrl pdivoom_read_pcm_handle = NULL;
static uint16 s_pcm_silence_buf[DIVOOM_PCM_STUB_MAX_SAMPLES];

static int divoom_pcm_stub_frame_bytes(int sample_rate)
{
	int samples;

	if(sample_rate <= 0)
	{
		sample_rate = (int)DIVOOM_PCM_STUB_DEFAULT_RATE;
	}
	samples = sample_rate * (int)DIVOOM_PCM_STUB_TICK_MS / 1000;
	if(samples <= 0)
	{
		samples = (int)DIVOOM_PCM_STUB_MAX_SAMPLES;
	}
	if(samples > (int)DIVOOM_PCM_STUB_MAX_SAMPLES)
	{
		samples = (int)DIVOOM_PCM_STUB_MAX_SAMPLES;
	}
	return samples * (int)sizeof(uint16);
}

void divoom_pcm_try_prime_playback_default(void)
{
}

void divoom_pcm_apply_capture_gain(void)
{
}

static void *divoom_pcm_callback_thread(void *arg)
{
	Pdivoom_read_pcm_ctrl ctrl = (Pdivoom_read_pcm_ctrl)arg;

	while(ctrl && ctrl->is_running)
	{
		if(ctrl->user_callback)
		{
			int frame_bytes = divoom_pcm_stub_frame_bytes(ctrl->g_sample_rate);

			/* 本地无 mic：用静音帧驱动 FFT/EQ 回调，禁止传 NULL */
			ctrl->user_callback(s_pcm_silence_buf, frame_bytes);
		}
		divoom_system_delay_ms(DIVOOM_PCM_STUB_TICK_MS);
	}
	return NULL;
}

static int divoom_pcm_start_callback_thread(Pdivoom_read_pcm_ctrl ctrl)
{
	if(ctrl == NULL)
	{
		return -1;
	}
	if(ctrl->is_running)
	{
		return 0;
	}
	ctrl->is_running = 1;
	divoom_system_create_task(divoom_pcm_callback_thread, "pcm_callback", ctrl, DIVOOM_TASK_STACK_READ_PCM);
	return 0;
}

int divoom_pcm_init(unsigned int sample_rate)
{
	if(pdivoom_read_pcm_handle)
	{
		return -1;
	}

	pdivoom_read_pcm_handle = divoom_system_malloc(sizeof(divoom_read_pcm_ctrl));
	if(pdivoom_read_pcm_handle == NULL)
	{
		return -1;
	}
	memset(pdivoom_read_pcm_handle, 0, sizeof(divoom_read_pcm_ctrl));
	pdivoom_read_pcm_handle->g_sample_rate = (int)((sample_rate > 0u) ? sample_rate : DIVOOM_PCM_STUB_DEFAULT_RATE);

	if(divoom_pcm_start_callback_thread(pdivoom_read_pcm_handle) != 0)
	{
		divoom_system_free(pdivoom_read_pcm_handle);
		pdivoom_read_pcm_handle = NULL;
		return -1;
	}
	return 0;
}

void divoom_pcm_set_callback(pcm_data_callback callback)
{
	if(pdivoom_read_pcm_handle)
	{
		pdivoom_read_pcm_handle->user_callback = callback;
	}
}

unsigned int divoom_pcm_get_sample_rate(void)
{
	if(pdivoom_read_pcm_handle == NULL)
	{
		return 0;
	}
	return (unsigned int)pdivoom_read_pcm_handle->g_sample_rate;
}

int divoom_pcm_ensure_capture(void)
{
	if(pdivoom_read_pcm_handle == NULL)
	{
		return divoom_pcm_init(DIVOOM_PCM_STUB_DEFAULT_RATE);
	}
	if(!pdivoom_read_pcm_handle->is_running)
	{
		return divoom_pcm_start_callback_thread(pdivoom_read_pcm_handle);
	}
	return 0;
}

int divoom_pcm_start(void)
{
	return divoom_pcm_ensure_capture();
}

void divoom_pcm_stop(void)
{
	if(pdivoom_read_pcm_handle == NULL)
	{
		return;
	}
	pdivoom_read_pcm_handle->is_running = 0;
	divoom_system_delay_ms(150);
}

void divoom_pcm_deinit(void)
{
	if(pdivoom_read_pcm_handle == NULL)
	{
		return;
	}
	pdivoom_read_pcm_handle->is_running = 0;
	divoom_system_delay_ms(250);
	divoom_system_free(pdivoom_read_pcm_handle);
	pdivoom_read_pcm_handle = NULL;
}
#else
void divoom_pcm_try_prime_playback_default(void) {}
void divoom_pcm_apply_capture_gain(void) {}
int divoom_pcm_init(unsigned int sample_rate) { (void)sample_rate; return 0; }
void divoom_pcm_set_callback(pcm_data_callback callback) { (void)callback; }
unsigned int divoom_pcm_get_sample_rate(void) { return 0; }
int divoom_pcm_ensure_capture(void) { return 0; }
int divoom_pcm_start(void) { return 0; }
void divoom_pcm_stop(void) {}
void divoom_pcm_deinit(void) {}
#endif
