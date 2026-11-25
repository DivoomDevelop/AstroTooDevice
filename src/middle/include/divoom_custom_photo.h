#ifndef _divoom_custom_photo_h_
#define _divoom_custom_photo_h_
void divoom_custom_photo_init(void);
void divoom_custom_photo_clear_all(Pdivoom_clock_id_info clock_info);
void divoom_custom_photo_delete_item(Pdivoom_clock_id_info clock_info, int id);
void *divoom_custom_photo_open(Pdivoom_clock_id_info clock_info, int order_flag, int reverse_flag, int *play_pos);
int divoom_custom_photo_get_pos_info(Pdivoom_clock_id_info clock_info, int *pause_flag);
void divoom_custom_photo_update_pos_info(Pdivoom_clock_id_info clock_info, int pic_id);
void divoom_custom_photo_set_play_one(Pdivoom_clock_id_info clock_info, int pic_id);
int divoom_custom_photo_get_play_one_flag(void *handle);

int divoom_custom_photo_get_total_items(void *handle);
int divoom_custom_photo_get_last_pic_id(void *handle);

int divoom_custom_photo_get_file_name(void *handle,char *file_name);
int divoom_custom_photo_get_file_name_from_pos(void *handle, int pos, char *file_name);
void divoom_custom_photo_close(void *handle);
//得到相片ID
int divoom_custom_photo_get_pic_id(void *handle, int pos);
//得到相片位置
int divoom_custom_photo_get_pic_pos(void *handle,int pic_id);
int divoom_custom_photo_add_item(Pdivoom_clock_id_info clock_info, double save_time, int photo_time, char *file_name);
/*********************************************************************
  Function:        divoom_custom_photo_get_album_local_pic_list
  Description:     APP获取相簿相片列表
  Input:           clock_info
                   start
                   end
  Return:          NONE
**********************************************************************/

void divoom_custom_photo_get_album_local_pic_list(Pdivoom_clock_id_info clock_info, int start, int end);
/*********************************************************************
  Function:        divoom_custom_photo_get_pic_name_from_id
  Description:     通过id得到文件名
  Input:           pic_id
                   file_name
  Return:          NONE
**********************************************************************/

void divoom_custom_photo_get_pic_name_from_id(int pic_id, char *file_name);

#endif

