#ifndef _divoom_disp_lcd_clock_h_
#define _divoom_disp_lcd_clock_h_


void *divoom_disp_lcd_clock_init(void *data, uint8 lcd_id);

void divoom_disp_lcd_clock_update(void *handle, void *data);

void divoom_disp_lcd_clock_exit(void *handle);


#endif

