#ifndef _divoom_lvgl_top_danmaku_menu_h_
#define _divoom_lvgl_top_danmaku_menu_h_

void divoom_lvgl_top_danmaku_menu_init(void);

void divoom_lvgl_top_danmaku_menu_exit(void);

void divoom_lvgl_top_danmaku_menu_add_danmaku_image_item(int change_flag, int lock_flag, int face_index);
void divoom_lvgl_top_danmaku_menu_add_danmaku_label_item(char *disp_info, int text_color, int lock_flag, int user_id);
void divoom_lvgl_top_danmaku_menu_del_menu(void);
//void divoom_lvgl_top_danmaku_menu_update_danmaku_config(int lock_flag);
//void divoom_lvgl_top_danmaku_menu_add_danmaku_label_item_test(char *disp_info, int text_color, int lock_flag);


#endif

