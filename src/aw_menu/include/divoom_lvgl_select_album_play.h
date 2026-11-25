#ifndef _divoom_lvgl_select_album_play_h_
#define _divoom_lvgl_select_album_play_h_

typedef struct _divoom_lvgl_select_album_play_para
{
    int type;           //0:进入相簿浏览；1：将照片添加到相簿;
    int pic_num;
    int *pic_list;  //只有type为1，才有效果
    int album_id;
}divoom_lvgl_select_album_play_para, *Pdivoom_lvgl_select_album_play_para;
void *divoom_lvgl_select_album_play_init(void *user_data, int menu_id);

void divoom_lvgl_select_album_play_disp(void *handle, void *active_obj);

void divoom_lvgl_select_album_play_pause(void *handle);

void divoom_lvgl_select_album_play_exit(void *handle);

#endif


