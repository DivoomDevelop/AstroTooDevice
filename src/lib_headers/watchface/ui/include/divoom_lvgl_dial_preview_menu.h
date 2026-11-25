#ifndef _divoom_lvgl_dial_preview_menu_
#define _divoom_lvgl_dial_preview_menu_

/* 1=表盘预览弹窗显示「收藏/取消收藏」、格子上爱心、长按收藏键；0=仅「设为当前」「移除」 */
#ifndef DIVOOM_DIAL_PREVIEW_FAVORITES_UI
#define DIVOOM_DIAL_PREVIEW_FAVORITES_UI  0
#endif

void divoom_lvgl_dial_preview_menu_queue_destroy(void *img_cache_queue);
void divoom_lvgl_dial_preview_menu_queue_free(void *item);
int divoom_lvgl_dial_preview_menu_queue_compare(void *item1, void *item2);

void divoom_lvgl_dial_preview_market_review_begin(int out_w, int out_h);
void divoom_lvgl_dial_preview_market_review_end(void);

void divoom_lvgl_dial_preview_menu_set_img_cache(int clock_id, int type, int main_clock_id, void *img_cache_queue, void *obj);

void divoom_lvgl_dial_preview_menu_load_img_cache_from_clock_id(int clock_id, void *img_cache_queue, int type, int main_clock_id);

void divoom_lvgl_dial_preview_menu_get_clock_name(int clock_id, uint8 *name_buf);

void divoom_lvgl_dial_preview_menu_get_schedule_clock_id(int clock_id, int *schedule_list);

void *divoom_lvgl_dial_preview_menu_init(void *user_data, int menu_id);

void divoom_lvgl_dial_preview_menu_disp(void *handle, void *active_obj);

void divoom_lvgl_dial_preview_menu_pause(void *handle);

void divoom_lvgl_dial_preview_menu_exit(void *handle);

#endif
