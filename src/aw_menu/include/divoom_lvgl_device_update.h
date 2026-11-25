#ifndef _divoom_lvgl_device_update_h_
#define _divoom_lvgl_device_update_h_

void *divoom_lvgl_device_update_init(void *user_data, int menu_id);
void divoom_lvgl_device_update_disp(void *handle, void *active_obj);
void divoom_lvgl_device_update_exit(void *handle);

void *divoom_lvgl_jl_ota_update_init(void *user_data, int menu_id);
void divoom_lvgl_jl_ota_update_disp(void *handle, void *active_obj);
void divoom_lvgl_jl_ota_update_exit(void *handle);
void divoom_lvgl_jl_ota_update_set_percent(uint8 percent);

/* F101 主控 ZIP 解压阶段：ota_rtos 线程写 pending，LVGL 定时器刷新 */
void divoom_lvgl_device_update_begin_unzip(void);
void divoom_lvgl_device_update_begin_flash(void);
void divoom_lvgl_device_update_set_ota_progress(uint8 percent);
void divoom_lvgl_device_update_end_ota_progress(void);

/* 插卡升级进度（页 54）。勿写入 s_rtos_ota_ui_* / s_jl_ota_pending_percent */
void divoom_lvgl_device_update_tf_reset(void);
void divoom_lvgl_device_update_tf_set_percent(uint8 percent);
void divoom_lvgl_device_update_tf_end(void);

#endif

