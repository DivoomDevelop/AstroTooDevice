#ifndef _divoom_photo_play_ctrl_h_
#define _divoom_photo_play_ctrl_h_


void *divoom_photo_play_ctrl_init(void *main_obj,int start_x, int start_y,int width, int high, int album_id,int effect_id, int play_speed, int parent_id, int clock_pos, int play_movie, char *default_name, int default_model_id, int default_img_id);
//获取正在播放的表盘相簿信息
int divoom_photo_play_ctrl_get_play_album_info(Pdivoom_clock_id_info pclock_info, int *pic_id);



void divoom_photo_play_ctrl_update_effect(void *handle, int effect_id);
void divoom_photo_play_ctrl_update_album_id(void *handle, int album_id);


void divoom_photo_play_ctrl_update_speed(void *handle, int play_speed);
void divoom_photo_play_ctrl_play(void *handle, int play_status);
void divoom_photo_play_ctrl_pause(void *handle);
void divoom_photo_play_ctrl_exit(void *handle);
void divoom_photo_play_ctrl_update_all_info(void *handle, int album_id, int slide_mode, int play_speed, int parent_id, int clock_pos);

#endif

