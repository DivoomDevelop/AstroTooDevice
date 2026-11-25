#ifndef _divoom_manage_image_h_
#define _divoom_manage_image_h_

enum{
    DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE = 1,    //自定义一
    DIVOOM_MANAGE_IMAGE_USER_DEFINE_OTHER_DIR_SEP=DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE + DIVOOM_LIGHT_APP_USER_DEFINE_MAX_NUM,  //不同目录区分
    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16=12,  //推荐图库
    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_32=13,  //推荐图库
    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_64=14,  //推荐图库
    DIVOOM_MANAGE_IMAGE_CLOCK_DIAL=15,            //表盘图库
    DIVOOM_MANAGE_IMAGE_LOCAL_GIF=16,              //本地ＡＰＰ发送的动画
    DIVOOM_MANAGE_IMAGE_SERVER_GIF=17,            //APP通过服务器转发的动画ＩＤ
    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE=18,    //收藏订阅
    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_OTHER=19,		//不用
    DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_ITEM=20,		//表盘元素图
    DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_FONT_DATA=21,            //表盘字体数据
    DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC=22,            //社交app
    DIVOOM_MANAGE_IMAGE_PowerOnGif=23,				//开机显示
    DIVOOM_MANAGE_IMAGE_LIGHT_MUSIC_MODE=24,		//音乐律动
    DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE=25,			//内置图片模块
    DIVOOM_MANAGE_IMAGE_ALARM_GIF=26,				//闹钟动画
    DIVOOM_MANAGE_IMAGE_TIME_PLAN_GIF=27,		    //时间计划动画
    DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF=28,		    //本地使用的一些网络文件
    DIVOOM_MANAGE_IMAGE_USER_DEFINE_CLOCK_PIC=29,		    //用户自定义表盘基础图
    DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC=30,		    //专辑订阅
    DIVOOM_MANAGE_IMAGE_SET_ICON_MODE=31,		    //组件内部  使用的图
	DIVOOM_MANAGE_IMAGE_COMPONENT_ICON_PIC=32, //组件UI图标
    DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_REVIEW_PIC=33,		//表盘预览图
	DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE=34, //番茄时钟动画
	DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE=35,			//留言文件
	DIVOOM_MANAGE_IMAGE_USER_HEAD_IMAGE_FILE=36,			//用户头像数据
	DIVOOM_MANAGE_IMAGE_EXPORT_URL_IMAGE_FILE = 37, //外部url获取的图片
    DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE = 38,    //自定义音频文件
    DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_REVIEW2_PIC=39,		//纪念日弹窗预览图
    DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC=40,		    //用户播放列表
    DIVOOM_MANAGE_IMAGE_SLEEP_AID_PIC=41,		    //睡眠音频
    DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD=42,		    //睡眠录音
    DIVOOM_MANAGE_IMAGE_ALARM_RECORD=43,		    //闹钟录音
    DIVOOM_MANAGE_IMAGE_CLOCK_CLASSIFY_IMG=44,		    //表盘应用市场
	DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG=45,		//AI图鉴卡图片


    
    DIVOOM_MANAGE_IMAGE_END_ID,		    //所有的ID必须在这前面添加
    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_EXPERT_START = 10000,		//达人订阅超始值 ，用达人的ＩＤ

};

#define DIVOOM_MANAGE_IMAGE_ID_START_ID 1
//所有的动画的ＩＤ的起始位置，所有模块的ＩＤ，必须加上这个偏移
enum{
    DIVOOM_MANAGE_IMAGE_ID_BASE_DIVOOM_GIF = DIVOOM_MANAGE_IMAGE_ID_START_ID,  //HOT或达人订阅,基础图库
    DIVOOM_MANAGE_IMAGE_ID_BASE_POWER_ON = DIVOOM_MANAGE_IMAGE_ID_START_ID,    //开机动画
    DIVOOM_MANAGE_IMAGE_ID_BASE_SYSTEM_DEFAULT_FONT = DIVOOM_MANAGE_IMAGE_ID_START_ID,    //系统默认动画字体
    DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC = DIVOOM_MANAGE_IMAGE_ID_START_ID,     //本地使用的一些网络文件
    DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF = DIVOOM_MANAGE_IMAGE_ID_START_ID,     //内置图片起始
    DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_END_GIF =   DIVOOM_MANAGE_IMAGE_ID_START_ID,     //内置图片结束
    DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_PLAN_PIC =    DIVOOM_MANAGE_IMAGE_ID_START_ID,     //用户选择的动画
    DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_FONT =    DIVOOM_MANAGE_IMAGE_ID_START_ID,     //字体文件
    DIVOOM_MANAGE_IMAGE_ID_BASE_USER_DEFINE_CLOCK_PIC =    DIVOOM_MANAGE_IMAGE_ID_START_ID,     //用户自定义表盘基础图
    DIVOOM_MANAGE_IMAGE_ID_BASE_USER_DEFINE = 0,   //自定义动画
    DIVOOM_MANAGE_IMAGE_ID_BASE_MUSIC_GIF = DIVOOM_MANAGE_IMAGE_ID_START_ID,     //音乐ＥＱ动画
    DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_PIC =     DIVOOM_MANAGE_IMAGE_ID_START_ID,     //表盘动画
    DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_REVIEW_PIC =     DIVOOM_MANAGE_IMAGE_ID_START_ID,     //表盘动画
    DIVOOM_MANAGE_IMAGE_ID_BASE_APP_PIC =           DIVOOM_MANAGE_IMAGE_ID_START_ID,            //应用图标，只需要几个社交app
    DIVOOM_MANAGE_IMAGE_ID_BASE_ALARM_GIF =         DIVOOM_MANAGE_IMAGE_ID_START_ID,   //闹钟与纪念日动画
    DIVOOM_MANAGE_IMAGE_ID_BASE_DIALY_GIF =         DIVOOM_MANAGE_IMAGE_ID_START_ID,   //闹钟与纪念日动画
    DIVOOM_MANAGE_IMAGE_ID_BASE_CLOCK_USER_PIC =    DIVOOM_MANAGE_IMAGE_ID_START_ID,     //用户选择的动画
	DIVOOM_MANAGE_IMAGE_ID_BASE_COMPONENT_ICON_PIC = DIVOOM_MANAGE_IMAGE_ID_START_ID, //组件UI图标
	DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF = DIVOOM_MANAGE_IMAGE_ID_START_ID, //番茄时钟图标
    DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_REVIEW2_PIC =     DIVOOM_MANAGE_IMAGE_ID_START_ID,     //表盘动画
};

#define DIVOOM_MANAGE_FILE_ADDR_LEN	128//56

typedef void DIVOOM_MANAGE_IMAGE_DOWNLOAD_OK_CALLBACK(int ret);    
/* 带资源身份的下载结果回调；owner_id 由调用模块定义，ret 为 1 成功、0 失败。 */
typedef void DIVOOM_MANAGE_IMAGE_DOWNLOAD_RESULT_CALLBACK(int32 owner_id, uint32 model_id,
    uint32 image_id, int32 ret);
typedef struct _divoom_manage_image_item_info
{
    int    flag;         //使用标记，０：未使用，１：使用 2:本地图片
    uint32 image_id;
    uint32 utc_time;
    char file_addr[DIVOOM_MANAGE_FILE_ADDR_LEN];
    DIVOOM_MANAGE_IMAGE_DOWNLOAD_OK_CALLBACK *pcallback;
}divoom_manage_image_item_info, *Pdivoom_manage_image_item_info;

 
typedef struct _divoom_manage_image_disp_info
{
    int black_flag; //是否是黑屏标记
    int image_id;
    int author_load_flag;
    int author_update_time;
    int author_id;
    char author_name[200];
    char image_name[200];
    char voice_id[64];
}divoom_manage_image_disp_info, *Pdivoom_manage_image_disp_info;



/*********************************************************************
  Function:        divoom_manage_image_init
  Description:     初始化图片管理模块
  Input:           
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_init(void);

/*********************************************************************
  Function:        divoom_manage_image_up_sdstatus
  Description:     更新ＳＤ卡状态,将释放所有资源
  Input:           sd_status  SD卡的状态 
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_up_sdstatus(uint8 sd_status);


/*********************************************************************
  Function:        divoom_manage_image_up_netstatus
  Description:     更新网络状态
  Input:           net_status  网络的状态 
                    
  Return:          返回以前的网络状态
**********************************************************************/

uint8 divoom_manage_image_up_netstatus(uint8 net_status);


/*********************************************************************
  Function:        divoom_manage_image_add_model_path
  Description:     图片管理模块添加ＳＤ卡路径，方便管理保存ＳＤ卡列表
  Input:           model_id
                   file_path        在ＳＤ卡的路径
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_add_model_path(uint32 model_id, char *file_path);

/*********************************************************************
  Function:        divoom_manage_image_add
  Description:     图片管理模块添加一些新图片资源
  Input:           model_id 
                   pimage_list
                   image_num
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_add(uint32 model_id, Pdivoom_manage_image_item_info pimage_list, uint32 image_num);
void divoom_manage_image_transfer_batch_begin(void);
void divoom_manage_image_transfer_batch_end(void);
void divoom_manage_image_add_transfer(uint32 model_id,
    Pdivoom_manage_image_item_info pimage_list, uint32 image_num);

/* 表盘等需要等待结果的业务使用：失败任务不重试，回调后从下载队列收敛退出。 */
void divoom_manage_image_add_with_result(uint32 model_id,
    Pdivoom_manage_image_item_info pimage_list, uint32 image_num, int32 owner_id,
    DIVOOM_MANAGE_IMAGE_DOWNLOAD_RESULT_CALLBACK *presult_callback);
void divoom_manage_image_add_font_with_result(uint32 model_id,
    Pdivoom_manage_image_item_info pimage_list, uint32 image_num, int32 owner_id,
    DIVOOM_MANAGE_IMAGE_DOWNLOAD_RESULT_CALLBACK *presult_callback);



/*********************************************************************
  Function:        divoom_manage_image_del
  Description:     删除一部分图片资源
  Input:           img_id
                   image_num
                   	update_sd_list_flag 	0更新 1不更新 考虑sd卡列表文件增多,动画切换一次写一次sd卡则太频繁
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_del(uint32 model_id, uint32 *img_id, uint32 image_num, uint8 update_sd_list_flag);


/*********************************************************************
  Function:        divoom_manage_image_cancle_del
  Description:     取消删除
  Input:           img_id
                   model_id
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_cancle_del(uint32 model_id, uint32 img_id);

/*********************************************************************
  Function:        divoom_manage_image_remove
  Description:     删除模块所有图片资源
  Input:           model_id
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_remove(uint32 model_id, int save_file);



/*********************************************************************
  Function:        divoom_manage_image_check_exist
  Description:     判断这个图片是否存在
  Input:           model_id
                   img_id
                    
  Return:          已经下载的图片ＩＤ
**********************************************************************/

uint32 divoom_manage_image_check_exist(uint32 model_id, uint32 img_id);
/*********************************************************************
  Function:        divoom_manage_image_check_reource_download
  Description:     检查资源文件是否下载完成
  Input:           model_id
                   img_id
                   url_addr
                    
  Return:          已经下载的图片ＩＤ
**********************************************************************/

int divoom_manage_image_check_reource_download(uint32 model_id, uint32 img_id, char *url_addr);

/*********************************************************************
  Function:        divoom_manage_image_remove
  Description:     获取图片的数据,没有加载进来的图片，将读url_addr地址，否则返回失败
  Input:           model_id
                   img_id
                   url_addr
                   pcallback  是否提前结束
                    
  Return:          图片数据
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_manage_image_read(uint32 model_id, uint32 img_id, char *url_addr, DIVOOM_HTTP_DOWNLOAD_STOP_READ_CALLBACK pcallback);
/*********************************************************************
  Function:        divoom_manage_image_redownload_local_file
  Description:     数据损坏，重新下载
  Input:           file_name  本地文件名
                   img_id
                    
  Return:          NONE
**********************************************************************/

void divoom_manage_image_redownload_local_file(char *file_name, int download_flag);

/*********************************************************************
  Function:        divoom_manage_image_get_image_info_from_name
  Description:     从文件名得到模块与图像    id
  Input:           file_name  本地文件名
                   img_id
                   model_id
                    
  Return:          NONE
**********************************************************************/

void divoom_manage_image_get_image_info_from_name(char *file_name, int *model_id, int *image_id);

/*********************************************************************
  Function:        divoom_manage_image_check_local_file_and_download
  Description:     检查本地文件是否存在，不存在将直接下载
  Input:           file_name  本地文件名
                    
  Return:          NONE
**********************************************************************/

void divoom_manage_image_check_local_file_and_download(char *file_name);

/*********************************************************************
  Function:        divoom_manage_image_confirm_download
  Description:     检查数据是否下载，如果没有下载，将直接下载文件
  Input:           model_id
                   img_id
                    
  Return:          NONE
**********************************************************************/

void divoom_manage_image_confirm_download(uint32 model_id, uint32 img_id, char *file_name);
/*********************************************************************
  Function:        divoom_manage_image_add_font
  Description:     图片管理模块添加一些非图片资源，只提供下载管理
  Input:           model_id 
                   pimage_list
                   image_num
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_add_font(uint32 model_id, Pdivoom_manage_image_item_info pimage_list, uint32 image_num);
/* Queue a failed watchface element; preserve any queued/in-flight owner callback. */
void divoom_manage_image_queue_clock_part_repair(uint32 img_id, char *url_addr, int file_type);

/*********************************************************************
  Function:        divoom_manage_image_add_local_font
  Description:     增加本地字体下载管理
  Input:           model_id 
                   pimage_list
                   image_num
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_add_local_font(uint32 model_id, Pdivoom_manage_image_item_info pimage_list, uint32 image_num);


/*********************************************************************
  Function:        divoom_manage_image_add_url
  Description:     从外部url下载图片
  Input:           model_id 
                   pimage_list
                   image_num
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_add_url(uint32 model_id, Pdivoom_manage_image_item_info pimage_list, uint32 image_num);


/*********************************************************************
  Function:        divoom_manage_image_format_gif_table
  Description:     将图片缓冲区数据转化为ＲＧＢ数据
  Input:           pcur_disp_image_handle
                   rgb_buf
                    
  Return:          图片数据
**********************************************************************/

void divoom_manage_image_format_gif_table(Pdivoom_http_download_image_file_info pcur_disp_image_handle, uint8 *rgb_buf);

/*********************************************************************
  Function:        divoom_manage_image_save_file_list
  Description:     将图像的文件列表保存
  Input:           NONE
                   
                    
  Return:          NONE
**********************************************************************/

void divoom_manage_image_save_file_list(void);
void divoom_manage_image_download_exit(void);


/*********************************************************************
  Function:        divoom_manage_image_del_direct
  Description:     马上删除一部分图片资源
  Input:           img_id
                   image_num
                   	
                    
  Return:           none
**********************************************************************/

void divoom_manage_image_del_direct(uint32 model_id, uint32 *img_id, uint32 image_num);
/* 表盘回收专用：移除排队下载并确认文件删除；忙或 I/O 失败返回 0，调用方须保留重试记录。 */
int divoom_manage_image_del_clock_resource(uint32 model_id, uint32 image_id);

/*********************************************************************
  Function:        divoom_manage_image_download_get_file_name
  Description:     通过模块ID获取文件名
  Input:

  Return:          none
**********************************************************************/

void divoom_manage_image_download_get_file_name(uint32 model_id, uint32 img_id, char *file_name);
/*********************************************************************
  Function:        divoom_manage_image_download_get_path_name
  Description:     通过模块ID获取目录名
  Input:

  Return:          none
**********************************************************************/

void divoom_manage_image_download_get_path_name(uint32 model_id, uint32 img_id, char *file_name);
/*********************************************************************
  Function:        divoom_manage_image_decode_rgb_buf
  Description:     对DIVOOM的图像数据进行解码
  Input:           raw_buf
                   raw_size
                   image_width
                   image_type
                   rgb_buf
                    
  Return:          图片数据
**********************************************************************/

void divoom_manage_image_decode_rgb_buf(uint8 *raw_buf, int raw_size, int image_width, int image_type, uint8 *rgb_buf);
void divoom_manage_image_decode_rgb565_buf(uint8 *raw_buf, int raw_size, int image_width, int image_type, uint8 *rgb565_buf);

/* 返回等待下载和正在下载的总项数，供 HOT 本地批次控制低水位。 */
uint32 divoom_manage_image_get_pending_num(void);

/* 返回当前订阅下载总批次；完成后由文件传输页面确认清除。 */
int divoom_manage_image_get_transfer_progress(uint32 *finished, uint32 *total);
int divoom_manage_image_ack_transfer_progress(void);

#endif

