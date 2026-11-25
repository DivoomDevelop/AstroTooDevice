#ifndef _divoom_http_download_image_h_
#define _divoom_http_download_image_h_

typedef int DIVOOM_HTTP_DOWNLOAD_STOP_READ_CALLBACK(void);
typedef struct _divoom_http_download_image_file_item
{
    uint8  image_type;          //0:为rgb, 1:minilzo压缩, 2:divoom压缩
    uint16 image_size;         //image_data缓冲区大小
    uint8 *image_data;      //缓冲区数据
}divoom_http_download_image_file_item, *Pdivoom_http_download_image_file_item;


typedef struct _divoom_http_download_image_font_item
{
    uint16 unicode16;       //当前文字的ＵＩＮＣＯＤＥ
    uint8  font_info[32];   //字体的16*16的点阵信息    
}divoom_http_download_image_font_item, *Pdivoom_http_download_image_font_item;

typedef struct _divoom_http_download_image_font_data
{
    int size;                     //字体大小
    int font;                     //字体类型
    int speed;                    //文字速度  
    int textEffect;               //文字效果
    int picEffect;                //图片效果
    int start_x;                  //框的起始x
    int start_y;                  //框的起始y
    int frameWidth;               //框的高度
    int frameHeight;              //框的高度
    uint8 color[3];               //字体颜色
    int    unicode_len;           //显示字体长度
    uint16 unicode_str[128];      //显示的字体unicode
}divoom_http_download_image_font_data, *Pdivoom_http_download_image_font_data;

typedef struct _divoom_server_image_file_info
{
    int     num;        //图像张数
    int     width;      //图像宽度
    int     image_type; //    DIVOOM_IMAGE_BUF_TYPE_MODE
    int     speed;      //图像速度
    int     font_flag;  //是否有文字信息
    int     cache_szie;  //缓冲区大小
    int     image_id;   //图像ＩＤ
    int 	sd_file_flag; //1是sd卡上获取 0 本地内存上获取
    int     user_id;
    int     play_voice;
    int     voice_flag;
    char    image_name[256];
   	void	*gif_handle; //gif句柄
   	void    *zstd_handle;
    Pdivoom_http_download_image_font_data pfont_data;      //字体数据
    Pdivoom_http_download_image_file_item *pimage_table;    //图像列表
}divoom_http_download_image_file_info, *Pdivoom_http_download_image_file_info;

typedef struct _divoom_http_download_image_raw_data_info
{
    int offset;             //当前读的缓冲区
    int file_size;         //文件大小
    void *divoom_queue;    //缓冲区队列
}divoom_http_download_image_raw_data_info, *Pdivoom_http_download_image_raw_data_info;

/*********************************************************************
  Function:        divoom_http_download_image_free_handle
  Description:     释放原始数据的缓冲区
  Input:            pdivoom_file
  Return:           NONE
**********************************************************************/

void divoom_http_download_image_free_handle(Pdivoom_http_download_image_raw_data_info pdivoom_file);
/*********************************************************************
  Function:        divoom_http_download_image_copy_part_image_handle
  Description:     拷贝部分图像缓冲区
  Input:            pdivoom_image_handle
  Return:           NONE
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_http_download_image_copy_part_image_handle(Pdivoom_http_download_image_file_info pdivoom_image_handle, int start, int cnt);

/*********************************************************************
  Function:        divoom_http_download_image_copy_part_list_image_handle
  Description:     拷贝部分图像缓冲区
  Input:            pdivoom_image_handle
  Return:           NONE
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_http_download_image_copy_part_list_image_handle(Pdivoom_http_download_image_file_info pdivoom_image_handle, int *start, int cnt, int list_num);


/*********************************************************************
  Function:        divoom_http_download_image_get_raw_data
  Description:     根据网址，将文件的原始数据获取，分段获取并输出到队列中
  Input:            file_addr
  Return:           NONE
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_http_download_image_get_raw_data(char *file_addr);

/*********************************************************************
  Function:        divoom_http_download_image_driect_to_file
  Description:     根据网址，  直接下载文件
  Input:            model_id
                    image_id
                    file_addr
  Return:           1：成功，0失败
**********************************************************************/

int divoom_http_download_image_driect_to_file(int model_id, int image_id, char *file_addr);

/*********************************************************************
  Function:        divoom_http_download_normal_driect_to_file
  Description:     根据网址，  直接下载普通文件
  Input:            model_id
                    image_id
                    file_addr
  Return:           1：成功，0失败
**********************************************************************/

int divoom_http_download_normal_driect_to_file(int model_id, int image_id, char *file_addr);

/*********************************************************************
  Function:        divoom_http_download_image_out_image
  Description:     将原始数据的缓冲区转换为上层使用的缓冲区
  Input:            pdivoom_file
  Return:           NONE
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_http_download_image_out_image(Pdivoom_http_download_image_raw_data_info pdivoom_file);

/*********************************************************************
  Function:        divoom_http_download_image_download_image
  Description:     根据网址，下载文件并输出相关信息
  Input:            file_addr
  Return:           NONE
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_http_download_image_download_image(char *file_addr);



/*********************************************************************
  Function:        divoom_http_download_image_download_image
  Description:     根据SD地址，下载文件并输出相关信息
  Input:            file_addr
  Return:           NONE
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_http_download_image_get_sd_image(char *file_addr, DIVOOM_HTTP_DOWNLOAD_STOP_READ_CALLBACK pcallback);

/*********************************************************************
  Function:        divoom_http_download_image_minilzo_decode_rgb
  Description:     将ＭＩＮＩＬＺＯ数据解压成ＲＧＢ
  Input:            minilzo_buf
                    minilzo_len    
                    rgb_buf     RGB
                    width     图像的宽度
  Return:           NONE
**********************************************************************/


void divoom_http_download_image_minilzo_decode_rgb(uint8 *minilzo_buf, int minilzo_len, uint8 *rgb_buf, int width);


/*********************************************************************
  Function:        divoom_http_download_image_free_image_handle
  Description:     释放图像缓冲区
  Input:            pdivoom_file
  Return:           NONE
**********************************************************************/

void divoom_http_download_image_free_image_handle(Pdivoom_http_download_image_file_info pdivoom_image_handle);

/*********************************************************************
  Function:        divoom_http_download_image_copy_image_handle
  Description:     拷贝图像缓冲区
  Input:            pdivoom_image_handle
  Return:           NONE
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_http_download_image_copy_image_handle(Pdivoom_http_download_image_file_info pdivoom_image_handle);

/*********************************************************************
  Function:        divoom_http_download_image_aes_128_cbc_decrypt
  Description:     对数据进行ＡＥＳ分断解码
  Input:            ctx
                    cbc
                    data
                    data_len
  Return:           0
**********************************************************************/


int divoom_http_download_image_aes_128_cbc_decrypt(void *ctx, uint8 *cbc, uint8 *data, int data_len);
/*********************************************************************
  Function:        divoom_http_download_image_get_raw_data
  Description:     根据路径，将文件的原始数据获取，分段获取并输出到队列中
  Input:            file_path
  Return:           NONE
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_http_download_image_get_sd_image_data(char *file_path, DIVOOM_HTTP_DOWNLOAD_STOP_READ_CALLBACK pcallback, int *file_err);


/*********************************************************************
  Function:        divoom_http_download_image_get_image_type
  Description:     得到文件的图像类型
  Input:            file_path
  Return:           图像类型
**********************************************************************/
int divoom_http_download_image_get_image_type(char *file_path);

/*********************************************************************
  Function:        divoom_http_download_image_get_local_buffer_callback
  Description:     读取数据的回调
  Input:            buf 存放数据缓冲区
                    offset  数据偏移
                    len     数据长度
                    
  Return:           读到的数据长度
**********************************************************************/

typedef int divoom_http_download_image_get_local_buffer_callback(uint8 *buf, int offset, int len);

/*********************************************************************
  Function:        divoom_http_download_image_get_image_aec_flag
  Description:     图像数据是否AES加密
  Input:           image_flag
                    
  Return:           返回true of false
**********************************************************************/

int  divoom_http_download_image_get_image_aec_flag(int image_flag);

/*********************************************************************
  Function:        divoom_http_download_image_get_image_handle_from_func
  Description:     　从回调函数中读取数据，再进行解码
  Input:            pcallback
                    
  Return:           返回图像句柄
**********************************************************************/
Pdivoom_http_download_image_file_info divoom_http_download_image_get_image_handle_from_func(divoom_http_download_image_get_local_buffer_callback *pcallback, int tol_len);
typedef struct _divoom_image_data_file_info
{
    uint8 image_type;
    uint8 image_mode;
    int cur_image_id;
    int image_num;
    int image_width;
    int image_speed;
    int start_off;
    int font_off;
    Pdivoom_http_download_image_font_data pfont_handle;
}divoom_image_data_file_info, *Pdivoom_image_data_file_info;

/*********************************************************************
  Function:        divoom_http_download_image_free_image_data_from_file_handle
  Description:     释放相关句柄
  Input:            pimage_handle
  Return:           NONE
**********************************************************************/

void divoom_http_download_image_free_image_data_from_file_handle(Pdivoom_image_data_file_info pimage_handle);



T_U16  divoom_http_download_image_unpack_uint16(T_U8 *data);

T_U32  divoom_http_download_image_unpack_uint32(T_U8 *data);

#endif
