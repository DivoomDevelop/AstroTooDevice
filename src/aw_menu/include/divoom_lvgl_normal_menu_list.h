#ifndef _divoom_lvgl_normal_menu_list_h_
#define _divoom_lvgl_normal_menu_list_h_
#define DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM   30

typedef int divoom_lvgl_normal_menu_list_callback(void *handle, int ret_value, uint8 *select_info, int item_num);
typedef void divoom_lvgl_normal_menu_list_change_callback(void *handle, int select_id);
typedef struct _divoom_lvgl_normal_menu_list_info
{
    void *phandle;
    int  bg_width;
    int  bg_high;
    char *bg_image_path;   //背景图片
    int  title_image_flag;  	//标题是否使用图片
	char *title_image_path;		//标题图片路径
    char *disp_title;   //标题
	int  title_tip_flag;  //标题提示标签
	char *title_tip[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];
    int  confirm_flag;  //是否有确认按键
    int  select_flag;  //是否有选择按键
    uint8  select_info[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];   //是否选择
    int  muti_flag;     //是否支持多选
    int  menu_items;    //菜单项总数
    int  btn_image_flag;	//按钮文本是否用图片
    char *btn_image_path[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];   //按钮文本图片路径
    char *select_str[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];   //下面菜单选择项
    divoom_lvgl_normal_menu_list_callback *pcallback;
    divoom_lvgl_normal_menu_list_change_callback *pchange_callback;
}divoom_lvgl_normal_menu_list_info, *Pdivoom_lvgl_normal_menu_list_info;

void *divoom_lvgl_normal_menu_list_init(void *user_data, int menu_id);

void divoom_lvgl_normal_menu_list_disp(void *handle, void *active_obj);

void divoom_lvgl_normal_menu_list_exit(void *handle);

#endif

