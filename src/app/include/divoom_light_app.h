#ifndef _divoon_light_app_h_
#define _divoon_light_app_h_

#define DIVOOM_LIGHT_APP_MIXER_EFFECT_COUNT 9
#define DIVOOM_LIGHT_APP_MIXER_COLOR_COUNT  6

typedef struct _divoom_light_app_mixer_state_
{
    uint8 effect;
    uint8 color;
    uint8 running;
    uint32 generation;
}divoom_light_app_mixer_state, *Pdivoom_light_app_mixer_state;

typedef struct _divoom_light_app_mix_music2_state_
{
    uint8 effect;
    uint8 icon;
    uint8 activation_count;
    uint32 generation;
}divoom_light_app_mix_music2_state, *Pdivoom_light_app_mix_music2_state;

enum
{
    DIVOOM_LIGHT_APP_EVENT_UPDATE_DATA_MODE,
	DIVOOM_LIGHT_APP_EVENT_DRAW_REMOTE, //线上动画
	DIVOOM_LIGHT_APP_EVENT_DRAW_REMOTE_BY_APP, //通过app获取服务器上的动画
	DIVOOM_LIGHT_APP_EVENT_DRAW_LOCAL,//本地动画
    DIVOOM_LIGHT_APP_EVENT_ENTER_SYNC_MODE,//实时画板
    DIVOOM_LIGHT_APP_EVENT_EXIT_SYNC_MODE,//退出实时画板
  	DIVOOM_LIGHT_APP_EVENT_DRAW_SET_INFO, //画板属性
  	DIVOOM_LIGHT_APP_EVENT_DRAW_SET_SPEED_MODE_INFO, //设置动画速度
    DIVOOM_LIGHT_APP_EVENT_DEL_USER_DEFINE_INDEX,   //删除自定义索引
    DIVOOM_LIGHT_APP_EVENT_DEL_USER_DEFINE_ONE,   //删除自定义索引
    DIVOOM_LIGHT_APP_EVENT_ADD_USER_DEFINE_INDEX,   //增加自定义索引
    DIVOOM_LIGHT_APP_EVENT_SET_USER_DEFINE_INDEX,   //自定义索引
    DIVOOM_LIGHT_APP_EVENT_GET_USER_DEFINE_INDEX,   //自定义索引
    DIVOOM_LIGHT_APP_EVENT_SET_PLAY_USER_DEFINE,   //播放自定义索引
    DIVOOM_LIGHT_APP_EVENT_SET_CHANNEL_INDEX,   //播放频道
    DIVOOM_LIGHT_APP_EVENT_GET_CHANNEL_INDEX,   //播放频道
    DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX,     //时钟索引
    DIVOOM_LIGHT_APP_EVENT_NET_SET_CLOCK_INDEX,     //app选择时钟索引
 	DIVOOM_LIGHT_APP_EVENT_SET_NEXT_CLOCK,
    DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_ALL,     //更新时钟全部配置信息
    DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_APP,     //更新时钟app配置信息
    DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_SYS,     //更新时钟系统配置信息
    DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_LIST,     //更新全部时钟列表信息
    DIVOOM_LIGHT_APP_EVENT_SET_PRODUCE_TIME,
    DIVOOM_LIGHT_APP_EVENT_SET_CUSTOM_TIME,
    DIVOOM_LIGHT_APP_EVENT_GET_CUSTOM_TIME,
    DIVOOM_LIGHT_APP_EVENT_UPDATE_EQ_LIST,
    DIVOOM_LIGHT_APP_EVENT_PLAY_REAL_EQ,
    DIVOOM_LIGHT_APP_EVENT_SET_EQ_POS,
    DIVOOM_LIGHT_APP_EVENT_GET_EQ_POS,
    DIVOOM_LIGHT_APP_EVENT_DEL_EQ_POS,
    DIVOOM_LIGHT_APP_EVENT_ADD_EQ_ITEM,
    DIVOOM_LIGHT_APP_EVENT_SET_START_UP_CHANNEL,   //开机频道
    DIVOOM_LIGHT_APP_EVENT_GET_START_UP_CHANNEL,   //开机频道
    DIVOOM_LIGHT_APP_EVENT_UPDATE_SUBCRIBE_INFO,   //更新订阅信息
    DIVOOM_LIGHT_APP_EVENT_ADD_REAL_EQ_DATA,       //实时ＥＱ数据
    DIVOOM_LIGHT_APP_EVENT_UPDATE_REAL_EQDATA_MODE, //实时ＥＱ数据更新完成
    DIVOOM_LIGHT_APP_EVENT_END_NOTICE_MSG_MODE,     //提醒超时
    DIVOOM_LIGHT_APP_EVENT_UPDATE_CHANNEL_GLOBAL_CONFIG,     //更新频道配置
    DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_HISTORY,     //更新表盘历记录
    DIVOOM_LIGHT_APP_EVENT_UPDATE_SERVE_CHANNEL_INFO,     //上传表盘相关信息
    DIVOOM_LIGHT_APP_EVENT_UPDATE_SUSCRIBE_EXPERT_LIST,     //更新达人列表
    DIVOOM_LIGHT_APP_EVENT_UPDATE_SUSCRIBE_FAVORITE_LIST,     //更新收藏列表
	DIVOOM_LIGHT_APP_EVENT_DRAW_LOCAL_REMOTE, //线上转本地图
	DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL,         //控制屏开关
	DIVOOM_LIGHT_APP_EVENT_DISPLAY_IFFFT_TEXT,         //显示ＩＦＦＦＴ的提醒信息
	DIVOOM_LIGHT_APP_EVENT_DISPLAY_IFFFT_AMAZON_MUSIC,   	 //显示IFTTT亚马逊音乐
	DIVOOM_LIGHT_APP_EVENT_EXIT_DISPLAY_IFFFT_TEXT,         //退出显示ＩＦＦＦＴ的提醒信息
	DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_GIF,              //显示用户通过ＨＴＴＰ发过来的图像
	DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_TEXT,              //显示用户通过ＨＴＴＰ发过来的文本
	DIVOOM_LIGHT_APP_EVENT_CLEAR_HTTP_IMAGE_TEXT,              //清除用户通过ＨＴＴＰ发过来的文本
	DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_TEXT_LIST,              //显示用户通过ＨＴＴＰ发过来的文本列表
	DIVOOM_LIGHT_APP_EVENT_SET_CLOUD_INDEX_CHANNEL,     //云频道选择
	DIVOOM_LIGHT_APP_EVENT_EXIT_GIF_WAIT_STATUS,     //退出等待动画的状态
	DIVOOM_LIGHT_APP_EVENT_NOTICE_CLOCK_CHANGE,     //表盘数据发生变化通知
	DIVOOM_LIGHT_APP_EVENT_PLAY_LOCAL_GIF_FILE,          //显示tf卡的ＧＩＦ文件
	DIVOOM_LIGHT_APP_EVENT_PLAY_LOCAL_GIF_FOLDER,          //显示tf卡的ＧＩＦ文件夹
	DIVOOM_LIGHT_APP_EVENT_DOWNLOAD_NET_GIF_FILE,          //下载外部的gif文件
	DIVOOM_LIGHT_APP_EVENT_PLAY_NET_GIF_FILE,          //播放外部的gif文件
    DIVOOM_LIGHT_APP_EVENT_UPDATE_HTTP_GIF_MODE,
    DIVOOM_LIGHT_APP_EVENT_RESET_HTTP_GIF_ID,
    DIVOOM_LIGHT_APP_EVENT_START_SCHEDULE_CLOCK,        //表盘计划
    DIVOOM_LIGHT_APP_EVENT_START_SCHEDULE_USER_DEFINE,
    DIVOOM_LIGHT_APP_EVENT_GET_USER_DEFINE_TIME_LIST,
    DIVOOM_LIGHT_APP_EVENT_SET_USER_DEFINE_TIME,
    DIVOOM_LIGHT_APP_EVENT_GET_SUSCRIBE_TIME_LIST,
    DIVOOM_LIGHT_APP_EVENT_SET_SUSCRIBE_TIME,
    DIVOOM_LIGHT_APP_EVENT_RESET_SAVE_CLOCK,
    DIVOOM_LIGHT_APP_EVENT_DIVOOM_SELF_RESET,
    DIVOOM_LIGHT_APP_EVENT_EXIT_NOTICE_CLOCK,
    DIVOOM_LIGHT_APP_EVENT_TEMP_CLOCK_PARA,
	DIVOOM_LIGHT_APP_EVENT_UPDATE_SYS_COMPONENT_LIST,	  //更新全部组件列表信息
	DIVOOM_LIGHT_APP_EVENT_UPDATE_SYS_COMPONENT_INFO, //更新单个组件信息
	DIVOOM_LIGHT_APP_EVENT_ENTER_ALBUM_PLAY, //app进入相簿，设备开始相簿播放
	DIVOOM_LIGHT_APP_EVENT_APP_HTTP_LOCAL_IMAG,              //app发过来的HTTP图像
	DIVOOM_LIGHT_APP_EVENT_APP_HTTP_LOCAL_EQ,              //app发过来的EQ图像
	DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO,              //app发过来的照片
	DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL,              //显示开机表盘
    DIVOOM_LIGHT_APP_EVENT_EXIT_PHOTO_VIEW,             //退出菜单显示
    DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL,             //显示表盘
    DIVOOM_LIGHT_APP_EVENT_MENU_SET_CLOCK_INDEX,     //菜单切换表盘
	DIVOOM_LIGHT_APP_EVENT_UPDATE_GIF_EDGE_CLOCK,          //更新表盘边框
	DIVOOM_LIGHT_APP_EVENT_SCREEN_APP_CTRL,         //APP控制屏开关
    DIVOOM_LIGHT_APP_EVENT_REMOVE_POHTO_INFO,       //移除相簿信息
    DIVOOM_LIGHT_APP_EVENT_GOOGLE_CALENDAR_CTRL,       //谷歌日历控制
    DIVOOM_LIGHT_APP_EVENT_ENTER_GOOGLE_CALENDAR,       //谷歌日历控制
    DIVOOM_LIGHT_APP_EVENT_ENTER_OUTLOOK_CALENDAR,       //OUTLOOK日历控制
    DIVOOM_LIGHT_APP_EVENT_OUTLOOK_CALENDAR_CTRL,       //OUTLOOK日历控制
    DIVOOM_LIGHT_APP_EVENT_ACTIVE_CALENDAR_TIMEOUT,       //日历超时退出
    DIVOOM_LIGHT_APP_EVENT_DISP_CUSTOM_CLOCK,       //显示自定义表盘
    DIVOOM_LIGHT_APP_EVENT_EXIT_DISP_CUSTOM_CLOCK,       //显示自定义表盘
    DIVOOM_LIGHT_APP_EVENT_UPDATE_VISION_PLAY_TIME,       //更新播放时间
	DIVOOM_LIGHT_APP_EVENT_DISP_DEMO_DIAL,       //显示表盘展示页面
	DIVOOM_LIGHT_APP_EVENT_PLAY_LIST,       //播放列表
    DIVOOM_LIGHT_APP_EVENT_UPDATE_SELF_CLOCK_SYS,     //更新时钟系统配置信息
    DIVOOM_LIGHT_APP_EVENT_MIXER_START,     //进入炫音工厂
    DIVOOM_LIGHT_APP_EVENT_MIXER_STOP,      //退出炫音工厂
    DIVOOM_LIGHT_APP_EVENT_MIXER_SET,       //旧版 Mixer/Set 控制
    DIVOOM_LIGHT_APP_EVENT_MIXER_SET_NEW,   //新版 Mixer/SetNew 控制
    DIVOOM_LIGHT_APP_EVENT_WAIT_CLOCK_DOWNLOAD, //添加表盘后等待配置和素材下载
    DIVOOM_LIGHT_APP_EVENT_CLOCK_RESOURCE_DOWNLOAD_DONE, //表盘资源成功或失败后均已收敛
    DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD, //用户主动退出表盘下载页
    DIVOOM_LIGHT_APP_EVENT_ADD_CLOCK_RECEIVED, //MyClockAdd 直接交给界面业务线程
    DIVOOM_LIGHT_APP_EVENT_LAN_CREATE_CLOCK_BEGIN, //LAN 创建表盘：先进入等待页释放显示资源
    DIVOOM_LIGHT_APP_EVENT_CLEAR_REMOVED_CLOCK, // deferred direct cleanup
};

enum{
    DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT,   //显示本地MQTT动画
    DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT,   //显示服务动画
    DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT,   //显示实时动画
    DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT,   //基本灯效
    DIVOOM_LIGHT_APP_NOTICE_MSG_EVENT,   //动画提醒
    DIVOOM_LIGHT_APP_IFFT_TEXT_EVENT,   //显示ＩＦＦＦＴ的字符串提醒
    DIVOOM_LIGHT_APP_TF_GIF_EVENT,   //显示TF卡ＧＩＦ
    DIVOOM_LIGHT_APP_HTTP_GIF_CTRL_EVENT,   //显示http动画
    DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_EVENT,   //时钟计划运行
    DIVOOM_LIGHT_APP_CLOCK_NOTICE_EVENT,   //表盘通知事件
    DIVOOM_LIGHT_APP_SCHEDULE_CLOCK_EVENT,   //播放定时表盘
    DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT,   //app发送照片
    DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_EVENT,   //激活谷歌日历
    DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_DISP,   //显示谷歌日历
    DIVOOM_LIGHT_APP_OUTLOOK_CALENDAR_DISP,   //显示OUTLOOK日历
    DIVOOM_LIGHT_APP_CUSTOM_DIAL_DISP,   //显示自定义表盘
	DIVOOM_LIGHT_APP_DEMO_DIAL_DISP,   //显示表盘展示
    DIVOOM_LIGHT_APP_PLAY_LIST_EVENT,   //播放列表
    DIVOOM_LIGHT_APP_MIXER_EVENT,       //炫音工厂全屏显示
    DIVOOM_LIGHT_APP_MIX_MUSIC2_EVENT,  //Mixer/SetNew 乐器页全屏显示
};
void divoom_light_app_get_mixer_state(Pdivoom_light_app_mixer_state mixer_state);
void divoom_light_app_get_mix_music2_state(Pdivoom_light_app_mix_music2_state mix_music2_state);
void divoom_light_app_set_clock_from_lvgl_dial_preview(int clock_id, int clock_mode);
/** 填充 base_light.clock_info 并返回指针，供 divoom_menu_control_clearall_load(SYSTEM_DIAL_ID, …, model_id, event_id)；失败返回 NULL */
void *divoom_light_app_system_dial_clearall_user_data(void);
int divoom_light_app_get_next_clock_id(void);
int divoom_light_app_get_pre_clock_id(void);

int divoom_light_app_get_pre_clock_from_id(int clock_id);
int divoom_light_app_get_pre_clock2_from_id(int clock_id);
int divoom_light_app_get_next_clock_from_id(int clock_id);
int divoom_light_app_get_next_clock2_from_id(int clock_id);
void divoom_light_app_get_my_clock_list(void);
void divoom_light_app_get_my_clock_recent_list(void);
void divoom_light_app_get_my_clock_favorite_list(void);
int divoom_light_app_get_my_clock_num(void);
int divoom_light_app_copy_my_clock_list(int *clock_list);
/** 最多拷贝 max_count 条，防止调用方缓冲区小于 history_num 时栈/堆溢出；返回实际拷贝条数 */
int divoom_light_app_copy_my_clock_list_limited(int *clock_list, int max_count);
int *divoom_light_app_get_normal_clock_list_exist(int *clock_num);

void divoom_light_app_update_my_clock_list(Pdivoom_devcice_get_clock_list_respone clock_list);
uint8 divoom_light_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);
uint8 divoom_light_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_light_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);
uint8 divoom_light_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);
void divoom_light_app_init(int sys_reset_status);
void divoom_light_app_deinit(void);
void divoom_light_ctrl_main_loop(void);

/*
  Function:       divoom_light_app_get_cur_clock_id
  Description:    得到当前播放的表盘
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_cur_clock_id(void);

/*
  Function:       divoom_light_app_show_plan_clock
  Description:    播放计划/定时表盘统一显示
  Input:          clock_id, voice_flag
  Return:         NONE
*/

void divoom_light_app_show_plan_clock(int clock_id, uint32 voice_flag);

/*
  Function:       divoom_light_app_get_change_clock_config
  Description:    得到表盘配置
  Input:          none
  Return:         NONE
*/

void divoom_light_app_get_change_clock_config(Pdivoom_channel_channel_cfg_info pconfig_info);

/*
  Function:       divoom_light_app_get_each_gif_play_time
  Description:    得到动画播放时间
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_each_gif_play_time(void);

/*
  Function:       divoom_ight_app_get_show_time_flag
  Description:    得到动画播放是否在右上角显示时间的功能
  Input:          none
  Return:         NONE
*/

uint8 divoom_ight_app_get_show_time_flag(uint8 index);

/*
  Function:       divoom_light_app_update_user_define_image_id
  Description:    保存正在播放的动画ＩＤ
  Input:          none
  Return:         NONE
*/

void divoom_light_app_update_user_define_image_id(int image_id);


/*
  Function:       divoom_light_app_exit_wait_gif_status
  Description:    退出等待动画的状态
  Input:          none
  Return:         NONE
*/

void divoom_light_app_exit_wait_gif_status(void);

/*
  Function:       divoom_light_app_get_user_define_handle
  Description:    得到自定义播放的句柄
  Input:          none
  Return:         NONE
*/

void *divoom_light_app_get_user_define_handle(void);

/*
  Function:       divoom_light_app_get_screen_status
  Description:    得到开关屏的相关状态
  Input:          none
  Return:         status
*/

uint8 divoom_light_app_get_screen_status(uint8 flag);

/*
  Function:       divoom_light_app_get_vision_play_time
  Description:    获取播放时间
  Input:          none
  Return:         time
*/

int divoom_light_app_get_vision_play_time(void);

void divoom_light_app_update_last_clock_id(int clock_id);
int divoom_light_app_open_screen(void);

/*
  Function:       divoom_light_app_get_user_define_play_time
  Description:    获取自自定义播放动画时间
  Input:          index
  Return:         
*/

int divoom_light_app_get_user_define_play_time(int index);

/*
  Function:       divoom_light_app_get_suscribe_play_time
  Description:    获取云图播放动画时间
  Input:          
  Return:         
*/

int divoom_light_app_get_suscribe_play_time(void);

/*
  Function:       divoom_light_app_request_timeout_resert
  Description:    获取云图播放动画时间
  Input:          
  Return:         
*/

void divoom_light_app_request_timeout_resert(void);

/*
  Function:       divoom_light_app_check_notice_status
  Description:    检查是否在表盘通知状态
  Input:          
  Return:         1:在表盘通知，0：不在表盘通知
*/

uint8 divoom_light_app_check_notice_status(void );

/*
  Function:       divoom_light_app_check_cloud_photo_clock
  Description:    检查是否是客户定制表盘
  Input:          
  Return:         1:　or 0
*/

int divoom_light_app_check_cloud_photo_clock(int clock_id);

/*
  Function:       divoom_light_app_clock_notice_event
  Description:    某个表盘的通知事件，
  Input:          pclock_info
                  msg
                  up_down_flag
                  notice_value
  Return:         none
*/


void divoom_light_app_clock_notice_event(Pdivoom_clock_id_info pclock_info, char *msg, int up_down_flag, double notice_value);

void divoom_light_app_play_all_id(void *user_data, T_U8 id, Pdivoom_play_gif_image_info image_info);
void divoom_light_app_subscribe_get_pic_id(void *user_data, T_U8 area_id, Pdivoom_play_gif_image_info image_info);
int divoom_light_app_subscribe_get_change(void *user_data);
int divoom_light_app_get_divoom_animation_edge_clock_id(void);

/*
  Function:       divoom_light_app_ctrl_cloud_clock
  Description:    启动表盘
  Input:          clock_id
  Return:         none
*/

void divoom_light_app_ctrl_cloud_clock(int clock_id, int clock_pos, int parent_id, int on_off);

/*
  Function:       divoom_light_app_get_user_define_gif_play_time
  Description:    得到自定义动画播放时间
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_user_define_gif_play_time(void *user_data, int index);
/*
  Function:       divoom_light_app_get_cloud_gif_play_time
  Description:    得到云图动画播放时间
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_cloud_gif_play_time(void *user_data, int index);
/*
  Function:       divoom_light_app_get_cloud_gif_play_time
  Description:    得到云图动画播放时间
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_once_gif_play_time(void *user_data, int index);

/*
  Function:       divoom_light_app_system_reset_up
  Description:    系统异常重启，调用显示表盘
  Input:          none
  Return:         NONE
*/

void divoom_light_app_system_reset_up(void);

/*
  Function:       divoom_light_app_check_clock_exist
  Description:    检查表盘是否存在，如果不存在，将获取一个可用的表盘
  Input:          none
  Return:         NONE
*/

int divoom_light_app_check_clock_exist(int clock_id);

/*
  Function:       divoom_light_app_set_divoom_animation_edge_clock_id
  Description:    设置默认边框
  Input:          clock_id
  Return:         NONE
*/

void divoom_light_app_set_divoom_animation_edge_clock_id(int clock_id);

/*
  Function:       divoom_light_app_play_next_clock
  Description:    在表盘页面切换表盘显示
  Input:          dir   1：下一个表盘；0：前一个表盘
  Return:         NONE
*/

void divoom_light_app_play_next_clock(int dir);

/*
  Function:       divoom_light_app_get_gif_change_flag
  Description:    获取APP发的动画属性是否被修改过
  Input:          
  Return:         1 or 0
*/

int divoom_light_app_get_gif_change_flag(void);
/*
  Function:       divoom_light_app_update_image_handle
  Description:    更新APP发的动画属性
  Input:          
  Return:         修改东西
*/

int divoom_light_app_update_image_handle(void *handle);

/*
  Function:       divoom_light_app_update_gif_use_flag
  Description:    获取APP发的动画属性是修改后是不是被应用过
  Input:          flag 小于0是设置，否则是获取
  Return:         1 or 0
*/

int divoom_light_app_update_gif_use_flag(int flag);


/*
  Function:       divoom_light_app_get_cur_author_name
  Description:    获取当前播放图像的作者名称
  Input:          none
  Return:         NONE
*/

char *divoom_light_app_get_cur_author_name(void);

/*
  Function:       divoom_light_app_get_cur_author_name
  Description:    获取当前播放图像的作者ID
  Input:          none
  Return:         NONE
*/

int divoom_light_app_set_cur_author_id(int user_id, int *update_time);   
/*
  Function:       divoom_light_app_get_cur_author_name
  Description:    获取当前播放图像的图像ID
  Input:          none
  Return:         NONE
*/

int divoom_light_app_set_cur_image_id(int image_id);

/*
  Function:       divoom_light_app_get_cur_image_name
  Description:    获取当前播放图像的文件名
  Input:          none
  Return:         NONE
*/

char *divoom_light_app_get_cur_image_name(void);

/*
  Function:       divoom_light_app_get_app_image_name
  Description:    获取当前APP发送播放图像的文件名
  Input:          none
  Return:         NONE
*/

char *divoom_light_app_get_app_image_name(void);


/*
  Function:       divoom_light_app_get_app_author_id
  Description:    获取当前APP发送播放图像的作者ID
  Input:          none
  Return:         ID
*/

int divoom_light_app_get_app_author_id(void);
/*
  Function:       divoom_light_app_app_gif_file_lock_ctrl
  Description:    对APP的文件进行加锁控制
  Input:          none
  Return:         ID
*/
//
void divoom_light_app_app_gif_file_lock_ctrl(int flag);

/* 当前 Server GIF 使用版本化文件 ID；旧版本只在解码句柄关闭后清理。 */
uint32 divoom_light_app_get_server_gif_file_id(void);
void divoom_light_app_cleanup_closed_server_gif_files(void);


/*
  Function:       divoom_light_app_add_clock_id
  Description:    向表盘列表中插入一个表盘
  Input:          clock_id；set_as_current 非 0 且本次为新插入时，设为当前表盘并刷新
  Return:         NONE
*/

void divoom_light_app_add_clock_id(int clock_id, int set_as_current);
/* 下载页读取新表盘等待状态；total_known 为 0 时显示 X/X。 */
int divoom_light_app_get_clock_download_progress(uint32 *finished, uint32 *total,
    uint8 *total_known);
uint32 divoom_light_app_get_clock_download_session(void);
uint32 divoom_light_app_get_clock_config_progress(uint32 session);
int divoom_light_app_clock_download_session_is_current(uint32 session);
void divoom_light_app_post_clock_download_event(uint8 event_id, uint32 session);


/*
  Function:       divoom_light_app_delete_clock_id
  Description:    向表盘列表中移除一个表盘
  Input:          clock_id
  Return:         NONE
*/

void divoom_light_app_delete_clock_id(int clock_id);
void divoom_light_app_delete_clock_id_direct(int clock_id);

/* 以 divoom_defalt_clock_inner_list 的实际数组长度判断是否为只读盘内置表盘。 */
int divoom_light_app_is_default_inner_clock(int clock_id);

/*
  Function:       divoom_light_app_delete_pixel_dial
  Description:    删除像素表盘
  Input:          clock_id
  Return:         NONE
*/
void divoom_light_app_delete_pixel_dial(int clock_id);

/*
  Function:       divoom_light_app_remove_clock_id
  Description:    向表盘列表中移动一个表盘位置
  Input:          clock_id
  Return:         NONE
*/

void divoom_light_app_move_clock_id(int clock_id, int pos);

int divoom_light_app_set_clear_bg_clock(int clock_id, int cur_clock);

/*
  Function:       divoom_light_app_get_current_clock_info
  Description:    获取当前显示的表盘信息
  Input:          clock_id
  Return:         NONE
*/

void *divoom_light_app_get_current_clock_info(void);

/* 增加一个最近使用的表盘。flag=0 只记录，flag=1 置顶。返回 1 表示列表有变化。 */
int divoom_light_app_add_recent_clock(int clock_id, int flag);
/* 用服务器返回的 ID 数组整表替换。没有有效 ID 时保留本机列表。 */
void divoom_light_app_replace_recent_clock_list(const int *ids, int count);
void divoom_light_app_replace_fav_clock_list(const int *ids, int count);
void divoom_light_app_report_add_recent_clock(int clock_id);
void divoom_light_app_report_set_favorite(int clock_id, int favorite_flag);
//flag=0是移除收藏，否则添加收藏
void divoom_light_app_add_remove_fav_clock(int clock_id, int flag);

#define DIVOOM_RECENT_USED_CLOCK_MAX_NUM  20

int divoom_light_app_get_recent_clock_num(void);
int divoom_light_app_copy_recent_clock_list_limited(int *clock_list, int max_count);
void divoom_light_app_remove_recent_clock(int clock_id);

int divoom_light_app_get_fav_clock_num(void);
int divoom_light_app_copy_fav_clock_list_limited(int *clock_list, int max_count);
int divoom_light_app_is_fav_clock(int clock_id);
int divoom_light_app_copy_my_clock_list_num(void);
void divoom_light_app_check_font_task(void *para);
void divoom_light_app_change_default_clock(int lang);
void divoom_light_app_del_config(void);

/* 回收与列表增删共享事务边界；先取此锁，再取表盘管理/FIFO 的锁。 */
void divoom_light_app_clock_list_lock(void);
void divoom_light_app_clock_list_unlock(void);

#if DIVOOM_SHOW_MODE
void divoom_light_app_show_clock_rotate_pause(void);
#endif

#endif
