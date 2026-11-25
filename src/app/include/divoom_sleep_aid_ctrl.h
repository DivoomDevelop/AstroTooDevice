#ifndef _divoom_sleep_aid_ctrl_h_
#define _divoom_sleep_aid_ctrl_h_

#include "divoom_sleep_aid_config.h"
#include "divoom_sys_model_ctrl.h"

/** 申请声道调度（优先级见 divoom_app_schedule.c）。 */
enum
{
	DIVOOM_SLEEP_AID_APP_EVENT_PLAY,       /* 助眠播放页：占用语音轨，获准后再进入 active 菜单 */
	DIVOOM_SLEEP_AID_APP_EVENT_TRY_LISTEN, /* 列表试听（预留） */
	DIVOOM_SLEEP_AID_APP_EVENT_RECORD,     /* 助眠录音：占用语音轨，获准后再启动采集 */
};

/** 发往 `DIVOOM_MODEL_SLEEP_AID` 的同步消息（`msg_type==0`，由 `divoom_sys_model_crl_msg_push` 投递）。 */
enum
{
	DIVOOM_SLEEP_AID_MSG_ENTER_PLAY_VOICE = 0,
	DIVOOM_SLEEP_AID_MSG_RECORD_CTRL,
	DIVOOM_SLEEP_AID_MSG_RECORD_DELETE,
};

/** `divoom_sleep_aid_ctrl_get_play_time_phase` 返回值。 */
enum
{
	DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_NORMAL = 0,
	DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_BEFORE,
	DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_AFTER,
};

/** 进入助眠播放页时传给 `divoom_lvgl_sleep_aid_active_init` 的 user_data（由 ctrl 分配，active exit 释放）。 */
typedef struct _divoom_sleep_aid_active_enter
{
	int voice_type;
	int voice_id;
	int video_type;
} divoom_sleep_aid_active_enter, *Pdivoom_sleep_aid_active_enter;

/**
 * 睡眠辅助控制模块唯一状态载体（实现文件内仅保留一个本结构体的静态实例，不再散落其它全局变量）。
 */
typedef struct _divoom_sleep_aid_ctrl
{
	divoom_sleep_aid_info settings;
	uint8 settings_inited;
	int sel_voice_type;
	int sel_voice_id;
	uint8 record_active;
	uint32 record_time[5];
	uint8 record_times_loaded;
	int record_pending_slot;
	uint8 record_capture_active;
	uint32 record_enc_duration_sec;     /* 0x64 给出的秒；0=尚未结束 */
	uint8 play_session_active;
	uint8 record_schedule_armed;
	uint8 play_phase;
	uint32 play_phase_deadline;
	/** DIY 混合音：走 mix_pcm + snd_pcm，非 local_music tone。 */
	uint8 diy_mix_active;
	/** 助眠 MP3 libmpg123 无缝循环。 */
	uint8 mpg_loop_active;
	char mpg_loop_src_path[256];
	/** 设置页音量调节预览：使用 slider 档位而非 play_phase 音量。 */
	uint8 settings_preview_vol_active;
	/** 设置页音量预览：1=入睡后音量行，0=入睡前音量行。 */
	uint8 settings_preview_vol_is_after;
	int settings_preview_vol_percent;
	/** GetAllList 合并 cfg/语言文件后递增；助眠列表 UI 据此失效 TAB 缓存。 */
	uint8 voice_list_cfg_revision;
	/** 进入助眠播放前保存的系统亮度，退出会话时恢复。 */
	uint8 saved_sys_light;
	uint8 saved_sys_light_valid;
	/** APP AidSleep/Add|Play 试听会话（非 107 播放页）；按键可退出或 VOL 调音量。 */
	uint8 app_aid_sleep_preview_active;
} divoom_sleep_aid_ctrl, *Pdivoom_sleep_aid_ctrl;

/** 模块单例（只读指针；勿在外部修改成员，请通过下方 API）。 */
Pdivoom_sleep_aid_ctrl divoom_sleep_aid_ctrl_instance(void);

/** 注册到 `divoom_sys_model_crl` 与全局调度表；须在 `divoom_sys_model_crl_init` 之后调用。 */
void divoom_sleep_aid_ctrl_init(void);

uint8 divoom_sleep_aid_ctrl_event(Pdivoom_sys_msg_info msg);

uint8 divoom_sleep_aid_ctrl_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);
uint8 divoom_sleep_aid_ctrl_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);
uint8 divoom_sleep_aid_ctrl_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);
uint8 divoom_sleep_aid_ctrl_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

/** UI/业务可调用的本地设置快照（首次访问时从 `DIVOOM_SLEEP_AID_SETTINGS_PATH` 读取）。 */
divoom_sleep_aid_info *divoom_sleep_aid_ctrl_get_settings(void);

/** 将当前设置写入 `DIVOOM_SLEEP_AID_SETTINGS_PATH`；成功返回 0，失败返回 -1。 */
int divoom_sleep_aid_ctrl_settings_save(void);

void divoom_sleep_aid_ctrl_get_selected_voice(int *voice_type, int *voice_id);

int divoom_sleep_aid_ctrl_record_clip_count(void);
/** sorted_row 0=最新；返回槽 0..4（对应 1..5.mp3），无效 -1 */
int divoom_sleep_aid_ctrl_record_sorted_slot(int sorted_row);
uint32 divoom_sleep_aid_ctrl_record_slot_utc(int slot);
int divoom_sleep_aid_ctrl_record_clip_id(int sorted_row);
uint8 divoom_sleep_aid_ctrl_record_busy(void);
/** 编码器结束后的实际秒数；未结束返回 0。 */
uint32 divoom_sleep_aid_ctrl_record_enc_duration_sec(void);

/** 下次读取录音列表前强制从索引文件重新加载（录音保存等写入盘后刷新 UI 时用）。 */
void divoom_sleep_aid_ctrl_record_invalidate_manifest(void);

/**
 * 以下接口把音频起停投递到助眠 job worker 串行执行，调用线程不阻塞；
 * UI 可立即读到的状态（sel_voice_*、settings_preview_vol_*）在投递前已写好。
 * 录音在开始时会 `divoom_app_schedule_push` 申请声道。
 */
void divoom_sleep_aid_ctrl_select_voice(int voice_type, int voice_id);

/** 停止当前助眠音源试听/列表播放，并停掉配套视频。 */
void divoom_sleep_aid_ctrl_stop_play_voice(void);

/** 调节当前试听/播放音源音量（0~100，DIY 走 mix_pcm，其它走 tone）。 */
void divoom_sleep_aid_ctrl_set_voice_vol(int volume);

/** 设置页：以 level(0..100) 试听最近一次睡眠音源；@p is_after_vol 区分入睡前/入睡后音量行。 */
void divoom_sleep_aid_ctrl_settings_volume_preview_start(int vol_level_0_100, uint8 is_after_vol);

/** 设置页音量调节中：更新试听音量（0..100 档位）。 */
void divoom_sleep_aid_ctrl_settings_volume_preview_set_level(int vol_level_0_100);

/** 设置页退出音量调节：停止试听并清除预览状态。 */
void divoom_sleep_aid_ctrl_settings_volume_preview_stop(void);

/** 设置页入睡后音源选择：试听指定 after_music 项（不更新 sel_voice）。 */
void divoom_sleep_aid_ctrl_settings_after_music_preview(int music_idx, int vol_level_0_100);

/** 收藏键 DIVOOM_KEY_MUSIC_FAVORITE：短按未在播放时进入助眠播放；长按未在播放时进入助眠收藏列表(106)。已在 107 播放页时由页面切下一个收藏。 */
void divoom_sleep_aid_ctrl_on_music_favorite_key(void);
/** 长按收藏键：打开助眠菜单并停留在收藏 TAB（非助眠播放 session 时）。 */
void divoom_sleep_aid_ctrl_open_fav_list_menu(void);

/** 107 播放页 ESC：收藏键快捷进入时清栈回表盘；否则走普通 return_pre。 */
void divoom_sleep_aid_ctrl_active_esc_back(void);

/** 助眠播放页 MODE 短按：退出助眠（与 ESC 相同回退路径）。已处理返回 1。 */
uint8 divoom_sleep_aid_ctrl_exit_play_on_music_mode_key(uint8 key_status);

/** 是否已进入助眠播放页会话（106→107 切换时 UI 勿误停音视频）。 */
uint8 divoom_sleep_aid_ctrl_is_play_session_active(void);

/** 当前是否有助眠音源在播（试听/会话/设置预览/PCM 等）；仅 UI 在助眠页时用于 VOL± 路由。 */
uint8 divoom_sleep_aid_ctrl_is_sleep_voice_audible(void);

/** APP AidSleep 试听：占 voice schedule，VOL± 调 before_vol，其它键退出。 */
void divoom_sleep_aid_ctrl_app_preview_begin(void);
void divoom_sleep_aid_ctrl_app_preview_end(void);
uint8 divoom_sleep_aid_ctrl_app_preview_is_active(void);
/** 统一结束 APP 试听：清会话、释放 TRY_LISTEN schedule、停播。 */
void divoom_sleep_aid_ctrl_app_preview_stop(void);
void divoom_sleep_aid_ctrl_app_preview_stop_by_user_key(void);

void divoom_sleep_aid_ctrl_enter_play_voice(int voice_type, int voice_id);

/** 播放页内：启动当前音源试听（音频 + 视频）。 */
void divoom_sleep_aid_ctrl_play_voice(int voice_type, int voice_id);

/** 播放试听；@p video_type 为 cfg 歌词动效分类，<0 时内部查询。 */
void divoom_sleep_aid_ctrl_play_voice_with_video(int voice_type, int voice_id, int video_type);

/** 助眠音源配套视频；@p video_type <0 时内部查询。 */
void sleep_aid_voice_start_play_video(int voice_type, uint32 voice_id, int video_type);

/**
 * flag 为 1 开始录音，为 0 停止并保存。
 * 成功：开始返回新录音将落入的槽索引；停止返回 0。
 * 错误：槽满 -1；已在录音 -2；启动采集失败 -3；保存失败 -4；停止时未在录 -1；调度拒绝声道 -5。
 */
int divoom_sleep_aid_ctrl_record_ctrl(int flag);

/** 按槽索引删除一条本地录音（0 .. clip_count-1），并更新清单与镜像管理。成功 0，失败 -1。 */
int divoom_sleep_aid_ctrl_record_delete_at(int idx);

/**
 * @deprecated 请使用 divoom_audio_policy_tone_play_allowed()
 * 助眠 DIY/mpg 在播时，外部系统 tone_play 应丢弃（不抢 I2S）。
 */
int divoom_sleep_aid_ctrl_should_suppress_system_tone(void);

/**
 * @deprecated 请使用 divoom_audio_policy_keep_jl_after_tone()
 * 助眠 DIY/mpg123 在播时，提示音结束勿把 JL TONE 静音。
 */
int divoom_sleep_aid_ctrl_should_keep_jl_after_tone(void);

/** 退出播放页：取消 `DIVOOM_SLEEP_AID_APP_EVENT_PLAY` 调度并停止音视频。 */
void divoom_sleep_aid_ctrl_cancel_play_voice(void);
void divoom_sleep_aid_ctrl_cancel_play_voice_wait(unsigned int timeout_ms);

/** 主循环周期调用（与 `divoom_sleep_main` 同级，约 10s 一次）。 */
void divoom_sleep_aid_ctrl_main_loop(void);

/**
 * 助眠播放页计时阶段：未在播放会话为 NORMAL；会话中为 BEFORE/AFTER（见 `play_phase`）。
 * @return `DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_*`
 */
int divoom_sleep_aid_ctrl_get_play_time_phase(void);

/** 当前助眠阶段剩余秒数；无倒计时返回 -1。 */
int divoom_sleep_aid_ctrl_get_phase_remain_sec(void);

/** 播放页调整入睡前定时（分钟，0=永不），保存设置并重置当前阶段倒计时。 */
void divoom_sleep_aid_ctrl_set_before_time_minutes(int minutes);

/** 播放页调整入睡后定时（分钟），保存设置并重置当前阶段倒计时。 */
void divoom_sleep_aid_ctrl_set_after_time_minutes(int minutes);

/** 按当前播放阶段写入 before/after 定时并重置倒计时。 */
void divoom_sleep_aid_ctrl_set_phase_timer_minutes(int minutes);

#endif
