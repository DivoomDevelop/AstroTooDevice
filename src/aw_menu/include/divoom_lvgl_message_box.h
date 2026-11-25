#ifndef _divoom_lvgl_message_box_h_

#define _divoom_lvgl_message_box_h_

#include "divoom_type.h"

//如果ctrl_id<0,表示用户按了取消键
typedef void divoom_lvgl_message_box_event_callback(void *handle, int ctrl_id);
typedef void divoom_lvgl_message_box_focus_callback(void *user_data, int ctrl_id);

/*********************************************************************
  Function:        divoom_lvgl_message_box_init
  Description:     创建消息提示框
  Input:           main_obj  主窗体句柄
                   user_data   
                   pcallback   确定或取消的回调
                   pos_x   
                   pos_y
                   size_x
                   size_y
                   bg_path  背景图案如果为空，将使用bg_color
                   bg_color
  Return:          句柄
**********************************************************************/
void *divoom_lvgl_message_box_init(int menu_id, void *main_obj, void *user_data, divoom_lvgl_message_box_event_callback *pcallback, int pos_x, int pos_y, int size_x, int size_y, char *bg_path, lv_color_t bg_color, int max_item);


/*********************************************************************
  Function:        divoom_lvgl_message_box_add_label_item
  Description:     为提示框增加一个LABEL控件
  Input:           handle  主窗体句柄
                   disp_str   
                   ctrl_id   控件ID必须是唯一的,确定后返回的值，如果不关心可以设置为-1
                   font_id   字号
                   font_color  字体颜色
                   align_mode  对齐模式
                   pos_x   
                   pos_y
                   size_x
                   size_y
  Return:          NONE
**********************************************************************/

void divoom_lvgl_message_box_add_label_item(void *handle, char *disp_str, int ctrl_id, int font_id, lv_color_t font_color, int align_mode,int pos_x, int pos_y, int size_x, int size_y);

/*********************************************************************
  Function:        divoom_lvgl_message_box_add_label_item
  Description:     为提示框增加一个LABEL图像
  Input:           handle  主窗体句柄
                   image_path   
                   ctrl_id   控件ID必须是唯一的,确定后返回的值，如果不关心可以设置为-1
                   pos_x   
                   pos_y
                   size_x
                   size_y
  Return:          NONE
**********************************************************************/

void divoom_lvgl_message_box_add_image_item(void *handle, char *image_path, int ctrl_id, int pos_x, int pos_y, int size_x, int size_y);
/*********************************************************************
  Function:        divoom_lvgl_message_box_add_label_item
  Description:     为提示框增加一个button控件
  Input:           handle  主窗体句柄
                   btn_name    
                   font_id   字号
                   font_color  字体颜色
                   ctrl_id   控件ID必须是唯一的,确定后返回的值，如果不关心可以设置为-1
                   pos_x   
                   pos_y
                   size_x
                   size_y
  Return:          NONE
**********************************************************************/

void divoom_lvgl_message_box_add_btn_item(void *handle, char *btn_name, int font_id, lv_color_t normal_color, lv_color_t active_color,lv_color_t font_color,int ctrl_id, int pos_x, int pos_y, int size_x, int size_y);

/*********************************************************************
  Function:        divoom_lvgl_message_box_destroy
  Description:     释放所有资源
  Input:           handle  主窗体句柄
  Return:          NONE
**********************************************************************/

void divoom_lvgl_message_box_destroy(void *handle, int sync_flag);

/*********************************************************************
  Function:        divoom_lvgl_message_box_set_active_item
  Description:     设置焦点控件
  Input:           handle  主窗体句柄
                   ctrl_id  控件ID
  Return:          NONE
**********************************************************************/
void divoom_lvgl_message_box_set_active_item(void *handle, int ctrl_id);

int divoom_lvgl_message_box_get_active_item(void *handle);

void divoom_lvgl_message_box_set_focus_change_cb(void *handle,
	divoom_lvgl_message_box_focus_callback *pcallback, void *user_data);

/* block_next_pre=1：弹层内忽略 NEXT/PRE（仅特殊场景；列表选择器默认用 NEXT/PRE 切换项） */
void divoom_lvgl_message_box_set_block_next_pre(void *handle, uint8 block_next_pre);

void divoom_lvgl_message_box_hide(void *handle);

#endif


