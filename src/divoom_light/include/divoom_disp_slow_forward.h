#ifndef _divoom_disp_slow_forward_h_
#define _divoom_disp_slow_forward_h_

void *divoom_disp_slow_forward_init(void *data, uint8 *lcd_list, uint8 lcd_num);

void divoom_disp_slow_forward_update_lcd_list(void *handle, uint8 *lcd_list, uint8 lcd_num);
void divoom_disp_slow_forward_exit(void *handle);
#endif

