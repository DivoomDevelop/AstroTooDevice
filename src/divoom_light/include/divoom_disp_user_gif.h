#ifndef _divoom_disp_user_gif_h_
#define _divoom_disp_user_gif_h_
void *divoom_disp_user_gif_init(void *data, uint8 *lcd_id, uint8 lcd_num);
void divoom_disp_user_gif_update(void *handle, void *data, uint8 *lcd_id, uint8 lcd_num);
void divoom_disp_user_gif_update_lcd_list(void *handle, uint8 *lcd_id, uint8 lcd_num);
void divoom_disp_user_gif_exit(void *handle);

#endif


