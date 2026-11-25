#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_restore_ctrl
{
    int menu_id;
    int select_item;
    void *pdialog_handle;
    void *handle;
}divoom_lvgl_setting_restore_ctrl, *Pdivoom_lvgl_setting_restore_ctrl;


int divoom_lvgl_setting_restore_callback(void *handle, int ret_value, uint8 *select_info, int item_num)
{
    return 1;
}

void divoom_lvgl_setting_restore_select_confirm_fill_diallog_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_setting_restore_ctrl phandle = (Pdivoom_lvgl_setting_restore_ctrl)handle;

    divoom_print("confirm id: %d\n", ctrl_id);

    divoom_lvgl_message_box_destroy(phandle->pdialog_handle, 0);
    if(ctrl_id == 0)
    {
        //这个是确认
        int event_info[] = {DIVOOM_MAIN_EVENT_CONFIG_NET_WORK, DIVOOM_MAIN_EVENT_FORMAT_USERDATA, DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING};
        divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, event_info[phandle->select_item]);
    }
    phandle->pdialog_handle = NULL;
}

void divoom_lvgl_setting_restore_select_callback(void *handle, int select_item)
{
    Pdivoom_lvgl_setting_restore_ctrl phandle = (Pdivoom_lvgl_setting_restore_ctrl)handle;

    phandle->select_item = select_item;
    phandle->pdialog_handle = divoom_lvgl_message_box_init(phandle->menu_id, lv_scr_act(), phandle, divoom_lvgl_setting_restore_select_confirm_fill_diallog_callback
        , 37, 460, 726, 360, "26/26_confirm_dialog.png", lv_color_hex(0x222222), 2);

    if(select_item == 0)
    {
        divoom_lvgl_message_box_add_label_item(phandle->pdialog_handle, divoom_menu_control_get_disp_str("ResetWifiConfig"), 
            -1, 32, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER, 20, 27, 686, 42);
        divoom_lvgl_message_box_add_label_item(phandle->pdialog_handle, divoom_menu_control_get_disp_str("ResetWifiConfigConfirm"), 
            -1, 32, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER, 20, 90, 686, 180);
    }
    else if(select_item == 1)
    {
        divoom_lvgl_message_box_add_label_item(phandle->pdialog_handle, divoom_menu_control_get_disp_str("FormatSDConfig"), 
            -1, 32, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER, 20, 20, 686, 42);
        divoom_lvgl_message_box_add_label_item(phandle->pdialog_handle, divoom_menu_control_get_disp_str("FormatSDConfigConfirm"), 
            -1, 32, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER, 20, 90, 686, 180);
    }
    else
    {
        divoom_lvgl_message_box_add_label_item(phandle->pdialog_handle, divoom_menu_control_get_disp_str("FormatSDConfig"), 
            -1, 32, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER, 20, 20, 686, 42);
        divoom_lvgl_message_box_add_label_item(phandle->pdialog_handle, divoom_menu_control_get_disp_str("FormatSDConfigConfirm"), 
            -1, 32, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER, 20, 90, 686, 180);
    }
    
    divoom_lvgl_message_box_add_label_item(phandle->pdialog_handle, divoom_menu_control_get_disp_str("Confirm"), 
        0, 32, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER, 0, 294, 309, 37);
    divoom_lvgl_message_box_add_label_item(phandle->pdialog_handle, divoom_menu_control_get_disp_str("Cancel"), 
        1, 32, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER, 311, 294, 309, 37);
}

void *divoom_lvgl_setting_restore_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_restore_ctrl phandle;
    divoom_lvgl_normal_menu_list_info para_info = {0};
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_restore_ctrl));
    phandle->menu_id = menu_id;
    para_info.phandle = phandle;
    para_info.bg_width = 726;
    para_info.bg_high = 270;
    para_info.bg_image_path = "26/26_3line_bg.png";   //背景图片
    para_info.disp_title = divoom_menu_control_get_disp_str("Restore");   //标题
    para_info.confirm_flag = 0;  //是否有确认按键
    para_info.muti_flag = 0;     //是否支持多选
    para_info.menu_items = 3;    //菜单项总数
    para_info.select_str[0] = divoom_menu_control_get_disp_str("ResetWifiConfig");
    para_info.select_str[1] = divoom_menu_control_get_disp_str("FormatSDConfig");
    para_info.select_str[2] = divoom_menu_control_get_disp_str("RestoreFactory");
    para_info.select_flag = 1;
    para_info.pcallback = divoom_lvgl_setting_restore_callback;
    para_info.pchange_callback = divoom_lvgl_setting_restore_select_callback;

    phandle->handle = divoom_lvgl_normal_menu_list_init(&para_info, menu_id);

    return phandle;
}

void divoom_lvgl_setting_restore_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_restore_ctrl phandle = (Pdivoom_lvgl_setting_restore_ctrl)handle;

    divoom_lvgl_normal_menu_list_disp(phandle->handle, active_obj);
}

void divoom_lvgl_setting_restore_exit(void *handle)
{
    Pdivoom_lvgl_setting_restore_ctrl phandle = (Pdivoom_lvgl_setting_restore_ctrl)handle;

    if(phandle->pdialog_handle)
    {
        divoom_lvgl_message_box_destroy(phandle->pdialog_handle, 1);
    }
    divoom_lvgl_normal_menu_list_exit(phandle->handle);
    divoom_system_free(phandle);
}


