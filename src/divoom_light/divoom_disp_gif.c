#include "divoom.h"

#define DIVOOM_DISP_GIF_MAX_TRY_TIME 20
typedef struct _divoom_disp_gif_ctrl_info
{
    uint8 need_load_data;
    uint8 task_id;
    uint8 send_err_flag;
    uint8 http_text_font_table[20];
    void *phttp_font_handle;
    int try_time;
}divoom_disp_gif_ctrl_info, *Pdivoom_disp_gif_ctrl_info;

Pdivoom_disp_gif_ctrl_info pdivoom_disp_gif_ctrl_handle = NULL;
void divoom_disp_gif_load_data_main(void * data)
{
}

void divoom_disp_gif_init(void *data)
{
}

void divoom_disp_gif_text_get_font_data(Pdivoom_image_word_font_info pword_info)
{
}

void divoom_disp_gif_clear_text(void)
{
}

void divoom_disp_gif_add_text(void *data)
{
}

void divoom_disp_gif_update(void *data)
{
}

void divoom_disp_gif_close(void)
{
}

