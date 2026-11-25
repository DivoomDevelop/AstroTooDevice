#ifndef _divoom_disp_word_whole_h_
#define _divoom_disp_word_whole_h_


void *divoom_disp_word_whole_init(void *data, uint8 *lcd_list, uint8 lcd_num);
void divoom_disp_word_whole_update(void *handle, void *data, uint8 *lcd_list, uint8 lcd_num);
void divoom_disp_word_whole_update_lcd_list(void *handle, uint8 *lcd_list, uint8 lcd_num);
void divoom_disp_word_whole_exit(void *handle);

#endif


