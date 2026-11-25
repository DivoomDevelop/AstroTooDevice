#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_photo_slide_effect.h"

//幻灯片选项菜单
typedef struct _divoom_lvgl_photo_setting_menu_slide_item
{
    void *main_obj;
    void *speed_slide_obj;
    int  select_id;
    void *slide_effect_tbl_obj[DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_END_TYPE * 2];
    void *slide_select_tbl_obj[DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_END_TYPE * 2];
}divoom_lvgl_photo_setting_menu_slide_item, *Pdivoom_lvgl_photo_setting_menu_slide_item;


//显示偏好菜单
typedef struct _divoom_lvgl_photo_setting_menu_display_item
{
    void *fill_dialog;
    void *main_obj;
    void *fill_obj;
    void *order_obj;
    void *order_label_obj;
    void *display_reverse_btn_obj[10];
    void *display_reverse_image_obj[10];
    void *frame_bg_btn_obj[10];
    void *frame_bg_image_obj[10];
}divoom_lvgl_photo_setting_menu_display_item, *Pdivoom_lvgl_photo_setting_menu_display_item;


//小组件选项菜单
typedef struct _divoom_lvgl_photo_setting_menu_component_item
{
    void *main_obj;
    void *on_off_table_obj[3];
}divoom_lvgl_photo_setting_menu_component_item, *Pdivoom_lvgl_photo_setting_menu_component_item;


//视频选项菜单
typedef struct _divoom_lvgl_photo_setting_menu_video_item
{
    void *main_obj;
    void *auto_play_obj;
    void *play_type_obj[3];
    void *play_image_obj[3];
    void *auto_mute_obj;
    void *volume_obj;
}divoom_lvgl_photo_setting_menu_video_item, *Pdivoom_lvgl_photo_setting_menu_video_item;

typedef struct _divoom_lvgl_photo_setting_menu_ctrl
{
    int menu_id;
    void *main_obj;
    void *back_menu_obj;
    lv_style_t lable_style;
    void *select_bg_obj[4];
    void *select_btn_obj[4];
    divoom_lvgl_photo_setting_menu_slide_item slide_menu;
    divoom_lvgl_photo_setting_menu_component_item component_menu;
    divoom_lvgl_photo_setting_menu_display_item display_menu;
    divoom_lvgl_photo_setting_menu_video_item video_menu;
}divoom_lvgl_photo_setting_menu_ctrl, *Pdivoom_lvgl_photo_setting_menu_ctrl;


typedef struct _divoom_lvgl_photo_setting_menu_setting_item
{
    char  *disp_str;
    char  *image_name;
}divoom_lvgl_photo_setting_menu_setting_item, *Pdivoom_lvgl_photo_setting_menu_setting_item;

static const divoom_lvgl_photo_setting_menu_setting_item psetting_menu_list_info[] = 
{
    {"Setting", "20/20_slide_play.png"},
    {"DisplayPreferences", "20/20_display_item.png"},
    {"SmallComponent", "20/20_component.png"},
    {"VideoSetting", "20/20_video_setting.png"},
};

typedef struct _divoom_lvgl_photo_setting_menu_slide_disp_info
{
    int slide_type;
    char  *disp_str;
}divoom_lvgl_photo_setting_menu_slide_disp_info, *Pdivoom_lvgl_photo_setting_menu_slide_disp_info;

static const divoom_lvgl_photo_setting_menu_slide_disp_info pslide_effect_menu_list_info[] = 
{
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL, "SlideNormal"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH, "SlidePush"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT, "SlideFade"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_ROTATE, "SlideRotate"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PHOTO_STEAM, "SlidePhotoStream"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE0, "Creative0"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE1, "Creative1"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE2, "Creative2"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE3, "Creative3"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE4, "Creative4"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE5, "Creative5"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE6, "Creative6"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE7, "Creative7"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE8, "Creative8"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE9, "Creative9"},
};

static void divoom_lvgl_photo_setting_menu_slide_menu_event_cb(lv_event_t * e)
{
    int i;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle = (Pdivoom_lvgl_photo_setting_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
        
        if(cur_obj == phandle->slide_menu.speed_slide_obj)
        {            
            
            pslide_config->play_time = lv_slider_get_value(phandle->slide_menu.speed_slide_obj);
            divoom_pic_phone_manage_save_slide_config(1);
        }
        else
        {
            for(i = 0; i < sizeof(pslide_effect_menu_list_info)/sizeof(pslide_effect_menu_list_info[0]); i ++)
            {
                if(cur_obj == phandle->slide_menu.slide_effect_tbl_obj[i])
                {
                    if(phandle->slide_menu.select_id != i)
                    {
                        lv_obj_add_flag(phandle->slide_menu.slide_select_tbl_obj[phandle->slide_menu.select_id], LV_OBJ_FLAG_HIDDEN);
                        lv_obj_clear_flag(phandle->slide_menu.slide_select_tbl_obj[i], LV_OBJ_FLAG_HIDDEN);
                        pslide_config->effect_type = pslide_effect_menu_list_info[i].slide_type;
                        pslide_config->cur_play_effect_type = pslide_config->effect_type;
                        divoom_pic_phone_manage_save_slide_config(1);
                    }
                    phandle->slide_menu.select_id = i;
                    break;
                }
            }
        }
    }
}

//这个是幻灯片选项菜单
void divoom_lvgl_photo_setting_menu_create_slide_menu(Pdivoom_lvgl_photo_setting_menu_ctrl phandle)
{
    int i;
    void *label_obj, *image_obj, *effect_cout_obj;
    Pdivoom_lvgl_photo_setting_menu_slide_item pslide_handle = &phandle->slide_menu;
    
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    
    pslide_handle->main_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(pslide_handle->main_obj, 290, 0);
    lv_obj_set_size(pslide_handle->main_obj, DIVOOM_SCREEN_WIDTH - 290, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(pslide_handle->main_obj, lv_color_hex(0x121213), 0);
    divoom_menu_control_lv_img_set_src(pslide_handle->main_obj, "20/20_slide_bg.png");


    label_obj = lv_label_create(pslide_handle->main_obj);
    lv_obj_set_pos(label_obj, 28, 44);
    lv_obj_set_size(label_obj, 361, 25);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("PlaySpeed"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);    


    pslide_handle->speed_slide_obj = lv_slider_create(pslide_handle->main_obj);
    lv_slider_set_range(pslide_handle->speed_slide_obj, 0, 100);
    lv_obj_set_pos(pslide_handle->speed_slide_obj, 88, 99);
    lv_obj_set_size(pslide_handle->speed_slide_obj, 319, 4);
    lv_slider_set_value(pslide_handle->speed_slide_obj, divoom_pic_phone_manage_convert_slide_speed_to_percent(pslide_config->play_time), LV_ANIM_ON);
    lv_obj_set_style_bg_color(pslide_handle->speed_slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(pslide_handle->speed_slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
    lv_obj_add_flag(pslide_handle->speed_slide_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pslide_handle->speed_slide_obj, divoom_lvgl_photo_setting_menu_slide_menu_event_cb, LV_EVENT_KEY, phandle);
    


    label_obj = lv_label_create(pslide_handle->main_obj);
    lv_obj_set_pos(label_obj, 28, 153);
    lv_obj_set_size(label_obj, 361, 25);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("SlideTopic"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);    
    
    for(i = 0; i < sizeof(pslide_effect_menu_list_info)/sizeof(pslide_effect_menu_list_info[0]); i++)
    {

        label_obj = lv_label_create(pslide_handle->main_obj);
        lv_obj_set_pos(label_obj, 84, 206 + 49*i);
        lv_obj_set_size(label_obj, 333, 25);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(pslide_effect_menu_list_info[i].disp_str));
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0); 
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
        lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);  
        lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(label_obj, divoom_lvgl_photo_setting_menu_slide_menu_event_cb, LV_EVENT_KEY, phandle);
        pslide_handle->slide_effect_tbl_obj[i] = label_obj;
        
        image_obj = lv_img_create(pslide_handle->main_obj);
        lv_obj_set_pos(image_obj, 460, 205 + 49*i);
    	lv_obj_set_size(image_obj, 28, 28);
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_select.png");
        pslide_handle->slide_select_tbl_obj[i] = image_obj;
        
        if(pslide_effect_menu_list_info[i].slide_type == pslide_config->effect_type)
        {
            pslide_handle->select_id = i;
            lv_obj_clear_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }

    
    lv_obj_add_flag(pslide_handle->main_obj, LV_OBJ_FLAG_HIDDEN);
}

void divoom_lvgl_photo_setting_menu_create_confirm_fill_diallog_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle = (Pdivoom_lvgl_photo_setting_menu_ctrl)handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

    divoom_print("confirm id: %d\n", ctrl_id);
    pslide_config->skip_fill_flag = ctrl_id;
    divoom_pic_phone_manage_save_slide_config(1);

    divoom_lvgl_message_box_destroy(phandle->display_menu.fill_dialog, 0);
    phandle->display_menu.fill_dialog = NULL;
}

void divoom_lvgl_photo_setting_menu_create_confirm_fill_diallog(Pdivoom_lvgl_photo_setting_menu_ctrl phandle)
{
    phandle->display_menu.fill_dialog = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, divoom_lvgl_photo_setting_menu_create_confirm_fill_diallog_callback
        , 90, 435, 620, 410, "20/20_confirm_dialog.png",  lv_color_hex(0x222222), 3);
    divoom_lvgl_message_box_add_label_item(phandle->display_menu.fill_dialog, divoom_menu_control_get_disp_str("ApplyAllPicNotice"), 
        -1, 28, lv_color_hex(0xC4C4C4), LV_TEXT_ALIGN_CENTER, 20, 20, 580, 288);
    divoom_lvgl_message_box_add_label_item(phandle->display_menu.fill_dialog, divoom_menu_control_get_disp_str("Ignore"), 
        0, 28, lv_color_hex(0xC4C4C4), LV_TEXT_ALIGN_CENTER, 0, 347, 309, 37);
    divoom_lvgl_message_box_add_label_item(phandle->display_menu.fill_dialog, divoom_menu_control_get_disp_str("ApplyAllPic"), 
        1, 28, lv_color_hex(0xC4C4C4), LV_TEXT_ALIGN_CENTER, 311, 347, 309, 37);
    
}

static void divoom_lvgl_photo_setting_menu_display_menu_event_cb(lv_event_t * e)
{
    int i;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle = (Pdivoom_lvgl_photo_setting_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
        
        if(cur_obj == phandle->display_menu.fill_obj)
        {         
            if(pslide_config->fill_flag)
            {
                pslide_config->fill_flag = 0;
                divoom_menu_control_lv_img_set_src(phandle->display_menu.fill_obj, "20/20_off.png");
            }
            else
            {
                pslide_config->fill_flag = 1;
                divoom_menu_control_lv_img_set_src(phandle->display_menu.fill_obj, "20/20_on.png");
                divoom_lvgl_photo_setting_menu_create_confirm_fill_diallog(phandle);
            }
            divoom_pic_phone_manage_save_slide_config(1);
            return;
        }
        else if(cur_obj == phandle->display_menu.order_obj)
        {         
            if(pslide_config->disp_reverse)
            {
                pslide_config->disp_reverse = 0;
                lv_label_set_text(phandle->display_menu.order_label_obj, divoom_menu_control_get_disp_str("DisplayReverseDesc1"));
                divoom_menu_control_lv_img_set_src(cur_obj, "20/20_off.png");
            }
            else
            {
                pslide_config->disp_reverse = 1;
                lv_label_set_text(phandle->display_menu.order_label_obj, divoom_menu_control_get_disp_str("DisplayReverseDesc0"));
                divoom_menu_control_lv_img_set_src(cur_obj, "20/20_on.png");
            }  
            divoom_pic_phone_manage_save_slide_config(1);
            return;
        }
        for(i = 0; i < 3; i ++)
        {
            if(cur_obj == phandle->display_menu.display_reverse_btn_obj[i])
            {
                lv_obj_add_flag(phandle->display_menu.display_reverse_image_obj[pslide_config->disp_reverse], LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(phandle->display_menu.display_reverse_image_obj[i], LV_OBJ_FLAG_HIDDEN);
                pslide_config->disp_reverse = i;
                divoom_pic_phone_manage_save_slide_config(1);
                return;
            }
        }
        
        for(i = 0; i < 3; i ++)
        {
            if(cur_obj == phandle->display_menu.frame_bg_btn_obj[i])
            {
                lv_obj_add_flag(phandle->display_menu.frame_bg_image_obj[pslide_config->bg_type], LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(phandle->display_menu.frame_bg_image_obj[i], LV_OBJ_FLAG_HIDDEN);
                pslide_config->bg_type = i;
                divoom_pic_phone_manage_save_slide_config(1);
                return;
            }
        }
    }
}

//显示选项菜单

static const char *pdisplay_reverse_list_info[] = 
{
    "OrderByAcceptTime",
    "OrderByPhotoTime",
    "OrderByRandom",
};

static const char *ppicture_bg_list_info[] = 
{
    "BgMisty",
    "BgBlack",
    "BgFade",
};

void divoom_lvgl_photo_setting_menu_create_display_menu(Pdivoom_lvgl_photo_setting_menu_ctrl phandle)
{
    int i;
    void *label_obj, *image_obj;
    Pdivoom_lvgl_photo_setting_menu_display_item pdisplay_handle = &phandle->display_menu;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    

    pdisplay_handle->main_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(pdisplay_handle->main_obj, 290, 0);
    lv_obj_set_size(pdisplay_handle->main_obj, DIVOOM_SCREEN_WIDTH - 290, DIVOOM_SCREEN_HIGH);
    divoom_menu_control_lv_img_set_src(pdisplay_handle->main_obj, "20/20_dispitem_bg.png");
    

    label_obj = lv_label_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 44);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Fill"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);    
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);

    label_obj = lv_label_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 68);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("FillDesc"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);        
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 35*LV_OPA_100/100, 0);

    image_obj = lv_img_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(image_obj, 429, 53);
	lv_obj_set_size(image_obj, 44, 25);
    if(pslide_config->fill_flag)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_off.png");
    }
    pdisplay_handle->fill_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_photo_setting_menu_display_menu_event_cb, LV_EVENT_KEY, phandle);

    label_obj = lv_label_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 115);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("FrameBg"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);    
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
    
    label_obj = lv_label_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 139);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("FrameBgDesc"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);    
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 35*LV_OPA_100/100, 0);
    
    
    for(i = 0; i < sizeof(ppicture_bg_list_info)/sizeof(ppicture_bg_list_info[0]); i++)
    {
        
        label_obj = lv_label_create(pdisplay_handle->main_obj);
        lv_obj_set_pos(label_obj, 84, 182 + i * 49);
        lv_obj_set_size(label_obj, 333, 25);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(ppicture_bg_list_info[i]));
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);   
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
        lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
        lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(label_obj, divoom_lvgl_photo_setting_menu_display_menu_event_cb, LV_EVENT_KEY, phandle);
        pdisplay_handle->frame_bg_btn_obj[i] = label_obj;
        
        image_obj = lv_img_create(pdisplay_handle->main_obj);
        lv_obj_set_pos(image_obj, 431, 182 + i * 49);
    	lv_obj_set_size(image_obj, 28, 28);
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_select.png");
        pdisplay_handle->frame_bg_image_obj[i] = image_obj;
        if(pslide_config->bg_type == i)
        {
            lv_obj_clear_flag(image_obj, LV_OBJ_FLAG_HIDDEN);            
        }
        else
        {
            lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);    
        }
        
    }


    label_obj = lv_label_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 345);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("DisplayOrder"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);    
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
    
    for(i = 0; i < sizeof(pdisplay_reverse_list_info)/sizeof(pdisplay_reverse_list_info[0]); i++)
    {
        
        label_obj = lv_label_create(pdisplay_handle->main_obj);
        lv_obj_set_pos(label_obj, 84, 390 + i * 49);
        lv_obj_set_size(label_obj, 333, 25);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(pdisplay_reverse_list_info[i]));
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);   
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
        lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
        lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(label_obj, divoom_lvgl_photo_setting_menu_display_menu_event_cb, LV_EVENT_KEY, phandle);
        pdisplay_handle->display_reverse_btn_obj[i] = label_obj;
        
        image_obj = lv_img_create(pdisplay_handle->main_obj);
        lv_obj_set_pos(image_obj, 431, 389 + i * 49);
    	lv_obj_set_size(image_obj, 28, 28);
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_select.png");
        pdisplay_handle->display_reverse_image_obj[i] = image_obj;
        if(pslide_config->bg_type == i)
        {
            lv_obj_clear_flag(image_obj, LV_OBJ_FLAG_HIDDEN);            
        }
        else
        {
            lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);    
        }
        
    }

    

    
    label_obj = lv_label_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 550);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("DisplayReverse"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);    
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);

    label_obj = lv_label_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 574);
    lv_obj_set_size(label_obj, 361, 20);
    pdisplay_handle->order_label_obj = label_obj;
    if(pslide_config->disp_reverse)
    {
        lv_label_set_text(pdisplay_handle->order_label_obj, divoom_menu_control_get_disp_str("DisplayReverseDesc0"));
    }
    else
    {
        lv_label_set_text(pdisplay_handle->order_label_obj, divoom_menu_control_get_disp_str("DisplayReverseDesc1"));
    }
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 35*LV_OPA_100/100, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);    
    
    image_obj = lv_img_create(pdisplay_handle->main_obj);
    lv_obj_set_pos(image_obj, 429, 558);
	lv_obj_set_size(image_obj, 44, 25);
    if(pslide_config->disp_reverse)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_off.png");
    }
    pdisplay_handle->order_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_photo_setting_menu_display_menu_event_cb, LV_EVENT_KEY, phandle);


    
    lv_obj_add_flag(pdisplay_handle->main_obj, LV_OBJ_FLAG_HIDDEN);
    
}


static const char *pcomponent_disp_list_info[] = 
{
    "DisplayTitle",
    "DisplayTime",
    "DisplayWeather",
};


static void divoom_lvgl_photo_setting_menu_component_menu_event_cb(lv_event_t * e)
{
    int i;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle = (Pdivoom_lvgl_photo_setting_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
        
        for(i = 0; i < 3; i ++)
        {
            if(cur_obj == phandle->component_menu.on_off_table_obj[i])
            {
                int on_off;
                
                if(i ==  0)
                {
                    on_off = pslide_config->title_flag;
                    pslide_config->title_flag = on_off?0:1;
                }
                else if(i == 1)
                {
                    on_off = pslide_config->clock_flag;
                    pslide_config->clock_flag = on_off?0:1;
                }
                else
                {
                    on_off = pslide_config->weather_flag;
                    pslide_config->weather_flag = on_off?0:1;
                }
                divoom_pic_phone_manage_save_slide_config(1);
                divoom_menu_control_lv_img_set_src(phandle->component_menu.on_off_table_obj[i], on_off?"20/20_off.png":"20/20_on.png");
                return;
            }
        }
    }
}

//这个是小组件菜单
void divoom_lvgl_photo_setting_menu_create_component_menu(Pdivoom_lvgl_photo_setting_menu_ctrl phandle)
{
    int i, on_off;
    void *label_obj, *image_obj;
    Pdivoom_lvgl_photo_setting_menu_component_item pcomponent_handle = &phandle->component_menu;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    

    pcomponent_handle->main_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(pcomponent_handle->main_obj, 290, 0);
    lv_obj_set_size(pcomponent_handle->main_obj, DIVOOM_SCREEN_WIDTH - 290, DIVOOM_SCREEN_HIGH);
    divoom_menu_control_lv_img_set_src(pcomponent_handle->main_obj, "20/20_component_bg.png");

    for(i = 0; i < 3; i++)
    {
        
        label_obj = lv_label_create(pcomponent_handle->main_obj);
        lv_obj_set_pos(label_obj, 56, 57 + i * 71);
        lv_obj_set_size(label_obj, 361, 20);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(pcomponent_disp_list_info[i]));
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
        lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);  
        
        image_obj = lv_img_create(pcomponent_handle->main_obj);
        lv_obj_set_pos(image_obj, 429, 53 + i * 71);
    	lv_obj_set_size(image_obj, 44, 25);
        if(i ==  0)
        {
            on_off = pslide_config->title_flag;
        }
        else if(i == 1)
        {
            on_off = pslide_config->clock_flag;
        }
        else
        {
            on_off = pslide_config->weather_flag;
        }
        divoom_menu_control_lv_img_set_src(image_obj, on_off?"20/20_on.png":"20/20_off.png");
        pcomponent_handle->on_off_table_obj[i] = image_obj;
        lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(image_obj, divoom_lvgl_photo_setting_menu_component_menu_event_cb, LV_EVENT_KEY, phandle);

    }
    
    lv_obj_add_flag(pcomponent_handle->main_obj, LV_OBJ_FLAG_HIDDEN);
}


static void divoom_lvgl_photo_setting_menu_video_menu_event_cb(lv_event_t * e)
{
    int i;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle = (Pdivoom_lvgl_photo_setting_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
        
        if(cur_obj == phandle->video_menu.auto_play_obj)
        {            
            if(pslide_config->video_auto_play)
            {
                pslide_config->video_auto_play = 0;
                divoom_menu_control_lv_img_set_src(cur_obj, "20/20_off.png");
            }
            else
            {
                pslide_config->video_auto_play = 1;
                divoom_menu_control_lv_img_set_src(cur_obj, "20/20_on.png");
            }
            divoom_pic_phone_manage_save_slide_config(1);
            return;
        }
        else if(cur_obj == phandle->video_menu.volume_obj)
        {
            pslide_config->video_volume = lv_slider_get_value(cur_obj);
            divoom_pic_phone_manage_save_slide_config(1);
        }
        else if(cur_obj == phandle->video_menu.auto_mute_obj)
        {            
            if(pslide_config->video_auto_mute)
            {
                pslide_config->video_auto_mute = 0;
                divoom_menu_control_lv_img_set_src(cur_obj, "20/20_off.png");
            }
            else
            {
                pslide_config->video_auto_mute = 1;
                divoom_menu_control_lv_img_set_src(cur_obj, "20/20_on.png");
            }
            divoom_pic_phone_manage_save_slide_config(1);
            return;
        }
        for(i = 0; i < 3; i ++)
        {
            if(cur_obj == phandle->video_menu.play_type_obj[i])
            {
                lv_obj_add_flag(phandle->video_menu.play_image_obj[pslide_config->video_play_flag], LV_OBJ_FLAG_HIDDEN);
                pslide_config->video_play_flag = i;
                divoom_pic_phone_manage_save_slide_config(1);
                lv_obj_clear_flag(phandle->video_menu.play_image_obj[pslide_config->video_play_flag], LV_OBJ_FLAG_HIDDEN);
                return;
            }
        }
        
    }
}

static const char *pvideo_disp_list_info[] = 
{
    "CyclePlay",
    "PlayOnce",
    "PlayOnceContinue",
};

//视频设置菜单
void divoom_lvgl_photo_setting_menu_create_video_menu(Pdivoom_lvgl_photo_setting_menu_ctrl phandle)
{
    int i;
    void *label_obj, *image_obj;
    Pdivoom_lvgl_photo_setting_menu_video_item pvideo_handle = &phandle->video_menu;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    

    pvideo_handle->main_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(pvideo_handle->main_obj, 290, 0);
    lv_obj_set_size(pvideo_handle->main_obj, DIVOOM_SCREEN_WIDTH - 290, DIVOOM_SCREEN_HIGH);
    divoom_menu_control_lv_img_set_src(pvideo_handle->main_obj, "20/20_video_bg.png");
    
    
    
    label_obj = lv_label_create(pvideo_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 57);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("AutoPlay"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);  
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
    
    image_obj = lv_img_create(pvideo_handle->main_obj);
    lv_obj_set_pos(image_obj, 429, 53);
    lv_obj_set_size(image_obj, 44, 25);
    if(pslide_config->video_auto_play)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_off.png");
    }
    pvideo_handle->auto_play_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_photo_setting_menu_video_menu_event_cb, LV_EVENT_KEY, phandle);
    

    label_obj = lv_label_create(pvideo_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 105);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("PlayType"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);  
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);


    for(i = 0; i < 3; i++)
    {
        
        label_obj = lv_label_create(pvideo_handle->main_obj);
        lv_obj_set_pos(label_obj, 84, 150 + 50 * i);
        lv_obj_set_size(label_obj, 333, 25);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(pvideo_disp_list_info[i]));
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);  
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
        lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
        lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(label_obj, divoom_lvgl_photo_setting_menu_video_menu_event_cb, LV_EVENT_KEY, phandle);
        pvideo_handle->play_type_obj[i] = label_obj;
        
        image_obj = lv_img_create(pvideo_handle->main_obj);
        lv_obj_set_pos(image_obj, 431, 150 + 50 * i );
        lv_obj_set_size(image_obj, 28, 28);
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_select.png");
        pvideo_handle->play_image_obj[i] = image_obj;
        if(pslide_config->video_play_flag == i)
        {
            lv_obj_clear_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
        }
        
    }
    

    label_obj = lv_label_create(pvideo_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 342);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("AutoMute"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);  
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
    
    label_obj = lv_label_create(pvideo_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 366);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("AutoMuteDesc"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);  
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
    
    image_obj = lv_img_create(pvideo_handle->main_obj);
    lv_obj_set_pos(image_obj, 429, 351);
    lv_obj_set_size(image_obj, 44, 25);
    if(pslide_config->video_auto_mute)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_off.png");
    }
    pvideo_handle->auto_mute_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_photo_setting_menu_video_menu_event_cb, LV_EVENT_KEY, phandle);

    label_obj = lv_label_create(pvideo_handle->main_obj);
    lv_obj_set_pos(label_obj, 56, 400);
    lv_obj_set_size(label_obj, 361, 20);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("VideoVolume"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(17, phandle->menu_id), 0);  
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
    lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
    
    pvideo_handle->volume_obj = lv_slider_create(pvideo_handle->main_obj);
    lv_slider_set_range(pvideo_handle->volume_obj, 0, 100);
    lv_obj_set_pos(pvideo_handle->volume_obj, 91, 456);
    lv_obj_set_size(pvideo_handle->volume_obj, 319, 4);
    lv_slider_set_value(pvideo_handle->volume_obj, pslide_config->video_volume, LV_ANIM_ON);
    lv_obj_set_style_bg_color(pvideo_handle->volume_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(pvideo_handle->volume_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
    lv_obj_add_flag(pvideo_handle->volume_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pvideo_handle->volume_obj, divoom_lvgl_photo_setting_menu_video_menu_event_cb, LV_EVENT_KEY, phandle);
    

    
    lv_obj_add_flag(pvideo_handle->main_obj, LV_OBJ_FLAG_HIDDEN);
}

void divoom_lvgl_photo_setting_menu_change_menu(Pdivoom_lvgl_photo_setting_menu_ctrl phandle, int select_id)
{
    int i;
    
    for(i = 0; i < 4; i++)
    {
        if(i == select_id)
        {
            lv_obj_clear_flag(phandle->select_bg_obj[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->select_bg_obj[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(select_id == 0)
    {
        lv_obj_clear_flag(phandle->slide_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->display_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->component_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->video_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else if(select_id == 1)
    {
        lv_obj_add_flag(phandle->slide_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->display_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->component_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->video_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else if(select_id == 2)
    {
        lv_obj_add_flag(phandle->slide_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->display_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->component_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->video_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else 
    {
        lv_obj_add_flag(phandle->slide_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->display_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->component_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->video_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
    }
    
}

static void divoom_lvgl_photo_setting_menu_left_btn_event_cb(lv_event_t * e)
{
    int i;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle = (Pdivoom_lvgl_photo_setting_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        if(cur_obj == phandle->back_menu_obj)
        {
            divoom_menu_control_return_pre();
            return;
        }
        
        for(i = 0; i < 4; i++)
        {
            if(cur_obj == phandle->select_btn_obj[i])
            {
                divoom_lvgl_photo_setting_menu_change_menu(phandle, i);
                break;
            }
        }
    }
}

void *divoom_lvgl_photo_setting_menu_init(void *user_data, int menu_id)
{
    int i;
    void *label_obj, *image_obj, *btn_obj;
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle;

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_photo_setting_menu_ctrl));
    phandle->menu_id = menu_id;
    phandle->main_obj = lv_obj_create(NULL);    
    divoom_menu_control_init_lable_style(&phandle->lable_style);
    lv_obj_add_style(phandle->main_obj, &phandle->lable_style, 0);
    image_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(image_obj, 0, 0);
	lv_obj_set_size(image_obj, 290, 1280);
    divoom_menu_control_lv_img_set_src(image_obj, "20/20_setting_bg.png");
    
    image_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(image_obj, 13, 21);
	lv_obj_set_size(image_obj, 48, 48);
    divoom_menu_control_lv_img_set_src(image_obj, "20/20_back.png");
    
    phandle->back_menu_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event(image_obj, divoom_lvgl_photo_setting_menu_left_btn_event_cb, LV_EVENT_KEY, phandle);
        
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 28, 28);
    lv_obj_set_size(label_obj, 234, 36);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Setting"));
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(36, phandle->menu_id), 0);    

    

    for(i = 0; i < 4; i++)
    {
        image_obj = lv_img_create(phandle->main_obj);
        lv_obj_set_pos(image_obj, 28, 90 + i *58);
    	lv_obj_set_size(image_obj, 25, 25);
        divoom_menu_control_lv_img_set_src(image_obj, psetting_menu_list_info[i].image_name);
        
        label_obj = lv_label_create(phandle->main_obj);
        lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(label_obj, 61, 90 + i *58);
    	lv_obj_set_size(label_obj, 201, 25);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str( psetting_menu_list_info[i].disp_str));
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);  
        lv_obj_set_style_text_opa(label_obj, 85*LV_OPA_100/100, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);   
        
        phandle->select_btn_obj[i] = label_obj;
        lv_obj_add_event(label_obj, divoom_lvgl_photo_setting_menu_left_btn_event_cb, LV_EVENT_KEY, phandle);
        
        image_obj = lv_img_create(phandle->main_obj);
        lv_obj_set_pos(image_obj, 14, 85 + i *58);
    	lv_obj_set_size(image_obj, 262, 35);
        divoom_menu_control_lv_img_set_src(image_obj, "20/20_slide_item.png");
        phandle->select_bg_obj[i] = image_obj;
        lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
    } 
    divoom_lvgl_photo_setting_menu_create_slide_menu(phandle);
    divoom_lvgl_photo_setting_menu_create_display_menu(phandle);
    divoom_lvgl_photo_setting_menu_create_component_menu(phandle);
    divoom_lvgl_photo_setting_menu_create_video_menu(phandle);
    divoom_lvgl_photo_setting_menu_change_menu(phandle, 0);
    
    return phandle;
}

void divoom_lvgl_photo_setting_menu_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle = (Pdivoom_lvgl_photo_setting_menu_ctrl)handle;

    
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_photo_setting_menu_exit(void *handle)
{
    Pdivoom_lvgl_photo_setting_menu_ctrl phandle = (Pdivoom_lvgl_photo_setting_menu_ctrl)handle;
    
    lv_style_reset(&phandle->lable_style);
    if(phandle->display_menu.fill_dialog)
    {
        divoom_lvgl_message_box_destroy(phandle->display_menu.fill_dialog, 1);
    }
    lv_obj_del(phandle->main_obj);
    divoom_system_free(phandle);
}


