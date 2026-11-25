
#include "divoom_disp_com.h"

/***************************************************
关于灯效文件的保存数据:
闹钟:本地只保存数据配置，不保存闹钟图片，图片只存将要启动的闹钟图片数据，在本地一个独立的alarm.inf
hot文件:维持TIVOO一样，所有的文件最大64K，打到一个包里面
时间管理:将图片数据打包成RGB的，存固定数据大小，最多20项，默认文件为20个；
律动数据:和其它格式动画一个，一个一个动画的顺序保存，在主结构里面记录偏移，如果修改，将文件重新改写
自定义相框:和蓝牙数据一样，独立一个文件保存
开机图案:独立的一个文件。
灯板信息:主数据结构保存一个独立文件。
****************************************************/

#define DIVOOM_DISP_CTRL_MALLOC		divoom_com_malloc
#define DIVOOM_DISP_CTRL_FREE		divoom_com_free
#define DIVOOM_DISP_CTRL_DBG		divoom_print
#define DIVOOM_DISP_CTRL_ERR		divoom_print

#define DIVOOM_DISP_CTRL_TIME_MANAGE_DEBUG		divoom_print

#define DIVOOM_DISP_TEMP_NET_UPDATE_TIME    (3*3600)

#define DIVOOM_DISP_TEMP_WEATHER_HOUR_NOTIC_TIME   (12*1)
#define DIVOOM_DISP_TEMP_WEATHER_FIXED_NOTIC_TIME  (12*1)

#define DIVOOM_DISP_MIXER_NUMBER    6


enum
{
    DIVOOM_DISP_TEMP_HOUR_DISP,
    DIVOOM_DISP_TEMP_FIXED_DISP,
    DIVOOM_DISP_TEMP_END_DISP,
};



typedef struct _divoom_disp_ctrl_info
{
    uint8 system_flag;
    uint8 real_draw_flag;
    uint8 cur_mode;
    uint8 sys_light;
    uint8 switch_light;
    void *light_lock;
    
	uint8 mix_data_buf[DIVOOM_DISP_MIXER_NUMBER];
}divoom_disp_ctrl_info, *Pdivoom_disp_ctrl_info;

static Pdivoom_disp_ctrl_info pdivoom_disp_ctrl_handle = NULL;

uint8 divoom_disp_ctrl_check_real_drawing(void)
{
    return pdivoom_disp_ctrl_handle->system_flag == DIVOOM_SYSTEM_RTOS;
}

void divoom_disp_ctrl_set_light(uint8 light)
{
    /* Battery events can arrive before the display controller is initialized. */
    if (pdivoom_disp_ctrl_handle != NULL)
        pdivoom_disp_ctrl_handle->sys_light = light;
    divoom_screen_bright(light); //80%
}

uint8 divoom_disp_ctrl_get_light(void)
{
    if (pdivoom_disp_ctrl_handle != NULL)
	    return pdivoom_disp_ctrl_handle->sys_light;
    else
        return 0;
}


void divoom_disp_ctrl_set_bright(uint8 light)
{
    if (pdivoom_disp_ctrl_handle != NULL)
    pdivoom_disp_ctrl_handle->sys_light = light;
    divoom_screen_bright(light); //80%
}
void divoom_disp_ctrl_switch_open(void)
{
    if(pdivoom_disp_ctrl_handle)
    {
        if(pdivoom_disp_ctrl_handle->switch_light == 0)
        {
            pdivoom_disp_ctrl_handle->switch_light = 1;
            divoom_screen_bright(pdivoom_disp_ctrl_handle->sys_light);
        }
        
    }
}

void divoom_disp_ctrl_switch_close(void)
{
    if(pdivoom_disp_ctrl_handle)
    {
        if(pdivoom_disp_ctrl_handle->switch_light)
            {
            pdivoom_disp_ctrl_handle->switch_light = 0;
            divoom_screen_bright(0);
        }        
    }
}



/*********************************************************************
  Function:        divoom_disp_ctrl_exit_drawing_pad
  Description:    退出实时画板
  Input:            NONE
  Return:           none
  Author:           albert
**********************************************************************/


uint8 divoom_disp_ctrl_get_mode(void)
{
    if(pdivoom_disp_ctrl_handle == NULL)
    {
        return 0Xff;
    }
    return pdivoom_disp_ctrl_handle->cur_mode;
}

void divoom_disp_ctrl_init(Pdivoom_disp_ctrl_callback pcallback, DIVOOM_SYSTEM_FLAG sys_flag, uint8 start_mode, Pdivoom_sys_setting_info psys_setting)
{
	divoom_com_init(pcallback);
	divoom_screen_init();
	//divoom_gif_mmu_init();
	pdivoom_disp_ctrl_handle = DIVOOM_DISP_CTRL_MALLOC(sizeof(divoom_disp_ctrl_info),__func__,__LINE__);
	if(pdivoom_disp_ctrl_handle == NULL)
	{
		divoom_print("%s:%d: it fails to malloc !\n",__FILE__, __LINE__);
		return ;
	}
	memset(pdivoom_disp_ctrl_handle, 0, sizeof(divoom_disp_ctrl_info));
	pdivoom_disp_ctrl_handle->system_flag = sys_flag;
	pdivoom_disp_ctrl_handle->cur_mode = DIVOOM_DISP_NO_DISP_MODE;
	pdivoom_disp_ctrl_handle->sys_light = psys_setting->brightness;
	pdivoom_disp_ctrl_handle->switch_light = 1;
    pdivoom_disp_ctrl_handle->light_lock  = divoom_system_create_lock();
//    divoom_disp_ctrl_set_mixer_off();    //初始化一下混音的备份数据
	divoom_disp_ctrl_set_mode(start_mode, NULL);
}



//不涉及再存储之前的灯效数据了
//在preproc负责调用light优先级函数
//优先级函数再调用此函数
//在preproc调用light优先级函数之后，并释放数据,所以需要定义一个结构体：灯效模式，灯效数据，释放函数
//divoom_disp_ctrl_app_light_data
//要有一个转换数据的接口，这样原来的数据就在preproc结束之后就释放了，新的申请数据在莫快结束释放

/*********************************************************************
  Function:        divoom_disp_ctrl_set_mode
  Description:    启动相应的灯效
  Input:            mode        灯效的ＩＤ
					light_data         灯效的数据，可以为空
  Return:           NONE
  Author:           albert
**********************************************************************/
void divoom_disp_ctrl_set_mode(uint8 mode, void *light_data)
{
#if 0
    return;
    divoom_system_get_lock(pdivoom_disp_ctrl_handle->light_lock);

	if(mode == pdivoom_disp_ctrl_handle->cur_mode)
	{
        divoom_print("update: mode: %d\n",mode);
        switch(mode)
        {
            case DIVOOM_DISP_LOCAL_GIF_MODE:
                divoom_disp_local_gif_update(light_data);
                break;
            
            case DIVOOM_DISP_DIS_IFFT_TEXT_MODE:
                divoom_disp_iffft_text_update(light_data);
                break;
            
            case DIVOOM_DISP_PLAY_NET_IMAGE_CTRL:
                divoom_disp_Play_net_image_update(light_data);
                break;
            
            case DIVOOM_DISP_CLOCK_MODE:
                divoom_disp_base_clock_update(light_data, 1);
                break;
                
			case DIVOOM_DISP_DEVICE_UPDATE_MODE:
				divoom_disp_update_update(light_data);
				break;

    		case DIVOOM_DISP_ENV_MODE:
    			//divoom_light_env_update_mode((Pdivoom_disp_ctrl_env_info)light_data);
    			break;
    		case DIVOOM_DISP_LIGHT_MODE:
                divoom_disp_light_set_attr((Pdivoom_disp_ctrl_light_color_info)light_data);
    			break;
    		case DIVOOM_DISP_SUBSCRIBE_MODE:
                divoom_disp_subscribe_update_config((Pdivoom_disp_subscribe_data)light_data);                
    			break;
    		case DIVOOM_DISP_SPECIAL_MODE:
    			//divoom_light_special_set_mode((Pdivoom_disp_ctrl_special_info)light_data);
    			break;
    		case DIVOOM_DISP_MUISE_MODE:
    			divoom_light_music_update_mode((Pdivoom_disp_ctrl_music_info)light_data);
    			break;
    		case DIVOOM_DISP_USER_DEFINE_MODE:
                divoom_disp_user_define_update(light_data);
    			break;
    		case DIVOOM_DISP_NO_DISP_MODE:
    			break;
    		case DIVOOM_DISP_PHONE_CTRL:
    			{                    
                    divoom_disp_phone_ctrl_update(light_data);
    			}
    			break;
    		case DIVOOM_DISP_PHONE_GIF_CTRL:
                divoom_disp_gif_update(light_data);
                //divoom_disp_gif_update((Pdivoom_disp_ctrl_gif_info)data, 0, 1);
    			break;
    		case DIVOOM_DISP_HTTP_GIF_CTRL:
                divoom_disp_custom_dial_update(light_data);
                //divoom_disp_gif_update((Pdivoom_disp_ctrl_gif_info)data, 0, 1);
    			break;
    		case DIVOOM_DISP_PIC_SCAN_CTRL:
                divoom_light_pic_scan_update((Pdivoom_light_pic_scan_para)light_data);
    			break;
    		case DIVOOM_DISP_MOVIE_CTRL:
    			divoom_light_word_update((Pdivoom_disp_ctrl_movie_ctrl)light_data);
    			break;
    		case DIVOOM_DISP_GAME_CTRL:
    			//divoom_disp_game_set_game((Pdivoom_disp_ctrl_game_info)light_data);
    			break;
    		case DIVOOM_DISP_TALK_CTRL:
                
    			break;
    		case DIVOOM_DISP_EVENT_DIS_CTRL:
                divoom_disp_event_update((Pdivoom_disp_event_disp_info)light_data);
    			break;
    		case DIVOOM_DISP_ALARM_MODE:
                
    			divoom_disp_alarm_update_disp((Pdivoom_disp_ctrl_alarm_info)light_data);
    			break;
            
    		case DIVOOM_DISP_DIALY_MODE:
                
    			divoom_disp_dialy_update_disp((Pdivoom_disp_ctrl_alarm_info)light_data);
    			break;
            
    		case DIVOOM_DISP_SLEEP_MODE:
                divoom_disp_sleep_update((void *)light_data);
    			break;
    		case DIVOOM_DISP_PHONE_NUMBER_MODE:
    			break;
    		case DIVOOM_DISP_SET_ALARM_MODE:
    			break;
    		case DIVOOM_DISP_LEAVE_MSG_MODE:
                //divoom_disp_leave_msg_update(data);
    			break;
    		case DIVOOM_DISP_LEAVE_MSG_PLAY_MODE:
    			break;
    		case DIVOOM_DISP_TEST_MODE:
				divoom_disp_test_update((Pdivoom_disp_ctrl_product_test_info)light_data);
    			break;
    		case DIVOOM_DISP_WATCH_MODE:
    			{
    				//Pdivoom_disp_ctrl_watch_ctrl pwatch_info = (Pdivoom_disp_ctrl_watch_ctrl)data;
    				//divoom_disp_ctrl_update_watch_info(pwatch_info);
					divoom_disp_watch_play((Pdivoom_disp_ctrl_watch_ctrl)light_data);
    			}
    			break;
    		case DIVOOM_DISP_SCORE_MODE:
    			divoom_disp_score_update((Pdivoom_disp_ctrl_score_ctrl)light_data);
    			break;
    		case DIVOOM_DISP_TIME_MANAGE_MODE:
    			divoom_disp_time_manange_update(light_data);
    			break;
    		case DIVOOM_DISP_TEMP_EVENT_MODE:
    			break;
    		case DIVOOM_DISP_MIXER_CTRL:
                //divoom_light_mixer_update_mode(light_data);
    			break;
    		case DIVOOM_DISP_MUSICAL_INSTRUMENTS_MODE:
                //divoom_light_musical_instruments_update(light_data);
    			break;
    		case DIVOOM_DISP_WORD_MODE:
                divoom_light_word_update(light_data);
    			break;
    		case DIVOOM_DISP_POHONE_NAME:
    			break;
    		case DIVOOM_DISP_NOISE_MODE:
    			break;
    		case DIVOOM_DISP_MUL_SEARCH_MODE:
    			break;
    		case DIVOOM_DISP_DOWNLOAD_FILE_MODE:
                divoom_disp_download_file_update(light_data);
    			break;
    			break;
    		case DIVOOM_DISP_SLEEP_TIME:
    			break;
    		case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
				divoom_disp_count_time_down_update_info((Pdivoom_disp_count_time_down_struct)light_data);
    			break;
    		case DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL:
    			break;
    		case DIVOOM_DISP_ROLL_CLOSE:
    			break;
        }
        divoom_system_free_lock(pdivoom_disp_ctrl_handle->light_lock);
        return ;
	}
	divoom_print("%s: cur mode: %d, new mode: %d\n", __func__, pdivoom_disp_ctrl_handle->cur_mode, mode);

	switch(pdivoom_disp_ctrl_handle->cur_mode)
	{
        case DIVOOM_DISP_LOCAL_GIF_MODE:
            divoom_disp_local_gif_exit();
            break;
        
        case DIVOOM_DISP_DIS_IFFT_TEXT_MODE:
            divoom_disp_iffft_text_exit();
            break;
            
        case DIVOOM_DISP_PLAY_NET_IMAGE_CTRL:
            divoom_disp_Play_net_image_exit();
            break;
        
        case DIVOOM_DISP_CLOCK_MODE:
            divoom_disp_base_clock_close(1);
            //divoom_font_manage_clear_font_cache();
            break;
            
        case DIVOOM_DISP_DEVICE_UPDATE_MODE:
            divoom_disp_update_exit();
            break;

		case DIVOOM_DISP_ENV_MODE:
    	    //divoom_light_env_close();
			break;
		case DIVOOM_DISP_LIGHT_MODE:
            divoom_disp_light_close();
			break;
		case DIVOOM_DISP_SUBSCRIBE_MODE:
            divoom_disp_subscribe_close();  
			break;
		case DIVOOM_DISP_SPECIAL_MODE:
            //divoom_light_special_close();
			break;
		case DIVOOM_DISP_MUISE_MODE:
            divoom_light_music_close();            
			break;
		case DIVOOM_DISP_USER_DEFINE_MODE:
			divoom_disp_user_define_close();
			break;
		case DIVOOM_DISP_NO_DISP_MODE:
            divoom_disp_no_disp_close();
			break;
		case DIVOOM_DISP_PHONE_CTRL:
            divoom_disp_phone_ctrl_close();
			break;
		case DIVOOM_DISP_PHONE_GIF_CTRL:
            divoom_disp_gif_close();
            if(mode <= DIVOOM_DISP_NO_DISP_MODE && mode != DIVOOM_DISP_MUISE_MODE)
            {
                divoom_gif_ctrl_clear_cache();
            }
			break;
		case DIVOOM_DISP_HTTP_GIF_CTRL:
            divoom_disp_custom_dial_exit();
            if(mode <= DIVOOM_DISP_NO_DISP_MODE && mode != DIVOOM_DISP_MUISE_MODE)
            {
                divoom_gif_ctrl_clear_cache();
            }
		case DIVOOM_DISP_PIC_SCAN_CTRL:
            divoom_light_pic_scan_close();
			break;
		case DIVOOM_DISP_MOVIE_CTRL:
            divoom_light_word_close();
			break;
		case DIVOOM_DISP_GAME_CTRL:
            //divoom_disp_game_close();
			break;
		case DIVOOM_DISP_TALK_CTRL:
            //divoom_disp_talk_close();
			break;
		case DIVOOM_DISP_POWER_ON:
            divoom_disp_power_on_close();
			break;
		case DIVOOM_DISP_EVENT_DIS_CTRL:
            divoom_disp_event_close();
			break;
		case DIVOOM_DISP_FM_MODE:
			break;
		case DIVOOM_DISP_ALARM_MODE:
            divoom_disp_alarm_close(); 
			break;
        
		case DIVOOM_DISP_DIALY_MODE:
            divoom_disp_dialy_close(); 
			break;
        
		case DIVOOM_DISP_SLEEP_MODE:
            divoom_disp_sleep_close();
			break;
		case DIVOOM_DISP_PHONE_NUMBER_MODE:
            //divoom_disp_phone_number_close();
			break;
		case DIVOOM_DISP_SET_ALARM_MODE:
            //divoom_disp_set_alarm_close();
			break;
		case DIVOOM_DISP_LEAVE_MSG_MODE:
            //divoom_disp_leave_msg_close();
			break;
		case DIVOOM_DISP_LEAVE_MSG_PLAY_MODE:
            //divoom_disp_gif_close();
			break;
		case DIVOOM_DISP_TEST_MODE:
            divoom_disp_test_close();
			break;
		case DIVOOM_DISP_WATCH_MODE:
            divoom_disp_watch_close();
			break;
		case DIVOOM_DISP_SCORE_MODE:
            divoom_disp_score_close();
			break;
		case DIVOOM_DISP_TIME_MANAGE_MODE:
            divoom_disp_time_manange_close();
			break;
		case DIVOOM_DISP_TEMP_EVENT_MODE:
    	    //divoom_light_env_close();
			break;
		case DIVOOM_DISP_MIXER_CTRL:
            //divoom_light_mixer_close();
			break;
		case DIVOOM_DISP_MUSICAL_INSTRUMENTS_MODE:
            //divoom_light_musical_instruments_close();
			break;
		case DIVOOM_DISP_POHONE_NAME:
		case DIVOOM_DISP_WORD_MODE:
            divoom_light_word_close();
			break;
		case DIVOOM_DISP_NOISE_MODE:
			divoom_disp_noise_close();
			break;
		case DIVOOM_DISP_MUL_SEARCH_MODE:
			break;
		case DIVOOM_DISP_DOWNLOAD_FILE_MODE:
            divoom_disp_download_file_close();
			break;
		case DIVOOM_DISP_SLEEP_TIME:
    	    //divoom_light_env_close();
			break;
		case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
			divoom_disp_count_time_down_close();
			break;
		case DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL:
			divoom_disp_high_pri_event_dis_ctrl_close();
			break;
		case DIVOOM_DISP_ROLL_CLOSE:
			break;
	}
	switch(mode)
	{
        case DIVOOM_DISP_LOCAL_GIF_MODE:
            divoom_disp_local_gif_init(light_data);
            break;
        
        case DIVOOM_DISP_DIS_IFFT_TEXT_MODE:
            divoom_disp_iffft_text_init(light_data);
            break;
            
        case DIVOOM_DISP_PLAY_NET_IMAGE_CTRL:
            divoom_disp_Play_net_image_init(light_data);
            break;
        
        case DIVOOM_DISP_CLOCK_MODE:
            //divoom_gif_ctrl_clear_cache();//特殊做法清楚一下ＣＡＣＨＥ
            divoom_disp_base_clock_init(light_data);
            break;
            
        case DIVOOM_DISP_DEVICE_UPDATE_MODE:
            divoom_disp_update_init(light_data);
            break;

		case DIVOOM_DISP_ENV_MODE:
             //   divoom_light_env_init((Pdivoom_disp_ctrl_env_info)light_data, divoom_com_get_env_info, 0);            
			break;
		case DIVOOM_DISP_LIGHT_MODE:
            divoom_disp_light_init((Pdivoom_disp_ctrl_light_color_info)light_data);
			break;
		case DIVOOM_DISP_SUBSCRIBE_MODE:
            divoom_disp_subscribe_init((Pdivoom_disp_subscribe_data)light_data);  
			break;
		case DIVOOM_DISP_SPECIAL_MODE:
            //divoom_light_special_init(light_data);
			break;
		case DIVOOM_DISP_MUISE_MODE:
            divoom_light_music_init((Pdivoom_disp_ctrl_music_info)light_data, 1);
			break;
		case DIVOOM_DISP_USER_DEFINE_MODE:
			divoom_disp_user_define_init((void *)light_data);
			break;
		case DIVOOM_DISP_NO_DISP_MODE:
			divoom_disp_no_disp_init();
			break;
		case DIVOOM_DISP_PHONE_CTRL:
			{
                divoom_disp_phone_ctrl_init(light_data);
				//Pdivoom_disp_ctrl_pic_info pic_data = (Pdivoom_disp_ctrl_pic_info)data;
            	//divoom_disp_draw_ctrl_init(pdivoom_disp_ctrl_handle->pic_buf, pic_data->pic_type);
			}
			break;
		case DIVOOM_DISP_PHONE_GIF_CTRL:
			//CHANGE_BY_COMPILE
            divoom_disp_gif_init(light_data);
			break;
		case DIVOOM_DISP_HTTP_GIF_CTRL:
            divoom_disp_custom_dial_init(light_data);
            break;
		case DIVOOM_DISP_PIC_SCAN_CTRL:
            divoom_light_pic_scan_init((Pdivoom_light_pic_scan_para)light_data);
			break;
		case DIVOOM_DISP_MOVIE_CTRL:
            divoom_light_word_init(light_data, 0);
			break;
		case DIVOOM_DISP_GAME_CTRL:
			//divoom_disp_game_init(light_data);
			break;
		case DIVOOM_DISP_TALK_CTRL:
            //divoom_disp_talk_init(light_data);
			break;
		case DIVOOM_DISP_POWER_ON:
            divoom_disp_power_on_init(light_data);
			break;
		case DIVOOM_DISP_EVENT_DIS_CTRL:
            divoom_disp_event_init((void *)light_data, pdivoom_disp_ctrl_handle->cur_mode);
			break;
		case DIVOOM_DISP_FM_MODE:
			break;
		case DIVOOM_DISP_ALARM_MODE:            
			divoom_disp_alarm_init((Pdivoom_disp_ctrl_alarm_info)light_data);
			break;
        
		case DIVOOM_DISP_DIALY_MODE:            
			divoom_disp_dialy_init((void *)light_data);
			break;
        
		case DIVOOM_DISP_SLEEP_MODE:
            divoom_disp_sleep_init((void *)light_data);
			break;
		case DIVOOM_DISP_PHONE_NUMBER_MODE:
            //divoom_disp_phone_number_init(light_data);
			break;
		case DIVOOM_DISP_SET_ALARM_MODE:
            //divoom_disp_set_alarm_init();
			break;
		case DIVOOM_DISP_LEAVE_MSG_MODE:
			break;
		case DIVOOM_DISP_LEAVE_MSG_PLAY_MODE:
            //divoom_disp_gif_init((Pdivoom_disp_ctrl_gif_info)data, 1, 0);
			break;
		case DIVOOM_DISP_TEST_MODE:
			divoom_disp_test_init((Pdivoom_disp_ctrl_product_test_info)light_data);
			break;
		case DIVOOM_DISP_WATCH_MODE:
            divoom_disp_watch_init(light_data, divoom_com_get_tool_info);
			break;
		case DIVOOM_DISP_SCORE_MODE:
			divoom_disp_score_init((Pdivoom_disp_ctrl_score_ctrl)light_data);
			break;
		case DIVOOM_DISP_TIME_MANAGE_MODE:
            divoom_disp_time_manange_init(light_data);
			break;
		case DIVOOM_DISP_TEMP_EVENT_MODE:
            //divoom_light_env_init((Pdivoom_disp_ctrl_env_info)light_data, divoom_com_get_env_info, 0);            
			break;
		case DIVOOM_DISP_MIXER_CTRL:
		    //divoom_light_mixer_init(light_data);
			break;
		case DIVOOM_DISP_MUSICAL_INSTRUMENTS_MODE:
            //divoom_light_musical_instruments_init(light_data);
			break;
            
		case DIVOOM_DISP_POHONE_NAME:
		case DIVOOM_DISP_WORD_MODE:
            divoom_light_word_init(light_data, 0);
			break;
		case DIVOOM_DISP_NOISE_MODE:
			divoom_disp_noise_init();
			break;
		case DIVOOM_DISP_MUL_SEARCH_MODE:
			break;
		case DIVOOM_DISP_DOWNLOAD_FILE_MODE:
            divoom_disp_download_file_init(light_data);
			break;
		case DIVOOM_DISP_SLEEP_TIME:
			//	divoom_light_env_init((Pdivoom_disp_ctrl_env_info)light_data, divoom_com_get_env_info, 0);
			break;
		case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
			divoom_disp_count_time_down_init((Pdivoom_disp_count_time_down_struct)light_data, divoom_com_get_tool_info);
			break;
		case DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL:
			divoom_disp_high_pri_event_dis_ctrl_init((Pdivoom_disp_high_pri_event_struct)light_data);
			break;
		case DIVOOM_DISP_ROLL_CLOSE:
            //divoom_disp_ctrl_clr_screen();
			break;
	}

	pdivoom_disp_ctrl_handle->cur_mode = mode;

    divoom_system_free_lock(pdivoom_disp_ctrl_handle->light_lock);
#endif
}


uint8 divoom_disp_ctrl_app_active_model(uint8 *is_in_notice)
{
	if(pdivoom_disp_ctrl_handle)
	{
		return pdivoom_disp_ctrl_handle->cur_mode;
	}
	return DIVOOM_DISP_END_MODE;
}

void divoom_disp_ctrl_app_exit_mode(uint8 light_model, uint8 model_id, uint8 event_id)
{
	divoom_sys_msg_info msg_info = {0};
	if(light_model == DIVOOM_DISP_EVENT_DIS_CTRL)
	{
		uint8 *user_data = divoom_system_malloc(sizeof(uint8));
		*user_data = event_id;
		//发送到main模块，退出这个灯效
		msg_info.msg_type = 1;
		msg_info.dest_id = DIVOOM_MODEL_MAIN;
		msg_info.msg_id = DIVOOM_MAIN_NOTICE_OVER_MESSAGE;
		msg_info.result = 0;
		msg_info.data = user_data;
		divoom_sys_model_crl_msg_push(&msg_info);
	}
	else
	{
		//do somethig
		//在main模块执行退出当前的schedule
		Pdivoom_disp_event_extend_info send_info = NULL;
		send_info = divoom_com_malloc(sizeof(divoom_disp_event_extend_info), __func__, __LINE__);
		send_info->model_id = model_id;
		send_info->event_id = event_id;
		msg_info.msg_type = 1;
		msg_info.dest_id = DIVOOM_MODEL_MAIN;
		msg_info.msg_id = DIVOOM_MAIN_EVENT_SCHEDULE_CANCEL;
		msg_info.data = (uint8 *)send_info;
		divoom_sys_model_crl_msg_push(&msg_info);
	}
}



void divoom_disp_ctrl_clr_screen(void)
{
	divoom_screen_back_screen();
}


/*********************************************************************
  Function:        divoom_disp_ctrl_fill
  Description:    将显示数据清屏成什么某一颜色
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_fill(uint8 *disp_buf, const divoom_screen_color_info * color)
{
	uint8 i , j;

	for (i = 0; i < SCREEN_HEIGHT_32; i++)
	{
		for (j = 0; j < SCREEN_WIDTH_32; j++)
		{
			divoom_disp_ctrl_set_dot(j, i, color, disp_buf);
		}
	}
	
}

void divoom_disp_ctrl_fill_64(uint8 *disp_buf, const divoom_screen_color_info * color)
{
	uint8 i , j;

	for (i = 0; i < SCREEN_HEIGHT_64; i++)
	{
		for (j = 0; j < SCREEN_WIDTH_64; j++)
		{
			divoom_disp_ctrl_set_dot_64(j, i, color, disp_buf);
		}
	}
}

/*********************************************************************
  Function:        divoom_disp_ctrl_get_data
  Description:    从显示缓冲区中得到某个点的RGB
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/


void divoom_disp_ctrl_get_data(uint16 off, Pdivoom_screen_color_info color, uint8 *disp_buf)
{
    off = (off << 1) + off;
    color->r_c = disp_buf[off];
    color->g_c = disp_buf[off+1];
    color->b_c = disp_buf[off+2];    
}


void divoom_disp_ctrl_small_set_data(uint16 off,  const divoom_screen_color_info *  color, uint8 *disp_buf)
{
    
    if(off >= SCREEN_WIDTH_16 * SCREEN_HEIGHT_16)
    {
        return ;
    }
    off = (off << 1) + off;
    disp_buf[off] = color->r_c;
    disp_buf[off+1] = color->g_c;
    disp_buf[off+2] = color->b_c;    
}


/*********************************************************************
  Function:        divoom_disp_ctrl_set_data
  Description:   设置某个点的RGB
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/


void divoom_disp_ctrl_set_data(uint16 off,  const divoom_screen_color_info *  color, uint8 *disp_buf)
{
    
    off = (off << 1) + off;
    disp_buf[off] = color->r_c;
    disp_buf[off+1] = color->g_c;
    disp_buf[off+2] = color->b_c;    
}


/*********************************************************************
  Function:        divoom_disp_ctrl_set_dot
  Description:   设置某个点的RGB
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_set_dot(uint8 x, uint8 y, const divoom_screen_color_info *pcolor, uint8 *disp_buf)
{
    uint16 off;
//    if(x < 0 || y < 0 || x >= SCREEN_WIDTH_32 || y >= SCREEN_HEIGHT_32)
    if(x >= SCREEN_WIDTH_32 || y >= SCREEN_HEIGHT_32)        
        return;
    off = (y * SCREEN_WIDTH_32 + x);
    off = (off << 1) + off;
    disp_buf[off] = pcolor->r_c;
    disp_buf[off+1] = pcolor->g_c;
    disp_buf[off+2] = pcolor->b_c;
    
}

/*********************************************************************
  Function:     divoom_disp_ctrl_set_dot_64
  Description:  把24bit的rgb转成15bit，设置某个点的RGB，一个点占两个字节 
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区 
                       disp_buf     显示缓冲区
  Return:           void
  Author:           wangft
  Data:            2020-10-29
**********************************************************************/

void divoom_disp_ctrl_set_dot_64(uint8 x, uint8 y, const divoom_screen_color_info *pcolor, uint8 *disp_buf)
{
    uint16 off;
//    if(x < 0 || y < 0 || x >= SCREEN_WIDTH_32 || y >= SCREEN_HEIGHT_32)
    if(x >= SCREEN_WIDTH_64 || y >= SCREEN_HEIGHT_64)        
        return;
    off = (y * SCREEN_WIDTH_64 + x);
    off = (off << 1) + off;
    disp_buf[off] = pcolor->r_c;
    disp_buf[off+1] = pcolor->g_c;
    disp_buf[off+2] = pcolor->b_c;

/*
	uint16 r, g, b;
	uint16 index = 0;
	uint16 off, rgb = 0;

	r = divoom_screen_get_gamma_64(pcolor->r_c);
	g = divoom_screen_get_gamma_64(pcolor->g_c);
	b = divoom_screen_get_gamma_64(pcolor->b_c);
	
	off = (y * SCREEN_WIDTH_64 + x);
	//[0]r[11111]g[11111]b[11111] rgb排序
	index = off*2;
	rgb = (r << 10);
	rgb += (g << 5);
	rgb += b;
//小端
	disp_buf[index] = (uint8)rgb;
	disp_buf[index+1] = (uint8)(rgb >> 8); 
*/
}

/*********************************************************************
  Function:        divoom_disp_ctrl_get_dot
  Description:    从显示缓冲区中得到某个点的RGB
  Input:            X    
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_get_dot(uint8 x, uint8 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf)
{
    uint16 off;

    off = (y * SCREEN_WIDTH_32 + x);
    off = (off << 1) + off;
    pcolor->r_c = disp_buf[off];
    pcolor->g_c = disp_buf[off+1];
    pcolor->b_c = disp_buf[off+2];
    
}

/*********************************************************************
  Function:     divoom_disp_ctrl_get_dot_64
  Description:    从显示缓冲区中得到某个点的RGB，此时的rgb是15bit长度
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           wangft
  Data:            2020-10-29
**********************************************************************/

void divoom_disp_ctrl_get_dot_64(uint8 x, uint8 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf)
{
	uint16 off;

	off = (y * SCREEN_WIDTH_64 + x);
	off = (off << 1) + off;
	pcolor->r_c = disp_buf[off];
	pcolor->g_c = disp_buf[off+1];
	pcolor->b_c = disp_buf[off+2];


/*
	uint16 index = 0;
	uint16 off, rgb = 0;
	off = (y * SCREEN_WIDTH_64 + x);
	index = off*16/8; //一个点占两个字节
	//[0]r[11111]g[11111]b[11111] rgb排序
	rgb = (uint16)disp_buf[index];
	rgb += (uint16)disp_buf[index+1] << 8;
	pcolor->r_c = (rgb >> 10) & 0x1f;
	pcolor->g_c = (rgb >> 5) & 0x1f;
	pcolor->b_c = rgb & 0x1f;
*/
}

/**
 * @function:    divoom_disp_ctrl_set_data_xy
 * @description: 设置(x,y)点的RGB
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        x              [x坐标]
 * @param        y              [y坐标]
 * @param        pcolor         [颜色缓冲区]
 * @param        disp_buf       [显示缓冲区]
 */
void divoom_disp_ctrl_set_data_xy(int16 x,int16 y,const divoom_screen_color_info *pcolor, uint8 *disp_buf)
{
    uint16 off;
    if(x < 0 || y < 0 || x >= SCREEN_WIDTH_32 || y >= SCREEN_HEIGHT_32)
        return;
    // divoom_disp_ctrl_set_data(x + y * SCREEN_WIDTH_32,pcolor,disp_buf);
//    divoom_disp_ctrl_set_data(x + (y << 4),pcolor,disp_buf); 
    off = (y * SCREEN_WIDTH_32 + x);
    off = (off << 1) + off;
    disp_buf[off] = pcolor->r_c;
    disp_buf[off+1] = pcolor->g_c;
    disp_buf[off+2] = pcolor->b_c;    
}



/*****************************************************************************
 Prototype    : divoom_disp_ctrl_pic_extension
 Description  : 将边长为base_len的图片pic_data,扩大边长multi倍到disp_buf上
 Input        : uint8 *disp_buf   
                uint8 *pic_data  
                uint16 base_len  
                uint16 multi     
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/1/15
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_ctrl_pic_extension(uint8 *disp_buf,uint8 *pic_data,uint16 base_len,uint16 multi)
{
	uint16 i,j,x,y,real_len = base_len*multi;
	for(j = 0; j < real_len; j++)
	{
		for(i = 0; i < real_len; i++)
		{
			x = i/multi;
			y = j/multi;
			memcpy((disp_buf+(i+real_len*j)*3),(pic_data+(x+base_len*y)*3),3);
		}
	}
}


//-----------------------------------------------------------------------------------------------------------------
//------------数字显示------------//
divoom_screen_color_info divoom_color_black = {0,0,0};
typedef struct _divoom_disp_digit_info{
    uint8 x;        //  字宽
    uint8 y;        //  字高
    uint8 *arr;     //  字模指针
}divoom_disp_digit_info, *Pdivoom_disp_digit_info;

//关于这个数组的解释     digit_disp_info[11][x]   11代表  前10个数组为0-9，   x代表字模的高为x个点
//数组中的值表示这一行中点状态 比如7的二进制为   0000 0111    根据字模宽为3  所以有效的为低3位，其它的同理
//数据０到９的显示像点为３*５
static const uint8 divoom_disp_digit_small_disp_Info[11][5] = {                                  
    {7,5,5,5,7},
    {2,2,2,2,2},
    {7,1,7,4,7},
    {7,1,7,1,7},
    {5,5,7,1,1},
    {7,4,7,1,7},
    {7,4,7,5,7},
    {7,1,1,1,1},
    {7,5,7,5,7},
    {7,5,7,1,7},
    {0,0,0,0,0},
};

static const uint8 divoom_disp_dight_3X7_disp_info[11][7] = {
	{7,5,5,5,5,5,7},
	{2,6,2,2,2,2,7},
	{7,1,1,7,4,4,7},
	{7,1,1,7,1,1,7},
	{5,5,5,7,1,1,1},
	{7,4,4,7,1,1,7},
	{7,4,4,7,5,5,7},
	{7,5,1,1,1,1,1},
	{7,5,5,7,5,5,7},
	{7,5,5,7,1,5,7},
    {0,0,0,0,0,0,0},
};

static const uint8 divoom_disp_digit_shadow_3x5_disp_Info[11][6] = {                                  
    {7,5,5,5,7,0},
    {1,1,1,1,1,0},
    {7,1,7,4,7,0},
    {7,1,7,1,7,0},
    {5,5,7,1,1,0},
    {7,4,7,1,7,0},
    {7,4,7,5,7,0},
    {7,1,1,1,1,0},
    {7,5,7,5,7,0},
    {7,5,7,1,7,0},
    {0,0,0,0,0,0},
};

//数据０到９的显示像点为4*7
static const uint8 divoom_disp_digit_big_disp_Info[11][7] = {                                  
    {15,9,9,9,9,9,15},
    {1,1,1,1,1,1,1},
    {15,1,1,15,8,8,15},
    {15,1,1,15,1,1,15},
    {9,9,9,15,1,1,1},
    {15,8,8,15,1,1,15},
    {15,8,8,15,9,9,15},
    {15,1,1,1,1,1,1},
    {15,9,9,15,9,9,15},
    {15,9,9,15,1,1,1},
    {0,0,0,0,0,0,0},
};
static const uint8 divoom_disp_digit_shadow_4x7_disp_Info[11][8] = {                                  
    {15,9,9,9,9,9,15,0},
    {1,1,1,1,1,1,1,0},
    {15,1,1,15,8,8,15,0},
    {15,1,1,15,1,1,15,0},
    {9,9,9,15,1,1,1,0},
    {15,8,8,15,1,1,15,0},
    {15,8,8,15,9,9,15,0},
    {15,1,1,1,1,1,1,0},
    {15,9,9,15,9,9,15,0},
    {15,9,9,15,1,1,1,0},
    {0,0,0,0,0,0,0,0},
};

const uint8 divoom_disp_digit_fang_shadow_disp_info[11][11] = {
	//0
	{63, 63, 51, 51, 51, 51, 51, 51, 63, 63, 0},
	//1
	{12, 28, 28, 12, 12, 12, 12, 12, 63, 63, 0},
	//2
	{63, 63, 3, 3, 63, 63, 48, 48, 63, 63, 0},
	//3
	{63, 63, 3, 3, 63, 63, 3, 3, 63, 63, 0},
	//4
	{51, 51, 51, 51, 63, 63, 3, 3, 3, 3, 0},
	//5
	{63, 63, 48, 48, 63, 63, 3, 3, 63, 63, 0},
	//6
	{63, 63, 48, 48, 63, 63, 51, 51, 51, 63, 0},
	//7
	{63, 63, 3, 3, 3, 3, 3, 3, 3, 3, 0},
	//8
	{63, 63, 51, 51, 63, 63, 51, 51, 63, 63, 0},
	//9
	{63, 63, 51, 51, 63, 63, 3, 3, 3, 3, 0},
};

static const uint8 divoom_disp_digit_8X14_shadow_disp_info[11][15] = {
	{126, 255, 195, 195, 195, 195, 195, 195, 195, 195, 195, 195, 255, 126, 0},
	{24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0},
	{126, 255, 195, 3, 3, 3, 127, 254, 192, 192, 192, 195, 255, 126, 0},
	{254, 255, 3, 3, 3, 3, 255, 255, 3, 3, 3, 3, 255, 254, 0},
	{195, 195, 195, 195, 195, 195, 255, 255, 3, 3, 3, 3, 3, 3, 0},
	{254, 255, 195, 192, 192, 192, 254, 127, 3, 3, 3, 195, 255, 126, 0},
	{126, 255, 195, 192, 192, 192, 254, 255, 195, 195, 195, 195, 255, 126, 0},
	{255, 255, 195, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0},
	{126, 255, 195, 195, 195, 195, 255, 255, 195, 195, 195, 195, 255, 126, 0},
	{126, 255, 195, 195, 195, 195, 255, 255, 3, 3, 3, 255, 255, 126, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static const uint8 divoom_disp_digit_6X12_shadow_disp_info[11][13] = {
	{30, 63, 51, 51, 51, 51, 51, 51, 51, 51, 63, 30, 0},
	{6, 14, 22, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0},
	{30, 63, 51, 51, 3, 3, 7, 14, 28, 56, 63, 63, 0},
	{30, 63, 51, 3, 3, 30, 31, 3, 3, 51, 63, 30, 0},
	{51, 51, 51, 51, 51, 51, 63, 31, 3, 3, 3, 3, 0},
	{31, 63, 48, 48, 62, 63, 3, 3, 3, 51, 63, 30, 0},
	{30, 63, 51, 48, 48, 62, 63, 51, 51, 51, 63, 30, 0},
	{63, 63, 3, 3, 7, 6, 6, 12, 12, 28, 24, 24, 0},
	{30, 63, 51, 51, 51, 30, 63, 51, 51, 51, 63, 30, 0},
	{30, 63, 51, 51, 51, 63, 31, 3, 3, 51, 63, 30, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static const uint8 divoom_disp_digit_more_disp_info[11][10] = {
	{30, 33, 33, 33, 33, 33, 33, 33, 33, 30},
	{4, 28, 4, 4, 4, 4, 4, 4, 4, 4},
	{30, 33, 33, 1, 2, 4, 8, 16, 32, 63},
	{30, 33, 1, 1, 14, 3, 1, 1, 35, 30},
	{2, 6, 10, 10, 18, 34, 63, 2, 2, 2},
	{63, 32, 32, 32, 46, 51, 1, 1, 33, 30},
	{14, 17, 32, 32, 46, 49, 33, 33, 33, 30},
	{63, 1, 1, 2, 2, 4, 4, 8, 8, 16},
	{30, 33, 33, 51, 30, 51, 33, 33, 51, 30},
	{30, 33, 33, 33, 35, 29, 1, 1, 34, 28},
	{0,0,0,0,0,0,0,0, 0, 0},
};

static const uint8 divoom_disp_digit_6X11_disp_info[11][11] = {
	{30,33,33,33,33,0,33,33,33,33,30},
	{2,2,2,2,2,0,2,2,2,2,2},
	{30,1,1,1,1,30,32,32,32,32,30},
	{30,1,1,1,1,30,1,1,1,1,30},
	{33,33,33,33,33,30,1,1,1,1,1},
	{30,32,32,32,32,30,1,1,1,1,30},
	{30,32,32,32,32,30,33,33,33,33,30},
	{30,33,33,1,1,0,1,1,1,1,1},
	{30,33,33,33,33,30,33,33,33,33,30},
	{30,33,33,33,33,30,1,1,1,33,30},
	{0,0,0,0,0,0,0,0,0,0,0},
};

const uint8 divoom_disp_digit_fang_disp_info[11][10] = {
	//0
	{63, 63, 51, 51, 51, 51, 51, 51, 63, 63},
	//1
	{12, 28, 28, 12, 12, 12, 12, 12, 63, 63},
	//2
	{63, 63, 3, 3, 63, 63, 48, 48, 63, 63},
	//3
	{63, 63, 3, 3, 63, 63, 3, 3, 63, 63},
	//4
	{51, 51, 51, 51, 63, 63, 3, 3, 3, 3},
	//5
	{63, 63, 48, 48, 63, 63, 3, 3, 63, 63},
	//6
	{63, 63, 48, 48, 63, 63, 51, 51, 51, 63},
	//7
	{63, 63, 3, 3, 3, 3, 3, 3, 3, 3},
	//8
	{63, 63, 51, 51, 63, 63, 51, 51, 63, 63},
	//9
	{63, 63, 51, 51, 63, 63, 3, 3, 3, 3},
};

static const uint8 divoom_disp_digit_more_big_disp_info[11][13] = {
	{126, 129, 129, 129, 129, 129, 0, 129, 129, 129, 129, 129, 126},
	{0, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2, 2, 0},
	{126, 1, 1, 1, 1, 1, 126, 128, 128, 128, 128, 128, 126},
	{126, 1, 1, 1, 1, 1, 126, 1, 1, 1, 1, 1, 126},
	{0, 130, 130, 130, 130, 130, 124, 2, 2, 2, 2, 2, 0},
	{126, 128, 128, 128, 128, 128, 126, 1, 1, 1, 1, 1, 126},
	{126, 128, 128, 128, 128, 128, 126, 129, 129, 129, 129, 129, 126},
	{126, 129, 129, 129, 129, 129, 0, 1, 1, 1, 1, 1, 0},
	{126, 129, 129, 129, 129, 129, 126, 129, 129, 129, 129, 129, 126},
	{126, 129, 129, 129, 129, 129, 126, 1, 1, 1, 1, 1, 126},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static const uint8 divoom_disp_digit_8X14_disp_info[11][14] = {
	{126, 255, 195, 195, 195, 195, 195, 195, 195, 195, 195, 195, 255, 126},
	{24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24},
	{126, 255, 195, 3, 3, 3, 127, 254, 192, 192, 192, 195, 255, 126},
	{254, 255, 3, 3, 3, 3, 255, 255, 3, 3, 3, 3, 255, 254},
	{195, 195, 195, 195, 195, 195, 255, 255, 3, 3, 3, 3, 3, 3},
	{254, 255, 195, 192, 192, 192, 254, 127, 3, 3, 3, 195, 255, 126},
	{126, 255, 195, 192, 192, 192, 254, 255, 195, 195, 195, 195, 255, 126},
	{255, 255, 195, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
	{126, 255, 195, 195, 195, 195, 255, 255, 195, 195, 195, 195, 255, 126},
	{126, 255, 195, 195, 195, 195, 255, 255, 3, 3, 3, 255, 255, 126},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static const uint8 divoom_disp_digit_6X12_disp_info[11][12] = {
	{30, 63, 51, 51, 51, 51, 51, 51, 51, 51, 63, 30},
	{6, 14, 22, 6, 6, 6, 6, 6, 6, 6, 6, 6},
	{30, 63, 51, 51, 3, 3, 7, 14, 28, 56, 63, 63},
	{30, 63, 51, 3, 3, 30, 31, 3, 3, 51, 63, 30},
	{51, 51, 51, 51, 51, 51, 63, 31, 3, 3, 3, 3},
	{31, 63, 48, 48, 62, 63, 3, 3, 3, 51, 63, 30},
	{30, 63, 51, 48, 48, 62, 63, 51, 51, 51, 63, 30},
	{63, 63, 3, 3, 7, 6, 6, 12, 12, 28, 24, 24},
	{30, 63, 51, 51, 51, 30, 63, 51, 51, 51, 63, 30},
	{30, 63, 51, 51, 51, 63, 31, 3, 3, 51, 63, 30},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static const uint8 divoom_disp_digit_for_clock_num_disp_info[11][9] = {
	{127,65,65,65,65,65,65,65,127},
	{8,120,8,8,8,8,8,8,127},
	{127,1,1,1,127,64,64,64,127},
	{127,1,1,1,127,1,1,1,127},
	{65,65,65,65,127,1,1,1,1},
	{127,64,64,64,127,1,1,1,127},
	{127,64,64,64,127,65,65,65,127},
	{127,1,1,1,1,1,1,1,1},
	{127,65,65,65,127,65,65,65,127},
	{127,65,65,65,127,1,1,1,127},
	{0,0,0,0,0,0,0,0,0},
};

//5X7
static const uint8 divoom_disp_digit_5X7_disp_info[11][7] = {
	{0x0E, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x0E},
	{0x06, 0x0E, 0x06, 0x06, 0x06, 0x06, 0x1F},
	{0x0E, 0x13, 0x13, 0x03, 0x0E, 0x19, 0x1F},
	{0x1E, 0x1B, 0x13, 0x06, 0x13, 0x1B, 0x1F},
	{0x1B, 0x1B, 0x1B, 0x1F, 0x1F, 0x03, 0x03},
	{0x1F, 0x19, 0x1E, 0x01, 0x11, 0x1F, 0x0E},
	{0x1F, 0x19, 0x10, 0x1F, 0x1B, 0x1B, 0x1F},
	{0x1F, 0x1B, 0x13, 0x03, 0x03, 0x03, 0x03},
	{0x0E, 0x1B, 0x1B, 0x1F, 0x1B, 0x1B, 0x0E},
	{0x1F, 0x1B, 0x1B, 0x1F, 0x03, 0x1B, 0x1F},
	{0, 0, 0, 0, 0, 0, 0},
};

//4X13,斜体,需要算法把阴影部分整理出来
static const uint8 divoom_disp_digit_4X13_disp_info[11][13] = {
	{0x08, 0x0C, 0x0A, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0D, 0x07, 0x03, 0x01},
	{0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00},
	{0x08, 0x0C, 0x06, 0x03, 0x09, 0x0D, 0x0F, 0x0B, 0x09, 0x0C, 0x06, 0x03, 0x01},
	{0x08, 0x0C, 0x06, 0x03, 0x09, 0x0D, 0x07, 0x03, 0x09, 0x0D, 0x07, 0x03, 0x01},
	{0x08, 0x08, 0x08, 0x09, 0x09, 0x0D, 0x07, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01},
	{0x08, 0x0C, 0x0E, 0x0B, 0x09, 0x0C, 0x06, 0x03, 0x09, 0x0D, 0x07, 0x03, 0x01},
	{0x08, 0x0C, 0x0E, 0x0B, 0x09, 0x0C, 0x0E, 0x0B, 0x09, 0x0D, 0x07, 0x03, 0x01},
	{0x08, 0x0C, 0x06, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
	{0x08, 0x0C, 0x0E, 0x0B, 0x09, 0x0D, 0x0F, 0x0B, 0x09, 0x0D, 0x07, 0x03, 0x01},
	{0x08, 0x0C, 0x0E, 0x0B, 0x09, 0x0D, 0x07, 0x03, 0x09, 0x0D, 0x07, 0x03, 0x01},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

//8X14,双条线,区别于计分板的
static const uint8 divoom_disp_digit_8X14_big_disp_info[11][14] = {
	{60, 60, 195, 195, 195, 195, 0, 0, 195, 195, 195, 195, 60, 60},
	{3, 3, 3, 3, 3, 3, 0, 0, 3, 3, 3, 3, 3, 3},
	{60, 60, 3, 3, 3, 3, 60, 60, 192, 192, 192, 192, 60, 60},
	{60, 60, 3, 3, 3, 3, 60, 60, 3, 3, 3, 3, 60, 60},
	{195, 195, 195, 195, 195, 195, 60, 60, 3, 3, 3, 3, 3, 3},
	{60, 60, 192, 192, 192, 192, 60, 60, 3, 3, 3, 3, 60, 60},
	{60, 60, 192, 192, 192, 192, 60, 60, 195, 195, 195, 195, 60, 60},
	{252, 252, 3, 3, 3, 3, 0, 0, 3, 3, 3, 3, 3, 3},
	{60, 60, 195, 195, 195, 195, 60, 60, 195, 195, 195, 195, 60, 60},
	{60, 60, 195, 195, 195, 195, 60, 60, 3, 3, 3, 3, 60, 60},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static const divoom_disp_digit_info divoom_disp_digit_modle[] = {
    {   
        3,
        5,
        (uint8 *)divoom_disp_digit_small_disp_Info,
    },
	{
		3,
		7,
		(uint8 *)divoom_disp_dight_3X7_disp_info,
	},
    {
        4,
        7,
        (uint8 *)divoom_disp_digit_big_disp_Info,
    },
	{
		//DIVOOM_5X7_DIGIT,
		5,
		7,
		(uint8 *)divoom_disp_digit_5X7_disp_info,
	},
	{
		6,
		10,
		(uint8 *)divoom_disp_digit_more_disp_info,
	},
	{
		6,
		11,
		(uint8 *)divoom_disp_digit_6X11_disp_info
	},
	{
		7,
		9,
		(uint8 *)divoom_disp_digit_for_clock_num_disp_info,
	},
	{
		8,
		13,
		(uint8 *)divoom_disp_digit_more_big_disp_info,
	},
	{
		6,
		10,
		(uint8 *)divoom_disp_digit_fang_disp_info,
	},
	{
		8,
		14,
		(uint8 *)divoom_disp_digit_8X14_disp_info,
	},
	{
		6,
		12,
		(uint8 *)divoom_disp_digit_6X12_disp_info,
	},
	{
		//DIVOOM_13X4_ITALIC,
		4,
		13,
		(uint8 *)divoom_disp_digit_4X13_disp_info,
	},
	{
		//DIVOOM_14X8_DIGIT,
		8,
		14,
		(uint8 *)divoom_disp_digit_8X14_big_disp_info,
	},
};

static const divoom_disp_digit_info divoom_disp_digit_shadow_modle[] = {
    {   
        3,
        6,
        (uint8 *)divoom_disp_digit_shadow_3x5_disp_Info,
    },
    {
        4,
        8,
        (uint8 *)divoom_disp_digit_shadow_4x7_disp_Info,
    },
	{
		//4X7
	},
	{
		//5X7
	},
	{
		//6X10
	},
	{
		//6X11
	},
	{
		//7X9
	},
	{
		//8X13
	},
	{
		//6X10 fang
		6,
		11,
		(uint8 *)divoom_disp_digit_fang_shadow_disp_info,
	},
	{
		8,
		15,
		(uint8 *)divoom_disp_digit_8X14_shadow_disp_info,
	},
	{
		6,
		13,
		(uint8 *)divoom_disp_digit_6X12_shadow_disp_info,
	},
	{
		//13X4
	},
	{
		//14X8
	},
};


/*****************************************************************************
 Prototype    : divoom_disp_digit_fill_area
 Description  : 填写一个数字
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y     
                uint8 user_data            数字 0-9     
                uint8 type                 字体大小类型     DIVOOM_3X5_DIGIT   DIVOOM_4X7_DIGIT
                uint8 *disp_buf            要填的buf     
                Pdivoom_screen_color_info color      字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_digit_fill_area(uint8 start_x, uint8 start_y, uint8 user_data, 
        uint8 type, uint8 *disp_buf, const divoom_screen_color_info * color)
{
    uint8 i, j, tmp = 0;
    uint8 size_x = divoom_disp_digit_modle[type].x, size_y = divoom_disp_digit_modle[type].y;
    uint8 *digit_info = divoom_disp_digit_modle[type].arr;

//    uint8 *digit_info;    
    user_data %= 10;
//    digit_info = divoom_disp_digit_modle[type].arr + size_y * user_data;    
    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            tmp = (*(digit_info + size_y * user_data + i)) & (1 << (size_x-j-1));
            if(tmp)            
            {
                if(color != NULL)
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, color, disp_buf);
                }
            }
        }
    }    
}

/** 点阵格式：阴码；取模方式：逐行式；取模走向：逆向（低位在前） **/
void divoom_disp_font_fill_any_area(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, const uint8 *digit_info, 
        uint8 *disp_buf, Pdivoom_screen_color_info color)
{
    uint8 i, j, tmp = 0;
	uint8 line_size = size_x/8;
	if(size_x%8 != 0)
	{
		line_size++;
	}

    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            tmp = (*(digit_info + i* line_size + j/8)) & (1 << (j%8));            
            if(tmp)            
            {
                if(color != NULL)
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, color, disp_buf);
                }
            }
        }
    }
}



void divoom_disp_digit_fill_area_with_bit(uint8 start_x, uint8 start_y, uint8 user_data, uint8 *disp_buf, uint16 start_dot, uint16 end_dot)
{
    uint8 i, j, tmp = 0;
    uint16 off, m;
    uint8 size_x = divoom_disp_digit_modle[DIVOOM_8X14_BIG_DIGIT].x, size_y = divoom_disp_digit_modle[DIVOOM_8X14_BIG_DIGIT].y;
    uint8 *digit_info = divoom_disp_digit_modle[DIVOOM_8X14_BIG_DIGIT].arr;
 
    user_data %= 10;
    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            tmp = (*(digit_info + size_y * user_data + i)) & (1 << (size_x-j-1));
            if(tmp)            
            {
               off =  ((start_x + j)<<1) + ((start_y+ i)<< 1)*DIVOOM_IMAGE_HEIGHT_64;
               for(m = 0; m < 2; m++)
               {
                    if(off >= start_dot && off < end_dot)
                    {
                        DIVOOM_GBL_SYSTEM_SET_BIT(disp_buf,off - start_dot);
                        DIVOOM_GBL_SYSTEM_SET_BIT(disp_buf,off - start_dot + 1);
                    }
                    off += DIVOOM_IMAGE_HEIGHT_64;
               }
                    
            }
        }
    }    
}


/*****************************************************************************
 Prototype    : divoom_disp_digit_fill_area_shadow
 Description  : 填写一个数字,包含有阴影，要求字模最后一行全为0
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y     
                uint8 user_data            数字 0-9     
                uint8 type                 字体大小类型     DIVOOM_3X5_DIGIT   DIVOOM_4X7_DIGIT
                uint8 *disp_buf            要填的buf     
                Pdivoom_screen_color_info color      字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/11/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_digit_fill_area_shadow(uint8 start_x, uint8 start_y, uint8 user_data, 
        uint8 type, uint8 *disp_buf, Pdivoom_screen_color_info color)
{
    uint8 i, j, tmp = 0,  tmp_up = 0;
    uint8 size_x = divoom_disp_digit_shadow_modle[type].x, size_y = divoom_disp_digit_shadow_modle[type].y;
    uint8 *digit_info = divoom_disp_digit_shadow_modle[type].arr;

//    uint8 *digit_info;    
    user_data %= 10;
//    digit_info = divoom_disp_digit_modle[type].arr + size_y * user_data;    
    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            tmp = (*(digit_info + size_y * user_data + i)) & (1 << (size_x-j-1));
            if(tmp)            
            {
                if(color != NULL)
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, color, disp_buf);
                }
            }
            else
            {
                if(i != 0)
                    tmp_up = (*(digit_info + size_y * user_data + i - 1)) & (1 << (size_x-j-1));
                if(tmp_up)
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, &divoom_color_black, disp_buf);
                }
            }              
        }
    }    
}



/*****************************************************************************
 Prototype    : divoom_disp_font_fill_area
 Description  : 画出字库      
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y                   
                uint8 size_x               字库宽               注意程序限制，size_x小于8             
                uint8 size_y               字库高     
                uint8 *digit_info          字库数组地址         
                uint8 *disp_buf            要填的buf    
                Pdivoom_screen_color_info color    字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/30
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_font_fill_area(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, uint8 *digit_info, 
        uint8 *disp_buf, Pdivoom_screen_color_info color)
{
    uint8 i, j, tmp = 0;
    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            tmp = (*(digit_info + i)) & (1 << (size_x-j-1));            
            if(tmp)            
            {
                if(color != NULL)
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, color, disp_buf);
                }
            }
        }
    }
}


/*****************************************************************************
 Prototype    : divoom_disp_font_fill_area_shadow
 Description  : 画出含有阴影字库      ，要求字模最后一行全为0
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y                   
                uint8 size_x               字库宽               注意程序限制，size_x小于8             
                uint8 size_y               字库高     
                uint8 *digit_info          字库数组地址         
                uint8 *disp_buf            要填的buf    
                Pdivoom_screen_color_info color    字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/11/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_font_fill_area_shadow(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, uint8 *digit_info, 
        uint8 *disp_buf, Pdivoom_screen_color_info color)
{
    uint8 i, j, tmp = 0, tmp_up = 0;
    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            tmp = (*(digit_info + i)) & (1 << (size_x-j-1));            
            if(tmp)            
            {
                if(color != NULL)
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, color, disp_buf);
                }
            }
            else
            {
                if(i != 0)
                    tmp_up = (*(digit_info + i - 1)) & (1 << (size_x-j-1));     
                if(tmp_up)
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, &divoom_color_black, disp_buf);
                }
            }    
        }
    }
}

void divoom_disp_watch_set_pixel(uint16 pos, uint8 *data, const divoom_screen_color_info * color)
{
    if(pos >= SCREEN_WIDTH_32 * SCREEN_HEIGHT_32)
    {
        divoom_print("divoom_disp_watch_set_pixel err at: %d\n", pos);
    }
    divoom_disp_ctrl_set_data(pos, color, data);
}

const uint8 TimeInfoStr[10][4*7] = {
{1,1,1,1,
 1,0,0,1,
 1,0,0,1,
 1,0,0,1,
 1,0,0,1,
 1,0,0,1,
 1,1,1,1},

{0,0,0,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1},

{1,1,1,1,
 0,0,0,1,
 0,0,0,1,
 1,1,1,1,
 1,0,0,0,
 1,0,0,0,
 1,1,1,1},

{1,1,1,1,
 0,0,0,1,
 0,0,0,1,
 1,1,1,1,
 0,0,0,1,
 0,0,0,1,
 1,1,1,1},

{1,0,0,1,
 1,0,0,1,
 1,0,0,1,
 1,1,1,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1},

{1,1,1,1,
 1,0,0,0,
 1,0,0,0,
 1,1,1,1,
 0,0,0,1,
 0,0,0,1,
 1,1,1,1},

{1,1,1,1,
 1,0,0,0,
 1,0,0,0,
 1,1,1,1,
 1,0,0,1,
 1,0,0,1,
 1,1,1,1},

{1,1,1,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1},

{1,1,1,1,
 1,0,0,1,
 1,0,0,1,
 1,1,1,1,
 1,0,0,1,
 1,0,0,1,
 1,1,1,1},

{1,1,1,1,
 1,0,0,1,
 1,0,0,1,
 1,1,1,1,
 0,0,0,1,
 0,0,0,1,
 0,0,0,1},
};

void divoom_disp_watch_digit(uint8 x, uint8 y, uint8 width, uint8 height, uint8 digit,uint8 * screen_buf, const divoom_screen_color_info *color)
{
    uint8 i, j, off, dight_off = 0;
    uint8 const * buf = TimeInfoStr[digit];
    
    off = y * SCREEN_WIDTH_32;
    for(i = 0; i < height; i ++, off += SCREEN_WIDTH_32, dight_off += width)
    {
        for(j = 0; j < width; j ++)
        {
            if(buf[dight_off + j])
            {
                divoom_disp_watch_set_pixel(off + x + j,screen_buf, color);
            }
        }
    }            
}






































#if 0
void divoom_disp_ctrl_clr_screen(void)
{
//    divoom_image_disp_stop();
}


/*********************************************************************
  Function:        divoom_disp_ctrl_fill
  Description:    将显示数据清屏成什么某一颜色
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/


void divoom_disp_ctrl_fill(uint8 *disp_buf, const divoom_screen_color_info * color)
{
    uint16 off;

    for(off = 0; off < SCREEN_HEIGHT_32 * SCREEN_WIDTH_32; off ++)
    {
        divoom_disp_ctrl_set_data(off, color, disp_buf);
    }
}


/*********************************************************************
  Function:        divoom_disp_ctrl_get_data
  Description:    从显示缓冲区中得到某个点的RGB
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/


void divoom_disp_ctrl_get_data(uint16 off, Pdivoom_screen_color_info color, uint8 *disp_buf)
{
    off = (off << 1) + off;
    color->r_c = disp_buf[off];
    color->g_c = disp_buf[off+1];
    color->b_c = disp_buf[off+2];    
}


/*********************************************************************
  Function:        divoom_disp_ctrl_set_data
  Description:   设置某个点的RGB
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/


void divoom_disp_ctrl_set_data(uint16 off,  const divoom_screen_color_info *  color, uint8 *disp_buf)
{
    
    off = (off << 1) + off;
    disp_buf[off] = color->r_c;
    disp_buf[off+1] = color->g_c;
    disp_buf[off+2] = color->b_c;    
}


/*********************************************************************
  Function:        divoom_disp_ctrl_set_dot
  Description:   设置某个点的RGB
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_set_dot(uint8 x, uint8 y, const divoom_screen_color_info *pcolor, uint8 *disp_buf)
{
    uint16 off;
//    if(x < 0 || y < 0 || x >= SCREEN_WIDTH_32 || y >= SCREEN_HEIGHT_32)
    if(x >= SCREEN_WIDTH_32 || y >= SCREEN_HEIGHT_32)        
        return;
    off = (y * SCREEN_WIDTH_32 + x);
    off = (off << 1) + off;
    disp_buf[off] = pcolor->r_c;
    disp_buf[off+1] = pcolor->g_c;
    disp_buf[off+2] = pcolor->b_c;
    
}


/*********************************************************************
  Function:        divoom_disp_ctrl_get_dot
  Description:    从显示缓冲区中得到某个点的RGB
  Input:            X    
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_get_dot(uint8 x, uint8 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf)
{
    uint16 off;

    off = (y * SCREEN_WIDTH_32 + x);
    off = (off << 1) + off;
    pcolor->r_c = disp_buf[off];
    pcolor->g_c = disp_buf[off+1];
    pcolor->b_c = disp_buf[off+2];
    
}


/**
 * @function:    divoom_disp_ctrl_set_data_xy
 * @description: 设置(x,y)点的RGB
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        x              [x坐标]
 * @param        y              [y坐标]
 * @param        pcolor         [颜色缓冲区]
 * @param        disp_buf       [显示缓冲区]
 */
void divoom_disp_ctrl_set_data_xy(int16 x,int16 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf)
{
    uint16 off;
    if(x < 0 || y < 0 || x >= SCREEN_WIDTH_32 || y >= SCREEN_HEIGHT_32)
        return;
    // divoom_disp_ctrl_set_data(x + y * SCREEN_WIDTH_32,pcolor,disp_buf);
//    divoom_disp_ctrl_set_data(x + (y << 4),pcolor,disp_buf); 
    off = (y * SCREEN_WIDTH_32 + x);
    off = (off << 1) + off;
    disp_buf[off] = pcolor->r_c;
    disp_buf[off+1] = pcolor->g_c;
    disp_buf[off+2] = pcolor->b_c;    
}



void divoom_disp_ctrl_set_mixer_off(void)
{
	if(pdivoom_disp_ctrl_handle)
		memset(pdivoom_disp_ctrl_handle->mix_data_buf, 0xff, sizeof(pdivoom_disp_ctrl_handle->mix_data_buf));
}

uint8 divoom_disp_ctrl_change_bright(uint8 flag)
{
    if(flag)
    {
        if(pdivoom_disp_ctrl_handle->sys_light < 90)
        {
            pdivoom_disp_ctrl_handle->sys_light += 10;
        }
        else
        {
            pdivoom_disp_ctrl_handle->sys_light = 100;
            return false;
        }
    }
    else
    {
        if(pdivoom_disp_ctrl_handle->sys_light > 10)
        {
            pdivoom_disp_ctrl_handle->sys_light -= 10;
        }
        else
        {
            pdivoom_disp_ctrl_handle->sys_light = 0;
            return false;
        }
    }
    divoom_disp_ctrl_set_light(pdivoom_disp_ctrl_handle->sys_light);
    return true;
}
#endif






