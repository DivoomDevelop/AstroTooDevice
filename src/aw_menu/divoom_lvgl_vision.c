//渐隐效果
#include <malloc.h>    
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"


typedef struct  _divoom_lvgl_vision_ctrl
{
    int video_num;
    int  exit_flag;
    int  end_flag;
    char file_name[256];
    lv_style_t main_style;   
    void *no_video_lable_obj;
    void *time_handle;
    void *main_obj;
    void *sem_wait;
    void *sem_exit;
    void *video_handle;
}divoom_lvgl_vision_ctrl, *Pdivoom_lvgl_vision_ctrl;

int divoom_lvgl_vision_play_over_callback(void *user_data, int status)
{
    int video_num = divoom_vision_ctrl_get_total();
    Pdivoom_lvgl_vision_ctrl phandle = (Pdivoom_lvgl_vision_ctrl)user_data;

    if(video_num < 2 && video_num == phandle->video_num)
    {
        divoom_print("replay!\n");
        return 0;
    }
    if(status == 1)
    {
        divoom_print("play over!\n");
        divoom_system_put_sem(phandle->sem_wait);
    }

    return  1;
}

void* divoom_lvgl_vision_main_task(void *param)
{
	int time;
    Pdivoom_lvgl_vision_ctrl phandle = (Pdivoom_lvgl_vision_ctrl)param;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

    while(phandle->exit_flag == 0)
    {
        
        
        divoom_system_get_sem(phandle->sem_wait);
        
        

        if(phandle->video_num > 0 && divoom_vision_ctrl_get_total() == 0)
        {
            if(phandle->video_handle)
            {
                divoom_lvgl_movie_play_stop(phandle->video_handle, NULL);
                phandle->video_handle = NULL;
            }
        }
        phandle->video_num = divoom_vision_ctrl_get_total();
        if(phandle->exit_flag==0 && divoom_vision_ctrl_get_item(phandle->file_name))
        {
			time = divoom_light_app_get_vision_play_time();
			time *= 1000;
            phandle->video_handle = divoom_lvgl_movie_play_play(phandle->main_obj, phandle,0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 
                phandle->file_name, divoom_pic_phone_manage_convert_video_volume(pslide_config->video_volume), 0, time, divoom_lvgl_vision_play_over_callback);
        }
        
    }
    
    divoom_system_put_sem(phandle->sem_exit);
    return NULL;
}
void divoom_lvgl_vision_refresh_label(struct _lv_timer_t *time_data)
{
    Pdivoom_lvgl_vision_ctrl phandle = (Pdivoom_lvgl_vision_ctrl)lv_timer_get_user_data(time_data);
    
    if(divoom_vision_ctrl_get_total() == 0)
    {
        lv_obj_clear_flag(phandle->no_video_lable_obj, LV_OBJ_FLAG_HIDDEN);
        if(phandle->video_handle)
        {
            divoom_lvgl_movie_play_stop(phandle->video_handle, NULL);
            divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
            phandle->video_handle = NULL;
        }
    }
    else
    {
        lv_obj_add_flag(phandle->no_video_lable_obj, LV_OBJ_FLAG_HIDDEN);
    }
}


void *divoom_lvgl_vision_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_vision_ctrl phandle;

    
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_vision_ctrl));
    divoom_vision_ctrl_open_handle();
    phandle->video_num = divoom_vision_ctrl_get_total();
    phandle->sem_wait = divoom_system_create_sem(1, 0);
    phandle->sem_exit = divoom_system_create_sem(1, 0);
    phandle->main_obj = lv_btn_create(NULL); 
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_white(), 0);
    divoom_menu_control_init_btn_stype(NULL, NULL, &phandle->main_style, NULL, NULL, NULL, 0);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
    lv_obj_add_style(phandle->main_obj, &phandle->main_style, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

    divoom_system_create_task(divoom_lvgl_vision_main_task, "lv_vision", phandle, DIVOOM_TASK_STACK_LVGL_VISION);
    
    //给出一个提示，没有照片的提示,理论是不会出现这情况，在上一级就返回了
    void* use_font = divoom_menu_control_get_font_info(56, menu_id);
    void *label_obj = lv_label_create(phandle->main_obj);
    if(use_font != NULL)
    {
        lv_obj_set_style_text_font(label_obj, use_font, 0);
    }
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
    
    // 设置标签宽度（必需，否则文本不会换行）
    lv_obj_set_width(label_obj, 600);  // 设置想要的宽度，比如200像素
    
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_align(label_obj, LV_ALIGN_CENTER);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("PlsAddPhoto"));
    phandle->no_video_lable_obj = label_obj;
    phandle->time_handle = lv_timer_create(divoom_lvgl_vision_refresh_label, 1000, phandle);

    if(divoom_vision_ctrl_get_total())
    {
        lv_obj_add_flag(phandle->no_video_lable_obj, LV_OBJ_FLAG_HIDDEN);
    }

    return phandle;
}

void divoom_lvgl_vision_disp(void *handle, void *active_obj)
{
	int time;
    Pdivoom_lvgl_vision_ctrl phandle = (Pdivoom_lvgl_vision_ctrl)handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    
    
    lv_scr_load(phandle->main_obj);
    if(divoom_vision_ctrl_get_item(phandle->file_name))
    {
        time = divoom_light_app_get_vision_play_time();
		time *= 1000;
        divoom_menu_control_change_disp_mode(phandle->main_obj, 1);
        phandle->video_handle = divoom_lvgl_movie_play_play(phandle->main_obj, phandle,0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 
            phandle->file_name, divoom_pic_phone_manage_convert_video_volume(pslide_config->video_volume), 0, time, divoom_lvgl_vision_play_over_callback);
    }
    
}


void divoom_lvgl_vision_pause(void *handle)
{
    Pdivoom_lvgl_vision_ctrl phandle = (Pdivoom_lvgl_vision_ctrl)handle;
  
    if(phandle->video_handle)
    {
        divoom_lvgl_movie_play_stop(phandle->video_handle, NULL);
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        phandle->video_handle = NULL;
    }
    
}


void divoom_lvgl_vision_exit(void *handle)
{
    Pdivoom_lvgl_vision_ctrl phandle = (Pdivoom_lvgl_vision_ctrl)handle;
    

    lv_timer_del(phandle->time_handle);
    
    phandle->exit_flag = 1;
    if(phandle->video_handle)
    {
        
        divoom_lvgl_movie_play_stop(phandle->video_handle, NULL);
    
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        phandle->video_handle = NULL;
    }
    
    
    divoom_system_put_sem(phandle->sem_wait);
    
    divoom_system_get_sem(phandle->sem_exit);
    
    
    divoom_system_destroy_sem(phandle->sem_wait);
    divoom_system_destroy_sem(phandle->sem_exit);
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->main_style);
    divoom_system_free(phandle);
    divoom_vision_ctrl_close_handle();
    
    
}

