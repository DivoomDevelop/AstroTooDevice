#ifndef _divoom_white_noise_player_h_
#define _divoom_white_noise_player_h_

/**
 * 白噪音播放器 - 供列表页按宫格+睡眠定时播放
 * 根据宫格索引播放对应 MP3 提示音（tinplayer），根据睡眠定时决定播放时长（0=一直播放）
 */

enum
{
	DIVOOM_WHITE_NOISE_SCHEDULE_PLAY_ACTIVE = 1,
};

/** 启动播放：宫格索引 0~8，睡眠定时分钟数 0=关闭定时一直播，>0 则播满后自动停 */
void divoom_white_noise_player_start(int grid_index, int sleep_timer_minutes);

/** 停止播放 */
void divoom_white_noise_player_stop(void);

/** 是否正在播放 */
int divoom_white_noise_player_is_playing(void);

/** 设置当前睡眠定时（仅保存，供下次 start 使用）；0=关闭定时 */
void divoom_white_noise_player_set_sleep_timer_minutes(int minutes);

/** 获取当前保存的睡眠定时分钟数 */
int divoom_white_noise_player_get_sleep_timer_minutes(void);

/** 切换收藏：若 index 不在收藏列表则加入，否则移除。宫格索引 0~8。返回 0 成功，1 失败 */
int divoom_white_noise_player_toggle_favorite(int index);

/** 判断宫格索引是否已收藏。返回 1 已收藏，0 未收藏 */
int divoom_white_noise_player_is_favorite(int index);

/** 收藏列表：返回当前收藏数量（0~9） */
int divoom_white_noise_player_get_favorite_count(void);

/** 收藏列表：返回列表中第 list_index 项对应的音效宫格索引（0~8），list_index 从 0 起；越界返回 0 */
int divoom_white_noise_player_get_favorite_at(int list_index);

void divoom_white_noise_player_play_current_effect(void);

int divoom_white_noise_player_get_current_effect(void);

void divoom_white_noise_player_init(void);

/**
 * 后台助眠播放时：若当前不在白噪音菜单内且正在播助眠音，消费一次「播放/暂停」键并停止助眠。
 * @return 1 已处理并应跳过后续音乐键逻辑；0 未处理
 */
uint8 divoom_white_noise_player_try_consume_play_key(uint8 key_value, uint8 key_status);

uint8 divoom_white_noise_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);
uint8 divoom_white_noise_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);
uint8 divoom_white_noise_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);
uint8 divoom_white_noise_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

#endif
