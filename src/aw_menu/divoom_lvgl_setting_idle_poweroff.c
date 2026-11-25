#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_idle_poweroff.h"

typedef struct _divoom_lvgl_setting_idle_poweroff_ctrl
{
    int menu_id;
    char disp_info[6][128];
    void *handle;
}divoom_lvgl_setting_idle_poweroff_ctrl, *Pdivoom_lvgl_setting_idle_poweroff_ctrl;

int divoom_lvgl_setting_idle_poweroff_callback(void *handle, int ret_value, uint8 *select_info, int item_num)
{
    int i;
    Pdivoom_lvgl_setting_idle_poweroff_ctrl phandle = (Pdivoom_lvgl_setting_idle_poweroff_ctrl)handle;

    for(i = 0; i < item_num; i++)
    {
        if(select_info[i])
        {
            Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

            psys_config->idle_poweroff_time = divoom_idle_poweroff_secs[i];
			divoom_main_save_config();
            return 1;
        }
    }
    return 1;
}

void *divoom_lvgl_setting_idle_poweroff_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_idle_poweroff_ctrl phandle;
    divoom_lvgl_normal_menu_list_info para_info = {0};
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_idle_poweroff_ctrl));
    phandle->menu_id = menu_id;
    para_info.phandle = phandle;
    //para_info.bg_width = 726;
    //para_info.bg_high = 540;
    //para_info.bg_image_path = "26/26_6line_bg.png";   //背景图片
    para_info.disp_title = divoom_menu_control_get_disp_str("IdlePowerOffTime");   //标题
    para_info.confirm_flag = 0;  //是否有确认按键
    para_info.muti_flag = 0;     //是否支持多选
    para_info.select_flag = 1;	 //是否有选择按键
    para_info.menu_items = DIVOOM_IDLE_POWEROFF_OPTION_CNT;    //菜单项总数
    for(int i = 0; i < DIVOOM_IDLE_POWEROFF_OPTION_CNT; i++)
    {
        if(psys_config->idle_poweroff_time == divoom_idle_poweroff_secs[i])
        {
            para_info.select_info[i] = 1;
        }
        else
        {
            para_info.select_info[i] = 0;
        }
        if(divoom_idle_poweroff_secs[i] == 0)
        {
            strcpy(phandle->disp_info[i], divoom_menu_control_get_disp_str("Never"));
        }
        else if(divoom_idle_poweroff_secs[i] < 3600)
        {
            sprintf(phandle->disp_info[i], "%d%s",divoom_idle_poweroff_secs[i]/ 60, divoom_menu_control_get_disp_str("Minute"));
        }
        else
        {
            sprintf(phandle->disp_info[i], "%d%s",divoom_idle_poweroff_secs[i]/3600, divoom_menu_control_get_disp_str("Hour"));
        }
        para_info.select_str[i] = phandle->disp_info[i];   //下面菜单选择项
    }
    para_info.pcallback = divoom_lvgl_setting_idle_poweroff_callback;

    phandle->handle = divoom_lvgl_normal_menu_list_init(&para_info, menu_id);

    return phandle;
}

void divoom_lvgl_setting_idle_poweroff_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_idle_poweroff_ctrl phandle = (Pdivoom_lvgl_setting_idle_poweroff_ctrl)handle;

    divoom_lvgl_normal_menu_list_disp(phandle->handle, active_obj);
}

void divoom_lvgl_setting_idle_poweroff_exit(void *handle)
{
    Pdivoom_lvgl_setting_idle_poweroff_ctrl phandle = (Pdivoom_lvgl_setting_idle_poweroff_ctrl)handle;

    divoom_lvgl_normal_menu_list_exit(phandle->handle);
    divoom_system_free(phandle);
}


