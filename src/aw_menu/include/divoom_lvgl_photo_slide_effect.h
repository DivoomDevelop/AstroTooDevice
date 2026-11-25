#ifndef _divoom_lvgl_photo_slide_effect_h_

#define _divoom_lvgl_photo_slide_effect_h_


typedef struct _divoom_lvgl_photo_slide_effect_info
{
    int    pic_pos;    //幻灯片播放位置
    uint32 album_id;   //相册名称
}divoom_lvgl_photo_slide_effect_info, *Pdivoom_lvgl_photo_slide_effect_info;


typedef struct _divoom_lvgl_photo_slide_effect_para
{
    int slide_type;         //幻灯片效果
    uint32 album_id;   //相册名称
}divoom_lvgl_photo_slide_effect_para, *Pdivoom_lvgl_photo_slide_effect_para;

void *divoom_lvgl_photo_slide_effect_init(void *user_data, int menu_id);
void divoom_lvgl_photo_slide_effect_disp(void *handle, void *active_obj);
void divoom_lvgl_photo_slide_effect_pause(void *handle);
void divoom_lvgl_photo_slide_effect_change_slide_effect_mode(void *handle);
void divoom_lvgl_photo_slide_effect_exit(void *handle);
int divoom_lvgl_photo_slide_effect_is_empty_album(void *handle);
void *divoom_lvgl_photo_slide_effect_create_title_disp(void *main_obj, char *disp_title, int menu_id);
void divoom_lvgl_photo_slide_effect_update_title_disp(void *title_obj, char *disp_title);


#endif


