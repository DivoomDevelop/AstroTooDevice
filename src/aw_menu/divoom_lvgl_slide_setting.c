#include "divoom.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_slide_setting_ctrl
{
    int menu_id;
    int status;     //0:自由选择状态， 1：修改状态
    int select_id;
    lv_style_t normal;
    lv_style_t select_press;
    lv_style_t press;    
    void *main_obj;
    void *play_speed_slide_obj;
    void *video_volume_slide_obj;
    void *text_list_obj[10];
    void *btn_list_obj[10];
    void *switch_list_obj[10];
    void *pdirect_menu_ctrl;
    void *message_box_handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config;
}divoom_lvgl_slide_setting_ctrl, *Pdivoom_lvgl_slide_setting_ctrl;

void divoom_lvgl_slide_setting_create_disp_order_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_slide_setting_ctrl phandle = (Pdivoom_lvgl_slide_setting_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
        phandle->pslide_config->disp_order = ctrl_id;
        if(phandle->pslide_config->disp_order == 0)
        {
            lv_label_set_text(phandle->text_list_obj[2], divoom_menu_control_get_disp_str("OrderByAcceptTime"));
        }
        else if(phandle->pslide_config->disp_order == 1)
        {
            lv_label_set_text(phandle->text_list_obj[2], divoom_menu_control_get_disp_str("OrderByPhotoTime"));
        }
        else
        {
            lv_label_set_text(phandle->text_list_obj[2], divoom_menu_control_get_disp_str("OrderByRandom"));
        }
        divoom_pic_album_manage_set_refresh_album_info();
    }
    lv_group_focus_obj(phandle->btn_list_obj[phandle->select_id]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    
    phandle->message_box_handle = NULL;
}

void divoom_lvgl_slide_setting_create_fill_setting_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_slide_setting_ctrl phandle = (Pdivoom_lvgl_slide_setting_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
        phandle->pslide_config->bg_type = ctrl_id;
        if(phandle->pslide_config->bg_type == 0)
        {
            lv_label_set_text(phandle->text_list_obj[1], divoom_menu_control_get_disp_str("BgMisty"));
        }
        else if(phandle->pslide_config->bg_type == 1)
        {
            lv_label_set_text(phandle->text_list_obj[1], divoom_menu_control_get_disp_str("BgBlack"));
        }
        else
        {
            lv_label_set_text(phandle->text_list_obj[1], divoom_menu_control_get_disp_str("BgFade"));
        }
        divoom_pic_album_manage_set_refresh_album_info();
    }
    lv_group_focus_obj(phandle->btn_list_obj[phandle->select_id]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    
    phandle->message_box_handle = NULL;
}

void divoom_lvgl_slide_setting_create_fill_setting(Pdivoom_lvgl_slide_setting_ctrl phandle)
{
    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, 
        divoom_lvgl_slide_setting_create_fill_setting_callback, 88, 415, 640, 390, NULL, lv_color_hex(0xe4e4e4), 3);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("BgMisty"), 26,lv_color_hex(0xe4e4e4), lv_color_hex(0),lv_color_hex(0), 0, 
        32, 32, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("BgBlack"), 26, lv_color_hex(0xe4e4e4),lv_color_hex(0),lv_color_hex(0), 1, 
        32, 146, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("BgFade"), 26,lv_color_hex(0xe4e4e4), lv_color_hex(0),lv_color_hex(0), 2, 
        32, 260, 576, 98);
    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle, phandle->pslide_config->bg_type);
}

void divoom_lvgl_slide_setting_create_disp_order(Pdivoom_lvgl_slide_setting_ctrl phandle)
{
    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, 
        divoom_lvgl_slide_setting_create_disp_order_callback, 88, 415, 640, 390, NULL, lv_color_hex(0xe4e4e4), 3);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("OrderByAcceptTime"), 26,lv_color_hex(0xe4e4e4), lv_color_hex(0),lv_color_hex(0), 0, 
        32, 32, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("OrderByPhotoTime"), 26, lv_color_hex(0xe4e4e4),lv_color_hex(0),lv_color_hex(0), 1, 
        32, 146, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("OrderByRandom"), 26,lv_color_hex(0xe4e4e4), lv_color_hex(0),lv_color_hex(0), 2, 
        32, 260, 576, 98);
    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle, phandle->pslide_config->disp_reverse);
}

void divoom_lvgl_slide_setting_create_video_play_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_slide_setting_ctrl phandle = (Pdivoom_lvgl_slide_setting_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
        phandle->pslide_config->video_play_flag = ctrl_id;
    }
    lv_group_focus_obj(phandle->btn_list_obj[phandle->select_id]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    phandle->message_box_handle = NULL;
}

void divoom_lvgl_slide_setting_create_video_play(Pdivoom_lvgl_slide_setting_ctrl phandle)
{
    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, 
        divoom_lvgl_slide_setting_create_disp_order_callback, 88, 415, 640, 390, NULL, lv_color_hex(0xe4e4e4), 3);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("CyclePlay"), 26, lv_color_hex(0xe4e4e4),lv_color_hex(0),lv_color_hex(0), 0, 
        32, 32, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("PlayOnce"), 26, lv_color_hex(0xe4e4e4),lv_color_hex(0),lv_color_hex(0), 1, 
        32, 146, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("PlayOnceContinue"), 26, lv_color_hex(0xe4e4e4),lv_color_hex(0),lv_color_hex(0), 2, 
        32, 260, 576, 98);
    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle, phandle->pslide_config->video_play_flag);
}

static void divoom_lvgl_slide_setting_btn_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_slide_setting_ctrl phandle = (Pdivoom_lvgl_slide_setting_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        char *key_code = ((char *)lv_event_get_param(e));

        switch(key_code[0])
        {            
            case DIVOOM_LVGL_KEY_NEXT:
            case DIVOOM_LVGL_KEY_PRE:
                if((phandle->status == 0) || !(phandle->select_id == 0 || phandle->select_id == 7))
                {
                    if(phandle->status == 1)
                    {
                        phandle->status = 0;
                        lv_obj_add_style(phandle->btn_list_obj[phandle->select_id], &phandle->press, LV_STATE_FOCUSED);
                        lv_obj_add_style(phandle->btn_list_obj[phandle->select_id], &phandle->press, LV_STATE_FOCUS_KEY);
                    }
                    cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_menu_ctrl, cur_obj, key_code[0]==DIVOOM_LVGL_KEY_NEXT?DIVOOM_MENU_DIRECT_LEFT:DIVOOM_MENU_DIRECT_RIGHT);                            
                    lv_group_focus_obj(cur_obj);
                    for(int i = 0; i < 10; i++)
                    {
                        if(cur_obj == phandle->btn_list_obj[i])
                        {
                            phandle->select_id = i;
                            break;
                        }
                    }
                }
                else
                {
                    //只处理两个音量
                    int value;
                    if(phandle->select_id == 0 || phandle->select_id == 7)
                    {
                        if(phandle->select_id == 0)
                        {
                            value = lv_slider_get_value(phandle->play_speed_slide_obj);
                        }
                        else 
                        {
                            value = lv_slider_get_value(phandle->video_volume_slide_obj);
                        }
                        if(key_code[0] == DIVOOM_LVGL_KEY_PRE)
                        {
                            value += 5;
                            if(value > 100)
                            {
                                value = 100;
                            }
                        }
                        else
                        {
                            value -= 5;
                            if(value < 0)
                            {
                                value = 0;
                            }
                        }
                        if(phandle->select_id == 0)
                        {
                            lv_slider_set_value(phandle->play_speed_slide_obj, value, LV_ANIM_ON);
                            phandle->pslide_config->play_time = divoom_pic_phone_manage_convert_percent_to_slide_speed(value);
                        }
                        else 
                        {
                            lv_slider_set_value(phandle->video_volume_slide_obj, value, LV_ANIM_ON);
                            phandle->pslide_config->video_volume = value;
                        }
                    }
                }
                break;
               
            case DIVOOM_LVGL_KEY_ESC:
                if(phandle->status)
                {
                    phandle->status = 0;
                    lv_obj_add_style(phandle->btn_list_obj[phandle->select_id], &phandle->press, LV_STATE_FOCUSED);
                    lv_obj_add_style(phandle->btn_list_obj[phandle->select_id], &phandle->press, LV_STATE_FOCUS_KEY);
                }
                else
                {
                    divoom_pic_phone_manage_save_slide_config(1);
                    divoom_menu_control_return_pre();
                }
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:
                if(phandle->status == 0)
                {
                    phandle->status = 1;
                    lv_obj_add_style(phandle->btn_list_obj[phandle->select_id], &phandle->select_press, LV_STATE_FOCUSED);
                    lv_obj_add_style(phandle->btn_list_obj[phandle->select_id], &phandle->select_press, LV_STATE_FOCUS_KEY);
                }
                else
                {
                    //这里处理ENTER
                    if(phandle->select_id == 0)
                    {
                        int value;
                        value = lv_slider_get_value(phandle->play_speed_slide_obj);
                        if(value == 100)
                        {
                            value = 0;
                        }
                        else
                        {
                            value += 5;
                            if(value > 100)
                            {
                                value = 100;
                            }
                        }
                        lv_slider_set_value(phandle->play_speed_slide_obj, value, LV_ANIM_ON);
                        phandle->pslide_config->play_time = divoom_pic_phone_manage_convert_percent_to_slide_speed(value);
                    }
                    else if(phandle->select_id == 1)
                    {
                        //照片填充配置
                        divoom_lvgl_slide_setting_create_fill_setting(phandle);
                    }
                    else if(phandle->select_id == 2)
                    {
                        //照片显示顺序
                        divoom_lvgl_slide_setting_create_disp_order(phandle);
                    }
                    else if(phandle->select_id == 3)
                    {
                        //倒转显示
                        if(phandle->pslide_config->disp_reverse == 0)
                        {
                            phandle->pslide_config->disp_reverse = 1;
                            divoom_menu_control_lv_img_set_src(phandle->switch_list_obj[phandle->select_id], "68/on.png");
                        }
                        else
                        {
                            phandle->pslide_config->disp_reverse = 0;
                            divoom_menu_control_lv_img_set_src(phandle->switch_list_obj[phandle->select_id], "68/off.png");
                        }
                        divoom_pic_album_manage_set_refresh_album_info();
                    }
                    else if(phandle->select_id == 4)
                    {
                        if(phandle->pslide_config->title_flag == 0)
                        {
                            phandle->pslide_config->title_flag = 1;
                            divoom_menu_control_lv_img_set_src(phandle->switch_list_obj[phandle->select_id], "68/on.png");
                        }
                        else
                        {
                            phandle->pslide_config->title_flag = 0;
                            divoom_menu_control_lv_img_set_src(phandle->switch_list_obj[phandle->select_id], "68/off.png");
                        }
                    }
                    else if(phandle->select_id == 5)
                    {
                        if(phandle->pslide_config->clock_flag == 0)
                        {
                            phandle->pslide_config->clock_flag = 1;
                            divoom_menu_control_lv_img_set_src(phandle->switch_list_obj[phandle->select_id], "68/on.png");
                        }
                        else
                        {
                            phandle->pslide_config->clock_flag = 0;
                            divoom_menu_control_lv_img_set_src(phandle->switch_list_obj[phandle->select_id], "68/off.png");
                        }
                    }
                    else if(phandle->select_id == 6)
                    {
                        if(phandle->pslide_config->weather_flag == 0)
                        {
                            phandle->pslide_config->weather_flag = 1;
                            divoom_menu_control_lv_img_set_src(phandle->switch_list_obj[phandle->select_id], "68/on.png");
                        }
                        else
                        {
                            phandle->pslide_config->weather_flag = 0;
                            divoom_menu_control_lv_img_set_src(phandle->switch_list_obj[phandle->select_id], "68/off.png");
                        }
                    }
                    else if(phandle->select_id == 7)
                    {
                        int value;
                        value = lv_slider_get_value(phandle->video_volume_slide_obj);
                        if(value == 100)
                        {
                            value = 0;
                        }
                        else
                        {
                            value += 5;
                            if(value > 100)
                            {
                                value = 100;
                            }
                        }
                        lv_slider_set_value(phandle->video_volume_slide_obj, value, LV_ANIM_ON);
                        phandle->pslide_config->video_volume = value;
                    }
                }
                break;
               
            case DIVOOM_LVGL_KEY_HOME:
                //进入浏览照片模式
                break;
                          
        }

    }
}
void divoom_lvgl_slide_setting_disp_label_text(void *label_obj, char *disp_str, int font_high, int menu_id)
{
    int text_len;
    void* use_font = divoom_menu_control_get_font_info(font_high, menu_id);

    if(use_font == NULL)
    {
        return;
    }
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);

    lv_obj_set_style_text_font(label_obj, use_font, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0), 0);
    lv_label_set_text(label_obj, disp_str);

}

void *divoom_lvgl_slide_setting_init(void *user_data, int menu_id)
{
    void *label_obj, *image_obj, *btn_obj;
    Pdivoom_lvgl_slide_setting_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_slide_setting_ctrl));

    divoom_menu_control_init_btn_stype(&phandle->press,  NULL,&phandle->normal,  NULL,&phandle->select_press, NULL, 17);
    phandle->menu_id = menu_id;
    phandle->pdirect_menu_ctrl = divoom_menu_direct_ctrl_create(1, 8);
    phandle->pslide_config = divoom_pic_phone_manage_get_slide_config();
    phandle->main_obj = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(phandle->main_obj, LV_DIR_VER);
    
    image_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(image_obj, 0, 0);
	lv_obj_set_size(image_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_menu_control_lv_img_set_src(image_obj, "68/bg.png");

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 0, 50);
	lv_obj_set_size(label_obj, DIVOOM_SCREEN_WIDTH, 78);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("SlideSetting"), 32, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 76, 160);
	lv_obj_set_size(label_obj, 620, 32);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("PlaySpeed"), 24, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);

    btn_obj = lv_btn_create(phandle->main_obj);
    lv_obj_add_style(btn_obj, &phandle->normal, 0);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(btn_obj, 44, 204);
	lv_obj_set_size(btn_obj, 712, 90);
    
    phandle->play_speed_slide_obj = lv_slider_create(btn_obj);
    lv_slider_set_range(phandle->play_speed_slide_obj, 0, 100);
    lv_obj_set_pos(phandle->play_speed_slide_obj, 80, 30);
    lv_obj_set_size(phandle->play_speed_slide_obj, 509, 8);
    lv_slider_set_value(phandle->play_speed_slide_obj, divoom_pic_phone_manage_convert_slide_speed_to_percent(phandle->pslide_config->play_time), LV_ANIM_ON);
    lv_obj_set_style_bg_color(phandle->play_speed_slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(phandle->play_speed_slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
    
    lv_obj_add_event_cb(btn_obj, divoom_lvgl_slide_setting_btn_event_cb, LV_EVENT_KEY, phandle);
    
    phandle->btn_list_obj[0] = btn_obj;
    divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, 0, 0);

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 76, 342);
	lv_obj_set_size(label_obj, 620, 32);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("DisplayPreferences"), 24, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);


    btn_obj = lv_btn_create(phandle->main_obj);
    lv_obj_add_style(btn_obj, &phandle->normal, 0);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(btn_obj, 44, 386);
	lv_obj_set_size(btn_obj, 712, 90);
    
    image_obj = lv_img_create(btn_obj);
    lv_obj_set_pos(image_obj, 644, 16);
	lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "68/more.png");
    

    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 60, 20);
	lv_obj_set_size(label_obj, 400, 34);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("FrameBg"), 26, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);

    
    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 276, 20);
	lv_obj_set_size(label_obj, 360, 34);
    if(phandle->pslide_config->bg_type == 0)
    {
        divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("BgMisty"), 26, phandle->menu_id);
    }
    else if(phandle->pslide_config->bg_type == 1)
    {
        divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("BgBlack"), 26, phandle->menu_id);
    }
    else
    {
        divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("BgFade"), 26, phandle->menu_id);
    }
    phandle->text_list_obj[1] = label_obj;
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    
    lv_obj_add_event_cb(btn_obj, divoom_lvgl_slide_setting_btn_event_cb, LV_EVENT_KEY, phandle);
    phandle->btn_list_obj[1] = btn_obj;
    divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, 0, 1);


    

    btn_obj = lv_btn_create(phandle->main_obj);
    lv_obj_add_style(btn_obj, &phandle->normal, 0);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(btn_obj, 44, 483);
	lv_obj_set_size(btn_obj, 712, 90);
    
    image_obj = lv_img_create(btn_obj);
    lv_obj_set_pos(image_obj, 644, 16);
	lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "68/more.png");
    

    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 60, 20);
	lv_obj_set_size(label_obj, 400, 34);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("DisplayOrder"), 26, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);

    
    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 276, 20);
	lv_obj_set_size(label_obj, 360, 34);
    if(phandle->pslide_config->disp_order == 0)
    {
        divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("OrderByAcceptTime"), 26, phandle->menu_id);
    }
    else if(phandle->pslide_config->disp_order == 1)
    {
        divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("OrderByPhotoTime"), 26, phandle->menu_id);
    }
    else
    {
        divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("OrderByRandom"), 26, phandle->menu_id);
    }
    phandle->text_list_obj[2] = label_obj;
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    
    lv_obj_add_event_cb(btn_obj, divoom_lvgl_slide_setting_btn_event_cb, LV_EVENT_KEY, phandle);
    phandle->btn_list_obj[2] = btn_obj;
    divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, 0, 2);

    
    btn_obj = lv_btn_create(phandle->main_obj);
    lv_obj_add_style(btn_obj, &phandle->normal, 0);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(btn_obj, 44, 580);
	lv_obj_set_size(btn_obj, 712, 90);

    image_obj = lv_img_create(btn_obj);
    lv_obj_set_pos(image_obj, 608, 12);
	lv_obj_set_size(image_obj, 72, 44);
    if(phandle->pslide_config->disp_reverse)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "68/on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "68/off.png");
    }
    phandle->switch_list_obj[3] = image_obj;
    

    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 60, 4);
	lv_obj_set_size(label_obj, 400, 34);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("DisplayReverse"), 26, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);

    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 60, 44);
	lv_obj_set_size(label_obj, 400, 22);
    if(phandle->pslide_config->disp_reverse)
    {
        divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("DisplayReverseDesc1"), 17, phandle->menu_id);
    }
    else
    {
        divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("DisplayReverseDesc0"), 17, phandle->menu_id);
    }
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    phandle->text_list_obj[3] = label_obj;

    
    lv_obj_add_event_cb(btn_obj, divoom_lvgl_slide_setting_btn_event_cb, LV_EVENT_KEY, phandle);
    phandle->btn_list_obj[3] = btn_obj;
    divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, 0, 3);

    

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 76, 718);
	lv_obj_set_size(label_obj, 620, 32);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("SmallComponent"), 24, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);


    btn_obj = lv_btn_create(phandle->main_obj);
    lv_obj_add_style(btn_obj, &phandle->normal, 0);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(btn_obj, 44, 762);
	lv_obj_set_size(btn_obj, 712, 90);


    image_obj = lv_img_create(btn_obj);
    lv_obj_set_pos(image_obj, 608, 12);
	lv_obj_set_size(image_obj, 72, 44);
    if(phandle->pslide_config->title_flag)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "68/on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "68/off.png");
    }
    phandle->switch_list_obj[4] = image_obj;
    

    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 60, 20);
	lv_obj_set_size(label_obj, 400, 34);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("DisplayTitle"), 26, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);

        
    lv_obj_add_event_cb(btn_obj, divoom_lvgl_slide_setting_btn_event_cb, LV_EVENT_KEY, phandle);
    phandle->btn_list_obj[4] = btn_obj;
    divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, 0, 4);

    

    btn_obj = lv_btn_create(phandle->main_obj);
    lv_obj_add_style(btn_obj, &phandle->normal, 0);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(btn_obj, 44, 860);
	lv_obj_set_size(btn_obj, 712, 90);


    image_obj = lv_img_create(btn_obj);
    lv_obj_set_pos(image_obj, 608, 12);
	lv_obj_set_size(image_obj, 72, 44);
    if(phandle->pslide_config->clock_flag)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "68/on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "68/off.png");
    }
    phandle->switch_list_obj[5] = image_obj;
    

    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 60, 20);
	lv_obj_set_size(label_obj, 400, 34);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("DisplayTime"), 26, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);

        
    lv_obj_add_event_cb(btn_obj, divoom_lvgl_slide_setting_btn_event_cb, LV_EVENT_KEY, phandle);
    phandle->btn_list_obj[5] = btn_obj;
    divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, 0, 5);

    
    btn_obj = lv_btn_create(phandle->main_obj);
    lv_obj_add_style(btn_obj, &phandle->normal, 0);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(btn_obj, 44, 958);
	lv_obj_set_size(btn_obj, 712, 90);


    image_obj = lv_img_create(btn_obj);
    lv_obj_set_pos(image_obj, 608, 12);
	lv_obj_set_size(image_obj, 72, 44);
    if(phandle->pslide_config->weather_flag)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "68/on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "68/off.png");
    }
    phandle->switch_list_obj[6] = image_obj;
    

    label_obj = lv_label_create(btn_obj);
    lv_obj_set_pos(label_obj, 60, 20);
	lv_obj_set_size(label_obj, 400, 34);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("DisplayWeather"), 26, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);

        
    lv_obj_add_event_cb(btn_obj, divoom_lvgl_slide_setting_btn_event_cb, LV_EVENT_KEY, phandle);
    phandle->btn_list_obj[6] = btn_obj;
    divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, 0, 6);



    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 76, 1096);
	lv_obj_set_size(label_obj, 620, 32);
    divoom_lvgl_slide_setting_disp_label_text(label_obj, divoom_menu_control_get_disp_str("VedioVolume"), 24, phandle->menu_id);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);

    btn_obj = lv_btn_create(phandle->main_obj);
    lv_obj_add_style(btn_obj, &phandle->normal, 0);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_obj, &phandle->press, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(btn_obj, 44, 1140);
	lv_obj_set_size(btn_obj, 712, 90);

    phandle->video_volume_slide_obj = lv_slider_create(btn_obj);
    lv_slider_set_range(phandle->video_volume_slide_obj, 0, 100);
    lv_obj_set_pos(phandle->video_volume_slide_obj, 80, 30);
    lv_obj_set_size(phandle->video_volume_slide_obj, 509, 8);
    lv_slider_set_value(phandle->video_volume_slide_obj, phandle->pslide_config->video_volume, LV_ANIM_ON);
    lv_obj_set_style_bg_color(phandle->video_volume_slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR); 
    lv_obj_set_style_bg_color(phandle->video_volume_slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
    
    lv_obj_add_event_cb(btn_obj, divoom_lvgl_slide_setting_btn_event_cb, LV_EVENT_KEY, phandle);
    phandle->btn_list_obj[7] = btn_obj;
    divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, 0, 7);

    


    return phandle;
}

void divoom_lvgl_slide_setting_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_slide_setting_ctrl phandle = (Pdivoom_lvgl_slide_setting_ctrl)handle;

    if(active_obj)
    {
        lv_group_focus_obj(active_obj);
    }
    else
    {
        lv_group_focus_obj(phandle->btn_list_obj[phandle->select_id]);
    }
    lv_scr_load(phandle->main_obj);
}


void divoom_lvgl_slide_setting_exit(void *handle)
{
    Pdivoom_lvgl_slide_setting_ctrl phandle = (Pdivoom_lvgl_slide_setting_ctrl)handle;

    if(phandle->message_box_handle)
    {
        divoom_lvgl_message_box_destroy(phandle->message_box_handle, 1);
        phandle->message_box_handle = NULL;
    }

    lv_style_reset(&phandle->normal);
    lv_style_reset(&phandle->select_press);
    lv_style_reset(&phandle->press);
    divoom_menu_direct_ctrl_destory(phandle->pdirect_menu_ctrl);
    lv_obj_del(phandle->main_obj);
    divoom_system_free(phandle);
}

