#ifndef _divoom_disp_mul_gif_file_h_
#define _divoom_disp_mul_gif_file_h_

typedef struct _divoom_disp_mul_gif_file_item
{
    uint8 lcd_num;
    uint8 lcd_list[DIVOOM_LCD_NUM];
    char  file_name[64];
    DIVOOM_CLOCK_GIF_CONTROL_DISP_END *pcallback;
    DIVOOM_CLOCK_GIF_CONTROL_READ_END *preadback;
    void  *para;
}divoom_disp_mul_gif_file_item , *Pdivoom_disp_mul_gif_file_item;

typedef struct _divoom_disp_mul_gif_file_list
{
    int num;
    int tot_time;
    int noice_id;      //结束通知标记
    void *para;
    DIVOOM_LIGHT_DISP_OVER_CALLBACK *pover_callback;
    divoom_disp_mul_gif_file_item plist[DIVOOM_LCD_NUM];
}divoom_disp_mul_gif_file_list , *Pdivoom_disp_mul_gif_file_list;

void *divoom_disp_mul_gif_file_init(void *data);
void divoom_disp_mul_gif_file_update(void *handle,void *data);
void divoom_disp_mul_gif_file_exit(void *handle);
void divoom_disp_mul_gif_file_update_lcd_list(void *handle, uint8 *lcd_list, int lcd_num);
#endif


