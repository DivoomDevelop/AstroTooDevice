#ifndef _divoom_speex_record_h_
#define _divoom_speex_record_h_
#include "divoom_type.h"
void *divoom_speex_record_init(int sample_rate,int frame_size);
void divoom_speex_record_do(void *handle,uint8 *pcm_buf);
void divoom_speex_record_destory(void *handle);

#endif

