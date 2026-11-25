/*
 * AstroToo 过渡期：middle/app 仍引用旧 aw_menu 顶层/表盘 API 时的空实现。
 * 随各模块迁入 astrootoo/ 后逐步删除对应桩。
 */
#include "divoom.h"

#ifdef DIVOOM_PRODUCT_ASTRO_TOO

#include "divoom_lvgl_top_main_menu.h"
#include "divoom_lvgl_top_ambient_menu.h"
#include "divoom_lvgl_top_bright_volume_menu.h"
#include "divoom_lvgl_top_reached_home_menu.h"
#include "divoom_lvgl_top_anniversary_reminder_menu.h"
#include "divoom_lvgl_top_danmaku_menu.h"
#include "divoom_lvgl_top_social_reminder_menu.h"
#include "divoom_lvgl_top_audio_tip_menu.h"
#include "divoom_lvgl_top_battery_menu.h"
#include "divoom_lvgl_top_audio_menu.h"
#include "divoom_lvgl_wifi_disconnect.h"
#include "divoom_lvgl_base_dial_menu.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_tomato_time_menu.h"
#include "divoom_white_noise_menu.h"

void divoom_lvgl_top_main_menu_init(void)
{
}

void divoom_lvgl_top_main_menu_exit(void)
{
}

void divoom_lvgl_top_ambient_menu_init(void)
{
}

void divoom_lvgl_top_ambient_menu_exit(void)
{
}

#if 0 /* 真实实现见 divoom_lvgl_top_bright_volume_menu.c（AstroToo / SPEAKER 分支） */
void divoom_lvgl_top_bright_volume_menu_init(int bright_volume_flag)
{
    (void)bright_volume_flag;
}
#endif

void divoom_lvgl_top_reached_home_menu_init(void)
{
}

void divoom_lvgl_top_anniversary_reminder_menu_init(void *para)
{
    (void)para;
}

void divoom_lvgl_top_anniversary_reminder_menu_main_loop(void)
{
}

void divoom_lvgl_top_social_reminder_menu_init(void *para)
{
    (void)para;
}

void divoom_lvgl_top_social_reminder_menu_main_loop(void)
{
}

#if 0 /* 真实实现见 divoom_lvgl_top_audio_tip_menu.c */
void divoom_lvgl_top_audio_tip_menu_init(int audio_tip_flag)
{
    (void)audio_tip_flag;
}
#endif

void divoom_lvgl_top_battery_menu_init(int battery_flag)
{
    (void)battery_flag;
}

void divoom_lvgl_top_audio_menu_init(void)
{
}

void divoom_lvgl_top_audio_menu_refresh_select(void)
{
}

uint8 divoom_lvgl_top_audio_menu_is_open(void)
{
    return 0;
}

void divoom_lvgl_wifi_disconnect_init(void)
{
}

void divoom_lvgl_wifi_disconnect_exit(void)
{
}

#if 0 /* 真实实现见 divoom_lvgl_base_dial_menu.c / divoom_lvgl_dial_menu.c（自 LvglPlatform 恢复） */

int divoom_lvgl_base_dial_menu_is_play_photo_slide(void *handle)
{
    (void)handle;
    return 0;
}

int divoom_lvgl_base_dial_convert_clock_to_slide(int clock_id)
{
    (void)clock_id;
    return 0;
}

void *divoom_lvgl_dial_menu_open_resource_handle(char *file_name, int *resource_type, int *out_w, int *out_h, int *out_num)
{
    (void)file_name;
    if (resource_type)
    {
        *resource_type = 0;
    }
    if (out_w)
    {
        *out_w = 0;
    }
    if (out_h)
    {
        *out_h = 0;
    }
    if (out_num)
    {
        *out_num = 0;
    }
    return NULL;
}

void divoom_lvgl_dial_menu_release_resource_handle(void *pwebp_handle, int resource_type)
{
    (void)pwebp_handle;
    (void)resource_type;
}

void *divoom_lvgl_dial_menu_add_stock_chart(void *handle, Pdivoom_lvgl_dial_menu_stock_chart_info pstock_info, void *obj, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)pstock_info;
    (void)obj;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_text_handle(void *text_handle, Pdivoom_lvgl_dial_menu_text_item ptext_info, void *obj, Pdivoom_clock_id_info pclock_info, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)text_handle;
    (void)ptext_info;
    (void)obj;
    (void)pclock_info;
    (void)user_data;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_move_text_handle(void *text_handle, Pdivoom_lvgl_dial_menu_text_item ptext_info, void *obj, Pdivoom_clock_id_info pclock_info, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)text_handle;
    (void)ptext_info;
    (void)obj;
    (void)pclock_info;
    (void)user_data;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_create_component_handle(void *phandle, Pdivoom_lvgl_dial_menu_component_info pcomponet_info, void *obj)
{
    (void)phandle;
    (void)pcomponet_info;
    (void)obj;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_componet_text_handle(void *text_handle, void *pcomponet_obj, Pdivoom_lvgl_dial_menu_text_item ptext_info, void *obj, Pdivoom_clock_id_info pclock_info, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)text_handle;
    (void)pcomponet_obj;
    (void)ptext_info;
    (void)obj;
    (void)pclock_info;
    (void)user_data;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_component_png_handle(void *pdisp_handle, void *pcomponet_obj, Pdivoom_lvgl_dial_menu_image_item pimage_info, int model_id, int image_id, void *obj, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)pdisp_handle;
    (void)pcomponet_obj;
    (void)pimage_info;
    (void)model_id;
    (void)image_id;
    (void)obj;
    (void)user_data;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_draw_calendar_watch(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int offset_y, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)text_handle;
    (void)pdate_info;
    (void)text_attr;
    (void)obj;
    (void)pclock_info;
    (void)offset_y;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_draw_calendar_watch_with_week(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int week_high, int week_widht, int today_x, int today_y, int week_type, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)text_handle;
    (void)pdate_info;
    (void)text_attr;
    (void)obj;
    (void)pclock_info;
    (void)week_high;
    (void)week_widht;
    (void)today_x;
    (void)today_y;
    (void)week_type;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_draw_calendar_watch3(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)text_handle;
    (void)pdate_info;
    (void)text_attr;
    (void)obj;
    (void)pclock_info;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_draw_calendar_watch_with_week2(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int week_high, int week_widht, int today_x, int today_y, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)text_handle;
    (void)pdate_info;
    (void)text_attr;
    (void)obj;
    (void)pclock_info;
    (void)week_high;
    (void)week_widht;
    (void)today_x;
    (void)today_y;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_hour_min_color_text(void *text_handle, int hour, int min, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info)
{
    (void)text_handle;
    (void)hour;
    (void)min;
    (void)text_attr;
    (void)obj;
    (void)pclock_info;
    return NULL;
}

void *divoom_lvgl_dial_menu_draw_calendar_watch_with_img(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int img_width, int img_offset, divoom_lvgl_dial_menu_set_obj_level_callback *plevel_callback)
{
    (void)handle;
    (void)text_handle;
    (void)pdate_info;
    (void)text_attr;
    (void)obj;
    (void)pclock_info;
    (void)img_width;
    (void)img_offset;
    (void)plevel_callback;
    return NULL;
}

void *divoom_lvgl_dial_menu_draw_calendar_watch_with_week_use_img(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int week_high, int week_widht, int today_x, int today_y, int week_type, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)text_handle;
    (void)pdate_info;
    (void)text_attr;
    (void)obj;
    (void)pclock_info;
    (void)week_high;
    (void)week_widht;
    (void)today_x;
    (void)today_y;
    (void)week_type;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_pinball_marble(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)pimage_info;
    (void)obj;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_local_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int lock_flag, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback, uint8 *exit_flag, int reload_flag)
{
    (void)handle;
    (void)pimage_info;
    (void)obj;
    (void)lock_flag;
    (void)user_data;
    (void)pcallback;
    (void)exit_flag;
    (void)reload_flag;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_adaptive_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int lock_flag, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback, uint8 *exit_flag)
{
    (void)handle;
    (void)pimage_info;
    (void)obj;
    (void)lock_flag;
    (void)user_data;
    (void)pcallback;
    (void)exit_flag;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_rotate_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj)
{
    (void)handle;
    (void)pimage_info;
    (void)obj;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_divoom_file_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int apha_flag, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback, int is_app_flag)
{
    (void)handle;
    (void)pimage_info;
    (void)obj;
    (void)apha_flag;
    (void)user_data;
    (void)pcallback;
    (void)is_app_flag;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_photo_slide_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int movie_flag, int clock_id, int disp_type, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)pimage_info;
    (void)obj;
    (void)movie_flag;
    (void)clock_id;
    (void)disp_type;
    (void)user_data;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_local_part_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, int *start, int cnt, int list_num, void *obj, int play_speed, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)pimage_info;
    (void)start;
    (void)cnt;
    (void)list_num;
    (void)obj;
    (void)play_speed;
    (void)user_data;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_local_average_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, int start, int total, void *obj, int play_speed, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback)
{
    (void)handle;
    (void)pimage_info;
    (void)start;
    (void)total;
    (void)obj;
    (void)play_speed;
    (void)user_data;
    (void)pcallback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_divoom_eq_info(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, DIVOOM_GET_EQ_DISP_BUF_CALLBACK pcallback, void *obj, void *user_data, void *clock_handle, divoom_lvgl_dial_menu_set_obj_level_callback *plevel_callback)
{
    (void)handle;
    (void)pimage_info;
    (void)pcallback;
    (void)obj;
    (void)user_data;
    (void)clock_handle;
    (void)plevel_callback;
    return NULL;
}

void *divoom_lvgl_dial_menu_add_divoom_image_queue(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, DIVOOM_PLAY_GIF_COM_GET_NEW_ID pcallback, DIVOOM_PLAY_GIF_COM_GET_PLAY_TIME gif_time_callback, int index, void *obj, void *user_data, void *clock_handle, divoom_lvgl_dial_menu_set_obj_level_callback *plevel_callback, DIVOOM_PLAY_GIF_COM_GET_CHANGE_FLAG pchange_callback)
{
    (void)handle;
    (void)pimage_info;
    (void)pcallback;
    (void)gif_time_callback;
    (void)index;
    (void)obj;
    (void)user_data;
    (void)clock_handle;
    (void)plevel_callback;
    (void)pchange_callback;
    return NULL;
}

void divoom_lvgl_dial_menu_del_image_disp_handle(void *pimage_handle, int lock_flag)
{
    (void)pimage_handle;
    (void)lock_flag;
}

int divoom_lvgl_dial_menu_check_handle_exist(void)
{
    return 0;
}

void divoom_lvgl_dial_menu_init_lyric_shell(void *main_obj, int menu_id, int clock_id)
{
    (void)main_obj;
    (void)menu_id;
    (void)clock_id;
}

void divoom_lvgl_dial_menu_exit_lyric_shell(void)
{
}

void divoom_lvgl_dial_menu_del_component_disp_handle(void *pcomponent_handle, int lock_flag)
{
    (void)pcomponent_handle;
    (void)lock_flag;
}

void divoom_lvgl_dial_menu_clear_all(void *handle)
{
    (void)handle;
}

void divoom_lvgl_dial_menu_set_finish_image_handle_callback(void *handle, DIVOOM_LVGL_DIAL_MENU_FINISH_CALLBACK pcallback, void *user_data)
{
    (void)handle;
    (void)pcallback;
    (void)user_data;
}

void divoom_lvgl_dial_menu_pause_image_handle(void *handle)
{
    (void)handle;
}

void divoom_lvgl_dial_menu_del_obj(void *obj)
{
    (void)obj;
}

void *divoom_lvgl_dial_menu_add_obj(int x, int y, int w, int h, int flag, int angle)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)flag;
    (void)angle;
    return NULL;
}

void divoom_lvgl_dial_menu_pause_all_in(void *handle)
{
    (void)handle;
}

void *divoom_lvgl_dial_menu_add_play_video_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj)
{
    (void)handle;
    (void)pimage_info;
    (void)obj;
    return NULL;
}

#endif /* legacy dial stubs */

#endif
