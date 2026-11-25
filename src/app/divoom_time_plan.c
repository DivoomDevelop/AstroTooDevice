#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_lvgl_base_dial_menu.h"
#include "divoom_astrootoo_theme.h"

#define DIVOOM_TIME_PLAN_TAG					"DIVOOM_TIME_PLAN"
#define DIVOOM_TIME_PLAN_PRINT(format, ...)		divoom_print(format, ##__VA_ARGS__)

enum
{
    DIVOOM_TIME_PLAN_CTRL_WAIT,
    DIVOOM_TIME_PLAN_CTRL_ACTIVE,
    DIVOOM_TIME_PLAN_CTRL_CANCEL,
};

#define DIVOOM_TIME_PLAY_BUZZER_TIME  3

typedef struct _divoom_time_plan_ctrl
{
	//强制下一次一定刷新一次，因为刚好这个时间有个时间安排已经开始了
	uint8 time_init_flag;
	uint8 refresh_flag;
    uint8 play_info;
    uint8  plan_switch; //开关
    uint32 plan_num;
    uint32 plan_id;
    uint32 active_plan_num;
    uint32 active_plan_index;
    uint32 buzzer_start_time;       //蜂鸣器开始时间
    uint32 update_time;
    int srv_update_time;
    uint32 resource_type;    
    divoom_lvgl_dial_ctr_info dial_info;
	//pthread_mutex_t safe_lock;	//管理的线程锁
	void	*safe_lock;	//管理的线程锁
	//char  tts_info[256];
    Pdivoom_time_plan_item_info plan_info;
}divoom_time_plan_ctrl, *Pdivoom_time_plan_ctrl;

static Pdivoom_time_plan_ctrl pdivoom_time_plan_handle = NULL;
void divoom_time_plan_play_over_callback(uint8 status);
void divoom_time_plan_save(void);
void divoom_time_plan_play_end_over_callback(uint8 status);
void divoom_time_plan_cancel_active(uint8 index);
//void divoom_time_plan_play_tts_over_callback(uint8 status);

int divoom_time_plan_get_plan_num(void)
{
	if(pdivoom_time_plan_handle == NULL)
    {
        return 0;
    }

	return pdivoom_time_plan_handle->plan_num;
}

char *divoom_time_plan_get_plan_name(int index)
{
	if(pdivoom_time_plan_handle == NULL || pdivoom_time_plan_handle->plan_num == 0 || index >= pdivoom_time_plan_handle->plan_num)
	{
		return NULL;
	}
	
	return pdivoom_time_plan_handle->plan_info[index].plan_name;
}

void divoom_time_plan_clear_all(void)
{    
    divoom_free(pdivoom_time_plan_handle->plan_info);
    pdivoom_time_plan_handle->plan_num = 0;
    pdivoom_time_plan_handle->plan_id = T_U32_MAX;
    pdivoom_time_plan_handle->active_plan_num = 0;
    pdivoom_time_plan_handle->plan_info = NULL;
    divoom_time_plan_save();
}


//这个是就直接保存的,不管了
void divoom_time_plan_save_local(void)
{
    char *json_info;
    int  cyc_flag[7];

    if(pdivoom_time_plan_handle == NULL)
    {
        return;
    }

    divoom_cJSON *root, *list, *item;

	root = divoom_cJSON_CreateObject();

    if(root == NULL)
    {
        return;
    }
    
    divoom_cJSON_AddNumberToObject(root , "plan_switch", pdivoom_time_plan_handle->plan_switch);
    divoom_cJSON_AddNumberToObject(root , "update_time", pdivoom_time_plan_handle->update_time);
	divoom_cJSON_AddItemToObject(root, "PlanList", list = divoom_cJSON_CreateArray());
    for(int i = 0; i < pdivoom_time_plan_handle->plan_num; i++)
    {
        for(int j = 0; j < 7; j++)
        {
            cyc_flag[j] = pdivoom_time_plan_handle->plan_info[i].cyc_flag[j];
        }
		divoom_cJSON_AddItemToArray(list, item = divoom_cJSON_CreateObject());   
        
        divoom_cJSON_AddNumberToObject(item , "status", pdivoom_time_plan_handle->plan_info[i].status); 
        divoom_cJSON_AddNumberToObject(item , "enable", pdivoom_time_plan_handle->plan_info[i].enable); 
        divoom_cJSON_AddNumberToObject(item , "cyc_attr", pdivoom_time_plan_handle->plan_info[i].cyc_attr); 
        divoom_cJSON_AddNumberToObject(item , "disp_mode", pdivoom_time_plan_handle->plan_info[i].disp_mode); 
        divoom_cJSON_AddNumberToObject(item , "start_time", pdivoom_time_plan_handle->plan_info[i].start_time); 
        divoom_cJSON_AddNumberToObject(item , "end_time", pdivoom_time_plan_handle->plan_info[i].end_time); 
        divoom_cJSON_AddNumberToObject(item , "clock_id", pdivoom_time_plan_handle->plan_info[i].clock_id); 
        divoom_cJSON_AddNumberToObject(item , "voice_switch", pdivoom_time_plan_handle->plan_info[i].voice_switch); 
        divoom_cJSON_AddStringToObject(item , "plan_name", pdivoom_time_plan_handle->plan_info[i].plan_name); 
        divoom_cJSON_AddStringToObject(item , "file_id", pdivoom_time_plan_handle->plan_info[i].file_id); 
        divoom_cJSON_AddItemToObject(item , "cyc_flag", divoom_cJSON_CreateIntArray(cyc_flag, 7));
        
    }
    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_TIME_PLAN_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
    divoom_system_free(json_info);

}


//这个是就直接保存的,不管了
void divoom_time_plan_save(void)
{
    divoom_time_plan_save_local();
}

void divoom_time_plan_format_info(void);

void divoom_time_plan_load(void)
{
    char *file_buf;
    divoom_cJSON *root, *list, *item, *cyc_list, *cyc_item;

    if(pdivoom_time_plan_handle == NULL)
    {
        return;
    }

    file_buf = divoom_app_com_read_file(DIVOOM_TIME_PLAN_SAVE_PATH, NULL);
    if(file_buf == NULL)
    {
        return ;
    }
    root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);
    if(root == NULL)
    {
        return  ;
    }

    pdivoom_time_plan_handle->plan_switch = divoom_cJSON_GetObjectInt(root, "plan_switch");
    pdivoom_time_plan_handle->update_time = divoom_cJSON_GetObjectInt(root, "update_time");
    
    list = divoom_cJSON_GetObjectItem(root, "PlanList");
    pdivoom_time_plan_handle->plan_num = divoom_cJSON_GetArraySize(list);
    if(pdivoom_time_plan_handle->plan_num <= 0)
    {
        pdivoom_time_plan_handle->plan_num = 0;
        divoom_cJSON_Delete(root);
        return;
    }
    pdivoom_time_plan_handle->plan_info = divoom_system_malloc(sizeof(divoom_time_plan_item_info) * pdivoom_time_plan_handle->plan_num);
    if(pdivoom_time_plan_handle->plan_info == NULL)
    {
        pdivoom_time_plan_handle->plan_num = 0;
        divoom_cJSON_Delete(root);
        return;
    }

    for(int i = 0; i < pdivoom_time_plan_handle->plan_num; i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        pdivoom_time_plan_handle->plan_info[i].status = divoom_cJSON_GetObjectInt(item, "status");    
        pdivoom_time_plan_handle->plan_info[i].enable = divoom_cJSON_GetObjectInt(item, "enable");    
        pdivoom_time_plan_handle->plan_info[i].cyc_attr = divoom_cJSON_GetObjectInt(item, "cyc_attr");    
        pdivoom_time_plan_handle->plan_info[i].disp_mode = divoom_cJSON_GetObjectInt(item, "disp_mode");    
        pdivoom_time_plan_handle->plan_info[i].start_time = divoom_cJSON_GetObjectInt(item, "start_time");    
        pdivoom_time_plan_handle->plan_info[i].end_time = divoom_cJSON_GetObjectInt(item, "end_time");    
        pdivoom_time_plan_handle->plan_info[i].clock_id = divoom_cJSON_GetObjectInt(item, "clock_id");    
        pdivoom_time_plan_handle->plan_info[i].voice_switch = divoom_cJSON_GetObjectInt(item, "voice_switch");    
        divoom_cJSON_GetObjectString(item, "plan_name", pdivoom_time_plan_handle->plan_info[i].plan_name);    
        divoom_cJSON_GetObjectString(item, "file_id", pdivoom_time_plan_handle->plan_info[i].file_id);  
        cyc_list = divoom_cJSON_GetObjectItem(item, "cyc_flag");
        for(int j = 0; j < 7; j++)
        {
            cyc_item = divoom_cJSON_GetArrayItem(cyc_list, j);
            if(cyc_item)
            {
                pdivoom_time_plan_handle->plan_info[i].cyc_flag[j] = cyc_item->valueint;
            }
        }
        
    }
    divoom_cJSON_Delete(root);
    
    divoom_time_plan_format_info();
}

void divoom_time_plan_format_info(void)
{    
    uint32 i, j, cur_time , item_time;//, create_second;//, create_time;
	divoom_time_plan_item_info temp_info;
	//uint32 create_zero_sec = 0;
	uint32 end_utc_sec = 0;

	struct tm create_tm;
	struct tm cur_tm;
    struct timeval sys_tick;
    divoom_manage_image_item_info image_item;

    if(pdivoom_time_plan_handle->plan_switch == 0)
    {
        return;
    }

    gettimeofday(&sys_tick, NULL);
    cur_time = sys_tick.tv_sec;
    //cur_second = cur_time % (24*3600);
	localtime_r((time_t *)&cur_time, &cur_tm);
	//得到当天0点到现在的总秒数
    //create_second = pdivoom_time_plan_handle->create_time % (24*3600);
	//得到当天0点的时间
    //create_zero_sec = pdivoom_time_plan_handle->update_time % (24*3600);
    //create_time = cur_second -  create_second;  //取当天0点的时间

    pdivoom_time_plan_handle->active_plan_num = 0;

	localtime_r((time_t *)&pdivoom_time_plan_handle->update_time, &create_tm);

    for(i = 0; i < pdivoom_time_plan_handle->plan_num; i ++)
	{
		pdivoom_time_plan_handle->plan_info[i].status = DIVOOM_TIME_PLAN_CTRL_WAIT;
        if(pdivoom_time_plan_handle->plan_info[i].voice_switch != 1)
        {
            pdivoom_time_plan_handle->plan_info[i].voice_switch = 0;
        }
		if(pdivoom_time_plan_handle->plan_info[i].cyc_attr == 0)
		{
			//得到当前开始时间的utc时间
			//end_utc_sec = pdivoom_time_plan_handle->plan_info[i].end_time + create_zero_sec;
			create_tm.tm_hour = pdivoom_time_plan_handle->plan_info[i].end_time/3600;
			create_tm.tm_min = pdivoom_time_plan_handle->plan_info[i].end_time%3600/60;
			create_tm.tm_sec = 0;
			end_utc_sec = mktime(&create_tm);
			//DIVOOM_TIME_PLAN_PRINT("end_utc_sec = %d\n", end_utc_sec);
			//得到这个item的utc时间，结束时间超过创建时间，就需要+24*3600，作为明天响的item
			//如果结束时间还不到，那就不+
			if(end_utc_sec > pdivoom_time_plan_handle->update_time)
			{
				//item_time = pdivoom_time_plan_handle->plan_info[i].end_time;
				item_time = end_utc_sec;
			}
			else
			{
				//应该是第二天的时间
				//item_time = create_time + pdivoom_time_plan_handle->plan_info[i].end_time + 24 * 3600;
				item_time = end_utc_sec + 24 * 3600;
			}

			//DIVOOM_TIME_PLAN_PRINT("item_time = %d\n", item_time);

			//用最终得到的结束时间和当前的utc时间比较，小于当前的utc时间，就证明至少离现在超过了24小时，就不响了
			if(item_time < cur_time)
			{
				//比当前的utc时间还少，就不要了
				pdivoom_time_plan_handle->plan_info[i].enable = 0;
			}
			else
			{
				pdivoom_time_plan_handle->plan_info[i].enable = 1;
				pdivoom_time_plan_handle->active_plan_num ++;
			}
		}
		else
		{
			pdivoom_time_plan_handle->plan_info[i].enable = 1;
			pdivoom_time_plan_handle->active_plan_num ++;
		}

		if(pdivoom_time_plan_handle->plan_info[i].enable && pdivoom_time_plan_handle->plan_info[i].disp_mode == 1)
		{
            //将加载图片
            image_item.flag = 1;
            image_item.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_PLAN_PIC + i;
            image_item.pcallback = divoom_app_com_finishcallback;
            strcpy(image_item.file_addr, pdivoom_time_plan_handle->plan_info[i].file_id);
            divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_TIME_PLAN_GIF, &image_item, 1);
		}

		//DIVOOM_TIME_PLAN_PRINT("pdivoom_time_plan_handle->plan_info[%d].enable = %d\n", i, pdivoom_time_plan_handle->plan_info[i].enable);
	}

	for(i=0; i<pdivoom_time_plan_handle->plan_num-1; i++)
	{
		for(j=i+1;j<pdivoom_time_plan_handle->plan_num;j++)
		{
			if(pdivoom_time_plan_handle->plan_info[i].start_time > pdivoom_time_plan_handle->plan_info[j].start_time)
			{
				memcpy(&temp_info, &pdivoom_time_plan_handle->plan_info[i], sizeof(divoom_time_plan_item_info));
				memcpy(&pdivoom_time_plan_handle->plan_info[i], &pdivoom_time_plan_handle->plan_info[j], sizeof(divoom_time_plan_item_info));
				memcpy(&pdivoom_time_plan_handle->plan_info[j], &temp_info, sizeof(divoom_time_plan_item_info));
			}
		}
	}

    if(pdivoom_time_plan_handle->active_plan_num == 0 && pdivoom_time_plan_handle->plan_num)
    {
        pdivoom_time_plan_handle->plan_switch = 0;
    }
}

//看一下当前有没有在运行的，有的话就取消运行，只在每次更新的时候检查
void divoom_time_plan_check_plan_cancle(void)
{
	uint8 i = 0;
	if(pdivoom_time_plan_handle == NULL)
		return;
	for(i=0;i<pdivoom_time_plan_handle->plan_num;i++)
	{
		//判断是否有正在运行的
		if(pdivoom_time_plan_handle->plan_info[i].status == DIVOOM_TIME_PLAN_CTRL_ACTIVE)
		{
			//divoom_time_plan_cancel_active(0xFF);
			divoom_time_plan_cancel_active(i);
			break;
		}
	}
}


void divoom_time_plan_update(Pdivoom_system_tool_json_get_plan_info plan_info)
{
    int i;
    //struct timeval sys_tick;

    if(plan_info == NULL)
    {
        return;
    }
    if(plan_info->update_time && pdivoom_time_plan_handle->update_time != T_U32_MAX && pdivoom_time_plan_handle->update_time >= plan_info->update_time)
    {       
        divoom_print("%s: it is the old item and pass it: %d, %d !\n", __FUNCTION__, pdivoom_time_plan_handle->update_time, plan_info->update_time);
        return;
    }
    
    divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_TIME_PLAN_GIF, 1);
    divoom_system_get_lock(pdivoom_time_plan_handle->safe_lock);
    if(plan_info->update_time)
    {
        pdivoom_time_plan_handle->update_time = plan_info->update_time;
    }
    if(pdivoom_time_plan_handle->plan_info)
    {
		//如果当前有正在运行的时间安排，先取消
		divoom_time_plan_check_plan_cancle();
        divoom_free(pdivoom_time_plan_handle->plan_info);
        pdivoom_time_plan_handle->plan_info = NULL;
        pdivoom_time_plan_handle->plan_num = 0;
        pdivoom_time_plan_handle->active_plan_num = 0;
    }
    if(plan_info->enable == 0)
    {
        pdivoom_time_plan_handle->plan_switch = 0;
        divoom_system_free_lock(pdivoom_time_plan_handle->safe_lock);
        divoom_time_plan_save();
        return;
    }

    pdivoom_time_plan_handle->plan_switch = plan_info->enable;
    pdivoom_time_plan_handle->plan_num = plan_info->plan_item_num;
    pdivoom_time_plan_handle->update_time = plan_info->update_time;

    
    if(plan_info->plan_item_num)
    {
        pdivoom_time_plan_handle->plan_info = (Pdivoom_time_plan_item_info)divoom_malloc(pdivoom_time_plan_handle->plan_num * sizeof(divoom_time_plan_item_info),DIVOOM_MODEL_TIME_PLAN,__LINE__, __FILE__);
		
        for(i = 0; i < plan_info->plan_item_num; i ++)
        {
            divoom_print("plan id: %d, start: %d, end: %d, mode: %d\n", i, plan_info->plan_item_list[i].start_time, plan_info->plan_item_list[i].end_time, plan_info->plan_item_list[i].disp_mode);
            pdivoom_time_plan_handle->plan_info[i].status = DIVOOM_TIME_PLAN_CTRL_WAIT;  //这个计划处于的状态
            pdivoom_time_plan_handle->plan_info[i].enable = 1;  //这个字项是关闭状态，还是开启状态
            pdivoom_time_plan_handle->plan_info[i].cyc_attr = plan_info->plan_item_list[i].cyc_attr;  //如果为0表示非循环，为其它为循环星期
            pdivoom_time_plan_handle->plan_info[i].start_time = plan_info->plan_item_list[i].start_time;
            pdivoom_time_plan_handle->plan_info[i].end_time = plan_info->plan_item_list[i].end_time;
            pdivoom_time_plan_handle->plan_info[i].disp_mode = plan_info->plan_item_list[i].disp_mode;
            pdivoom_time_plan_handle->plan_info[i].voice_switch = plan_info->plan_item_list[i].voice_switch;
            memcpy(pdivoom_time_plan_handle->plan_info[i].cyc_flag,plan_info->plan_item_list[i].cyc_flag, 7);
            pdivoom_time_plan_handle->plan_info[i].clock_id = plan_info->plan_item_list[i].clock_id;
            strcpy(pdivoom_time_plan_handle->plan_info[i].file_id, plan_info->plan_item_list[i].file_id);
            strcpy(pdivoom_time_plan_handle->plan_info[i].plan_name, plan_info->plan_item_list[i].plan_name);
        }
    }
    divoom_time_plan_format_info();
    divoom_system_free_lock(pdivoom_time_plan_handle->safe_lock);
    divoom_time_plan_save();
}



uint8 divoom_time_plan_event(Pdivoom_sys_msg_info msg)
{
    if(pdivoom_time_plan_handle == NULL)
    {
        return false;
    }

	int update_time = 0;
	
    divoom_print("%s: event id: %d\n", __func__,msg->msg_id);
    switch(msg->msg_id)
    {    
        case DIVOOM_DEVICE_SYSTEM_RESET_ALL:
            //先删除开机动画，再向系统获取相关配置
            divoom_app_com_del_file(DIVOOM_TIME_PLAN_SAVE_PATH);
            divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_TIME_PLAN_GIF, 0);
            break;
            
        case DIVOOM_TIME_PLAN_OVER:
        {
            divoom_app_schedule_item_info event_info = {0};
            
            event_info.model_id = DIVOOM_MODEL_TIME_PLAN;
            event_info.event_id = DIVOOM_TIME_PLAN_APP_EVENT_RUN;
            event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
            event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&event_info);
            break;
        }
        
		case DIVOOM_DEVICE_CON_SRV_ID:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_TIME_PLAN, DIVOOM_SERVER_TOPIC, (void *)&update_time);
            break;
        
        case DIVOOM_TIME_PLAN_UPDATE_TIME:
            pdivoom_time_plan_handle->srv_update_time = *(int *)msg->data;
            break;
            
        case DIVOOM_TIME_PLAN_INIT:
            divoom_print("aj test!\n");
            divoom_time_plan_update((Pdivoom_system_tool_json_get_plan_info)msg->data);
            divoom_print("aj test!\n");
            break;
            
        case DIVOOM_APP_KEY_EVENT_ID:
            break;
            
            
    }
    return true;
}

/*********************************************************************
  Function:        divoom_time_plan_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_time_plan_get_url_addr(int model_id, int image_id, char *url_addr)
{
    image_id -= DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_PLAN_PIC;
    if(image_id < pdivoom_time_plan_handle->plan_num)
    {
        strcpy(url_addr, pdivoom_time_plan_handle->plan_info[image_id].file_id);
    }
    return 1;
}

void divoom_time_plan_get_active_plan_time(char *disp_str)
{
    if(pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
    {
        Pdivoom_time_plan_item_info pcur_info = &pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index];

		if(divoom_main_get_time_flag())
        {
            sprintf(disp_str, "%02d:%02d-%02d:%02d", pcur_info->start_time/3600, pcur_info->start_time/60%60, pcur_info->end_time/3600, pcur_info->end_time/60%60);
        }
        else
        {
            int start_hour, save_start_hour, end_hour, save_end_hour;
            char temp_char[24];
			
            start_hour = pcur_info->start_time/3600;
            save_start_hour = pcur_info->start_time/3600;
            
            if(start_hour != 12)
            {
                start_hour = start_hour % 12;
                if(start_hour == 0)
                {
                   start_hour = 12;
                }
            }

			
			end_hour = pcur_info->end_time/3600;
			save_end_hour = pcur_info->end_time/3600;
						
			if(end_hour != 12)
			{
				end_hour = end_hour % 12;
				if(end_hour == 0)
				{
				   end_hour = 12;
				}
			}
            
            if((save_start_hour >= 12) && (save_end_hour >= 12))
            {
                sprintf(disp_str, "%d:%02dpm-%d:%02dpm", start_hour, pcur_info->start_time/60%60, end_hour, pcur_info->end_time/60%60);
            }
            else if((save_start_hour >= 12) && (save_end_hour < 12))
            {
                sprintf(disp_str, "%d:%02dpm-%d:%02dam", start_hour, pcur_info->start_time/60%60, end_hour, pcur_info->end_time/60%60);
            }
			else if((save_start_hour < 12) && (save_end_hour >= 12))
            {
                sprintf(disp_str, "%d:%02dam-%d:%02dpm", start_hour, pcur_info->start_time/60%60, end_hour, pcur_info->end_time/60%60);
            }
			else if((save_start_hour < 12) && (save_end_hour < 12))
            {
                sprintf(disp_str, "%d:%02dam-%d:%02dam", start_hour, pcur_info->start_time/60%60, end_hour, pcur_info->end_time/60%60);
            }
			else
			{
				sprintf(disp_str, " ");
			}

        }
    }
    else
    {
		strcpy(disp_str, " ");
    }
}


/* 480 卡片：宽同其它 notice；多条由 system_msg_notice 纵向叠放（与 T113 一致，不共用一张卡） */
#define DIVOOM_TIME_PLAN_POPUP_W          DIVOOM_ASTRO_TOP_TOAST_W
#define DIVOOM_TIME_PLAN_POPUP_H         96
#define DIVOOM_TIME_PLAN_POPUP_NAME_H    28
#define DIVOOM_TIME_PLAN_POPUP_TIME_H    22
#define DIVOOM_TIME_PLAN_POPUP_ROW_GAP   4

int divoom_time_plan_lvgl_message_box_notice(void *para, int result)
{
	(void)para;
	(void)result;
	divoom_time_plan_play_end_over_callback(1);
	return 0;
}

static void divoom_time_plan_popup_apply_card_style(lv_obj_t *root)
{
	static const lv_style_selector_t idle_sel[] = {
		LV_PART_MAIN,
	};
	static const lv_style_selector_t focus_sel[] = {
		LV_PART_MAIN | LV_STATE_FOCUSED,
		LV_PART_MAIN | LV_STATE_FOCUS_KEY,
		LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY,
	};
	unsigned i;

	for (i = 0; i < sizeof(idle_sel) / sizeof(idle_sel[0]); i++) {
		lv_obj_set_style_bg_color(root, lv_color_hex(DIVOOM_ASTRO_BG), idle_sel[i]);
		lv_obj_set_style_bg_opa(root, LV_OPA_COVER, idle_sel[i]);
		lv_obj_set_style_border_width(root, 2, idle_sel[i]);
		lv_obj_set_style_border_color(root, lv_color_hex(DIVOOM_ASTRO_STROKE), idle_sel[i]);
		lv_obj_set_style_radius(root, DIVOOM_ASTRO_RADIUS_MD, idle_sel[i]);
		lv_obj_set_style_shadow_width(root, 0, idle_sel[i]);
		lv_obj_set_style_outline_width(root, 0, idle_sel[i]);
		lv_obj_set_style_pad_all(root, 0, idle_sel[i]);
	}
	/* 设置列表聚焦主色是琥珀底；这里只改描边，底仍奶油。ITEM_BORDER_ACTIVE 是浅灰，奶油底上看不清 */
	for (i = 0; i < sizeof(focus_sel) / sizeof(focus_sel[0]); i++) {
		lv_obj_set_style_bg_color(root, lv_color_hex(DIVOOM_ASTRO_BG), focus_sel[i]);
		lv_obj_set_style_bg_opa(root, LV_OPA_COVER, focus_sel[i]);
		lv_obj_set_style_border_width(root, 2, focus_sel[i]);
		lv_obj_set_style_border_color(root, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), focus_sel[i]);
		lv_obj_set_style_radius(root, DIVOOM_ASTRO_RADIUS_MD, focus_sel[i]);
		lv_obj_set_style_shadow_width(root, 0, focus_sel[i]);
		lv_obj_set_style_outline_width(root, 0, focus_sel[i]);
		lv_obj_set_style_pad_all(root, 0, focus_sel[i]);
	}
	lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
}

void divoom_time_plan_active_disp_play_add_lvgl_message_box(void)
{
	char time_plan_time[128];
	char time_plan_name[64];
	lv_obj_t *lvgl_obj;
	lv_obj_t *icon_badge;
	lv_obj_t *name_label;
	lv_obj_t *time_label;
	int icon_y;
	int text_x;
	int text_w;
	int text_block_h;
	int text_y0;

	lvgl_obj = divoom_system_msg_notice_add(DIVOOM_TIME_PLAN_POPUP_W, DIVOOM_TIME_PLAN_POPUP_H,
			divoom_time_plan_lvgl_message_box_notice, NULL);
	if (lvgl_obj == NULL)
		return;

	text_x = DIVOOM_ASTRO_TOP_TOAST_PAD_X + DIVOOM_ASTRO_TOP_TOAST_ICON + DIVOOM_ASTRO_TOP_TOAST_GAP;
	text_w = DIVOOM_TIME_PLAN_POPUP_W - text_x - DIVOOM_ASTRO_TOP_TOAST_PAD_X;
	text_block_h = DIVOOM_TIME_PLAN_POPUP_NAME_H + DIVOOM_TIME_PLAN_POPUP_ROW_GAP + DIVOOM_TIME_PLAN_POPUP_TIME_H;
	text_y0 = (DIVOOM_TIME_PLAN_POPUP_H - text_block_h) / 2;
	icon_y = (DIVOOM_TIME_PLAN_POPUP_H - DIVOOM_ASTRO_TOP_TOAST_ICON) / 2;

	divoom_get_lvgl_lock();
	divoom_time_plan_popup_apply_card_style(lvgl_obj);

	/* 无 70_time_plan.png：用焦点色圆角块当图标位，避免缺图撑破 480 布局 */
	icon_badge = lv_obj_create(lvgl_obj);
	lv_obj_remove_style_all(icon_badge);
	lv_obj_set_pos(icon_badge, DIVOOM_ASTRO_TOP_TOAST_PAD_X, icon_y);
	lv_obj_set_size(icon_badge, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);
	lv_obj_set_style_bg_color(icon_badge, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
	lv_obj_set_style_bg_opa(icon_badge, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(icon_badge, DIVOOM_ASTRO_RADIUS_SM, 0);
	lv_obj_clear_flag(icon_badge, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

	name_label = lv_label_create(lvgl_obj);
	lv_obj_set_pos(name_label, text_x, text_y0);
	lv_obj_set_size(name_label, text_w, DIVOOM_TIME_PLAN_POPUP_NAME_H);
	lv_obj_set_style_pad_all(name_label, 0, 0);
	lv_obj_set_style_text_font(name_label,
		divoom_menu_control_get_font_info(22, DIVOOM_MENU_CONTROL_TIME_PLAN_DIAL_ID), 0);
	lv_obj_set_style_text_color(name_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
	lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);

	if (pdivoom_time_plan_handle
		&& pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
	{
		memcpy(time_plan_name,
			pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].plan_name,
			sizeof(time_plan_name));
		time_plan_name[sizeof(time_plan_name) - 1] = '\0';
		lv_label_set_text(name_label, time_plan_name);
	}
	else
	{
		lv_label_set_text(name_label, " ");
	}

	time_label = lv_label_create(lvgl_obj);
	lv_obj_set_pos(time_label, text_x, text_y0 + DIVOOM_TIME_PLAN_POPUP_NAME_H + DIVOOM_TIME_PLAN_POPUP_ROW_GAP);
	lv_obj_set_size(time_label, text_w, DIVOOM_TIME_PLAN_POPUP_TIME_H);
	lv_obj_set_style_pad_all(time_label, 0, 0);
	lv_obj_set_style_text_font(time_label,
		divoom_menu_control_get_font_info(20, DIVOOM_MENU_CONTROL_TIME_PLAN_DIAL_ID), 0);
	lv_obj_set_style_text_color(time_label, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
	lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_long_mode(time_label, LV_LABEL_LONG_DOT);
	divoom_time_plan_get_active_plan_time(time_plan_time);
	lv_label_set_text(time_label, time_plan_time);
	divoom_free_lvgl_lock();
}

void divoom_time_plan_active_disp_play(int disp_mode, int clock_id)
{
    pdivoom_time_plan_handle->dial_info.disp_mode = 0;
    if(disp_mode == 1)
    {
        pdivoom_time_plan_handle->dial_info.clock_id = DIVOOM_TIME_PLAN_CLOCK_ID; //divoom_light_app_get_divoom_animation_edge_clock_id();
        divoom_main_set_global_image_handle(DIVOOM_MANAGE_IMAGE_TIME_PLAN_GIF, DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_PLAN_PIC + pdivoom_time_plan_handle->active_plan_index, 0);
		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_time_plan_handle->dial_info, DIVOOM_MODEL_TIME_PLAN, DIVOOM_TIME_PLAN_APP_EVENT_RUN);
    }
    else
    {
        //pdivoom_time_plan_handle->dial_info.clock_id = clock_id;
        divoom_time_plan_active_disp_play_add_lvgl_message_box();
    }
}

char *divoom_time_plan_get_active_name(void)
{
    if(pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
    {
        return pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].plan_name;
    }
    else
    {
        return NULL;
    }
}

void divoom_time_plan_get_active_time_info(char *time_info)
{
    if(pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
    {
        Pdivoom_time_plan_item_info pcur_info = &pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index];
        sprintf(time_info, "%02d:%02d ~ %02d:%02d",pcur_info->start_time/3600, (pcur_info->start_time%3600)/60,pcur_info->end_time/3600, (pcur_info->end_time%3600)/60);
    }
    else
    {
        strcpy(time_info, " ");
    }
}

void divoom_time_plan_get_active_count_down_info(char *time_info)
{
    if(pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
    {
        Pdivoom_time_plan_item_info pcur_info = &pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index];
        struct tm cur_tm;
        time_t  system_time;

        system_time = time(0);
        localtime_r((time_t *)&system_time, &cur_tm);
        
        //得到当天0点到现在的总秒数
        int cur_time = cur_tm.tm_hour * 3600 + cur_tm.tm_min*60 + cur_tm.tm_sec;

        cur_time = pcur_info->end_time - cur_time;
        if(cur_time>=3600)
        {
            sprintf(time_info, "%02d:%02d:%02d",cur_time/3600,(cur_time%3600)/60, cur_time%60);
        }
        else
        {
            sprintf(time_info, "%02d:%02d",cur_time/60, cur_time%60);
        }
    }
    else
    {
        strcpy(time_info, " ");
    }
}

void divoom_time_plan_active(uint8 index)
{
    divoom_app_schedule_item_info event_info = {0};
    
    divoom_print("%s: we will run the time plan item!: %d\n", __func__, index);

    pdivoom_time_plan_handle->active_plan_index = index;
    
    event_info.model_id = DIVOOM_MODEL_TIME_PLAN;
    event_info.event_id = DIVOOM_TIME_PLAN_APP_EVENT_RUN;
    if(pdivoom_time_plan_handle->plan_info[index].disp_mode != 1)
    {
        event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
    }
    else
    {
        event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
    }
    event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    pdivoom_time_plan_handle->resource_type = divoom_app_schedule_push(&event_info);
	pdivoom_time_plan_handle->plan_info[index].status = DIVOOM_TIME_PLAN_CTRL_ACTIVE;
    if(0 == pdivoom_time_plan_handle->resource_type)
    {
        if(pdivoom_time_plan_handle->plan_info[index].disp_mode == 0)
        {
            divoom_time_plan_active_disp_play_add_lvgl_message_box();        
        }
        return;
    }
    divoom_light_app_open_screen();
    
    divoom_time_plan_active_disp_play(pdivoom_time_plan_handle->plan_info[index].disp_mode, pdivoom_time_plan_handle->plan_info[index].clock_id);  
    if(pdivoom_time_plan_handle->plan_info[index].voice_switch)
    {
        //后面这里增加一下提示音
        char file_name[128];
        
        divoom_voice_read_get_filename(DIVOOM_VOICE_ID_TIME_MANAGE, file_name);
        divoom_local_music_ctrl_tone_play(file_name, divoom_main_get_tone_volume(), NULL, 0);
    }
    divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_TIMEPLAN_START, DIVOOM_SERVER_TOPIC, pdivoom_time_plan_handle->plan_info[index].plan_name);
        
}

void divoom_time_plan_play_end_over_callback(uint8 status)
{
    divoom_app_schedule_item_info event_info = {0};

    if(pdivoom_time_plan_handle->buzzer_start_time)
    {
        pdivoom_time_plan_handle->buzzer_start_time = 0;
    }
    event_info.model_id = DIVOOM_MODEL_TIME_PLAN;
    event_info.event_id = DIVOOM_TIME_PLAN_APP_EVENT_RUN;
    event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&event_info);
}

//时间计划结束，将播放结束提示音，后将全部取消
void divoom_time_plan_cancel_active(uint8 index)
{
    divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_TIMEPLAN_STOP, DIVOOM_SERVER_TOPIC, pdivoom_time_plan_handle->plan_info[index].plan_name);
    divoom_time_plan_play_end_over_callback(1);
}

uint8 divoom_time_plan_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{    
    if(pdivoom_time_plan_handle->active_plan_index > 0 && pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
    {
        divoom_time_plan_active_disp_play(pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].disp_mode, pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].clock_id);  


    	if(pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].voice_switch)
        {
            char file_name[128];
            
            divoom_voice_read_get_filename(DIVOOM_VOICE_ID_TIME_MANAGE, file_name);
            divoom_local_music_ctrl_tone_play(file_name, divoom_main_get_tone_volume(), NULL, 0);
        }
    }

	return 1;
}


uint8 divoom_time_plan_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	
    if(pdivoom_time_plan_handle->active_plan_index > 0 && pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
    {
    	if(pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].voice_switch)
        {
    		divoom_local_music_ctrl_stop_tone();
    	}
    }
    return 1;
}


uint8 divoom_time_plan_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
    if(pdivoom_time_plan_handle->active_plan_index > 0 && pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
    {
    	if(pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].voice_switch)
        {
    		divoom_local_music_ctrl_stop_tone();
    	}
    }
	
    return 1;
}

uint8 divoom_time_plan_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
    if(pdivoom_time_plan_handle->active_plan_index > 0 && pdivoom_time_plan_handle->active_plan_index < pdivoom_time_plan_handle->plan_num)
    {
        if(pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].status == DIVOOM_TIME_PLAN_CTRL_ACTIVE)
        {
            pdivoom_time_plan_handle->plan_info[pdivoom_time_plan_handle->active_plan_index].status = DIVOOM_TIME_PLAN_CTRL_CANCEL;

            divoom_time_plan_play_end_over_callback(1);
        }
    }
	return 1;
}

void divoom_time_plan_init(void)
{
    pdivoom_time_plan_handle = (Pdivoom_time_plan_ctrl)divoom_malloc(sizeof(divoom_time_plan_ctrl),DIVOOM_MODEL_TIME_PLAN,__LINE__, __FILE__);

    memset(pdivoom_time_plan_handle, 0, sizeof(divoom_time_plan_ctrl));
    //pthread_mutex_init(&pdivoom_time_plan_handle->safe_lock, NULL);
    pdivoom_time_plan_handle->safe_lock = divoom_system_create_lock();
    divoom_time_plan_load();
    divoom_sys_model_crl_register(DIVOOM_MODEL_TIME_PLAN,divoom_time_plan_event);
}

void divoom_time_plan_play_over_callback(uint8 status)
{
    divoom_app_schedule_item_info event_info = {0};

    event_info.model_id = DIVOOM_MODEL_TIME_PLAN;
    event_info.event_id = DIVOOM_TIME_PLAN_APP_EVENT_RUN;
    event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&event_info);
}


void divoom_time_plan_main(void)
{
    int i;//, stop_flag = 0;
    uint32 cur_time;
    struct timeval sys_tick;
	struct tm cur_tm;
	uint8 real_week;
    static uint32 pre_tick = 0;
    time_t  system_time;

    system_time = time(0);
    if(system_time < DIVOOM_BASE_UTC_TIME)
    {
        return;
    }

    if(pdivoom_time_plan_handle == NULL)
    {
        return;
    }
    
    if(pdivoom_time_plan_handle->buzzer_start_time && pdivoom_time_plan_handle->buzzer_start_time + DIVOOM_TIME_PLAY_BUZZER_TIME < system_time )
    {
        pdivoom_time_plan_handle->buzzer_start_time = 0;
    }
    
    gettimeofday(&sys_tick, NULL);
    cur_time = sys_tick.tv_sec;
    if(cur_time == pre_tick)
    {
        return;
    }
	pdivoom_time_plan_handle->refresh_flag = 0;
    pre_tick = cur_time;
	localtime_r((time_t *)&system_time, &cur_tm);

	//得到当天0点到现在的总秒数
	cur_time = cur_tm.tm_hour * 3600 + cur_tm.tm_min*60 + cur_tm.tm_sec;
	//转换当前星期几
	real_week = (cur_tm.tm_wday==0)?6:(cur_tm.tm_wday-1);
    
    divoom_system_get_lock(pdivoom_time_plan_handle->safe_lock);
    if(pdivoom_time_plan_handle->active_plan_num && pdivoom_time_plan_handle->plan_switch)
    {
        for(i = 0; i < pdivoom_time_plan_handle->plan_num; i ++)
        {
            //每次保证只有一个才是有效的，不考虑时间交替的方式
	        if(pdivoom_time_plan_handle->plan_info[i].enable == 0 || divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON)
            {
                continue;
            }
	        if(pdivoom_time_plan_handle->plan_info[i].status == DIVOOM_TIME_PLAN_CTRL_WAIT)
            {
                //检查是否启动
                if((cur_time >= pdivoom_time_plan_handle->plan_info[i].start_time) && (cur_time < pdivoom_time_plan_handle->plan_info[i].end_time))						
                {
					//判断循环是否一样
					if((pdivoom_time_plan_handle->plan_info[i].cyc_attr == 0)
							|| (pdivoom_time_plan_handle->plan_info[i].cyc_attr == 1 && pdivoom_time_plan_handle->plan_info[i].cyc_flag[real_week] == 1))
					{
	                    divoom_time_plan_active(i);
					}
                }
            }
            else if(pdivoom_time_plan_handle->plan_info[i].status == DIVOOM_TIME_PLAN_CTRL_ACTIVE)
            {
                //检查是否超时结束
                if((cur_time >= pdivoom_time_plan_handle->plan_info[i].end_time) || (cur_time < pdivoom_time_plan_handle->plan_info[i].start_time))
                {
                    pdivoom_time_plan_handle->plan_info[i].status = DIVOOM_TIME_PLAN_CTRL_WAIT;
                    if(pdivoom_time_plan_handle->plan_num > i + 1 && (cur_time >= pdivoom_time_plan_handle->plan_info[i + 1].start_time) && (cur_time < pdivoom_time_plan_handle->plan_info[i + 1].end_time))                     
                    {
                        //判断循环是否一样
                        if((pdivoom_time_plan_handle->plan_info[i + 1].cyc_attr == 0)
                                || (pdivoom_time_plan_handle->plan_info[i + 1].cyc_attr == 1 && pdivoom_time_plan_handle->plan_info[i + 1].cyc_flag[real_week] == 1))
                        {
                            divoom_time_plan_active(i + 1);
                        }
                    }
                    else
                    {
                        divoom_print("%s: we will end the time plan item!: %d\n", __func__, i);					
    					divoom_time_plan_cancel_active(i);
                    }
                    if(pdivoom_time_plan_handle->plan_info[i].cyc_attr == 0)
                    {
                        pdivoom_time_plan_handle->plan_info[i].enable = 0;
                        if(pdivoom_time_plan_handle->active_plan_num)
                        {
							pdivoom_time_plan_handle->plan_info[i].status = DIVOOM_TIME_PLAN_CTRL_CANCEL;
                            pdivoom_time_plan_handle->active_plan_num --;
                            divoom_print("%s: we will del the time plan item!: %d, %d\n", __func__, i, pdivoom_time_plan_handle->active_plan_num);
                            if(pdivoom_time_plan_handle->active_plan_num == 0)
                            {
                                divoom_net_app_send_packet(DIVOOM_NET_COMM_CLOSE_TIME_PLAN, DIVOOM_SERVER_TOPIC, NULL);
                                pdivoom_time_plan_handle->plan_switch = 0;
                            }
                        }
                    }
                }
                break;
            }
            else
            {
                //检查按键取消的，过期后，将设置这个信息
                if((pdivoom_time_plan_handle->plan_info[i].status == DIVOOM_TIME_PLAN_CTRL_CANCEL) && ((cur_time >= pdivoom_time_plan_handle->plan_info[i].end_time) || (cur_time < pdivoom_time_plan_handle->plan_info[i].start_time)))
                {
                    pdivoom_time_plan_handle->plan_info[i].status = DIVOOM_TIME_PLAN_CTRL_WAIT;
                }
                break;
            }
        }
    }
    divoom_system_free_lock(pdivoom_time_plan_handle->safe_lock);
}

void divoom_time_plan_exit(void)
{
}

