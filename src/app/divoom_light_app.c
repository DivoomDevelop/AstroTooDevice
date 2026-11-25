#include "divoom.h"
#include "divoom_lan_storage.h"
#include "lvgl.h"
#include "divoom_clock_favorite.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_lvgl_base_dial_menu.h"
#include "divoom_lvgl_hot_update_prompt.h"
#include "divoom_clock_component_index.h"
#include "divoom_lvgl_poweron.h"
#if DIVOOM_SHOW_MODE
#include "divoom_show_mode_ctrl.h"
#include "divoom_show_clock_rotate_list.h"
#endif
//#define DIVOOM_GET_DEFAULT_ALL_CLOCK_INFO
#define DIVOOM_LIGHT_APP_MAX_CLOCK_HISTORY_NUM   500
#define DIVOOM_LIGHT_APP_DEFAULT_CLOCK_RESTARTS  2
#ifndef DIVOOM_LIGHT_APP_RANDOM_CLOCK_ENABLE
#define DIVOOM_LIGHT_APP_RANDOM_CLOCK_ENABLE    0
#endif
#define DIVOOM_LIGHT_APP_RANDOM_CLOCK_INTERVAL_MS  (15 * 1000)
#define DIVOOM_LIGHT_APP_RANDOM_CLOCK_START_DELAY_MS  (60 * 1000)
#if DIVOOM_SHOW_MODE
#define DIVOOM_SHOW_CLOCK_ROTATE_INTERVAL_MS   (60 * 1000)
#endif
#if DIVOOM_LIGHT_APP_RANDOM_CLOCK_ENABLE && defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
void divoom_malloc_print(void);
void sim_lvgl_leak_report(uint32 sample);
#endif
/* App 的 HOT 云图库固定使用表盘 ID 250，仅该表盘触发更新询问。 */

#define DIVOOM_LIGHT_APP_GIF_ACCPET_TIME_OUT  4
#define DIVOOM_LIGHT_APP_SERVER_GIF_INITIAL_FILE_ID  1U
#define DIVOOM_LIGHT_APP_SERVER_GIF_VOICE_FILE_ID    2U
#define DIVOOM_LIGHT_APP_SERVER_GIF_FIRST_VERSION_ID 3U
#define DIVOOM_LIGHT_APP_SERVER_GIF_DIR               "/userdata/divoom/17"

enum{
    DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE,
    DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE,
    DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE,
    DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE,
    DIVOOM_LIGHT_APP_DISP_PLAY_ALBUM_MENU_MODE,
    DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_MODE,   //表盘定时功能
    DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE,
    DIVOOM_LIGHT_APP_DISP_IFFT_TEXT_MODE,
 	DIVOOM_LIGHT_APP_DISP_IFFT_AMAZON_MUSIC,
    DIVOOM_LIGHT_APP_DISP_TF_GIF_MODE,
	    DIVOOM_LIGHT_APP_DISP_HTTP_GIF_MODE,
	    DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE,
	    DIVOOM_LIGHT_APP_DISP_FILE_TRANSFER_MODE,
	    DIVOOM_LIGHT_APP_DISP_GOOGLE_CALENDAR_MODE,
    DIVOOM_LIGHT_APP_DISP_OUTLOOK_CALENDAR_MODE,
    DIVOOM_LIGHT_APP_DISP_ENTER_GOOGLE_CALENDAR,
    DIVOOM_LIGHT_APP_DISP_ENTER_OUTLOOK_CALENDAR,
    DIVOOM_LIGHT_APP_DISP_ENTER_CUSTOM_DIAL,
	DIVOOM_LIGHT_APP_DISP_DEMO_DIAL_MODE,
    DIVOOM_LIGHT_APP_DISP_PLAYLIST_MODE,
    DIVOOM_LIGHT_APP_DISP_MIXER_MODE,
    DIVOOM_LIGHT_APP_DISP_MIX_MUSIC2_MODE,
}DIVOOM_LIGHT_APP_DISP_MODE;

//DIVOOM_DISP_LOCAL_GIF_MODE的数据结构
typedef struct _divoom_light_app_tf_gif_info_
{
    int flag;           //１：文件夹，0:文件
    int net_num;
    char file_name[256];    //文件名路径
}divoom_light_app_tf_gif_info, *Pdivoom_light_app_tf_gif_info;

#include "divoom_light_app.h"

typedef struct _divoom_light_app_base_info_
{
    uint8 mode;    //以前的保留
    uint8 disp_mode;    //0:表盘，1：显示相册
    uint8 power_on_mode;    //开机显示的频道 0:表盘，1：显示相册
    int   power_on_clock_id;    //开机显示的表盘id
    uint8 play_roll;        //是否轮流播放
    int power_on_disp_clock_flag;    //开机显示上一次的表盘（0），还是开机设置的表盘（1）
    int clock_disp_time;    //时钟显示时间的时间
    int clock_disp_gif;     //时钟显示动画的时间
    int each_gif_time;     //时钟显示动画的时间
    int show_time_flag;     //作品是否显示时间
    int divoom_app_animation_edge_id;   //APP发送动画的边框ID
    int history_num;        //表盘历史记录个数
    int *clock_history;      //表盘历史记录数组
    int fav_num;        //表盘收藏个数
    int *clock_fav;      //表盘收藏记录数组
    int recent_used_num;        //表盘最近使用个数
    int clock_recent_used[DIVOOM_RECENT_USED_CLOCK_MAX_NUM];      //表盘最近使用记录数组
    int clock_id;
    int pre_clock_id;
	int clock_mode; //0 显示模式 1 编辑模式
	int vision_play_time;
	divoom_lvgl_dial_ctr_info clock_info;
}divoom_light_app_base_info, *Pdivoom_light_app_base_info;

typedef struct _divoom_light_app_power_on_cfg_
{
    int channel_index;
    int clock_id;
}divoom_light_app_power_on_cfg, *Pdivoom_light_app_power_on_cfg;

typedef struct _divoom_light_app_ctrl_
{
    uint8 app_request_flag;
    uint8 first_flag;
    uint8 watch_dog_flag;
    uint8 active_flag;     //0:模块未激活,1:模块正在运行
    uint8 cur_mode;        //DIVOOM_LIGHT_APP_DISP_MODE
    uint8 light_switch;    //灯的开关控制
    uint8 file_transfer_exit_requested; //用户主动退出时允许绕过下载页驻留保护
    uint32 resrouce_type;
    int    custom_clock_num;    //用户定制表盘数目
    int    clear_bg_clock;
    int    wait_gif_time;
    int    power_off_clock_mode;
    int    power_off_clock_id;
    uint32 suscribe_play_time[DIVOOM_LIGHT_APP_USER_DEFINE_MAX_NUM];
    uint32 clock_id;
    uint32 playlist_id;
    void   *lock_handle;
    void   *clock_list_lock;
    char   *ifft_text_buf;
    void    *gif_file_queue;
    void    *next_clock_queue;
    void    *app_gif_file_lock;
    void    *server_gif_del_queue;
    uint32  server_gif_file_id;
    uint32  server_gif_file_generation;
    uint16  download_font_total;    //下载文件的总数
    uint16  download_font_number;   //已经下载文件的数目
    int     pending_clock_download_id; //正在等待配置/素材的新添加表盘
    uint8   clock_download_progress_active; //下载页正在等待新表盘配置或资源
    uint8   clock_download_total_known; //已收到配置并统计出当前待下载文件数
    uint8   clock_download_failed; //本次资源批次失败，退出到此前可用的表盘
    uint8   clock_download_config_pending; //业务锁保护：完整配置更新前禁止统计和播放
    uint8   clock_download_config_dirty; //业务锁保护：配置等待期间收到更新，显示前重新获取
    uint8   clock_download_config_completed; //资源锁保护：配置进展，仅用于等待超时判定
    uint32  clock_download_session; //非零下载会话编号；资源回调与菜单事件均携带此编号
    uint32  clock_select_intent; //在等待列表/业务锁之前记录用户操作次序
    void    *download_font_lock;
    void    *download_font_queue;
    divoom_manage_image_disp_info   remote_image_info;
    divoom_manage_image_disp_info   cur_play_image_info;
    divoom_lvgl_dial_ctr_info schedule_dial_info;
    divoom_clock_schedule_play_info   schedule_clock_info;    //定时播放表盘的表盘ID
    divoom_light_app_base_info base_light;
	divoom_disp_ctrl_gif_info gif_info; //控制动画
	divoom_disp_ctrl_phone_draw_pic_info draw_info; //控制画板
    divoom_light_app_tf_gif_info tf_gif;  //本地ＧＩＦ
    divoom_light_app_mixer_state mixer;  //Mixer 页面唯一运行状态
    divoom_light_app_mix_music2_state mix_music2; //Mixer/SetNew 独立乐器页面状态
}divoom_light_app_ctrl, *Pdivoom_light_app_ctrl;

static Pdivoom_light_app_ctrl pdivoom_light_app_handle = NULL;

static void divoom_light_app_select_added_clock(int clock_id, int clock_mode, uint32 intent,
    uint8 refresh_config);
static uint32 divoom_light_app_reserve_clock_intent(void);
static int divoom_light_app_clock_intent_is_current(uint32 intent);
void divoom_light_app_check_download_font(int32 clock_id, uint32 model_id, uint32 image_id,
    int32 result);
static void divoom_light_app_set_clock_download_progress_state(uint8 active,
    uint8 total_known);
static uint32 divoom_light_app_begin_clock_download(int clock_id);
static int divoom_light_app_check_clock_download_files(int clock_id);

/* 与低空间回收共用：锁顺序固定为列表锁 -> removed_clock_lock -> clock_lock。 */
void divoom_light_app_clock_list_lock(void)
{
    if(pdivoom_light_app_handle != NULL && pdivoom_light_app_handle->clock_list_lock != NULL)
    {
        divoom_system_get_lock(pdivoom_light_app_handle->clock_list_lock);
    }
}

void divoom_light_app_clock_list_unlock(void)
{
    if(pdivoom_light_app_handle != NULL && pdivoom_light_app_handle->clock_list_lock != NULL)
    {
        divoom_system_free_lock(pdivoom_light_app_handle->clock_list_lock);
    }
}


//系统内置表盘列表
const uint16 divoom_defalt_clock_inner_list[] = 
{
    255, 1642, 1721, 1734, 1748, 1752, 1786, 1804, 1808, 1814, 1847, 1856, 1890, 1892, 1906, 1910, 1920, 1940, 1954, 1955, 1966, 2051, 2052, 2186, 2335, 2387, 2388, 2389, 2390,
    197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,254,1635,1636,1637,1638,2092,2094,
    2096,2098,2100,2104,2106,2108,2110,2112,2114,2116,2118,2120,2121,2123,2125,2126,2127,2128,2131,2132,2139,2140,2142,2143,
    2144,2145,2146,2147,2148,2149,2150,2151,2153,2154,2155,2156,2157,2158,2159,2160,2212,2303,2329
};

const  int divoom_defult_clock_history_cn[] =
{1856	,255	,1814	,2387	,1954	,1847	,1955	,1906	,2335	,2052	,2051	,1910	,1940	,1966	,1808	,1804	,1642	,1721	,1892	,2390	,1920	,1786	,1748	,1734	,1890
};

const  int divoom_defult_clock_history_en[] =
{2052	,1966	,1940	,255	,1954	,1847	,1955	,1906	,2335	,2186	,1734	,1748	,2051	,1856	,1814	,1808	,1721	,1892	,1910	,1920	,1752	,1890	,2388	,2389
};


#ifdef DIVOOM_GET_DEFAULT_ALL_CLOCK_INFO

void divoom_light_app_load_all_default_dail_info(void)
{
    Pdivoom_net_command ret_data;
    
    //将重新加载所有信息
    #if 1
    ret_data = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_SEND_GET_DEVICE_CLOCK_CONFIG, DIVOOM_SERVER_TOPIC,NULL);
    if(ret_data && ret_data->cmd_data)
    {
        Pdivoom_devcice_time_dial_clock_list_respone plistInfo = (Pdivoom_devcice_time_dial_clock_list_respone)ret_data->cmd_data;

        for(int i = 0; i < plistInfo->num; i++ )
        {
             divoom_clock_manage_check_clock_all_file(plistInfo->plist[i].clock_id, NULL);
            divoom_clock_id_info clock_info;
            
            clock_info.clock_id = plistInfo->plist[i].clock_id;
            clock_info.clock_pos = 0;
            clock_info.parent_id = 0;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA, DIVOOM_SERVER_TOPIC,&clock_info);
        }
        
    }
    divoom_net_app_free_packet(ret_data);
    #else
    for(int i = 0; i < (uint32)(sizeof(divoom_defalt_clock_inner_list) /
        sizeof(divoom_defalt_clock_inner_list[0])); i++)
    {
             divoom_clock_manage_check_clock_all_file(divoom_defalt_clock_inner_list[i], NULL);
            divoom_clock_id_info clock_info;
            
            clock_info.clock_id = divoom_defalt_clock_inner_list[i];
            clock_info.clock_pos = 0;
            clock_info.parent_id = 0;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA, DIVOOM_SERVER_TOPIC,&clock_info);
    }
    #endif

}
#endif
int divoom_light_app_is_default_inner_clock(int clock_id)
{
    uint32 i;

    for(i = 0; i < (uint32)(sizeof(divoom_defalt_clock_inner_list) /
        sizeof(divoom_defalt_clock_inner_list[0])); i++)
    {
        if(divoom_defalt_clock_inner_list[i] == clock_id)
        {
            return 1;
        }
    }
    return 0;
}


uint8 divoom_light_app_request_resource(int cur_mode);

void divoom_light_app_get_mixer_state(Pdivoom_light_app_mixer_state mixer_state)
{
    if(mixer_state == NULL)
    {
        return;
    }

    memset(mixer_state, 0, sizeof(divoom_light_app_mixer_state));
    if(pdivoom_light_app_handle == NULL)
    {
        return;
    }

    if(pdivoom_light_app_handle->lock_handle != NULL)
    {
        divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
    }
    *mixer_state = pdivoom_light_app_handle->mixer;
    if(pdivoom_light_app_handle->lock_handle != NULL)
    {
        divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
    }
}

void divoom_light_app_get_mix_music2_state(Pdivoom_light_app_mix_music2_state mix_music2_state)
{
    if(mix_music2_state == NULL)
    {
        return;
    }

    memset(mix_music2_state, 0, sizeof(divoom_light_app_mix_music2_state));
    mix_music2_state->icon = 0xff;
    if(pdivoom_light_app_handle == NULL)
    {
        return;
    }
    if(pdivoom_light_app_handle->lock_handle != NULL)
    {
        divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
    }
    *mix_music2_state = pdivoom_light_app_handle->mix_music2;
    if(pdivoom_light_app_handle->lock_handle != NULL)
    {
        divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
    }
}

void divoom_light_app_load_save(void);
static void divoom_light_app_report_selected_clock_id(int clock_id);


void divoom_light_app_play_all_id(void *user_data, T_U8 id, Pdivoom_play_gif_image_info image_info)
{
    int user_id;
    int vocice_flag;
    char *image_name = divoom_light_app_get_cur_image_name();
    
    image_info->model_id = DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE;
    image_info->image_id = divoom_user_define_ctrl_get_next_id(user_data, &user_id, image_info->image_name, &vocice_flag);
    image_info->user_id = user_id;
    if(image_info->image_id == 0)
    {
        image_info->model_id = DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
        image_info->image_id = DIVOOM_DISP_USER_DEFINE_TV_PIC;
    }
    if(image_info->image_id != pdivoom_light_app_handle->cur_play_image_info.image_id)
    {
        pdivoom_light_app_handle->cur_play_image_info.image_id = pdivoom_light_app_handle->cur_play_image_info.image_id;
    }
    image_info->voice_flag = vocice_flag;
}

void divoom_light_app_subscribe_get_pic_id(void *user_data, T_U8 area_id, Pdivoom_play_gif_image_info image_info)
{
    divoom_subscribe_manage_get_image_id(user_data, image_info);
    if(image_info->image_id != pdivoom_light_app_handle->cur_play_image_info.image_id)
    {
        pdivoom_light_app_handle->cur_play_image_info.image_id = pdivoom_light_app_handle->cur_play_image_info.image_id;
    }
    
}

int divoom_light_app_subscribe_get_change(void *user_data)
{
    return divoom_subscribe_manage_get_change_flag(user_data);
    
}

/*
  Function:       divoom_light_app_update_clock_history
  Description:    更新表盘历史记录
  Input:          play_clock_id
  Return:         NONE
  Author:		wangft
*/

void divoom_light_app_get_my_clock_list(void)
{
	divoom_device_get_clock_list_para para;

	para.start_num = 1;
	para.end_num = DIVOOM_MAX_CLOCK_NUM;
    divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_CLOCK_LIST, DIVOOM_SERVER_TOPIC,&para);
}

void divoom_light_app_get_my_clock_recent_list(void)
{
	divoom_device_get_clock_list_para para;

	para.start_num = 1;
	para.end_num = DIVOOM_MAX_CLOCK_NUM;
	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_CLOCK_RECENT, DIVOOM_SERVER_TOPIC, &para);
}

void divoom_light_app_get_my_clock_favorite_list(void)
{
	divoom_device_get_clock_list_para para;

	para.start_num = 1;
	para.end_num = DIVOOM_MAX_CLOCK_NUM;
	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_CLOCK_FAVORITE, DIVOOM_SERVER_TOPIC, &para);
}

int divoom_light_app_copy_my_clock_list_limited(int *clock_list, int max_count)
{
	int n;

	if (clock_list == NULL || max_count <= 0 || pdivoom_light_app_handle == NULL)
	{
		return 0;
	}
	n = pdivoom_light_app_handle->base_light.history_num;
	if (n <= 0 || pdivoom_light_app_handle->base_light.clock_history == NULL)
	{
		return 0;
	}
	if (n > max_count)
	{
		n = max_count;
	}
	memcpy(clock_list, pdivoom_light_app_handle->base_light.clock_history, (size_t)n * sizeof(int));
	return n;
}

int divoom_light_app_copy_my_clock_list_num(void)
{
	if (pdivoom_light_app_handle == NULL)
	{
		return 0;
	}
	return pdivoom_light_app_handle->base_light.history_num;
}

int divoom_light_app_copy_my_clock_list(int *clock_list)
{
	if (clock_list == NULL || pdivoom_light_app_handle == NULL)
	{
		return 0;
	}
	return divoom_light_app_copy_my_clock_list_limited(clock_list,
		pdivoom_light_app_handle->base_light.history_num);
}

int divoom_light_app_get_recent_clock_num(void)
{
	if (pdivoom_light_app_handle == NULL)
	{
		return 0;
	}
	return pdivoom_light_app_handle->base_light.recent_used_num;
}

int divoom_light_app_copy_recent_clock_list_limited(int *clock_list, int max_count)
{
	int n;

	if (clock_list == NULL || max_count <= 0 || pdivoom_light_app_handle == NULL)
	{
		return 0;
	}
	n = pdivoom_light_app_handle->base_light.recent_used_num;
	if (n <= 0)
	{
		return 0;
	}
	if (n > max_count)
	{
		n = max_count;
	}
	memcpy(clock_list, pdivoom_light_app_handle->base_light.clock_recent_used, (size_t)n * sizeof(int));
	return n;
}

void divoom_light_app_remove_recent_clock(int clock_id)
{
	int i;
	int changed = 0;

	if (pdivoom_light_app_handle == NULL || clock_id <= 0)
	{
		return;
	}
	divoom_light_app_clock_list_lock();
	for (i = 0; i < pdivoom_light_app_handle->base_light.recent_used_num; i++)
	{
		if (pdivoom_light_app_handle->base_light.clock_recent_used[i] == clock_id)
		{
			if (i < pdivoom_light_app_handle->base_light.recent_used_num - 1)
			{
				memmove(&pdivoom_light_app_handle->base_light.clock_recent_used[i],
					&pdivoom_light_app_handle->base_light.clock_recent_used[i + 1],
					(size_t)(pdivoom_light_app_handle->base_light.recent_used_num - i - 1) * sizeof(int));
			}
			pdivoom_light_app_handle->base_light.recent_used_num--;
			changed = 1;
			break;
		}
	}
	divoom_light_app_clock_list_unlock();
	if(changed)
	{
		divoom_light_app_load_save();
	}
}

int divoom_light_app_get_fav_clock_num(void)
{
	if (pdivoom_light_app_handle == NULL)
	{
		return 0;
	}
	return pdivoom_light_app_handle->base_light.fav_num;
}

int divoom_light_app_copy_fav_clock_list_limited(int *clock_list, int max_count)
{
	int n;

	if (clock_list == NULL || max_count <= 0 || pdivoom_light_app_handle == NULL)
	{
		return 0;
	}
	n = pdivoom_light_app_handle->base_light.fav_num;
	if (n <= 0 || pdivoom_light_app_handle->base_light.clock_fav == NULL)
	{
		return 0;
	}
	if (n > max_count)
	{
		n = max_count;
	}
	memcpy(clock_list, pdivoom_light_app_handle->base_light.clock_fav, (size_t)n * sizeof(int));
	return n;
}

int divoom_light_app_is_fav_clock(int clock_id)
{
	int i;

	if (pdivoom_light_app_handle == NULL || clock_id <= 0)
	{
		return 0;
	}
	for (i = 0; i < pdivoom_light_app_handle->base_light.fav_num; i++)
	{
		if (pdivoom_light_app_handle->base_light.clock_fav[i] == clock_id)
		{
			return 1;
		}
	}
	return 0;
}

void divoom_light_app_add_clock_id(int clock_id, int set_as_current)
{
	int *new_clock_history = NULL;
	int *old_clock_history = NULL;
	int old_history_num;
	int added = 0;
	int i = 0;
    uint32 intent = 0;

	if(pdivoom_light_app_handle == NULL)
	{
		return;
	}
    if(set_as_current && clock_id < DIVOOM_CUSTOM_START_CLOCK_ID)
        intent = divoom_light_app_reserve_clock_intent();
	divoom_light_app_clock_list_lock();
	for (i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
	{
		if (clock_id == pdivoom_light_app_handle->base_light.clock_history[i])
		{
			break;
		}
	}
	
	if (i == pdivoom_light_app_handle->base_light.history_num)
	{
		old_history_num = pdivoom_light_app_handle->base_light.history_num;
		if(old_history_num > 0 && pdivoom_light_app_handle->base_light.clock_history == NULL)
		{
			divoom_light_app_clock_list_unlock();
			divoom_print_err("clock history is NULL, num=%d\n", old_history_num);
			return;
		}
		new_clock_history = divoom_system_malloc((old_history_num + 1) * sizeof(int));
		if(new_clock_history == NULL)
		{
			divoom_light_app_clock_list_unlock();
			divoom_print_err("alloc clock history failed, num=%d\n", old_history_num + 1);
			return;
		}
		new_clock_history[0] = clock_id;
		/* 分配成功后再取消；取消写盘失败时，回收端仍以锁内活跃列表保护此 ID。 */
		divoom_clock_manage_cancel_removed_clock(clock_id);
		if(old_history_num > 0)
		{
			memcpy(&new_clock_history[1], pdivoom_light_app_handle->base_light.clock_history,
				old_history_num * sizeof(int));
		}
		old_clock_history = pdivoom_light_app_handle->base_light.clock_history;
		pdivoom_light_app_handle->base_light.clock_history = new_clock_history;
		pdivoom_light_app_handle->base_light.history_num = old_history_num + 1;
		added = 1;
		if(old_clock_history != NULL)
		{
			divoom_system_free(old_clock_history);
		}
	}
	divoom_light_app_clock_list_unlock();

	if(added)
	{
		/* 相框保留原有 SetClockSelectId 选择/播放流程，不进入新增表盘下载分支。 */
		if (set_as_current && clock_id < DIVOOM_CUSTOM_START_CLOCK_ID)
		{
            int *request_id = divoom_system_malloc(sizeof(int));
            if(request_id)
            {
                divoom_sys_msg_info message = {0};
                *request_id = clock_id;
                message.src_id = DIVOOM_MODEL_LIGHT_APP;
                message.dest_id = DIVOOM_MODEL_LIGHT_APP;
                message.msg_type = 1;
                message.msg_id = DIVOOM_LIGHT_APP_EVENT_ADD_CLOCK_RECEIVED;
                message.result = (int)intent;
                message.data = (uint8 *)request_id;
                message.data_len = sizeof(int);
                divoom_sys_model_crl_msg_push(&message);
            }
		}
//    	divoom_light_app_load_save();
	}
}

static void divoom_light_app_clear_clock_event(int clock_id)
{
    int i, attempt, active = 0, cleaned = 0;
    uint32 free_space;

    if(pdivoom_light_app_handle == NULL)
        return;
    /* 页面切换是异步的，在业务消息线程中等待旧表盘释放资源。 */
    divoom_system_delay_ms(500);
    for(attempt = 0; attempt < 20; attempt++)
    {
        if(attempt > 0)
            divoom_system_delay_ms(100);
        divoom_light_app_clock_list_lock();
        active = 0;
        for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
        {
            if(pdivoom_light_app_handle->base_light.clock_history[i] == clock_id)
            {
                active = 1;
                break;
            }
        }
        if(!active)
            cleaned = divoom_clock_manage_clear_clock_data_direct(clock_id);
        divoom_light_app_clock_list_unlock();
        if(active || cleaned)
            break;
    }
    if(active)
        divoom_print("[storage_gc] direct_delete_cancelled clock=%d reason=readded\n", clock_id);
    else if(cleaned)
        divoom_print("[storage_gc] direct_delete_done clock=%d attempts=%d\n", clock_id, attempt + 1);
    else
        divoom_print_err("[storage_gc] direct_delete_failed clock=%d attempts=%d\n", clock_id, attempt);

    free_space = divoom_port_fs_free_space(DIVOOM_USER_ROOT);
    if(free_space < DIVOOM_ROOM_DEL_CLOCK_DATA)
        divoom_clock_manage_clear_all_removed_clock_data();
    else
        divoom_main_update_storage_status(free_space);
}

static void divoom_light_app_delete_clock_id_internal(int clock_id, int force_clear)
{
	int new_clock_id = -1;
    divoom_clock_id_info clock_info = {0};
	int i = 0;
	int found;
	int clear_direct = 0;

	if(pdivoom_light_app_handle == NULL)
	{
		return;
	}
	divoom_light_app_clock_list_lock();
	for (i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
	{
		if (clock_id == pdivoom_light_app_handle->base_light.clock_history[i])
		{
			break;
		}
	}

	{
		found = (i < pdivoom_light_app_handle->base_light.history_num);
        divoom_print("[storage_gc] list delete request: clock=%d found=%d old_count=%d\n",
            clock_id, found, pdivoom_light_app_handle->base_light.history_num);
        clear_direct = !divoom_light_app_is_default_inner_clock(clock_id) &&
            (force_clear || divoom_port_fs_free_space(DIVOOM_USER_ROOT) < DIVOOM_ROOM_DEL_CLOCK_DATA);
        /* 指定直接清理或空间不足时，不写入延迟删除队列。 */
		if(!clear_direct && !divoom_light_app_is_default_inner_clock(clock_id) &&
			divoom_clock_manage_record_removed_clock(clock_id) == 0)
		{
			divoom_light_app_clock_list_unlock();
			divoom_print_err("record removed clock failed: %d\n", clock_id);
			return;
		}

		if (i < pdivoom_light_app_handle->base_light.history_num - 1)
		{
			memmove(&pdivoom_light_app_handle->base_light.clock_history[i], &pdivoom_light_app_handle->base_light.clock_history[i + 1], (pdivoom_light_app_handle->base_light.history_num - i - 1)*sizeof(int));
			pdivoom_light_app_handle->base_light.history_num--;
			new_clock_id = pdivoom_light_app_handle->base_light.clock_history[i];
		}
		else if (i == pdivoom_light_app_handle->base_light.history_num - 1)
		{
			pdivoom_light_app_handle->base_light.clock_history[i] = 0;
			pdivoom_light_app_handle->base_light.history_num--;
			new_clock_id = pdivoom_light_app_handle->base_light.clock_history[0];
		}

	}
	divoom_light_app_clock_list_unlock();
	if(found)
	{
		divoom_clock_favorite_remove_if_present(clock_id);
		divoom_light_app_remove_recent_clock(clock_id);
		divoom_light_app_add_remove_fav_clock(clock_id, 0);
	}

    if(clock_id == pdivoom_light_app_handle->base_light.clock_id && new_clock_id != -1)
    {
        pdivoom_light_app_handle->base_light.clock_id = new_clock_id;
        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
    }
    if(clear_direct && force_clear)
    {
        divoom_sys_msg_info msg = {0};
        msg.msg_type = 1;
        msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
        msg.msg_id = DIVOOM_LIGHT_APP_EVENT_CLEAR_REMOVED_CLOCK;
        msg.result = clock_id;
        divoom_sys_model_crl_msg_push(&msg);
    }
    else if(clear_direct)
    {
        int attempt, active, cleaned = 0;
        uint32 free_space;

        /* 等待时不持列表锁；重试前重新检查，避免删除重新加入的表盘。 */
        for(attempt = 0; attempt < 1; attempt++)
        {
            if(attempt > 0)
                divoom_system_delay_ms(100);
            divoom_light_app_clock_list_lock();
            active = 0;
            for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
            {
                if(pdivoom_light_app_handle->base_light.clock_history[i] == clock_id)
                {
                    active = 1;
                    break;
                }
            }
            if(!active)
                cleaned = divoom_clock_manage_clear_clock_data_direct(clock_id);
            divoom_light_app_clock_list_unlock();
            if(active || cleaned)
                break;
        }
        if(active)
            divoom_print("[storage_gc] direct_delete_cancelled clock=%d reason=readded\n", clock_id);
        else if(cleaned)
            divoom_print("[storage_gc] direct_delete_done clock=%d attempts=%d\n", clock_id, attempt + 1);
        else
            divoom_print_err("[storage_gc] direct_delete_failed clock=%d attempts=%d\n", clock_id, attempt);

        free_space = divoom_port_fs_free_space(DIVOOM_USER_ROOT);
        if(free_space < DIVOOM_ROOM_DEL_CLOCK_DATA)
            divoom_clock_manage_clear_all_removed_clock_data();
        else
            divoom_main_update_storage_status(free_space);
    }
    /* Direct removal must not immediately request and recreate the deleted config. */
    if(!clear_direct)
    {
        clock_info.clock_id = clock_id;
        divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_SYS_DATA, DIVOOM_SERVER_TOPIC,&clock_info);
    }
}

void divoom_light_app_delete_clock_id(int clock_id)
{
    divoom_light_app_delete_clock_id_internal(clock_id, 0);
}

void divoom_light_app_delete_clock_id_direct(int clock_id)
{
    divoom_light_app_delete_clock_id_internal(clock_id, 1);
}

void divoom_light_app_delete_pixel_dial(int clock_id)
{
    Pdivoom_clock_manage_item pclock_info = divoom_clock_manage_get_clock_info(clock_id, 0, 0);

    if(pclock_info == NULL)
    {
        return;
    }

    if(divoom_disp_clock_is_user_define_clock(pclock_info->clock_type))
    {
        divoom_devcice_user_define_item pclean_item = {0};

        pclean_item.clock_id = clock_id;
        divoom_user_define_ctrl_clean_item(&pclean_item);
    }
    else if(divoom_disp_clock_is_cloud_image_clock(pclock_info->clock_type))
    {
        divoom_clock_id_info clock_id_info = {0};

        clock_id_info.clock_id = clock_id;
        divoom_eq_image_manage_clean_item(&clock_id_info);
    }
    else if(divoom_disp_clock_is_eq_clock(pclock_info->clock_type))
    {
        divoom_clock_id_info clock_id_info = {0};
        divoom_channel_set_subscribe_info psubscribe_info = {0};

        clock_id_info.clock_id = clock_id;
        divoom_subscribe_manage_update(&clock_id_info, &psubscribe_info);
    }
    divoom_clock_manage_free_clock(pclock_info);
    divoom_clock_manage_del_clock(clock_id);
}

int divoom_light_app_get_my_clock_num(void)
{
	return pdivoom_light_app_handle->base_light.history_num;
}

static int divoom_light_app_clock_id_in_list(const int *clock_id_list, int clock_num, int clock_id)
{
    int i;

    if(clock_id_list == NULL || clock_num <= 0)
    {
        return 0;
    }
    for(i = 0; i < clock_num; i++)
    {
        if(clock_id_list[i] == clock_id)
        {
            return 1;
        }
    }
    return 0;
}

/*
  Function:       divoom_light_app_update_clock_history
  Description:    更新表盘历史记录
  Input:          play_clock_id
  Return:         NONE
  Author:		wangft
*/

void divoom_light_app_update_my_clock_list(Pdivoom_devcice_get_clock_list_respone clock_list)
{
    int32 default_clock_id;
    int32 effective_clock_num;
    int32 i;
    int32 update_flag;
    const int32 *effective_clock_list;
    int32 *new_clock_history;

    update_flag = (pdivoom_light_app_handle->custom_clock_num != clock_list->custom_num);
    pdivoom_light_app_handle->custom_clock_num = clock_list->custom_num;
    for(i = 0; i < pdivoom_light_app_handle->custom_clock_num; i++)
    {
        divoom_cloud_photo_ctrl_request_more_info(clock_list->pclock_list[i]);
    }

    /* 服务端空列表在本地等价为默认表盘，避免先删后加造成无效写盘。 */
    default_clock_id = DIVOOM_START_CLOCK_ID;
    effective_clock_num = clock_list->clock_num;
    effective_clock_list = clock_list->pclock_list;
    if(effective_clock_num == 0)
    {
        effective_clock_num = 1;
        effective_clock_list = &default_clock_id;
    }
	divoom_light_app_clock_list_lock();
    divoom_print("[storage_gc] list sync: old=%d server=%d effective=%d\n",
        pdivoom_light_app_handle->base_light.history_num, clock_list->clock_num, (int)effective_clock_num);
    if(effective_clock_num == pdivoom_light_app_handle->base_light.history_num &&
        memcmp(effective_clock_list,
            pdivoom_light_app_handle->base_light.clock_history,
            effective_clock_num * sizeof(effective_clock_list[0])) == 0)
    {
        divoom_print("[storage_gc] list unchanged: no removal candidates generated\n");
		divoom_light_app_clock_list_unlock();
        if(update_flag)
        {
            divoom_light_app_load_save();
        }
        return;
    }

    new_clock_history = divoom_system_malloc(
        effective_clock_num * sizeof(new_clock_history[0]));
    if(new_clock_history == NULL)
    {
		divoom_light_app_clock_list_unlock();
        return;
    }
    memcpy(new_clock_history, effective_clock_list,
        effective_clock_num * sizeof(new_clock_history[0]));

    /* 全量列表只做 ID 差异：删除项移除片段，新增项只生成该表盘。 */
    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
    {
        int32 clock_id = pdivoom_light_app_handle->base_light.clock_history[i];

        if(divoom_light_app_clock_id_in_list(
            effective_clock_list, effective_clock_num, clock_id) == 0)
        {
            divoom_print("[storage_gc] list removed: clock=%d builtin=%d\n", (int)clock_id,
                divoom_light_app_is_default_inner_clock(clock_id));
            if(!divoom_light_app_is_default_inner_clock(clock_id) &&
                divoom_clock_manage_record_removed_clock(clock_id) == 0)
            {
                /* 整批登记失败不提交新列表；已登记的活跃 ID 由回收端保护。 */
                divoom_print("[storage_gc] list sync aborted: clock=%d reason=enqueue_failed\n", (int)clock_id);
                divoom_system_free(new_clock_history);
                divoom_light_app_clock_list_unlock();
                return;
            }
        }
    }
    for(i = 0; i < effective_clock_num; i++)
    {
        int32 clock_id = effective_clock_list[i];

        if(divoom_light_app_clock_id_in_list(pdivoom_light_app_handle->base_light.clock_history,
            pdivoom_light_app_handle->base_light.history_num, clock_id) == 0)
        {
            divoom_clock_manage_cancel_removed_clock(clock_id);
            (void)divoom_clock_component_index_refresh(clock_id);
        }
    }

    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
    {
        int32 clock_id = pdivoom_light_app_handle->base_light.clock_history[i];
        if(!divoom_light_app_clock_id_in_list(effective_clock_list, effective_clock_num, clock_id))
        {
            (void)divoom_clock_component_index_remove_clock(clock_id);
        }
    }
    if(pdivoom_light_app_handle->base_light.clock_history)
    {
        divoom_system_free(pdivoom_light_app_handle->base_light.clock_history);
        pdivoom_light_app_handle->base_light.clock_history = NULL;
    }
    pdivoom_light_app_handle->base_light.clock_history = new_clock_history;
    pdivoom_light_app_handle->base_light.history_num = effective_clock_num;
    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == pdivoom_light_app_handle->base_light.clock_id)
        {
            break;
        }
    }
    if(i >= pdivoom_light_app_handle->base_light.history_num)
    {
        pdivoom_light_app_handle->base_light.clock_id = pdivoom_light_app_handle->base_light.clock_history[0];
    }
	divoom_light_app_clock_list_unlock();
    divoom_light_app_load_save();
    
}


/*
  Function:       divoom_light_app_get_change_clock_config
  Description:    得到表盘配置
  Input:          none
  Return:         NONE
*/

void divoom_light_app_get_change_clock_config(Pdivoom_channel_channel_cfg_info pconfig_info)
{
    pconfig_info->ChannelIndex = pdivoom_light_app_handle->base_light.power_on_mode;
    pconfig_info->RotationFlag = pdivoom_light_app_handle->base_light.play_roll;
    pconfig_info->ClockTime = pdivoom_light_app_handle->base_light.clock_disp_time;
    pconfig_info->GalleryTime = pdivoom_light_app_handle->base_light.clock_disp_gif;
    pconfig_info->SingleGalleyTime = pdivoom_light_app_handle->base_light.each_gif_time;
    pconfig_info->GalleryShowTimeFlag = pdivoom_light_app_handle->base_light.show_time_flag;
}


/*
  Function:       divoom_light_app_get_cur_clock_id
  Description:    得到当前播放的表盘
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_cur_clock_id(void)
{
	if (pdivoom_light_app_handle->base_light.clock_id == 0)
		return 10;

    return pdivoom_light_app_handle->base_light.clock_id;
}

/*
  Function:       divoom_light_app_show_plan_clock
  Description:    播放计划/定时表盘统一显示
  Input:          clock_id, voice_flag
  Return:         NONE
*/

void divoom_light_app_show_plan_clock(int clock_id, uint32 voice_flag)
{
    if(pdivoom_light_app_handle == NULL || clock_id <= 0)
    {
        return;
    }

    pdivoom_light_app_handle->schedule_dial_info.disp_mode = 0;
    pdivoom_light_app_handle->schedule_dial_info.clock_id = clock_id;
    if(voice_flag)
    {
        char file_name[128];

        divoom_voice_read_get_filename(DIVOOM_VOICE_ID_TIME_MANAGE, file_name);
        divoom_local_music_ctrl_tone_play(file_name, divoom_main_get_tone_volume(), NULL, 0);
    }
    divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_light_app_handle->schedule_dial_info, DIVOOM_MODEL_CLOCK_PLAY_PLAN, DIVOOM_CLOCK_PLAY_PLAN_APP_EVENT_RUN);
}

/*
  Function:       divoom_light_app_get_user_define_gif_play_time
  Description:    得到自定义动画播放时间
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_user_define_gif_play_time(void *user_data, int index)
{
    return divoom_user_define_ctrl_get_play_time(user_data) * 1000;
}

/*
  Function:       divoom_light_app_get_cloud_gif_play_time
  Description:    得到云图动画播放时间
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_once_gif_play_time(void *user_data, int index)
{
    return 0;
}

/*
  Function:       divoom_light_app_get_cloud_gif_play_time
  Description:    得到云图动画播放时间
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_cloud_gif_play_time(void *user_data, int index)
{
    if(index)
    {
        return 10*1000;
    }
    else
    {
        return divoom_subscribe_manage_get_play_time(user_data) * 1000;
    }
}

/*
  Function:       divoom_light_app_remove_clock_id
  Description:    向表盘列表中移动一个表盘位置
  Input:          clock_id
  Return:         NONE
*/

void divoom_light_app_move_clock_id(int clock_id, int pos)
{
    int i;

	if(pdivoom_light_app_handle == NULL)
	{
		return;
	}
	divoom_light_app_clock_list_lock();
    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == pos)
        {
            pos = i;
            break;
        }
    }
    
    if(i >= pdivoom_light_app_handle->base_light.history_num)
    {
		divoom_light_app_clock_list_unlock();
        return;
    }

    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == clock_id)
        {
            if(pos > i)
            {
                if(pos-i > 1)
                {
                    memmove(&pdivoom_light_app_handle->base_light.clock_history[i], &pdivoom_light_app_handle->base_light.clock_history[i+1],abs(pos-i)*sizeof(int));
                }
            }
            else
            {
                if(i - pos > 1)
                {
                    memmove(&pdivoom_light_app_handle->base_light.clock_history[pos + 1], &pdivoom_light_app_handle->base_light.clock_history[pos],abs(i-pos)*sizeof(int));
                }
            }
            pdivoom_light_app_handle->base_light.clock_history[pos] = clock_id;
			divoom_light_app_clock_list_unlock();
            return;
        }
    }
	divoom_light_app_clock_list_unlock();
}


#if DIVOOM_SHOW_MODE
static int divoom_light_app_show_list_step(int clock_id, int dir)
{
	int i;
	int n;

	n = DIVOOM_SHOW_CLOCK_ROTATE_LIST_NUM;
	if (n <= 0)
		return clock_id;
	if (n == 1)
		return divoom_show_clock_rotate_list[0];
	for (i = 0; i < n; i++)
	{
		if (divoom_show_clock_rotate_list[i] == clock_id)
		{
			if (dir > 0)
				return divoom_show_clock_rotate_list[(i + 1) % n];
			return divoom_show_clock_rotate_list[(i + n - 1) % n];
		}
	}
	return divoom_show_clock_rotate_list[0];
}
#endif

/*
  Function:       divoom_light_app_get_next_clock_id
  Description:    从历史记录中得到下一个播放的表盘
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_next_clock_id(void)
{
    int i;

#if DIVOOM_SHOW_MODE
    return divoom_light_app_show_list_step(pdivoom_light_app_handle->base_light.clock_id, 1);
#endif

    divoom_print("clock history number: %d\n",pdivoom_light_app_handle->base_light.history_num);
    if(pdivoom_light_app_handle->base_light.history_num == 0)
    {
		if(pdivoom_light_app_handle->base_light.clock_id == 0)
		{
			return DIVOOM_DEFAULT_CLOCK_ID;
		}
		else
		{
			return pdivoom_light_app_handle->base_light.clock_id;
		}
    }
    
    if(pdivoom_light_app_handle->base_light.clock_id == 0)
    {
        return pdivoom_light_app_handle->base_light.clock_history[0];
    }
    
    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i ++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == pdivoom_light_app_handle->base_light.clock_id)
        {
            if(i + 1 < pdivoom_light_app_handle->base_light.history_num)
            {
                return pdivoom_light_app_handle->base_light.clock_history[i + 1];
            }
            else
            {
                return pdivoom_light_app_handle->base_light.clock_history[0];
            }
        }
    }
    return pdivoom_light_app_handle->base_light.clock_history[0];
}

int divoom_light_app_get_next_clock_from_id(int clock_id)
{
    int i;
#if DIVOOM_SHOW_MODE
    return divoom_light_app_show_list_step(clock_id, 1);
#endif
//    divoom_print("clock history number: %d\n",pdivoom_light_app_handle->base_light.history_num);
    if (pdivoom_light_app_handle->base_light.history_num <= 1)
    {
		return 0;
    }
    
    for (i = 0; i < pdivoom_light_app_handle->base_light.history_num; i ++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == clock_id)
        {
            if(i + 1 < pdivoom_light_app_handle->base_light.history_num)
            {
                return pdivoom_light_app_handle->base_light.clock_history[i + 1];
            }
            else
            {
                return pdivoom_light_app_handle->base_light.clock_history[0];
            }
        }
    }
    return pdivoom_light_app_handle->base_light.clock_history[0];
}

int divoom_light_app_get_next_clock2_from_id(int clock_id)
{
    int i;
//    divoom_print("clock history number: %d\n",pdivoom_light_app_handle->base_light.history_num);
    if (pdivoom_light_app_handle->base_light.history_num <= 1)
    {
		return 0;
    }
    
    for (i = 0; i < pdivoom_light_app_handle->base_light.history_num; i ++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == clock_id)
        {
            if(i + 1 < pdivoom_light_app_handle->base_light.history_num)
            {
                return pdivoom_light_app_handle->base_light.clock_history[i + 1];
            }
            else
            {
                return 0;
            }
        }
    }
    return 0;
}

/*
  Function:       divoom_light_app_get_pre_clock_id
  Description:    从历史记录中得到前一个播放的表盘
  Input:          none
  Return:         NONE
*/

int divoom_light_app_get_pre_clock_id(void)
{
    int i;

#if DIVOOM_SHOW_MODE
    return divoom_light_app_show_list_step(pdivoom_light_app_handle->base_light.clock_id, 0);
#endif
    divoom_print("clock history number: %d\n",pdivoom_light_app_handle->base_light.history_num);
    if(pdivoom_light_app_handle->base_light.history_num <= 1)
    {
        if(pdivoom_light_app_handle->base_light.clock_id == 0)
        {
            return 10;
        }
        else
        {
            return pdivoom_light_app_handle->base_light.clock_id;
        }
    }


    if(pdivoom_light_app_handle->base_light.clock_id == 0)
    {
        return pdivoom_light_app_handle->base_light.clock_history[0];
    }

    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i ++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == pdivoom_light_app_handle->base_light.clock_id)
        {
            if(i > 0)
            {
                return pdivoom_light_app_handle->base_light.clock_history[i - 1];
            }
            else
            {
                return pdivoom_light_app_handle->base_light.clock_history[pdivoom_light_app_handle->base_light.history_num - 1];
            }
        }
    }
    return pdivoom_light_app_handle->base_light.clock_history[0];
}

int divoom_light_app_get_pre_clock_from_id(int clock_id)
{
    int i, index = 0;

#if DIVOOM_SHOW_MODE
    return divoom_light_app_show_list_step(clock_id, 0);
#endif
//    divoom_print("clock history number: %d\n",pdivoom_light_app_handle->base_light.history_num);
    if(pdivoom_light_app_handle->base_light.history_num < 1)
    {
    	return -1;
    }

	index = pdivoom_light_app_handle->base_light.history_num -1;
    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i ++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == clock_id)
        {
            if(i > 0)
            {
                return pdivoom_light_app_handle->base_light.clock_history[i - 1];
            }
            else
            {
				return pdivoom_light_app_handle->base_light.clock_history[index]; 
            }
        }
    }
	
    return pdivoom_light_app_handle->base_light.clock_history[0];
}

int divoom_light_app_get_pre_clock2_from_id(int clock_id)
{
    int i;

//    divoom_print("clock history number: %d\n",pdivoom_light_app_handle->base_light.history_num);
    if(pdivoom_light_app_handle->base_light.history_num < 1)
    {
    	return -1;
    }

    for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i ++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == clock_id)
        {
            if(i > 0)
            {
                return pdivoom_light_app_handle->base_light.clock_history[i - 1];
            }
            else
            {
				return -1; 
            }
        }
    }
	
	if (clock_id == 0)
	{
		i--;
		return pdivoom_light_app_handle->base_light.clock_history[i];
	}
    return -1;
}

int divoom_light_app_notice_end(uint8 mode)
{
    //发异步通知开机事件完成
    divoom_sys_msg_info model_msg = {0};
    divoom_print("notice pic finished!\n");

    model_msg.src_id = DIVOOM_MODEL_LIGHT_APP;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;  
    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_END_NOTICE_MSG_MODE;
    model_msg.msg_type = 1;
    divoom_sys_model_crl_msg_push(&model_msg);
    return 1;
    
}

void divoom_light_app_tf_gif_load_all(char *path)
{
    divoom_disp_ctrl_local_gif_info gif_info;
    Pdivoom_file_list_info file_item;
    void *all_gif_handle;

    divoom_print("we will play gif folder: %s\n", path);
    all_gif_handle = divoom_file_manage_get_list(path, ".gif", NULL);
    if(all_gif_handle == NULL)
    {
        divoom_print("the gif folder is empty: %s\n", path);
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_LOCAL_GIF_MODE, NULL);
        return;
    }
    gif_info.flag = 1;
    while(divoom_queue_items(all_gif_handle))
    {
        file_item = divoom_queue_pop(all_gif_handle);
        if(file_item)
        {
            gif_info.file_name = file_item->filename;
            divoom_print("add gif file: %s\n", file_item->filename);
            divoom_disp_ctrl_set_mode(DIVOOM_DISP_LOCAL_GIF_MODE, &gif_info);
            gif_info.flag = 0;
            divoom_system_free(file_item);
        }
    }

    divoom_queue_destroy(all_gif_handle, NULL);
    
}

int divoom_light_app_check_clock_server_exist(int clock_id)
{
	return 1;
}

//获取设备存在表盘配置的列表
int *divoom_light_app_get_normal_clock_list_exist(int *clock_num)
{
	int clock_id, i, clock_file_count = 0, *clock_list;
    DIR *clock_dir_handle;
    Pdivoom_clock_manage_item pclock_info;

	(*clock_num) = 0;
	clock_file_count = 0;

	clock_dir_handle = divoom_port_dir_open(DIVOOM_SD_SYSTEM_SYS_CLOCKS_DIR);
    if(clock_dir_handle)
    {
		const divoom_port_dirent_t *dirp;
		
		while((dirp = divoom_port_dir_read(clock_dir_handle)) != NULL)
        {
            //divoom_print("find: type: %d, %s\n", dirp->d_type, dirp->name);
            if(dirp->is_dir == 0)
            {
				clock_id = 0;
                sscanf(dirp->name, "clock%d.cfg", &clock_id);

				if(divoom_light_app_check_clock_server_exist(clock_id) == 0)
				{
					continue;
				}

                if(clock_id)
                {
                    pclock_info = divoom_clock_manage_get_clock_info_no_download(clock_id, 0, 0);
                    if(pclock_info && pclock_info->dial_type == 0)
                    {
						clock_file_count++;
                    }
                    if(pclock_info)
                    {
                        divoom_clock_manage_free_clock(pclock_info);
                    }
                }
				else
				{
					break;
				}
            }
        }
        
		clock_list = (int *)divoom_system_malloc(sizeof(int) * clock_file_count);		
		(*clock_num) = 0;
		rewinddir(clock_dir_handle);

		while((*clock_num) < clock_file_count)
		{
			dirp = divoom_port_dir_read(clock_dir_handle);

			if(dirp == NULL)
			{
				break;
			}

			if(dirp->is_dir == 0)
            {
				clock_id = 0;
                sscanf(dirp->name, "clock%d.cfg", &clock_id);

				if(divoom_light_app_check_clock_server_exist(clock_id) == 0)
				{
					continue;
				}

                if(clock_id)
                {
                    pclock_info = divoom_clock_manage_get_clock_info_no_download(clock_id, 0, 0);
                    if(pclock_info && pclock_info->dial_type == 0)
                    {
						clock_list[*clock_num] = pclock_info->clock_id; 
						(*clock_num)++;
                    }
                    if(pclock_info)
                    {
                        divoom_clock_manage_free_clock(pclock_info);
                    }
                }
				else
				{
					break;
				}
            }
		}

        divoom_port_dir_close(clock_dir_handle);
		return clock_list;
	}

	return NULL;
}

//检查表盘是否存在，如果存在就返回自己，否则返回一个已经存在的表盘
int divoom_light_app_get_normal_clock_exist(void)
{
    int clock_id;
    DIR *clock_dir_handle;
    Pdivoom_clock_manage_item pclock_info;
    

    //查找目录下面的所有图片文件
    clock_dir_handle = divoom_port_dir_open(DIVOOM_SD_SYSTEM_SYS_CLOCKS_DIR);
    if(clock_dir_handle)
    {
        const divoom_port_dirent_t *dirp;

        while((dirp = divoom_port_dir_read(clock_dir_handle)) != NULL)
        {
            divoom_print("find: type: %d, %s\n", dirp->is_dir, dirp->name);
            if(dirp->is_dir == 0)
            {
                sscanf(dirp->name, "clock%d.cfg", &clock_id);
                if(clock_id)
                {
                    pclock_info = divoom_clock_manage_get_clock_info(clock_id, 0, 0);
                    if(pclock_info && pclock_info->dial_type == 0)
                    {
                        divoom_clock_manage_free_clock(pclock_info);
                        divoom_port_dir_close(clock_dir_handle);
                        divoom_print("find clock: %d\n", clock_id);
                        return clock_id;
                    }
                    if(pclock_info)
                    {
                        divoom_clock_manage_free_clock(pclock_info);
                    }
                }
            }
        }
        
        divoom_port_dir_close(clock_dir_handle);
    }
    else
    {
        return 0;
    }
    return 0;
}
int divoom_light_app_check_clock_exist(int clock_id)
{
    Pdivoom_clock_manage_item pclock_info;
    /* Existence is local; network refresh must not hold the display lock at boot. */

    if(clock_id)
    {
        pclock_info = divoom_clock_manage_get_raw_clock_info(clock_id, 0, 0);
        if(pclock_info)
        {
            divoom_clock_manage_free_clock(pclock_info);
            return clock_id;
        }
    }
    //查找表盘列表
    for(int i = 0; pdivoom_light_app_handle->base_light.clock_history && i < pdivoom_light_app_handle->base_light.history_num; i++)
    {
        pclock_info = divoom_clock_manage_get_raw_clock_info(pdivoom_light_app_handle->base_light.clock_history[i], 0, 0);
        if(pclock_info && pclock_info->clock_type != DIVOOM_CLOCK_TYPE_DIVOOM_PHOTO_DIAL)
        {
            divoom_clock_manage_free_clock(pclock_info);
            return pdivoom_light_app_handle->base_light.clock_history[i];
        }
    }
    return divoom_light_app_get_normal_clock_exist();
    
}


/*
  Function:       divoom_light_app_set_divoom_animation_edge_clock_id
  Description:    设置默认边框
  Input:          clock_id
  Return:         NONE
*/

void divoom_light_app_set_divoom_animation_edge_clock_id(int clock_id)
{
    if(pdivoom_light_app_handle->base_light.divoom_app_animation_edge_id != clock_id)
    {
        pdivoom_light_app_handle->base_light.divoom_app_animation_edge_id = clock_id;

        divoom_sys_msg_info model_msg = {0}; 
        model_msg.msg_type = 1;         
        model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
        model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_GIF_EDGE_CLOCK;
        divoom_light_app_load_save();
        divoom_sys_model_crl_msg_push(&model_msg);
    }
}

int divoom_light_app_get_divoom_animation_edge_clock_id(void)
{
    return pdivoom_light_app_handle->base_light.divoom_app_animation_edge_id;
}

int divoom_http_download_image_check_image_is_black(char *file_path);
int divoom_light_app_check_image_is_block(int model_id, int image_id)
{
    char file_name[128];
    divoom_manage_image_download_get_file_name(model_id, image_id, file_name);
    if(divoom_http_download_image_check_image_is_black(file_name))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void *divoom_light_app_get_current_clock_info(void)
{
    if(pdivoom_light_app_handle->base_light.clock_info.clock_id == 0)
    {
        pdivoom_light_app_handle->base_light.clock_info.clock_id = pdivoom_light_app_handle->base_light.clock_id;
    }
    return &pdivoom_light_app_handle->base_light.clock_info;
}
int divoom_light_app_start_disp(void)
{
    int update_flag = 0;

    divoom_print("disp_mode: %d\n", pdivoom_light_app_handle->cur_mode);
    if(pdivoom_light_app_handle->cur_mode != DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE)
    {
        pdivoom_light_app_handle->wait_gif_time = 0;
    }
    switch(pdivoom_light_app_handle->cur_mode)
    {
        case DIVOOM_LIGHT_APP_DISP_ENTER_CUSTOM_DIAL:
			pdivoom_light_app_handle->schedule_dial_info.clock_id = DIVOOM_USER_DEFINE_CLOCK_ID;
			pdivoom_light_app_handle->schedule_dial_info.disp_mode = 0;
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_light_app_handle->schedule_dial_info, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_EVENT);
            break;
            
        case DIVOOM_LIGHT_APP_DISP_ENTER_OUTLOOK_CALENDAR:
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_OUTLOOK_CALENDAR_MENU, NULL, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_EVENT);
            break;
            
        case DIVOOM_LIGHT_APP_DISP_ENTER_GOOGLE_CALENDAR:
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_GOOGLE_CALENDAR_MENU, (void *)&pdivoom_light_app_handle->schedule_dial_info, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_EVENT);
            break;
            
        case DIVOOM_LIGHT_APP_DISP_OUTLOOK_CALENDAR_MODE:
            divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_OUTLOOK_CALENDAR_ACTIVE_MENU, NULL, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT);
            break;
        
        case DIVOOM_LIGHT_APP_DISP_GOOGLE_CALENDAR_MODE:
            divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_GOOGLE_CALENDAR_ACTIVE_MENU,  (void *)&pdivoom_light_app_handle->schedule_dial_info, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT);
            break;
            
        case DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE:
            divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW,
                (void *)(uintptr_t)divoom_light_app_get_clock_download_session(),
                DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT);
            break;

        case DIVOOM_LIGHT_APP_DISP_FILE_TRANSFER_MODE:
            divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_FILE_TRANSFER, NULL, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT);
            break;
            
        case DIVOOM_LIGHT_APP_DISP_TF_GIF_MODE:
            break;
            
        case DIVOOM_LIGHT_APP_DISP_IFFT_TEXT_MODE:
            
            break;
            
        case DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE:
        
            break;

        case DIVOOM_LIGHT_APP_DISP_HTTP_GIF_MODE:
            break;
            
        case DIVOOM_LIGHT_APP_DISP_PLAYLIST_MODE:
			pdivoom_light_app_handle->schedule_dial_info.clock_id = DIVOOM_USE_IMAGE_LIST_CLOCK_ID;
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_light_app_handle->schedule_dial_info, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_EVENT);
            break;

        case DIVOOM_LIGHT_APP_DISP_MIXER_MODE:
        {
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_MIXER, NULL,
                DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_MIXER_EVENT);
            break;
        }

        case DIVOOM_LIGHT_APP_DISP_MIX_MUSIC2_MODE:
        {
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_MIX_MUSIC2, NULL,
                DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_MIX_MUSIC2_EVENT);
            break;
        }
            
        case DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE:
        case DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE:
        case DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE:
			pdivoom_light_app_handle->schedule_dial_info.clock_id = DIVOOM_ANIMATION_CLOCK_ID;
			pdivoom_light_app_handle->schedule_dial_info.disp_mode = 0;
            divoom_main_set_global_image_handle(DIVOOM_MANAGE_IMAGE_SERVER_GIF,
                pdivoom_light_app_handle->server_gif_file_id,
                strlen(pdivoom_light_app_handle->remote_image_info.voice_id));
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_light_app_handle->schedule_dial_info, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_EVENT);
            pdivoom_light_app_handle->cur_play_image_info.author_id = pdivoom_light_app_handle->remote_image_info.author_id;
            pdivoom_light_app_handle->cur_play_image_info.author_update_time = time(0); 
            pdivoom_light_app_handle->cur_play_image_info.black_flag = divoom_light_app_check_image_is_block(
                DIVOOM_MANAGE_IMAGE_SERVER_GIF, pdivoom_light_app_handle->server_gif_file_id);
            pdivoom_light_app_handle->cur_play_image_info.author_load_flag = 0;
            strcpy(pdivoom_light_app_handle->cur_play_image_info.image_name, pdivoom_light_app_handle->remote_image_info.image_name);
            strcpy(pdivoom_light_app_handle->cur_play_image_info.author_name, pdivoom_light_app_handle->remote_image_info.author_name);
            break;
        case DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_MODE:
        {

            pdivoom_light_app_handle->schedule_dial_info.disp_mode = 0;
            pdivoom_light_app_handle->schedule_dial_info.clock_id = pdivoom_light_app_handle->schedule_clock_info.clock_id;
            if(pdivoom_light_app_handle->schedule_clock_info.voice_flag)
            {
                //后面这里增加一下提示音
                char file_name[128];
                
                divoom_voice_read_get_filename(DIVOOM_VOICE_ID_TIME_MANAGE, file_name);
                divoom_local_music_ctrl_tone_play(file_name, divoom_main_get_tone_volume(), NULL, 0);
            }
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_light_app_handle->schedule_dial_info, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_EVENT);
        }
            break;
            
        case DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE:
            
#ifdef DIVOOM_GET_DEFAULT_ALL_CLOCK_INFO
            divoom_light_app_load_all_default_dail_info();
#endif
            divoom_print("base_mode: %d, %d\n", pdivoom_light_app_handle->base_light.mode, pdivoom_light_app_handle->base_light.clock_id);  
            pdivoom_light_app_handle->watch_dog_flag = 0;
            if(pdivoom_light_app_handle->base_light.clock_id != pdivoom_light_app_handle->base_light.pre_clock_id)
            {
                pdivoom_light_app_handle->base_light.pre_clock_id = pdivoom_light_app_handle->base_light.clock_id;
                divoom_light_app_load_save();
            }
            pdivoom_light_app_handle->base_light.clock_id = divoom_light_app_check_clock_exist(pdivoom_light_app_handle->base_light.clock_id);
            if(pdivoom_light_app_handle->base_light.clock_id == 0)
            {
                break;
            }
			pdivoom_light_app_handle->base_light.clock_info.clock_id = pdivoom_light_app_handle->base_light.clock_id;
            if(pdivoom_light_app_handle->base_light.clock_info.clock_id != pdivoom_light_app_handle->base_light.clock_id)
            {
                update_flag = 1;
            }
			pdivoom_light_app_handle->base_light.clock_info.disp_mode = pdivoom_light_app_handle->base_light.clock_mode;
            {
                Pdivoom_clock_manage_item pclock_info = divoom_clock_manage_get_raw_clock_info(pdivoom_light_app_handle->base_light.clock_info.clock_id,  0, 0);

                if(pclock_info == NULL)
                {
                    pdivoom_light_app_handle->base_light.clock_info.clock_id = pdivoom_light_app_handle->base_light.clock_history[0];
                }
                else
                {
                    if(divoom_net_app_net_is_ok())
                    {
                        if(strlen(pclock_info->review_addr) == 0)
                        {
                            divoom_clock_id_info clock_info = {0 };
                            clock_info.clock_id = pdivoom_light_app_handle->base_light.clock_info.clock_id;
                            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA, DIVOOM_SERVER_TOPIC,&clock_info);
                        }
                    }
                    divoom_clock_manage_free_clock(pclock_info);
                }
            
            }
            /*
             * 本地：表盘→工具→番茄，栈里还有上一页。cancel 同步 play KEY_LIGHT
             * 时 PRE 未处理，clearall 会把工具栈清掉。只恢复 RGB，不改菜单。
             * App 启动：clearall 进番茄，栈仅一页。仍走 clearall/START_UP_DISP_DIAL 回表盘。
             */
            if (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_POMODORO_MENU
                && divoom_menu_control_menu_items() > 1)
            {
                divoom_print("light app: skip dial clearall, keep pomodoro menu stack\n");
                break;
            }
            /* 仅 APP 主动切表盘等用 clearall；开机/闹钟黑屏退出(app_request_flag==2)用带 pop 的下一页，避免 clearall 与唤醒页并发崩 */
            if (pdivoom_light_app_handle->app_request_flag == 1 || pdivoom_light_app_handle->app_request_flag == 2)
            {
                divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_light_app_handle->base_light.clock_info, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT);
            }
            else
            {
                divoom_menu_control_load_next_with_disp(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_light_app_handle->base_light.clock_info, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT, 1);
            }
         break;
            
		case DIVOOM_LIGHT_APP_DISP_DEMO_DIAL_MODE:
		  	divoom_menu_control_load_next_with_disp(DIVOOM_MENU_CONTROL_ID_DEMO_DIAL_MENU, NULL, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_DEMO_DIAL_DISP, 1);
		    break;
    }
    return update_flag;
}


/*
  Function:       divoom_light_app_clock_notice_event
  Description:    某个表盘的通知事件，
  Input:          pclock_info
                  msg
                  up_down_flag
                  notice_value
  Return:         none
*/


void divoom_light_app_clock_notice_event(Pdivoom_clock_id_info pclock_info, char *msg, int up_down_flag,double notice_value)
{
    divoom_print("notice clock info: %d, %d, %d; msg:%s; up: %d, notice: %f\n", pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, msg, up_down_flag, (float)notice_value);

}

int divoom_light_app_set_clear_bg_clock(int clock_id, int cur_clock)
{
    if(clock_id  < 0)
    {
        /* 登记下一次进入该表盘时强制清背景，避免菜单退出后局部刷新表盘残留上一屏内容。 */
        pdivoom_light_app_handle->clear_bg_clock = clock_id;
        return clock_id;
    }
    if(cur_clock == pdivoom_light_app_handle->clear_bg_clock)
    {
        clock_id = pdivoom_light_app_handle->clear_bg_clock;
        pdivoom_light_app_handle->clear_bg_clock  = 0;
        return 1;
    }
    else
    {
        return 0;
    }
}


uint8 divoom_light_app_request_resource(int cur_mode)
{
    int event_id = 0, update_flag = 0;
    uint32 transfer_finished = 0;
    uint32 transfer_total = 0;
    divoom_app_schedule_item_info channel_schedule = {0};

    /*
     * 达人等列表可能连续返回多批文件。下载总批次尚未完成时，
     * Channel/SetClockSelectId 等普通表盘刷新不能覆盖文件传输页。
     */
    if(cur_mode == DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE &&
        ((divoom_manage_image_get_transfer_progress(
            &transfer_finished, &transfer_total) &&
            transfer_finished < transfer_total) ||
            divoom_subscribe_manage_transfer_request_is_pending()))
    {
        if(pdivoom_light_app_handle->file_transfer_exit_requested)
        {
            /* 只绕过本次切回表盘；下载队列仍在后台继续消费。 */
            pdivoom_light_app_handle->file_transfer_exit_requested = 0;
        }
        else
        {
            if(pdivoom_light_app_handle->cur_mode ==
                    DIVOOM_LIGHT_APP_DISP_FILE_TRANSFER_MODE &&
                pdivoom_light_app_handle->active_flag &&
                divoom_menu_control_get_active_menu_id() ==
                    DIVOOM_MENU_CONTROL_ID_FILE_TRANSFER)
            {
                divoom_print("light app: keep file transfer %u/%u, skip base light\n",
                    transfer_finished, transfer_total);
                return 1;
            }
            cur_mode = DIVOOM_LIGHT_APP_DISP_FILE_TRANSFER_MODE;
        }
    }
    
    if(pdivoom_light_app_handle->cur_mode != cur_mode || !pdivoom_light_app_handle->active_flag)
    {
        if(cur_mode == DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE &&
            divoom_clock_play_plan_is_holding_display())
        {
            divoom_print("%s: skip base light, play plan holding display\n", __func__);
            return 0;
        }
        channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT; 
        switch(cur_mode)
        {
            case DIVOOM_LIGHT_APP_DISP_PLAYLIST_MODE:
                event_id = DIVOOM_LIGHT_APP_PLAY_LIST_EVENT;
                break;

            case DIVOOM_LIGHT_APP_DISP_MIXER_MODE:
            {
                event_id = DIVOOM_LIGHT_APP_MIXER_EVENT;
                break;
            }

            case DIVOOM_LIGHT_APP_DISP_MIX_MUSIC2_MODE:
            {
                event_id = DIVOOM_LIGHT_APP_MIX_MUSIC2_EVENT;
                break;
            }
                
            case DIVOOM_LIGHT_APP_DISP_ENTER_CUSTOM_DIAL:
                event_id = DIVOOM_LIGHT_APP_CUSTOM_DIAL_DISP;
                break;
                
            case DIVOOM_LIGHT_APP_DISP_ENTER_OUTLOOK_CALENDAR:
                event_id = DIVOOM_LIGHT_APP_OUTLOOK_CALENDAR_DISP;
                break;
            
            case DIVOOM_LIGHT_APP_DISP_ENTER_GOOGLE_CALENDAR:
                event_id = DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_DISP;
                break;

            case DIVOOM_LIGHT_APP_DISP_OUTLOOK_CALENDAR_MODE:
            case DIVOOM_LIGHT_APP_DISP_GOOGLE_CALENDAR_MODE:
                event_id = DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_EVENT;
                break;
                
            case DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE:
            case DIVOOM_LIGHT_APP_DISP_FILE_TRANSFER_MODE:
                channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;  
                event_id = DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT;
                break;
                
            case DIVOOM_LIGHT_APP_DISP_TF_GIF_MODE:
                event_id = DIVOOM_LIGHT_APP_TF_GIF_EVENT;
                break;
                
            case DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE:
                event_id = DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT;
                break;

            case DIVOOM_LIGHT_APP_DISP_HTTP_GIF_MODE:
                event_id = DIVOOM_LIGHT_APP_HTTP_GIF_CTRL_EVENT;
                break;
            
            case DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE:
                event_id = DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT;
                break;
                
            case DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE:
                event_id = DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT;
                break;
                
            case DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE:    
                channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;         
                event_id = DIVOOM_LIGHT_APP_NOTICE_MSG_EVENT;
                break;

			case DIVOOM_LIGHT_APP_DISP_IFFT_AMAZON_MUSIC:
            case DIVOOM_LIGHT_APP_DISP_IFFT_TEXT_MODE:    
                channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;         
                event_id = DIVOOM_LIGHT_APP_IFFT_TEXT_EVENT;
                break;
				
            case DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_MODE:
                event_id = DIVOOM_LIGHT_APP_SCHEDULE_CLOCK_EVENT;
                break;
                   
			case DIVOOM_LIGHT_APP_DISP_DEMO_DIAL_MODE:
				event_id = DIVOOM_LIGHT_APP_DEMO_DIAL_DISP;
				break;
			
            default:
                event_id = DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT;
                break;
                
        }
        pdivoom_light_app_handle->cur_mode = cur_mode;
        channel_schedule.model_id= DIVOOM_MODEL_LIGHT_APP;  
        channel_schedule.event_id = event_id;  
        channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        pdivoom_light_app_handle->resrouce_type =divoom_app_schedule_push(&channel_schedule);
        if( 0 == pdivoom_light_app_handle->resrouce_type)
        {
            divoom_print("%s: we will pass the event \n", __func__);
            return 0;
        }
    }
    pdivoom_light_app_handle->active_flag = 1;
    if(divoom_light_app_start_disp())
    {
        update_flag = 1;
    }
    pdivoom_light_app_handle->app_request_flag = 0;
    return 1;
}

void *divoom_light_app_local_malloc(void *msg_data)
{
    Pdivoom_sys_msg_info msg_info;

    msg_info = divoom_malloc(sizeof(divoom_sys_msg_info), DIVOOM_MODEL_NET, __LINE__, __FILE__);
    if(msg_info == NULL)
    {
        return NULL;
    }
    memcpy(msg_info, msg_data, sizeof(divoom_sys_msg_info));
    return msg_info;
}

void *divoom_light_app_local_free(void *msg_data)
{
    Pdivoom_sys_msg_info msg_info = (Pdivoom_sys_msg_info)msg_data;
	
	if(msg_info->data)
		divoom_free(msg_info->data);
    
    divoom_free(msg_info);
    return NULL;
}

/*
  Function:       divoom_light_app_local_gif_notice
  Description:    本地图片下载完接收通知
  Input:          status 下载状态
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_light_app_local_gif_notice(int status)
{
	if (status > 0)
	{
        divoom_sys_msg_info model_msg = {0}; 
    	uint8 *type = (uint8 *)divoom_system_malloc(sizeof(uint8));

        if(status == 1)
        {
		    *type = 1;
        }
        else
        {
		    *type = 0;
        }
        model_msg.msg_type = 1;         
        model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
        model_msg.data = type;  
        model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_DATA_MODE;
        divoom_sys_model_crl_msg_push(&model_msg);

	}
}

/*
  Function:       divoom_light_app_http_gif_notice
  Description:    http图片下载完接收通知
  Input:          status 下载状态
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_light_app_http_gif_notice(int status)
{
	if (status > 0)
	{
        divoom_sys_msg_info model_msg = {0}; 
    	uint8 *type = (uint8 *)divoom_system_malloc(sizeof(uint8));

        if(status == 1)
        {
		    *type = 1;
        }
        else
        {
		    *type = 0;
        }
        model_msg.msg_type = 1;         
        model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
        model_msg.data = type;  
        model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_HTTP_GIF_MODE;
        divoom_sys_model_crl_msg_push(&model_msg);

	}
}

/*
  Function:       divoom_light_app_real_eq_gif_notice
  Description:    实时ＥＱ图片下载完接收通知
  Input:          status 下载状态
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_light_app_real_eq_gif_notice(int status)
{
	if (status > 0)
	{
        divoom_sys_msg_info model_msg = {0}; 
        model_msg.msg_type = 1;         
        model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
        model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_REAL_EQDATA_MODE;
        divoom_sys_model_crl_msg_push(&model_msg);

	}
}

/*
  Function:       divoom_light_app_save_real_draw_pic_data
  Description:    保存当前画板图片
  Input:          none
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_light_app_save_real_draw_pic_data(void)
{
	uint8 *disp_pic_buf = divoom_image_with_text_get_disp_rgb(0);

	if (disp_pic_buf != NULL)
		memcpy(pdivoom_light_app_handle->draw_info.disp_pic_buf, disp_pic_buf, SCREEN_BUF_SIZE);
}

/*
  Function:       divoom_light_app_exit_drawing_pad
  Description:    退出实时画板
  Input:          none
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_light_app_exit_drawing_pad(T_VOID)
{
    T_U32 i;

    if(pdivoom_light_app_handle->cur_play_image_info.black_flag)
    {        
        divoom_app_schedule_item_info event_info = {0};
        event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
		event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        event_info.event_id = DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT;
        divoom_app_schedule_cancel(&event_info);
        event_info.event_id = DIVOOM_LIGHT_APP_HTTP_GIF_CTRL_EVENT;
        divoom_app_schedule_cancel(&event_info);
        event_info.event_id = DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT;
        divoom_app_schedule_cancel(&event_info);
    }
}

/*
  Function:       divoom_light_app_get_cur_image_name
  Description:    获取当前播放图像的文件名
  Input:          none
  Return:         NONE
*/

char *divoom_light_app_get_cur_image_name(void)
{
    return pdivoom_light_app_handle->cur_play_image_info.image_name;
}

/*
  Function:       divoom_light_app_get_app_image_name
  Description:    获取当前APP发送播放图像的文件名
  Input:          none
  Return:         NONE
*/

char *divoom_light_app_get_app_image_name(void)
{
    return pdivoom_light_app_handle->remote_image_info.image_name;
}

/*
  Function:       divoom_light_app_get_app_author_id
  Description:    获取当前APP发送播放图像的作者ID
  Input:          none
  Return:         ID
*/

int divoom_light_app_get_app_author_id(void)
{
    return pdivoom_light_app_handle->remote_image_info.author_id;
}

/*
  Function:       divoom_light_app_get_cur_author_name
  Description:    获取当前播放图像的作者名称
  Input:          none
  Return:         NONE
*/

char *divoom_light_app_get_cur_author_name(void)
{
    if(pdivoom_light_app_handle->cur_play_image_info.author_load_flag)
    {
        divoom_user_head_manage_get_info(pdivoom_light_app_handle->cur_play_image_info.author_id, pdivoom_light_app_handle->cur_play_image_info.author_name, NULL);
        pdivoom_light_app_handle->cur_play_image_info.author_load_flag = 0;
    }
    return pdivoom_light_app_handle->cur_play_image_info.author_name;
}

/*
  Function:       divoom_light_app_get_cur_author_name
  Description:    获取当前播放图像的作者ID
  Input:          none
  Return:         NONE
*/

int divoom_light_app_set_cur_author_id(int user_id, int *update_time)
{
    if(user_id)
    {
        if(pdivoom_light_app_handle->cur_play_image_info.author_id != user_id)
        {
            pdivoom_light_app_handle->cur_play_image_info.author_id = user_id;
            pdivoom_light_app_handle->cur_play_image_info.author_load_flag = 1;
        }
        pdivoom_light_app_handle->cur_play_image_info.author_update_time = time(0); 
    }
    else if(update_time)
    {
        *update_time = pdivoom_light_app_handle->cur_play_image_info.author_update_time;
    }
    return pdivoom_light_app_handle->cur_play_image_info.author_id;
}

/*
  Function:       divoom_light_app_get_cur_author_name
  Description:    获取当前播放图像的作者ID
  Input:          none
  Return:         NONE
*/

int divoom_light_app_set_cur_image_id(int image_id)
{
    if(image_id)
    {
        if(pdivoom_light_app_handle->cur_play_image_info.image_id != image_id)
        {
            //divoom_print("set image: %d\n", image_id);
            pdivoom_light_app_handle->cur_play_image_info.image_id = image_id;
        }
    }
    return pdivoom_light_app_handle->cur_play_image_info.image_id;
}

/*
  Function:       divoom_light_app_exit_wait_gif_status
  Description:    退出等待动画的状态
  Input:          none
  Return:         NONE
*/


void divoom_light_app_exit_wait_gif_status(void)
{
    int *index;
    divoom_sys_msg_info model_msg = {0}; 

    index=  divoom_system_malloc(sizeof(int));
    if(index == NULL)
    {
        return;
    }

    *index = 1;
    model_msg.msg_type = 1;         
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CHANNEL_INDEX;
    model_msg.data = (uint8 *)index;
    divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_light_app_download_gif_file_finish(Pdivoom_web_get_msg_info pmsg, void *para)
{
    divoom_sys_msg_info model_msg = {0};

    if(pmsg)
    {
    	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_PLAY_NET_GIF_FILE;
        model_msg.msg_type = 1;
    	divoom_sys_model_crl_msg_push(&model_msg);
    }
    else
    {
        char *file_name;
        
        file_name = divoom_queue_pop(pdivoom_light_app_handle->gif_file_queue);
        if(file_name)
        {
            divoom_print("it fails to download file: %s\n", file_name);
            divoom_system_free(file_name);
        }
        if(divoom_queue_items(pdivoom_light_app_handle->gif_file_queue) == 0)
        {
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
        }
    }
}

/*
  Function:       divoom_light_app_get_vision_play_time
  Description:    获取播放时间
  Input:          none
  Return:         time
*/

int divoom_light_app_get_vision_play_time(void)
{
    if(pdivoom_light_app_handle->base_light.vision_play_time < 10)
    {
        return 10;
    }
    return pdivoom_light_app_handle->base_light.vision_play_time;
}

/*
  Function:       divoom_light_app_get_screen_status
  Description:    得到开关屏的相关状态
  Input:          none
  Return:         status
*/

uint8 divoom_light_app_get_screen_status(uint8 flag)
{
    if(pdivoom_light_app_handle == NULL)
    {
        return 1;
    }
    
    if(pdivoom_light_app_handle->light_switch == flag)
    {
        return flag;
    }
    if(flag != 0xFF)
    {
        //将保存到配置中去
        pdivoom_light_app_handle->light_switch = flag;
        divoom_main_set_screen_switch(flag==0);
    }
    return pdivoom_light_app_handle->light_switch;
}
int divoom_light_app_open_screen(void)
{    
    if(pdivoom_light_app_handle->light_switch == 0)
    {
        int brightness;
        divoom_light_app_get_screen_status(1);
        divoom_main_apply_sys_light();

        divoom_btcore_key_light_follow_screen(1);

        divoom_light_app_load_save();
        brightness = 1;
        divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_SCREEN_CTRL, DIVOOM_SERVER_TOPIC, &brightness);
        return 1;
    }
    return 0;
}


/*
  Function:       divoom_light_app_get_suscribe_play_time
  Description:    获取云图播放动画时间
  Input:          
  Return:         
*/

int divoom_light_app_get_suscribe_play_time(void)
{
    return pdivoom_light_app_handle->suscribe_play_time[0] *1000;
}

/*
  Function:       divoom_light_app_request_timeout_resert
  Description:    获取云图播放动画时间
  Input:          
  Return:         
*/

void divoom_light_app_request_timeout_resert(void)
{
    divoom_sys_msg_info model_msg = {0};

	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_RESET_SAVE_CLOCK;
    model_msg.msg_type = 1;
	divoom_sys_model_crl_msg_push(&model_msg);
}

int divoom_light_app_remove_file(int model_id, int image_id, char *path_name, char *net_name)
{
    char file_name[128], src_name[128];

    if(strlen(net_name) == 0)
    {
        return 0;
    }
	divoom_manage_image_download_get_path_name(model_id, image_id, file_name);
	divoom_app_com_mk_dir(file_name);
	divoom_manage_image_download_get_file_name(model_id, image_id, file_name);
    strcpy(src_name, path_name);
    strcat(src_name, "/");
    strcat(src_name, net_name);
    divoom_app_com_del_file(file_name);
    divoom_app_com_rename_file(src_name, file_name);
    if(divoom_port_file_exists(file_name) == 0)
    {
        divoom_print("server gif file stage failed: model=%d, id=%d, src=%s, dst=%s\n",
            model_id, image_id, src_name, file_name);
        return 0;
    }
    divoom_http_client_convert_file_undecode(file_name);
    divoom_print("server gif file staged: model=%d, id=%d, src=%s, dst=%s\n",
        model_id, image_id, src_name, file_name);
    return 1;
}

static uint32 divoom_light_app_server_gif_next_file_id(void)
{
    pdivoom_light_app_handle->server_gif_file_generation++;
    if(pdivoom_light_app_handle->server_gif_file_generation <
        DIVOOM_LIGHT_APP_SERVER_GIF_FIRST_VERSION_ID)
    {
        pdivoom_light_app_handle->server_gif_file_generation =
            DIVOOM_LIGHT_APP_SERVER_GIF_FIRST_VERSION_ID;
    }
    return pdivoom_light_app_handle->server_gif_file_generation;
}

static uint32 divoom_light_app_server_gif_prepare_file(char *path_name, char *net_name)
{
    uint32 new_file_id;
    uint32 *closed_file_id;

    if(pdivoom_light_app_handle == NULL ||
        pdivoom_light_app_handle->server_gif_del_queue == NULL)
    {
        return 0;
    }
    closed_file_id = divoom_system_malloc(sizeof(uint32));
    if(closed_file_id == NULL)
    {
        return 0;
    }
    new_file_id = divoom_light_app_server_gif_next_file_id();
    if(divoom_light_app_remove_file(DIVOOM_MANAGE_IMAGE_SERVER_GIF,
        new_file_id, path_name, net_name) == 0)
    {
        divoom_system_free(closed_file_id);
        return 0;
    }

    *closed_file_id = pdivoom_light_app_handle->server_gif_file_id;
    if(divoom_queue_push(pdivoom_light_app_handle->server_gif_del_queue,
        closed_file_id) == 0)
    {
        char new_file_name[128];

        divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SERVER_GIF,
            new_file_id, new_file_name);
        divoom_app_com_del_file(new_file_name);
        divoom_system_free(closed_file_id);
        return 0;
    }
    pdivoom_light_app_handle->server_gif_file_id = new_file_id;
    divoom_print("server gif version switch queued: old=%u, new=%u, pending=%d\n",
        *closed_file_id, new_file_id,
        divoom_queue_items(pdivoom_light_app_handle->server_gif_del_queue));
    return new_file_id;
}

uint32 divoom_light_app_get_server_gif_file_id(void)
{
    if(pdivoom_light_app_handle == NULL)
    {
        return DIVOOM_LIGHT_APP_SERVER_GIF_INITIAL_FILE_ID;
    }
    return pdivoom_light_app_handle->server_gif_file_id;
}

void divoom_light_app_cleanup_closed_server_gif_files(void)
{
    uint32 *closed_file_id;
    char file_name[128];
    int pending_count;
    int i;

    if(pdivoom_light_app_handle == NULL ||
        pdivoom_light_app_handle->server_gif_del_queue == NULL)
    {
        return;
    }
    pending_count = divoom_queue_items(pdivoom_light_app_handle->server_gif_del_queue);
    for(i = 0; i < pending_count; i++)
    {
        int remove_ret;
        int exists_after;

        closed_file_id = divoom_queue_pop(pdivoom_light_app_handle->server_gif_del_queue);
        if(closed_file_id == NULL)
        {
            break;
        }
        if(*closed_file_id != pdivoom_light_app_handle->server_gif_file_id &&
            *closed_file_id != DIVOOM_LIGHT_APP_SERVER_GIF_VOICE_FILE_ID)
        {
            divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SERVER_GIF,
                *closed_file_id, file_name);
            remove_ret = divoom_port_remove_file(file_name);
            exists_after = divoom_port_file_exists(file_name);
            if(remove_ret != 0 && exists_after)
            {
                /* 删除失败通常表示仍有读取句柄，保留任务等待下一次换帧关闭后重试。 */
                if(divoom_queue_push(pdivoom_light_app_handle->server_gif_del_queue,
                    closed_file_id))
                {
                    divoom_print("server gif delete deferred: id=%u, file=%s, ret=%d, pending=%d\n",
                        *closed_file_id, file_name, remove_ret,
                        divoom_queue_items(pdivoom_light_app_handle->server_gif_del_queue));
                    continue;
                }
                divoom_print("server gif delete retry queue failed: id=%u\n",
                    *closed_file_id);
            }
        }
        divoom_system_free(closed_file_id);
    }
}

static void divoom_light_app_cleanup_server_gif_dir(void)
{
    void *dir;
    const divoom_port_dirent_t *dir_item;
    int cleanup_count = 0;
    int cleanup_fail_count = 0;

    dir = divoom_port_dir_open(DIVOOM_LIGHT_APP_SERVER_GIF_DIR);
    if(dir == NULL)
    {
        return;
    }
    while((dir_item = divoom_port_dir_read(dir)) != NULL)
    {
        char file_name[256];
        int remove_ret;
        int path_len;

        if(dir_item->is_dir || dir_item->name[0] == '.')
        {
            continue;
        }
        path_len = snprintf(file_name, sizeof(file_name), "%s/%s",
            DIVOOM_LIGHT_APP_SERVER_GIF_DIR, dir_item->name);
        if(path_len <= 0 || path_len >= (int)sizeof(file_name))
        {
            divoom_print("server gif startup cleanup path too long: name=%s\n",
                dir_item->name);
            cleanup_fail_count++;
            continue;
        }
        remove_ret = divoom_port_remove_file(file_name);
        if(remove_ret == 0 || divoom_port_file_exists(file_name) == 0)
        {
            cleanup_count++;
        }
        else
        {
            divoom_print("server gif startup cleanup failed: file=%s, ret=%d\n",
                file_name, remove_ret);
            cleanup_fail_count++;
        }
    }
    divoom_port_dir_close(dir);
    divoom_print("server gif startup cleanup end: removed=%d, failed=%d\n",
        cleanup_count, cleanup_fail_count);
}

void divoom_light_app_system_reset_up(void)
{
    divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
	divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
}

/*
  Function:       divoom_light_app_get_gif_change_flag
  Description:    获取APP发的动画属性是否被修改过
  Input:          
  Return:         1 or 0
*/

int divoom_light_app_get_gif_change_flag(void)
{
    return pdivoom_light_app_handle->gif_info.attr_change_flag;
}

/*
  Function:       divoom_light_app_update_image_handle
  Description:    更新APP发的动画属性
  Input:          
  Return:         修改东西
*/

int divoom_light_app_update_image_handle(void *handle)
{
    Pdivoom_http_download_image_file_info pimage_handle = (Pdivoom_http_download_image_file_info)handle;

    if(pdivoom_light_app_handle->gif_info.attr_change_flag == 1)
    {
        pimage_handle->speed = pdivoom_light_app_handle->gif_info.attr_info.gif_speed;
    }
    else if(pdivoom_light_app_handle->gif_info.attr_change_flag == 2)
    {
        pimage_handle->speed = pdivoom_light_app_handle->gif_info.attr_info.gif_speed;
        if(pdivoom_light_app_handle->gif_info.attr_info.unicode_len)
        {
            //有文字
            if(pimage_handle->pfont_data == NULL)
            {
                pimage_handle->pfont_data = divoom_system_malloc(sizeof(divoom_http_download_image_font_data));
            }
            pimage_handle->pfont_data->size = pdivoom_light_app_handle->gif_info.attr_info.text_size;                     //字体大小
            pimage_handle->pfont_data->font = pdivoom_light_app_handle->gif_info.attr_info.font_id;                     //字体类型
            pimage_handle->pfont_data->speed = pdivoom_light_app_handle->gif_info.attr_info.gif_speed;                    //文字速度  
            pimage_handle->pfont_data->textEffect = pdivoom_light_app_handle->gif_info.attr_info.text_effect;               //文字效果
            pimage_handle->pfont_data->picEffect  = pdivoom_light_app_handle->gif_info.attr_info.pic_effect;                //图片效果
            pimage_handle->pfont_data->start_x = pdivoom_light_app_handle->gif_info.attr_info.text_start_x;                  //框的起始x
            pimage_handle->pfont_data->start_y = pdivoom_light_app_handle->gif_info.attr_info.text_start_y;                  //框的起始y
            pimage_handle->pfont_data->frameWidth = pdivoom_light_app_handle->gif_info.attr_info.text_frame_width;               //框的高度
            pimage_handle->pfont_data->frameHeight = pdivoom_light_app_handle->gif_info.attr_info.text_frame_hight;              //框的高度
            memcpy(pimage_handle->pfont_data->color, pdivoom_light_app_handle->gif_info.attr_info.text_color, 3);               //字体颜色
            pimage_handle->pfont_data->unicode_len = pdivoom_light_app_handle->gif_info.attr_info.unicode_len;           //显示字体长度
            memcpy(pimage_handle->pfont_data->unicode_str, pdivoom_light_app_handle->gif_info.attr_info.unicode_str, pdivoom_light_app_handle->gif_info.attr_info.unicode_len *2);      //显示的字体unicode
            pimage_handle->font_flag = 1;
        }
        else
        {
            if(pimage_handle->font_flag)
            {
                pimage_handle->font_flag = 0;
                divoom_system_free(pimage_handle->pfont_data);
                pimage_handle->pfont_data = NULL;
            }
        }
    }
    return pdivoom_light_app_handle->gif_info.attr_change_flag;
}

/*
  Function:       divoom_light_app_update_gif_use_flag
  Description:    获取APP发的动画属性是修改后是不是被应用过
  Input:          flag 小于0是设置，否则是获取
  Return:         1 or 0
*/

int divoom_light_app_update_gif_use_flag(int flag)
{
    if(flag >= 0)
    {
        pdivoom_light_app_handle->gif_info.use_flag = flag;
    }
    return pdivoom_light_app_handle->gif_info.use_flag && pdivoom_light_app_handle->gif_info.attr_change_flag;
}

/*
  Function:       divoom_light_app_play_next_clock
  Description:    在表盘页面切换表盘显示
  Input:          dir   1：下一个表盘；0：前一个表盘
  Return:         NONE
*/

void divoom_light_app_play_next_clock(int dir)
{
    divoom_sys_msg_info model_msg = {0};
    int *clock_id;

    clock_id = divoom_queue_pop(pdivoom_light_app_handle->next_clock_queue);
    if(clock_id == NULL)
    {
        clock_id  = divoom_system_malloc(sizeof(int));
        *clock_id = pdivoom_light_app_handle->base_light.clock_id;
    }
    else
    {
        divoom_print("pass the event!\n");
    }
    if(dir)
    {
        *clock_id = divoom_light_app_get_next_clock_from_id(*clock_id);
    }
    else
    {
        *clock_id = divoom_light_app_get_pre_clock_from_id(*clock_id);
    }
    if(*clock_id <= 0)
    {
        *clock_id = DIVOOM_DEFAULT_CLOCK_ID;
    }

    divoom_queue_push(pdivoom_light_app_handle->next_clock_queue, clock_id);
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_MENU_SET_CLOCK_INDEX;
    model_msg.msg_type = 1;
	divoom_sys_model_crl_msg_push(&model_msg);
}

typedef struct _divoom_light_app_download_font_item
{
    int clock_id;
    uint16 font_num;
    struct { uint32 model_id; uint32 image_id; } image_list[50];
}divoom_light_app_download_font_item, *Pdivoom_light_app_download_font_item;

int divoom_light_app_check_download_font_cmp_func(void *item1, void *item2)
{
    int clock_id = *(int *)item1;
    Pdivoom_light_app_download_font_item pitem = (Pdivoom_light_app_download_font_item)item2;

    return clock_id == pitem->clock_id ? 0 : 1;
}

static void divoom_light_app_set_clock_download_progress_state(uint8 active,
    uint8 total_known)
{
    if(pdivoom_light_app_handle == NULL ||
        pdivoom_light_app_handle->download_font_lock == NULL)
    {
        return;
    }

    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    if(!active || !pdivoom_light_app_handle->clock_download_progress_active)
        pdivoom_light_app_handle->clock_download_failed = 0;
    pdivoom_light_app_handle->clock_download_progress_active = active;
    if(!active)
        pdivoom_light_app_handle->clock_download_config_pending = 0;
    pdivoom_light_app_handle->clock_download_total_known = total_known;
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    divoom_print("clock download state: active=%d, total_known=%d\n", active, total_known);
}

uint32 divoom_light_app_get_clock_download_session(void)
{
    uint32 session = 0;
    if(pdivoom_light_app_handle && pdivoom_light_app_handle->download_font_lock)
    {
        divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
        if(pdivoom_light_app_handle->clock_download_progress_active)
            session = pdivoom_light_app_handle->clock_download_session;
        divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    }
    return session;
}

static uint32 divoom_light_app_reserve_clock_intent(void)
{
    uint32 intent;
    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    intent = ++pdivoom_light_app_handle->clock_select_intent;
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    return intent;
}

static int divoom_light_app_clock_intent_is_current(uint32 intent)
{
    int current;
    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    current = intent == pdivoom_light_app_handle->clock_select_intent;
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    return current;
}

int divoom_light_app_clock_download_session_is_current(uint32 session)
{
    return session != 0 && session == divoom_light_app_get_clock_download_session();
}

void divoom_light_app_post_clock_download_event(uint8 event_id, uint32 session)
{
    divoom_sys_msg_info message = {0};
    if(session == 0)
        return;
    message.src_id = DIVOOM_MODEL_LIGHT_APP;
    message.dest_id = DIVOOM_MODEL_LIGHT_APP;
    message.msg_type = 1;
    message.msg_id = event_id;
    message.result = (int)session;
    divoom_sys_model_crl_msg_push(&message);
}

/* 调用者持有业务锁；只在资源锁内换会话，不等待网络和 LVGL。 */
static uint32 divoom_light_app_begin_clock_download(int clock_id)
{
    Pdivoom_light_app_download_font_item item;
    uint32 session;
    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    while((item = divoom_queue_pop(pdivoom_light_app_handle->download_font_queue)) != NULL)
        divoom_system_free(item);
    session = (pdivoom_light_app_handle->clock_download_session + 1) & 0x7fffffffU;
    if(session == 0)
        session = 1;
    pdivoom_light_app_handle->clock_download_session = session;
    pdivoom_light_app_handle->download_font_total = 0;
    pdivoom_light_app_handle->download_font_number = 0;
    pdivoom_light_app_handle->clock_download_failed = 0;
    pdivoom_light_app_handle->clock_download_progress_active = 1;
    pdivoom_light_app_handle->clock_download_total_known = 0;
    pdivoom_light_app_handle->clock_download_config_pending = 0;
    pdivoom_light_app_handle->clock_download_config_completed = 0;
    pdivoom_light_app_handle->clock_download_config_dirty = 0;
    pdivoom_light_app_handle->pending_clock_download_id = clock_id;
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    return session;
}

static int divoom_light_app_check_clock_download_files(int clock_id)
{
    return divoom_clock_manage_check_clock_all_file_with_owner(clock_id,
        (int)divoom_light_app_get_clock_download_session(), divoom_light_app_check_download_font);
}

/* 0 表示不在配置阶段；非零值随完整配置成功更新而递增，不是下载文件数。 */
uint32 divoom_light_app_get_clock_config_progress(uint32 session)
{
    uint32 progress = 0;
    if(pdivoom_light_app_handle == NULL || pdivoom_light_app_handle->download_font_lock == NULL)
        return 0;
    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    if(session != 0 && session == pdivoom_light_app_handle->clock_download_session &&
        pdivoom_light_app_handle->clock_download_progress_active &&
        pdivoom_light_app_handle->clock_download_config_pending)
        progress = (uint32)pdivoom_light_app_handle->clock_download_config_completed + 1;
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    return progress;
}

int divoom_light_app_get_clock_download_progress(uint32 *finished, uint32 *total,
    uint8 *total_known)
{
    int active;

    if(finished)
    {
        *finished = 0;
    }
    if(total)
    {
        *total = 0;
    }
    if(total_known)
    {
        *total_known = 0;
    }
    if(pdivoom_light_app_handle == NULL ||
        pdivoom_light_app_handle->download_font_lock == NULL)
    {
        return 0;
    }

    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    active = pdivoom_light_app_handle->clock_download_progress_active;
    if(finished)
    {
        *finished = pdivoom_light_app_handle->download_font_number;
    }
    if(total)
    {
        *total = pdivoom_light_app_handle->download_font_total;
    }
    if(total_known)
    {
        *total_known = pdivoom_light_app_handle->clock_download_total_known;
    }
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    return active;
}

void divoom_light_app_check_download_font(int32 clock_id, uint32 model_id, uint32 image_id,
    int32 result)
{
    int i;
    int free_slot = -1;
    int matched_slot = -1;
    uint8 resource_finished = 0;
    uint8 resource_failed = 0;
    uint8 queue_empty = 0;
    uint16 total_num;
    uint16 finished_num;
    Pdivoom_light_app_download_font_item pfont_item = NULL;

    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    if(!pdivoom_light_app_handle->clock_download_progress_active ||
        (uint32)clock_id != pdivoom_light_app_handle->clock_download_session)
    {
        divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
        return;
    }
    pfont_item = divoom_queue_get_cmp(pdivoom_light_app_handle->download_font_queue,
        &clock_id, divoom_light_app_check_download_font_cmp_func);
    if(result == DIVOOM_CLOCK_MANAGE_DOWNLOAD_PENDING)
    {
        if(pfont_item == NULL)
        {
            /* 主、子表盘合计最多50项；累计总数只在换会话时清零。 */
            pfont_item = divoom_system_malloc(sizeof(divoom_light_app_download_font_item));
            if(pfont_item)
            {
                memset(pfont_item, 0, sizeof(*pfont_item));
                pfont_item->clock_id = clock_id;
                if(!divoom_queue_push(pdivoom_light_app_handle->download_font_queue, pfont_item))
                {
                    divoom_system_free(pfont_item);
                    pfont_item = NULL;
                }
            }
            if(pfont_item == NULL)
            {
                resource_failed = 1;
                pdivoom_light_app_handle->clock_download_failed = 1;
            }
        }
    }
    if(pfont_item)
    {
        for(i = 0; i < (int)(sizeof(pfont_item->image_list) / sizeof(pfont_item->image_list[0])); i++)
        {
            if(pfont_item->image_list[i].model_id == model_id &&
                pfont_item->image_list[i].image_id == image_id)
                matched_slot = i;
            if(pfont_item->image_list[i].model_id == 0 && free_slot < 0)
                free_slot = i;
        }
        if(result == DIVOOM_CLOCK_MANAGE_DOWNLOAD_PENDING && matched_slot < 0)
        {
            if(free_slot >= 0)
            {
                pfont_item->image_list[free_slot].model_id = model_id;
                pfont_item->image_list[free_slot].image_id = image_id;
                pfont_item->font_num++;
                pdivoom_light_app_handle->download_font_total++;
                divoom_print("clock download add: %d, %d, num: %d\n", model_id, image_id,
                    pdivoom_light_app_handle->download_font_total);
            }
            else
            {
                resource_failed = 1;
                pdivoom_light_app_handle->clock_download_failed = 1;
            }
        }
        else if(result != DIVOOM_CLOCK_MANAGE_DOWNLOAD_PENDING && matched_slot >= 0)
        {
            pfont_item->image_list[matched_slot].model_id = 0;
            pfont_item->image_list[matched_slot].image_id = 0;
            pfont_item->font_num--;
            pdivoom_light_app_handle->download_font_number++;
            resource_finished = 1;
            if(result == 0)
            {
                resource_failed = 1;
                pdivoom_light_app_handle->clock_download_failed = 1;
            }
            divoom_print("clock download %s: %d, %d, progress: %d/%d\n",
                result ? "success" : "failed", model_id, image_id,
                pdivoom_light_app_handle->download_font_number,
                pdivoom_light_app_handle->download_font_total);
            if(pfont_item->font_num == 0)
            {
                pfont_item = divoom_queue_pop_cmp(pdivoom_light_app_handle->download_font_queue,
                    &clock_id, divoom_light_app_check_download_font_cmp_func);
                divoom_system_free(pfont_item);
            }
        }
    }
    queue_empty = (resource_finished || resource_failed) &&
        divoom_queue_items(pdivoom_light_app_handle->download_font_queue) == 0;
    total_num = pdivoom_light_app_handle->download_font_total;
    finished_num = pdivoom_light_app_handle->download_font_number;
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);

    if(result == DIVOOM_CLOCK_MANAGE_DOWNLOAD_PENDING || resource_finished)
    {
        divoom_download_aliyun_file_set_app_last_info(total_num, finished_num);
    }
    if(resource_failed)
    {
        /* 任一素材失败立即退出，不再等待其余在途下载；旧会话回调会被忽略。 */
        divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD,
            (uint32)clock_id);
    }
    else if(queue_empty)
    {
        /* 下载线程只报告终态，页面切换交给 light app 事件线程串行处理。 */
        divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_CLOCK_RESOURCE_DOWNLOAD_DONE,
            (uint32)clock_id);
    }
}

static void divoom_light_app_clear_download_font_queue(void)
{
    uint16 total_num;
    Pdivoom_light_app_download_font_item pfont_item;

    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    while((pfont_item = divoom_queue_pop(pdivoom_light_app_handle->download_font_queue)) != NULL)
    {
        divoom_system_free(pfont_item);
    }
    pdivoom_light_app_handle->download_font_number =
        pdivoom_light_app_handle->download_font_total;
    total_num = pdivoom_light_app_handle->download_font_total;
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);

    /* 配置尚未到达时总数可能为 0，也要把等待页明确置为完成态。 */
    if(total_num == 0)
    {
        divoom_download_aliyun_file_set_app_last_info(1, 1);
    }
    else
    {
        divoom_download_aliyun_file_set_app_last_info(total_num, total_num);
    }
}

void divoom_light_app_set_disp_download_font(int clock_id)
{
    uint8 queue_empty;
    uint16 total_num;
    uint16 finished_num;

    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    queue_empty = divoom_queue_items(pdivoom_light_app_handle->download_font_queue) == 0;
    total_num = pdivoom_light_app_handle->download_font_total;
    finished_num = pdivoom_light_app_handle->download_font_number;
    if(pdivoom_light_app_handle->pending_clock_download_id == clock_id || !queue_empty)
    {
        /* 普通切表盘也显式标记等待来源，避免页面混用相册的下载标记。 */
        pdivoom_light_app_handle->clock_download_progress_active = 1;
        pdivoom_light_app_handle->clock_download_total_known = 1;
    }
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    if(queue_empty)
    {
        if(!pdivoom_light_app_handle->clock_download_failed &&
            pdivoom_light_app_handle->pending_clock_download_id == clock_id &&
            !divoom_clock_manage_check_clock_all_file(clock_id, NULL))
        {
            /* 资源队列空时可能还在等子表盘配置，保留 X/X 等待回包。 */
            divoom_light_app_set_clock_download_progress_state(1, 0);
            return;
        }
        /* URL/元数据缺失可能同步失败，不能再排一个随后卡住的下载页事件。 */
        divoom_download_aliyun_file_set_app_last_info(total_num, total_num);
        divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_CLOCK_RESOURCE_DOWNLOAD_DONE,
            divoom_light_app_get_clock_download_session());
    }
    else
    {
        divoom_download_aliyun_file_set_app_last_info(total_num, finished_num);
        if(pdivoom_light_app_handle->pending_clock_download_id != clock_id)
        {
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
                DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
        }
    }
}

void divoom_light_app_check_font_task(void *para)
{
    uint8 queue_empty;
    uint16 total_num;
    uint16 finished_num;
    char file_name[128];

    Pdivoom_light_app_download_font_item pfont_item;

    divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
    if(divoom_queue_items(pdivoom_light_app_handle->download_font_queue) == 0)
    {
        divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
        return;
    }
    for(int i = 0; i < divoom_queue_items(pdivoom_light_app_handle->download_font_queue); i++)
    {
        pfont_item = divoom_queue_pop(pdivoom_light_app_handle->download_font_queue);
        if(pfont_item == NULL)
        {
            break;
        }
        for(int j = 0; pfont_item && j < sizeof(pfont_item->image_list)/sizeof(pfont_item->image_list[0]); j++)
        {
            if(pfont_item->image_list[j].image_id != 0)
            {
                divoom_manage_image_download_get_file_name(pfont_item->image_list[j].model_id, pfont_item->image_list[j].image_id, file_name);
                if(divoom_port_file_exists(file_name))
                {
                    pfont_item->image_list[j].model_id = 0;
                    pfont_item->image_list[j].image_id = 0;
                    pfont_item->font_num --;
                    pdivoom_light_app_handle->download_font_number ++;
                    divoom_print("file num: %d, %d\n", pdivoom_light_app_handle->download_font_total, pdivoom_light_app_handle->download_font_number);
                    divoom_download_aliyun_file_set_app_last_info(pdivoom_light_app_handle->download_font_total, pdivoom_light_app_handle->download_font_number);
                    if(pfont_item->font_num == 0)
                    {
                        divoom_system_free(pfont_item);
                        pfont_item = NULL;
                        break;
                    }
                }
            }
        }
        if(pfont_item)
        {
            divoom_queue_push(pdivoom_light_app_handle->download_font_queue, pfont_item);
        }
    }
    queue_empty = divoom_queue_items(pdivoom_light_app_handle->download_font_queue) == 0;
    total_num = pdivoom_light_app_handle->download_font_total;
    finished_num = pdivoom_light_app_handle->download_font_number;
    divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    if(queue_empty)
    {
        if(total_num == 0)
        {
            divoom_download_aliyun_file_set_app_last_info(1, 1);
        }
        else
        {
            divoom_download_aliyun_file_set_app_last_info(total_num, finished_num);
        }
        divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_CLOCK_RESOURCE_DOWNLOAD_DONE,
            divoom_light_app_get_clock_download_session());
    }
}

void divoom_light_app_restore_power_on_disp(void)
{   
    if(pdivoom_light_app_handle->base_light.power_on_disp_clock_flag)
    {
        pdivoom_light_app_handle->base_light.clock_id = pdivoom_light_app_handle->base_light.power_on_clock_id;
        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
    }
}

static void divoom_light_app_mixer_enter(void)
{
    if(pdivoom_light_app_handle->cur_mode != DIVOOM_LIGHT_APP_DISP_MIXER_MODE ||
        !pdivoom_light_app_handle->active_flag)
    {
        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_MIXER_MODE);
    }
}

static void divoom_light_app_mix_music2_enter(void)
{
    if(pdivoom_light_app_handle->cur_mode != DIVOOM_LIGHT_APP_DISP_MIX_MUSIC2_MODE ||
        !pdivoom_light_app_handle->active_flag)
    {
        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_MIX_MUSIC2_MODE);
    }
}

static uint8 divoom_light_app_mixer_effect_from_mode(uint8 mode)
{
    if(mode >= DIVOOM_LIGHT_APP_MIXER_EFFECT_COUNT)
    {
        /* 超出协议 0..8 范围的 Mode 钳位到最后一种双向穿越效果。 */
        return DIVOOM_LIGHT_APP_MIXER_EFFECT_COUNT - 1;
    }
    return mode;
}

static void divoom_light_app_mixer_stop(void)
{
    divoom_app_schedule_item_info event_info = {0};

    pdivoom_light_app_handle->mixer.running = 0;
    pdivoom_light_app_handle->mixer.generation ++;
    event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
    event_info.event_id = DIVOOM_LIGHT_APP_MIXER_EVENT;
    event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    divoom_app_schedule_cancel(&event_info);

    /* Mixer/Stop 是两套炫音页面的公共退出命令，必须同时终止 SetNew 页面。 */
    pdivoom_light_app_handle->mix_music2.icon = 0xff;
    pdivoom_light_app_handle->mix_music2.generation ++;
    event_info.event_id = DIVOOM_LIGHT_APP_MIX_MUSIC2_EVENT;
    divoom_app_schedule_cancel(&event_info);
}


void divoom_light_app_reload_all_clock_info(int clock_id)
{
    //
    divoom_clock_id_info clock_info = {0};
    
    clock_info.clock_id = clock_id;
    Pdivoom_clock_manage_item pclock_info = divoom_clock_manage_get_raw_clock_info(clock_id,  0, 0);

    if(pclock_info)
    {
        if(strlen(pclock_info->review_addr))
        {
            //没有预览图的，后面会请求
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_SYS_DATA, DIVOOM_SERVER_TOPIC,&clock_info);
        }
        if(pclock_info->app_num == 1 && pclock_info->papp_list && pclock_info->papp_list[0].Config)
        {
            int sub_clock_id = 0;
    		for (int i = 0; i < pclock_info->disp_item_num; i++)
    		{
                if (pclock_info->pdisp_item_list[i].type >= DIVOOM_CLOCK_DISP_SUPPORT_DAIL_COMPONENT_START_ID && 
    					pclock_info->pdisp_item_list[i].type < DIVOOM_CLOCK_DISP_SUPPORT_DAIL_COMPONENT_END_ID)
    			{
    				//读取配置
    					divoom_app_com_get_int_from_config(pclock_info->papp_list[0].Config, pclock_info->pdisp_item_list[i].item_id, &sub_clock_id);
    					if (sub_clock_id > 0)
    					{
                            clock_info.clock_id = sub_clock_id;
                            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_SYS_DATA, DIVOOM_SERVER_TOPIC,&clock_info);

    					}
    			}
    		}
		}
        
        divoom_clock_manage_free_clock(pclock_info);
    }
    else
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA, DIVOOM_SERVER_TOPIC,&clock_info);
    }
}



uint8 divoom_light_app_event(Pdivoom_sys_msg_info msg)
{
    divoom_print("event id : %d\n", msg->msg_id);
	pdivoom_light_app_handle->base_light.clock_mode = 0;
	switch(msg->msg_id)
	{  
        case DIVOOM_LIGHT_APP_EVENT_MIXER_START:
        {
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            pdivoom_light_app_handle->mixer.running = 0;
            pdivoom_light_app_handle->mixer.generation ++;
            divoom_light_app_mixer_enter();
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;
        }

        case DIVOOM_LIGHT_APP_EVENT_MIXER_STOP:
        {
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            divoom_light_app_mixer_stop();
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;
        }

        case DIVOOM_LIGHT_APP_EVENT_MIXER_SET:
        {
            uint8 *mixer_data = (uint8 *)msg->data;

            if(mixer_data != NULL)
            {
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                if(mixer_data[0] == 0 && mixer_data[2] == 0)
                {
                    /* 兼容蓝牙 0x6A：Mode=0/OnOff=0 表示退出。 */
                    divoom_light_app_mixer_stop();
                }
                else if(mixer_data[2] == 0)
                {
                    uint8 stop_effect = divoom_light_app_mixer_effect_from_mode(mixer_data[0]);

                    if(stop_effect == pdivoom_light_app_handle->mixer.effect)
                    {
                        /* 关闭包只允许停止同一 Mode，避免其它按键的松开事件中断当前动画。 */
                        pdivoom_light_app_handle->mixer.running = 0;
                        pdivoom_light_app_handle->mixer.generation ++;
                        divoom_light_app_mixer_enter();
                    }
                }
                else
                {
                    if(mixer_data[0] == 0)
                    {
                        /* Mode=0/OnOff=1 只进入页面，不触发动效。 */
                        pdivoom_light_app_handle->mixer.running = 0;
                    }
                    else
                    {
                        /* Mode 选择九种独立动效，ButtonSelect 选择六套色板。 */
                        pdivoom_light_app_handle->mixer.effect =
                            divoom_light_app_mixer_effect_from_mode(mixer_data[0]);
                        pdivoom_light_app_handle->mixer.color = mixer_data[1] % DIVOOM_LIGHT_APP_MIXER_COLOR_COUNT;
                        pdivoom_light_app_handle->mixer.running = 1;
                    }
                    pdivoom_light_app_handle->mixer.generation ++;
                    divoom_light_app_mixer_enter();
                }
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            }
            break;
        }

        case DIVOOM_LIGHT_APP_EVENT_MIXER_SET_NEW:
        {
            uint8 *mixer_data = (uint8 *)msg->data;

            if(mixer_data != NULL)
            {
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                if(pdivoom_light_app_handle->mix_music2.effect == mixer_data[0] &&
                    pdivoom_light_app_handle->mix_music2.icon == mixer_data[1])
                {
                    /* 同一按键重复触发也必须被页面识别；计数行为与 FlowToo 保持一致。 */
                    if(pdivoom_light_app_handle->mix_music2.activation_count < 0xff)
                    {
                        pdivoom_light_app_handle->mix_music2.activation_count ++;
                    }
                }
                else
                {
                    pdivoom_light_app_handle->mix_music2.activation_count = 0;
                }
                pdivoom_light_app_handle->mix_music2.effect = mixer_data[0];
                pdivoom_light_app_handle->mix_music2.icon = mixer_data[1];
                pdivoom_light_app_handle->mix_music2.generation ++;
                divoom_light_app_mix_music2_enter();
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            }
            break;
        }

        case DIVOOM_LIGHT_APP_EVENT_PLAY_LIST:
            if(pdivoom_light_app_handle->playlist_id != *(int *)msg->data)
            {
                pdivoom_light_app_handle->playlist_id = *(int *)msg->data;
                divoom_userimagelist_manage_set_temp_album_id(pdivoom_light_app_handle->playlist_id);
            }
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            pdivoom_light_app_handle->base_light.clock_id = DIVOOM_USE_IMAGE_LIST_CLOCK_ID;
            pdivoom_light_app_handle->base_light.mode = DIVOOM_DISP_CLOCK_MODE;
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_VISION_PLAY_TIME:
            pdivoom_light_app_handle->base_light.vision_play_time = *(int *)msg->data;
            divoom_light_app_load_save();
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_EXIT_DISP_CUSTOM_CLOCK:
            {
                divoom_app_schedule_item_info event_info = {0};
                event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
        		event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
                event_info.event_id = DIVOOM_LIGHT_APP_CUSTOM_DIAL_DISP;
                divoom_app_schedule_cancel(&event_info);
            }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_DISP_CUSTOM_CLOCK:
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_ENTER_CUSTOM_DIAL);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_ACTIVE_CALENDAR_TIMEOUT:
        {
            divoom_app_schedule_item_info event_info = {0};
            event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
    		event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            event_info.event_id = DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_EVENT;
            divoom_app_schedule_cancel(&event_info);
        }
            break;
        case DIVOOM_LIGHT_APP_EVENT_ENTER_OUTLOOK_CALENDAR:
                divoom_outlook_calendar_update_menu_flag(1);
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_ENTER_OUTLOOK_CALENDAR);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_ENTER_GOOGLE_CALENDAR:
                divoom_google_calendar_update_menu_flag(1);
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_ENTER_GOOGLE_CALENDAR);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;

        case DIVOOM_LIGHT_APP_EVENT_OUTLOOK_CALENDAR_CTRL:
        case DIVOOM_LIGHT_APP_EVENT_GOOGLE_CALENDAR_CTRL:
            if(msg->data == NULL)
            {
                divoom_app_schedule_item_info channel_schedule = {0};
    
                channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT; 
                channel_schedule.model_id= DIVOOM_MODEL_LIGHT_APP;  
                channel_schedule.event_id = DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_EVENT;  
                channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
                divoom_app_schedule_cancel(&channel_schedule);
            }
            else
            {
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                divoom_light_app_request_resource(msg->msg_id==DIVOOM_LIGHT_APP_EVENT_GOOGLE_CALENDAR_CTRL?DIVOOM_LIGHT_APP_DISP_GOOGLE_CALENDAR_MODE:DIVOOM_LIGHT_APP_DISP_OUTLOOK_CALENDAR_MODE);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_REMOVE_POHTO_INFO:
        {
            Pdivoom_pic_album_manage_delete_info_item pitem_info = (Pdivoom_pic_album_manage_delete_info_item)msg->data;
            if(pitem_info->type)
            {
                divoom_pic_album_manage_del_list_from_id(pitem_info->album_id, pitem_info->pic_list, pitem_info->pic_num);
                divoom_pic_phone_manage_remove_pic_list(pitem_info->pic_list, pitem_info->pic_num);
            }
            else
            {
                divoom_pic_album_manage_remove_list_from_id(pitem_info->album_id, pitem_info->pic_list, pitem_info->pic_num);
            }
        }
            break;

        case DIVOOM_LIGHT_APP_EVENT_DISP_DEMO_DIAL:
        {
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            pdivoom_light_app_handle->app_request_flag = 1;
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_DEMO_DIAL_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
        }   
            break;

        case DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL:
        {
            Pdivoom_clock_manage_item pclock_info = NULL;
            int all_file_ready = 0;

            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            if(pdivoom_light_app_handle->clock_download_config_pending)
            {
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                break;
            }
            pdivoom_light_app_handle->app_request_flag = 1;
            if(pdivoom_light_app_handle->pending_clock_download_id != 0)
            {
                int clock_id = pdivoom_light_app_handle->pending_clock_download_id;

                pclock_info = divoom_clock_manage_get_raw_clock_info(clock_id, 0, 0);
                if(pclock_info)
                {
                    all_file_ready = divoom_light_app_check_clock_download_files(clock_id);
                    divoom_light_app_set_clock_download_progress_state(1, 1);
                }
                if(!pclock_info || !all_file_ready)
                {
                    if(pclock_info)
                    {
                        divoom_download_aliyun_file_set_app_last_info(pdivoom_light_app_handle->download_font_total,
                            pdivoom_light_app_handle->download_font_number);
                    }
                    else
                    {
                        divoom_download_aliyun_file_set_app_last_info(1, 0);
                    }
                    divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE);
                }
                else
                {
                    pdivoom_light_app_handle->pending_clock_download_id = 0;
                    divoom_light_app_set_clock_download_progress_state(0, 0);
                    divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
                }
            }
            else
            {
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
            }
            if(pclock_info)
            {
                divoom_clock_manage_free_clock(pclock_info);
            }
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
        }   
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_EXIT_PHOTO_VIEW:
        {
            divoom_app_schedule_item_info channel_schedule = {0};

            divoom_home_connect_cancel_event();
            channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT; 
            channel_schedule.model_id= DIVOOM_MODEL_LIGHT_APP;  
            channel_schedule.event_id = DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT;  
            channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&channel_schedule);
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL:
            /* Queue the ordinary clock below OTA/resource work so release can restore it. */
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
    	    pdivoom_light_app_handle->app_request_flag = 2;
            if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_ENTER_GOOGLE_CALENDAR)
            {
                pdivoom_light_app_handle->cur_mode = DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE;
            }
            if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
            {
			    divoom_light_app_request_resource(pdivoom_light_app_handle->cur_mode);
            }
            else
            {
                /* 调用方已用缓存参数显示表盘时，只同步模式状态，避免迟到的 clearall 覆盖后续页面。 */
                divoom_print("light app: startup dial already visible, skip duplicate resource request\n");
            }
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;

        case DIVOOM_LIGHT_APP_EVENT_LAN_CREATE_CLOCK_BEGIN:
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            divoom_print("LAN create clock: enter add-photo wait page\n");
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;
               
        case DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO:
		{
			int transfer_percent;
            uint32 transfer_finished;
            uint32 transfer_total;
#ifdef DIVOOM_SIMULATOR
            /* 白盒压测时 MQTT 会注入传图事件，弹出 menu 74 打断表盘切换路径 */
            if (getenv("DIVOOM_SIM_WHITEBOX") != NULL)
            {
                break;
            }
#endif
            #if 1
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
			if (divoom_http_server_get_upload_wait_progress(&transfer_percent) ||
			    divoom_http_server_get_pixel_upload_progress(&transfer_percent) ||
                divoom_manage_image_get_transfer_progress(
                    &transfer_finished, &transfer_total) ||
                divoom_subscribe_manage_transfer_request_is_pending())
			{
				divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_FILE_TRANSFER_MODE);
			}
			else
			{
				divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE);
			}
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            #endif
            break;
		}

        case DIVOOM_LIGHT_APP_EVENT_CLEAR_REMOVED_CLOCK:
            divoom_light_app_clear_clock_event(msg->result);
            break;

        case DIVOOM_LIGHT_APP_EVENT_ADD_CLOCK_RECEIVED:
        {
            int clock_id = *(int *)msg->data;
            uint32 intent;

            if(clock_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
            {
                divoom_light_app_add_clock_id(clock_id, 1);
                divoom_anniversary_clock_config_add_reminder(clock_id, 1);
                (void)divoom_clock_component_index_refresh(clock_id);
                break;
            }
            intent = msg->result > 0 ? (uint32)msg->result : divoom_light_app_reserve_clock_intent();

            divoom_print("clock add begin: %d, refresh full config\n", clock_id);
            divoom_light_app_select_added_clock(clock_id,
                pdivoom_light_app_handle->base_light.clock_mode, intent, 1);
        }
            break;

        case DIVOOM_LIGHT_APP_EVENT_WAIT_CLOCK_DOWNLOAD:
        {
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            if(divoom_light_app_clock_download_session_is_current((uint32)msg->result))
            {
                /* 此事件只切换页面，配置获取和资源统计由异步回包处理。 */
                divoom_print("clock download page: %d\n", pdivoom_light_app_handle->pending_clock_download_id);
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE);
            }
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
        }
            break;

        case DIVOOM_LIGHT_APP_EVENT_CLOCK_RESOURCE_DOWNLOAD_DONE:
        {
            uint8 queue_empty;

            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            if(!divoom_light_app_clock_download_session_is_current((uint32)msg->result) ||
                pdivoom_light_app_handle->clock_download_config_pending)
            {
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                break;
            }

            divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
            queue_empty = divoom_queue_items(
                pdivoom_light_app_handle->download_font_queue) == 0;
            divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
            if(queue_empty && (pdivoom_light_app_handle->pending_clock_download_id != 0 ||
                pdivoom_light_app_handle->clock_download_progress_active))
            {
                int clock_id = pdivoom_light_app_handle->pending_clock_download_id;
                if(pdivoom_light_app_handle->clock_download_failed)
                {
                    /* 失败是终态，不由完成事件反复重建失败下载。 */
                    divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD,
                        (uint32)msg->result);
                    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                    break;
                }
                if(clock_id && !divoom_light_app_check_clock_download_files(clock_id))
                {
                    /* 完成事件可能早于子配置回包，重新收束整张表盘的资源。 */
                    divoom_light_app_set_disp_download_font(clock_id);
                    /* SYS may have superseded the session still bound to the wait page. */
                    divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE);
                    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                    break;
                }
                pdivoom_light_app_handle->pending_clock_download_id = 0;
                divoom_light_app_set_clock_download_progress_state(0, 0);
                pdivoom_light_app_handle->app_request_flag = 1;
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
            }
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
        }
            break;

        case DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD:
        {
            divoom_app_schedule_item_info channel_schedule = {0};
            int restore_clock_id;

            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            if(!divoom_light_app_clock_download_session_is_current((uint32)msg->result) &&
                !(msg->result == 0 && !divoom_light_app_get_clock_download_session() &&
                    pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_FILE_TRANSFER_MODE))
            {
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                break;
            }
            /* cancel 会同步恢复旧调度并调用 start_disp，必须先发布回退 ID 和终态。
             * 否则回调会把失败目标写进 clock_info，随后再读它已无法找回旧表盘。 */
            restore_clock_id = pdivoom_light_app_handle->base_light.clock_info.clock_id;
            /* 获取失败或断网都回退，不能播放尚未获取完整的新表盘。 */
            if(pdivoom_light_app_handle->pending_clock_download_id != 0)
            {
                /* 同 ID 更新时原显示配置也可能已失效，退出不再触发补下载。 */
                if(restore_clock_id <= 0 ||
                    restore_clock_id == pdivoom_light_app_handle->pending_clock_download_id)
                    restore_clock_id = DIVOOM_DEFAULT_CLOCK_ID;
                pdivoom_light_app_handle->base_light.clock_id = restore_clock_id;
                divoom_light_app_load_save();
            }
            pdivoom_light_app_handle->pending_clock_download_id = 0;
            divoom_light_app_set_clock_download_progress_state(0, 0);
            pdivoom_light_app_handle->file_transfer_exit_requested = 1;
            /* 在途下载完成后只会被静默回收。 */
            divoom_light_app_clear_download_font_queue();
            divoom_home_connect_cancel_event();
            channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
            channel_schedule.model_id = DIVOOM_MODEL_LIGHT_APP;
            channel_schedule.event_id = DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT;
            channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&channel_schedule);

            /* 同步恢复回调可能已消费一次退出标记，本次显式回退仍须放行。 */
            pdivoom_light_app_handle->file_transfer_exit_requested = 1;
            pdivoom_light_app_handle->app_request_flag = 1;
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
        }
            break;
            
    	case DIVOOM_LIGHT_APP_EVENT_APP_HTTP_LOCAL_IMAG:
    	{
            Pdivoom_draw_remote_json_image_info remote_info = (Pdivoom_draw_remote_json_image_info)msg->data;
            uint32 server_gif_file_id;
            //将文件重命令，开始播放
            pdivoom_light_app_handle->draw_info.gif_info.attr_change_flag = 0;
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            divoom_light_app_app_gif_file_lock_ctrl(1);
            server_gif_file_id = divoom_light_app_server_gif_prepare_file(
                DIVOOM_SD_APP_PIC_DIR, remote_info->file_id);
            divoom_light_app_remove_file(DIVOOM_MANAGE_IMAGE_SERVER_GIF,
                DIVOOM_LIGHT_APP_SERVER_GIF_VOICE_FILE_ID,
                DIVOOM_SD_APP_PIC_DIR, remote_info->voice_id);
            //divoom_light_app_remove_file(DIVOOM_MANAGE_IMAGE_USER_HEAD_IMAGE_FILE, remote_info->author_id, DIVOOM_SD_APP_PIC_DIR, remote_info->author_file_name);
            divoom_light_app_app_gif_file_lock_ctrl(0);
            if(server_gif_file_id == 0)
            {
                divoom_print("server gif update aborted: downloaded file was not staged\n");
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                break;
            }
            divoom_user_head_manage_update_info(remote_info->author_id, remote_info->author_name, remote_info->author_file_id);
            pdivoom_light_app_handle->remote_image_info.author_id = remote_info->author_id;
            strcpy(pdivoom_light_app_handle->remote_image_info.image_name, remote_info->image_name);
            strcpy(pdivoom_light_app_handle->remote_image_info.author_name, remote_info->author_name);
            strcpy(pdivoom_light_app_handle->remote_image_info.voice_id, remote_info->voice_id);
            divoom_gif_ctrl_set_image_id(remote_info->flag);
			pdivoom_light_app_handle->gif_info.attr_change_flag = 0;
			divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
    	}
            break;
            
    	case DIVOOM_LIGHT_APP_EVENT_APP_HTTP_LOCAL_EQ:
    	{
            Pdivoom_draw_remote_json_image_info remote_info = (Pdivoom_draw_remote_json_image_info)msg->data;
            uint32 server_gif_file_id;
            //将文件重命令，开始播放
            divoom_light_app_app_gif_file_lock_ctrl(1);
            server_gif_file_id = divoom_light_app_server_gif_prepare_file(
                DIVOOM_SD_APP_PIC_DIR, remote_info->file_id);
            divoom_light_app_remove_file(DIVOOM_MANAGE_IMAGE_USER_HEAD_IMAGE_FILE, remote_info->author_id, DIVOOM_SD_APP_PIC_DIR, remote_info->author_file_name);
            divoom_light_app_app_gif_file_lock_ctrl(0);
            if(server_gif_file_id == 0)
            {
                divoom_print("server eq gif update aborted: downloaded file was not staged\n");
                break;
            }
            divoom_user_head_manage_update_info(remote_info->author_id, remote_info->author_name, remote_info->author_file_id);
            pdivoom_light_app_handle->remote_image_info.author_id = remote_info->author_id;
            strcpy(pdivoom_light_app_handle->remote_image_info.image_name, remote_info->image_name);
            strcpy(pdivoom_light_app_handle->remote_image_info.author_name, remote_info->author_name);
            divoom_gif_ctrl_set_image_id(remote_info->flag);
            divoom_light_app_real_eq_gif_notice(1);
    	}
            break;
            
    	case DIVOOM_LIGHT_APP_EVENT_ENTER_ALBUM_PLAY: //app进入相簿，设备开始相簿播放
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
    	    pdivoom_light_app_handle->app_request_flag = 1;
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_PLAY_ALBUM_MENU_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
    	    break;
            
        case DIVOOM_LIGHT_APP_EVENT_TEMP_CLOCK_PARA:
    	    pdivoom_light_app_handle->app_request_flag = 1;
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
	        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE);
            if(divoom_clock_manage_update_temp_clock_config((PDivoomSystemConfigJson)msg->data))
            {
                pdivoom_light_app_handle->base_light.clock_id = DIVOOM_TEMP_CLOCK_ID;
                pdivoom_light_app_handle->base_light.mode = DIVOOM_DISP_CLOCK_MODE;
            }
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_EXIT_NOTICE_CLOCK:
        {
            divoom_app_schedule_item_info channel_schedule = {0};

            divoom_home_connect_cancel_event();
            channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT; 
            channel_schedule.model_id= DIVOOM_MODEL_LIGHT_APP;  
            channel_schedule.event_id = DIVOOM_LIGHT_APP_CLOCK_NOTICE_EVENT;  
            channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&channel_schedule);
        }
            break;
        
        case DIVOOM_LIGHT_APP_EVENT_DIVOOM_SELF_RESET:
        {
            uint8 model_id, event_id;
            model_id = divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);

            if(model_id == DIVOOM_MODEL_LIGHT_APP && (event_id == DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT ||
                event_id == DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT || event_id == DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT))
            {
                divoom_system_delay_ms(1000);
            }
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_RESET_SAVE_CLOCK:
            break;
           
        case DIVOOM_LIGHT_APP_EVENT_START_SCHEDULE_CLOCK:
            if(msg->data == NULL)
            {
                divoom_app_schedule_item_info event_info = {0};
                event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
                event_info.event_id = DIVOOM_LIGHT_APP_SCHEDULE_CLOCK_EVENT;
        		event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
                divoom_app_schedule_cancel(&event_info);
            }
            else
            {
                Pdivoom_clock_schedule_play_info clock_info = (Pdivoom_clock_schedule_play_info)msg->data;

                pdivoom_light_app_handle->schedule_clock_info.clock_id = clock_info->clock_id;
                pdivoom_light_app_handle->schedule_clock_info.voice_flag = clock_info->voice_flag;
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_MODE);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_PLAY_NET_GIF_FILE:
        {
            char *file_name;

            file_name = divoom_queue_pop(pdivoom_light_app_handle->gif_file_queue);
            if(file_name)
            {
                if(divoom_queue_pop(pdivoom_light_app_handle->gif_file_queue) == 0)
                {
                    pdivoom_light_app_handle->tf_gif.flag = 0;
                    strcpy(pdivoom_light_app_handle->tf_gif.file_name, file_name);
                    divoom_system_free(file_name);
                    divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                    divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_TF_GIF_MODE);
                    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                }
            }
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_GIF_EDGE_CLOCK:
        {
            if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE ||
                pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE ||
                pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE)
            {
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                divoom_light_app_request_resource(pdivoom_light_app_handle->cur_mode);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            }
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_DOWNLOAD_NET_GIF_FILE:
        {
            char *file_name;
            divoom_web_get_request_info *url_info;

            url_info = divoom_system_malloc(sizeof(divoom_web_get_request_info));
            if(url_info)
            {
                strcpy(url_info->url, (char *)msg->data);
                divoom_play_local_gif_wait_notice();
                pdivoom_light_app_handle->tf_gif.net_num ++;
                sprintf(url_info->file_id, DIVOOM_LEGACY_SD_ROOT "/divoom_net/divoomtemp%d.gif", pdivoom_light_app_handle->tf_gif.net_num%20);
                url_info->mode = DIVOOM_WEB_GET_TYPE_DOWN_TO_FILE;
                url_info->pcallback = divoom_light_app_download_gif_file_finish;
                divoom_web_get_push(url_info);
                file_name = divoom_app_com_copy_string(url_info->file_id, 0);
                if(file_name)
                {
                    divoom_queue_push(pdivoom_light_app_handle->gif_file_queue, file_name);
                }
            }
            divoom_system_free(url_info);
            break;
        }
    	case DIVOOM_LIGHT_APP_EVENT_PLAY_LOCAL_GIF_FILE:
            pdivoom_light_app_handle->tf_gif.flag = 0;
            strcpy(pdivoom_light_app_handle->tf_gif.file_name,DIVOOM_LEGACY_SD_ROOT "/");
            strcat(pdivoom_light_app_handle->tf_gif.file_name, (char *)msg->data);
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_TF_GIF_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;
            
    	case DIVOOM_LIGHT_APP_EVENT_PLAY_LOCAL_GIF_FOLDER:
            pdivoom_light_app_handle->tf_gif.flag = 1;
            strcpy(pdivoom_light_app_handle->tf_gif.file_name,DIVOOM_LEGACY_SD_ROOT "/");
            strcat(pdivoom_light_app_handle->tf_gif.file_name, (char *)msg->data);
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_TF_GIF_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;

        case DIVOOM_LIGHT_APP_EVENT_RESET_HTTP_GIF_ID:
            divoom_gif_ctrl_reset_http_gif_id();
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_CLEAR_HTTP_IMAGE_TEXT:
            divoom_disp_custom_dial_clear(NULL);
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_TEXT:
            divoom_disp_custom_dial_add((void *)msg->data);
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_TEXT_LIST:
            divoom_disp_custom_dial_add_item_list((void *)msg->data);
            break;
        
        case DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_GIF:
		{
            Pdivoom_drawing_json_http_gif_info image_info = (Pdivoom_drawing_json_http_gif_info)msg->data;
            
            if(image_info->pic_num <= DIVOOM_LIGHT_APP_HTTP_IMAGE_NUM && (image_info->pic_width == DIVOOM_IMAGE_WIDTH ||
                image_info->pic_width == DIVOOM_IMAGE_SMALL_WIDTH ||image_info->pic_width == DIVOOM_IMAGE_WIDTH_64))
            {
    			if (image_info->pic_off == 0)
    			{
                    if(image_info->pic_num  > 1)
                    {
                        divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
    			        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE);
                        pdivoom_light_app_handle->wait_gif_time = time(0) + DIVOOM_LIGHT_APP_GIF_ACCPET_TIME_OUT;
                        divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                    }
    				divoom_gif_ctrl_set_callback(divoom_light_app_http_gif_notice);
    			}
                else
                {
                    pdivoom_light_app_handle->wait_gif_time = time(0) + DIVOOM_LIGHT_APP_GIF_ACCPET_TIME_OUT;
                }
    			divoom_gif_ctrl_update_http_image(image_info);
            }
		}
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_EXIT_DISPLAY_IFFFT_TEXT:
        {            
            divoom_app_schedule_item_info event_info = {0};
            event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
            event_info.event_id = DIVOOM_LIGHT_APP_IFFT_TEXT_EVENT;
    		event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&event_info);
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_DISPLAY_IFFFT_TEXT:
            if(msg->data)
            {
                if(pdivoom_light_app_handle->ifft_text_buf)
                {
                    divoom_system_free(pdivoom_light_app_handle->ifft_text_buf);
                    pdivoom_light_app_handle->ifft_text_buf = NULL;
                }
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                pdivoom_light_app_handle->ifft_text_buf = divoom_app_com_copy_string((char *)msg->data, 0);                
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_IFFT_TEXT_MODE);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            }
            break;

		case DIVOOM_LIGHT_APP_EVENT_DISPLAY_IFFFT_AMAZON_MUSIC:
			if(msg->data)
            {
                Pdivoom_clock_manage_item pclock_info;
                Pdivoom_clock_control_disp_data pdisp_data;
                divoom_clock_id_info clock_para = {0};

                clock_para.clock_id = DIVOOM_AMAZON_MUSIC_CLOCK_ID;
                pclock_info = divoom_clock_manage_get_clock_info(DIVOOM_AMAZON_MUSIC_CLOCK_ID, 0, 0);

                if(pclock_info)
                {
                    if(pdivoom_light_app_handle->active_flag == 1 && pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE
                        && pdivoom_light_app_handle->base_light.mode == DIVOOM_DISP_CLOCK_MODE && pdivoom_light_app_handle->base_light.clock_id == pclock_info->clock_id)
                    {
                        //如果当前是ＡＬＡＸＡ音乐，直接更新数据
                        pdisp_data = divoom_system_malloc(sizeof(divoom_clock_control_disp_data));
                        if(pdisp_data)
                        {
                            divoom_clock_control_disp_data_init(pdisp_data);
                            pdisp_data->num = 1;
                            pdisp_data->update = 1;
                            DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = 1;
                            strncpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[0], (char *)msg->data, DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN);
                            divoom_print("we will display alaxa music: %s\n", DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[0]);
                            divoom_disp_clock_get_update_disp_data(NULL, &clock_para, pdisp_data);
                            divoom_clock_control_disp_data_deinit(pdisp_data);
                            divoom_system_free(pdisp_data);
                        }
                    }
                    else
                    {
                        if(pclock_info->papp_list && pclock_info->papp_list[0].Config)
                        {
                            int notice_flag = 0;
                            
                            divoom_app_com_get_int_from_config(pclock_info->papp_list[0].Config, "NoticeSwitch", &notice_flag);
                            if(notice_flag)
                            {
                                pdisp_data = divoom_system_malloc(sizeof(divoom_clock_control_disp_data));
                                if(pdisp_data)
                                {
                                    divoom_clock_control_disp_data_init(pdisp_data);
                                    pdisp_data->num = 1;
                                    pdisp_data->update = 1;
                                    DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = 1;
                                    strncpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[0], (char *)msg->data, DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN);
                                    divoom_print("we will display alaxa music: %s\n", DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[0]);
                                    divoom_cache_clock_data_update(pclock_info->clock_id, 0, 0,pdisp_data);
                                    divoom_clock_control_disp_data_deinit(pdisp_data);
                                    divoom_system_free(pdisp_data);
                                    
                                    divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                                    pdivoom_light_app_handle->base_light.clock_id = pclock_info->clock_id;
                                    pdivoom_light_app_handle->base_light.mode = DIVOOM_DISP_CLOCK_MODE;
                                    divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
                                    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                                }                                
                            }
                        }
                    }
                    divoom_clock_manage_free_clock(pclock_info);
                }
            }
			break;

        case DIVOOM_LIGHT_APP_EVENT_SCREEN_APP_CTRL:
        case DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL:
        {
            int *flag = (int *)msg->data, on_off;

            if(pdivoom_light_app_handle->light_switch != *flag)
            {
                if(pdivoom_light_app_handle->light_switch)
                {
                    on_off = 0;
                    divoom_light_app_get_screen_status(0);
                    divoom_screen_bright(0);
					divoom_disp_ambient_ctrl_set_rgb_on_off(0);
					divoom_btcore_key_light_follow_screen(0);
                }
                else
                {
                    on_off = 1;
                    if(msg->src_id == DIVOOM_MODEL_NET)
                    {
                         divoom_main_cancel_on_off();
                    }

                    divoom_light_app_get_screen_status(1);
                    divoom_main_apply_sys_light();
					divoom_disp_ambient_ctrl_set_rgb_on_off(1);
					divoom_btcore_key_light_follow_screen(1);
                }

                if(DIVOOM_LIGHT_APP_EVENT_SCREEN_APP_CTRL != msg->msg_id)
                {
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_SCREEN_CTRL, DIVOOM_SERVER_TOPIC, &on_off);
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_SCREEN_STATUS, DIVOOM_APP_TOPIC, &on_off);
                }                
                divoom_light_app_load_save();
            }
            break;
        }

        case DIVOOM_LIGHT_APP_EVENT_UPDATE_SUSCRIBE_FAVORITE_LIST:
            
            break;
            
        case DIVOOM_DEVICE_SYSTEM_RESET_ALL:
        {
            
            //先删除开机动画，再向系统获取相关配置
            divoom_clock_schedule_clear_all();
            divoom_app_com_del_file(DIVOOM_USER_DEFINE_LIST_SAVE_PATH);
            divoom_app_com_del_file(DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH);
            divoom_app_com_del_file(DIVOOM_CLOCK_SCHEDULE_SAVE_PATH);          
            divoom_app_com_del_file(DIVOOM_USER_DEFINE_LIST_SAVE_PATH);  
        }   
            break;
            
        case DIVOOM_DEVICE_WIFI_CON_SRV:
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_SERVE_CHANNEL_INFO:
            //将频道里面的一些基本配置上传服务器，目前还不上传，看后面是否需要
            break;

        #if 0
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_HISTORY:
        {
            int i, j;
            Pdivoom_device_clock_history_info phistory_clock = (Pdivoom_device_clock_history_info)msg->data;

            if(phistory_clock->num > DIVOOM_LIGHT_APP_MAX_CLOCK_HISTORY_NUM)
            {
                phistory_clock->num = DIVOOM_LIGHT_APP_MAX_CLOCK_HISTORY_NUM;
            }
            pdivoom_light_app_handle->base_light.history_num = 0;
            for(i = 0; i < phistory_clock->num; i++)
            {
                pdivoom_light_app_handle->base_light.clock_history[pdivoom_light_app_handle->base_light.history_num] = phistory_clock->list[i];
                pdivoom_light_app_handle->base_light.history_num ++;
            }
            divoom_light_app_load_save();
            
        }
            break;
            #endif
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_CHANNEL_GLOBAL_CONFIG:
        {         
            int i;
            int update_flag = 0, channel_index = 0, clock_id = 10, reset_flag = 0;   
            Pdivoom_channel_channel_cfg_info pconfig_info = (Pdivoom_channel_channel_cfg_info)msg->data;


            if(pconfig_info->PowerOnClockId != pdivoom_light_app_handle->base_light.power_on_clock_id)
            {
                pdivoom_light_app_handle->base_light.power_on_clock_id = pconfig_info->PowerOnClockId;
                update_flag = 1;       
                clock_id = pdivoom_light_app_handle->base_light.power_on_clock_id;         
            }

            pdivoom_light_app_handle->first_flag = 0;

            pdivoom_light_app_handle->base_light.clock_disp_time = pconfig_info->ClockTime;
            if(pconfig_info->StartUpClockOnOff >= 0)
            {
                pdivoom_light_app_handle->base_light.power_on_disp_clock_flag = pconfig_info->StartUpClockOnOff;
            }
            pdivoom_light_app_handle->base_light.clock_disp_gif = pconfig_info->GalleryTime;
            if(pconfig_info->SingleGalleyTime >= 0)
            {
                if(pdivoom_light_app_handle->base_light.each_gif_time != pconfig_info->SingleGalleyTime)
                {
                    pdivoom_light_app_handle->base_light.each_gif_time = pconfig_info->SingleGalleyTime;
                }
            }
            else
            {
                pdivoom_light_app_handle->base_light.each_gif_time = -1;
            }

            divoom_light_app_load_save();
            
        }
            break;

        case DIVOOM_LIGHT_APP_EVENT_UPDATE_SUBCRIBE_INFO:
        {
            divoom_sys_msg_info model_msg = {0};
            Pdivoom_channel_update_subscribe_info psubscribe = (Pdivoom_channel_update_subscribe_info)msg->data;

            divoom_subscribe_manage_update(&psubscribe->clock_info, &psubscribe->subscribe_info);
            if(psubscribe->clock_info.clock_pos == 0 && psubscribe->clock_info.parent_id == 0)
            {
    			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
    			model_msg.data = (uint8 *)&psubscribe->clock_info.clock_id;
    			divoom_sys_model_crl_msg_push(&model_msg);
            }
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_PLAY_REAL_EQ:
			{
				Pdivoom_draw_remote_json_image_info image_info = (Pdivoom_draw_remote_json_image_info)msg->data;
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);

				divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
				divoom_gif_ctrl_set_callback(divoom_light_app_real_eq_gif_notice);
                divoom_user_head_manage_update_info(image_info->author_id, image_info->author_name, image_info->author_file_id);
                pdivoom_light_app_handle->remote_image_info.author_id = image_info->author_id;
                strcpy(pdivoom_light_app_handle->remote_image_info.image_name, image_info->image_name);
                strcpy(pdivoom_light_app_handle->remote_image_info.author_name, image_info->author_name);
                pdivoom_light_app_handle->wait_gif_time = time(0) + DIVOOM_LIGHT_APP_GIF_ACCPET_TIME_OUT;
				divoom_gif_ctrl_update_remote_image(image_info);			
			}
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_SET_PLAY_USER_DEFINE:
        {
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_SET_START_UP_CHANNEL:
            if(pdivoom_light_app_handle->base_light.power_on_mode != *(int *)msg->data)
            {
                pdivoom_light_app_handle->base_light.power_on_mode = *(int *)msg->data;
                divoom_light_app_load_save();
            }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_GET_START_UP_CHANNEL:
            msg->result = pdivoom_light_app_handle->base_light.power_on_mode;
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_EQ_LIST:
        {
            divoom_eq_image_manage_update_image_list(msg->data);
        }
            break;
        case DIVOOM_LIGHT_APP_EVENT_SET_EQ_POS:
        {
            Pdivoom_channel_eq_item_select pinfo = (Pdivoom_channel_eq_item_select)msg->data;
            if(pinfo)
            {
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                divoom_eq_image_manage_update_eq_mode(&pinfo->clock_info, pinfo->id);
				#if 0
				if(pinfo->clock_info.parent_id == 0)
				{
					pdivoom_light_app_handle->base_light.clock_id = pinfo->clock_info.clock_id;
					pdivoom_light_app_handle->app_request_flag = 1;
					divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
				}
				#else
                if(pinfo->clock_info.parent_id == 0)
                {
                    int prev_clock_id = pdivoom_light_app_handle->base_light.clock_id;
                    int eq_only = (prev_clock_id == pinfo->clock_info.clock_id &&
                        pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE &&
                        pdivoom_light_app_handle->active_flag);

                    pdivoom_light_app_handle->base_light.clock_id = pinfo->clock_info.clock_id;
                    if(!eq_only)
                    {
                        pdivoom_light_app_handle->app_request_flag = 1;
                        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
                    }
                }
				#endif
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                divoom_light_app_load_save();
            }
        }   
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_GET_EQ_POS:
            if(msg->result == DIVOOM_DISP_MUSIC_TMP_INDEX)
            {
                msg->result = 0;
            }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_DEL_EQ_POS:
        {
            Pdivoom_channel_eq_item_select pinfo = (Pdivoom_channel_eq_item_select)msg->data;
            divoom_eq_image_manage_delete_image_id(&pinfo->clock_info, pinfo->id - DIVOOM_DISP_MUSIC_END_MODE);
            if(divoom_eq_image_manage_get_eq_mode(&pinfo->clock_info) == pinfo->id)
            {
                divoom_eq_image_manage_update_eq_mode(&pinfo->clock_info, pinfo->id - 1);
            }
            
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_ADD_EQ_ITEM:
        {
            divoom_clock_id_info  clock_info;
            Pdivoom_channel_eq_item_info pinfo = (Pdivoom_channel_eq_item_info)msg->data;

            memset(&clock_info, 0, sizeof(clock_info));
            clock_info.clock_id = pinfo->clock_id;
            clock_info.clock_pos = pinfo->clock_pos;
            clock_info.parent_id = pinfo->parent_id;
            divoom_eq_image_manage_add_image_id(&clock_info, pinfo->update_time, pinfo->file_addr[0], pinfo->author_id[0], pinfo->image_name[0]);
            divoom_user_head_manage_update_info(pinfo->author_id[0], pinfo->author_name[0], pinfo->author_file_id[0]);            
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_SELF_CLOCK_SYS:
            {
                divoom_clock_manage_update_clock_item((Pdivoom_clock_manage_item)msg->data, 0, 1);
                divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
            }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_SYS:
            {
                Pdivoom_device_clock_list_info plist = (Pdivoom_device_clock_list_info)msg->data;
                
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                if(pdivoom_light_app_handle->clock_download_config_pending &&
                    (plist->list[0].clock_id == pdivoom_light_app_handle->pending_clock_download_id ||
                        plist->list[0].parent_clock_id == pdivoom_light_app_handle->pending_clock_download_id))
                {
                    pdivoom_light_app_handle->clock_download_config_dirty = 1;
                    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                    break;
                }
                if(plist->list[0].clock_id == pdivoom_light_app_handle->base_light.clock_id &&
                    !divoom_light_app_get_clock_download_session())
                    divoom_light_app_begin_clock_download(pdivoom_light_app_handle->base_light.clock_id);
                divoom_clock_manage_update_clock_item(&plist->list[0], 0, 1);
                if(pdivoom_light_app_handle->pending_clock_download_id != 0)
                {
                    /* 子表盘可能先回 APP 再回 SYS；SYS 落盘后重新检查根表盘。 */
                    divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_CLOCK_RESOURCE_DOWNLOAD_DONE,
                        divoom_light_app_get_clock_download_session());
                }
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_APP:
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_ALL:
        {
            Pdivoom_device_clock_list_info plist = (Pdivoom_device_clock_list_info)msg->data;

            if(plist->num == 1)
            {
                int update_flag;
                int all_file_ready;
                int clock_id = plist->list[0].parent_clock_id == 0 ?
                    plist->list[0].clock_id : plist->list[0].parent_clock_id;
                int pending_added_clock;
                
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
                if(pdivoom_light_app_handle->clock_download_config_pending &&
                    clock_id == pdivoom_light_app_handle->pending_clock_download_id)
                {
                    pdivoom_light_app_handle->clock_download_config_dirty = 1;
                    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                    break;
                }
                /* 替换配置会删除旧素材；必须先封住旧配置的播放/完成判断。 */
                if(clock_id == pdivoom_light_app_handle->base_light.clock_id &&
                    !divoom_light_app_get_clock_download_session())
                    divoom_light_app_begin_clock_download(clock_id);
                update_flag = divoom_clock_manage_update_clock_item(&plist->list[0], msg->msg_id==DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_APP?1:2,1);
                pending_added_clock = pdivoom_light_app_handle->pending_clock_download_id == clock_id;
                if(clock_id == pdivoom_light_app_handle->base_light.clock_id &&
                    ((plist->is_app == 1 && (update_flag || plist->list[0].app_num)) ||
                        pending_added_clock || (update_flag == 1 && plist->list[0].clock_id > 10000)))
                {
                    if(!divoom_light_app_get_clock_download_session())
                        divoom_light_app_begin_clock_download(clock_id);
                    pdivoom_light_app_handle->app_request_flag = 1;
					divoom_light_app_add_clock_id(pdivoom_light_app_handle->base_light.clock_id, 0);
                    all_file_ready = divoom_light_app_check_clock_download_files(pdivoom_light_app_handle->base_light.clock_id);
                    if(pending_added_clock)
                    {
                        divoom_light_app_set_clock_download_progress_state(1, 1);
                    }
                    if(all_file_ready)
                    {
                        pdivoom_light_app_handle->pending_clock_download_id = 0;
                        divoom_light_app_set_clock_download_progress_state(0, 0);
                        divoom_download_aliyun_file_set_app_last_info(0, 0);
                        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
                    }
                    else
                    {
                        divoom_light_app_set_disp_download_font(clock_id);
                        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE);
                    }
                    divoom_light_app_load_save();
                }
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                /* APP/ALL 只处理本次主表盘，预览等非“我的表盘”回包不进索引。 */
                if(divoom_light_app_clock_id_in_list(
                    pdivoom_light_app_handle->base_light.clock_history,
                    pdivoom_light_app_handle->base_light.history_num, clock_id))
                {
                    (void)divoom_clock_component_index_refresh(clock_id);
                }
            }
            else
            {
                //divoom_clock_manage_update_all(plist->list, plist->num, plist->update_time);
            }
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_LIST:
            divoom_clock_manage_update_clock_list_time((Pdivoom_devcice_time_dial_clock_list_respone)msg->data);
            break;

        case DIVOOM_LIGHT_APP_EVENT_DEL_USER_DEFINE_ONE:
        {            
            Pdivoom_devcice_user_define_item pimage_file = (Pdivoom_devcice_user_define_item)msg->data;

            divoom_user_define_ctrl_del_item(pimage_file);
        }
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_DEL_USER_DEFINE_INDEX:
        {
            Pdivoom_devcice_user_define_item pimage_file = (Pdivoom_devcice_user_define_item)msg->data;   
                divoom_user_define_ctrl_clean_item(pimage_file);
        }
            break;

        case DIVOOM_LIGHT_APP_EVENT_ADD_USER_DEFINE_INDEX:
        {
            Pdivoom_devcice_user_define_item pimage_file = (Pdivoom_devcice_user_define_item)msg->data;
            
            divoom_user_define_ctrl_set_item(pimage_file);
            
        }
            break;

        case DIVOOM_LIGHT_APP_EVENT_MENU_SET_CLOCK_INDEX:
        {
            int *cur_clock_id = divoom_queue_pop(pdivoom_light_app_handle->next_clock_queue);

            if(cur_clock_id)
            {
                divoom_sys_msg_info model_msg = {0};
    			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
    			model_msg.data = (uint8 *)cur_clock_id;
    			divoom_sys_model_crl_msg_push(&model_msg);
                divoom_system_free(cur_clock_id);
            }
        }
            break;
        case DIVOOM_LIGHT_APP_EVENT_NET_SET_CLOCK_INDEX:
        case DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX:
        {
            int clock_id, sys_update_time = 0;
            int report_select = 0;
            uint32 intent;
#if DIVOOM_SHOW_MODE
            int show_rotate = 0;
#endif

            if(msg->msg_id == DIVOOM_LIGHT_APP_EVENT_NET_SET_CLOCK_INDEX)
            {
                Pdivoom_channel_select_clock_index select_clock = (Pdivoom_channel_select_clock_index)msg->data;
                clock_id = select_clock->clock_id;
                sys_update_time = select_clock->sys_update_time;
#if DIVOOM_SHOW_MODE
                divoom_light_app_show_clock_rotate_pause();
#endif
            }
#if DIVOOM_SHOW_MODE
            else if(msg->src_id == DIVOOM_MODEL_LIGHT_APP && msg->data == NULL)
            {
                clock_id = divoom_light_app_get_next_clock_from_id(
                    pdivoom_light_app_handle->base_light.clock_info.clock_id);
                if(clock_id <= 0)
                {
                    break;
                }
                show_rotate = 1;
            }
#elif DIVOOM_LIGHT_APP_RANDOM_CLOCK_ENABLE
            else if(msg->src_id == DIVOOM_MODEL_LIGHT_APP && msg->data == NULL)
            {
                /* 在消息处理线程选取表盘，测试线程不直接访问历史列表。 */
                int count;
                int index;
                int previous;

				divoom_light_app_clock_list_lock();
				count = pdivoom_light_app_handle->base_light.history_num;
                if(count <= 0 || pdivoom_light_app_handle->base_light.clock_history == NULL)
                {
					divoom_light_app_clock_list_unlock();
                    break;
                }
                index = rand() % count;
                if(count > 1 && pdivoom_light_app_handle->base_light.clock_history[index] ==
                    pdivoom_light_app_handle->base_light.clock_info.clock_id)
                {
                    index = (index + 1 + rand() % (count - 1)) % count;
                }
                clock_id = pdivoom_light_app_handle->base_light.clock_history[index];
				previous = pdivoom_light_app_handle->base_light.clock_info.clock_id;
				divoom_light_app_clock_list_unlock();
                divoom_print("[my_clock_leak] select id=%d index=%d count=%d previous=%d\n",
                    clock_id, index, count, previous);
            }
#endif
            else
            {
                clock_id = *(int *)msg->data;
            }
            if(msg->msg_id == DIVOOM_LIGHT_APP_EVENT_NET_SET_CLOCK_INDEX &&
                clock_id < DIVOOM_CUSTOM_START_CLOCK_ID &&
                clock_id == pdivoom_light_app_handle->pending_clock_download_id)
            {
                /* MyClockAdd 后紧跟的选择命令复用同一下载，等待 ALL 回包统一统计。 */
                divoom_print("clock select joins download: %d\n", clock_id);
                break;
            }
            intent = divoom_light_app_reserve_clock_intent();
            if(divoom_clock_play_plan_take_internal_restore_flag() == 0)
            {
                divoom_clock_play_plan_user_dismiss();
            }
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
            if(!divoom_light_app_clock_intent_is_current(intent))
            {
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                break;
            }
            /* 获取锁前可能刚开始完整配置请求，同 ID 选择继续复用该等待。 */
            if(pdivoom_light_app_handle->clock_download_config_pending &&
                clock_id == pdivoom_light_app_handle->pending_clock_download_id)
            {
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                break;
            }
            if (pdivoom_light_app_handle->pending_clock_download_id != 0 ||
                (clock_id > 0 && clock_id != pdivoom_light_app_handle->base_light.clock_info.clock_id) ||
                (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID) ||
                (pdivoom_light_app_handle->cur_mode != DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE))
            {
                Pdivoom_clock_manage_item pclock_info;
                pclock_info = divoom_clock_manage_get_raw_clock_info(clock_id, 0, 0);
                if(pdivoom_light_app_handle->pending_clock_download_id != 0 &&
                    pdivoom_light_app_handle->pending_clock_download_id != clock_id)
                {
                    pdivoom_light_app_handle->pending_clock_download_id = 0;
                    divoom_light_app_clear_download_font_queue();
                    divoom_light_app_set_clock_download_progress_state(0, 0);
                }
                if(msg->src_id != DIVOOM_LAN_SOURCE &&
                    (pclock_info == NULL || pclock_info->dial_config_timeout || pclock_info->load_app_config))
                {
                    if(pclock_info)
                    {
                        divoom_clock_manage_free_clock(pclock_info);
                    }
                    /* 先等有效配置，再由 UPDATE_CLOCK_ALL 检查新配置的资源。 */
                    divoom_print("clock select: fetch config asynchronously id=%d\n", clock_id);
                    /* 重读配置可能已就绪，select_added_clock 的播放分支会重新取此锁。 */
                    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
                    divoom_light_app_select_added_clock(clock_id,
                        pdivoom_light_app_handle->base_light.clock_mode, intent, 0);
                    divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_WAIT_CLOCK_DOWNLOAD,
                        divoom_light_app_get_clock_download_session());
                    break;
                }
                
                if(pclock_info)
                {
                    /* A complete local static dial legitimately has no APP_LIST.
                     * Explicit SysUpdateTime checks below still refresh stale metadata. */
                     #if 0
                    if(msg->msg_id == DIVOOM_LIGHT_APP_EVENT_NET_SET_CLOCK_INDEX && clock_id < DIVOOM_USER_START_CLOCK_ID && pclock_info->app_num == 0
                        && pclock_info->sys_update_time == 0
                       )
                    {
                        //通过APP选择表盘，如果没有应用的，主动获取一下表盘配置
                        divoom_clock_id_info clock_info = {0};
                        
                        clock_info.clock_id = clock_id;
                        if(clock_info.clock_id < DIVOOM_USER_START_CLOCK_ID)
                        {
                            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA, DIVOOM_SERVER_TOPIC,&clock_info);
                        }
                    }
                    #endif
                    //divoom_cache_clock_data_clear_all();
                    pdivoom_light_app_handle->base_light.clock_id = clock_id;
                    pdivoom_light_app_handle->base_light.mode = DIVOOM_DISP_CLOCK_MODE;
                    pdivoom_light_app_handle->app_request_flag = 1;
                    divoom_light_app_begin_clock_download(clock_id);
                    if(divoom_wifi_model_get_wifi_status() == 0 || divoom_light_app_check_clock_download_files(pdivoom_light_app_handle->base_light.clock_id))
                    {
                        pdivoom_light_app_handle->pending_clock_download_id = 0;
                        divoom_light_app_set_clock_download_progress_state(0, 0);
                        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
                    }
                    else
                    {
                        divoom_light_app_set_disp_download_font(pdivoom_light_app_handle->base_light.clock_id);
                        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE);
                    }
#if DIVOOM_SHOW_MODE
                    if(!show_rotate)
                    {
                        divoom_light_app_load_save();
                    }
#else
                    divoom_light_app_load_save();
#endif
                    /* APP 路径已在 net_app 上报 SetClockSelectId；设备端切表盘在此补报，避免开机被云端旧 LastClockId 覆盖。 */
                    if(
#if DIVOOM_SHOW_MODE
                        !show_rotate &&
#endif
                        msg->msg_id == DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX && msg->src_id != DIVOOM_LAN_SOURCE && !divoom_lan_clock_exists(clock_id))
                    {
                        report_select = 1;
                    }
                    if(sys_update_time && pclock_info->sys_update_time < sys_update_time)
                    {
                        divoom_clock_id_info request = {0};
                        request.clock_id = clock_id;
                        divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA, DIVOOM_SERVER_TOPIC, &request);
                    }
                    divoom_clock_manage_free_clock(pclock_info);
                }
                else
                {
                    divoom_print("this clock isn't exist!: %d\n", clock_id);
                }
            }
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            if(report_select)
            {
                divoom_light_app_report_selected_clock_id(pdivoom_light_app_handle->base_light.clock_id);
            }
        }
			break;

		case DIVOOM_LIGHT_APP_EVENT_SET_NEXT_CLOCK:
			divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
			pdivoom_light_app_handle->base_light.clock_id = divoom_light_app_get_next_clock_id();
            pdivoom_light_app_handle->base_light.mode = DIVOOM_DISP_CLOCK_MODE;
    	    pdivoom_light_app_handle->app_request_flag = 1;
            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            divoom_light_app_load_save();
            divoom_light_app_report_selected_clock_id(pdivoom_light_app_handle->base_light.clock_id);
        	break;
            
        case DIVOOM_LIGHT_APP_EVENT_SET_PRODUCE_TIME:
//            pdivoom_light_app_handle->base_light.product_time = *(int *)msg->data;
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_SET_CUSTOM_TIME:
//            pdivoom_light_app_handle->base_light.user_define_info.insert_time = *(int *)msg->data;
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_GET_CUSTOM_TIME:
         //   msg->result = pdivoom_light_app_handle->base_light.user_define_info.insert_time;
            break;
         
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_REAL_EQDATA_MODE:
			{
				divoom_clock_id_info clock_info = {0};
 				clock_info.clock_id = DIVOOM_TMP_EQ_CLOCK_ID;
	            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
				divoom_eq_image_manage_update_eq_mode(&clock_info, DIVOOM_DISP_MUSIC_TMP_INDEX);
				pdivoom_light_app_handle->base_light.clock_id = DIVOOM_TMP_EQ_CLOCK_ID;
	    	    pdivoom_light_app_handle->app_request_flag = 1;
				divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
	            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
			}
            break;
            
        case DIVOOM_LIGHT_APP_EVENT_ADD_REAL_EQ_DATA:
			{
				Pdivoom_draw_local_json_image_info image_info = (Pdivoom_draw_local_json_image_info)msg->data;
                
				if (image_info->offset == 0)
					divoom_gif_ctrl_set_callback(divoom_light_app_real_eq_gif_notice);
				divoom_gif_ctrl_update_local_image(image_info);	
			}
            break;
        
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_HTTP_GIF_MODE:
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
			divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_HTTP_GIF_MODE);
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;

            
        case DIVOOM_LIGHT_APP_EVENT_UPDATE_DATA_MODE:
            pdivoom_light_app_handle->draw_info.gif_info.attr_change_flag = 0;
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
//			if (*msg->data == 1)
//			{
//				memset(&pdivoom_light_app_handle->draw_info, 0, sizeof(divoom_disp_ctrl_phone_draw_pic_info));
//				pdivoom_light_app_handle->draw_info.flag = DIVOOM_DISP_PHONE_SET_PIC;
//				divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE);
//			}
//			else
			{
				pdivoom_light_app_handle->gif_info.attr_change_flag = 0;
				divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE);
			}
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            break;

            
		case DIVOOM_LIGHT_APP_EVENT_DRAW_REMOTE: //显示服务器上的动画
			{
				Pdivoom_draw_remote_json_image_info image_info = (Pdivoom_draw_remote_json_image_info)msg->data;
//                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);

//				divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE);
//                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
				divoom_gif_ctrl_set_callback(divoom_light_app_local_gif_notice);
                pdivoom_light_app_handle->wait_gif_time = time(0) + DIVOOM_LIGHT_APP_GIF_ACCPET_TIME_OUT;
                //divoom_user_head_manage_update_info(image_info->author_id, image_info->author_name, image_info->author_file_id);
                pdivoom_light_app_handle->remote_image_info.author_id = image_info->author_id;
                strcpy(pdivoom_light_app_handle->remote_image_info.image_name, image_info->image_name);
                strcpy(pdivoom_light_app_handle->remote_image_info.author_name, image_info->author_name);
                strcpy(pdivoom_light_app_handle->remote_image_info.voice_id, image_info->voice_id);
				divoom_gif_ctrl_update_remote_image(image_info);			
			}
			break;

		case DIVOOM_LIGHT_APP_EVENT_DRAW_REMOTE_BY_APP: 
			{
				Pdivoom_draw_remote_by_app_json_image_info image_info = (Pdivoom_draw_remote_by_app_json_image_info)msg->data;
				if (image_info->offset == 0)
				{
					if(image_info->pic_len < image_info->total_len)
					{
						divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
						divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_NOTICE_MSG_MODE);
						pdivoom_light_app_handle->wait_gif_time = time(0) + DIVOOM_LIGHT_APP_GIF_ACCPET_TIME_OUT;
						divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
					}
					divoom_gif_ctrl_set_callback(divoom_light_app_local_gif_notice);
				}
				else
				{
					pdivoom_light_app_handle->wait_gif_time = time(0) + DIVOOM_LIGHT_APP_GIF_ACCPET_TIME_OUT;
				}
				divoom_gif_ctrl_update_remote_by_app_image(image_info);			
			}
			break;
		
		case DIVOOM_LIGHT_APP_EVENT_DRAW_SET_INFO:
			{
				Pdivoom_draw_set_info_json_image_info set_image_info = (Pdivoom_draw_set_info_json_image_info)msg->data;
				divoom_draw_set_speed_json_mode_info speed_mode_info = {0};
				
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
				speed_mode_info.speed = pdivoom_light_app_handle->gif_info.attr_info.gif_speed;
				speed_mode_info.scroll_mode = pdivoom_light_app_handle->gif_info.attr_info.scorll_mode;
				if (pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE || 
					pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE)//动画模式
				{
					pdivoom_light_app_handle->gif_info.attr_change_flag = 2;
					pdivoom_light_app_handle->gif_info.use_flag = 1;
					memcpy(&pdivoom_light_app_handle->gif_info.attr_info, set_image_info, sizeof(divoom_draw_set_info_json_image_info));
				}
				else if (pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE)//画板模式
				{
					pdivoom_light_app_handle->draw_info.gif_info.attr_change_flag = 1;
					pdivoom_light_app_handle->draw_info.flag = DIVOOM_DISP_PHONE_SET_OTHER;
					memcpy(&pdivoom_light_app_handle->draw_info.gif_info.attr_info, set_image_info, sizeof(divoom_draw_set_info_json_image_info));
				}
				pdivoom_light_app_handle->gif_info.attr_info.gif_speed = speed_mode_info.speed;
				pdivoom_light_app_handle->gif_info.attr_info.scorll_mode = speed_mode_info.scroll_mode;
                pdivoom_light_app_handle->app_request_flag = 1;
	            divoom_light_app_request_resource(pdivoom_light_app_handle->cur_mode);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
			}
			break;
		
		case DIVOOM_LIGHT_APP_EVENT_DRAW_SET_SPEED_MODE_INFO:
			{
				Pdivoom_draw_set_speed_json_mode_info set_image_info = (Pdivoom_draw_set_speed_json_mode_info)msg->data;
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);

				if (pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE || 
					pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE)//动画模式
				{
					pdivoom_light_app_handle->gif_info.attr_change_flag = 1;
					pdivoom_light_app_handle->gif_info.use_flag = 1;
					pdivoom_light_app_handle->gif_info.attr_info.gif_speed = set_image_info->speed;
					pdivoom_light_app_handle->gif_info.attr_info.scorll_mode = set_image_info->scroll_mode;
				}
				else if (pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE)//画板模式
				{
					pdivoom_light_app_handle->draw_info.flag = DIVOOM_DISP_PHONE_SET_OTHER;
					pdivoom_light_app_handle->draw_info.gif_info.attr_change_flag = 1;
					pdivoom_light_app_handle->draw_info.gif_info.attr_info.gif_speed = set_image_info->speed;
					pdivoom_light_app_handle->draw_info.gif_info.attr_info.scorll_mode = set_image_info->scroll_mode;
				}
                pdivoom_light_app_handle->app_request_flag = 1;
				divoom_light_app_request_resource(pdivoom_light_app_handle->cur_mode);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
			}
			break;

        case DIVOOM_LIGHT_APP_EVENT_ENTER_SYNC_MODE: //实时画板
            {
                Pdivoom_drawing_json_sync_image_info dot_info = (Pdivoom_drawing_json_sync_image_info)msg->data;
                
                divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
				pdivoom_light_app_handle->draw_info.flag = DIVOOM_DISP_PHONE_SET_DOT;
                if(pdivoom_light_app_handle->draw_info.dot_info.off_table)
                {
                    divoom_system_free(pdivoom_light_app_handle->draw_info.dot_info.off_table);
                }
                pdivoom_light_app_handle->draw_info.dot_info.off_table = dot_info->dot_pos;
                dot_info->dot_pos = NULL;
                pdivoom_light_app_handle->draw_info.dot_info.color.r_c = dot_info->color[0];
                pdivoom_light_app_handle->draw_info.dot_info.color.g_c = dot_info->color[1];
                pdivoom_light_app_handle->draw_info.dot_info.color.b_c = dot_info->color[2];
                pdivoom_light_app_handle->draw_info.dot_info.num = dot_info->num;
                pdivoom_light_app_handle->app_request_flag = 1;
	            divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE);
                divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
            }
            break;

        
        case DIVOOM_LIGHT_APP_EVENT_EXIT_SYNC_MODE: //退出实时画板
            {
             	//如果是黑屏则退出显示
             	divoom_light_app_exit_drawing_pad();
            }
            break;

		case DIVOOM_LIGHT_APP_EVENT_UPDATE_SYS_COMPONENT_LIST:
			//divoom_component_manage_update_component_list_time((Pdivoom_component_config_request_component_update_time_list)msg->data);
			break;
				
		case DIVOOM_LIGHT_APP_EVENT_UPDATE_SYS_COMPONENT_INFO:
			//divoom_component_manage_update_component_sys_conf((Pdivoom_component_sys_config)msg->data);
			break;
        
		default:
			divoom_print("No this command:%d in this model!\n", msg->msg_id);
			return 0;
			break;
	}
	return 1;
}

void divoom_light_app_change_default_clock(int lang)
{
	const int *default_clock_history;
	int *new_clock_history;
	int *old_clock_history;
	int history_num;
	int clock_id;

	if(pdivoom_light_app_handle == NULL)
	{
		return;
	}
    if(lang == DIVOOM_LVGL_MENU_LANG_SC)
    {
		default_clock_history = (const int *)divoom_defult_clock_history_cn;
		history_num = sizeof(divoom_defult_clock_history_cn)/sizeof(divoom_defult_clock_history_cn[0]);
		clock_id = 2388;
    }
    else
    {
		default_clock_history = (const int *)divoom_defult_clock_history_en;
		history_num = sizeof(divoom_defult_clock_history_en)/sizeof(divoom_defult_clock_history_en[0]);
		clock_id = 1642;
    }
	new_clock_history = divoom_system_malloc(sizeof(int) * history_num);
	if(new_clock_history == NULL)
	{
		divoom_print_err("alloc default clock history failed, num=%d\n", history_num);
		return;
	}
	memcpy(new_clock_history, default_clock_history, sizeof(int) * history_num);

	divoom_light_app_clock_list_lock();
	old_clock_history = pdivoom_light_app_handle->base_light.clock_history;
	pdivoom_light_app_handle->base_light.clock_history = new_clock_history;
	pdivoom_light_app_handle->base_light.history_num = history_num;
	pdivoom_light_app_handle->base_light.clock_id = clock_id;
	if(old_clock_history != NULL)
	{
		divoom_system_free(old_clock_history);
	}
	divoom_light_app_clock_list_unlock();
}


void divoom_light_app_load_default_config(void)
{
    int i, j;

    divoom_print("we will load default config!\n");
    pdivoom_light_app_handle->cur_mode = DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE;
    pdivoom_light_app_handle->base_light.power_on_mode = 0;
    pdivoom_light_app_handle->base_light.mode = DIVOOM_DISP_SUBSCRIBE_MODE;
    pdivoom_light_app_handle->base_light.clock_disp_gif = 60;
    pdivoom_light_app_handle->base_light.clock_disp_time = 60;
    pdivoom_light_app_handle->base_light.each_gif_time = 0;
    pdivoom_light_app_handle->base_light.play_roll = 0;
    divoom_light_app_change_default_clock(divoom_main_set_lang_flag(-1));
    pdivoom_light_app_handle->base_light.clock_id = pdivoom_light_app_handle->base_light.clock_history[0];
}

//flag=0是移除收藏，否则添加收藏
void divoom_light_app_add_remove_fav_clock(int clock_id, int flag)
{
    int i;
    int changed = 0;

    if(pdivoom_light_app_handle == NULL)
    {
        return;
    }
    divoom_light_app_clock_list_lock();
    if(flag == 0)
    {
        for(i = 0; i < pdivoom_light_app_handle->base_light.fav_num; i++)
        {
            if(pdivoom_light_app_handle->base_light.clock_fav[i] == clock_id)
            {
                if(i != pdivoom_light_app_handle->base_light.fav_num -1)
                {
                    memmove(&pdivoom_light_app_handle->base_light.clock_fav[i], &pdivoom_light_app_handle->base_light.clock_fav[i + 1], (pdivoom_light_app_handle->base_light.fav_num - (i+1)) * sizeof(int));
                }
                pdivoom_light_app_handle->base_light.fav_num --;
                if(pdivoom_light_app_handle->base_light.fav_num == 0)
                {
                    divoom_system_free(pdivoom_light_app_handle->base_light.clock_fav);
                    pdivoom_light_app_handle->base_light.clock_fav = NULL;
                }
                changed = 1;
                break;
            }
        }
    }
    else
    {
        int *clock_list;

        for(i = 0; i < pdivoom_light_app_handle->base_light.fav_num; i++)
        {
            if(pdivoom_light_app_handle->base_light.clock_fav[i] == clock_id)
            {
                divoom_light_app_clock_list_unlock();
                return;
            }
        }
        clock_list = divoom_system_malloc(sizeof(int) * (pdivoom_light_app_handle->base_light.fav_num + 1));
        if (clock_list == NULL)
        {
            divoom_light_app_clock_list_unlock();
            return;
        }
        if(pdivoom_light_app_handle->base_light.fav_num)
        {
            memcpy(clock_list, pdivoom_light_app_handle->base_light.clock_fav, sizeof(int) * pdivoom_light_app_handle->base_light.fav_num);
            divoom_system_free(pdivoom_light_app_handle->base_light.clock_fav);
        }
        clock_list[pdivoom_light_app_handle->base_light.fav_num] = clock_id;
        pdivoom_light_app_handle->base_light.clock_fav = clock_list;
        pdivoom_light_app_handle->base_light.fav_num++;
        changed = 1;
    }
    divoom_light_app_clock_list_unlock();
    if(changed)
    {
        divoom_light_app_load_save();
    }
}


static int divoom_light_app_collect_clock_ids(const int *ids, int count, int *out, int max_count)
{
    int i;
    int n = 0;

    if(ids == NULL || out == NULL || count <= 0 || max_count <= 0)
    {
        return 0;
    }
    for(i = 0; i < count && n < max_count; i++)
    {
        if(ids[i] > 0)
        {
            out[n++] = ids[i];
        }
    }
    return n;
}

void divoom_light_app_replace_recent_clock_list(const int *ids, int count)
{
    int kept[DIVOOM_RECENT_USED_CLOCK_MAX_NUM];
    int n;

    if(pdivoom_light_app_handle == NULL)
    {
        return;
    }
    n = divoom_light_app_collect_clock_ids(ids, count, kept, DIVOOM_RECENT_USED_CLOCK_MAX_NUM);
    if(n <= 0)
    {
        return;
    }
    divoom_light_app_clock_list_lock();
    memcpy(pdivoom_light_app_handle->base_light.clock_recent_used, kept, (size_t)n * sizeof(int));
    pdivoom_light_app_handle->base_light.recent_used_num = n;
    divoom_light_app_clock_list_unlock();
    divoom_light_app_load_save();
}

void divoom_light_app_replace_fav_clock_list(const int *ids, int count)
{
    int *kept;
    int n;
    int cap;

    if(pdivoom_light_app_handle == NULL || ids == NULL || count <= 0)
    {
        return;
    }
    cap = count;
    if(cap > DIVOOM_MAX_CLOCK_NUM)
    {
        cap = DIVOOM_MAX_CLOCK_NUM;
    }
    kept = divoom_system_malloc(sizeof(int) * cap);
    if(kept == NULL)
    {
        return;
    }
    n = divoom_light_app_collect_clock_ids(ids, count, kept, cap);
    if(n <= 0)
    {
        divoom_system_free(kept);
        return;
    }
    divoom_light_app_clock_list_lock();
    if(pdivoom_light_app_handle->base_light.clock_fav)
    {
        divoom_system_free(pdivoom_light_app_handle->base_light.clock_fav);
    }
    pdivoom_light_app_handle->base_light.clock_fav = kept;
    pdivoom_light_app_handle->base_light.fav_num = n;
    divoom_light_app_clock_list_unlock();
    divoom_light_app_load_save();
}

void divoom_light_app_report_add_recent_clock(int clock_id)
{
    if(clock_id <= 0)
    {
        return;
    }
    divoom_net_app_send_packet(DIVOOM_NET_COMM_ADD_MY_CLOCK_RECENT, DIVOOM_SERVER_TOPIC, &clock_id);
}

void divoom_light_app_report_set_favorite(int clock_id, int favorite_flag)
{
    divoom_channel_my_clock_favorite_info info;

    if(clock_id <= 0)
    {
        return;
    }
    info.clock_id = clock_id;
    info.favorite_flag = favorite_flag ? 1 : 0;
    divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_MY_CLOCK_FAVORITE, DIVOOM_SERVER_TOPIC, &info);
}

//增加一个最近使用的表盘， flag=0只是使用，flag=1,使用时间达到标准。返回 1 表示列表有变化。
int divoom_light_app_add_recent_clock(int clock_id, int flag)
{
    int i;
    int changed = 0;

    if(pdivoom_light_app_handle == NULL)
    {
        return 0;
    }
    divoom_light_app_clock_list_lock();
    if(flag == 0)
    {
        if(pdivoom_light_app_handle->base_light.recent_used_num == DIVOOM_RECENT_USED_CLOCK_MAX_NUM)
        {
            divoom_light_app_clock_list_unlock();
            return 0;
        }
        for(i = 0; i < pdivoom_light_app_handle->base_light.recent_used_num; i++)
        {
            if(pdivoom_light_app_handle->base_light.clock_recent_used[i] == clock_id)
            {
                divoom_light_app_clock_list_unlock();
                return 0;
            }
        }
        pdivoom_light_app_handle->base_light.clock_recent_used[pdivoom_light_app_handle->base_light.recent_used_num] = clock_id;
        pdivoom_light_app_handle->base_light.recent_used_num ++;
        changed = 1;
    }
    else
    {
        for(i = 0; i < pdivoom_light_app_handle->base_light.recent_used_num; i++)
        {
            if(pdivoom_light_app_handle->base_light.clock_recent_used[i] == clock_id)
            {
                if(i == 0)
                {
                    divoom_light_app_clock_list_unlock();
                    return 0;
                }
                memmove(&pdivoom_light_app_handle->base_light.clock_recent_used[1],
                    &pdivoom_light_app_handle->base_light.clock_recent_used[0],
                    (size_t)i * sizeof(int));
                pdivoom_light_app_handle->base_light.clock_recent_used[0] = clock_id;
                changed = 1;
                break;
            }
        }
        if(!changed)
        {
            if(pdivoom_light_app_handle->base_light.recent_used_num < DIVOOM_RECENT_USED_CLOCK_MAX_NUM)
            {
                if(pdivoom_light_app_handle->base_light.recent_used_num > 0)
                {
                    memmove(&pdivoom_light_app_handle->base_light.clock_recent_used[1],
                        &pdivoom_light_app_handle->base_light.clock_recent_used[0],
                        (size_t)pdivoom_light_app_handle->base_light.recent_used_num * sizeof(int));
                }
                pdivoom_light_app_handle->base_light.recent_used_num ++;
            }
            else
            {
                memmove(&pdivoom_light_app_handle->base_light.clock_recent_used[1],
                    &pdivoom_light_app_handle->base_light.clock_recent_used[0],
                    (DIVOOM_RECENT_USED_CLOCK_MAX_NUM - 1) * sizeof(int));
            }
            pdivoom_light_app_handle->base_light.clock_recent_used[0] = clock_id;
            changed = 1;
        }
    }
    divoom_light_app_clock_list_unlock();
    if(changed)
    {
        divoom_light_app_load_save();
    }
    return changed;
}



void divoom_light_app_load_config(void)
{    
    int i, j;
    char *json_info;
    divoom_cJSON *root, *list, *item, *obj, *sub_list, *sub_item;


    json_info = divoom_app_com_read_file(DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH, NULL);
    if(json_info == NULL)
    {
        goto divoom_light_app_load_config_default_info;
        return ;
    }
    root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    if(root == NULL)
    {
        goto divoom_light_app_load_config_default_info;
        return ;
    }
    pdivoom_light_app_handle->custom_clock_num = divoom_cJSON_GetObjectInt(root, "custom_clock_nums");
    item = divoom_cJSON_GetObjectItem(root, "light_switch");
    if(item)
    {
        pdivoom_light_app_handle->light_switch = divoom_cJSON_GetObjectInt(root, "light_switch");
    }
    else
    {
        pdivoom_light_app_handle->light_switch = 1;
    }
    pdivoom_light_app_handle->base_light.power_on_disp_clock_flag = divoom_cJSON_GetObjectInt(root, "power_on_disp_clock_flag");
    pdivoom_light_app_handle->base_light.vision_play_time = divoom_cJSON_GetObjectInt(root, "vision_play_time");
    pdivoom_light_app_handle->base_light.disp_mode = divoom_cJSON_GetObjectInt(root, "disp_mode");
    pdivoom_light_app_handle->base_light.power_on_mode = divoom_cJSON_GetObjectInt(root, "power_on_mode");
    pdivoom_light_app_handle->base_light.power_on_clock_id = divoom_cJSON_GetObjectInt(root, "power_on_clock_id");
    pdivoom_light_app_handle->base_light.clock_id = divoom_cJSON_GetObjectInt(root, "clock_id");
    pdivoom_light_app_handle->base_light.each_gif_time = divoom_cJSON_GetObjectInt(root, "each_gif_time");
    pdivoom_light_app_handle->base_light.divoom_app_animation_edge_id = divoom_cJSON_GetObjectInt(root, "divoom_app_animation_edge_id");
    list = divoom_cJSON_GetObjectItem(root, "clock_history");
    pdivoom_light_app_handle->base_light.history_num = divoom_cJSON_GetArraySize(list);
    if(pdivoom_light_app_handle->base_light.history_num)
    {
        pdivoom_light_app_handle->base_light.clock_history = divoom_system_malloc(sizeof(int)*pdivoom_light_app_handle->base_light.history_num);
        for(i = 0; i < pdivoom_light_app_handle->base_light.history_num; i++)
        {
            item  = divoom_cJSON_GetArrayItem(list, i);
            if(item)
            {
                pdivoom_light_app_handle->base_light.clock_history[i] = item->valueint;
            }
        }
    }
    else
    {
        divoom_light_app_change_default_clock(divoom_main_set_lang_flag(-1));
    }
    if(pdivoom_light_app_handle->base_light.clock_id == 0)
    {
        pdivoom_light_app_handle->base_light.clock_id = pdivoom_light_app_handle->base_light.clock_history[0];
    }
    
    list = divoom_cJSON_GetObjectItem(root, "clock_fav");
    pdivoom_light_app_handle->base_light.fav_num = divoom_cJSON_GetArraySize(list);
    if(pdivoom_light_app_handle->base_light.fav_num)
    {
        pdivoom_light_app_handle->base_light.clock_fav = divoom_system_malloc(sizeof(int)*pdivoom_light_app_handle->base_light.fav_num);
        for(i = 0; i < pdivoom_light_app_handle->base_light.fav_num; i++)
        {
            item  = divoom_cJSON_GetArrayItem(list, i);
            if(item)
            {
                pdivoom_light_app_handle->base_light.clock_fav[i] = item->valueint;
            }
        }
    }
    else
    {
        pdivoom_light_app_handle->base_light.fav_num = 0;
        pdivoom_light_app_handle->base_light.clock_fav = NULL;
    }
    
    list = divoom_cJSON_GetObjectItem(root, "clock_recent_used");
    pdivoom_light_app_handle->base_light.recent_used_num = divoom_cJSON_GetArraySize(list);
    if(pdivoom_light_app_handle->base_light.recent_used_num > DIVOOM_RECENT_USED_CLOCK_MAX_NUM)
    {
        pdivoom_light_app_handle->base_light.recent_used_num = DIVOOM_RECENT_USED_CLOCK_MAX_NUM;
    }
    if(pdivoom_light_app_handle->base_light.recent_used_num)
    {
        for(i = 0; i < pdivoom_light_app_handle->base_light.recent_used_num; i++)
        {
            item  = divoom_cJSON_GetArrayItem(list, i);
            if(item)
            {
                pdivoom_light_app_handle->base_light.clock_recent_used[i] = item->valueint;
            }
        }
    }

    
    divoom_cJSON_Delete(root);
    return;
    divoom_light_app_load_config_default_info:
    pdivoom_light_app_handle->light_switch = 1;
    pdivoom_light_app_handle->base_light.vision_play_time = 0;
    pdivoom_light_app_handle->base_light.divoom_app_animation_edge_id = DIVOOM_ANIMATION_CLOCK_ID;
    divoom_light_app_change_default_clock(divoom_main_set_lang_flag(-1));
}
void divoom_light_app_del_config(void)
{
    char *json_info;
    divoom_cJSON *root, *list, *item;
    int *ids = NULL;
    int count, i, saved;

    if(pdivoom_light_app_handle != NULL)
    {
        divoom_light_app_clock_list_lock();
        count = pdivoom_light_app_handle->base_light.history_num;
        saved = divoom_clock_manage_record_removed_clock_list(
            pdivoom_light_app_handle->base_light.clock_history, count);
        divoom_print("[storage_gc] del_config: source=memory count=%d recorded=%d\n", count, saved);
        if(saved)
        {
            divoom_app_com_del_file(DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH);
        }
        divoom_light_app_clock_list_unlock();
        return;
    }

    /* Product-test boot calls this before both managers are initialized. */
    json_info = divoom_app_com_read_file(DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH, NULL);
    if(json_info == NULL)
    {
        divoom_print("[storage_gc] del_config: config missing or unreadable, keep file\n");
        return;
    }
    root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    if(root == NULL)
    {
        divoom_print("[storage_gc] del_config blocked: invalid JSON\n");
        return;
    }
    list = divoom_cJSON_GetObjectItem(root, "clock_history");
    if(list != NULL && !(list->type & divoom_cJSON_Array))
    {
        divoom_print("[storage_gc] del_config blocked: invalid clock_history\n");
        divoom_cJSON_Delete(root);
        return;
    }
    count = divoom_cJSON_GetArraySize(list);
    if(count < 0 || count > DIVOOM_MAX_CLOCK_NUM)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    if(count > 0)
    {
        ids = divoom_system_malloc((uint32)count * sizeof(*ids));
        if(ids == NULL)
        {
            divoom_cJSON_Delete(root);
            return;
        }
        for(i = 0; i < count; i++)
        {
            item = divoom_cJSON_GetArrayItem(list, i);
            if(item == NULL || !(item->type & divoom_cJSON_Number))
            {
                divoom_print("[storage_gc] del_config blocked: invalid clock ID index=%d\n", i);
                divoom_system_free(ids);
                divoom_cJSON_Delete(root);
                return;
            }
            ids[i] = item->valueint;
        }
    }
    saved = divoom_clock_manage_record_removed_clock_list(ids, count);
    divoom_print("[storage_gc] del_config: source=file count=%d recorded=%d\n", count, saved);
    divoom_system_free(ids);
    divoom_cJSON_Delete(root);
    if(saved)
    {
        divoom_app_com_del_file(DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH);
    }
}

static void divoom_light_app_report_selected_clock_id(int clock_id)
{
    int report_id;

    if(divoom_main_get_custom_product_flag())
    {
        return;
    }
    if(clock_id <= 0)
    {
        return;
    }
    report_id = clock_id;
    divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_CLOCK_SELECT_ID,
        DIVOOM_SERVER_TOPIC, (void *)&report_id);
}

void divoom_light_app_load_save(void)
{
    int i, j;
    char *json_info;
    divoom_cJSON *root, *list, *item, *obj, *sub_list, *sub_item;

	if(pdivoom_light_app_handle == NULL)
	{
		return;
	}
	root = divoom_cJSON_CreateObject();

    if(root == NULL)
    {
        return;
    }
    divoom_cJSON_AddNumberToObject(root , "custom_clock_num", pdivoom_light_app_handle->custom_clock_num);
    divoom_cJSON_AddNumberToObject(root , "vision_play_time", pdivoom_light_app_handle->base_light.vision_play_time);
    divoom_cJSON_AddNumberToObject(root , "light_switch", pdivoom_light_app_handle->light_switch);
    divoom_cJSON_AddNumberToObject(root , "disp_mode", pdivoom_light_app_handle->base_light.disp_mode);
    divoom_cJSON_AddNumberToObject(root , "power_on_mode", pdivoom_light_app_handle->base_light.power_on_mode);
    divoom_cJSON_AddNumberToObject(root , "power_on_clock_id", pdivoom_light_app_handle->base_light.power_on_clock_id);
    divoom_cJSON_AddNumberToObject(root , "power_on_disp_clock_flag", pdivoom_light_app_handle->base_light.power_on_disp_clock_flag);
    divoom_cJSON_AddNumberToObject(root , "clock_id", pdivoom_light_app_handle->base_light.clock_id);
    divoom_cJSON_AddNumberToObject(root , "each_gif_time", pdivoom_light_app_handle->base_light.each_gif_time);
    divoom_cJSON_AddNumberToObject(root , "divoom_app_animation_edge_id", pdivoom_light_app_handle->base_light.divoom_app_animation_edge_id);

	/* CreateIntArray 会逐项读取源数组；与列表扩容/释放互斥，避免读到旧指针。 */
	divoom_light_app_clock_list_lock();
    divoom_cJSON_AddItemToObject(root, "clock_history", divoom_cJSON_CreateIntArray(pdivoom_light_app_handle->base_light.clock_history, pdivoom_light_app_handle->base_light.history_num));
    divoom_cJSON_AddItemToObject(root, "clock_fav", divoom_cJSON_CreateIntArray(pdivoom_light_app_handle->base_light.clock_fav, pdivoom_light_app_handle->base_light.fav_num));
    divoom_cJSON_AddItemToObject(root, "clock_recent_used", divoom_cJSON_CreateIntArray(pdivoom_light_app_handle->base_light.clock_recent_used, pdivoom_light_app_handle->base_light.recent_used_num));
	divoom_light_app_clock_list_unlock();
    
    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
	if(json_info == NULL)
	{
		return;
	}
    divoom_app_com_write_file(DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
    divoom_system_free(json_info);
    exec_command_system("sync");
    
}

//这个模块只是显示一些标准的控制１６的ＲＧＢ效果
void divoom_light_app_update_last_clock_id(int clock_id)
{
    /* Keep the recovery dial selected at startup when login returns the old one. */
    if(divoom_main_is_app_exit_flag() >= DIVOOM_LIGHT_APP_DEFAULT_CLOCK_RESTARTS ||
        divoom_main_get_custom_product_flag())
    {
        return ;
    }
    if(clock_id == 0)
    {
        clock_id = DIVOOM_START_CLOCK_ID;
    }
    if(pdivoom_light_app_handle->watch_dog_flag == 1 || pdivoom_light_app_handle->base_light.power_on_disp_clock_flag == 0)
    {
        pdivoom_light_app_handle->base_light.clock_id = clock_id;
    }
}
#if DIVOOM_LIGHT_APP_RANDOM_CLOCK_ENABLE
static void divoom_light_app_test_include_log_clocks(void)
{
    const int log_clock_list[] = {
        2603, 2216, 2631, 1848, 2432, 2654, 2152, 1880, 1748, 1853,
        1817, 2327, 2373, 1872, 2138, 2331, 2206, 2122, 1912, 2410
    };
    uint32 i;

    /* 仅专用仿真构建把故障日志中的高风险表盘加入内存列表，正式配置文件保持不变。 */
    for(i = 0; i < (uint32)(sizeof(log_clock_list) / sizeof(log_clock_list[0])); i++)
    {
        divoom_light_app_add_clock_id(log_clock_list[i], 0);
    }
}

static void* divoom_light_app_test_change_clock(void *param)
{
    divoom_sys_msg_info model_msg = {0};
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
    uint32 sample = 0;
#endif

    (void)param;
    model_msg.src_id = DIVOOM_MODEL_LIGHT_APP;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
    model_msg.msg_type = 1;
    /* NULL 数据表示从当前历史列表随机选取，消息队列负责异步处理。 */
    divoom_system_delay_ms(DIVOOM_LIGHT_APP_RANDOM_CLOCK_START_DELAY_MS);
    while(1)
    {
        if(divoom_wifi_model_get_wifi_status())
        {
            /* 云端初始化可能重写我的表盘，每轮先补齐专用复现集合。 */
            divoom_light_app_test_include_log_clocks();
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
            lv_mem_monitor_t mem_info;
            uint32 tick = divoom_system_get_tickcount();

            divoom_print("[my_clock_leak] sample_begin=%u tick=%u menu=%u\n",
                sample, tick, divoom_menu_control_get_active_menu_id());
            divoom_malloc_print();
            lv_mem_monitor(&mem_info);
            divoom_print("[lvgl_heap] sample=%u used_bytes=%u used_blocks=%u free_bytes=%u largest_free=%u frag_pct=%u\n",
                sample, mem_info.total_size - mem_info.free_size, mem_info.used_cnt,
                mem_info.free_size, mem_info.free_biggest_size, mem_info.frag_pct);
            sim_lvgl_leak_report(sample);
            divoom_print("[my_clock_leak] sample_end=%u\n", sample);
            sample++;
#endif
            divoom_sys_model_crl_msg_push(&model_msg);
        }
        divoom_system_delay_ms(DIVOOM_LIGHT_APP_RANDOM_CLOCK_INTERVAL_MS);
    }
    return NULL;
}
#endif

#if DIVOOM_SHOW_MODE
void divoom_light_app_show_clock_rotate_pause(void)
{
    divoom_show_mode_ctrl_notice();
}

static int divoom_light_app_show_clock_rotate_ready(void)
{
    if(pdivoom_light_app_handle == NULL)
    {
        return 0;
    }
    if(divoom_lvgl_poweron_is_boot_pending())
    {
        return 0;
    }
    if(divoom_product_test_is_active())
    {
        return 0;
    }
    if(divoom_onoff_is_poweroff_active())
    {
        return 0;
    }
    if(pdivoom_light_app_handle->light_switch == 0)
    {
        return 0;
    }
    if(divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
    {
        return 0;
    }
    if(!divoom_show_mode_ctrl_idle_ready())
    {
        return 0;
    }
    return 1;
}

static void *divoom_light_app_show_clock_rotate_task(void *param)
{
    divoom_sys_msg_info model_msg = {0};

    (void)param;
    model_msg.src_id = DIVOOM_MODEL_LIGHT_APP;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
    model_msg.msg_type = 1;
    while(1)
    {
        divoom_system_delay_ms(DIVOOM_SHOW_CLOCK_ROTATE_INTERVAL_MS);
        if(divoom_show_mode_ctrl_idle_ready()
            && divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
        {
            divoom_show_mode_ctrl_try_return_dial();
        }
        if(divoom_light_app_show_clock_rotate_ready())
        {
            divoom_sys_model_crl_msg_push(&model_msg);
        }
    }
    return NULL;
}
#endif

//对APP的文件进行加锁控制
void divoom_light_app_app_gif_file_lock_ctrl(int flag)
{
    if(flag)
    {
        divoom_system_get_lock(pdivoom_light_app_handle->app_gif_file_lock);
    }
    else
    {
        divoom_system_free_lock(pdivoom_light_app_handle->app_gif_file_lock);
    }
}


//这个模块只是显示一些标准的控制１６的ＲＧＢ效果
void divoom_light_app_init(int sys_reset_status)
{
    divoom_app_schedule_item_info channel_schedule = {0};
    
	if(pdivoom_light_app_handle)
	{
		return;
	}

	pdivoom_light_app_handle = divoom_system_malloc(sizeof(divoom_light_app_ctrl));
	if(pdivoom_light_app_handle == NULL)
	{
		return ;
	}
	memset(pdivoom_light_app_handle, 0, sizeof(divoom_light_app_ctrl));
    /* SetNew 初始态只显示默认钢琴页，不得把索引 0 当作一次激活。 */
    pdivoom_light_app_handle->mix_music2.icon = 0xff;
    divoom_light_app_load_config();   
    if(pdivoom_light_app_handle->custom_clock_num == 0 && divoom_main_get_custom_product_flag())
    {
        pdivoom_light_app_handle->custom_clock_num = 1;
    }
    /* 升级、异常及普通重启均沿用保存的开关屏状态。 */


    pdivoom_light_app_handle->first_flag = 1;
    pdivoom_light_app_handle->lock_handle = divoom_system_create_lock();
    pdivoom_light_app_handle->clock_list_lock = divoom_system_create_lock();
    pdivoom_light_app_handle->app_gif_file_lock = divoom_system_create_lock();
    /* 网络与 LIGHT_APP 两条回包路径会共同刷新索引，启动阶段先创建模块锁。 */
    (void)divoom_clock_component_index_init();
    pdivoom_light_app_handle->server_gif_file_id = DIVOOM_LIGHT_APP_SERVER_GIF_INITIAL_FILE_ID;
    pdivoom_light_app_handle->server_gif_file_generation = DIVOOM_LIGHT_APP_SERVER_GIF_VOICE_FILE_ID;
    /* 初始化解码句柄前清空 Server GIF 目录，避免复用上次运行遗留的图像或语音。 */
    divoom_light_app_cleanup_server_gif_dir();
    pdivoom_light_app_handle->server_gif_del_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_light_app_handle->gif_file_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_light_app_handle->next_clock_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_light_app_handle->download_font_lock = divoom_system_create_lock();
    pdivoom_light_app_handle->download_font_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_light_app_handle->cur_mode = DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE;
    pdivoom_light_app_handle->playlist_id = divoom_userimagelist_manage_set_temp_album_id(-1);
    channel_schedule.model_id= DIVOOM_MODEL_LIGHT_APP;  
    channel_schedule.event_id = DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT;  
    channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT; 
    divoom_app_schedule_push(&channel_schedule);
    pdivoom_light_app_handle->cur_mode = DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE;
    if(sys_reset_status < DIVOOM_LIGHT_APP_DEFAULT_CLOCK_RESTARTS && divoom_main_get_custom_product_flag() &&
        pdivoom_light_app_handle->base_light.clock_history)
    {
        pdivoom_light_app_handle->base_light.clock_id = pdivoom_light_app_handle->base_light.clock_history[0];
    }
    else if(sys_reset_status)
    {
        pdivoom_light_app_handle->watch_dog_flag = 1;
        divoom_print("system reboot and we will restore!: %d\n", sys_reset_status);
        if(sys_reset_status >= DIVOOM_LIGHT_APP_DEFAULT_CLOCK_RESTARTS)
        {
            //如果连续两次启动失败，将显示默认的表盘
            pdivoom_light_app_handle->base_light.clock_id = DIVOOM_DEFAULT_CLOCK_ID;
        }
    }
    else if(pdivoom_light_app_handle->base_light.power_on_clock_id && pdivoom_light_app_handle->base_light.power_on_disp_clock_flag)
    {
        pdivoom_light_app_handle->base_light.clock_id = pdivoom_light_app_handle->base_light.power_on_clock_id;
    }
    
    if(pdivoom_light_app_handle->light_switch == 0)
    {
        /* 仅恢复背光，不改写已保存的关屏状态。 */
        divoom_screen_bright(0); 
    }
	divoom_sys_model_crl_register(DIVOOM_MODEL_LIGHT_APP, divoom_light_app_event);
#ifndef DIVOOM_SIMULATOR
    //divoom_system_create_task(divoom_light_app_check_font_task, "light_font", NULL, DIVOOM_TASK_STACK_LIGHT_CHECK_FONT);
#endif
    /* 仿真器不在 init 时推 START_UP_DISP_DIAL：会与开机动画竞态，约 800ms 就切表盘导致视频只显示首帧 */
#if DIVOOM_LIGHT_APP_RANDOM_CLOCK_ENABLE
    if(!divoom_system_create_task(divoom_light_app_test_change_clock, "light_clk_t", NULL, DIVOOM_TASK_STACK_LIGHT_TEST_CLOCK))
    {
        divoom_print_err("create random clock task failed\n");
    }
#endif
#if DIVOOM_SHOW_MODE
    if(!divoom_system_create_task(divoom_light_app_show_clock_rotate_task, "show_clk_r", NULL, DIVOOM_TASK_STACK_SHOW_CLOCK_ROTATE))
    {
        divoom_print_err("create show clock rotate task failed\n");
    }
#endif

}

uint8 divoom_light_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{
	T_U8 cur_mode = 0;
	
    divoom_print("run event-id: %d\n", event_id);
    pdivoom_light_app_handle->active_flag = 1;
	switch(event_id)
    {
        case DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_EVENT:
            divoom_clock_schedule_restore_run();
            break;
            
        case DIVOOM_LIGHT_APP_IFFT_TEXT_EVENT:
			cur_mode = DIVOOM_LIGHT_APP_DISP_IFFT_TEXT_MODE;
            break;
        
        case DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT:
			cur_mode = DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE;
            break;

        case DIVOOM_LIGHT_APP_HTTP_GIF_CTRL_EVENT:
			cur_mode = DIVOOM_LIGHT_APP_DISP_HTTP_GIF_MODE;
            break;

        case DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT:
			cur_mode = DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE;
            break;
            
        case DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT:
        	{
				//还原上一次实时画板的图片和文字数据
				pdivoom_light_app_handle->draw_info.flag = DIVOOM_DISP_PHONE_RESUME_PIC;
				cur_mode = DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE;
			}
			break;
		case DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_DISP:
            cur_mode = DIVOOM_LIGHT_APP_DISP_ENTER_GOOGLE_CALENDAR;
            break;
            
		case DIVOOM_LIGHT_APP_OUTLOOK_CALENDAR_DISP:
            cur_mode = DIVOOM_LIGHT_APP_DISP_ENTER_OUTLOOK_CALENDAR;
            break;
            
		case DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT:
            /* A resource/OTA release may uncover a clock selected while its page was held. */
            cur_mode = divoom_light_app_get_clock_download_session() ?
                DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE : DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE;
            if (cur_mode == DIVOOM_LIGHT_APP_DISP_APP_ADD_PHOTO_MODE)
                divoom_print("light app: resume pending clock download after scheduler release\n");
			break;
         
		case DIVOOM_LIGHT_APP_SCHEDULE_CLOCK_EVENT:
			cur_mode = DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_MODE;
			break;
		
		case DIVOOM_LIGHT_APP_DEMO_DIAL_DISP:
			cur_mode = DIVOOM_LIGHT_APP_DISP_DEMO_DIAL_MODE;
			break;

        case DIVOOM_LIGHT_APP_MIXER_EVENT:
        {
            cur_mode = DIVOOM_LIGHT_APP_DISP_MIXER_MODE;
            break;
        }

        case DIVOOM_LIGHT_APP_MIX_MUSIC2_EVENT:
        {
            cur_mode = DIVOOM_LIGHT_APP_DISP_MIX_MUSIC2_MODE;
            break;
        }

		 default:
            return 0;
    }

	pdivoom_light_app_handle->cur_mode = cur_mode;
    pdivoom_light_app_handle->app_request_flag = 1;
    divoom_light_app_start_disp();
	return true;
}

uint8 divoom_light_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
    pdivoom_light_app_handle->active_flag = 0;
	switch(event_id)
    {
        case DIVOOM_LIGHT_APP_CUSTOM_DIAL_DISP:
            return 0;
            
        case DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT:
            return 0;
            
        case DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_EVENT:
            divoom_google_calendar_user_cancle_event();
            divoom_outlook_calendar_user_cancle_event();
            return 0;
            
        case DIVOOM_LIGHT_APP_CLOCK_NOTICE_EVENT:
            divoom_home_connect_cancel_event();
            return 0;
            
        case DIVOOM_LIGHT_APP_NOTICE_MSG_EVENT:
            return 0;
            
        case DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT:
            if(active_model == DIVOOM_MODEL_LIGHT_APP && (active_event == DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT
                || active_event == DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT || active_event == DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT) )
            {
                return 0;
            }
            return 1;

        case DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT:
            if(active_model == DIVOOM_MODEL_LIGHT_APP && (active_event == DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT
                || active_event == DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT || active_event == DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT) )
            {
                return 0;
            }
            return 1;
            
        case DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT:
            if(active_model == DIVOOM_MODEL_LIGHT_APP && (active_event == DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT
                || active_event == DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT || active_event == DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT) )
            {
                return 0;
            }
            else
        	{
				//保存当前图
				divoom_light_app_save_real_draw_pic_data();
			}
			return 1;

		case DIVOOM_LIGHT_APP_DEMO_DIAL_DISP:
			return 0;

        case DIVOOM_LIGHT_APP_MIXER_EVENT:
        {
            return 0;
        }

        case DIVOOM_LIGHT_APP_MIX_MUSIC2_EVENT:
        {
            return 0;
        }
    }
	return true;
}

uint8 divoom_light_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
    if(event_id == DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_EVENT)
    {
        divoom_google_calendar_user_cancle_event();
        divoom_outlook_calendar_user_cancle_event();
    }
    pdivoom_light_app_handle->active_flag = 0;
	return true;
}

void *divoom_light_app_system_dial_clearall_user_data(void)
{
	if (!pdivoom_light_app_handle)
		return NULL;

	pdivoom_light_app_handle->base_light.clock_id =
			divoom_light_app_check_clock_exist(pdivoom_light_app_handle->base_light.clock_id);
	if (pdivoom_light_app_handle->base_light.clock_id == 0)
		return NULL;

	pdivoom_light_app_handle->base_light.clock_info.clock_id =
			pdivoom_light_app_handle->base_light.clock_id;
	pdivoom_light_app_handle->base_light.clock_info.disp_mode =
			pdivoom_light_app_handle->base_light.clock_mode;
	return (void *)&pdivoom_light_app_handle->base_light.clock_info;
}

/* 调用者持有业务锁；网络期间释放锁，返回时仍持锁，配置快照由调用者释放。 */
static Pdivoom_clock_manage_item divoom_light_app_refresh_clock_config(
    divoom_clock_id_info *request, uint32 session)
{
    Pdivoom_net_command packet;
    Pdivoom_device_clock_list_info config;
    Pdivoom_clock_manage_item saved;
    int attempt;

    if(!divoom_light_app_clock_download_session_is_current(session))
        return NULL;
    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
    packet = NULL;
    for(attempt = 0; attempt < 2; attempt++)
    {
        uint32 reconnect_start = divoom_system_get_tickms();
        /* MQTT can reconnect while Wi-Fi stays up. Keep this session, not the old dial. */
        while(!divoom_net_app_net_is_ok() && divoom_wifi_model_get_wifi_status() &&
            divoom_light_app_clock_download_session_is_current(session) &&
            (uint32)(divoom_system_get_tickms() - reconnect_start) < 15000U)
            divoom_system_delay_ms(50);
        if(!divoom_light_app_clock_download_session_is_current(session) ||
            !divoom_wifi_model_get_wifi_status() || !divoom_net_app_net_is_ok())
            break;
        packet = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA,
            DIVOOM_SERVER_TOPIC, request);
        if(packet)
            break;
        divoom_print("clock config retry: id=%d pos=%d parent=%d attempt=%d wifi=%d mqtt=%d\n",
            request->clock_id, request->clock_pos, request->parent_id, attempt + 1,
            divoom_wifi_model_get_wifi_status(), divoom_net_app_net_is_ok());
    }
    divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
    if(!divoom_light_app_clock_download_session_is_current(session))
    {
        divoom_net_app_free_packet(packet);
        return NULL;
    }
    config = packet ? (Pdivoom_device_clock_list_info)packet->cmd_data : NULL;
    if(config == NULL || config->num != 1 || config->list[0].clock_id != request->clock_id)
    {
        divoom_print("clock config unavailable: id=%d pos=%d parent=%d session=%u\n",
            request->clock_id, request->clock_pos, request->parent_id, session);
        divoom_net_app_free_packet(packet);
        return NULL;
    }
    /* 请求携带的父 ID 和位置才是本次引用的身份，不能保存成独立表盘。 */
    config->list[0].parent_clock_id = request->parent_id;
    config->list[0].clock_pos = request->clock_pos;
    divoom_clock_manage_update_clock_item(&config->list[0], 2, 1);
    divoom_net_app_free_packet(packet);
    saved = divoom_clock_manage_get_raw_clock_info(request->clock_id,
        request->clock_pos, request->parent_id);
    if(saved && (saved->dial_config_timeout || saved->load_app_config))
    {
        divoom_clock_manage_free_clock(saved);
        return NULL;
    }
    if(saved)
    {
        divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
        pdivoom_light_app_handle->clock_download_config_completed++;
        divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    }
    return saved;
}

/* 所有引用的子配置均更新后才返回成功；重复引用按 ID、位置去重。 */
static int divoom_light_app_refresh_sub_clock_configs(Pdivoom_clock_manage_item root, uint32 session)
{
    divoom_clock_id_info requests[DIVOOM_CLOCK_DISP_SUPPORT_DAIL_COMPONENT_END_ID -
        DIVOOM_CLOCK_DISP_SUPPORT_DAIL_COMPONENT_START_ID];
    char *config_text;
    int count = 0;
    int i, j;

    if(root->app_num != 1 || root->papp_list == NULL || root->papp_list[0].Config == NULL)
        return 1;
    config_text = root->papp_list[0].Config;
    for(i = 0; i < root->disp_item_num; i++)
    {
        int child_id = 0;
        int position;
        char *suffix;
        if(root->pdisp_item_list[i].type < DIVOOM_CLOCK_DISP_SUPPORT_DAIL_COMPONENT_START_ID ||
            root->pdisp_item_list[i].type >= DIVOOM_CLOCK_DISP_SUPPORT_DAIL_COMPONENT_END_ID)
            continue;
        divoom_app_com_get_int_from_config(config_text, root->pdisp_item_list[i].item_id, &child_id);
        if(child_id <= 0)
            continue;
        suffix = strchr(root->pdisp_item_list[i].item_id, '_');
        position = suffix ? atoi(suffix + 1) : 0;
        for(j = 0; j < count; j++)
            if(requests[j].clock_id == child_id && requests[j].clock_pos == position)
                break;
        if(j < count)
            continue;
        if(count >= (int)(sizeof(requests) / sizeof(requests[0])))
            return 0;
        requests[count].clock_id = child_id;
        requests[count].clock_pos = position;
        requests[count].parent_id = root->clock_id;
        count++;
    }

    for(i = 0; i < count; i++)
    {
        Pdivoom_clock_manage_item saved;
        divoom_print("clock add child config: root=%d, child=%d, pos=%d, %d/%d\n",
            root->clock_id, requests[i].clock_id, requests[i].clock_pos, i + 1, count);
        saved = divoom_light_app_refresh_clock_config(&requests[i], session);
        if(saved == NULL)
            return 0;
        divoom_clock_manage_free_clock(saved);
    }
    return 1;
}

static void divoom_light_app_select_added_clock(int clock_id, int clock_mode, uint32 intent,
    uint8 refresh_config)
{
    Pdivoom_clock_manage_item pclock_info;
    divoom_clock_id_info clock_info = {0};
    int all_file_ready;

    uint32 session;

    divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
    if(!divoom_light_app_clock_intent_is_current(intent))
    {
        divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
        return;
    }
    pdivoom_light_app_handle->base_light.clock_id = clock_id;
    pdivoom_light_app_handle->base_light.clock_mode = clock_mode;
    pdivoom_light_app_handle->base_light.mode = DIVOOM_DISP_CLOCK_MODE;
    pdivoom_light_app_handle->app_request_flag = 1;
    session = divoom_light_app_begin_clock_download(clock_id);
    if(refresh_config)
    {
        divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
        pdivoom_light_app_handle->clock_download_config_pending = 1;
        divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    }
    divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW,
        (void *)(uintptr_t)session, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT);
    /* 列表更新可能阻塞；先公布会话，使紧随的同 ID 选择复用新增流程。 */
    if(refresh_config)
        divoom_light_app_add_clock_id(clock_id, 0);
    divoom_light_app_load_save();

    if(refresh_config)
    {
        /* 等待页先入队；网络阻塞期间不持业务锁，切换和退出可正常推进。 */
        divoom_download_aliyun_file_set_app_last_info(1, 0);
        clock_info.clock_id = clock_id;
        divoom_light_app_report_selected_clock_id(clock_id);
        for(int refresh_attempt = 0; refresh_attempt < 3; refresh_attempt++)
        {
            /* 子配置请求会释放业务锁；期间的 APP/SYS 更新不能被旧快照吞掉。 */
            pdivoom_light_app_handle->clock_download_config_dirty = 0;
            pclock_info = divoom_light_app_refresh_clock_config(&clock_info, session);
            if(pclock_info && !divoom_light_app_refresh_sub_clock_configs(pclock_info, session))
            {
                divoom_clock_manage_free_clock(pclock_info);
                pclock_info = NULL;
            }
            if(pclock_info == NULL || !pdivoom_light_app_handle->clock_download_config_dirty)
                break;
            divoom_clock_manage_free_clock(pclock_info);
            pclock_info = NULL;
            /* 连续更新最多重取三轮，耗尽后走失败退出，不能放行已过时的配置。 */
            divoom_print("clock config changed while pending: id=%d session=%u round=%d/3%s\n",
                clock_id, session, refresh_attempt + 1,
                refresh_attempt < 2 ? ", refresh again" : ", abort unstable config");
        }
        clock_info.clock_id = 0;
    }
    else
    {
        pclock_info = divoom_clock_manage_get_raw_clock_info(clock_id, 0, 0);
        if(pclock_info && (pclock_info->dial_config_timeout || pclock_info->load_app_config))
        {
            divoom_clock_manage_free_clock(pclock_info);
            pclock_info = NULL;
        }
    }
    if(pclock_info == NULL)
    {
        if(refresh_config)
        {
            if(divoom_light_app_clock_download_session_is_current(session))
                divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD, session);
        }
        else
        {
            divoom_download_aliyun_file_set_app_last_info(1, 0);
            clock_info.clock_id = clock_id;
        }
        if(!refresh_config)
            divoom_light_app_report_selected_clock_id(clock_id);
        divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
        if(!refresh_config)
        {
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA,
                DIVOOM_SERVER_TOPIC, &clock_info);
        }
        return;
    }
    if(refresh_config)
    {
        divoom_system_get_lock(pdivoom_light_app_handle->download_font_lock);
        pdivoom_light_app_handle->clock_download_config_pending = 0;
        divoom_system_free_lock(pdivoom_light_app_handle->download_font_lock);
    }
    all_file_ready = divoom_light_app_check_clock_download_files(clock_id);
    divoom_light_app_set_clock_download_progress_state(1, 1);
    if(all_file_ready)
    {
        pdivoom_light_app_handle->pending_clock_download_id = 0;
        divoom_light_app_set_clock_download_progress_state(0, 0);
        divoom_download_aliyun_file_set_app_last_info(0, 0);
        divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
    }
    else
    {
        divoom_light_app_set_disp_download_font(clock_id);
    }

    divoom_clock_manage_free_clock(pclock_info);
    if(!refresh_config)
        divoom_light_app_report_selected_clock_id(clock_id);
    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
    if(refresh_config)
        (void)divoom_clock_component_index_refresh(clock_id);
}

void divoom_light_app_set_clock_from_lvgl_dial_preview(int clock_id, int clock_mode)
{
	pdivoom_light_app_handle->base_light.clock_id = clock_id;
	pdivoom_light_app_handle->base_light.clock_mode = clock_mode;
    divoom_light_app_report_selected_clock_id(clock_id);

	divoom_light_app_load_save();
    pdivoom_light_app_handle->app_request_flag = 1;
	if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE)
	{
		if(pdivoom_light_app_handle->active_flag == 1)
		{
            divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);
			divoom_light_app_start_disp();
            divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
			return;
		}
	}
    divoom_system_get_lock(pdivoom_light_app_handle->lock_handle);

    divoom_light_app_request_resource(DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE);
    divoom_system_free_lock(pdivoom_light_app_handle->lock_handle);
}

uint8 divoom_light_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
	//TODO
	uint8 event_id;

    divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
    divoom_print("aj test!: %d\n", event_id);
	if(DIVOOM_LIGHT_APP_IFFT_TEXT_EVENT == event_id)
	{
        //如果有按键，将取消这个显示
        divoom_sys_msg_info model_msg = {0};
        model_msg.src_id = DIVOOM_MODEL_LIGHT_APP;
        model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;  
        model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_EXIT_DISPLAY_IFFFT_TEXT;
        model_msg.msg_type = 1;
        divoom_sys_model_crl_msg_push(&model_msg);
        
        return 1;
	}
    else if(DIVOOM_LIGHT_APP_CLOCK_NOTICE_EVENT == event_id)
    {
        divoom_sys_msg_info model_msg = {0};
        model_msg.src_id = DIVOOM_MODEL_LIGHT_APP;
        model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;  
        model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_EXIT_NOTICE_CLOCK;
        model_msg.msg_type = 1;
        divoom_sys_model_crl_msg_push(&model_msg);
        return 1;
    }
    else if(DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT == event_id)
    {
        if(key_id == DIVOOM_KEY_ESC || key_id == DIVOOM_KEY_LIGHT_ENTER ||
            key_id == DIVOOM_KEY_PLAY)
        {
            divoom_sys_msg_info model_msg = {0};
            model_msg.src_id = DIVOOM_MODEL_LIGHT_APP;
            model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;  
            /* 下载页的 ESC/ENTER 仅投递退出事件，避免按键线程执行清队列和切页。 */
            model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD;
            model_msg.result = (int)divoom_light_app_get_clock_download_session();
            model_msg.msg_type = 1;
            divoom_sys_model_crl_msg_push(&model_msg);
            return 1;
        }
    }
    else if(DIVOOM_LIGHT_APP_SCHEDULE_CLOCK_EVENT == event_id)
    {
        divoom_clock_play_plan_user_dismiss();
        return 1;
    }
    else if(DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_EVENT == event_id)
    {
        divoom_google_calendar_user_cancle_event();
        divoom_outlook_calendar_user_cancle_event();
        return 1;
    }
    
    divoom_print("aj test!: %d\n", pdivoom_light_app_handle->cur_mode);
	if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_BASE_LIGHT_MODE)
	{
        if(pdivoom_light_app_handle->active_flag == 1)
        {
            switch(key_id)
            {
                case DIVOOM_KEY_LIGHT_RIGHT:
                    break;

                case DIVOOM_KEY_LIGHT_LEFT:
                    break;

                case DIVOOM_KEY_LIGHT_ENTER:
                    break;
            }
            return 1;
        }
	}
    else if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE ||
        pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE ||
        pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE ||
        pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_MODE ||
        pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_ENTER_GOOGLE_CALENDAR ||
        pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_ENTER_OUTLOOK_CALENDAR)
    {
        if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_CLOCK_SCHEDULE_MODE)
        {
            divoom_app_schedule_item_info event_info = {0};
            event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
            event_info.event_id = DIVOOM_LIGHT_APP_SCHEDULE_CLOCK_EVENT;
    		event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&event_info);
        }
        else if(DIVOOM_KEY_ESC == key_id)
        {
            divoom_app_schedule_item_info event_info = {0};
            if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_SERVER_GIF_MODE)
            {
                event_info.event_id = DIVOOM_LIGHT_APP_SERVER_DRAW_CTRL_EVENT;
            }
            else if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_LOCAL_GIF_MODE)
            {
                event_info.event_id = DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT;
            }
            else if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_REAL_DRAW_MODE)
            {
                event_info.event_id = DIVOOM_LIGHT_APP_REAL_DRAW_CTRL_EVENT;
            }      
            else if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_ENTER_GOOGLE_CALENDAR)
            {
                event_info.event_id = DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_DISP;
            }                    
            else if(pdivoom_light_app_handle->cur_mode == DIVOOM_LIGHT_APP_DISP_ENTER_OUTLOOK_CALENDAR)
            {
                event_info.event_id = DIVOOM_LIGHT_APP_OUTLOOK_CALENDAR_DISP;
            }
            event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
    		event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&event_info);
        }
    }
    //
	return 0;
}

/*
  Function:       divoom_light_app_check_cloud_photo_clock
  Description:    检查是否是客户定制表盘
  Input:          
  Return:         1:　or 0
*/

int divoom_light_app_check_cloud_photo_clock(int clock_id)
{
    for(int i = 0; i < pdivoom_light_app_handle->custom_clock_num; i++)
    {
        if(pdivoom_light_app_handle->base_light.clock_history[i] == clock_id)
        {
            return 1;
        }
    }
    return 0;
}

/*
  Function:       divoom_light_app_check_notice_status
  Description:    检查是否在表盘通知状态
  Input:          
  Return:         1:在表盘通知，0：不在表盘通知
*/

uint8 divoom_light_app_check_notice_status(void )
{
    uint8 event_id, model_id;

    model_id = divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);

    return ((model_id == DIVOOM_MODEL_LIGHT_APP) && (event_id == DIVOOM_LIGHT_APP_CLOCK_NOTICE_EVENT));
}

void divoom_light_ctrl_main_loop(void)
{
    int cur_time;
    static int request_gif_packet_time = 0;
    
    if(pdivoom_light_app_handle)
    {
        if(pdivoom_light_app_handle->wait_gif_time)
        {
            cur_time = time(0);
            
            if(pdivoom_light_app_handle->wait_gif_time < cur_time)
            {
                if(request_gif_packet_time > 3)
                {
                    divoom_app_schedule_item_info event_info = {0};
                    
                    event_info.model_id = DIVOOM_MODEL_LIGHT_APP;
                    event_info.event_id = DIVOOM_LIGHT_APP_NOTICE_MSG_EVENT;
            		event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
                    divoom_app_schedule_cancel(&event_info);    
                    pdivoom_light_app_handle->wait_gif_time = 0;
                    request_gif_packet_time = 0;
                }
                else
                {
                    pdivoom_light_app_handle->wait_gif_time = cur_time;
                    divoom_gif_ctrl_request_other_pacet();
                    request_gif_packet_time ++;
                }
            }
        }
        else
        {
            request_gif_packet_time = 0;
        }
            }
}

void divoom_light_ctrl_deinit(void)
{
}
