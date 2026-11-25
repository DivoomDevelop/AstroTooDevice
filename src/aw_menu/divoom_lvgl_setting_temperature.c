#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_temperature_ctrl
{
    int menu_id;
    void *handle;
}divoom_lvgl_setting_temperature_ctrl, *Pdivoom_lvgl_setting_temperature_ctrl;

int divoom_lvgl_setting_temperature_callback(void *handle, int ret_value, uint8 *select_info, int item_num)
{
    int temp_flag;
    Pdivoom_lvgl_setting_temperature_ctrl phandle = (Pdivoom_lvgl_setting_temperature_ctrl)handle;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

	temp_flag = psys_config->temp_c_f_flag;
    if(select_info[0])
    {
        //temp_flag = 0;
        psys_config->temp_c_f_flag = 0;
    }
    else
    {
        //temp_flag = 1;
        psys_config->temp_c_f_flag = 1;
    }
    if(psys_config->temp_c_f_flag  != temp_flag)
    {
    	divoom_main_send_sys_settings_config();
        divoom_main_save_config();
    }
    return 1;
}

void *divoom_lvgl_setting_temperature_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_temperature_ctrl phandle;
    divoom_lvgl_normal_menu_list_info para_info = {0};
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_temperature_ctrl));
    phandle->menu_id = menu_id;
    para_info.phandle = phandle;
    //para_info.bg_width = 726;
    //para_info.bg_high = 180;
    //para_info.bg_image_path = "26/26_2line_bg.png";   //背景图片
    para_info.disp_title = divoom_menu_control_get_disp_str("Temperature");   //标题
    para_info.confirm_flag = 0;  //是否有确认按键
    para_info.muti_flag = 0;     //是否支持多选
    para_info.menu_items = 2;    //菜单项总数
	para_info.title_tip_flag = 0;
    para_info.select_str[0] = divoom_menu_control_get_disp_str("Centigrade");   //下面菜单选择项
    para_info.select_str[1] = divoom_menu_control_get_disp_str("Fahrenheit");   //下面菜单选择项
    para_info.select_info[psys_config->temp_c_f_flag] = 1;
	para_info.select_flag = 1;
    para_info.pcallback = divoom_lvgl_setting_temperature_callback;

    phandle->handle = divoom_lvgl_normal_menu_list_init(&para_info, menu_id);

    return phandle;
}

void divoom_lvgl_setting_temperature_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_temperature_ctrl phandle = (Pdivoom_lvgl_setting_temperature_ctrl)handle;

    divoom_lvgl_normal_menu_list_disp(phandle->handle, active_obj);
}

void divoom_lvgl_setting_temperature_exit(void *handle)
{
    Pdivoom_lvgl_setting_temperature_ctrl phandle = (Pdivoom_lvgl_setting_temperature_ctrl)handle;

    divoom_lvgl_normal_menu_list_exit(phandle->handle);
    divoom_system_free(phandle);
}



