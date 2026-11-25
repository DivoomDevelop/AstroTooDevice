#ifndef _divoom_jl_mic_record_h_
#define _divoom_jl_mic_record_h_

#include "divoom_mic_record.h"

int divoom_jl_mic_record_start(const char *path, uint32 duration_sec,
			       divoom_mic_record_done_fn done_cb);
void divoom_jl_mic_record_stop(void);
void divoom_jl_mic_record_stop_sync(void);
int divoom_jl_mic_record_is_active(void);

#endif
