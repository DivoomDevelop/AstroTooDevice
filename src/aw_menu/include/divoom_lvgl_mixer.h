#ifndef _divoom_lvgl_mixer_h_
#define _divoom_lvgl_mixer_h_

void *divoom_lvgl_mixer_init(void *user_data, int menu_id);
void divoom_lvgl_mixer_exit(void *handle);
void divoom_lvgl_mixer_disp(void *handle, void *active_obj);
void divoom_lvgl_mixer_pause(void *handle);

#endif
