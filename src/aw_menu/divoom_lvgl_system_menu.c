#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_select_album_play.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_pixel_format.h"

typedef struct _divoom_lvgl_system_menu_ctrl
{
    int menu_id;
    int lang;
    int clock_id;
    void *label_lang_table_obj[10];
    void *main_obj;
    void *clock_second_obj;
    void *clock_min_obj;
    void *clock_hour_obj;
    void *month_day_obj;
    void *hour_obj;

    void *digit_album_obj;
    void *lyric_obj;
    void *lyric_name_obj;   //歌名
    void *lyric_lyric_obj;   //歌词
    void *lyric_pre_obj;
    void *lyric_next_obj;
    void *lyric_play_obj;
    int   play_status;
    
    void *dial_obj;
    void *dial_label_obj;
    void *dial_img_obj;
    void *album_obj;
    void *album_name_obj;
    void *album_image_obj;
    void *alarm_obj;
    void *alarm_name_obj;
    void *alarm_time_obj;
    void *alarm_switch_obj;
    void *white_noise_obj;
    void *white_noise_time_obj;
    void *white_noise_switch_obj;
    void *more_obj;
    lv_style_t cout_style;

    int  disp_album_id;
    lv_img_dsc_t *album_img_info;
    lv_img_dsc_t *clock_img_info;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO palarm_info;
    struct tm disp_time_info;
    
    void *time_handle;
}divoom_lvgl_system_menu_ctrl, *Pdivoom_lvgl_system_menu_ctrl;

void divoom_lvgl_system_menu_read_album_image(Pdivoom_lvgl_system_menu_ctrl phandle, int album_id);

const char *gdivoom_week_day_info[] = 
{
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Firday",
    "Saturday",
    "Sunday",
};

void divoom_lvgl_system_menu_refresh_menu(Pdivoom_lvgl_system_menu_ctrl phandle)
{
    int time_update = 0;
    time_t cur_time;
	struct tm time_now; 
    Pdivoom_pic_phone_manage_slide_config pslide_config;

    cur_time = time(0);
    
	localtime_r(&cur_time, &time_now);
    time_now.tm_year += 1900;
	time_now.tm_mon ++;
    
    if(phandle->disp_time_info.tm_hour != time_now.tm_hour)
    {
        lv_img_set_angle(phandle->clock_hour_obj, time_now.tm_hour%12 * (3600/12));
        time_update = 1;
        phandle->disp_time_info.tm_hour = time_now.tm_hour;        
    }
    
    if(phandle->disp_time_info.tm_min != time_now.tm_min)
    {
        lv_img_set_angle(phandle->clock_min_obj, time_now.tm_min * (3600/60));
        time_update = 1;
        phandle->disp_time_info.tm_min = time_now.tm_min;        
    }
    if(time_update)
    {
        int hour = time_now.tm_hour;
        
        if(divoom_main_get_time_flag() == 0)
        {
            if(hour != 12)
            {
                hour = hour % 12;
                if(hour == 0)
                {
                    hour = 12;
                }
            }
        }
        lv_label_set_text_fmt(phandle->hour_obj, "%d:%02d", hour, time_now.tm_min);
    }
    
    if(phandle->disp_time_info.tm_sec != time_now.tm_sec)
    {
        lv_img_set_angle(phandle->clock_second_obj, time_now.tm_sec * (3600/60));
        phandle->disp_time_info.tm_sec = time_now.tm_sec;        
    }
    
    if(phandle->disp_time_info.tm_mday != time_now.tm_mday)
    {
        lv_label_set_text_fmt(phandle->month_day_obj, "%d %s", time_now.tm_mday, divoom_menu_control_get_disp_str(gdivoom_week_day_info[time_now.tm_wday]));     
        phandle->disp_time_info.tm_mday = time_now.tm_mday;
    }

    
    //更新相簿
    pslide_config = divoom_pic_phone_manage_get_slide_config();
    divoom_lvgl_system_menu_read_album_image(phandle, pslide_config->album_id);

    //更新氛围歌词
    if(phandle->play_status != divoom_main_get_play_music_status())
    {
        phandle->play_status = phandle->play_status?0:1;
        if(phandle->play_status)
        {
            divoom_menu_control_lv_img_set_src(phandle->lyric_play_obj, "1/1_pause.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(phandle->lyric_play_obj, "1/1_play.png");
        }
    }
    lv_label_set_text(phandle->lyric_lyric_obj, divoom_main_get_music_lyric());
    lv_label_set_text(phandle->lyric_name_obj, divoom_main_get_music_name());

    //更新闹钟的信息
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info = divoom_alarm_get_last_alarm_info();

    if(palarm_info)
    {
        if(palarm_info->hour !=  phandle->palarm_info.hour || palarm_info->minute !=  phandle->palarm_info.minute  )
        {
            lv_label_set_text_fmt(phandle->alarm_time_obj, "%d:%d",palarm_info->hour, palarm_info->minute);
        }


        if(strcmp(palarm_info->alarm_name, phandle->palarm_info.alarm_name))
        {
            lv_label_set_text(phandle->alarm_name_obj, palarm_info->alarm_name);
        }
        
        if(phandle->palarm_info.on_off != palarm_info->on_off)
        {
            if(palarm_info->on_off)
            {
                divoom_menu_control_lv_img_set_src(phandle->alarm_switch_obj, "1/1_on.png");
            }
            else
            {
                divoom_menu_control_lv_img_set_src(phandle->alarm_switch_obj, "1/1_off.png");
            }
        }
        memcpy(&phandle->palarm_info, palarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
    }
    else
    {
        lv_label_set_text(phandle->alarm_time_obj, "--:--");
        lv_label_set_text(phandle->alarm_name_obj, "-----");
        divoom_menu_control_lv_img_set_src(phandle->alarm_switch_obj, "1/1_off.png");
    }
    
    
}

void divoom_lvgl_system_menu_btn_event(lv_event_t * e)
{
    Pdivoom_lvgl_system_menu_ctrl phandle = (Pdivoom_lvgl_system_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    void *cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        Pdivoom_pic_phone_manage_slide_config pslide_config;
        if(cur_obj == phandle->album_obj)
        {
            //进入幻灯片播放
            /*
            Pdivoom_lvgl_photo_slide_effect_para para = divoom_system_malloc(sizeof(divoom_lvgl_photo_slide_effect_para));
            
            pslide_config = divoom_pic_phone_manage_get_slide_config();
            pslide_config->album_id = divoom_pic_album_manage_check_album_id(pslide_config->album_id);            
            para->album_id = pslide_config->album_id;
            para->slide_type = pslide_config->effect_type;
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT, para, 0, 0);
            */
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_ENTER_ALBUM_PLAY);
            
        }
        else if(cur_obj == phandle->dial_obj)
        {
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_SET_NEXT_CLOCK);
            //divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, NULL, 0, 0);
            
        }
        else if(cur_obj == phandle->alarm_obj)
        {
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU, NULL, 0, 0);
            
        }
        else if(cur_obj == phandle->alarm_switch_obj)
        {
            //更新闹钟开关
            phandle->palarm_info.on_off = phandle->palarm_info.on_off?0:1;
            if(phandle->palarm_info.on_off)
            {
                divoom_menu_control_lv_img_set_src(phandle->alarm_switch_obj, "1/1_on.png");
            }
            else
            {
                divoom_menu_control_lv_img_set_src(phandle->alarm_switch_obj, "1/1_off.png");
            }
            divoom_alarm_menu_update_alarm_item(&phandle->palarm_info);
            
        }
        else if(cur_obj == phandle->lyric_obj)
        {
            //进入播放页面
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_LYRIC_MENU, NULL, 0, 0);
        }
        else if(cur_obj == phandle->lyric_next_obj)
        {
            //播放下一首
        }
        else if(cur_obj == phandle->lyric_pre_obj)
        {
            //播放上一首
        }
        else if(cur_obj == phandle->lyric_play_obj)
        {
            //播放与暂停
        }
        else if(cur_obj == phandle->more_obj)
        {
            //进入更多页面
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_MORE_FUNCTION, NULL, 0, 0);
        }
    }
}

int divoom_lvgl_system_menu_get_first_pic_data_from_album(void *image_obj, lv_img_dsc_t *new_img_info, int album_id)
{
    int image_w = 232, image_h = 232, first_pic_id;

    first_pic_id = divoom_pic_phone_manage_get_first_pic_id(album_id);
    if(first_pic_id < 0)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "15/photo_empty_album.png");  
        return 0;
    }
    
    divoom_pic_album_manage_read_pic(NULL, 0, first_pic_id, 1,
        (uint8 *)new_img_info->data, image_w, image_h, NULL, NULL);
    
    lv_img_set_src(image_obj, new_img_info);
    
    
    return 1;
}

void divoom_lvgl_system_menu_read_clock_image(Pdivoom_lvgl_system_menu_ctrl phandle, int clock_id)
{
    int image_w, image_h;
    uint8 *image_buf = NULL;
    char file_name[128];
    lv_img_dsc_t *new_img_info;

    
    
    if(phandle->clock_img_info)
    {
        image_buf = (uint8 *)phandle->clock_img_info->data;
    }
    new_img_info = divoom_system_malloc(sizeof(lv_img_dsc_t));
    divoom_system_free(phandle->clock_img_info);
    phandle->clock_img_info = new_img_info;

    if(image_buf == NULL)
    {
        image_buf = divoom_system_malloc(divoom_pixel_format_size(232, 232, LV_COLOR_FORMAT_RGB565A8));
    }
    
    divoom_lv_img_dsc_init_fmt(phandle->clock_img_info, 232, 232, LV_COLOR_FORMAT_RGB565A8, image_buf);
	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_REVIEW_PIC, DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_REVIEW_PIC + clock_id, file_name);
    uint8 *tmp_buf = divoom_rk_mpp_get_png_info(file_name, &image_w, &image_h);
    if(tmp_buf)
    {
        divoom_rk_mpp_stretch_image_lvfmt(tmp_buf, image_w, image_h,
            RK_FORMAT_BGRA_8888, image_buf, 232, 232, LV_COLOR_FORMAT_RGB565A8);
        divoom_system_free(tmp_buf);
        lv_img_set_src(phandle->dial_img_obj, phandle->clock_img_info);
    }
    else
    {
        divoom_menu_control_lv_img_set_src(phandle->dial_img_obj, "15/15_empty_album.png");        
    }
    phandle->clock_id = clock_id;
}

void divoom_lvgl_system_menu_read_album_image(Pdivoom_lvgl_system_menu_ctrl phandle, int album_id)
{
    uint8 *image_buf = NULL;
    lv_img_dsc_t *new_img_info;

    if(phandle->disp_album_id == album_id)
    {
        return;
    }
    
    
    if(phandle->album_img_info)
    {
        image_buf = (uint8 *)phandle->album_img_info->data;
    }
    new_img_info = divoom_system_malloc(sizeof(lv_img_dsc_t));
    divoom_system_free(phandle->album_img_info);
    phandle->album_img_info = new_img_info;
    
    lv_label_set_text(phandle->album_name_obj, divoom_pic_album_manage_get_static_album_name(album_id));
    
    if(image_buf == NULL)
    {
        image_buf = divoom_system_malloc(divoom_pixel_format_size(232, 232, LV_COLOR_FORMAT_RGB565A8));
    }
    
    divoom_lv_img_dsc_init_fmt(phandle->album_img_info, 232, 232, LV_COLOR_FORMAT_RGB565A8, image_buf);
    
    divoom_lvgl_system_menu_get_first_pic_data_from_album(phandle->album_image_obj, phandle->album_img_info, album_id);
    phandle->disp_album_id = album_id;
    
}

void divoom_lvgl_system_menu_time_handle(struct _lv_timer_t *ptime_info)
{
    Pdivoom_lvgl_system_menu_ctrl phandle = (Pdivoom_lvgl_system_menu_ctrl)lv_timer_get_user_data(ptime_info);

    divoom_lvgl_system_menu_refresh_menu(phandle);
}

void *divoom_lvgl_system_menu_init(void *user_data, int menu_id)
{
    int clock_id;
    void *cout_obj, *image_obj, *label_obj;
    Pdivoom_lvgl_system_menu_ctrl phandle;
    Pdivoom_pic_phone_manage_slide_config pslide_config;
    Pdivoom_clock_manage_item pclock_info;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_system_menu_ctrl));
    phandle->menu_id = menu_id;
    phandle->lang = psys_config->lang;
    phandle->disp_album_id = -1;
    divoom_menu_control_init_lable_style(&phandle->cout_style);
    phandle->main_obj = lv_img_create(NULL);
    lv_obj_add_style(phandle->main_obj, &phandle->cout_style, 0);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_menu_control_lv_img_set_src(phandle->main_obj, "1/1_bg.png");

    cout_obj = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(cout_obj, &phandle->cout_style, 0);
    lv_obj_set_pos(cout_obj, 0, 0);
	lv_obj_set_size(cout_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(cout_obj, lv_color_hex(0), 0);
    lv_obj_set_style_bg_opa(cout_obj, LV_OPA_TRANSP,  0);
    lv_obj_set_scrollbar_mode(cout_obj, LV_SCROLLBAR_MODE_OFF);
    


    image_obj = lv_img_create(cout_obj);
    lv_obj_set_pos(image_obj, 40, 33);
	lv_obj_set_size(image_obj, 230, 230);
    divoom_menu_control_lv_img_set_src(image_obj, "1/1_clock.png");
    
    phandle->clock_second_obj = lv_img_create(image_obj);
    lv_obj_set_pos(phandle->clock_second_obj, 0, 0);
	lv_obj_set_size(phandle->clock_second_obj, 230, 230);
    divoom_menu_control_lv_img_set_src(phandle->clock_second_obj, "1/1_second.png");
    
    phandle->clock_min_obj = lv_img_create(image_obj);
    lv_obj_set_pos(phandle->clock_min_obj, 0, 0);
	lv_obj_set_size(phandle->clock_min_obj, 230, 230);
    divoom_menu_control_lv_img_set_src(phandle->clock_min_obj, "1/1_min.png");
    
    phandle->clock_hour_obj = lv_img_create(image_obj);
    lv_obj_set_pos(phandle->clock_hour_obj, 0, 0);
	lv_obj_set_size(phandle->clock_hour_obj, 230, 230);
    divoom_menu_control_lv_img_set_src(phandle->clock_hour_obj, "1/1_hour.png");

    
    label_obj = lv_label_create(cout_obj);
    phandle->month_day_obj = label_obj;
    lv_obj_set_pos(label_obj, 203, 33);
	lv_obj_set_size(label_obj, 538, 72);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xE1E1E1), 0); 
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(66, phandle->menu_id), 0); 
    lv_label_set_text(label_obj, "26 Monday");
      
    label_obj = lv_label_create(cout_obj);
    phandle->hour_obj = label_obj;
    lv_obj_set_pos(label_obj, 256, 108);
	lv_obj_set_size(label_obj, 504, 166);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xE1E1E1), 0); 
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(154, phandle->menu_id), 0); 
    lv_label_set_text(label_obj, "12:00");
      
    
    //创建表盘
    label_obj = lv_label_create(cout_obj);
    lv_obj_set_pos(label_obj, 45, 300);
	lv_obj_set_size(label_obj, 305, 61);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xF1F1F1), 0); 
    phandle->label_lang_table_obj[0] = label_obj;
    lv_label_set_text(phandle->label_lang_table_obj[0], divoom_menu_control_get_disp_str("Dial"));
    
    image_obj = lv_img_create(cout_obj);
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    phandle->dial_obj = image_obj;
    lv_obj_set_pos(image_obj, 25, 376);
	lv_obj_set_size(image_obj, 750, 331);
    divoom_menu_control_lv_img_set_src(image_obj, "1/1_cout_bg.png");
    
    
    pslide_config = divoom_pic_phone_manage_get_slide_config();
    label_obj = lv_label_create(phandle->album_obj);
    phandle->dial_label_obj = label_obj;
    lv_obj_set_pos(label_obj, 75, 135);
	lv_obj_set_size(label_obj, 300, 61);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0); 
    clock_id = divoom_light_app_get_cur_clock_id();
    phandle->clock_id = clock_id;
    pclock_info = divoom_clock_manage_get_clock_info(clock_id, 0, 0);
    if(pclock_info)
    {
        lv_label_set_text(label_obj, psys_config->lang == DIVOOM_LVGL_MENU_LANG_SC?pclock_info->name_cn:pclock_info->name_en);
    }
    else
    {
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("WaitPlay"));
    }
    divoom_clock_manage_free_clock(pclock_info);
    
    image_obj = lv_img_create(phandle->dial_obj);
    phandle->dial_img_obj = image_obj;
    lv_obj_set_pos(image_obj, 444, 41);
	lv_obj_set_size(image_obj, 232, 232);
	divoom_lvgl_system_menu_read_clock_image(phandle, clock_id);
  

    //创建电子相册的UI
    label_obj = lv_label_create(cout_obj);
    lv_obj_set_pos(label_obj, 45, 735);
	lv_obj_set_size(label_obj, 305, 61);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xF1F1F1), 0); 
    phandle->label_lang_table_obj[0] = label_obj;
    lv_label_set_text(phandle->label_lang_table_obj[0], divoom_menu_control_get_disp_str("DigitAlbum"));
    
    image_obj = lv_img_create(cout_obj);
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    phandle->album_obj = image_obj;
    lv_obj_set_pos(image_obj, 25, 811);
	lv_obj_set_size(image_obj, 750, 331);
    divoom_menu_control_lv_img_set_src(image_obj, "1/1_cout_bg.png");
    
    
    pslide_config = divoom_pic_phone_manage_get_slide_config();
    label_obj = lv_label_create(phandle->album_obj);
    phandle->album_name_obj = label_obj;
    lv_obj_set_pos(label_obj, 75, 135);
	lv_obj_set_size(label_obj, 300, 61);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0); 
    
    //lv_label_set_text(label_obj, divoom_pic_album_manage_get_static_album_name(pslide_config->album_id));
    
    
    image_obj = lv_img_create(phandle->album_obj);
    phandle->album_image_obj = image_obj;
    lv_obj_set_pos(image_obj, 444, 41);
	lv_obj_set_size(image_obj, 232, 232);
    
    divoom_lvgl_system_menu_read_album_image(phandle, pslide_config->album_id);
    
      
    
    //创建氛围歌词的UI
    label_obj = lv_label_create(cout_obj);
    lv_obj_set_pos(label_obj, 45, 1170);
	lv_obj_set_size(label_obj, 305, 61);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xF1F1F1), 0); 
    phandle->label_lang_table_obj[1] = label_obj;
    lv_label_set_text(phandle->label_lang_table_obj[1], divoom_menu_control_get_disp_str("Lyric"));

    image_obj = lv_img_create(cout_obj);
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    phandle->lyric_obj = image_obj;
    lv_obj_set_pos(image_obj, 25, 1246);
	lv_obj_set_size(image_obj, 750, 331);
    divoom_menu_control_lv_img_set_src(phandle->lyric_obj, "1/1_cout_bg.png");
    

    label_obj = lv_label_create(phandle->lyric_obj);
    phandle->lyric_name_obj = label_obj;
    lv_obj_set_pos(label_obj, 48, 33);
	lv_obj_set_size(label_obj, 670, 61);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0); 
    lv_label_set_text(label_obj, divoom_main_get_music_name());

    label_obj = lv_label_create(phandle->lyric_obj);
    phandle->lyric_lyric_obj = label_obj;
    lv_obj_set_pos(label_obj, 48, 115);
	lv_obj_set_size(label_obj, 670, 47);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(36, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0); 
    lv_label_set_text(label_obj, divoom_main_get_music_lyric());

    image_obj = lv_img_create(phandle->lyric_obj);
    phandle->lyric_pre_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    lv_obj_set_pos(image_obj, 126, 181);
	lv_obj_set_size(image_obj, 150, 150);
    divoom_menu_control_lv_img_set_src(phandle->lyric_pre_obj, "1/1_pre.png");

    

    image_obj = lv_img_create(phandle->lyric_obj);
    phandle->lyric_play_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    lv_obj_set_pos(image_obj, 300, 181);
	lv_obj_set_size(image_obj, 150, 150);
    phandle->play_status = divoom_main_get_play_music_status();
    if(phandle->play_status)
    {
        divoom_menu_control_lv_img_set_src(phandle->lyric_play_obj, "1/1_pause.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(phandle->lyric_play_obj, "1/1_play.png");
    }

    
    image_obj = lv_img_create(phandle->lyric_obj);
    phandle->lyric_next_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    lv_obj_set_pos(image_obj, 474, 181);
	lv_obj_set_size(image_obj, 150, 150);
    divoom_menu_control_lv_img_set_src(phandle->lyric_next_obj, "1/1_next.png");

    
    //创建闹钟的UI
    label_obj = lv_label_create(cout_obj);
    lv_obj_set_pos(label_obj, 45, 1605);
	lv_obj_set_size(label_obj, 305, 61);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xF1F1F1), 0); 
    phandle->label_lang_table_obj[2] = label_obj;
    lv_label_set_text(phandle->label_lang_table_obj[2], divoom_menu_control_get_disp_str("Alarm"));

    image_obj = lv_img_create(cout_obj);
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    phandle->alarm_obj = image_obj;
    lv_obj_set_pos(image_obj, 25, 1681);
	lv_obj_set_size(image_obj, 750, 331);
    divoom_menu_control_lv_img_set_src(phandle->alarm_obj, "1/1_cout_bg.png");
    
    
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info = divoom_alarm_get_last_alarm_info();

    label_obj = lv_label_create(phandle->alarm_obj);
    phandle->alarm_time_obj = label_obj;
    lv_obj_set_pos(label_obj, 70, 55);
	lv_obj_set_size(label_obj, 315, 130);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(120, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0); 
    if(palarm_info)
    {
        lv_label_set_text_fmt(label_obj, "%d:%d",palarm_info->hour, palarm_info->minute);
    }
    else
    {
        lv_label_set_text(label_obj, "--:--");
    }

    label_obj = lv_label_create(phandle->alarm_obj);
    phandle->alarm_name_obj = label_obj;
    lv_obj_set_pos(label_obj, 55, 195);
	lv_obj_set_size(label_obj, 298, 61);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0); 
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0); 
    if(palarm_info)
    {
        lv_label_set_text(label_obj, palarm_info->alarm_name);
    }
    else
    {
        lv_label_set_text(label_obj, "-----");
    }

    image_obj = lv_img_create(phandle->alarm_obj);
    phandle->alarm_switch_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    lv_obj_set_pos(image_obj, 491, 91);
	lv_obj_set_size(image_obj, 185, 118);
    if(palarm_info && palarm_info->on_off)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "1/1_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "1/1_off.png");
    }
    

    
    //创建白噪音的UI
    label_obj = lv_label_create(cout_obj);
    lv_obj_set_pos(label_obj, 45, 2040);
	lv_obj_set_size(label_obj, 305, 61);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xF1F1F1), 0); 
    phandle->label_lang_table_obj[3] = label_obj;
    lv_label_set_text(phandle->label_lang_table_obj[3], divoom_menu_control_get_disp_str("WhiteNoise"));

    image_obj = lv_img_create(cout_obj);
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    phandle->white_noise_obj = image_obj;
    lv_obj_set_pos(image_obj, 25, 2116);
	lv_obj_set_size(image_obj, 750, 331);
    divoom_menu_control_lv_img_set_src(phandle->white_noise_obj, "1/1_cout_bg.png");
    

    image_obj = lv_img_create(phandle->white_noise_obj);
    phandle->white_noise_switch_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    lv_obj_set_pos(image_obj, 491, 91);
	lv_obj_set_size(image_obj, 185, 118);
    divoom_menu_control_lv_img_set_src(image_obj, "1/1_off.png");
    
    image_obj = lv_img_create(phandle->white_noise_obj);
    lv_obj_set_pos(image_obj, 84, 68);
	lv_obj_set_size(image_obj, 171, 125);
    divoom_menu_control_lv_img_set_src(image_obj, "1/1_sleep.png");
    
    label_obj = lv_label_create(phandle->white_noise_obj);
    phandle->white_noise_time_obj = label_obj;
    lv_obj_set_pos(label_obj, 42, 203);
	lv_obj_set_size(label_obj, 254, 61);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0); 
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0); 
    lv_label_set_text(label_obj, "15 mins");

    //更多
    image_obj = lv_img_create(cout_obj);
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_menu_btn_event, LV_EVENT_KEY, phandle);
    phandle->more_obj = image_obj;
    lv_obj_set_pos(image_obj, 70, 2507);
	lv_obj_set_size(image_obj, 660, 220);
    divoom_menu_control_lv_img_set_src(phandle->more_obj, "1/1_last_cout.png");
    

    label_obj = lv_label_create(phandle->more_obj);
    lv_obj_set_pos(label_obj, 272, 59);
	lv_obj_set_size(label_obj, 223, 153);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(76, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0); 
    phandle->label_lang_table_obj[4] = label_obj;
    lv_label_set_text(phandle->label_lang_table_obj[4], divoom_menu_control_get_disp_str("More"));
    
    phandle->time_handle = lv_timer_create(divoom_lvgl_system_menu_time_handle, 1000, phandle);
    lv_timer_pause(phandle->time_handle);
    
    return  phandle;
}

void divoom_lvgl_system_menu_disp(void *handle, void *active_obj)
{    
    Pdivoom_clock_manage_item pclock_info;
    Pdivoom_lvgl_system_menu_ctrl phandle = (Pdivoom_lvgl_system_menu_ctrl)handle;    
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    if(phandle->clock_id != divoom_light_app_get_cur_clock_id())
    {
        
        phandle->clock_id = divoom_light_app_get_cur_clock_id();
        pclock_info = divoom_clock_manage_get_clock_info(phandle->clock_id, 0, 0);
        
        lv_label_set_text(phandle->dial_label_obj, psys_config->lang == DIVOOM_LVGL_MENU_LANG_SC?pclock_info->name_cn:pclock_info->name_en);
        divoom_clock_manage_free_clock(pclock_info);        
        divoom_lvgl_system_menu_read_clock_image(phandle, phandle->clock_id);
        
    }
    if(psys_config->lang != phandle->lang)
    {
        phandle->lang = psys_config->lang;
        
        pclock_info = divoom_clock_manage_get_clock_info(phandle->clock_id, 0, 0);        
        lv_label_set_text(phandle->dial_label_obj, psys_config->lang == DIVOOM_LVGL_MENU_LANG_SC?pclock_info->name_cn:pclock_info->name_en);
        divoom_clock_manage_free_clock(pclock_info);
        
        lv_label_set_text(phandle->label_lang_table_obj[0], divoom_menu_control_get_disp_str("Dial"));
        lv_label_set_text(phandle->label_lang_table_obj[1], divoom_menu_control_get_disp_str("DigitAlbum"));
        lv_label_set_text(phandle->label_lang_table_obj[2], divoom_menu_control_get_disp_str("Lyric"));
        lv_label_set_text(phandle->label_lang_table_obj[3], divoom_menu_control_get_disp_str("Alarm"));
        lv_label_set_text(phandle->label_lang_table_obj[4], divoom_menu_control_get_disp_str("WhiteNoise"));
        lv_label_set_text(phandle->label_lang_table_obj[5], divoom_menu_control_get_disp_str("More"));
        
    }
    lv_timer_resume(phandle->time_handle);
    divoom_lvgl_system_menu_refresh_menu(phandle);
    lv_scr_load(phandle->main_obj);
}


void divoom_lvgl_system_menu_pause(void *handle)
{
    Pdivoom_lvgl_system_menu_ctrl phandle = (Pdivoom_lvgl_system_menu_ctrl)handle;
    
    lv_timer_pause(phandle->time_handle);
}


void divoom_lvgl_system_menu_exit(void *handle)
{
    Pdivoom_lvgl_system_menu_ctrl phandle = (Pdivoom_lvgl_system_menu_ctrl)handle;

    lv_timer_pause(phandle->time_handle);
    lv_timer_del(phandle->time_handle);
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->cout_style);
    if(phandle->album_img_info)
    {
        divoom_system_free((uint8 *)phandle->album_img_info->data);
        divoom_system_free(phandle->album_img_info);
    }
    if(phandle->clock_img_info)
    {
        divoom_system_free((uint8 *)phandle->clock_img_info->data);
        divoom_system_free(phandle->clock_img_info);
    }
    divoom_system_free(phandle);
}
