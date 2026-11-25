#ifndef _divoom_music_ctrl_h_
#define _divoom_music_ctrl_h_

/* 保留传输三态；playing 布尔值不能区分暂停与终止。 */
typedef enum
{
    DIVOOM_MUSIC_TRANSPORT_STOPPED = 0,
    DIVOOM_MUSIC_TRANSPORT_PLAYING,
    DIVOOM_MUSIC_TRANSPORT_PAUSED
} divoom_music_transport_state_t;
/* 只读状态快照，无 UART 查询或音频控制。 */
uint8 divoom_music_ctrl_get_transport_state(void);
/* 状态来源补充明确的 STOP/PAUSE，不发送播放命令。 */
void divoom_music_ctrl_post_transport_state(uint8 state);

#ifndef DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
#define DIVOOM_MUSIC_CTRL_LINEIN_ENABLE  0  /* AstroToo 无 LINEIN；TimesVibe 编译选项置 1 */
#endif

#ifndef DIVOOM_MUSIC_CTRL_UAC_ENABLE
#define DIVOOM_MUSIC_CTRL_UAC_ENABLE  1  /* AstroToo 有 UAC */
#endif

/* 顶栏音源 OSD 图标槽位数：BT + TF + (LINEIN) + (UAC) */
#define DIVOOM_TOP_AUDIO_MENU_SLOT_MAX \
	(2 + DIVOOM_MUSIC_CTRL_LINEIN_ENABLE + DIVOOM_MUSIC_CTRL_UAC_ENABLE)

#define DIVOOM_PAUSE_BT_MUSIC_DELAY_TIME	1000

/*********************************************************************
 * 音乐模式管理模块
 * 
 * 管理多种音乐模式的互斥切换:
 *   1. 蓝牙音乐 (BT) - 通过管道与蓝牙管理进程通信
 *   2. TF卡音乐 (LOCAL) - 本地文件播放
 *   3. Line-In - ALSA音频直接输入输出
 *   4. UAC - USB音频类，从PC/手机USB输入播放
 * 
 * 同一时刻只能有一种模式在工作
 **********************************************************************/
/*
typedef enum
{
    DIVOOM_WORK_MODE_BT = 0,    			//SPP 命令蓝牙工作模式
    DIVOOM_WORK_MODE_FM,    			//SPP 命令FM工作模式
    DIVOOM_WORK_MODE_LINEIN,    		//SPP 命令LineIn工作模式
    DIVOOM_WORK_MODE_SD,    			//SPP 命令T 卡工作模式
    DIVOOM_WORK_MODE_USBHOST,    	//SPP 命令USB HOST工作模式
    DIVOOM_WORK_MODE_RECORD,    		//SPP 命令录音工作模式
	DIVOOM_WORK_MODE_RECORDPLAY, 	//SPP 命令录音播放工作模式
	DIVOOM_WORK_MODE_UAC, 			//SPP 命令uac工作模式
	DIVOOM_WORK_MODE_PHONE,          //SPP 命令phone
	DIVOOM_WORK_MODE_DIVOOM_SHOW,    //SPP 命令DIVOOM_SHOW模式
	DIVOOM_WORK_MODE_ALARM_SET,      //SPP 命令闹钟设置
	DIVOOM_WORK_MODE_GAME,           //SPP 命令游戏模式
	DIVOOM_WORK_MODE_SLEEP,         // 睡眠模式
	DIVOOM_WORK_MODE_FILE_UPDATE,   // 文件更新

	DIVOOM_WORK_MODE_END,           //SPP 命令以上是跟APP通讯的部分

	DIVOOM_WORK_MODE_DISP_SD_ERR,
	DIVOOM_WORK_MODE_MIX_MUSIC,		 //多音频文件播放
	DIVOOM_WORK_MODE_NULL = 0Xff,           //SPP 空模式

}DIVOOM_WORK_MODE_INFO;
*/

// 音乐模式枚举
enum{
    DIVOOM_MUSIC_CTRL_MODE_BT,      // 蓝牙模式
	DIVOOM_MUSIC_CTRL_MODE_NET_FM,  // 网络电台模式
    DIVOOM_MUSIC_CTRL_MODE_LINEIN,  // Line-In模式
    DIVOOM_MUSIC_CTRL_MODE_LOCAL,   // 本地TF卡模式
    DIVOOM_MUSIC_CTRL_MODE_UAC = 7,     // UAC(USB音频)模式
	DIVOOM_MUSIC_CTRL_MODE_SLEEP = 12, //睡眠模式
    DIVOOM_MUSIC_CTRL_MODE_MAX,      // 模式数量
};

enum
{
    DIVOOM_MUSIC_SCHEDULE_ACTIVE,
};

uint8 divoom_music_ctrl_schedule_pause_callback(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);
uint8 divoom_music_ctrl_schedule_play_callback(uint8 event_id, void *user_data, uint32 resouce_type);
uint8 divoom_music_ctrl_schedule_destory_callback(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);
uint8 divoom_music_ctrl_schedule_key_callback(uint8 key_value, uint8 key_status, uint32 resouce_type);

int divoom_music_ctrl_is_mode_supported(uint8 mode);

/*********************************************************************
  Function:        divoom_music_ctrl_init
  Description:     初始化音乐控制模块
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_init(void);

/*********************************************************************
  Function:        divoom_music_ctrl_deinit
  Description:     反初始化音乐控制模块
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_deinit(void);

/*********************************************************************
  Function:        divoom_music_ctrl_set_next_mode
  Description:     切换到下一个音乐模式；若下一模式当前不支持则顺延，
                   直到找到当前支持的模式（蓝牙始终支持）。
  Input:           none
  Return:          NONE
**********************************************************************/
void divoom_music_ctrl_set_next_mode(void);

/*********************************************************************
  Function:        divoom_music_ctrl_set_mode
  Description:     设置音乐模式（T113 主动切换，TimesVibe 会向 JL 发 0x39 SET）
                   切换时会自动停止当前模式并启动新模式
  Input:           mode - 目标模式
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_set_mode(uint8 mode);
/* 电台worker只传意图代号；控制器在串行切源提交边界校验后才接入/释放音源。 */
void divoom_music_ctrl_radio_source_request(int32 generation, uint8 active);

#ifdef DIVOOM_TIMES_VIBE
/*********************************************************************
  Function:        divoom_music_ctrl_apply_mode
  Description:     仅同步本地模式，不向 JL 发 0x39（JL 已通知或热插拔跟随）
  Input:           mode - 目标模式
  Return:          NONE
**********************************************************************/
void divoom_music_ctrl_apply_mode(uint8 mode);

/*********************************************************************
  Function:        divoom_music_ctrl_advance_mode_local
  Description:     本地切到下一支持模式，不向 JL 发 0x39（拔卡/拔线等）
  Input:           none
  Return:          NONE
**********************************************************************/
void divoom_music_ctrl_advance_mode_local(void);
#endif

/*********************************************************************
  Function:        divoom_music_ctrl_get_mode
  Description:     获取当前音乐模式
  Input:           none
  Return:          当前模式 
**********************************************************************/

uint8 divoom_music_ctrl_get_mode(void);

/*********************************************************************
  Function:        divoom_music_ctrl_stop_play
  Description:     停止当前模式播放（不切换模式）
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_stop_play(void);

/*********************************************************************
  Function:        divoom_music_ctrl_post_play_status
  Description:     JL上报播放状态
  Input:           play_status  1:播放 0:暂停
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_post_play_status(uint8 play_status);

#ifdef DIVOOM_TIMES_VIBE
/** UAC：无条件发 JL toggle(0x30 payload=2)，不走 set_play 的 sync+比较 */
void divoom_music_ctrl_toggle_play_uac(void);
typedef void (*divoom_music_ctrl_play_status_notify_fn)(void);
void divoom_music_ctrl_set_play_status_notify(divoom_music_ctrl_play_status_notify_fn fn);
#endif

/*********************************************************************
  Function:        divoom_music_ctrl_set_play
  Description:     设置播放/暂停状态
  Input:           play_status  1:播放 0:暂停
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_set_play(uint8 play_status);
/* 按键/音源页 PP：发 0x30=2，不查 0x32。闹钟等 0/1 意图仍用 set_play。 */
void divoom_music_ctrl_toggle_play(void);
#if !defined(DIVOOM_OPENSOURCE_SWITCH) && defined(CONFIG_DIVOOM_AI_VOICE) && DIVOOM_AI_VOICE_DIAL_TEST_ENABLE
/* Private dial-test adapter: explicit pause and cancel saved music restoration. */
int divoom_music_ctrl_pause_for_ai_voice(void);
#endif

/*********************************************************************
  Function:        divoom_music_ctrl_pause_output_only
  Description:     仅暂停出声（JL/本地），不触发语音 schedule 释放；
                   供闹钟等在 schedule_cancel 回调里停歌，避免重入死锁。
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_music_ctrl_pause_output_only(void);

/** 仅恢复出声（JL/本地），不触发语音 schedule；与 pause_output_only 对称 */
void divoom_music_ctrl_resume_output_only(void);

/** 闹钟蓝牙铃声：0x62 切 BT+设闹钟音量，再 0x30 toggle 恢复播放；不写 music_volume_bt 配置 */
int divoom_music_ctrl_alarm_play_bt(uint8 alarm_vol_ui);

/** 提示音前暂停当前音乐（走 set_play_internal；TimesVibe 下 BT/TF/UAC 等统一 JL UART） */
void divoom_music_ctrl_pause_for_tone(void);
/** 提示音结束后恢复（同上；并兼容 schedule 语音轨暂存） */
void divoom_music_ctrl_resume_after_tone(void);

/*********************************************************************
  Function:        divoom_music_ctrl_set_bt_remote_play_state
  Description:     同步手机端 AVRCP 播放状态到本机（仅更新 playing，不向蓝牙进程回发指令）
  Input:           play_status  1:播放 0:暂停/停止
  Return:          NONE
**********************************************************************/

void divoom_music_ctrl_set_bt_remote_play_state(uint8 play_status);

/*********************************************************************
  Function:        divoom_music_ctrl_play_next
  Description:     播放下一首（蓝牙和TF卡模式有效）
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_play_next(void);

/*********************************************************************
  Function:        divoom_music_ctrl_play_pre
  Description:     播放上一首（蓝牙和TF卡模式有效）
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_play_pre(void);

/*********************************************************************
  Function:        divoom_music_ctrl_set_volume
  Description:     设置音量
  Input:           volume  0-16（兼容旧版 0-100）
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_set_volume(uint8 volume);

/*********************************************************************
  Function:        divoom_music_ctrl_get_volume
  Description:     获取当前音量
  Input:           none
  Return:          当前音量 0-100，未初始化返回 0
**********************************************************************/

uint8 divoom_music_ctrl_get_volume(void);

/*********************************************************************
  Function:        divoom_music_ctrl_save_bt_volume
  Description:     仅保存蓝牙音量到配置（如手机端下发的音量），不改变当前播放音量
  Input:           volume  0-16（兼容旧版 0-100）
  Return:          NONE
**********************************************************************/
void divoom_music_ctrl_save_bt_volume(uint8 volume, uint8 flag);

/*********************************************************************
  Function:        divoom_music_ctrl_is_playing
  Description:     获取当前是否在播放
  Input:           none
  Return:          1:播放中 0:未播放 
**********************************************************************/

uint8 divoom_music_ctrl_is_playing(void);

#ifdef DIVOOM_TIMES_VIBE
uint8 divoom_music_ctrl_is_ready(void);
/* JL TF 通路可发 music_id(0x0e)：0x5e 握手完成、已有当前曲目、或用户已恢复播放 */
uint8 divoom_music_ctrl_tf_jl_ready_for_play(void);
void divoom_music_ctrl_jl_tf_mark_ready(void);
/* 先切 JL 到 TF 并确认，再发 music_id（闹钟/预览等）
 * 返回：-1 失败；0 已发 0x0e 起播；1 等待 JL 切模式/0x5e
 * voice_owned=1：调用方已持有语音轨，切模式时不再 schedule_push */
int divoom_music_ctrl_play_tf_track_id_ex(int music_id, uint8 volume, uint8 voice_owned);
int divoom_music_ctrl_play_tf_track_id(int music_id, uint8 volume);
int divoom_music_ctrl_sync_play_status_from_jl(void);
int divoom_music_ctrl_sync_mode_from_jl(void);
void divoom_music_ctrl_on_jl_work_mode_notify(uint8 jl_work_mode);
void divoom_music_ctrl_on_jl_sd_music_ready(void);
uint8 divoom_music_ctrl_is_phone_call(void);
uint8 divoom_music_ctrl_get_hfp_call_level(void);
uint8 divoom_music_ctrl_get_hfp_call_volume_ui(void);
void divoom_music_ctrl_set_hfp_call_volume(uint8 level);
void divoom_music_ctrl_on_hfp_call_volume_notify(uint8 level);
/* JL 0x07 SD_PLAY_INFO：cur/total 单位秒（uint16，0xFFFF=未就绪） */
void divoom_music_ctrl_on_jl_sd_play_info(uint32 cur_time_sec, uint32 totle_time_sec, uint8 status);
void divoom_music_ctrl_on_tf_hotplug(int inserted);
/* tf_music 拔卡拆除结束后调用；只置 pending，真正 apply 在 music_jl_def */
void divoom_music_ctrl_on_tf_unplug_teardown_done(void);
void divoom_music_ctrl_on_linein_hotplug(int inserted);
/** 进入/退出 JL 睡眠 work_mode（0x39 SET）；退出时恢复进入前模式 */
void divoom_music_ctrl_enter_jl_sleep_mode(void);
void divoom_music_ctrl_exit_jl_sleep_mode(void);
int divoom_music_ctrl_is_jl_sleep_mode_active(void);
#endif

/* 仅更新 playing 标志供 UI 使用，不向 BtCore/本地解码发控制命令 */
void divoom_music_ctrl_set_playing_flag(uint8 play_status);

/*********************************************************************
  Function:        divoom_music_ctrl_restore_last_mode
  Description:     恢复到上一个模式
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_restore_last_mode(void);

typedef struct _divoom_music_ctrl_tf_play_info
{
	uint32 cur_time;
	uint32 totle_time;
	uint8 status;
} divoom_music_ctrl_tf_play_info, *Pdivoom_music_ctrl_tf_play_info;

Pdivoom_music_ctrl_tf_play_info divoom_music_ctrl_get_tf_info(void);

void divoom_music_ctrl_poll_tf_play_info(void);

#endif

