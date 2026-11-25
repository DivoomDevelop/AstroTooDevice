#ifndef _divoom_lvgl_gallery_transfer_h_
#define _divoom_lvgl_gallery_transfer_h_

void *divoom_lvgl_gallery_transfer_init(void *user_data, int menu_id);

void divoom_lvgl_gallery_transfer_disp(void *handle, void *active_obj);

void divoom_lvgl_gallery_transfer_pause(void *handle);

void divoom_lvgl_gallery_transfer_exit(void *handle);

/* divoom_photo_album_josn / 下载链路进度占位：current/total 为已传/总数 */
void divoom_lvgl_gallery_transfer_set_progress(int current, int total);
void divoom_lvgl_gallery_transfer_bind_session(void *handle, unsigned int session);

#endif
