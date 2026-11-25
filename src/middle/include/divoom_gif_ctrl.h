#ifndef _divoom_gif_ctrl_
#define _divoom_gif_ctrl_

typedef struct _divoom_need_local_info
{
	uint32 device_id;
	uint32 user_id;
	uint32 packet_id;
	uint32 next_time; //利用时间来控制
}divoom_need_local_info, *Pdivoom_need_local_info;

typedef void (*DIVOOM_GIF_CTRL_NOTICE_CALLBACK)(uint8 status);


/*
  Function:       divoom_gif_ctrl_set_callback
  Description:	  设置callback，当该动画准备好了之后通知light_app
  Input:          NONE
  Return:         cb 回调函数
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_gif_ctrl_set_callback(DIVOOM_MANAGE_IMAGE_DOWNLOAD_OK_CALLBACK *cb);


/*
  Function:       divoom_gif_ctrl_get_image_id
  Description:	  获取当前要显示的动画id
  Input:          NONE
  Return:         image id
  Author:         wangft
  Data:           2020-12-18
*/
int divoom_gif_ctrl_get_image_id(void);

/*
  Function:       divoom_gif_ctrl_get_image_handle
  Description:	  获取当前要显示的本地动画句柄
  Input:          NONE
  Return:         句柄
  Author:         wangft
  Data:           2020-12-18
*/
Pdivoom_http_download_image_file_info divoom_gif_ctrl_get_image_handle(uint8 first_flag, Pdivoom_http_download_image_file_info pcur_image);

/*
  Function:       divoom_gif_ctrl_update_local_image
  Description:	  把本地动画加载到image管理模块
  Input:          image_info 本地动画信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
int divoom_gif_ctrl_update_local_image(Pdivoom_draw_local_json_image_info image_info);

/*
  Function:       divoom_gif_ctrl_update_remote_image
  Description:	  把服务器上的动画加载到image管理模块
  Input:          image_info 服务器上动画信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_gif_ctrl_update_remote_image(Pdivoom_draw_remote_json_image_info image_info);


/*
  Function:       divoom_gif_ctrl_update_remote_by_app_image
  Description:	  通过app从服务器上获取图片数据
  Input:          image_info 服务器上动画信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/

int divoom_gif_ctrl_update_remote_by_app_image(Pdivoom_draw_remote_by_app_json_image_info image_info);

/*
  Function:       divoom_gif_ctrl_init
  Description:	  对要显示的gif进行管理
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_gif_ctrl_init(void);


/*
  Function:       divoom_gif_ctrl_clear_cache
  Description:	  清除ＧＩＦ的缓冲
  Input:          NONE
  Return:         NONE
*/
void divoom_gif_ctrl_clear_cache(void);
void divoom_gif_ctrl_clear_local_image(void);
/*
  Function:       divoom_gif_ctrl_update_http_image
  Description:	  把HTTP动画加载到image管理模块
  Input:          image_info 本地动画信息
  Return:         0下载未完成 1表示下载完成
*/
int divoom_gif_ctrl_update_http_image(Pdivoom_drawing_json_http_gif_info image_info);

/*
  Function:       divoom_gif_ctrl_reset_http_gif_id
  Description:	  把HTTP gif id 重置
  Input:          
  Return:         
*/
void divoom_gif_ctrl_reset_http_gif_id(void);

/*
  Function:       divoom_gif_ctrl_request_other_pacet
  Description:	  图像数据包不完整，将请求其它的包
  Input:          NONE
  Return:         NONE
*/

int divoom_gif_ctrl_request_other_pacet(void);

/*
  Function:       divoom_gif_ctrl_get_http_image_pic_id
  Description:	  得到HTTP动画可以使用的ＰＩＣ　ＩＤ
  Input:          image_info 本地动画信息
  Return:         0下载未完成 1表示下载完成
*/
int divoom_gif_ctrl_get_http_image_pic_id(void);


/*
  Function:        divoom_gif_ctrl_set_image_id
  Description:     设置图像ID
  Input:            
                    
  Return:           none
*/
void divoom_gif_ctrl_set_image_id(int image_id);


void *divoom_gif_ctrl_wite_task(void *param);

#endif
