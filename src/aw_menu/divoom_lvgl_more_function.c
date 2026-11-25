#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_dial_menu.h"

typedef struct _divoom_lvgl_more_function_ctrl
{
    int menu_id;
    int lang;
    lv_style_t cout_style;
    void *back_obj;
    void *title_obj;
    void *main_obj;
    void *func_img_tbl_obj[20];
    void *func_label_tbl_obj[20];
}divoom_lvgl_more_function_ctrl, *Pdivoom_lvgl_more_function_ctrl;

typedef struct _divoom_lvgl_more_function_item
{
    int  menu_id;
    char *item_name;
    char *item_image;
}divoom_lvgl_more_function_item, *Pdivoom_lvgl_more_function_item;

static const divoom_lvgl_more_function_item  pdivoom_lvgl_more_function_menu_info[] = 
{
    {DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, "Dial", "22/22_dial.png"},
    {DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT, "DigitAlbum", "22/22_album.png"},
    {DIVOOM_MENU_CONTROL_ID_LYRIC_MENU,"Lyric", "22/22_lyric.png"},
    {DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU, "Alarm", "22/22_alarm.png"},
    {DIVOOM_MENU_CONTROL_ID_BLUE_MUSIC_MENU, "BlueMusic", "22/22_bt_music.png"},
    {DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_MENU, "LocalMusic", "22/22_local_music.png"},
    {DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_PLAY_MAIN, "WhiteNoise", "22/22_white_noise.png"},
    {DIVOOM_MENU_CONTROL_ID_SLEEP_AID, "SleepAid", "22/22_white_noise.png"},
    {DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU,"SystemSetting", "22/22_system_setting.png"},
    {DIVOOM_MENU_CONTROL_ID_POMODORO_MENU,"Pomodoro", "22/22_moto.png"},
    {DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU, "StopWatch", "22/22_timer.png"},
    {DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU,"Countdown", "22/22_count_down.png"},
    {DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU, "Scoreboard", "22/22_score.png"},
    {DIVOOM_MENU_CONTROL_ID_NOTICE_MESSAGE_MENU, "NoticeMessage", "22/22_notice_msg.png"},
    {DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU, "Noisemeter", "22/22_noise.png"},
};
  
void divoom_lvgl_more_function_btn_event(lv_event_t * e)
{
    int i;
    Pdivoom_lvgl_more_function_ctrl phandle = (Pdivoom_lvgl_more_function_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    void *cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        if(cur_obj == phandle->back_obj)
        {
            divoom_menu_control_return_pre();
            return;
        }
        for(i  = 0; i < sizeof(pdivoom_lvgl_more_function_menu_info)/ sizeof(pdivoom_lvgl_more_function_menu_info[0]); i++)
        {
            if(phandle->func_img_tbl_obj[i] == cur_obj)
            {
                //不同的MENU，需要带参数的，要做特殊处理
                switch(pdivoom_lvgl_more_function_menu_info[i].menu_id)
                {
                    case DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID:
                    {
                        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_SET_NEXT_CLOCK);
                    }
                        break;

                    case DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT:
                    {
                        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_ENTER_ALBUM_PLAY);
                    }
                        break;

                    default:
                        divoom_menu_control_load_next(pdivoom_lvgl_more_function_menu_info[i].menu_id, NULL, 0, 0);
                        break;
                }
            }
        }
    }
}
void *divoom_lvgl_more_function_init(void *user_data, int menu_id)
{
    int i;
    void *image_obj, *label_obj;
    Pdivoom_lvgl_more_function_ctrl phandle;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();


    phandle = divoom_system_malloc(sizeof(divoom_lvgl_more_function_ctrl));
    phandle->menu_id = menu_id;
    divoom_menu_control_init_lable_style(&phandle->cout_style);
    phandle->lang = psys_config->lang;
    phandle->main_obj = lv_img_create(NULL);
    lv_obj_add_style(phandle->main_obj, &phandle->cout_style, 0);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_menu_control_lv_img_set_src(phandle->main_obj, "1/1_bg.png");
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);

    image_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(image_obj, 36, 51);
    lv_obj_set_size(image_obj, 78, 78);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_back.png");
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_more_function_btn_event, LV_EVENT_KEY, phandle);
    phandle->back_obj = image_obj;
    
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 159, 63);
    lv_obj_set_size(label_obj, 482, 48);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(40, phandle->menu_id), 0); 
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("More"));        
    phandle->title_obj = label_obj;
    

    for(i  = 0; i < sizeof(pdivoom_lvgl_more_function_menu_info)/ sizeof(pdivoom_lvgl_more_function_menu_info[0]); i++)
    {
        phandle->func_img_tbl_obj[i] = lv_obj_create(phandle->main_obj);
        lv_obj_add_style(phandle->func_img_tbl_obj[i], phandle->main_obj, 0);
        lv_obj_clear_flag(phandle->func_img_tbl_obj[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(phandle->func_img_tbl_obj[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(phandle->func_img_tbl_obj[i], divoom_lvgl_more_function_btn_event, LV_EVENT_KEY, phandle);
        lv_obj_set_style_bg_color(phandle->func_img_tbl_obj[i], lv_color_hex(0), 0);
        lv_obj_set_style_bg_opa(phandle->func_img_tbl_obj[i], LV_OPA_TRANSP,  0);
        lv_obj_add_style(phandle->func_img_tbl_obj[i], &phandle->cout_style, 0);
        lv_obj_set_pos(phandle->func_img_tbl_obj[i], 40 + 255 *(i % 3), 176 + 340 *(i/3));
        lv_obj_set_size(phandle->func_img_tbl_obj[i], 210, 316);
        image_obj = lv_img_create(phandle->func_img_tbl_obj[i]);
        lv_obj_set_pos(image_obj, 0, 0);
        lv_obj_set_size(image_obj, 210, 210);
        divoom_menu_control_lv_img_set_src(image_obj, pdivoom_lvgl_more_function_menu_info[i].item_image);
        
        label_obj = lv_label_create(phandle->func_img_tbl_obj[i]);
        lv_obj_set_pos(label_obj, 0, 231);
    	lv_obj_set_size(label_obj, 210, 90);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xDFDFDF), 0);
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(pdivoom_lvgl_more_function_menu_info[i].item_name));        
        phandle->func_label_tbl_obj[i] = label_obj;
    }

    return  phandle;
}


void divoom_lvgl_more_function_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_more_function_ctrl phandle = (Pdivoom_lvgl_more_function_ctrl)handle;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    if(psys_config->lang != phandle->lang)
    {
        phandle->lang = psys_config->lang;
        for(int i  = 0; i < sizeof(pdivoom_lvgl_more_function_menu_info)/ sizeof(pdivoom_lvgl_more_function_menu_info[0]); i++)
        {
            lv_label_set_text(phandle->func_label_tbl_obj[i], divoom_menu_control_get_disp_str(pdivoom_lvgl_more_function_menu_info[i].item_name));        
        }
        lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("More"));        
    }
    //lv_scr_load_anim(phandle->main_obj, DIVOOM_MENU_ANIM_MODE, DIVOOM_MENU_ANIM_TIME, 0, 0);
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_more_function_pause(void *handle)
{
}

void divoom_lvgl_more_function_exit(void *handle)
{
    Pdivoom_lvgl_more_function_ctrl phandle = (Pdivoom_lvgl_more_function_ctrl)handle;
    
    lv_style_reset(&phandle->cout_style);
    lv_obj_del(phandle->main_obj);
    divoom_system_free(phandle);
}

