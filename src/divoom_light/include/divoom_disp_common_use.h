#ifndef _DIVOOM_DISP_COMMON_H_
#define _DIVOOM_DISP_COMMON_H_

#include "divoom_light_env.h"

void divoom_disp_time_common_time_init(void);
void divoom_disp_time_common_time_colse(void);
//void divoom_disp_time_common_get_time_buf(uint8 *time_buf, Pdivoom_screen_color_info colour);
void divoom_disp_time_common_get_time_buf(uint8 *time_buf, Pdivoom_light_env_time_info time_info_in, uint8 dot_flag);
//void divoom_disp_time_common_get_time_buf(uint8 *time_buf, Pdivoom_light_env_time_info time_info_in);
void divoom_disp_time_common_get_time_buf_ext(uint8 *time_buf, Pdivoom_light_env_time_info time_info_in, uint8 dot_flag);

#endif
