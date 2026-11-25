#ifndef _divoom_disp_world_time_h_
#define _divoom_disp_world_time_h_

void * divoom_disp_world_time_init(void *data, uint8 *lcd_list, uint8 lcd_num);

void divoom_disp_world_time_update_lcd_list(void *handle, uint8 *lcd_list, uint8 lcd_num);

void divoom_disp_world_time_exit(void *handle);

#endif


