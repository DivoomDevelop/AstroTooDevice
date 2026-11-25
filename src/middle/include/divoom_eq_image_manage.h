#ifndef _divoom_eq_image_manage_h_
#define _divoom_eq_image_manage_h_

typedef struct _divoom_eq_image_manage_image_item
{
    uint32  image_id;
    int     user_id;
    char    image_name[200];
    char file_id[64];
}divoom_eq_image_manage_image_item, *Pdivoom_eq_image_manage_image_item;


void *divoom_eq_image_manage_open_handle(Pdivoom_clock_id_info pclock_info);
void divoom_eq_image_manage_close_handle(Pdivoom_clock_id_info pclock_info);
void divoom_eq_image_manage_update_image_list(void *data);
void divoom_eq_image_manage_add_image_id(Pdivoom_clock_id_info pclock_info, int update_time,char *file_id, int user_id, char *image_name);
void divoom_eq_image_manage_clean_item(Pdivoom_clock_id_info pclock_info);
void divoom_eq_image_manage_delete_image_id(Pdivoom_clock_id_info pclock_info, int  index);
int divoom_eq_image_manage_get_image_id(Pdivoom_clock_id_info pclock_info, int  index, int *user_id, char *image_name);
void divoom_eq_image_manage_init(void);
void divoom_eq_image_manage_update_eq_mode(Pdivoom_clock_id_info pclock_info, int mode);
int divoom_eq_image_manage_get_eq_mode(Pdivoom_clock_id_info pclock_info);
/*********************************************************************
  Function:        divoom_eq_image_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_eq_image_manage_get_url_addr(int model_id, int image_id, char *url_addr);
int divoom_eq_image_manage_get_change_eq_mode(Pdivoom_clock_id_info pclock_info);
int  divoom_eq_image_manage_get_update_time(void);

#endif
