#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom_speex_record.h"
#include "divoom_type.h"

void *divoom_speex_record_init(int sample_rate, int frame_size)
{
	(void)sample_rate;
	(void)frame_size;
	return (void *)1;
}
void divoom_speex_record_do(void *handle, uint8 *pcm_buf)
{
	(void)handle;
	(void)pcm_buf;
}
void divoom_speex_record_destory(void *handle) { (void)handle; }
