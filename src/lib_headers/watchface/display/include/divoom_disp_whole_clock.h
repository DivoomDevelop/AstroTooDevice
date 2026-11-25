#ifndef _divoom_disp_whole_clock_h_

#define _divoom_disp_whole_clock_h_

void *divoom_disp_whole_clock_init(void *data, uint8 *lcd_list, int lcd_num);

void divoom_disp_whole_clock_update(void *handle, void *data, uint8 *lcd_list, int lcd_num);


void divoom_disp_whole_clock_update_lcd_list(void *handle, uint8 *lcd_list, int lcd_num);


void divoom_disp_whole_clock_exit(void *handle);

#endif

