#ifndef _divoom_system_font_h_
#define _divoom_system_font_h_



/*********************************************************************
  Function:        divoom_system_font_init
  Description:     系统将可能使用的所有字体，完成系统字体的加载,字体只考虑增加，不考虑删除的情况
  Input:           none
  Return:           NONE
**********************************************************************/

void divoom_system_font_init(void);
/* Factory NEXT: initialize metadata without rescanning/deleting font files. */
void divoom_system_font_init_without_check(void);
/*********************************************************************
  Function:        divoom_system_font_get_first_font_id
  Description:     得到一个系统能用的字体
  Input:           none
  Return:           NONE
**********************************************************************/

int divoom_system_font_get_first_font_id(void);

/*********************************************************************
  Function:        divoom_system_font_redownload_file
  Description:     字体文件损坏，重新下载文件
  Input:           font_id
                   
  Return:           NONE
**********************************************************************/
void divoom_system_font_redownload_file(int font_id);

/*********************************************************************
  Function:        divoom_system_font_download_all_fond_info
  Description:     闲时下载没有下载完的字体
  Input:           
                   
  Return:           NONE
**********************************************************************/

void divoom_system_font_download_all_fond_info(void);

/*********************************************************************
  Function:        divoom_system_font_update
  Description:     更新系统将可能使用的所有字体，完成系统字体的加载
  Input:           pfont_list
                   num
  Return:           NONE
**********************************************************************/

void divoom_system_font_update(Pdivoom_system_font_item pfont_list, int num);

/*********************************************************************
  Function:        divoom_system_font_get_update_time
  Description:     获取字体系统更新时间
  Input:           none
                   
  Return:           NONE
**********************************************************************/

int divoom_system_font_get_update_time(void);


/*********************************************************************
  Function:        divoom_system_font_get_unicode_font_info
  Description:     得到播放的所有字体信息
  Input:           pfont_data
                   
  Return:           NONE
**********************************************************************/

void divoom_system_font_get_unicode_font_info(Pdivoom_http_download_image_font_data pfont_data);

/*********************************************************************
  Function:        divoom_system_font_open_font_handle
  Description:     打开字体文件
  Input:           font_id
                   
  Return:           handle
**********************************************************************/

void *divoom_system_font_open_font_handle(int font_id);

/*********************************************************************
  Function:        divoom_system_font_close_font_handle
  Description:     关闭文件句柄
  Input:           handle
                   
  Return:           none
**********************************************************************/

void divoom_system_font_close_font_handle(void *handle);


/*********************************************************************
  Function:        divoom_system_font_read_font_handle
  Description:     读某个字体文件
  Input:           handle
                   pfont_item
                   
  Return:           handle
**********************************************************************/

void divoom_system_font_read_font_handle(void *handle, Pdivoom_http_download_image_font_item pfont_item);


/*********************************************************************
  Function:        divoom_system_font_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_system_font_get_url_addr(int model_id, int image_id, char *url_addr);

/*********************************************************************
  Function:        divoom_system_font_get_font_info
  Description:     得到字体相关信息
  Input:           font_id
                   font_type
                   file_name
  Return:           NONE
**********************************************************************/

void divoom_system_font_get_font_info(int font_id, int *font_type, char *file_name);


/*********************************************************************
  Function:        divoom_system_font_check_font_exist
  Description:     检查字体是否存在，不存在的话，将启动下载
  Input:           font_id
  Return:           NONE
**********************************************************************/

int divoom_system_font_check_font_exist(int font_id);

/* 表盘进入前等待字体时使用；下载成功或失败均携带 owner_id 回传。 */
int divoom_system_font_check_font_exist_with_result(int font_id, int32 owner_id,
    DIVOOM_MANAGE_IMAGE_DOWNLOAD_RESULT_CALLBACK *presult_callback);

/*********************************************************************
  Function:        divoom_system_font_check_font_file
  Description:     检查字体文件是否存在
  Input:           font_id
  Return:           NONE
**********************************************************************/

int divoom_system_font_check_font_file(int font_id);
/* 仅确认 TTF/OTF 表不完整时返回负数；完整或无法确认均返回 0，调用方保留文件。 */
int divoom_system_font_check_ttf_tables_completeness(const char *font_path);


typedef struct _divoom_system_font_image_font_item
{
    int width;      //宽度
    int high;       //高度
    char  disp_char;    //显示的字符
    uint8 *image_buf;   //显示缓冲区
}divoom_system_font_image_font_item, *Pdivoom_system_font_image_font_item;
typedef struct _divoom_system_font_image_font_info
{
    int font_num;   //字体个数
    Pdivoom_system_font_image_font_item pfont_list[100];
}divoom_system_font_image_font_info, *Pdivoom_system_font_image_font_info;

/*********************************************************************
  Function:        divoom_system_font_free_imaga_font_buffer
  Description:     释放图像字体相关信息
  Input:           pfont_image_info    字体返回信息
  Return:          nONE
**********************************************************************/

void divoom_system_font_free_imaga_font_buffer(Pdivoom_system_font_image_font_info pfont_image_info);

/*********************************************************************
  Function:        divoom_system_font_get_imaga_font_buffer
  Description:     得到图像字体相关信息
  Input:           font_id
                   disp_str   显示字串
                   pfont_image_info    字体返回信息
  Return:          加载到字体的数目
**********************************************************************/

int divoom_system_font_get_imaga_font_buffer(Pdivoom_clock_id_info clock_info, int font_id, char *disp_str,Pdivoom_system_font_image_font_info pfont_image_info);

/*********************************************************************
  Function:        divoom_system_font_get_imaga_font_item_buffer
  Description:     得到一个图像字体相关信息
  Input:           font_id
                   disp_char   显示字串
                   pfont_info    
  Return:          0 or 1
**********************************************************************/

int divoom_system_font_get_imaga_font_item_buffer(int font_id, char disp_char,Pdivoom_system_font_image_font_item pfont_info);
void divoom_system_font_save_font_name(void *para);

#endif


