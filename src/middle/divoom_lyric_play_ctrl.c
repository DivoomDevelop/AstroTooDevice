#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "divoom.h"
#include "divoom_lyric_play_ctrl.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_voice_read.h"
#include "divoom_lvgl_lyric_word_ctrl.h"

#define DIVOOM_LYRIC_GRID_NUM    9

typedef struct _divoom_lyric_play_ctrl
{
	int cur_play_bg_index;   //当前播放的歌词背景index
    //int cur_play_effect_volume; //当前播放的歌词背景音量
	//int sleep_timer_minutes;    // 当前定时（分钟），0 表示关闭
	int select_lyric_effect;		//当前歌词效果
	int favorite_bg[DIVOOM_LYRIC_GRID_NUM];    // 收藏的提示音index队列
    int favorite_bg_count;    // 收藏的提示音index队列长度

}divoom_lyric_play_ctrl, *Pdivoom_lyric_play_ctrl;

static Pdivoom_lyric_play_ctrl pdivoom_lyric_play_handle = NULL;

// 获取当前播放的歌词背景（宫格索引）
int divoom_lyric_play_get_current_bg(void)
{
    if (!pdivoom_lyric_play_handle)
        return 0;

    return pdivoom_lyric_play_handle->cur_play_bg_index;
}

// 获取当前播放的歌词背景（宫格索引）
void divoom_lyric_play_set_current_bg(int bg_index)
{
    if (!pdivoom_lyric_play_handle)
        return;

	if(pdivoom_lyric_play_handle->cur_play_bg_index != bg_index)
	{
		pdivoom_lyric_play_handle->cur_play_bg_index = bg_index;
		divoom_lyric_play_save_cfg();
	}

}


// 获取当前播放的歌词效果（宫格索引）
int divoom_lyric_play_get_current_lyric_effect(void)
{
    if (!pdivoom_lyric_play_handle)
        return 0;

    return pdivoom_lyric_play_handle->cur_play_bg_index;
}

// 获取当前播放的歌词效果（宫格索引）
void divoom_lyric_play_set_current_lyric_effect(int lyric_effect)
{
    if (!pdivoom_lyric_play_handle)
        return;

    if(pdivoom_lyric_play_handle->cur_play_bg_index != lyric_effect)
	{
		pdivoom_lyric_play_handle->cur_play_bg_index = lyric_effect;
		divoom_lyric_play_save_cfg();
	}
}


void divoom_lyric_play_play_current_effect(void)
{
    if (!pdivoom_lyric_play_handle)
        return;

    int grid_index = pdivoom_lyric_play_handle->cur_play_bg_index;
    //int sleep_timer_minutes = pdivoom_lyric_play_handle->sleep_timer_minutes;


    // 防止越界
    if (grid_index < 0 || grid_index >= DIVOOM_LYRIC_GRID_NUM)
        return;

    //divoom_lyric_play_start(grid_index, sleep_timer_minutes);
}



int divoom_lyric_play_toggle_favorite(int index)
{
	int i, j;

	if (index < 0 || index >= DIVOOM_LYRIC_GRID_NUM || !pdivoom_lyric_play_handle)
		return 1;

	for (i = 0; i < pdivoom_lyric_play_handle->favorite_bg_count; i++)
	{
		if (pdivoom_lyric_play_handle->favorite_bg[i] == index)
		{
			/* 已在收藏中，移除 */
			pdivoom_lyric_play_handle->favorite_bg_count--;
			for (j = i; j < pdivoom_lyric_play_handle->favorite_bg_count; j++)
				pdivoom_lyric_play_handle->favorite_bg[j] = pdivoom_lyric_play_handle->favorite_bg[j + 1];
			divoom_lyric_play_save_cfg();
			return 0;
		}
	}
	/* 未收藏，加入 */
	if (pdivoom_lyric_play_handle->favorite_bg_count >= DIVOOM_LYRIC_GRID_NUM)
		return 1;
	pdivoom_lyric_play_handle->favorite_bg[pdivoom_lyric_play_handle->favorite_bg_count] = index;
	pdivoom_lyric_play_handle->favorite_bg_count++;
	divoom_lyric_play_save_cfg();
	return 0;
}


int divoom_lyric_play_get_favorite_count(void)
{
	if (!pdivoom_lyric_play_handle)
		return 0;
	return pdivoom_lyric_play_handle->favorite_bg_count;
}

int divoom_lyric_play_get_favorite_at(int list_index)
{
	if (!pdivoom_lyric_play_handle || list_index < 0 || list_index >= pdivoom_lyric_play_handle->favorite_bg_count)
		return 0;
	return pdivoom_lyric_play_handle->favorite_bg[list_index];
}

int divoom_lyric_play_is_favorite(int index)
{
	int i;

	if (index < 0 || index >= DIVOOM_LYRIC_GRID_NUM || !pdivoom_lyric_play_handle)
		return 0;
	for (i = 0; i < pdivoom_lyric_play_handle->favorite_bg_count; i++) {
		if (pdivoom_lyric_play_handle->favorite_bg[i] == index)
			return 1;
	}
	return 0;
}


void divoom_lyric_play_set_default(void)
{
	pdivoom_lyric_play_handle->cur_play_bg_index = 0;
	pdivoom_lyric_play_handle->select_lyric_effect = 0;
	
	//pdivoom_lyric_play_handle->cur_play_effect_volume = 20;
	//pdivoom_lyric_play_handle->sleep_timer_minutes = 0;
}

void divoom_lyric_play_save_cfg(void)
{
    if (!pdivoom_lyric_play_handle)
        return;

    divoom_cJSON *root = divoom_cJSON_CreateObject();

    // 保存当前播放的提示音index
    divoom_cJSON_AddNumberToObject(root, "cur_play_bg_index", pdivoom_lyric_play_handle->cur_play_bg_index);
	divoom_cJSON_AddNumberToObject(root, "select_lyric_effect", pdivoom_lyric_play_handle->select_lyric_effect);

    // 保存当前音量
    //divoom_cJSON_AddNumberToObject(root, "cur_play_effect_volume", pdivoom_lyric_play_handle->cur_play_effect_volume);

    // 保存当前定时时长
    //divoom_cJSON_AddNumberToObject(root, "cur_sleep_timer_minutes", pdivoom_lyric_play_handle->sleep_timer_minutes);

    // 保存收藏队列
    divoom_cJSON *favorite_bg_array = divoom_cJSON_CreateIntArray(pdivoom_lyric_play_handle->favorite_bg, 
		pdivoom_lyric_play_handle->favorite_bg_count);
    divoom_cJSON_AddItemToObject(root, "favorite_bg_array", favorite_bg_array);

    // 转化为字符串并保存到文件
    char *json_str = divoom_cJSON_Print(root);
    if (json_str == NULL)
        return;
    FILE *fp = divoom_port_file_open(DIVOOM_LYRIC_PLAY_CTRL_PATH, "w");
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


//获取配置（上次选择的提示音、定时时长）
void divoom_lyric_play_load_cfg(void)
{
    FILE *fp = divoom_port_file_open(DIVOOM_LYRIC_PLAY_CTRL_PATH, "r");
    if (fp == NULL)
	{
    	divoom_lyric_play_set_default();
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
    pdivoom_lyric_play_handle->cur_play_bg_index = divoom_cJSON_GetObjectInt(root, "cur_play_bg_index");
  	pdivoom_lyric_play_handle->select_lyric_effect = divoom_cJSON_GetObjectInt(root, "select_lyric_effect");
    
	//pdivoom_lyric_play_handle->cur_play_effect_volume = divoom_cJSON_GetObjectInt(root, "cur_play_effect_volume");
    //pdivoom_lyric_play_handle->sleep_timer_minutes = divoom_cJSON_GetObjectInt(root, "cur_sleep_timer_minutes");
    
	divoom_cJSON *favorite_bg_array = divoom_cJSON_GetObjectItem(root, "favorite_bg_array");
    if (favorite_bg_array == NULL)
        return;
    pdivoom_lyric_play_handle->favorite_bg_count = divoom_cJSON_GetArraySize(favorite_bg_array);
    for (int i = 0; i < pdivoom_lyric_play_handle->favorite_bg_count; i++)
    {
        pdivoom_lyric_play_handle->favorite_bg[i] = divoom_cJSON_GetArrayItem(favorite_bg_array, i)->valueint;
    }
    divoom_cJSON_Delete(root);
}


void divoom_lyric_play_init(void)
{
    // 防止重复初始化
    if (pdivoom_lyric_play_handle != NULL) 
	{
        return;
    }

    // 分配和初始化控制结构体
    pdivoom_lyric_play_handle = (Pdivoom_lyric_play_ctrl)divoom_system_malloc(sizeof(divoom_lyric_play_ctrl));
    if (pdivoom_lyric_play_handle == NULL) 
	{
        // 分配失败，直接返回
        return;
    }
    memset(pdivoom_lyric_play_handle, 0, sizeof(divoom_lyric_play_ctrl));

    // 尝试加载历史配置(收藏、音量、定时等)
    divoom_lyric_play_load_cfg();

    // 注册模型到系统调度
    //divoom_sys_model_crl_register(DIVOOM_MODEL_APP_WHITE_NOISE, divoom_lyric_play_event);
}


void divoom_lyric_play_exit(void)
{
    if (pdivoom_lyric_play_handle == NULL)
        return;

    // 保存当前配置
    divoom_lyric_play_save_cfg();

    // 释放控制结构体
    divoom_system_free(pdivoom_lyric_play_handle);
    pdivoom_lyric_play_handle = NULL;
}      


