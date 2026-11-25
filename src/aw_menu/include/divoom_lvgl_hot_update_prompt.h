#ifndef _divoom_lvgl_hot_update_prompt_h_
#define _divoom_lvgl_hot_update_prompt_h_

/* App 选择云表盘后，在每个自然日首次命令完成切表盘后显示询问页。 */
void divoom_lvgl_hot_update_prompt_request(void);

void *divoom_lvgl_hot_update_prompt_init(void *user_data, int menu_id);
void divoom_lvgl_hot_update_prompt_disp(void *handle, void *active_obj);
void divoom_lvgl_hot_update_prompt_pause(void *handle);
void divoom_lvgl_hot_update_prompt_exit(void *handle);

#endif
