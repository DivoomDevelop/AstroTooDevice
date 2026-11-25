#ifndef _divoom_download_aliyun_file_h_
#define  _divoom_download_aliyun_file_h_

struct _divoom_download_aliyun_file_item;

typedef void divoom_download_aliyun_file_finish_callback(struct _divoom_download_aliyun_file_item *handle);

typedef struct _divoom_download_aliyun_file_item
{
    int no_del_flag;
    int server_flag;
    int fail_times;
    int user_id;
    int photo_x;
    int photo_y;
    int photo_w;
    int photo_h;
    int album_id;
    int clock_pos;
    int parent_id;
    int photo_id;
    int photo_flag;
    int photo_total;
    int file_size;
    long photo_time;
    double save_time;
    char os_name[128];
    char preview_os_name[128];
    char photo_title[512];
    divoom_download_aliyun_file_finish_callback *pcallback;
}divoom_download_aliyun_file_item, *Pdivoom_download_aliyun_file_item;

/*********************************************************************
  Function:        divoom_download_aliyun_file_init
  Description:     初始化
  Input:           none
  Return:          none
**********************************************************************/

void divoom_download_aliyun_file_init(void);
void divoom_download_aliyun_file_stop(void);

/*********************************************************************
  Function:        divoom_download_aliyun_file_add_item
  Description:     增加一个文件下载
  Input:           pnew_item
                   pcallback
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_add_item(Pdivoom_download_aliyun_file_item pnew_item, int num,divoom_download_aliyun_file_finish_callback *pcallback, int init_flag);


/*********************************************************************
  Function:        divoom_download_aliyun_file_add_vision_item
  Description:     增加一个文件下载
  Input:           vision_id
                   oss_name
                   pcallback
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_add_vision_item(int vision_id, char *oss_name,divoom_download_aliyun_file_finish_callback *pcallback);

/*********************************************************************
  Function:        divoom_download_aliyun_file_add_local_pic_list
  Description:     APP通过本地发送一批文件
  Input:           papp_info
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_add_local_pic_list(void *para);


/*********************************************************************
  Function:        divoom_download_aliyun_add_create_thumb_pic
  Description:     预览图不存在，增加创建预览图
  Input:           file_name
  Return:          none
**********************************************************************/

void divoom_download_aliyun_add_create_thumb_pic(char *file_name);

/*********************************************************************
  Function:        divoom_download_aliyun_file_set_download_flag
  Description:     设置下载标记
  Input:           flag
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_set_download_flag(int flag);


/*********************************************************************
  Function:        divoom_download_aliyun_file_set_app_last_info
  Description:     设置下载文件的进度条
  Input:           total
                   cur_id
  Return:          none
**********************************************************************/

void divoom_download_aliyun_file_set_app_last_info(int total, int cur_id);

/*********************************************************************
  Function:        divoom_download_aliyun_file_get_app_last_info
  Description:     获取APP最后发送的相片信息
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

int divoom_download_aliyun_file_get_app_last_info(int *album_id, int *pic_id, int *total, int *cur_id);

/*********************************************************************
  Function:        divoom_download_aliyun_file_app_del_pic_notice
  Description:     App删除相簿或相片时，调用这个进行更新，如果删除相簿，将pic_id= 0
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

void divoom_download_aliyun_file_app_del_pic_notice(int album_id, int pic_id);

/*********************************************************************
  Function:        divoom_download_aliyun_file_add_cloud_photo_item
  Description:     增加一个文件下载
  Input:           vision_id
                   oss_name
                   pcallback
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_add_cloud_photo_item(int vision_id, int oss_flag,char *oss_name,divoom_download_aliyun_file_finish_callback *pcallback);
void *divoom_download_aliyun_create_thumb_pic_info(void *para);
void divoom_download_aliyun_file_task(void *user_data);

#endif

