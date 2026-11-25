#ifndef _divoom_tf_music_h_

#define _divoom_tf_music_h_

enum{
    DIVOOM_FT_MUSIC_MODE_LIST,  //列表顺序播放
    DIVOOM_FT_MUSIC_MODE_ONE,   //单曲循环
    DIVOOM_FT_MUSIC_MODE_RANDOM,    //随机播放
};

#define DIVOOM_TF_MUSIC_MAX_ROOT    	   	100
#define DIVOOM_TF_MUSIC_ROOT_NAME_LEN   	64
#define DIVOOM_TF_MUSIC_PATH_LEVEL	 		3

/* 1=显示 TF 音乐页收藏相关 UI；0=隐藏（主菜单/文件夹页左侧「收藏」、列表心形、底部收藏键） */
#ifndef DIVOOM_TF_MUSIC_FAVORITES_UI
#define DIVOOM_TF_MUSIC_FAVORITES_UI  0
#endif

#if DIVOOM_TF_MUSIC_FAVORITES_UI
#define DIVOOM_TF_MUSIC_MENU_SETTING_COUNT  4
#define DIVOOM_TF_MUSIC_FOLDER_LEFT_BASE    2
#elif defined(DIVOOM_TIMES_VIBE)
#define DIVOOM_TF_MUSIC_MENU_SETTING_COUNT  4
#define DIVOOM_TF_MUSIC_FOLDER_LEFT_BASE    1
#else
#define DIVOOM_TF_MUSIC_MENU_SETTING_COUNT  3
#define DIVOOM_TF_MUSIC_FOLDER_LEFT_BASE    1
#endif

typedef struct _divoom_tf_music_item
{
    int id;
    int like;
    char *music_name;
    char *root_name;   /* 从路径解析或从 DB 读取 */
}divoom_tf_music_item, *Pdivoom_tf_music_item;


/*********************************************************************
  Function:        divoom_tf_music_start
  Description:     启动播放
  Input:           none
  Return:          none
**********************************************************************/

void divoom_tf_music_start(void);

#ifdef DIVOOM_TIMES_VIBE
/* 仅同步歌单，不发送 music_id */
void divoom_tf_music_prepare(void);
int divoom_tf_music_root_is_mount_label(const char *seg);
/* JL 确认 TF 模式后再调用，发送当前曲目 music_id */
void divoom_tf_music_play_saved(void);
/* 0x5e 后首次播放：restore 已设 pcur 时仍重发 0x0e */
void divoom_tf_music_play_saved_force_replay(void);
void divoom_tf_music_clear_songlist_force_replay(void);
#endif

/*********************************************************************
  Function:        divoom_tf_music_set_mode
  Description:     设置播放模式
  Input:           mode
  Return:          none
**********************************************************************/


void divoom_tf_music_set_mode(int mode);

/*********************************************************************
  Function:        divoom_tf_music_set_order
  Description:     设置播放偏好
  Input:           mode
  Return:          none
**********************************************************************/


void divoom_tf_music_set_order(int mode);

/*********************************************************************
  Function:        divoom_tf_music_set_like_flag
  Description:     设置歌曲的收藏偏好
  Input:           id
                   flag
  Return:          none
**********************************************************************/


void divoom_tf_music_set_like_flag(int id, int like_flag);

/*********************************************************************
  Function:        divoom_tf_music_stop
  Description:     停止本地播放状态
  Input:           none
  Return:          none
**********************************************************************/

void divoom_tf_music_stop(void);

/*********************************************************************
  Function:        divoom_tf_music_get_music_status
  Description:     获取本地播放状态
  Input:           none
  Return:          1:播放；0：暂停
**********************************************************************/

int divoom_tf_music_get_music_status(void);

/*********************************************************************
  Function:        divoom_tf_music_music_play_ctrl
  Description:     本地播放与暂停控制
  Input:           play  1:播放；0：暂停
  Return:          none
**********************************************************************/

void divoom_tf_music_music_play_ctrl(uint8 play);

/*********************************************************************
  Function:        divoom_tf_music_set_volume
  Description:     设置 TF 卡音乐音量
  Input:           volume  0-100
  Return:          none
**********************************************************************/

void divoom_tf_music_set_volume(uint8 volume);

/*********************************************************************
  Function:        divoom_tf_music_get_volume
  Description:     获取 TF 卡音乐当前音量（0-100）
  Input:           none
  Return:          当前音量
**********************************************************************/

uint8 divoom_tf_music_get_volume(void);

/*********************************************************************
  Function:        divoom_tf_music_music_play_prev
  Description:     本地播放下一首歌曲
  Input:           none
  Return:          none
**********************************************************************/

void divoom_tf_music_music_play_next(void);

/*********************************************************************
  Function:        divoom_tf_music_music_play_prev
  Description:     本地播放上一首歌曲
  Input:           none
  Return:          none
**********************************************************************/

void divoom_tf_music_music_play_prev(void);

/*********************************************************************
  Function:        divoom_tf_music_get_playing_music_id
  Description:     获取本地播放歌曲ID
  Input:           none
  Return:          ID
**********************************************************************/

int divoom_tf_music_get_playing_music_id(void);

/*********************************************************************
  Function:        divoom_tf_music_get_playing_music_info
  Description:     获取本地播放歌曲信息
  Input:           none
  Return:          file_name
**********************************************************************/

Pdivoom_tf_music_item divoom_tf_music_get_playing_music_info(void);

/*********************************************************************
  Function:        divoom_tf_music_get_disp_name
  Description:     路径转显示名（最后一段文件名）。返回指向 path 内部的指针，勿 free
  Input:           path  完整路径，可为 NULL
  Return:          显示名；空路径返回 ""
**********************************************************************/
char *divoom_tf_music_get_disp_name(const char *path);

/*********************************************************************
  Function:        divoom_tf_music_get_playing_disp_name
  Description:     当前播放曲目的显示名（basename）。无当前曲返回 ""
  Input:           none
  Return:          内部缓冲，下次调用可能被覆盖
**********************************************************************/
char *divoom_tf_music_get_playing_disp_name(void);

/*********************************************************************
  Function:        divoom_tf_music_set_playing_music_id
  Description:     启动本地播放歌曲ID
  Input:           none
  Return:          ID
**********************************************************************/

void divoom_tf_music_set_playing_music_id(int play_order, int music_id);

/* 文件夹页 left_selected_index → play_order（与 divoom_lvgl_tf_music_menu 一致） */
int divoom_tf_music_folder_left_to_play_order(int left_selected_index);
int divoom_tf_music_folder_play_order_to_left(int play_order);
int divoom_tf_music_get_play_order(void);

/*********************************************************************
  Function:        divoom_tf_music_restore_saved_playback
  Description:     从配置恢复 play_order / 当前曲目到内存（不发 JL 播放命令）
  Input:           none
  Return:          none
**********************************************************************/

void divoom_tf_music_restore_saved_playback(void);
void divoom_tf_music_sync_play_status_ui(void);

/*********************************************************************
  Function:        divoom_tf_music_add_music_item
  Description:     开机扫描音乐，先查询，如果不存在就插入
  Input:           file_name  
  Return:          none
**********************************************************************/

void divoom_tf_music_add_music_item(char *file_name);


/*********************************************************************
  Function:        divoom_tf_music_get_all_music_info
  Description:     获取音乐列表
  Input:           total
                   flag  1:收藏列表； 0：全部列表
  Return:          音乐列表（深拷贝 music_name，须 divoom_tf_music_free_music_info_list 释放）
**********************************************************************/

Pdivoom_tf_music_item divoom_tf_music_get_all_music_info(int *total, int flag);
void divoom_tf_music_free_music_info_list(Pdivoom_tf_music_item list, int count);

/*********************************************************************
  Function:        divoom_tf_music_get_like_music_count
  Description:     获取收藏夹音乐数量
  Return:          收藏夹音乐数量，未初始化返回 0
**********************************************************************/
int divoom_tf_music_get_like_music_count(void);

/*********************************************************************
  Function:        divoom_tf_music_get_root_count
  Description:     获取根目录数量（供 UI 左侧栏用）
  Return:          根目录个数，未初始化返回 0
**********************************************************************/
int divoom_tf_music_get_root_count(void);

/*********************************************************************
  Function:        divoom_tf_music_get_root_name
  Description:     获取第 index 个根目录名
  Input:           index   0 ~ root_count-1
                   buf     输出缓冲区
                   buf_size 缓冲区大小
  Return:          1 成功，0 失败
**********************************************************************/
int divoom_tf_music_get_root_name(int index, char *buf, int buf_size);

/*********************************************************************
  Function:        divoom_tf_music_get_cur_play_root_name
  Description:     获取当前播放所在入口名（供 UI 显示）
                    play_order 0=全部 1=收藏 2+=根目录 root_names[play_order-2]
  Input:           	void
  Return:          
**********************************************************************/
char *divoom_tf_music_get_cur_play_root_name(void);

/*********************************************************************
  Function:        divoom_tf_music_get_root_list
  Description:     获取根目录名列表，将指针填入调用方提供的数组（指向内部，勿 free）
  Input:           out_list  输出数组，out_list[i] 指向第 i 个根目录名
                   max_count 数组最大容量
  Return:          实际根目录数量；0 表示未初始化或无数据
**********************************************************************/
int divoom_tf_music_get_root_list(char **out_list, int max_count);

/*********************************************************************
  Function:        divoom_tf_music_get_music_info_by_root
  Description:     按根目录名获取该根下的歌曲列表（供 UI 右侧列表用）
  Input:           root_name  根目录名
                   total      输出该根下歌曲数量
  Return:          歌曲数组（深拷贝 music_name），须 divoom_tf_music_free_music_info_list 释放
**********************************************************************/
Pdivoom_tf_music_item divoom_tf_music_get_music_info_by_root(const char *root_name, int *total);

/*********************************************************************
  Function:        divoom_tf_music_get_active_queue
  Description:     
  Input:           
  Return:          
**********************************************************************/
void *divoom_tf_music_get_active_queue(void);

void divoom_tf_music_sync_set_reason(const char *reason);
void divoom_tf_music_sync_begin(const uint8 *cmd_data, int cmd_len);
int divoom_tf_music_sync_store_item(const uint8 *cmd_data, int cmd_len);
int divoom_tf_music_sync_is_complete(void);
void divoom_tf_music_sync_commit(void);
Pdivoom_tf_music_item divoom_tf_music_get_item_by_id(int music_id);
int divoom_tf_music_find_id_by_path(const char *music_path);
int divoom_tf_music_alarm_resolve_music_id(const char *leave_url);
int divoom_tf_music_get_first_music_id(void);

#ifdef DIVOOM_TIMES_VIBE
int divoom_tf_music_play_music_id_ex(int music_id, uint8 volume, uint8 voice_owned);
int divoom_tf_music_play_music_id(int music_id, uint8 volume);
void divoom_tf_music_set_music_id(int music_id);
void divoom_tf_music_pause_jl_play(void);
#endif

void divoom_tf_music_get_music_list_callback(void *handle, int id, int like, char *file_name, char *root_name);
/* jl_err：JL 0x54 第二字节。0/1=正常切下一首，5=解码失败按方向跳过，其它不自动切 */
int divoom_tf_music_play_finished_send_play_next(int jl_err);

#ifdef DIVOOM_TIMES_VIBE
void divoom_tf_music_sync_prepare(void);
void divoom_tf_music_send_get_music_count(void);
void divoom_tf_music_bootstrap_at_boot(void);
#if DIVOOM_SHOW_TF_NAND
int divoom_tf_music_scan_root_dir(const char *root);
/* 正式 tf_music.crc 不在时，从文件服务器 AstroToo/showmode/tf_music/ 补齐曲目。 */
void divoom_show_tf_music_fetch_if_needed(void);
/* 恢复出厂格式化前调用：拦住补歌并等到线程退出。 */
void divoom_show_tf_music_fetch_cancel(void);
#endif
/* UI 手动触发：T113→JL 0x4d，请求 JL 重新扫描并创建歌单文件 */
void divoom_tf_music_send_scan_music_list(void);

#define DIVOOM_TF_MUSIC_SONGLIST_REFRESH_DONE  0
#define DIVOOM_TF_MUSIC_SONGLIST_REFRESH_FAIL  -1

typedef void (*divoom_tf_music_songlist_refresh_cb)(void *user, int event);

void divoom_tf_music_set_songlist_refresh_callback(divoom_tf_music_songlist_refresh_cb cb,
						   void *user);
void divoom_tf_music_poll_songlist_refresh(void);
int divoom_tf_music_is_songlist_refreshing(void);
void divoom_tf_music_on_songlist_info_replied(void);
void divoom_tf_music_on_songlist_transfer_begin(void);
void divoom_tf_music_on_scan_music_list_status(uint8_t status);
void divoom_tf_music_on_tf_hotplug(int inserted);
/** 插 TF 后等 JL 歌单：1=暂不响应 0x5e 发 0x0e（同卡超时后返回 0） */
uint8 divoom_tf_music_defer_jl_play_for_songlist(void);
/** 0x5e 路径因等歌单被 defer 时调用，用于启动同卡超时重试 */
void divoom_tf_music_on_jl_play_deferred_for_songlist(void);
/** defer 重试任务：1=仍需走 0x5e 补播 */
uint8 divoom_tf_music_take_jl_5e_pending_play(void);
/** TF 热插拔插入后待刷新 UI 列表 */
int divoom_tf_music_ui_hotplug_reload_pending(void);
int divoom_tf_music_take_ui_hotplug_reload(void);
#endif
#ifdef DIVOOM_SIMULATOR
void divoom_tf_music_sim_request_catalog(void);
int divoom_tf_music_sim_is_scanning(void);

#endif


#endif

