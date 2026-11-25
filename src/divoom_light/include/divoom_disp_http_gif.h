#ifndef _divoom_disp_http_gif_h_
#define _divoom_disp_http_gif_h_
void divoom_disp_http_gif_close(void *handle);
void divoom_disp_http_gif_update(void *handle,void *data, uint8 *lcd_id, uint8 lcd_num);
void divoom_disp_http_gif_update_lcd_list(void *handle,uint8 *lcd_id, uint8 lcd_num);
void *divoom_disp_http_gif_init(void *data, uint8 *lcd_id, uint8 lcd_num);

#endif


