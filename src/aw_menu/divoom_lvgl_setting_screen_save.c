#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_screen_save_ctrl
{
    int menu_id;
    char disp_info[6][128];
    void *handle;
}divoom_lvgl_setting_screen_save_ctrl, *Pdivoom_lvgl_setting_screen_save_ctrl;

int divoom_lvgl_setting_screen_save_callback(void *handle, int ret_value, uint8 *select_info, int item_num)
{
    int i;
    Pdivoom_lvgl_setting_screen_save_ctrl phandle = (Pdivoom_lvgl_setting_screen_save_ctrl)handle;

    for(i = 0; i < item_num; i++)
    {
        if(select_info[i])
        {
            Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

            psys_config->screen_save_mode = i;
			divoom_main_send_sys_settings_config();
			divoom_main_save_config();
            return 1;
        }
    }
    return 1;
}

void *divoom_lvgl_setting_screen_save_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_screen_save_ctrl phandle;
    divoom_lvgl_normal_menu_list_info para_info = {0};
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_screen_save_ctrl));
    phandle->menu_id = menu_id;
    para_info.phandle = phandle;
    //para_info.bg_width = 726;
    //para_info.bg_high = 270;
    //para_info.bg_image_path = "26/26_3line_bg.png";   //背景图片
    para_info.disp_title = divoom_menu_control_get_disp_str("ScreenSave");   //标题
    para_info.confirm_flag = 0;  //是否有确认按键
    para_info.muti_flag = 0;     //是否支持多选
    para_info.menu_items = 2;    //菜单项总数
    para_info.select_flag = 1;	//是否有选择按键
    para_info.select_str[0] = divoom_menu_control_get_disp_str("Dial");
    para_info.select_str[1] = divoom_menu_control_get_disp_str("DigitAlbum");
    //para_info.select_str[2] = divoom_menu_control_get_disp_str("Default");
    if(psys_config->screen_save_mode > 1)
    {
        psys_config->screen_save_mode = 0;
    }
    para_info.select_info[psys_config->screen_save_mode] = 1;
    para_info.pcallback = divoom_lvgl_setting_screen_save_callback;

    phandle->handle = divoom_lvgl_normal_menu_list_init(&para_info, menu_id);

    return phandle;
}

void divoom_lvgl_setting_screen_save_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_screen_save_ctrl phandle = (Pdivoom_lvgl_setting_screen_save_ctrl)handle;

    divoom_lvgl_normal_menu_list_disp(phandle->handle, active_obj);
}

void divoom_lvgl_setting_screen_save_exit(void *handle)
{
    Pdivoom_lvgl_setting_screen_save_ctrl phandle = (Pdivoom_lvgl_setting_screen_save_ctrl)handle;

    divoom_lvgl_normal_menu_list_exit(phandle->handle);
    divoom_system_free(phandle);
}



