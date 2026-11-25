#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct  _divoom_lvgl_bt_music_menu_ctrl
{
    int  menu_id;
    int  play_status;
    int  music_name_flag;
    int  music_title_flag;
    char music_name[256];
    char music_title[256];
    void *main_obj;
    void *back_obj;
    void  *music_album_img_obj;
    void *volume_obj;
    void *pre_obj;
    void *next_obj;
    void *play_obj;
    void *lyric_obj;
    void *music_name_obj;
    void *music_title_obj;
    void *time_handle;
    lv_style_t cout_style;
}divoom_lvgl_bt_music_menu_ctrl, *Pdivoom_lvgl_bt_music_menu_ctrl;


static void divoom_lvgl_bt_music_main_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_bt_music_menu_ctrl phandle = (Pdivoom_lvgl_bt_music_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_CLICKED)
    {
        if(cur_obj == phandle->back_obj)
        {
            divoom_menu_control_return_pre();
            return;
        }
        else if(cur_obj == phandle->pre_obj)
        {
            divoom_music_ctrl_play_pre();
        }
        else if(cur_obj == phandle->next_obj)
        {
            divoom_music_ctrl_play_next();
        }
        else if(cur_obj == phandle->play_obj)
        {
            phandle->play_status = phandle->play_status?0:1;
            divoom_music_ctrl_set_play(phandle->play_status);
            if(phandle->play_status)
            {
                divoom_menu_control_lv_img_set_src(phandle->play_obj, "33/33_pause.png");
            }
            else
            {
                divoom_menu_control_lv_img_set_src(phandle->play_obj, "33/33_play.png");
            }
        }
        else if(cur_obj == phandle->volume_obj)
        {
            int volume = lv_slider_get_value(cur_obj);
            divoom_bt_music_set_play_vol((uint8)volume);
        }
        else if(cur_obj == phandle->lyric_obj)
        {
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_LYRIC_MENU, NULL, 0, 0);
        
        }
    }
}

void divoom_lvgl_bt_music_menu_time_loop(struct _lv_timer_t * ptime_info)
{
    Pdivoom_lvgl_bt_music_menu_ctrl phandle = (Pdivoom_lvgl_bt_music_menu_ctrl)lv_timer_get_user_data(ptime_info);
    int cur_play_status = divoom_bt_ctrl_get_music_status();

    if(cur_play_status != phandle->play_status)
    {
        phandle->play_status = cur_play_status;
        if(phandle->play_status)
        {
            divoom_menu_control_lv_img_set_src(phandle->play_obj, "33/33_pause.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(phandle->play_obj, "33/33_play.png");
        }
    }
    
    //比较歌词，如果发生变化就更新
    if(phandle->music_name_flag)
    {
        lv_label_set_text(phandle->music_name_obj, phandle->music_name);
        phandle->music_name_flag = 0;
    }
    
    if(phandle->music_title_flag)
    {
        lv_label_set_text(phandle->music_title_obj, phandle->music_title);
        phandle->music_title_flag = 0;
    }
}

void divoom_lvgl_bt_music_menu_get_music_info(void *user_data, char *singer_name, char *music_name, char *music_title, int play_time)
{
    Pdivoom_lvgl_bt_music_menu_ctrl phandle = (Pdivoom_lvgl_bt_music_menu_ctrl)user_data;

    if(strcmp(phandle->music_name, music_name))
    {
        strcpy(phandle->music_name, music_name);
        phandle->music_name_flag = 1;
    }
    
    if(strcmp(phandle->music_title, music_title))
    {
        strcpy(phandle->music_title, music_title);
        phandle->music_title_flag = 1;
    }
}


void *divoom_lvgl_bt_music_menu_init(void *user_data, int menu_id)
{
    void *slide_obj, *label_obj;
    Pdivoom_lvgl_bt_music_menu_ctrl phandle;

    divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_BT);
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_bt_music_menu_ctrl));
    phandle->menu_id = menu_id;
    phandle->main_obj = lv_img_create(NULL);
    divoom_menu_control_init_lable_style(&phandle->cout_style);
    lv_obj_add_style(phandle->main_obj, &phandle->cout_style, 0);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_menu_control_lv_img_set_src(phandle->main_obj, "33/33_bg.png");
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
 
    phandle->back_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->back_obj, 30, 30);
    lv_obj_set_size(phandle->back_obj, 78, 78);
    lv_obj_add_flag(phandle->back_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(phandle->back_obj, "26/26_back.png");
    lv_obj_add_event(phandle->back_obj, divoom_lvgl_bt_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 160, 38);
	lv_obj_set_size(label_obj, 480, 56);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("BlueMusic"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(40, phandle->menu_id), 0); 

    phandle->music_album_img_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(phandle->music_album_img_obj, 90, 130);
	lv_obj_set_size(phandle->music_album_img_obj, 620, 620);
    divoom_menu_control_lv_img_set_src(phandle->music_album_img_obj, "33/33_bt_music_default.png");

    slide_obj = lv_slider_create(phandle->main_obj);
    lv_obj_set_style_opa(slide_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 153, 984);
    lv_obj_set_size(slide_obj, 494, 16);
    lv_slider_set_value(slide_obj, divoom_bt_music_get_play_vol(), LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
    lv_obj_add_flag(slide_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(slide_obj, divoom_lvgl_bt_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    phandle->volume_obj = slide_obj;
    

    phandle->pre_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->pre_obj, 194, 1071);
    lv_obj_set_size(phandle->pre_obj, 120, 120);
    lv_obj_add_flag(phandle->pre_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(phandle->pre_obj, "33/33_pre.png");
    lv_obj_add_event(phandle->pre_obj, divoom_lvgl_bt_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    phandle->next_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->next_obj, 498, 1071);
    lv_obj_set_size(phandle->next_obj, 120, 120);
    lv_obj_add_flag(phandle->next_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(phandle->next_obj, "33/33_next.png");
    lv_obj_add_event(phandle->next_obj, divoom_lvgl_bt_music_main_event_cb, LV_EVENT_CLICKED, phandle);

    phandle->play_status = divoom_bt_ctrl_get_music_status();
    phandle->play_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->play_obj, 346, 1071);
    lv_obj_set_size(phandle->play_obj, 120, 120);
    lv_obj_add_flag(phandle->play_obj, LV_OBJ_FLAG_CLICKABLE);   
    if(phandle->play_status)
    {
        divoom_menu_control_lv_img_set_src(phandle->play_obj, "33/33_pause.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(phandle->play_obj, "33/33_play.png");
    }
    lv_obj_add_event(phandle->play_obj, divoom_lvgl_bt_music_main_event_cb, LV_EVENT_CLICKED, phandle);


    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 90, 872);
	lv_obj_set_size(label_obj, 654, 37);
    lv_label_set_text(label_obj, "");
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xABABAB), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(30, phandle->menu_id), 0); 
    phandle->music_title_obj = label_obj; 

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 73, 814);
	lv_obj_set_size(label_obj, 654, 40);
    lv_label_set_text(label_obj, "");
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(34, phandle->menu_id), 0); 
    phandle->music_name_obj = label_obj; 
    divoom_bt_music_lrc_set_callback(divoom_lvgl_bt_music_menu_get_music_info, phandle);
    phandle->time_handle = lv_timer_create(divoom_lvgl_bt_music_menu_time_loop, 1000, phandle);
    lv_timer_pause(phandle->time_handle);
    return  phandle;
    
}

void divoom_lvgl_bt_music_menu_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_bt_music_menu_ctrl phandle = (Pdivoom_lvgl_bt_music_menu_ctrl)handle;

    lv_timer_resume(phandle->time_handle);
    lv_scr_load(phandle->main_obj);
    
}

void divoom_lvgl_bt_music_menu_pause(void *handle)
{
    Pdivoom_lvgl_bt_music_menu_ctrl phandle = (Pdivoom_lvgl_bt_music_menu_ctrl)handle;
    
    lv_timer_pause(phandle->time_handle);
}

void divoom_lvgl_bt_music_menu_exit(void *handle)
{
    Pdivoom_lvgl_bt_music_menu_ctrl phandle = (Pdivoom_lvgl_bt_music_menu_ctrl)handle;
    
    divoom_bt_music_lrc_set_callback(NULL, NULL);
    lv_timer_del(phandle->time_handle);
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->cout_style);
    divoom_system_free(phandle);
}


