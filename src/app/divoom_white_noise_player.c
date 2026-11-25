/**
 * 白噪音播放器：根据列表页宫格选择播放对应 MP3 提示音（tinplayer），按睡眠定时控制时长（0=一直播放）
 * 使用 divoom_local_music_ctrl_tone_play / stop_tone，不再使用 divoom_white_noise_* 及 PCM 混音
 */
//保存配置（当前选择的提示音、定时时长）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "divoom.h"
#include "divoom_white_noise_player.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_voice_read.h"
#include "divoom_menu_control.h"
#include "divoom_user_key.h"

/* 列表页 9 宫格对应 Sleep 类 MP3 的 voice_id（与 divoom_voice_read 一致）
 * 0篝火 1深涟 2雾流 3水波纹 4光影 5冥想 6日出 7雨声 8溪流
 */

//"篝火",
//"海浪",
//"涟漪",
//"水波纹",
//"光影",
//"冥想",
//"光影流动",
//"雨滴",
//"溪流",


#define GRID_NUM  9
//static const uint32_t grid_to_voice_id[GRID_NUM] = {

const char *divoom_lvgl_white_noise_effect_id_name[] =
{
	"/usr/share/divoom_app/white_noise_movie/篝火.mp3",
	"/usr/share/divoom_app/white_noise_movie/海浪.mp3",
	"/usr/share/divoom_app/white_noise_movie/涟漪.mp3",
	"/usr/share/divoom_app/white_noise_movie/水波纹.mp3",
	"/usr/share/divoom_app/white_noise_movie/光影.mp3",
	"/usr/share/divoom_app/white_noise_movie/冷色冥想.mp3",
	"/usr/share/divoom_app/white_noise_movie/光影流动.mp3",
	"/usr/share/divoom_app/white_noise_movie/雨滴.mp3",
	"/usr/share/divoom_app/white_noise_movie/溪流.mp3",
};

#define TONE_VOLUME_DEFAULT  80
/* 关闭定时时按“一直播放”处理，使用较大时长（秒），由 tinplayer 循环播 */
#define PLAY_FOREVER_SEC     (24 * 3600)


typedef struct _divoom_white_noise_player_ctrl{
	int cur_play_effect;   //当前播放的提示音index
    int cur_play_effect_volume; //当前播放的提示音音量
	int sleep_timer_minutes;    // 当前定时（分钟），0 表示关闭
	int favorite_effects[GRID_NUM];    // 收藏的提示音index队列
    int favorite_effects_count;    // 收藏的提示音index队列长度
	uint8 wn_session_active;         /* 当前助眠会话由本模块启动，用于与系统其它提示音区分 */

}divoom_white_noise_player_ctrl, *Pdivoom_white_noise_player_ctrl;

static Pdivoom_white_noise_player_ctrl pdivoom_white_noise_player = NULL;

void divoom_white_noise_player_save_cfg(void);

static uint32 wn_schedule_voice_push(void)
{
	divoom_app_schedule_item_info info = {0};

	info.model_id = DIVOOM_MODEL_APP_WHITE_NOISE;
	info.event_id = DIVOOM_WHITE_NOISE_SCHEDULE_PLAY_ACTIVE;
	info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	return divoom_app_schedule_push(&info);
}

/** 实际 tone 播放；抢占恢复时仍走 divoom_white_noise_app_schedule_play */
static void divoom_white_noise_player_play_tone_impl(void)
{
	char path[256];
	int play_time_sec;
	int grid_index;

	if (!pdivoom_white_noise_player)
		return;
	grid_index = pdivoom_white_noise_player->cur_play_effect;
	if (grid_index < 0 || grid_index >= GRID_NUM)
		return;
//	if (divoom_voice_read_get_filename(grid_to_voice_id[grid_index], path) == 0)
//		return;

	if (pdivoom_white_noise_player->sleep_timer_minutes > 0)
		play_time_sec = pdivoom_white_noise_player->sleep_timer_minutes * 60;
	else
		play_time_sec = PLAY_FOREVER_SEC;

	pdivoom_white_noise_player->wn_session_active = 1;
	divoom_print("wft test white noise play : %s, vol: %d\n", divoom_lvgl_white_noise_effect_id_name[grid_index], pdivoom_white_noise_player->cur_play_effect_volume);

	
	divoom_local_music_ctrl_tone_play(divoom_lvgl_white_noise_effect_id_name[grid_index], pdivoom_white_noise_player->cur_play_effect_volume, NULL, play_time_sec);
}

static int divoom_white_noise_player_is_in_wn_menu(void)
{
	int id = divoom_menu_control_get_active_menu_id();
	return (id == DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_PLAY_MAIN
		|| id == DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_LIST
		|| id == DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_MENU);
}

void divoom_white_noise_player_start(int grid_index, int sleep_timer_minutes)
{
	char path[256];
	uint32 ret;

	if (grid_index < 0 || grid_index >= GRID_NUM)
		return;

	if (pdivoom_white_noise_player == NULL)
		return;

//	if (divoom_voice_read_get_filename(grid_to_voice_id[grid_index], path) == 0)
//		return;

	pdivoom_white_noise_player->cur_play_effect = grid_index;
	pdivoom_white_noise_player->sleep_timer_minutes = sleep_timer_minutes;

	ret = wn_schedule_voice_push();
	if (ret != 0)
		divoom_white_noise_player_play_tone_impl();
}

void divoom_white_noise_player_stop(void)
{
	divoom_app_schedule_item_info info = {0};

	info.model_id = DIVOOM_MODEL_APP_WHITE_NOISE;
	info.event_id = DIVOOM_WHITE_NOISE_SCHEDULE_PLAY_ACTIVE;
	info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&info);

	divoom_local_music_ctrl_stop_tone();
	if (pdivoom_white_noise_player)
		pdivoom_white_noise_player->wn_session_active = 0;
}

uint8 divoom_white_noise_player_try_consume_play_key(uint8 key_value, uint8 key_status)
{
	if (key_value != DIVOOM_KEY_MUSIC_PLAY)
		return 0;
	if (pdivoom_white_noise_player == NULL || !pdivoom_white_noise_player->wn_session_active)
		return 0;
	if (!divoom_local_music_ctrl_get_tone_status())
		return 0;
	/* 仅在离开助眠菜单后的后台播放时，用播放/暂停键停止 */
	if (divoom_white_noise_player_is_in_wn_menu())
		return 0;
	if (key_status != DIVOOM_PRESS_SHORT)
		return 0;

	divoom_white_noise_player_stop();
	return 1;
}

int divoom_white_noise_player_is_playing(void)
{
	return divoom_local_music_ctrl_get_tone_status();
}

void divoom_white_noise_player_set_volume(int volume)
{
    if (!pdivoom_white_noise_player)
        return;

    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;

    if (pdivoom_white_noise_player->cur_play_effect_volume != volume)
    {
        pdivoom_white_noise_player->cur_play_effect_volume = volume;
        divoom_white_noise_player_save_cfg();
    }
}

int divoom_white_noise_player_get_volume(void)
{
    if (!pdivoom_white_noise_player)
        return 0;
    return pdivoom_white_noise_player->cur_play_effect_volume;
}

void divoom_white_noise_player_set_sleep_timer_minutes(int minutes)
{
	if (minutes < 0)
		minutes = 0;
	if (pdivoom_white_noise_player->sleep_timer_minutes != minutes)
	{
		pdivoom_white_noise_player->sleep_timer_minutes = minutes;
		divoom_white_noise_player_save_cfg();
	}
}

int divoom_white_noise_player_get_sleep_timer_minutes(void)
{
	return pdivoom_white_noise_player->sleep_timer_minutes;
}

uint8 divoom_white_noise_player_event(Pdivoom_sys_msg_info msg)
{
	divoom_debug("event:%d\n", msg->msg_id);

	if (msg)
	{
		switch (msg->msg_id)
		{
			case DIVOOM_WHITE_NOISE_SET:
				break;

			case DIVOOM_WHITE_NOISE_GET:
				break;

			case DIVOOM_APP_CONFIG_SAVE_EVENT_ID:
				divoom_white_noise_player_save_cfg();
				break;
		}
	}
	return 0;
}

uint8 divoom_white_noise_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{
	(void)user_data;
	if (!(resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE))
		return 1;
	if (event_id != DIVOOM_WHITE_NOISE_SCHEDULE_PLAY_ACTIVE)
		return 1;
	divoom_white_noise_player_play_tone_impl();
	return 1;
}

uint8 divoom_white_noise_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	(void)user_data;
	(void)active_model;
	(void)active_event;
	if (!(resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE))
		return 1;
	if (event_id != DIVOOM_WHITE_NOISE_SCHEDULE_PLAY_ACTIVE)
		return 1;
	divoom_local_music_ctrl_stop_tone();
	return 1;
}

uint8 divoom_white_noise_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
	(void)user_data;
	(void)status;
	if (!(resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE))
		return 1;
	if (event_id != DIVOOM_WHITE_NOISE_SCHEDULE_PLAY_ACTIVE)
		return 1;
	divoom_local_music_ctrl_stop_tone();
	if (pdivoom_white_noise_player)
		pdivoom_white_noise_player->wn_session_active = 0;
	return 1;
}

uint8 divoom_white_noise_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
	(void)key_id;
	(void)key_type;
	(void)resouce_type;
	return 0;
}

// 获取当前播放的提示音（宫格索引）
int divoom_white_noise_player_get_current_effect(void)
{
    if (!pdivoom_white_noise_player)
        return -1;
    return pdivoom_white_noise_player->cur_play_effect;
}


void divoom_white_noise_player_play_current_effect(void)
{
    if (!pdivoom_white_noise_player)
        return;

    int grid_index = pdivoom_white_noise_player->cur_play_effect;
    int sleep_timer_minutes = pdivoom_white_noise_player->sleep_timer_minutes;

	divoom_print("wft test white noise play_current_effect : %d\n", grid_index);
    // 防止越界
    if (grid_index < 0 || grid_index >= GRID_NUM)
        return;

    divoom_white_noise_player_start(grid_index, sleep_timer_minutes);
}

/** 切换收藏：若 index 不在收藏列表则加入收藏，否则从收藏中移除。返回 0 成功，1 参数错误 */
int divoom_white_noise_player_toggle_favorite(int index)
{
	int i, j;

	if (index < 0 || index >= GRID_NUM || !pdivoom_white_noise_player)
		return 1;

	for (i = 0; i < pdivoom_white_noise_player->favorite_effects_count; i++)
	{
		if (pdivoom_white_noise_player->favorite_effects[i] == index)
		{
			/* 已在收藏中，移除 */
			pdivoom_white_noise_player->favorite_effects_count--;
			for (j = i; j < pdivoom_white_noise_player->favorite_effects_count; j++)
				pdivoom_white_noise_player->favorite_effects[j] = pdivoom_white_noise_player->favorite_effects[j + 1];
			divoom_white_noise_player_save_cfg();
			return 0;
		}
	}
	/* 未收藏，加入 */
	if (pdivoom_white_noise_player->favorite_effects_count >= GRID_NUM)
		return 1;
	pdivoom_white_noise_player->favorite_effects[pdivoom_white_noise_player->favorite_effects_count] = index;
	pdivoom_white_noise_player->favorite_effects_count++;
	divoom_white_noise_player_save_cfg();
	return 0;
}

int divoom_white_noise_player_get_favorite_count(void)
{
	if (!pdivoom_white_noise_player)
		return 0;
	return pdivoom_white_noise_player->favorite_effects_count;
}

int divoom_white_noise_player_get_favorite_at(int list_index)
{
	if (!pdivoom_white_noise_player || list_index < 0 || list_index >= pdivoom_white_noise_player->favorite_effects_count)
		return 0;
	return pdivoom_white_noise_player->favorite_effects[list_index];
}

int divoom_white_noise_player_is_favorite(int index)
{
	int i;

	if (index < 0 || index >= GRID_NUM || !pdivoom_white_noise_player)
		return 0;
	for (i = 0; i < pdivoom_white_noise_player->favorite_effects_count; i++) {
		if (pdivoom_white_noise_player->favorite_effects[i] == index)
			return 1;
	}
	return 0;
}

void divoom_white_noise_player_save_cfg(void)
{
    if (!pdivoom_white_noise_player)
        return;

    divoom_cJSON *root = divoom_cJSON_CreateObject();

    // 保存当前播放的提示音index
    divoom_cJSON_AddNumberToObject(root, "cur_play_effect", pdivoom_white_noise_player->cur_play_effect);

    // 保存当前音量
    divoom_cJSON_AddNumberToObject(root, "cur_play_effect_volume", pdivoom_white_noise_player->cur_play_effect_volume);

    // 保存当前定时时长
    divoom_cJSON_AddNumberToObject(root, "cur_sleep_timer_minutes", pdivoom_white_noise_player->sleep_timer_minutes);

    // 保存收藏队列
    divoom_cJSON *favorite_effects_array = divoom_cJSON_CreateIntArray(pdivoom_white_noise_player->favorite_effects, 
		pdivoom_white_noise_player->favorite_effects_count);
    divoom_cJSON_AddItemToObject(root, "favorite_effects_array", favorite_effects_array);

    // 转化为字符串并保存到文件
    char *json_str = divoom_cJSON_Print(root);
    if (json_str == NULL)
        return;
    FILE *fp = divoom_port_file_open(DIVOOM_WHITE_NOISE_PLAYER_SAVE_PATH, "w");
    if (fp == NULL)
        return;
    if (fp && json_str) 
	{
        divoom_port_file_write(json_str, 1, strlen(json_str), fp);
    }
    divoom_port_file_close(fp);
    divoom_cJSON_Delete(root);
    divoom_system_free(json_str);
}

void divoom_white_noise_player_set_default(void)
{
	pdivoom_white_noise_player->cur_play_effect = 0;
	pdivoom_white_noise_player->cur_play_effect_volume = 20;
	pdivoom_white_noise_player->sleep_timer_minutes = 0;
}

//获取配置（上次选择的提示音、定时时长）
void divoom_white_noise_player_load_cfg(void)
{
    FILE *fp = divoom_port_file_open(DIVOOM_WHITE_NOISE_PLAYER_SAVE_PATH, "r");
    if (fp == NULL)
	{
    	divoom_white_noise_player_set_default();
		return;
	}
    char buf[128];
    size_t n = divoom_port_file_read(buf, 1, sizeof(buf) - 1, fp);
    divoom_port_file_close(fp);
    if (n == 0)
        return;
    buf[n] = '\0';
    divoom_cJSON *root = divoom_cJSON_Parse(buf);
    if (root == NULL)
        return;
    pdivoom_white_noise_player->cur_play_effect = divoom_cJSON_GetObjectInt(root, "cur_play_effect");
    pdivoom_white_noise_player->cur_play_effect_volume = divoom_cJSON_GetObjectInt(root, "cur_play_effect_volume");
    pdivoom_white_noise_player->sleep_timer_minutes = divoom_cJSON_GetObjectInt(root, "cur_sleep_timer_minutes");
    divoom_cJSON *favorite_effects_array = divoom_cJSON_GetObjectItem(root, "favorite_effects_array");
    if (favorite_effects_array == NULL)
        return;
    pdivoom_white_noise_player->favorite_effects_count = divoom_cJSON_GetArraySize(favorite_effects_array);
    for (int i = 0; i < pdivoom_white_noise_player->favorite_effects_count; i++)
    {
        pdivoom_white_noise_player->favorite_effects[i] = divoom_cJSON_GetArrayItem(favorite_effects_array, i)->valueint;
    }
    divoom_cJSON_Delete(root);
}

void divoom_white_noise_player_init(void)
{
    // 防止重复初始化
    if (pdivoom_white_noise_player != NULL) 
	{
        return;
    }

    // 分配和初始化控制结构体
    pdivoom_white_noise_player = (Pdivoom_white_noise_player_ctrl)divoom_system_malloc(sizeof(divoom_white_noise_player_ctrl));
    if (pdivoom_white_noise_player == NULL) 
	{
        // 分配失败，直接返回
        return;
    }
    memset(pdivoom_white_noise_player, 0, sizeof(divoom_white_noise_player_ctrl));

    // 尝试加载历史配置(收藏、音量、定时等)
    divoom_white_noise_player_load_cfg();

    // 注册模型到系统调度
    divoom_sys_model_crl_register(DIVOOM_MODEL_APP_WHITE_NOISE, divoom_white_noise_player_event);
}

void divoom_white_noise_player_exit(void)
{
    if (pdivoom_white_noise_player == NULL)
        return;

    divoom_white_noise_player_stop();

    // 保存当前配置
    divoom_white_noise_player_save_cfg();

    // 释放控制结构体
    divoom_system_free(pdivoom_white_noise_player);
    pdivoom_white_noise_player = NULL;
}       
