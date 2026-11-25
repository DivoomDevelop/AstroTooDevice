#ifndef _divoom_disp_font_dial_h_
#define _divoom_disp_font_dial_h_

void *divoom_disp_font_dial_init(void *data, uint8 *lcd_list, int lcd_num);
void divoom_disp_font_dial_update(void *handle, void *data, uint8 *lcd_list, int lcd_num);
void divoom_disp_font_dial_update_lcd_list(void *handle, uint8 *lcd_list, int lcd_num);
void divoom_disp_font_dial_exit(void *handle);

#endif


