#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_white_noise_player.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_leave_msg_ctrl.h"

/* 同线程重入看护：持 event_lock 再 push/cancel 只打印+记死锁文件，不拦截。1 开 / 0 关 */
#ifndef DIVOOM_APP_SCHEDULE_REENTRY_GUARD
#define DIVOOM_APP_SCHEDULE_REENTRY_GUARD	1
#endif

//还是根据模块的优先级来决定改怎么操作
//要有一个基础模块,每个新模块都跟queue队首元素对比,队首的元素一定是最高优先级的
//跟队首的对比之后,从数组里找到这个是需要push的还是active的,还是active_push的还是cancel的

typedef struct _divoom_app_schedule_queue_item_info
{
	uint8 model_id;
	uint8 event_id;
	uint8 destroy_flag; //当PUSH时，已经有这个事件了，是否需要调用DESTROY通知应用
	uint8 cur_model_id;//当前在表格中的第几个
	int priority;
	void *user_data;
}divoom_app_schedule_queue_item_info, *Pdivoom_app_schedule_queue_item_info;

typedef struct _divoom_app_schedule_ctrl
{
	void *ptask_queue[DIVOOM_APP_SCHEDULE_RESOURCCE_END];
	//Pdivoom_app_schedule_queue_item_info pcur_active_event;
	//浪费空间
	//divoom_app_schedule_model_info model_info[DIVOOM_MODEL_END];
	void *event_lock;
	void *thread_id;
}divoom_app_schedule_ctrl, *Pdivoom_app_schedule_ctrl;

int divoom_app_schedule_compare_event(void *item1, void *item2);

static Pdivoom_app_schedule_ctrl pdivoom_app_schedule_handle = NULL;
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
static void *s_event_lock_owner;
static uint8 s_event_lock_owned;
static uint8 s_cb_model_id;
static uint8 s_cb_event_id;
static const char *s_cb_kind;

static void divoom_app_schedule_mark_lock_owned(void)
{
	s_event_lock_owner = divoom_system_thread_self();
	s_event_lock_owned = 1;
}

static void divoom_app_schedule_clear_lock_owned(void)
{
	s_event_lock_owned = 0;
}

static int divoom_app_schedule_in_locked_ctx(void)
{
	return (s_event_lock_owned
		&& divoom_system_thread_equal(s_event_lock_owner, divoom_system_thread_self())) ? 1 : 0;
}

static void divoom_app_schedule_warn_reentry(const char *api, uint8 model_id, uint8 event_id)
{
	char buf[256];

	if (!divoom_app_schedule_in_locked_ctx())
		return;
	divoom_print("schedule reentry: %s model=%u event=%u while cb=%s model=%u event=%u\n",
		api, (unsigned)model_id, (unsigned)event_id,
		s_cb_kind ? s_cb_kind : "?", (unsigned)s_cb_model_id, (unsigned)s_cb_event_id);
	snprintf(buf, sizeof(buf),
		"schedule reentry:%s model=%u event=%u cb=%s:%u:%u\n",
		api, (unsigned)model_id, (unsigned)event_id,
		s_cb_kind ? s_cb_kind : "?", (unsigned)s_cb_model_id, (unsigned)s_cb_event_id);
	divoom_main_update_dead_lock_file(buf);
}
#endif
/** 为 1 时 cancel 释放资源后不 start_new_event（如助眠定时关机，避免切回歌词表盘） */
static uint8 pdivoom_app_schedule_suppress_resume = 0;
void divoom_app_schedule_main(void *arg);

#define DIVOOM_APP_SCHDULE_SAME_MODEL	6

typedef struct _divoom_app_schedule_priority_info
{
	uint8 model_id;
	uint8 event_id;
	uint16 priority;
    int    menu_list[20]; //事件对应的MENU ID,当这个事项要destroy时，会清除这个列表的MENU ID, 最后一项为0终止
}divoom_app_schedule_priority_info, *Pdivoom_app_schedule_priority_info;

//需要使用的,添加进来,不然用上面的方法内存使用太多
const static divoom_app_schedule_model_info all_model_info[] = 
{
	{DIVOOM_MODEL_MAIN, divoom_main_app_schedule_play, divoom_main_app_schedule_pause, divoom_main_app_schedule_destroy, divoom_main_app_schedule_key_handle},
	{DIVOOM_MODEL_ALARM, divoom_alarm_app_schedule_play, divoom_alarm_app_schedule_pause, divoom_alarm_app_schedule_destroy, divoom_alarm_app_schedule_key_handle},
	{DIVOOM_MODEL_SLEEP, divoom_sleep_app_schedule_play, divoom_sleep_app_schedule_pause, divoom_sleep_app_schedule_destroy, divoom_sleep_app_schedule_key_handle},
	{DIVOOM_MODEL_TIME_PLAN, divoom_time_plan_app_schedule_play, divoom_time_plan_app_schedule_pause, 
		divoom_time_plan_app_schedule_destroy, divoom_time_plan_app_schedule_key_handle},
	{DIVOOM_MODEL_CLOCK_PLAY_PLAN, divoom_clock_play_plan_app_schedule_play, divoom_clock_play_plan_app_schedule_pause,
		divoom_clock_play_plan_app_schedule_destroy, divoom_clock_play_plan_app_schedule_key_handle},
	{DIVOOM_MODEL_PRODUCT_TEST, divoom_product_test_app_schedule_play, divoom_product_test_app_schedule_pause, 
		divoom_product_test_app_schedule_destroy, divoom_product_test_app_schedule_key_handle},
	{DIVOOM_MODEL_LIGHT_APP, divoom_light_app_schedule_play, divoom_light_app_schedule_pause, 
		divoom_light_app_schedule_destroy, divoom_light_app_schedule_key_handle},
	{DIVOOM_MODEL_APP_TOOL, divoom_app_tool_schedule_play_callback, divoom_app_tool_schedule_pause_callback, 
		divoom_app_tool_schedule_destroy_callback, divoom_app_tool_scdedule_key_callback},
	{DIVOOM_MODEL_APP_TOMATO_TIME, divoom_tomato_time_schedule_play_callback, divoom_tomato_time_schedule_pause_callback, divoom_tomato_time_schedule_destory_callback, divoom_tomato_time_schedule_key_callback},
	{DIVOOM_MODEL_APP_MUSIC_CTRL, divoom_music_ctrl_schedule_play_callback, divoom_music_ctrl_schedule_pause_callback, divoom_music_ctrl_schedule_destory_callback, divoom_music_ctrl_schedule_key_callback},
	{DIVOOM_MODEL_APP_WHITE_NOISE, divoom_white_noise_app_schedule_play, divoom_white_noise_app_schedule_pause, divoom_white_noise_app_schedule_destroy, divoom_white_noise_app_schedule_key_handle},
	{DIVOOM_MODEL_SLEEP_AID, divoom_sleep_aid_ctrl_app_schedule_play, divoom_sleep_aid_ctrl_app_schedule_pause, divoom_sleep_aid_ctrl_app_schedule_destroy, divoom_sleep_aid_ctrl_app_schedule_key_handle},
	{DIVOOM_MODEL_LEAVE_MSG, divoom_leave_msg_ctrl_schedule_play_callback, divoom_leave_msg_ctrl_schedule_pause_callback, divoom_leave_msg_ctrl_schedule_destroy_callback, NULL},
	{DIVOOM_MODEL_OTA, divoom_ota_server_app_schedule_play, divoom_ota_server_app_schedule_pause, divoom_ota_server_app_schedule_destroy, divoom_ota_server_app_schedule_key_handle},
};
    
/*
 * 语音轨（DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE）优先级：数值越大越优先。
 * 闹钟 > 番茄进行中 > 系统睡眠 > 助眠 UI 录音 > 白噪音菜单播放 > 试听/短提示 > ...
 * 与 RGB 表分离，避免仅抢占屏幕却未让出声道。
 */
static const divoom_app_schedule_priority_info resource_voice_divoom_app_schedule_info[] = 
{
	{ DIVOOM_MODEL_ALARM, DIVOOM_ALARM_APP_EVENT_ALARM, 950, {0} },
	{ DIVOOM_MODEL_APP_TOMATO_TIME, DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE, 880, {0} },
	{ DIVOOM_MODEL_SLEEP, DIVOOM_SLEEP_APP_EVENT_WAITING, 850, {0} },
	{ DIVOOM_MODEL_SLEEP_AID, DIVOOM_SLEEP_AID_APP_EVENT_PLAY, 1000, {0} },
	{ DIVOOM_MODEL_SLEEP_AID, DIVOOM_SLEEP_AID_APP_EVENT_RECORD, 834, {0} },
	{ DIVOOM_MODEL_LEAVE_MSG, DIVOOM_LEAVE_MSG_CTRL_LOCAL_RECORD_EVENT, 833, {0} },
	{ DIVOOM_MODEL_APP_WHITE_NOISE, DIVOOM_WHITE_NOISE_SCHEDULE_PLAY_ACTIVE, 820, {0} },
	{ DIVOOM_MODEL_SLEEP, DIVOOM_SLEEP_APP_EVENT_TRY_LISTEN, 480, {0} },
	{ DIVOOM_MODEL_SLEEP_AID, DIVOOM_SLEEP_AID_APP_EVENT_TRY_LISTEN, 500, {0} },
	{ DIVOOM_MODEL_APP_TOMATO_TIME, DIVOOM_TOMATO_SCHEDULE_TOMATO_TRY_VOICE, 480, {0} },
	{ DIVOOM_MODEL_APP_TOOL, DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY, 470, {0} },
	{ DIVOOM_MODEL_ALARM, DIVOOM_ALARM_APP_EVENT_LISTEN, 460, {0} },
	{ DIVOOM_MODEL_NET_RADIO, DIVOOM_NET_RADIO_PLAY, 200, {0} },
	{ DIVOOM_MODEL_NET_RADIO, DIVOOM_NET_RADIO_PLAY_EVENT, 200, {0} },
	{ DIVOOM_MODEL_APP_MUSIC_CTRL, DIVOOM_MUSIC_SCHEDULE_ACTIVE, 100, {0} },
	{ DIVOOM_MODEL_END, 0, 0, {0} }
};

//RGB彩灯所有模块的优先级列表

static const divoom_app_schedule_priority_info resource_rgb_divoom_app_schedule_info[] = 
{
	//APP TOOL
	{
		DIVOOM_MODEL_APP_TOOL, DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY, 0, {0}
	},
	{
		DIVOOM_MODEL_APP_TOOL, DIVOOM_APP_TOOL_EVENT_PLAY,0, {0}
	},

	//按键功能
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_KEY_FUNC_EVENT, 150, {0}
	},

	//系统配置菜单
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_ENTER_SYSTEM_MENU_SETTING, 0, {0}
	},

	//新ＳＤ卡,系统下载图片
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_DOWNLOAD_OK_EVENT, 10000, {0}
	},

	//时间计划
	{
		DIVOOM_MODEL_TIME_PLAN, DIVOOM_TIME_PLAN_APP_EVENT_RUN, 0, {0}
	},

	//播放计划
	{
		DIVOOM_MODEL_CLOCK_PLAY_PLAN, DIVOOM_CLOCK_PLAY_PLAN_APP_EVENT_RUN, 0, {0}
	},

	//wifi的相关提醒
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT, 200, {0}
	},
	//blue的相关提醒
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT, 200, {0}
	},
	//其他通知提醒
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_ALL_NOTICE_EVENT, 0, {0}
	},
	
	//开机显示
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_WIFI_CONFIG_EVENT, 9000, {DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE, 0}
	},
	//低电提醒
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_LOW_POWER_EVENT, 1000, {0}
	},
	//设备重置
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_DEVICE_UPDATE_EVENT, 10000, {0}
	},
	//sd卡插入提醒
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_SD_IN_EVENT, 9000	, {0}
	},
	//sd卡拔出提醒
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_SD_OUT_EVENT, 9001, {0}
	},
	//sd卡格式错误
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_SD_ERR_EVENT, 9002, {0}
	},
	//app发送的图库相关命令
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_NOTICE_MSG_EVENT, 0, {0}
	},
	//app发图
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_LOCAL_DRAW_CTRL_EVENT, 0, {0}
	},
	//app发图
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_PLAY_LIST_EVENT, 0, {0}
	},
	//炫音工厂由 Start/Stop 显式管理生命周期
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_MIXER_EVENT, 0, {DIVOOM_MENU_CONTROL_ID_MIXER, 0}
	},
	//Mixer/SetNew 七种乐器页，与旧 Mixer 页面同级但使用独立菜单和状态
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_MIX_MUSIC2_EVENT, 0, {DIVOOM_MENU_CONTROL_ID_MIX_MUSIC2, 0}
	},
	//app发照片，显示
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_SEND_PHOTO_EVENT, 0, {0}
	},
	//app发照片，显示
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_EVENT, 800, {0}
	},
	//表盘
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT, 0, {DIVOOM_MENU_CONTROL_ID_COMPONENT, DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW,DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, 0}
	},
	//表盘展示
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_DEMO_DIAL_DISP, 0, {0}
	},
	//谷歌日历显示
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_GOOGLE_CALENDAR_DISP, 0, {DIVOOM_MENU_CONTROL_ID_COMPONENT, DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW,DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, 0}
	},
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_OUTLOOK_CALENDAR_DISP, 0, {DIVOOM_MENU_CONTROL_ID_COMPONENT, DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW,DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, 0}
	},
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_CUSTOM_DIAL_DISP, 0, {DIVOOM_MENU_CONTROL_ID_COMPONENT, DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW,DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, 0}
	},
	//表盘定时
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_SCHEDULE_CLOCK_EVENT, 0, {DIVOOM_MENU_CONTROL_SCHEDULE_DIAL_ID, 0}
	},
	//通知事件
	{
		DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_CLOCK_NOTICE_EVENT, 0, {0}
	},
	{
		//时间显示基本上可以被所有的打断
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_DEFAULT_TIME_EVENT, 0, {0}
	},
	{
		DIVOOM_MODEL_NET_RADIO, DIVOOM_NET_RADIO_PLAY, 0, {0}
	},
	//定时开关机
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT, 890, {0}
	},
	//闹钟模块:有闹钟激活与闹钟音乐试听功能
	{
		DIVOOM_MODEL_ALARM, DIVOOM_ALARM_APP_EVENT_ALARM, 900, {0}
	},
	//测试模块，此模块的优先级仅低于升级，而且不会主动释放，
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_POWER_OFF_EVENT, 9000, {0}
	},
	
	//SD卡空间不够，
	{
		DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_SD_FREE_WARNING, 300, {0}
	},
	
	//测试模块，此模块不会主动释放，
	{
		DIVOOM_MODEL_PRODUCT_TEST, DIVOOM_PRODUCT_TEST_START_EVENT, 11000, {0}
	},
	
    /* Resource installation outranks normal UI, below shutdown and both firmware engines. */
    {
        DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE, 8000,
        {DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW, 0}
    },
	//升级模块:
	{
		DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_APP_UPGRADE, 10000, {0}
	},

	//升级模块:
	{
		DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_SD_UPGRADE, 10000, {0}
	},

	// JL 蓝牙芯片 OTA
	{
		DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_JL_UPGRADE, 10000, {0}
	},


#if 0
	//倒计时时间安排界面,提醒五秒
	{
		DIVOOM_MODEL_COUNT_TIME_DOWM, DIVOOM_APP_COUNT_TIME_DOWN_EVENT_PLAY, 600
	},
#endif

	{
		DIVOOM_MODEL_SLEEP , DIVOOM_SLEEP_APP_EVENT_WAITING, 700, {0}
	},

	//番茄时钟
	{
		DIVOOM_MODEL_APP_TOMATO_TIME, DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE, 800, {DIVOOM_MENU_CONTROL_ID_TOMA_EDIT, DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_TIME, 
			DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_REPEAT, DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_BELL, DIVOOM_MENU_CONTROL_ID_POMODORO_MENU}
	},

	//助眠播放：与番茄钟同级，屏蔽 App 切表盘等低优先级 RGB 请求
	{
		DIVOOM_MODEL_SLEEP_AID, DIVOOM_SLEEP_AID_APP_EVENT_PLAY, 800, {DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE, 0}
	},

	//音乐模块
	{
		DIVOOM_MODEL_APP_MUSIC_CTRL, DIVOOM_MUSIC_SCHEDULE_ACTIVE, 0, {0}
	},

	{
		DIVOOM_MODEL_END , DIVOOM_SLEEP_APP_EVENT_WAITING, 700, {0}
	}
};

//白灯所有模块的优先级列表

static const divoom_app_schedule_priority_info resource_white_divoom_app_schedule_info[] = 
{
	{
		DIVOOM_MODEL_END , DIVOOM_SLEEP_APP_EVENT_WAITING, 700, {0}
	}

};
 
void divoom_app_schedule_clear_menu_list(int model_id, int event_id)
{
    int i = 0;

    for(i = 0; i < sizeof(resource_rgb_divoom_app_schedule_info)/sizeof(resource_rgb_divoom_app_schedule_info[0]); i++)
    {
        if(resource_rgb_divoom_app_schedule_info[i].model_id == model_id &&
            resource_rgb_divoom_app_schedule_info[i].event_id == event_id)
        {
            for(int j = 0; j < sizeof(resource_rgb_divoom_app_schedule_info[i].menu_list)/sizeof(resource_rgb_divoom_app_schedule_info[i].menu_list[0]); j++)
            {
                if(resource_rgb_divoom_app_schedule_info[i].menu_list[j] == 0)
                {
                    break;
                }
                divoom_menu_control_remove_menu(resource_rgb_divoom_app_schedule_info[i].menu_list[j]);
            }
            return;
        }
    }
}

Pdivoom_app_schedule_priority_info divoom_app_schedule_get_schedule_list(uint8 resource_type)
{
    switch(resource_type)
    {
        case DIVOOM_APP_SCHEDULE_RESOURCE_VOICE:
            return (Pdivoom_app_schedule_priority_info)resource_voice_divoom_app_schedule_info;
            
        case DIVOOM_APP_SCHEDULE_RESOURCCE_RGB:
            return (Pdivoom_app_schedule_priority_info)resource_rgb_divoom_app_schedule_info;
            
        case DIVOOM_APP_SCHEDULE_RESOURCCE_WHITE:
            return (Pdivoom_app_schedule_priority_info)resource_white_divoom_app_schedule_info;
            
    }

    return NULL;
}

void divoom_app_schedule_print(void)
{
	int i, j;
	Pdivoom_app_schedule_queue_item_info pcur_event;
    char notice_string[DIVOOM_APP_SCHEDULE_RESOURCCE_END][20] = 
    {
        "voice ","RGB16", "White48"
    };
    for(j = 0; j < DIVOOM_APP_SCHEDULE_RESOURCCE_END; j++ )
    {
	divoom_debug("schedule resource type: %s", notice_string[j]);
    	for(i = 0; i < divoom_queue_items(pdivoom_app_schedule_handle->ptask_queue[j]); i++)
    	{
    		pcur_event = divoom_queue_get_index_item(pdivoom_app_schedule_handle->ptask_queue[j], i);
		divoom_debug("queue event: model:%d,event:%d, pro:%d", pcur_event->model_id, pcur_event->event_id, pcur_event->priority);
    	}
    }
}

/**
 * *  @brief       	divoom_app_schedule_init
 事件模块初始化
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   none

 * *  @return        void
 * */

void divoom_app_schedule_init(void)
{
    int i;
    
	if(pdivoom_app_schedule_handle)
	{
		divoom_print("%s:it is error at reinit!\n", __FUNCTION__);
		return;
	}
	pdivoom_app_schedule_handle = (Pdivoom_app_schedule_ctrl)divoom_malloc(sizeof(divoom_app_schedule_ctrl),DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
	memset(pdivoom_app_schedule_handle, 0, sizeof(divoom_app_schedule_ctrl));
    for(i = 0; i < DIVOOM_APP_SCHEDULE_RESOURCCE_END; i ++)
    {
	    pdivoom_app_schedule_handle->ptask_queue[i] = divoom_queue_init(500, __func__, __LINE__);
    }
	pdivoom_app_schedule_handle->event_lock = divoom_system_create_lock();
	if(pdivoom_app_schedule_handle->event_lock == NULL)
	{
		divoom_print("LINE:%d", __LINE__);
	}
}

void divoom_app_schedule_set_suppress_resume(uint8 suppress)
{
	pdivoom_app_schedule_suppress_resume = suppress ? 1 : 0;
}

/**
 * *  @brief       	divoom_app_schedule_pause_event
 向相应模块发送暂停消息
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   model_id  模块名称
 event_id   事件
 user_data 用户数据指针
 resouce_type 资源id,

 * *  @return        1:成功，0:失败，将清除此事件
 * */

uint8 divoom_app_schedule_pause_event(uint8 model_id, uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	uint8 i = 0;
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	uint8 ret;
#endif
	for(i=0;i<sizeof(all_model_info)/sizeof(all_model_info[0]);i++)
	{
		if(model_id == all_model_info[i].model_id)
		{
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
			s_cb_model_id = model_id;
			s_cb_event_id = event_id;
			s_cb_kind = "pause";
			ret = all_model_info[i].pause_callback(event_id, user_data, active_model, active_event,resouce_type);
			s_cb_kind = NULL;
			return ret;
#else
			return all_model_info[i].pause_callback(event_id, user_data, active_model, active_event,resouce_type);
#endif
		}
	}
	return 0;
}


/**
 * *  @brief       	divoom_app_schedule_play_event
 向相应模块发送启动消息
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   model_id  模块名称
 event_id   事件
 user_data 用户数据指针
 resouce_type 资源id,

 * *  @return        1:成功，0:失败，将清除此事件
 * */

uint8 divoom_app_schedule_play_event(uint8 model_id, uint8 event_id, void *user_data, uint32 resouce_type)
{
	uint8 i = 0;
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	uint8 ret;
#endif
	for(i=0;i<sizeof(all_model_info)/sizeof(all_model_info[0]);i++)
	{
		if(model_id == all_model_info[i].model_id)
		{
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
			s_cb_model_id = model_id;
			s_cb_event_id = event_id;
			s_cb_kind = "play";
			ret = all_model_info[i].play_callback(event_id, user_data,resouce_type);
			s_cb_kind = NULL;
			return ret;
#else
			return all_model_info[i].play_callback(event_id, user_data,resouce_type);
#endif
		}
	}
	return 0;
}


/**
 * *  @brief       	divoom_app_schedule_destroy_event
 向相应模块发送清除消息
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   model_id  模块名称
 event_id   事件
 user_data 用户数据指针
 have_resource_type  当前模块已经申请的资源
 resouce_type 正在激活的资源ID,

 * *  @return        1:成功，0:失败，将清除此事件
 * */

uint8 divoom_app_schedule_destroy_event(uint8 model_id, uint8 event_id, void *user_data, uint32 have_resource_type, uint32 resouce_type)
{
	uint8 i = 0;
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	uint8 ret;
#endif
	for(i=0;i<sizeof(all_model_info)/sizeof(all_model_info[0]);i++)
	{
		if(model_id == all_model_info[i].model_id)
		{
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
			s_cb_model_id = model_id;
			s_cb_event_id = event_id;
			s_cb_kind = "destroy";
			ret = all_model_info[i].destroy_callback(event_id, user_data, have_resource_type,resouce_type);
			s_cb_kind = NULL;
			return ret;
#else
			return all_model_info[i].destroy_callback(event_id, user_data, have_resource_type,resouce_type);
#endif
		}
	}
	return 0;
}

//如果某个模块进行处理了,那么返回1,否则返回0
uint8 divoom_app_schedule_key_handle_event(uint8 key_id, uint8 key_type)
{
	uint8 i = 0, j, ret = 0;
	uint8 cur_model[DIVOOM_APP_SCHEDULE_RESOURCCE_END] = {0xFF};
    uint32 resouce_type;
    Pdivoom_app_schedule_queue_item_info atcive_mode;

    //先找出所有激活的模块
    for(j = 0; j < DIVOOM_APP_SCHEDULE_RESOURCCE_END; j++)
    {
        atcive_mode = (Pdivoom_app_schedule_queue_item_info)divoom_queue_get_head_note(pdivoom_app_schedule_handle->ptask_queue[j]);
        if(atcive_mode)
        {
            cur_model[j] = atcive_mode->model_id;
        }
        else
        {
            cur_model[j] = 0xFF;
        }
    }
    
    for(j = 0; j < DIVOOM_APP_SCHEDULE_RESOURCCE_END; j++)
    {
        if(cur_model[j] == 0xFF)
        {
            continue;
        }
        //查找这个模块有多少资源处于激活状态
        resouce_type = (1 << j);
        for(i = j + 1; i < DIVOOM_APP_SCHEDULE_RESOURCCE_END; i++)
        {
            if(cur_model[i] == 0xFF)
            {
                continue;
            }
            if(cur_model[j] != cur_model[i])
            {
                continue;
            }
            resouce_type |= (1 << i);
            cur_model[i] = 0xFF; //已经统计的模块标记一下
        }

        //向有激活资源的模块发送按键信息
        if(resouce_type)
        {
        	for(i=0;i<sizeof(all_model_info)/sizeof(all_model_info[0]);i++)
        	{
        		if(all_model_info[i].model_id == cur_model[j])
        		{
                    int model_ret;
                    if(all_model_info[i].key_callback == NULL)
                    {
                        break;
                    }
        			model_ret = all_model_info[i].key_callback(key_id, key_type,resouce_type);
					if(model_ret)
					{
						return model_ret;
					}
                    ret += model_ret;
                    break;
        		}
        	}
        }
    }
    
	return ret!=0;
}

/**
 * *  @brief       	divoom_app_schedule_get_highest_event
 从队列中取出优先最高的事件
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   none
     uint8 resouce_type   资源类型

 * *  @return        返回事件数据
 * */

Pdivoom_app_schedule_queue_item_info divoom_app_schedule_get_highest_event(uint8 resouce_type)
{
	int i;
	Pdivoom_app_schedule_queue_item_info pcur_event, pfind = NULL;

	if(divoom_queue_not_empty(pdivoom_app_schedule_handle->ptask_queue[resouce_type]) == 0)
	{
		return NULL;
	}

	for(i = 0; i < divoom_queue_items(pdivoom_app_schedule_handle->ptask_queue[resouce_type]); i++)
	{
		pcur_event = (Pdivoom_app_schedule_queue_item_info)divoom_queue_get_index_item(pdivoom_app_schedule_handle->ptask_queue[resouce_type], i);
		if(pfind == NULL)
		{
			pfind = pcur_event;
		}
		else if(pfind->priority <= pcur_event->priority)
		{
			pfind = pcur_event;
		}
	}
	pfind = (Pdivoom_app_schedule_queue_item_info)divoom_queue_pop_item(pdivoom_app_schedule_handle->ptask_queue[resouce_type],pfind);

	return pfind;
}

/**
 * *  @brief       	divoom_app_schedule_start_new_event
 从队列中取出优先最高的事件，并启动
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   none

 * *  @return        NONE
 * */

//默认第一个是最高优先级的
void divoom_app_schedule_start_new_event(uint32 resource_type)
{
    uint8 i, j;
    uint32 active_type = 0;
	divoom_app_schedule_queue_item_info src_item;
	Pdivoom_app_schedule_queue_item_info find;
    Pdivoom_app_schedule_queue_item_info pcur_active_event[DIVOOM_APP_SCHEDULE_RESOURCCE_END];

    for(i = 0; i < DIVOOM_APP_SCHEDULE_RESOURCCE_END; i ++)
    {
        if(resource_type & (1 << i))
        {
            pcur_active_event[i] = divoom_queue_get_head_note(pdivoom_app_schedule_handle->ptask_queue[i]);
        }
        else
        {
            pcur_active_event[i] = NULL;
        }
    }

    for(i = 0; i < DIVOOM_APP_SCHEDULE_RESOURCCE_END; i ++)
    {
        if(pcur_active_event[i] == NULL)
        {
            continue;
        }
        //divoom_print("%s:the mode %d have event!\n", __func__, i);
        //查找是否有一个模块的多个事件同时被激活
        active_type = (1 << i);
        for(j = i + 1; j < DIVOOM_APP_SCHEDULE_RESOURCCE_END; j++)
        {
            if(pcur_active_event[j] == NULL)
            {
                continue;
            }
            if(pcur_active_event[j]->model_id == pcur_active_event[i]->model_id &&
                pcur_active_event[j]->event_id == pcur_active_event[i]->event_id)
            {
                active_type |= (1 << j);
                pcur_active_event[j] = NULL;
            }
        }
    	if(divoom_app_schedule_play_event(pcur_active_event[i]->model_id, pcur_active_event[i]->event_id, pcur_active_event[i]->user_data, active_type) == 0)
    	{
    		divoom_print("%s: the event(%d:%d) can't run and destroy it\n", __FUNCTION__, pcur_active_event[i]->model_id, pcur_active_event[i]->event_id);
    		divoom_app_schedule_destroy_event(pcur_active_event[i]->model_id, pcur_active_event[i]->event_id, pcur_active_event[i]->user_data,active_type,active_type);
            divoom_app_schedule_clear_menu_list(pcur_active_event[i]->model_id, pcur_active_event[i]->event_id);
            //并且将内部的元素全部弹出队列
            for(j = 0; j < DIVOOM_APP_SCHEDULE_RESOURCCE_END; j++)
            {
                if(active_type & (1 << j))
                {
        			src_item.model_id = pcur_active_event[i]->model_id;
        			src_item.event_id = pcur_active_event[i]->event_id;
        			find = divoom_queue_pop_cmp(pdivoom_app_schedule_handle->ptask_queue[i], (void *)&src_item,divoom_app_schedule_compare_event);
        			if(find)
        			{
        				divoom_print("%s, we will remove the  event(%d, %d) reouce_type: %d from queue beacuse of fail to play!\n", __FUNCTION__, src_item.model_id, src_item.event_id, i);
        				divoom_free(find);
        			}
                    
                }
            }
            
    	}
    	else
    	{
		divoom_debug("event(%d:%d) resource:%d will run", pcur_active_event[i]->model_id, pcur_active_event[i]->event_id, active_type);
    	}
        pcur_active_event[i] = NULL;        
    }
}
/*void divoom_app_schedule_start_new_event(void)
{
	pdivoom_app_schedule_handle->pcur_active_event = NULL;
	while(pdivoom_app_schedule_handle->pcur_active_event == NULL)
	{
		if(divoom_queue_not_empty(pdivoom_app_schedule_handle->ptask_queue) == 0)
		{
			break;
		}
		pdivoom_app_schedule_handle->pcur_active_event = divoom_app_schedule_get_highest_event();
		if(pdivoom_app_schedule_handle->pcur_active_event)
		{
			//启动 这个事件,如果启动失败，将取消此事件
			if(divoom_app_schedule_play_event(pdivoom_app_schedule_handle->pcur_active_event->model_id, pdivoom_app_schedule_handle->pcur_active_event->event_id,
						pdivoom_app_schedule_handle->pcur_active_event->user_data) == 0)
			{
				divoom_print("%s: the event(%d:%d) can't run and destroy it\n", __FUNCTION__, pdivoom_app_schedule_handle->pcur_active_event->model_id, pdivoom_app_schedule_handle->pcur_active_event->event_id);
				divoom_app_schedule_destroy_event(pdivoom_app_schedule_handle->pcur_active_event->model_id, pdivoom_app_schedule_handle->pcur_active_event->event_id,
						pdivoom_app_schedule_handle->pcur_active_event->user_data, DIVOOM_APP_SCHEDULE_CANCEL_OK_QUEUE);
				divoom_free(pdivoom_app_schedule_handle->pcur_active_event);
				pdivoom_app_schedule_handle->pcur_active_event = NULL;
			}
			else
			{
				divoom_debug("event(%d:%d) will run", pdivoom_app_schedule_handle->pcur_active_event->model_id, pdivoom_app_schedule_handle->pcur_active_event->event_id);
			}
		}
	}
}*/

/**
 * *  @brief       	divoom_app_schedule_compare_event
 比较两个事件是否是同一个事件
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   item1    事件指针
 item2    事件指针

 * *  @return        1:是相同；0:不相同
 * */

int divoom_app_schedule_compare_event(void *item1, void *item2)
{
	Pdivoom_app_schedule_queue_item_info first, second;

	first = (Pdivoom_app_schedule_queue_item_info)item1;
	second = (Pdivoom_app_schedule_queue_item_info)item2;
	if(first == NULL || second == NULL)
		return 1;

	if((first->model_id == second->model_id) && (first->event_id == second->event_id))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

//queue是按照从高往低排列,放在相同的右边,就是大于当前这个元素
//push规则在divoom_queue_push_cmp
//item1是新数据,item2是老数据
int divoom_app_schedule_push_compare_same_priority_onright(void *item1, void *item2)
{
	if(((Pdivoom_app_schedule_queue_item_info)item1)->priority > ((Pdivoom_app_schedule_queue_item_info)item2)->priority)
	{
		return 1;
	}
	return 0;
}

//queue是按照从高往低排列,放在相同的左边,就是等于当前这个元素
//push规则在divoom_queue_push_cmp
//item1是新数据,item2是老数据
int divoom_app_schedule_push_compare_same_priority_onleft(void *item1, void *item2)
{
	if(((Pdivoom_app_schedule_queue_item_info)item1)->priority >= ((Pdivoom_app_schedule_queue_item_info)item2)->priority)
	{
		return 1;
	}
	return 0;
}


#if 1
/**
 * *  @brief       	divoom_app_schedule_get_active_model
 返回当前激活的模块,如果没有事件，默认返回音乐
 PIXOO-WiFi默认为灯效控制模式
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   NONE

 * *  @return        返回当前事件的运行状态
 * */

uint8 divoom_app_schedule_get_active_model(uint8 *event_id,uint8 res_type)
{
	Pdivoom_app_schedule_queue_item_info pinfo = NULL;
	if(res_type >=DIVOOM_APP_SCHEDULE_RESOURCCE_END)
	{
		return DIVOOM_MODEL_MAIN;
	}
	if((pdivoom_app_schedule_handle == NULL && pdivoom_app_schedule_handle->ptask_queue[res_type] == NULL) || divoom_queue_items(pdivoom_app_schedule_handle->ptask_queue[res_type]) == 0)
	{
		return DIVOOM_MODEL_MAIN;
	}
	pinfo = (Pdivoom_app_schedule_queue_item_info)divoom_queue_get_head_note(pdivoom_app_schedule_handle->ptask_queue[res_type]);
	if(pinfo)
	{
		if(event_id)
		{	
			*event_id = pinfo->event_id;
		}
		return pinfo->model_id;
	}
	return DIVOOM_MODEL_MAIN;
}
#endif

/**
 * *  @brief       	divoom_app_schedule_push
 增加一个事件
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   pevent_info  事件相关的数据

 * *  @return        返回当前事件的运行资源ID,如果为0表示不能运行
 * */

//搬运PIXOOMAX的逻辑过来,不同的是一个是数组,一个是queue
uint32 divoom_app_schedule_push(Pdivoom_app_schedule_item_info pevent_info)
{
	int i = 0, j;
	int priority = 0; 
    uint32 active_resource_type = 0, cur_reousrce_type = 0, destory_resource_type = 0;
	Pdivoom_app_schedule_queue_item_info pnew_event_info, pfind;
	Pdivoom_app_schedule_queue_item_info pcur_active_event, ppause_list[DIVOOM_APP_SCHEDULE_RESOURCCE_END] = {NULL};
    divoom_app_schedule_priority_info *pschedule_priority;

	if (pdivoom_app_schedule_handle == NULL)
		return 0;
    
    divoom_debug("new event: M:%d, E:%d, attr:%d, destroy:%d, resource:%d", pevent_info->model_id, pevent_info->event_id, pevent_info->attr, pevent_info->destroy_flag, pevent_info->resource_type);

#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	divoom_app_schedule_warn_reentry("push", pevent_info->model_id, pevent_info->event_id);
#endif
	divoom_system_get_lock(pdivoom_app_schedule_handle->event_lock);
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	divoom_app_schedule_mark_lock_owned();
#endif
    for(j = 0; j < DIVOOM_APP_SCHEDULE_RESOURCCE_END; j++)
    {
        if(pevent_info->resource_type & (1 << j))
        {   
            pnew_event_info = (Pdivoom_app_schedule_queue_item_info)divoom_malloc(sizeof(divoom_app_schedule_queue_item_info),DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
            if(pnew_event_info == NULL)
            {
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
                divoom_app_schedule_clear_lock_owned();
#endif
                divoom_system_free_lock(pdivoom_app_schedule_handle->event_lock);
                divoom_print("%s:it fails to malloc!\n",__FUNCTION__);
                return 0;
            }
            //先查询队列中是否有之前还没有运行的同类事件，有的话，将取消
            pnew_event_info->model_id = pevent_info->model_id;
            pnew_event_info->event_id = pevent_info->event_id;
            pfind = divoom_queue_pop_cmp(pdivoom_app_schedule_handle->ptask_queue[j], (void *)pnew_event_info, divoom_app_schedule_compare_event);
            //先添加到将要消毁的队列，到后面统一处理
            if(pfind)
            {
                if(pfind->destroy_flag)
                {
                    destory_resource_type |= (1 << j);
                }
                divoom_free(pfind);
            }

            pschedule_priority = divoom_app_schedule_get_schedule_list(j);
            for(priority = 0; pschedule_priority[priority].model_id != DIVOOM_MODEL_END; priority++)
            {
                if(pschedule_priority[priority].model_id == pevent_info->model_id && 
                        pschedule_priority[priority].event_id == pevent_info->event_id)
                {
                    break;
                }
            }
            if(pschedule_priority[priority].model_id == DIVOOM_MODEL_END)
            {
                divoom_free(pnew_event_info);
                divoom_print("%s:it not have this event(m:%d;e:%d;t:%d) in schdule!pls do with\n",__FUNCTION__,pevent_info->model_id,pevent_info->event_id, j);
                continue;
            }
            memset(pnew_event_info, 0, sizeof(divoom_app_schedule_queue_item_info));
            pnew_event_info->cur_model_id = priority;
            priority = pschedule_priority[priority].priority;
            pnew_event_info->model_id = pevent_info->model_id;
            pnew_event_info->event_id = pevent_info->event_id;
            pnew_event_info->destroy_flag = pevent_info->destroy_flag;
            pnew_event_info->priority = priority;
            pnew_event_info->user_data = pevent_info->user_data;
            pcur_active_event = divoom_queue_get_head_note(pdivoom_app_schedule_handle->ptask_queue[j]);
            divoom_debug("new event: %d, %d, %d, %d", pnew_event_info->model_id, pnew_event_info->event_id, pnew_event_info->priority, priority);
            if(pcur_active_event)
            {
                divoom_debug("type:%d current_model:%d new_model:%d", j, pcur_active_event->cur_model_id, pnew_event_info->cur_model_id);
                //divoom_print("pdivoom_app_schedule_handle->pcur_active_event->priority = %d, priority = %d\n", pdivoom_app_schedule_handle->pcur_active_event->priority, priority);
                if(pcur_active_event->priority > priority)
                {
                    //如果优先级比较低，而且是可等待的，将它压到队列中，否则将直接丢弃
                    if(pevent_info->attr == DIVOOM_APP_SCHEDULE_ATTR_WAIT)
                    {
                        //后进先出
                        divoom_queue_push_cmp(pdivoom_app_schedule_handle->ptask_queue[j], pnew_event_info, (void *)divoom_app_schedule_push_compare_same_priority_onleft);
                        divoom_debug("push and wait");
                    }
                    else
                    {
                        divoom_debug("cancel new event");
                        divoom_free(pnew_event_info);
                        pnew_event_info = NULL;
                    }
                }
                else if(pcur_active_event->priority == priority)
                {
                    ppause_list[j] = pcur_active_event;                                    
                    divoom_queue_push_head(pdivoom_app_schedule_handle->ptask_queue[j], pnew_event_info);
                    active_resource_type |= (1 << j);
                    divoom_debug("same priority becomes active, resource:%d", j);
                }
                else
                {
                    ppause_list[j] = pcur_active_event;
                    active_resource_type |= (1 << j);
                    //再把新事件压到队首
                    divoom_queue_push_head(pdivoom_app_schedule_handle->ptask_queue[j], pnew_event_info);
                    divoom_debug("run and push current event");
                }
            }
            else
            {
                
                active_resource_type |= (1 << j);   
                divoom_debug("current event empty, run new event");
                //把新事件压入队列
                divoom_queue_push_head(pdivoom_app_schedule_handle->ptask_queue[j], pnew_event_info);
            }
        }
        else
        {
            //当前请求没有的资源，也要这个模块事件对应的资源也清除了
	        divoom_app_schedule_queue_item_info src_item;
            
        	pcur_active_event = divoom_queue_get_head_note(pdivoom_app_schedule_handle->ptask_queue[j]);
        	if(pcur_active_event != NULL)
        	{
        		//如果当前正在运行的被取消了，将从队列中取一个最高优先级的,并启动它
        		if(pcur_active_event->model_id == pevent_info->model_id && pcur_active_event->event_id == pevent_info->event_id)
        		{
        			pevent_info->user_data = pcur_active_event->user_data;
        			pcur_active_event = divoom_queue_pop(pdivoom_app_schedule_handle->ptask_queue[j]);
        			divoom_free(pcur_active_event);
        	        pcur_active_event = divoom_queue_get_head_note(pdivoom_app_schedule_handle->ptask_queue[j]);
                    if(pcur_active_event)
                    {
                        divoom_app_schedule_play_event(pcur_active_event->model_id, pcur_active_event->event_id, pcur_active_event->user_data, (1 << j));
                    }
        		}
        		else
        		{
                    void *find;
                    
        			src_item.model_id = pevent_info->model_id;
        			src_item.event_id = pevent_info->event_id;
        			find = divoom_queue_pop_cmp(pdivoom_app_schedule_handle->ptask_queue[j], (void *)&src_item,divoom_app_schedule_compare_event);
        			if(find)
        			{
        				divoom_free(find);
        			}
        		}
        	}
        }
    }
    
    //先处理destory的，再处理pause,再发play命令
    if(destory_resource_type)
    {
        //只有自己设置了destroy标记才会走这个分支
        divoom_app_schedule_destroy_event(pevent_info->model_id, pevent_info->event_id,
            pevent_info->user_data, cur_reousrce_type, destory_resource_type);
    }
    

    //处理暂停任务
    cur_reousrce_type = 0;
    for(i = 0; i < DIVOOM_APP_SCHEDULE_RESOURCCE_END; i ++)
    {
        if(ppause_list[i] == NULL)
        {
            continue;
        }
        cur_reousrce_type = (1 << i);
        //汇总同一模块还是否有其它属性,统一发pause命令
        for(j = i + 1; j < DIVOOM_APP_SCHEDULE_RESOURCCE_END; j++)
        {
            if(ppause_list[j] == NULL)
            {
                continue;
            }
            if(ppause_list[j]->model_id == ppause_list[i]->model_id &&
                ppause_list[j]->event_id == ppause_list[i]->event_id)
            {
                cur_reousrce_type |= (1 << j);
                ppause_list[j] = NULL;
            }
        }
        //发送pause命令ppause_list[i], cur_reousrce_type
        if(0 == divoom_app_schedule_pause_event(ppause_list[i]->model_id, ppause_list[i]->event_id,
            ppause_list[i]->user_data, pevent_info->model_id, pevent_info->event_id,cur_reousrce_type))
        {
			divoom_app_schedule_queue_item_info info = {0};
			info.event_id = ppause_list[i]->event_id;
			info.model_id = ppause_list[i]->model_id;
            //如果暂停失败，将先发destory,再做资源清理动作
            divoom_app_schedule_destroy_event(ppause_list[i]->model_id, ppause_list[i]->event_id,
                ppause_list[i]->user_data, cur_reousrce_type, cur_reousrce_type);
            divoom_app_schedule_clear_menu_list(ppause_list[i]->model_id, ppause_list[i]->event_id);
            for(j = 0; j < DIVOOM_APP_SCHEDULE_RESOURCCE_END; j++)
            {
                if(cur_reousrce_type & (1 << j))
                {
                    //将清除队列中的资源
                    void * ptr = divoom_queue_pop_cmp(pdivoom_app_schedule_handle->ptask_queue[j], (void *)&info, divoom_app_schedule_compare_event);
                    divoom_free(ptr);
                }
            }
            
        }
        ppause_list[i] = NULL;        
    }

	divoom_app_schedule_print();
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	divoom_app_schedule_clear_lock_owned();
#endif
	divoom_system_free_lock(pdivoom_app_schedule_handle->event_lock);
    
    divoom_debug("run with resource %d", active_resource_type);

	return active_resource_type;
}

#if 0
//给出一个基础事件的信息
Pdivoom_app_schedule_queue_item_info divoom_schdule_get_default_item(void)
{
	uint8 i = 0;
	Pdivoom_app_schedule_queue_item_info base_item;
	base_item = divoom_malloc(sizeof(divoom_app_schedule_queue_item_info), DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
	base_item->destroy_flag = 0;
	base_item->event_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
	base_item->model_id = DIVOOM_MODEL_MAIN;
	for(i= 0; i< sizeof(all_divoom_app_schedule_info)/sizeof(divoom_app_schedule_priority_info); i++)
	{
		if(base_item->event_id == all_divoom_app_schedule_info[i].event_id &&
				base_item->model_id == all_divoom_app_schedule_info[i].model_id) 
		{
			base_item->priority = all_divoom_app_schedule_info[i].priority;
			base_item->cur_model_id = i;
			return base_item;
		}
	}
	divoom_free(base_item);
	return NULL;
}
#endif


/**
 * *  @brief       	divoom_app_schedule_cancel
 取消某个事件
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   pevent_info  事件相关信息

 * *  @return        TRUE:是成功；FALSE:不成功
 * */

uint8 divoom_app_schedule_cancel(Pdivoom_app_schedule_item_info pevent_info)
{
	uint8 ret = DIVOOM_APP_SCHEDULE_CANCEL_FAIL, i;
	divoom_app_schedule_queue_item_info src_item;
	Pdivoom_app_schedule_queue_item_info find;
	Pdivoom_app_schedule_queue_item_info pcur_active_event;
    uint32 resource_type = 0, have_resource_type = 0;

	if (pdivoom_app_schedule_handle == NULL)
		return DIVOOM_APP_SCHEDULE_CANCEL_FAIL;

#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	divoom_app_schedule_warn_reentry("cancel", pevent_info->model_id, pevent_info->event_id);
#endif
	divoom_system_get_lock(pdivoom_app_schedule_handle->event_lock);
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	divoom_app_schedule_mark_lock_owned();
#endif
	divoom_print("%s, we will cancel event(%d, %d)!\n", __FUNCTION__, pevent_info->model_id, pevent_info->event_id);

    //先统计所有的被取消与开启的模块，再统一发送消息，避免同一次启动多次发送的问题
    for(i = 0; i < DIVOOM_APP_SCHEDULE_RESOURCCE_END; i ++)
    {
        if(0 == (pevent_info->resource_type & (1 << i)))
        {
            continue;
        }
    	pcur_active_event = divoom_queue_get_head_note(pdivoom_app_schedule_handle->ptask_queue[i]);
    	if(pcur_active_event != NULL)
    	{
    		//如果当前正在运行的被取消了，将从队列中取一个最高优先级的,并启动它
    		if(pcur_active_event->model_id == pevent_info->model_id && pcur_active_event->event_id == pevent_info->event_id)
    		{
                divoom_print("%s, we will cacnel active  event(%d, %d) reouce_type: %d !\n", __FUNCTION__, pevent_info->model_id, pevent_info->event_id, i);
    			ret = DIVOOM_APP_SCHEDULE_CANCEL_OK_RUN;
    			pevent_info->user_data = pcur_active_event->user_data;
    			pcur_active_event = divoom_queue_pop(pdivoom_app_schedule_handle->ptask_queue[i]);
    			divoom_free(pcur_active_event);
                resource_type |= (1 << i);
                have_resource_type |= (1 << i);
    		}
    		else
    		{
    			src_item.model_id = pevent_info->model_id;
    			src_item.event_id = pevent_info->event_id;
    			find = divoom_queue_pop_cmp(pdivoom_app_schedule_handle->ptask_queue[i], (void *)&src_item,divoom_app_schedule_compare_event);
    			if(find)
    			{
    				divoom_print("%s, we will del the  event(%d, %d) reouce_type: %d from queue!\n", __FUNCTION__, pevent_info->model_id, pevent_info->event_id, i);
    				ret = DIVOOM_APP_SCHEDULE_CANCEL_OK_QUEUE;
    				pevent_info->user_data = find->user_data;
    				divoom_free(find);
                    have_resource_type |= (1 << i);
    			}
    			else
    			{
    				divoom_print("%s, we can't find the  event(%d, %d), reouce_type: %d!\n", __FUNCTION__, pevent_info->model_id, pevent_info->event_id, i);
    			}
    		}
    	}
    }

    //如果有被激活的取消，将启动新的资源
    if(have_resource_type != 0)
    {
        divoom_app_schedule_destroy_event(pevent_info->model_id, pevent_info->event_id, pevent_info->user_data, have_resource_type, resource_type);
        divoom_app_schedule_clear_menu_list(pevent_info->model_id, pevent_info->event_id);
    }
    
    if(resource_type)
    {
        if (pdivoom_app_schedule_suppress_resume)
        {
            pdivoom_app_schedule_suppress_resume = 0;
            divoom_print("%s: suppress resume after cancel (%d, %d)\n",
                __FUNCTION__, pevent_info->model_id, pevent_info->event_id);
        }
        else
        {
            divoom_app_schedule_start_new_event(resource_type);
        }
    }
	divoom_app_schedule_print();
#if DIVOOM_APP_SCHEDULE_REENTRY_GUARD
	divoom_app_schedule_clear_lock_owned();
#endif
	divoom_system_free_lock(pdivoom_app_schedule_handle->event_lock);

	return ret;
}


#if 0
//任务执行时间由上层任务自己控制

/**
 * *  @brief       	divoom_app_schedule_main
 事件主循环，主要为超时任务做检查
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   none

 * *  @return        none
 * */


void divoom_app_schedule_main(void *arg)
{
	Pdivoom_app_schedule_queue_item_info pcur_active_event;
	while(1)
	{
		if(pdivoom_app_schedule_handle == NULL)
		{
			return;
		}

		divoom_system_get_lock(pdivoom_app_schedule_handle->event_lock);
		pcur_active_event = divoom_queue_get_head_note(pdivoom_app_schedule_handle->ptask_queue);
		if(pcur_active_event)
		{
			//如果是定时任务，到时间将自动取消
			if(pcur_active_event->endtime && pcur_active_event->endtime < (uint32)time(0))
			{
				divoom_print("%s: we will cancel ths event(%d:%d) beacuse timeout!\n", __FUNCTION__, pcur_active_event->model_id, pcur_active_event->event_id);
				divoom_app_schedule_destroy_event(pcur_active_event->model_id, pcur_active_event->event_id, pcur_active_event->user_data, DIVOOM_APP_SCHEDULE_CANCEL_OK_RUN);
				pcur_active_event = divoom_queue_pop(pdivoom_app_schedule_handle->ptask_queue);
				//先取出来这个数据,然后释放,再从新取一个数据,但不弹出,然后启动新的事件
				divoom_free(pcur_active_event);
				divoom_app_schedule_start_new_event();
				divoom_app_schedule_print();
			}
		}
		divoom_system_free_lock(pdivoom_app_schedule_handle->event_lock);
		divoom_port_delay_ms((1) * 1000U);
	}

	return;
}
#endif
