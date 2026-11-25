#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_photo_view.h"
#include "divoom_lvgl_play_gif_file.h"
#include "divoom_pixel_format.h"

#define DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_EXIT_TIME 2


#define DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_WIDTH  412
#define DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_HIGH   656

typedef struct _divoom_lvgl_accept_app_photo_view_para
{
    int cur_id;
    int total;
    int album_id;
    int pic_id;
}divoom_lvgl_accept_app_photo_view_para, *Pdivoom_lvgl_accept_app_photo_view_para;

typedef struct _divoom_lvgl_accept_app_photo_view_ctrl
{
    void *pview_handle;
    int menu_id;
    int pre_disp_time;
    int downloading_flag;
    int play_video_flag;
    int playing_video;
    void *main_obj;
    void *image_obj;
    void *prog_label_obj;
    void *play_speed_slide_obj;
    lv_img_dsc_t image_data;
    lv_img_dsc_t bg_image_data;
    void *play_gif_handle;
    struct _lv_timer_t * update_time_handle;
    divoom_lvgl_accept_app_photo_view_para para_info;
    Pdivoom_pic_phone_manage_slide_config pslide_config;
    uint8 image_buf[DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_WIDTH * DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_HIGH * 3];
    uint8 bg_image_buf[DIVOOM_SCREEN_WIDTH * DIVOOM_SCREEN_HIGH * 3];
}divoom_lvgl_accept_app_photo_view_ctrl, *Pdivoom_lvgl_accept_app_photo_view_ctrl;


int divoom_lvgl_accept_app_photo_view_read_image_info(Pdivoom_lvgl_accept_app_photo_view_ctrl phandle, uint8 *image_buf, int image_id)
{
    int video_flag;
    char file_name[256];
    uint8 *jpeg_buf;

    if(image_id == 0)
    {
        lv_obj_add_flag(phandle->image_obj, LV_OBJ_FLAG_HIDDEN);
        return 1;
    }
    lv_obj_clear_flag(phandle->image_obj, LV_OBJ_FLAG_HIDDEN);
    
    if(phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_pic_phone_manage_get_picture_name(file_name, image_id);
    }
    else
    {
        divoom_custom_photo_get_pic_name_from_id(image_id, file_name);
    }
    video_flag = strstr(file_name, "mp4") != NULL;
    
    if(strlen(file_name) == 0 || divoom_app_com_check_file_exist(file_name) == 0)
    {
        divoom_print("the file isn't exist!: %s, %d\n", file_name, image_id);
        return 0;
    }
    divoom_lvgl_play_gif_file_destory_handle(phandle->play_gif_handle, 0);
    
    //divoom_print("pic info: %d, %s, %d, %d, size: %d, %d\n", image_id, file_name, rotate_flag, mirror_flag, dest_w, dest_h);
    phandle->play_gif_handle = divoom_lvgl_play_gif_file_create_handle(file_name, phandle->image_obj, &phandle->image_data, DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_WIDTH, DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_HIGH, 0, 0);
    divoom_lvgl_play_gif_file_start_play(phandle->play_gif_handle);


    lv_img_set_src(phandle->image_obj, &phandle->image_data);
    lv_obj_invalidate(phandle->image_obj);
    return 1;
}

void divoom_lvgl_accept_app_photo_view_update_disp_info(struct _lv_timer_t * time_handle)
{
    int album_id, pic_id, cur_id, total;
    Pdivoom_lvgl_accept_app_photo_view_ctrl phandle = (Pdivoom_lvgl_accept_app_photo_view_ctrl)lv_timer_get_user_data(time_handle);
    
    phandle->downloading_flag = divoom_download_aliyun_file_get_app_last_info(&album_id, &pic_id, &total, &cur_id);

    if(phandle->para_info.album_id != album_id || phandle->para_info.pic_id != pic_id)
    {
        phandle->para_info.album_id = album_id;
        phandle->para_info.pic_id = pic_id;
        divoom_lvgl_accept_app_photo_view_read_image_info(phandle, phandle->image_buf, pic_id);
        phandle->pre_disp_time = divoom_system_get_ticks();
        if(phandle->pre_disp_time == 0)
        {
            phandle->pre_disp_time = 1;
        }
        
    }
    if(phandle->para_info.cur_id != cur_id)
    {
        phandle->para_info.cur_id = cur_id;
        if(phandle->para_info.cur_id > total)
        {
            total = phandle->para_info.cur_id;
        }
        if(phandle->para_info.total != total)
        {
            int slider_max;

            phandle->para_info.total = total;
            slider_max = phandle->para_info.total;
            if(slider_max <= 0)
            {
                slider_max = 1;
            }
            lv_slider_set_range(phandle->play_speed_slide_obj, 0, slider_max);
        }
        lv_label_set_text_fmt(phandle->prog_label_obj, "%s%d/%d",divoom_menu_control_get_disp_str("PicAccept"),phandle->para_info.cur_id, phandle->para_info.total);
        lv_slider_set_value(phandle->play_speed_slide_obj, phandle->para_info.cur_id, LV_ANIM_ON);
        phandle->pre_disp_time = divoom_system_get_ticks() ;
        if(phandle->pre_disp_time == 0)
        {
            phandle->pre_disp_time = 1;
        }
    }
    if(phandle->pre_disp_time && phandle->downloading_flag == 0)
    {
        int tick = (phandle->para_info.cur_id >= phandle->para_info.total)?1:10;
        if(phandle->pre_disp_time + DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_EXIT_TIME*tick < divoom_system_get_ticks())
        {
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
        }
    }
}


void Pdivoom_lvgl_accept_app_photo_view_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	
	switch(code)
	{
		case LV_EVENT_KEY:
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
            break;
        default:
            break;
	}

}

void *divoom_lvgl_accept_app_photo_view_init(void* user_data, int menu_id)
{
    Pdivoom_lvgl_accept_app_photo_view_ctrl phandle;

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_accept_app_photo_view_ctrl));
    phandle->menu_id = menu_id;
    
    phandle->downloading_flag = divoom_download_aliyun_file_get_app_last_info(&phandle->para_info.album_id, &phandle->para_info.pic_id, &phandle->para_info.total, &phandle->para_info.cur_id);

    phandle->main_obj = lv_btn_create(NULL);
    
    void *image_obj = lv_img_create(phandle->main_obj);
    lv_img_dsc_t * tmp_image_data = &phandle->bg_image_data;
    
    divoom_lv_img_dsc_init_fmt(tmp_image_data, DIVOOM_SCREEN_WIDTH,
        DIVOOM_SCREEN_HIGH, LV_COLOR_FORMAT_RGB565A8, phandle->bg_image_buf);
    lv_obj_set_pos(image_obj, 0, 0);
    lv_obj_set_size(image_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_lvgl_play_gif_file_create_handle("/usr/share/divoom_app/ui/2/app_wait.webp", image_obj, &phandle->bg_image_data, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 0, 0);
    lv_img_set_src(image_obj, &phandle->bg_image_data);

    phandle->image_obj = lv_img_create(phandle->main_obj);
    tmp_image_data = &phandle->image_data;
    
    divoom_lv_img_dsc_init_fmt(tmp_image_data,
        DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_WIDTH,
        DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_HIGH,
        LV_COLOR_FORMAT_RGB565A8, phandle->image_buf);
    lv_obj_set_pos(phandle->image_obj, 194, 327);
    lv_obj_set_size(phandle->image_obj, DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_WIDTH, DIVOOM_LVGL_ACCEPT_APP_PHOTO_VIEW_IMG_HIGH);
    lv_img_set_src(phandle->image_obj, &phandle->image_data);
    
    void *label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 38, 36);
	lv_obj_set_size(label_obj, 723, 160);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Waiting"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(54, phandle->menu_id), 0);  


    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 64, 1104);
	lv_obj_set_size(label_obj, 672, 54);
    lv_label_set_text_fmt(label_obj, "%s%d/%d",divoom_menu_control_get_disp_str("PicAccept"),phandle->para_info.cur_id, phandle->para_info.total);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);  
    phandle->prog_label_obj = label_obj;

    phandle->play_speed_slide_obj = lv_slider_create(phandle->main_obj);
    {
        int slider_max = phandle->para_info.total;

        if(slider_max <= 0)
        {
            slider_max = 1;
        }
        lv_slider_set_range(phandle->play_speed_slide_obj, 0, slider_max);
    }
    lv_obj_set_pos(phandle->play_speed_slide_obj, 120, 1066);
    lv_obj_set_size(phandle->play_speed_slide_obj, 560, 14);
    lv_slider_set_value(phandle->play_speed_slide_obj, phandle->para_info.cur_id, LV_ANIM_ON);
    lv_obj_set_style_bg_color(phandle->play_speed_slide_obj, lv_color_hex(0x75C4FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(phandle->play_speed_slide_obj, lv_color_hex(0xB6B6B6), LV_PART_MAIN);
    

    phandle->downloading_flag = divoom_download_aliyun_file_get_app_last_info(&phandle->para_info.album_id, &phandle->para_info.pic_id,&phandle->para_info.total, &phandle->para_info.cur_id);
    divoom_print("play info: %d, %d\n", phandle->para_info.album_id, phandle->para_info.pic_id);
    phandle->update_time_handle = lv_timer_create(divoom_lvgl_accept_app_photo_view_update_disp_info, 200, phandle);
    lv_timer_pause(phandle->update_time_handle);

    phandle->pre_disp_time = divoom_system_get_ticks();
    if(phandle->pre_disp_time == 0)
    {
        phandle->pre_disp_time = 1;
    }
	lv_obj_add_event_cb(phandle->main_obj, Pdivoom_lvgl_accept_app_photo_view_event_cb, LV_EVENT_KEY, phandle);

    return phandle;
}

void divoom_lvgl_accept_app_photo_view_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_accept_app_photo_view_ctrl phandle = (Pdivoom_lvgl_accept_app_photo_view_ctrl)handle;
    
    lv_group_focus_obj(phandle->main_obj);
    
    divoom_lvgl_accept_app_photo_view_read_image_info(phandle, phandle->image_buf, phandle->para_info.pic_id);
    lv_scr_load(phandle->main_obj);
    
    lv_timer_resume(phandle->update_time_handle);
    phandle->pre_disp_time = divoom_system_get_ticks();
    
}

void divoom_lvgl_accept_app_photo_view_pause(void *handle)
{
    Pdivoom_lvgl_accept_app_photo_view_ctrl phandle = (Pdivoom_lvgl_accept_app_photo_view_ctrl)handle;
    
    
    lv_timer_pause(phandle->update_time_handle);
    
    if(phandle->play_video_flag && phandle->playing_video)
    {
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        divoom_local_movie_play_stop_movie();
        phandle->playing_video = 0;
    }
    
}

void divoom_lvgl_accept_app_photo_view_exit(void *handle)
{
    Pdivoom_lvgl_accept_app_photo_view_ctrl phandle = (Pdivoom_lvgl_accept_app_photo_view_ctrl)handle;

    lv_timer_pause(phandle->update_time_handle);
    lv_timer_del(phandle->update_time_handle);
    divoom_lvgl_play_gif_file_destory_handle(phandle->play_gif_handle, 0);
    phandle->play_gif_handle = NULL;    
    lv_obj_del(phandle->main_obj);
    divoom_system_free(phandle);
}
