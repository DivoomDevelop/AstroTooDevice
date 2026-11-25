#ifndef _divoom_disp_ctrl_h_
#define _divoom_disp_ctrl_h_
#include "divoom_type_tivoo.h"
#include "divoom_sys_data.h"
#include "divoom_leave_msg.h"
#include "divoom_image_decode.h"

typedef enum
{
    DIVOOM_SYSTEM_LINEX,
    DIVOOM_SYSTEM_RTOS,
    DIVOOM_SYSTEM_END,
}DIVOOM_SYSTEM_FLAG;


typedef struct _divoom_screen_color_info
{
    uint8 r_c;
    uint8 g_c;
    uint8 b_c;
}divoom_screen_color_info, *Pdivoom_screen_color_info, divoom_light_color_info, *Pdivoom_light_color_info, *Pdivoom_disp_color_info, divoom_disp_color_info;


typedef struct _divoom_disp_hsv_info
{
	int32 h; 
	uint8 s;
	uint8 v;
}divoom_disp_hsv_info, *Pdivoom_disp_hsv_info;




typedef void (*DIVOOM_LIGHT_PRINT)(char *format, ...);
typedef void * (*DIVOOM_LIGHT_MALLOC)(uint32 size, uint32 model_line, const char *file_name);
typedef void (*DIVOOM_LIGHT_FREE)(void *ptr, const char *file_name, int line);

//read need
typedef void *(*DIVOOM_LIGHT_OPEN_USER_DATA)(const char *file_name, int cache_flag);
typedef int  (*DIVOOM_LIGHT_RETUEN_ALL_LEN)(void *handle);
typedef int (*DIVOOM_LIGHT_READ_USER_DATA)(void *handle, void *buf, int size, int off);
typedef int (*DIVOOM_LIGHT_WRITE_USER_DATA)(void *handle, void *buf, int size, int off);
typedef void (*DIVOOM_LIGHT_CLOSE_USER_DATA)(void *handle);


//告知screen什么时候flag = 1开始是通知图案,flag = 0结束通知图案
typedef void (*DIVOOM_LIGHT_NOTICE_TO_SCREEN)(int flag);

typedef int (*DIVOOM_LIGHT_READ_CONFIG)(uint8 mode, void *buf, int size);
typedef int (*DIVOOM_LIGHT_WRITE_CONFIG)(uint8 mode, void *buf, int size);

//简单灯效写数据
typedef uint32 (*DIVOOM_LIGHT_SIMPLE_WRITE_DATA)(void *buf, int size);

//通知消息写数据
typedef uint32 (*DIVOOM_LIGHT_NOTICE_WRITE_DATA)(void *buf, int size);

//写命令
typedef uint8 (*DIVOOM_LIGHT_WRITE_CMD)(uint16 cmd, void *data, int size);

//写灯板数据
typedef uint32 (*DIVOOM_LIGHT_WRITE_DATA)(void *buf, int size);


//task
typedef void DIVOOM_LIGHT_CALLBACK(void *data);
typedef uint8 (*DIVOOM_LIGHT_TASK_CREATE)(DIVOOM_LIGHT_CALLBACK *pcallback, uint32 time);
typedef void (*DIVOOM_LIGHT_TASK_DESTROY)(uint8 task_id);
typedef void (*DIVOOM_LIGHT_TASK_PAUSE)(uint8 task_id);
typedef void (*DIVOOM_LIGHT_TASK_RESUME)(uint8 task_id);
typedef void (*DIVOOM_LIGHT_TASK_UPDATE)(uint8 task_id, uint32 time);

typedef int (*DIVOOM_LIGHT_GET_ENV_INFO)(uint8 mode, void *env_data);

typedef uint32 (*DIVOOM_LIGHT_GET_TOOL_INFO)(uint8 mode, void *tool_info);

typedef int (*DIVOOM_LIGHT_GET_RAND)(int max);

typedef int (*DIVOOM_LIGHT_GET_VOL)(void);

typedef void (*T_Voice_PalyEndCallBack)(void);
typedef void (*DIVOOM_LIGHT_PLAY_VOICE)(uint16 voice_tip, uint16 time, uint8 vol, T_Voice_PalyEndCallBack callback);

//light mid
typedef void *(*DIVOOM_LIGHT_MID_INIT)(void);
typedef void (*DIVOOM_LIGHT_MID_EXIT)(void *handle);

typedef void *(*DIVOOM_LIGHT_START_READ)(const char *file_name, uint32 *start_page);
typedef void (*DIVOOM_LIGHT_END_READ)(void *handle);

//外面给到diso_ctrl_app模块的函数
typedef void *(*DIVOOM_LIGHT_READ_INIT)(const char *file_name, uint32 *start_page);
typedef void (*DIVOOM_LIGHT_READ_FUNC)(void *handle);
typedef void (*DIVOOM_LIGHT_READ_EXIT)(void *handle);

typedef struct _divoom_disp_ctrl_init_cb_
{
	/*DIVOOM_LIGHT_READ_INIT read_init;
	DIVOOM_LIGHT_READ_FUNC read_func;
	DIVOOM_LIGHT_READ_EXIT read_exit;*/

	DIVOOM_LIGHT_NOTICE_TO_SCREEN notice_to_screen;

	/*DIVOOM_LIGHT_MID_INIT		light_mid_init;
	DIVOOM_LIGHT_MID_EXIT		light_mid_exit;*/
}divoom_disp_ctrl_init_cb, *Pdivoom_disp_ctrl_init_cb;

typedef struct _divoom_disp_ctrl_callback
{
    DIVOOM_LIGHT_PRINT  print;
    DIVOOM_LIGHT_MALLOC malloc;
    DIVOOM_LIGHT_FREE   free;


    DIVOOM_LIGHT_TASK_CREATE    create;
    DIVOOM_LIGHT_TASK_DESTROY   destroy;
    DIVOOM_LIGHT_TASK_PAUSE     pause;
    DIVOOM_LIGHT_TASK_RESUME    resume;
    DIVOOM_LIGHT_TASK_UPDATE    update;



	DIVOOM_LIGHT_GET_TOOL_INFO	get_tool_iofo;
    DIVOOM_LIGHT_GET_ENV_INFO   get_env_info;

    DIVOOM_LIGHT_GET_RAND       get_rand;
    DIVOOM_LIGHT_GET_VOL        get_vol;
}divoom_disp_ctrl_callback, *Pdivoom_disp_ctrl_callback;

//释放发送灯效数据的函数
//typedef (*DIVOOM_DISP_CTRL_APP_FREE_LIGHT_DATA)(uint8 light_model, uint8 *data);



typedef int DIVOOM_DISP_CTRL_OVER_CALL_END(uint8 mode);


enum
{
    DIVOOM_DISP_CLOCK_MODE= 0x80,
    DIVOOM_DISP_SUBSCRIBE_MODE,      //订阅频道
    DIVOOM_DISP_MUISE_MODE,
    DIVOOM_DISP_USER_DEFINE_MODE, //用户自定义图片模式
    DIVOOM_DISP_NO_DISP_MODE,
    DIVOOM_DISP_ENV_MODE,
    DIVOOM_DISP_LIGHT_MODE,
    DIVOOM_DISP_SPECIAL_MODE,
    DIVOOM_DISP_PHONE_CTRL,
    DIVOOM_DISP_PHONE_GIF_CTRL,
    DIVOOM_DISP_HTTP_GIF_CTRL,
    DIVOOM_DISP_PIC_SCAN_CTRL,
    DIVOOM_DISP_MOVIE_CTRL,
    DIVOOM_DISP_GAME_CTRL,
    DIVOOM_DISP_TALK_CTRL,
    DIVOOM_DISP_POWER_ON,
    DIVOOM_DISP_EVENT_DIS_CTRL,			//提醒模块，和其他灯效共存-->16*16
    DIVOOM_DISP_FM_MODE,
    DIVOOM_DISP_ALARM_MODE,
    DIVOOM_DISP_DIALY_MODE,
    DIVOOM_DISP_SLEEP_MODE,
    DIVOOM_DISP_ANCS_MODE,
    DIVOOM_DISP_PHONE_NUMBER_MODE,
    DIVOOM_DISP_SET_ALARM_MODE,
    DIVOOM_DISP_LEAVE_MSG_MODE,
    DIVOOM_DISP_LEAVE_MSG_PLAY_MODE,
    DIVOOM_DISP_TEST_MODE,
    DIVOOM_DISP_WATCH_MODE,
    DIVOOM_DISP_SCORE_MODE,
    DIVOOM_DISP_TIME_MANAGE_MODE,
    DIVOOM_DISP_TEMP_EVENT_MODE,
    DIVOOM_DISP_MIXER_CTRL, 
    DIVOOM_DISP_MUSICAL_INSTRUMENTS_MODE,
    DIVOOM_DISP_WORD_MODE,
    DIVOOM_DISP_POHONE_NAME,  
    DIVOOM_DISP_NOISE_MODE,  
    DIVOOM_DISP_MUL_SEARCH_MODE,    //多屏搜索的显示
    DIVOOM_DISP_DOWNLOAD_FILE_MODE,    //下载文件
    DIVOOM_DISP_SLEEP_TIME,
	DIVOOM_DISP_COUNT_TIME_DOWN_MODE,    //倒计时
	DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL,    //高等级提醒事件，仅次于闹钟的事件-->32*32
    DIVOOM_DISP_PLAY_NET_IMAGE_CTRL,			//播放某个网络图片
	DIVOOM_DISP_DIS_IFFT_TEXT_MODE,             //显示ＩＦＦＦＴ的文本方式
    DIVOOM_DISP_LOCAL_GIF_MODE,
    DIVOOM_DISP_TURN_DIAL_MODE,
    DIVOOM_DISP_ROLL_CLOSE,



	DIVOOM_DISP_DEVICE_UPDATE_MODE,
	DIVOOM_DISP_WEATHER_MODE,
	DIVOOM_DISP_NET_APP_MODE,
	DIVOOM_DISP_RADIO_MODE,
	DIVOOM_DISP_TIME_MODE,
	DIVOOM_DISP_EXPERT_PIC_MODE,	//显示达人图片
	DIVOOM_DISP_DIVOOM_CHANNEL_MODE,	//显示divoom订阅频道图片
	DIVOOM_DISP_APP_POWER_OFF_MODE,		//关机前的动作
	DIVOOM_DISP_IFFT_BUTTON_MODE,





	DIVOOM_DISP_END_MODE,
};

#define DIVOOM_DISP_INFO_ALL DIVOOM_DISP_END_MODE

//DIVOOM_DISP_DIVOOM_MODE的数据结构
typedef struct _divoom_disp_ctrl_divoom_info
{
    uint32 offset;       //数据的偏移
}divoom_disp_ctrl_divoom_info, *Pdivoom_disp_ctrl_divoom_info;

typedef void DIVOOM_DISP_CTRL_COM_GET_NEW_ID(T_U8 id, Pdivoom_play_gif_image_info image_info);
#define DIVOOM_LIGHT_APP_USER_DEFINE_MAX_NUM 10
#define DIVOOM_LIGHT_APP_SUSCRIBE_MAX_NUM 10
#define DIVOOM_LIGHT_APP_EQ_MAX_NUM 10

//DIVOOM_DISP_USER_DEFINE_MODE的数据结构
typedef struct _divoom_disp_ctrl_user_define_info
{
    int flag;           //１：全部更新显示，0:只更新显示时间
    int play_id;        //是否只播放这个，为０播放全部，否则播放某个
    int index;
    int custom_index;
    int custom;
    int offset[DIVOOM_LIGHT_APP_USER_DEFINE_MAX_NUM];
    DIVOOM_DISP_CTRL_COM_GET_NEW_ID  *pget_pic_id_callback ;//得到下一个启动的图片ＩＤ
}divoom_disp_ctrl_user_define_info, *Pdivoom_disp_ctrl_user_define_info;

//DIVOOM_DISP_LOCAL_GIF_MODE的数据结构
typedef struct _divoom_disp_ctrl_local_gif_info
{
    int flag;           //１：追加，0:删除以前的，
    char *file_name;    //文件名路径
}divoom_disp_ctrl_local_gif_info, *Pdivoom_disp_ctrl_local_gif_info;

typedef struct _divoom_disp_ctrl_pic_info
{
	uint16 pic_len;
	uint8 *pic_data;
	uint8 pic_type;  //1表示16*16 2表示32*32 3表示64*64
}divoom_disp_ctrl_pic_info, *Pdivoom_disp_ctrl_pic_info;

typedef struct _divoom_disp_ctrl_font_info
{
    uint8    font_id;
    uint16   font_off;      //字体数据偏移
    uint32   font_size;    //字体数据大小数据偏移
    void  *font_handle;    //字体保存在文件中的句柄
}divoom_disp_ctrl_font_info, *Pdivoom_disp_ctrl_font_info;


//DIVOOM_DISP_TIME_MANAGE_MODE的数据结构
typedef struct _divoom_disp_ctrl_time_manage_info
{
    uint16 index;
}divoom_disp_ctrl_time_manage_info, *Pdivoom_disp_ctrl_time_manage_info;

//DIVOOM_DISP_TEMP_EVENT_MODE的数据结构
typedef struct _divoom_disp_ctrl_update_net_temp_info
{
    uint8 each_hour_switch;
    uint8 spec_time_flag;
    uint16 spec_time;
}divoom_disp_ctrl_update_net_temp_info, *Pdivoom_disp_ctrl_update_net_temp_info;


//DIVOOM_DISP_MIXER_CTRL的数据结构
typedef struct _divoom_disp_ctrl_mixer_info
{
    uint8 mode;
    uint8 color_index;
    uint8 on_off;
    uint8 app_flag;    //APP控制标志
}divoom_disp_ctrl_mixer_info, *Pdivoom_disp_ctrl_mixer_info;

//DIVOOM_DISP_MUSICAL_INSTRUMENTS_MODE的数据结构
typedef struct _divoom_disp_ctrl_musical_instruments_info
{
    uint8 mode;
    uint8 index;
}divoom_disp_ctrl_musical_instruments_info, *Pdivoom_disp_ctrl_musical_instruments_info;

//DIVOOM_DISP_DOWNLOAD_FILE_MODE的数据结构
typedef struct _divoom_disp_ctrl_download_file_info
{
    uint32 down_num;
    uint32 wait_down_num;
}divoom_disp_ctrl_download_file_info, *Pdivoom_disp_ctrl_download_file_info;

//DIVOOM_DISP_LEAVE_MSG_MODE的数据结构
typedef struct _divoom_disp_ctrl_leave_notice_info
{
    uint8 number;
}divoom_disp_ctrl_leave_notice_info, *Pdivoom_disp_ctrl_leave_notice_info;

//DIVOOM_DISP_SET_ALARM_MODE的数据结构
typedef struct _divoom_disp_ctrl_set_alarm_info
{
    uint8 key_info;
}divoom_disp_ctrl_set_alarm_info, *Pdivoom_disp_ctrl_set_alarm_info;


typedef struct _divoom_disp_ctrl_alarm_info
{
    T_U8 status;    //0:闹钟显示状态，1:贪睡显示状态；２：闹钟过期状态
    char alarm_name[64];
	uint32 image_id;
}divoom_disp_ctrl_alarm_info, *Pdivoom_disp_ctrl_alarm_info;



//DIVOOM_DISP_SLEEP_MODE的数据结构
typedef struct _divoom_disp_sleep_ctrl
{
    uint8 mode;
    uint8 light;
    divoom_screen_color_info color;
}divoom_disp_sleep_ctrl, *Pdivoom_disp_sleep_ctrl;

//DIVOOM_DISP_EVENT_DIS_CTRL的数据结构
#define DIVOOM_DISP_EVENT_MAX_PIC 24

typedef struct _divoom_disp_event_extend_info
{
	uint8 model_id;
	uint8 event_id;
}divoom_disp_event_extend_info, *Pdivoom_disp_event_extend_info;

typedef struct _divoom_disp_event_disp_info
{
    uint8 on_time;       //??????  100ms???λ
    uint8 period_time;   //????             100ms????
    uint8 tot_pic;       //??????
    uint16 tot_time;      //?????      100ms*tot_time
    uint16 disp_id[DIVOOM_DISP_EVENT_MAX_PIC];    //??ID
    uint16 pic_len;
    uint8  *pic_data;
	divoom_disp_event_extend_info extend_info;

    T_U32 noice_id;      //结束通知标记
    char  *disp_title;
    DIVOOM_LIGHT_DISP_OVER_CALLBACK *pover_callback;
}divoom_disp_event_disp_info, *Pdivoom_disp_event_disp_info;

//DIVOOM_DISP_GAME_CTRL的数据结构
typedef struct _divoom_disp_ctrl_game_info
{
    uint8 mode;
	//在game 层,只有当前模式的新模式一样的时候才能相应这个按键,没有按键为0xFF
	uint8 key_id;
}divoom_disp_ctrl_game_info, *Pdivoom_disp_ctrl_game_info;

//DIVOOM_DISP_PHONE_GIF_CTRL的数据结构
typedef struct _divoom_disp_ctrl_talk_info
{
    uint8 mode;
}divoom_disp_ctrl_talk_info, *Pdivoom_disp_ctrl_talk_info;

enum{
    DIVOOM_DISP_CTRL_GIF_FONT_ACTIVE_POS,
    DIVOOM_DISP_CTRL_GIF_FONT_ACTIVE_SIZE,
    DIVOOM_DISP_CTRL_GIF_FONT_ACTIVE_WIDTH,
    DIVOOM_DISP_CTRL_GIF_FONT_ACTIVE_EFFECT,
    DIVOOM_DISP_CTRL_GIF_FONT_ACTIVE_COLOR_INFO,
    DIVOOM_DISP_CTRL_GIF_FONT_ACTIVE_SPEED,
};

#define DIVOOM_DISP_GIF_MAX_WORD_NUM    4
//DIVOOM_DISP_GIF_CTRL的数据结构
typedef struct _divoom_disp_ctrl_gif_info
{
    uint8 pic_type;   //动画大小
	uint8 use_flag;  //修改是否被使用过
	uint8 attr_change_flag;  //0没有改变 1改变过
	divoom_draw_set_info_json_image_info attr_info; //属性
}divoom_disp_ctrl_gif_info, *Pdivoom_disp_ctrl_gif_info;

//DIVOOM_DISP_PHONE_CTRL的数据结构
typedef struct _divoom_disp_ctrl_real_draw_ctrl
{
    divoom_screen_color_info color;
    int num;
    uint16 *off_table;
}divoom_disp_ctrl_real_draw_ctrl, *Pdivoom_disp_ctrl_real_draw_ctrl;

//当前屏幕的大小
#define BUF_SIZE		(64*64*3)
typedef struct _divoom_disp_phone_draw_pic_info
{
	uint8 flag; //区分是发送单张图片还是其他
	divoom_disp_ctrl_gif_info gif_info;
	divoom_disp_ctrl_real_draw_ctrl dot_info; //点数信息
	uint8 disp_pic_buf[BUF_SIZE]; //保存画板图片数据
}divoom_disp_ctrl_phone_draw_pic_info, *Pdivoom_disp_ctrl_phone_draw_pic_info;

typedef struct _divoom_disp_phone_ctrl_pic_info
{
	uint16 data_off;
    divoom_disp_ctrl_gif_info gif_info;
}divoom_disp_phone_ctrl_pic_info, *Pdivoom_disp_phone_ctrl_pic_info;


//DIVOOM_DISP_GIF_CTRL的数据结构
typedef struct _divoom_disp_ctrl_product_test_info
{
    uint8  mode;
    uint8  status;
}divoom_disp_ctrl_product_test_info, *Pdivoom_disp_ctrl_product_test_info;

#define DIVOOM_DISP_CTRL_MUSIC_MAX_ITEM  12
//DIVOOM_DISP_MUISE_MODE的数据结构
typedef struct _divoom_disp_ctrl_music_info
{  
    divoom_clock_id_info pclock_info;
    uint8 mode;
}divoom_disp_ctrl_music_info, *Pdivoom_disp_ctrl_music_info;


//DIVOOM_DISP_SPECIAL_MODE的数据结构
typedef struct _divoom_disp_ctrl_special_info
{
    uint8 mode;
}divoom_disp_ctrl_special_info, *Pdivoom_disp_ctrl_special_info;


//DIVOOM_DISP_WIFI_MUSIC_MODE的数据结构
typedef struct _divoom_disp_ctrl_wifi_music_mode_info
{
    uint8 mode;
}divoom_disp_ctrl_wifi_music_mode_info, *Pdivoom_disp_ctrl_wifi_music_mode_info;


//DIVOOM_DISP_LIGHT_MODE的数据结构
typedef struct _divoom_disp_ctrl_env_info
{
    uint8 effect_type;
    uint8 time_flag;
    uint8 weather_flag;
    uint8 temp_flag;
    uint8 date_flag;
    uint32 refresh_env_type;
    uint32 mode;
    divoom_screen_color_info color;
}divoom_disp_ctrl_env_info, *Pdivoom_disp_ctrl_env_info;


//DIVOOM_DISP_LIGHT_MODE的数据结构
typedef struct _divoom_disp_ctrl_light_color_info
{
    uint8 on_off;
    uint8 plant_mode;
    uint8 light;
    divoom_screen_color_info color;
}divoom_disp_ctrl_light_color_info, *Pdivoom_disp_ctrl_light_color_info;

//DIVOOM_DISP_SCORE_MODE的数据结构
typedef struct _divoom_disp_ctrl_bat_ctrl
{
    uint8  bat_level;
    uint8 bat_charge;
}divoom_disp_ctrl_bat_ctrl, *Pdivoom_disp_ctrl_bat_ctrl;


//DIVOOM_DISP_SCORE_MODE的数据结构
typedef struct _divoom_disp_ctrl_score_ctrl
{
    uint8  on_off;
    uint16 red_score;
    uint16 blue_score;
}divoom_disp_ctrl_score_ctrl, *Pdivoom_disp_ctrl_score_ctrl;

//DIVOOM_DISP_WATCH_MODE的数据结构
typedef struct _divoom_disp_ctrl_watch_ctrl
{
    //uint32 start_time;
	//防止被打断的时候不能正确获取到暂停之前的时间
	//uint32 pause_time;
	uint32 watch_disp_value;
    uint8  ctrl_flag;
}divoom_disp_ctrl_watch_ctrl, *Pdivoom_disp_ctrl_watch_ctrl;


//DIVOOM_DISP_MOVIE_MODE的数据结构
typedef struct _divoom_disp_ctrl_movie_ctrl
{
    uint8 attr_flag;    //0:更新图像
    void *attr_data;
}divoom_disp_ctrl_movie_ctrl, *Pdivoom_disp_ctrl_movie_ctrl;


//DIVOOM_DISP_COUNT_TIME_DOWN_MODE,		//倒计时数据结构-->状态+时间
//时间是换算好的utc时间
typedef struct _divoom_disp_ctrl_count_time_dowm_struct_
{
	uint8 status;
	uint32 next_reach_time;
}divoom_disp_count_time_down_struct, *Pdivoom_disp_count_time_down_struct;

#define DIVOOM_DISP_HIGH_PTI_EVENT_MAX_PIC 24
//DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL,	//高等级提醒事件，仅次于闹钟的事件
typedef struct _divoom_disp_high_pri_event_
{
	uint8 on_time;       //显示时间
	uint8 period_time;   //显示一张图片的总时间:图片时间和不显示图片时间
	uint8 tol_pic;       //
	uint16 tol_time;      //总时间 100ms
	uint16 disp_id[DIVOOM_DISP_HIGH_PTI_EVENT_MAX_PIC];    //??ID

	divoom_disp_event_extend_info extend_info;
}divoom_disp_high_pri_event_struct, *Pdivoom_disp_high_pri_event_struct;
enum{
    DIVOOM_DISP_SUBSCRIBE_MODE_HOT,
    DIVOOM_DISP_SUBSCRIBE_MODE_FAVORITE,
    DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT,
    DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM,
    DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST,
};
enum{
    DIVOOM_DISP_SUBSCRIBE_DISP_LAST,
    DIVOOM_DISP_SUBSCRIBE_DISP_ALL,
};


//DIVOOM_DISP_SUBSCRIBE_MODE的数据结构
typedef struct _divoom_disp_subscribe_data
{
    uint8 disp_default; //０:不显示默认图片，１：显示默认图片
    uint8 mode;    //订阅类型0:推荐图库1：收藏2：订阅作者
    uint8 disp;    //0：循环最新1：随机全部
    int   play_time;
    int   show_time;
    int   album_id;
    int   play_id;
    int   expert_num;
    int   expert_list[100];
}divoom_disp_subscribe_data, *Pdivoom_disp_subscribe_data;


typedef struct{
	uint8    on_off;          // 1:on; 0:off
	uint8	month;			/* 1-12 */
	uint8	day;			/* 1-31 */
    uint8    hour;           /* 0-23 */
    uint8    minute;         /* 0-59 */
	uint8    title_name[32];
    uint8    trigger_mode;
    uint8    check_code;
	uint32	alarm_set_time;
}divoom_disp_dialy_config_info;


/*********************************************************************
  Function:        divoom_disp_ctrl_set_mode
  Description:    启动相应的灯效
  Input:            mode        灯效的ＩＤ
					light_data         灯效的数据，可以为空
  Return:           NONE
  Author:           albert
**********************************************************************/
void divoom_disp_ctrl_set_mode(uint8 mode, void *light_data);
uint8 divoom_disp_ctrl_get_mode(void);

void divoom_disp_ctrl_init(Pdivoom_disp_ctrl_callback pcallback, DIVOOM_SYSTEM_FLAG sys_flag, uint8 start_mode, Pdivoom_sys_setting_info psys_setting);

void divoom_disp_ctrl_app_exit_mode(uint8 light_model, uint8 model_id, uint8 event_id);

/*********************************************************************
  Function:        divoom_disp_ctrl_fill
  Description:    将显示数据清屏成什么某一颜色
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/



int divoom_disp_ctrl_app_get_env_info(uint8 mode, void *env_data);


uint8 divoom_disp_ctrl_app_active_model(uint8 *is_in_notice);



#if 0

#define DIVOOM_DISP_ENCODE_MAX_BYTE 1100

enum
{
	DIVOOM_DISP_CTRL_LIGHT_50 = 50,
	DIVOOM_DISP_CTRL_LIGHT_70 = 70,
	DIVOOM_DISP_CTRL_LIGHT_SYS = 100,
};

enum
{
    DIVOOM_DISP_ENV_TIME,   //显示时间
    DIVOOM_DISP_ENV_CELS,   //显示摄温度    
    DIVOOM_DISP_ENV_DATE,   //显示日期    
    DIVOOM_DISP_ENV_WEATHER,  //显示天气
    DIVOOM_DISP_ENV_END_TYPE,
    DIVOOM_DISP_ENV_HUMI,   //显示湿度
    DIVOOM_DISP_ENV_BAT,   //显示湿度
};

enum{
	DIVOOM_DISP_COUNT_TIME_DOWM_WAIT,			//等待状态，没有运行
	DIVOOM_DISP_COUNT_TIME_DOWM_RUN,			//运行状态
	DIVOOM_DISP_COUNT_TIME_DOWM_TIME_OVER,		//时间到了的状态，持续五秒
	DIVOOM_DISP_COUNT_TIME_DOWM_PHONE_CANCEL,	//主动取消的状态，持续五秒
};


typedef struct _divoom_screen_color_info
{
    uint8 r_c;
    uint8 g_c;
    uint8 b_c;
}divoom_screen_color_info, *Pdivoom_screen_color_info, divoom_light_color_info, *Pdivoom_light_color_info, *Pdivoom_disp_color_info, divoom_disp_color_info;

typedef struct _divoom_disp_hsv_info
{
	int32 h; 
	uint8 s;
	uint8 v;
}divoom_disp_hsv_info, *Pdivoom_disp_hsv_info;


typedef struct _divoom_disp_ctrl_pic_info
{
    uint16 pic_len;
	uint8 *pic_data;
	uint8 pic_type;  //1表示16*16 2表示32*32 3表示64*64
}divoom_disp_ctrl_pic_info, *Pdivoom_disp_ctrl_pic_info;

typedef struct {
    uint8 tol;
    uint8 on_off;
    uint8 voice_on_off;
    uint8 tol_time;
    uint8 disp_mode;
    uint8 cyc_mode;
    uint32 start_second;
    uint32 end_second;
    uint16 pic_len;
    uint8 pic_data[DIVOOM_DISP_ENCODE_MAX_BYTE];
}divoom_disp_time_manage_config_info;


typedef struct{
	uint8    on_off;          // 1:on; 0:off
	uint8    is_same;          // 1:same; 0:no same
    uint8    hour;           /* 0-23 */
    uint8    minute;         /* 0-59 */
    uint8    week;           /* bit1:monday ~ bit7:sunday*/
    uint8    mode;           //ALARM_MODE
	uint8 	trigger_mode;  //ALARM_TRIGGER_MODE
	//uint8    app_mode;
	//uint8 	app_trigger_mode;  //
	uint8    fm_freq[2];
	uint8 	volume;
	T_BOOL  flag;         //标志位，设置为1
    uint8    check_code;
	uint32	alarm_set_time;
}divoom_disp_alarm_config_info;

typedef struct _divoom_disp_ctrl_time_manage
{
    uint8 id;
    uint8 pass_flag;
    uint8 active_flag;
    uint8 voice_on_off;
    uint8 tol_time;
    uint8 disp_mode;
    uint8 cyc_mode;
    uint8 week;
    uint32 start_second;
    uint32 end_second;
}divoom_disp_ctrl_time_manage, *Pdivoom_disp_ctrl_time_manage;

enum
{
    DIVOOM_DISP_TIME_MODE,
    DIVOOM_DISP_WEATHER_MODE,
    DIVOOM_DISP_NET_APP_MODE,
    DIVOOM_DISP_NO_DISP_MODE,
    DIVOOM_DISP_KEY_END_MODE,
    DIVOOM_DISP_POWER_LOW,
    DIVOOM_DISP_POWER_ON,
    DIVOOM_DISP_EVENT_DIS_CTRL,
    DIVOOM_DISP_ALARM_MODE,
    DIVOOM_DISP_SLEEP_MODE,
    DIVOOM_DISP_LEAVE_MSG_MODE,
    DIVOOM_DISP_TEST_MODE,
    DIVOOM_DISP_RADIO_MODE,
    DIVOOM_DISP_SPEED_BAR_MODE,
    DIVOOM_DISP_WATCH_MODE,
    DIVOOM_DISP_SCORE_MODE,
    DIVOOM_DISP_TIME_MANAGE_MODE,
    DIVOOM_DISP_MIXER_CTRL,
    DIVOOM_DISP_MUSICAL_INSTRUMENTS_MODE,
    DIVOOM_DISP_DEMO_MODE,
	DIVOOM_DISP_COUNT_TIME_DOWN_MODE,		//倒计时
	DIVOOM_DISP_EXPERT_PIC_MODE,	//显示达人图片
	DIVOOM_DISP_DIVOOM_CHANNEL_MODE,	//显示divoom订阅频道图片
	DIVOOM_DISP_APP_POWER_OFF_MODE,		//关机前的动作
	DIVOOM_DISP_IFFT_BUTTON_MODE,
	DIVOOM_DISP_DEVICE_UPDATE_MODE,
    DIVOOM_DISP_INFO_ALL,
};



//DIVOOM_DISP_TIME_MANAGE_MODE的数据结构

typedef struct _divoom_disp_ctrl_time_manage_info
{
    uint16 index;
}divoom_disp_ctrl_time_manage_info, *Pdivoom_disp_ctrl_time_manage_info;



//DIVOOM_DISP_TEMP_EVENT_MODE的数据结构

typedef struct _divoom_disp_ctrl_update_net_temp_info
{
    uint8 each_hour_switch;
    uint8 spec_time_flag;
    uint16 spec_time;
}divoom_disp_ctrl_update_net_temp_info, *Pdivoom_disp_ctrl_update_net_temp_info;


typedef struct _divoom_disp_ctrl_speed_bar_info
{
    uint16   pic_id;
    uint8    value;
    uint8    max_value;
    DIVOOM_LIGHT_DISP_OVER_CALLBACK *pover_callback;
}divoom_disp_ctrl_speed_bar_info, *Pdivoom_disp_ctrl_speed_bar_info;


//DIVOOM_DISP_MIXER_CTRL的数据结构

typedef struct _divoom_disp_ctrl_mixer_info
{
    uint8 mode;
    uint8 color_index;
    uint8 on_off;
    uint8 app_flag;    //APP控制标志
}divoom_disp_ctrl_mixer_info, *Pdivoom_disp_ctrl_mixer_info;

//DIVOOM_DISP_MUSICAL_INSTRUMENTS_MODE的数据结构
typedef struct _divoom_disp_ctrl_musical_instruments_info
{
    uint8 mode;
    uint8 index;
}divoom_disp_ctrl_musical_instruments_info, *Pdivoom_disp_ctrl_musical_instruments_info;

//DIVOOM_DISP_LEAVE_MSG_MODE的数据结构

typedef struct _divoom_disp_ctrl_leave_notice_info
{
    uint8 number;
    uint8 stats;
    char *image_id;
    Pdivoom_leave_msg_voice_item pinfo;
}divoom_disp_ctrl_leave_notice_info, *Pdivoom_disp_ctrl_leave_notice_info;


typedef struct _divoom_disp_ctrl_alarm_info
{
    uint8 status;    //0:闹钟显示状态，1:贪睡显示状态；２：闹钟过期状态
    char alarm_name[64];
    uint8 *image_data;
    int  image_len;
}divoom_disp_ctrl_alarm_info, *Pdivoom_disp_ctrl_alarm_info;


//DIVOOM_DISP_SET_ALARM_MODE的数据结构

typedef struct _divoom_disp_ctrl_set_alarm_info
{
    uint8 key_info;
}divoom_disp_ctrl_set_alarm_info, *Pdivoom_disp_ctrl_set_alarm_info;


//DIVOOM_DISP_SLEEP_MODE的数据结构

typedef struct _divoom_disp_sleep_ctrl
{
    uint8 light;
    uint8 mode;
    divoom_screen_color_info color;
}divoom_disp_sleep_ctrl, *Pdivoom_disp_sleep_ctrl;



//DIVOOM_DISP_EVENT_DIS_CTRL的数据结构

#define DIVOOM_DISP_EVENT_MAX_PIC 24
typedef struct _divoom_disp_event_disp_info
{
    uint8 on_time;       //??????  100ms
    uint8 period_time;   //????             100ms
    uint8 tot_pic;       //
    uint32 tot_time;      //	      100ms*tot_time
    uint16 disp_id[DIVOOM_DISP_EVENT_MAX_PIC];    //PIC ID
    uint16 pic_len;
    uint32 noice_id;      //结束通知标记
    uint8  *pic_data;
    char  *disp_title;
    DIVOOM_LIGHT_DISP_OVER_CALLBACK *pover_callback;
}divoom_disp_event_disp_info, *Pdivoom_disp_event_disp_info;



//DIVOOM_DISP_GAME_CTRL的数据结构

typedef struct _divoom_disp_ctrl_game_info
{
    uint8 mode;
}divoom_disp_ctrl_game_info, *Pdivoom_disp_ctrl_game_info;

//DIVOOM_DISP_PHONE_GIF_CTRL的数据结构

typedef struct _divoom_disp_ctrl_talk_info
{
    uint8 mode;
}divoom_disp_ctrl_talk_info, *Pdivoom_disp_ctrl_talk_info;

//DIVOOM_DISP_GIF_CTRL的数据结构

typedef struct _divoom_disp_ctrl_gif_info
{
    uint32 len;
    uint8* data;
}divoom_disp_ctrl_gif_info, *Pdivoom_disp_ctrl_gif_info;


//DIVOOM_DISP_GIF_CTRL的数据结构

typedef struct _divoom_disp_ctrl_product_test_info
{
    uint8  mode;
    uint8  status;
}divoom_disp_ctrl_product_test_info, *Pdivoom_disp_ctrl_product_test_info;


//DIVOOM_DISP_MUISE_MODE的数据结构

typedef struct _divoom_disp_ctrl_music_info
{
    uint8 mode;
    char file_name[64];
}divoom_disp_ctrl_music_info, *Pdivoom_disp_ctrl_music_info;

//DIVOOM_DISP_ALARM_MODE的数据结构

typedef struct _divoom_disp_ctrl_alarm_disp_info
{
    uint8 mode;
    uint8 index;
}divoom_disp_ctrl_alarm_disp_info, *Pdivoom_disp_ctrl_alarm_disp_info;

//DIVOOM_DISP_SPECIAL_MODE的数据结构

typedef struct _divoom_disp_ctrl_special_info
{
    uint8 mode;
}divoom_disp_ctrl_special_info, *Pdivoom_disp_ctrl_special_info;


//DIVOOM_DISP_WIFI_MUSIC_MODE的数据结构

typedef struct _divoom_disp_ctrl_wifi_music_mode_info
{
    uint8 mode;
}divoom_disp_ctrl_wifi_music_mode_info, *Pdivoom_disp_ctrl_wifi_music_mode_info;


//DIVOOM_DISP_LIGHT_MODE的数据结构

typedef struct _divoom_disp_ctrl_env_info
{
    uint8 mode;
    uint8 time_flag;
    uint8 weather_flag;
    uint8 temp_flag;
    uint8 date_flag;
    divoom_screen_color_info color;
}divoom_disp_ctrl_env_info, *Pdivoom_disp_ctrl_env_info;


//DIVOOM_DISP_LIGHT_MODE的数据结构

typedef struct _divoom_disp_ctrl_light_color_info
{
    uint8 on_off;
    uint8 plant_mode;
    divoom_screen_color_info color;
}divoom_disp_ctrl_light_color_info, *Pdivoom_disp_ctrl_light_color_info;

//DIVOOM_DISP_PHONE_CTRL的数据结构
typedef struct _divoom_disp_ctrl_real_draw_ctrl
{
    divoom_screen_color_info color;
    uint8 num;
    uint16 off_table[64];
}divoom_disp_ctrl_real_draw_ctrl, *Pdivoom_disp_ctrl_real_draw_ctrl;


#define DIVOOM_DISP_DIVOOM_MAX_HOT_NUM 16

//DIVOOM_DISP_DIVOOM_MODE的数据结构
typedef struct _divoom_disp_ctrl_hot_ctrl
{
    uint8 index;
}divoom_disp_ctrl_hot_ctrl, *Pdivoom_disp_ctrl_hot_ctrl;

//DIVOOM_DISP_SCORE_MODE的数据结构
typedef struct _divoom_disp_ctrl_bat_ctrl
{
    uint8  bat_level;
    uint8 bat_charge;
}divoom_disp_ctrl_bat_ctrl, *Pdivoom_disp_ctrl_bat_ctrl;


//DIVOOM_DISP_SCORE_MODE的数据结构
typedef struct _divoom_disp_ctrl_score_ctrl
{
    uint8  on_off;
    uint16 red_score;
    uint16 blue_score;
}divoom_disp_ctrl_score_ctrl, *Pdivoom_disp_ctrl_score_ctrl;

//DIVOOM_DISP_WATCH_MODE的数据结构

typedef struct _divoom_disp_ctrl_watch_ctrl
{
    uint32 start_time;
    uint8  ctrl_flag;
}divoom_disp_ctrl_watch_ctrl, *Pdivoom_disp_ctrl_watch_ctrl;

//DIVOOM_DISP_COUNT_TIME_DOWN_MODE,		//倒计时数据结构-->状态+时间
//时间是换算好的utc时间
typedef struct _divoom_disp_ctrl_count_time_dowm_struct_
{
	uint8 status;
	uint32 next_reach_time;
}divoom_disp_count_time_down_struct, *Pdivoom_disp_count_time_down_struct;


typedef struct{
	uint8    on_off;          // 1:on; 0:off
	uint8	month;			/* 1-12 */
	uint8	day;			/* 1-31 */
    uint8    hour;           /* 0-23 */
    uint8    minute;         /* 0-59 */
	uint8    title_name[32];
    uint8    trigger_mode;
    uint8    check_code;
	uint32	alarm_set_time;
}divoom_disp_dialy_config_info;


typedef void (*DIVOOM_LIGHT_PRINT)(char *format, ...);
typedef void * (*DIVOOM_LIGHT_MALLOC)(uint32 size, uint32 model_line, const char *file_name);
typedef void (*DIVOOM_LIGHT_FREE)(void *ptr);
typedef void *(*DIVOOM_LIGHT_OPEN_USER_DATA)(const char *file_name, int cache_flag);
typedef int  (*DIVOOM_LIGHT_RETUEN_ALL_LEN)(void *handle);
typedef int (*DIVOOM_LIGHT_READ_USER_DATA)(void *handle, void *buf, int size, int off);
typedef int (*DIVOOM_LIGHT_WRITE_USER_DATA)(void *handle, void *buf, int size, int off);
typedef void (*DIVOOM_LIGHT_CLOSE_USER_DATA)(void *handle);
typedef int (*DIVOOM_LIGHT_READ_CONFIG)(uint8 mode, void *buf, int size);
typedef int (*DIVOOM_LIGHT_WRITE_CONFIG)(uint8 mode, void *buf, int size);
typedef uint32 (*DIVOOM_LIGHT_WRITE_DATA)(void *buf, int size);
typedef void (*DIVOOM_LIGHT_WRITE_MULTI_SLAVE_DATA)(void *pic_data, int width, int high); //一张图片，拉升成n*m
typedef void (*DIVOOM_LIGHT_WRITE_MULTI_GIF_DATA)(void *pic_data, int width, int high); //一张图片，拉升成n*m
typedef void (*DIVOOM_LIGHT_SEND_MULTI_GIF_DATA)(void*gif_info, uint8 gif_type); //主机先发送动画数据到从机，再显示
typedef uint8 (*DIVOOM_LIGHT_WRITE_CMD)(uint16 cmd, void *data, int size);
typedef void DIVOOM_LIGHT_CALLBACK (void *data);
typedef uint32 (*DIVOOM_LIGHT_TASK_CREATE)(DIVOOM_LIGHT_CALLBACK *pcallback, uint32 time);
typedef void (*DIVOOM_LIGHT_TASK_DESTROY)(uint32 task_id);
typedef void (*DIVOOM_LIGHT_TASK_PAUSE)(uint32 task_id);
typedef void (*DIVOOM_LIGHT_TASK_RESUME)(uint32 task_id);
typedef void (*DIVOOM_LIGHT_TASK_UPDATE)(uint32 task_id, uint32 time);
typedef int (*DIVOOM_LIGHT_GET_ENV_INFO)(uint8 mode, void *env_data);
typedef int (*DIVOOM_LIGHT_GET_RAND)(int max);
typedef int (*DIVOOM_LIGHT_GET_VOL)(void);
typedef void (*T_Voice_PalyEndCallBack)(void);
typedef void (*DIVOOM_LIGHT_PLAY_VOICE)(uint16 voice_tip, uint16 time, uint8 vol, T_Voice_PalyEndCallBack callback);

typedef struct _divoom_disp_env_time_info
{
    uint8 hour;
    uint8 min;
    uint32 sys_tick;
}divoom_disp_env_time_info, *Pdivoom_disp_env_time_info;

typedef struct _divoom_disp_env_date_info
{
    uint8 month;
    uint8 day;
    uint8 week;
}divoom_disp_env_date_info, *Pdivoom_disp_env_date_info;




typedef struct _divoom_disp_ctrl_callback
{
    DIVOOM_LIGHT_PRINT  print;
    DIVOOM_LIGHT_MALLOC malloc;
    DIVOOM_LIGHT_FREE   free;
    DIVOOM_LIGHT_READ_CONFIG    read;
    DIVOOM_LIGHT_WRITE_CONFIG   write;
    DIVOOM_LIGHT_TASK_CREATE    create;
    DIVOOM_LIGHT_TASK_DESTROY   destroy;
    DIVOOM_LIGHT_TASK_PAUSE     pause;
    DIVOOM_LIGHT_TASK_RESUME    resume;
    DIVOOM_LIGHT_TASK_UPDATE    update;
    DIVOOM_LIGHT_OPEN_USER_DATA open_file;
	DIVOOM_LIGHT_RETUEN_ALL_LEN return_file_len;
    DIVOOM_LIGHT_READ_USER_DATA read_file;
    DIVOOM_LIGHT_WRITE_USER_DATA write_file;
    DIVOOM_LIGHT_CLOSE_USER_DATA close_file;
    DIVOOM_LIGHT_WRITE_CMD    write_light_cmd;
    DIVOOM_LIGHT_WRITE_DATA    write_light_data;
	DIVOOM_LIGHT_WRITE_MULTI_SLAVE_DATA write_light_muti_slave_data;
	DIVOOM_LIGHT_WRITE_MULTI_GIF_DATA write_light_muti_gif_data;
	DIVOOM_LIGHT_SEND_MULTI_GIF_DATA send_light_muti_gif_data;
    DIVOOM_LIGHT_GET_ENV_INFO   get_env_info;
    DIVOOM_LIGHT_GET_RAND       get_rand;
    DIVOOM_LIGHT_PLAY_VOICE     play_voice;
    DIVOOM_LIGHT_GET_VOL        get_vol;
}divoom_disp_ctrl_callback, *Pdivoom_disp_ctrl_callback;

typedef struct _divoom_light_config_info
{
    uint8 low_energy_ctrl;
    uint8 time_manage_flag;
    uint8 voice_ctrl;
    uint8 sys_light;
    uint8 light_mode;
    uint8 special_mode;
    uint8 color_r;
    uint8 color_g;
    uint8 color_b;
    uint8 time_color_r;
    uint8 time_color_g;
    uint8 time_color_b;
    uint8 time_just_num_color_r;
    uint8 time_just_num_color_g;
    uint8 time_just_num_color_b;
    uint8 muise_type;
    uint8 temp_type;
    uint8 time_type;
    uint8 env_effect_mode;
    uint32 env_type;
    uint32 net_temp_disp_type;
    uint32 net_temp_disp_time;
    divoom_disp_ctrl_hot_ctrl hot_info;
    uint8 muise_rhythm_index[32];
    uint8  rsv_info[32];
}divoom_light_config_info, *Pdivoom_light_config_info;


typedef enum
{
    DIVOOM_SYSTEM_LINEX,
    DIVOOM_SYSTEM_RTOS,
    DIVOOM_SYSTEM_END,
}DIVOOM_SYSTEM_FLAG;

/*********************************************************************
  Function:        divoom_disp_ctrl_init
  Description:    对灯效的初始化动作，
  Input:            pcallback        所有的回调函数
                       sys_flag         取决于实时性
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_disp_ctrl_init(Pdivoom_disp_ctrl_callback pcallback, DIVOOM_SYSTEM_FLAG sys_flag, uint8 start_mode, Pdivoom_sys_setting_info psys_setting);


/*********************************************************************
  Function:        divoom_disp_ctrl_deinit
  Description:    退出灯效模块，释放资源，关闭灯板
  Input:            NONE
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_disp_ctrl_deinit(void);

/*********************************************************************
  Function:        divoom_disp_ctrl_dis_task
  Description:    	控制底层显示任务
  Input:            mode        灯效的ＩＤ
                      	flag 	1 播放 0暂停
  Return:           NONE
  Author:           wangft
**********************************************************************/

void divoom_disp_ctrl_dis_task(uint8 mode, uint8 flag);

/*********************************************************************
  Function:        divoom_disp_ctrl_set_mode
  Description:    启动相应的灯效
  Input:            mode        灯效的ＩＤ
					light_data         灯效的数据，可以为空
  Return:           NONE
  Author:           albert
**********************************************************************/
void divoom_disp_ctrl_set_mode(uint8 mode, void *light_data);



/*********************************************************************
  Function:        divoom_disp_ctrl_update_light_info
  Description:    更新灯效的相关信息
  Input:            mode        灯效的ＩＤ
                      data         灯效的数据
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_disp_ctrl_update_light_info(uint8 mode, void *data);


/*********************************************************************
  Function:        divoom_disp_ctrl_get_light_info
  Description:    得到 所有灯效的数据
  Input:            data         灯效的数据，可以为空
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_disp_ctrl_get_config(Pdivoom_light_config_info plight_info);



/*********************************************************************
  Function:        divoom_disp_ctrl_set_bright
  Description:    设置灯的系统亮度
  Input:            light        灯的亮度，以１００%为最高亮度
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_disp_ctrl_set_bright(uint8 light);

/*********************************************************************
  Function:        divoom_disp_ctrl_get_bright
  Description:    得到灯的系统亮度
  Input:            ＮＯＮＥ
  Return:           系统亮度
  Author:           albert
**********************************************************************/

uint8 divoom_disp_ctrl_get_light(void);

/*********************************************************************
  Function:        divoom_disp_ctrl_set_hot_num
  Description:    设置hot文件数量
  Input:            hot_num 数量
  Return:           NONE
  Author:           wangft
**********************************************************************/

void divoom_disp_ctrl_set_hot_num(uint8 hot_num);


/*********************************************************************
  Function:        divoom_disp_ctrl_change_attr
  Description:    在当前效果中自动切换不同灯光
  Input:            arrow　　　切换的方向
  Return:           切换是否成功
  Author:           albert
**********************************************************************/

uint8 divoom_disp_ctrl_change_attr(T_BOOL arrow);


/*********************************************************************
  Function:        divoom_disp_ctrl_get_mode
  Description:    返回当前正在处理的灯效
  Input:            NONE
  Return:           none
  Author:           albert
**********************************************************************/


uint8 divoom_disp_ctrl_get_mode(void);

/*********************************************************************
  Function:        divoom_disp_ctrl_exit_cur_mode
  Description:    退出当前显示灯效模式
  Input:            NONE
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_disp_ctrl_exit_cur_mode(void);

/*********************************************************************
  Function:        divoom_disp_ctrl_switch_light
  Description:    灯的开关动作
  Input:            NONE
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_disp_ctrl_switch_light(void);


/*********************************************************************
  Function:        divoom_disp_ctrl_auto_mode
  Description:    按键切换到 上或下一个灯效状态
  Input:            NONE
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_disp_ctrl_auto_mode(uint8 flag);

void divoom_disp_ctrl_auto_light(uint8 light);
void divoom_disp_ctrl_word_update_font_info(uint16 len, uint16 *font_info);
void divoom_disp_ctrl_word_update_speed(uint16 speed);
void divoom_disp_ctrl_clear_drawing_pad(void);
uint8 divoom_disp_ctrl_update_pic(uint8 *buf);
void divoom_disp_ctrl_set_time_type(uint8 type);
void divoom_disp_ctrl_set_temp_type(uint8 type);

#endif

#endif


