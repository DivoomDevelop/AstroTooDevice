#include "divoom.h"

#define DIVOOM_SLEEP_TRY_LISTEN_TIME 60
#define DIVOOM_SLEEP_PREIOD_TIME 60
#define DIVOOM_SLEEP_TURN_OFF_TIME 60
#define DIVOOM_SLEEP_CYC_TIME		0xffffffff

/* 睡眠场景索引 -> divoom_voice_read 使用的 VoiceID（与 divoom_voice_read.h 中枚举一致） */
static const uint32 divoom_sleep_voice_ids[] = {
	DIVOOM_VOICE_ID_SLEEP_COUNTRYSIDE,
	DIVOOM_VOICE_ID_SLEEP_SEASHORE,
	DIVOOM_VOICE_ID_SLEEP_WATER_DROP,
	DIVOOM_VOICE_ID_SLEEP_MUSIC_BOX,
	DIVOOM_VOICE_ID_SLEEP_CAMPFIRE,
	DIVOOM_VOICE_ID_SLEEP_MEDITATE,
	DIVOOM_VOICE_ID_SLEEP_THUNDER,
	DIVOOM_VOICE_ID_SLEEP_NIGHT_FALL,
	DIVOOM_VOICE_ID_SLEEP_MIDSUMMER,
	DIVOOM_VOICE_ID_SLEEP_ROSA_CREEK,
	DIVOOM_VOICE_ID_SLEEP_UNDER_WATER,
	DIVOOM_VOICE_ID_SLEEP_RELAX,
	DIVOOM_VOICE_ID_SLEEP_BIRD_CHIRPING,
	DIVOOM_VOICE_ID_SLEEP_FROGS,
	DIVOOM_VOICE_ID_SLEEP_LULLABY,
	DIVOOM_VOICE_ID_SLEEP_CITY_RAIN,
	DIVOOM_VOICE_ID_SLEEP_DIVING,
	DIVOOM_VOICE_ID_SLEEP_GARDEN,
	DIVOOM_VOICE_ID_SLEEP_WHITE_NOISE,
	DIVOOM_VOICE_ID_SLEEP_WHISPER,
	DIVOOM_VOICE_ID_SLEEP_SNOWSTORM,
	DIVOOM_VOICE_ID_SLEEP_SUMMER,
};
#define DIVOOM_SLEEP_VOICE_ID_NUM (sizeof(divoom_sleep_voice_ids) / sizeof(divoom_sleep_voice_ids[0]))


//先保留
#define DIVOOM_MUSIC_MAX_VOL		15

typedef struct _divoom_sleep_ctrl
{
    uint8 status;
    uint8 play_status;
    uint8 update_flag;
    //下面是网络数据
    uint8 enable;							//close,open
    divoom_light_color_info color;			//color of display
    int light;								//亮度
    int sleep_time;
    time_t end_time;
    uint8 app_vol;
    uint8 cur_vol;
    uint8 music_type;
	uint8 mode;								//0->power off or sleep no 1->display
}divoom_sleep_ctrl, *Pdivoom_sleep_ctrl;


static Pdivoom_sleep_ctrl pdivoom_sleep_handle = NULL;
void divoom_sleep_check_test_mode(uint8 status);

uint8 divoom_sleep_get_music_type(uint8 bell_type, uint8 music_type, uint16 *voice_id);
void divoom_sleep_active_callback(void);
void divoom_sleep_save(void);
void divoom_sleep_load(void);

#ifdef DIVOOM_SLEEP_TEST_USE
void divoom_sleep_test_init(void);
#endif


void divoom_sleep_disp_test_ctrl(Pdivoom_disp_sleep_ctrl sleep_info)
{
    divoom_sys_msg_info model_msg = {0};
    model_msg.src_id = DIVOOM_MODEL_SLEEP;
    //model_msg.attr = DIVOOM_LIGHT_APP_CTRL_NORMAL;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
	//根据是16*16,还是16*48决定使用 DIVOOM_LIGHT_APP_EVENT_SMALL_DRAW 或者 DIVOOM_LIGHT_APP_EVENT_BIG_PLAY;
	//model_msg.msg_id = DIVOOM_DISP_SLEEP_MODE;
	model_msg.data = (uint8 *)sleep_info;
    divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_sleep_play_voice(uint8 id, uint8 volume, uint8 cyc_flag, divoom_local_music_ctrl_music_play_get_next pcallback)
{
	char url_info[128];
	uint32 play_time = 0;

	if (cyc_flag)
		play_time = DIVOOM_SLEEP_CYC_TIME;

	if (id >= DIVOOM_SLEEP_VOICE_ID_NUM)
		id = 0;
	divoom_voice_read_get_filename(divoom_sleep_voice_ids[id], url_info);
	divoom_local_music_ctrl_tone_play(url_info, volume, NULL, play_time);
}

void divoom_sleep_disp_ctrl(uint8 ctrl_flag, uint8 flag)
{
    divoom_disp_sleep_ctrl sleep_info;

    if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_STANDBY)
    {
        return;
    }
    
    if(ctrl_flag == DIVOOM_SLEEP_STATUS_STANDBY)
    {
//        divoom_screen_color_info rgb_color = {0};
//        divoom_local_music_stop();
//        divoom_mix_screen_oneline_led_init_screen(&rgb_color);
//        divoom_mix_screen_led_seg_switch(AK_TRUE);
		return;
    }
    else if(ctrl_flag == DIVOOM_SLEEP_STATUS_WAITING)
    {
        sleep_info.mode = pdivoom_sleep_handle->mode;
        sleep_info.light = pdivoom_sleep_handle->light;
        sleep_info.color.r_c = pdivoom_sleep_handle->color.r_c;
        sleep_info.color.g_c = pdivoom_sleep_handle->color.g_c;
        sleep_info.color.b_c = pdivoom_sleep_handle->color.b_c;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_SLEEP_MODE, (T_VOID *)&sleep_info);
    	//播放一个声音
    	if(flag)
    	{
            divoom_sleep_play_voice(pdivoom_sleep_handle->music_type, pdivoom_sleep_handle->app_vol, 1, NULL);
    	}
        else
        {
			divoom_local_music_ctrl_set_tone_volume(pdivoom_sleep_handle->app_vol);		
        }
    }
    else  if(ctrl_flag == DIVOOM_SLEEP_STATUS_SLEEP)
    {
        if(pdivoom_sleep_handle->status != DIVOOM_SLEEP_STATUS_SLEEP)
        {
			divoom_local_music_ctrl_stop_tone();
        }
        sleep_info.mode = pdivoom_sleep_handle->mode;
        if(flag >= DIVOOM_SLEEP_TURN_OFF_TIME)
        {
            sleep_info.light = 0;
        }
        else
        {
            sleep_info.light = (DIVOOM_SLEEP_TURN_OFF_TIME - flag) * pdivoom_sleep_handle->light / DIVOOM_SLEEP_TURN_OFF_TIME;
        }
        sleep_info.color.r_c = pdivoom_sleep_handle->color.r_c;
        sleep_info.color.g_c = pdivoom_sleep_handle->color.g_c;
        sleep_info.color.b_c = pdivoom_sleep_handle->color.b_c;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_SLEEP_MODE, (T_VOID *)&sleep_info);
    }
    else  if(ctrl_flag == DIVOOM_SLEEP_STATUS_TEST_MODE)
    {
        
    }

}


void divoom_sleep_cancel(void)
{
	divoom_app_schedule_item_info event_info = {0};

    pdivoom_sleep_handle->enable = 0;
    if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_STANDBY)
    {
        return;
    }
    pdivoom_sleep_handle->enable = 0;
    event_info.model_id = DIVOOM_MODEL_SLEEP;
    event_info.event_id = DIVOOM_SLEEP_APP_EVENT_WAITING;
    event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
    event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    divoom_app_schedule_cancel(&event_info);
    divoom_sleep_disp_ctrl(DIVOOM_SLEEP_STATUS_STANDBY, 0);
    pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_STANDBY;
}

void divoom_sleep_switch(int status);

void divoom_sleep_update_info(Pdivoom_system_tool_json_get_sleep pctrl_info)
{
    uint8 start_play = 0;
	divoom_app_schedule_item_info event_info = {0};
    time_t cur_time;

    time(&cur_time);

    if(pctrl_info->status == 0)
    {
        divoom_sleep_switch(0);
        return;
    }
	pdivoom_sleep_handle->mode = pctrl_info->mode;

    if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_STANDBY || pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_TEST_MODE)
    {
        event_info.model_id = DIVOOM_MODEL_SLEEP;
        event_info.event_id = DIVOOM_SLEEP_APP_EVENT_WAITING;
        event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
        event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        if(0 == divoom_app_schedule_push(&event_info))
        {
			pdivoom_sleep_handle->enable = 0;
            return;
        }
        pdivoom_sleep_handle->play_status = 1;
        start_play = 1;
    }

    
	//从关到开的状态更新时间
	if((pctrl_info->time != 0 && pctrl_info->time != pdivoom_sleep_handle->sleep_time) || (pctrl_info->status != pdivoom_sleep_handle->enable))
	{
		pdivoom_sleep_handle->sleep_time = pctrl_info->time;
		pdivoom_sleep_handle->end_time = cur_time + pdivoom_sleep_handle->sleep_time * DIVOOM_SLEEP_PREIOD_TIME;
	}
    divoom_print("%s: time: %d, %d, end_time:%lld\n", __func__, (int)pctrl_info->time, (int)pdivoom_sleep_handle->sleep_time, (long long)pdivoom_sleep_handle->end_time);
    
	if(pctrl_info->scene != -1 && pdivoom_sleep_handle->music_type != pctrl_info->scene)
	{
        pdivoom_sleep_handle->music_type = pctrl_info->scene;
        start_play = 1;
	}
	if(pctrl_info->vol != -1 )
    {
        pdivoom_sleep_handle->app_vol = pctrl_info->vol;
    }

    if(pctrl_info->light != -1)
    {
        pdivoom_sleep_handle->light = pctrl_info->light;
    }

    if(pctrl_info->color_flag)
    {
        pdivoom_sleep_handle->color.r_c = pctrl_info->color[0];
        pdivoom_sleep_handle->color.g_c = pctrl_info->color[1];
        pdivoom_sleep_handle->color.b_c = pctrl_info->color[2];
    }
    
    pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_WAITING;
    divoom_sleep_disp_ctrl(DIVOOM_SLEEP_STATUS_WAITING, start_play);
//    divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_SLEEP_START, DIVOOM_SERVER_TOPIC, &pdivoom_sleep_handle->sleep_time);
	pdivoom_sleep_handle->enable = 1;
}


void divoom_sleep_switch(int status)
{
    divoom_app_schedule_item_info event_info = {0};
    
    if(status)
    {
        pdivoom_sleep_handle->enable = 1;
        //先加载睡眠信息
        if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_STANDBY || pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_TEST_MODE)
        {
            time_t cur_time;

            time(&cur_time);
            event_info.model_id = DIVOOM_MODEL_SLEEP;
            event_info.event_id = DIVOOM_SLEEP_APP_EVENT_WAITING;
			event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
			event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
			//睡眠遇到留言播放和闹钟还是推迟响应吧
            if(0 == divoom_app_schedule_push(&event_info))
            {
                return;
            }

            pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_WAITING;
            pdivoom_sleep_handle->end_time = cur_time + pdivoom_sleep_handle->sleep_time * DIVOOM_SLEEP_PREIOD_TIME;
            pdivoom_sleep_handle->play_status = 1;
            divoom_sleep_disp_ctrl(pdivoom_sleep_handle->status, 1);
//            divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_SLEEP_START, DIVOOM_SERVER_TOPIC, &pdivoom_sleep_handle->sleep_time);
        }
    }
    else
    {
        divoom_sleep_cancel();
        pdivoom_sleep_handle->enable = 0;
//        divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_SLEEP_STOP, DIVOOM_SERVER_TOPIC, &pdivoom_sleep_handle->sleep_time);
    }
}

uint8 divoom_sleep_get_run_status(void)
{
    if(pdivoom_sleep_handle && pdivoom_sleep_handle->status != DIVOOM_SLEEP_STATUS_STANDBY)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void divoom_sleep_active_callback(void)
{
    //停止声音，关闭显示，进入睡眠状态
	divoom_print("%s:%d", __func__, __LINE__);
    if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_WAITING)
    {    
        divoom_sleep_disp_ctrl(DIVOOM_SLEEP_STATUS_SLEEP,  0);
        pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_SLEEP;
    }
    return ;
}

uint8 divoom_sleep_is_available(void)
{
    return pdivoom_sleep_handle != NULL;
}

void divoom_sleep_get(Pdivoom_system_tool_json_get_sleep pctrl_info)
{
	if(pctrl_info == NULL)
	{
		return;
	}
	memset(pctrl_info, 0, sizeof(*pctrl_info));
	if(pdivoom_sleep_handle == NULL)
	{
		return;
	}
	pctrl_info->status = (pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_WAITING);
    pctrl_info->time = pdivoom_sleep_handle->sleep_time;
    pctrl_info->color[0] = pdivoom_sleep_handle->color.r_c;
    pctrl_info->color[1] = pdivoom_sleep_handle->color.g_c;
    pctrl_info->color[2] = pdivoom_sleep_handle->color.b_c;
    pctrl_info->light = pdivoom_sleep_handle->light;
    pctrl_info->vol = pdivoom_sleep_handle->app_vol;
    pctrl_info->scene = pdivoom_sleep_handle->music_type;
    pctrl_info->mode = pdivoom_sleep_handle->mode;
}

void divoom_sleep_check_test_mode(uint8 status)
{
    divoom_app_schedule_item_info event_info = {0};
    
    event_info.model_id = DIVOOM_MODEL_SLEEP;
	divoom_print("[%s]Now exit test mode!\n", __FUNCTION__);
    if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_TEST_MODE)
    {
        //divoom_sleep_disp_ctrl(DIVOOM_SLEEP_STATUS_STANDBY, 0);
        event_info.event_id = DIVOOM_SLEEP_APP_EVENT_TRY_LISTEN;
    	event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
        event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
		divoom_app_schedule_cancel(&event_info);
        pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_STANDBY;
    }
    return ;
}

uint8 divoom_sleep_event(Pdivoom_sys_msg_info msg)
{
//    divoom_app_schedule_item_info event_info = {0};
    
    if(pdivoom_sleep_handle == NULL)
    {
        return false;
    }
//    event_info.model_id = DIVOOM_MODEL_SLEEP;
    
    divoom_print("divoom_sleep_event:%d\n",msg->msg_id);
    switch(msg->msg_id)
    {
            
        case DIVOOM_SLEEP_TEST_MODE:
            pdivoom_sleep_handle->update_flag = 1;
            if(pdivoom_sleep_handle->status != DIVOOM_SLEEP_STATUS_WAITING)
            {
                Pdivoom_system_tool_json_test_sleep pctrl = (Pdivoom_system_tool_json_test_sleep)msg->data;
    			divoom_disp_sleep_ctrl sleep_info = {0};
                time_t cur_time;
                
                if(pdivoom_sleep_handle->enable)
                {
                    break;
                }
                if(pctrl->light)
                {
                    sleep_info.light = pctrl->light;
    				pdivoom_sleep_handle->light = pctrl->light;
//                    divoom_mix_screen_oneline_led_bright(pdivoom_sleep_handle->light);
                }
                if(pctrl->color_flag)
                {
                    sleep_info.color.r_c = pctrl->color[0];
                    sleep_info.color.g_c = pctrl->color[1];
                    sleep_info.color.b_c = pctrl->color[2];

    				memcpy(&pdivoom_sleep_handle->color, &sleep_info.color, sizeof(divoom_light_color_info));
//                    divoom_mix_screen_oneline_led_init_screen(&sleep_info.color);
                }
    			if(pctrl->vol)
    			{
    				pdivoom_sleep_handle->app_vol = pctrl->vol;
                    if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_TEST_MODE)
                    {
//                        divoom_local_music_volume(pdivoom_sleep_handle->app_vol);
                    }
    			}
                
    			divoom_print("sleep_info.light = %d, sleep_info.color:%d-%d-%d\n", sleep_info.light, sleep_info.color.r_c, sleep_info.color.g_c, sleep_info.color.b_c);
    			if(pctrl->scene != -1)
    			{
                    #if 0
        			//睡眠遇到留言播放和闹钟还是推迟响应吧
        			if(pdivoom_sleep_handle->music_type != pctrl->scene)
        			{
                        event_info.event_id = DIVOOM_SLEEP_APP_EVENT_TRY_LISTEN;
                    	event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
                    	event_info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
                        if(divoom_app_schedule_push(&event_info))
                        {
                            divoom_sleep_play_voice(pctrl->scene, pdivoom_sleep_handle->app_vol, 0, NULL);
                            pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_TEST_MODE;
                        }
                        pdivoom_sleep_handle->music_type = pctrl->scene;
        			}
                    #endif
    			}
                //
//                divoom_mix_screen_oneline_led_init_screen(&sleep_info.color);
//                divoom_mix_screen_oneline_led_bright(sleep_info.light);
                if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_TEST_MODE)
                {
                    time(&cur_time);
                    pdivoom_sleep_handle->end_time = cur_time + DIVOOM_SLEEP_TRY_LISTEN_TIME;
                }
                
            }
            break;
            
        case DIVOOM_SLEEP_EXIT_TEST_MODE:
			//divoom_time_cancel(divoom_sleep_check_test_mode);
            divoom_sleep_check_test_mode( 0);
            break;
            
        case DIVOOM_SLEEP_GET_STATUS:
            msg->result = pdivoom_sleep_handle->status;
            break;
            
        case DIVOOM_SLEEP_GET_INFO:
            divoom_sleep_get((Pdivoom_system_tool_json_get_sleep)msg->data);
            break;
            
        case DIVOOM_SLEEP_CANCEL:
			divoom_print("pdivoom_sleep_handle->status = %d\n", pdivoom_sleep_handle->status);
            divoom_sleep_cancel();
            break;
            
        case DIVOOM_SLEEP_SWITCH:
            divoom_sleep_switch(*((int *)msg->data));
            
            break;
            
        case DIVOOM_SLEEP_UPDATE:
            pdivoom_sleep_handle->update_flag = 1;
            divoom_sleep_update_info((Pdivoom_system_tool_json_get_sleep)msg->data);
            break;

        #if 1
        //case DIVOOM_APP_CONFIG_SAVE_EVENT_ID:
        //case DIVOOM_SLEEP_SAVE:
		case DIVOOM_APP_CONFIG_SAVE_EVENT_ID:
            if(pdivoom_sleep_handle->update_flag)
            {
                divoom_sleep_save();
				pdivoom_sleep_handle->update_flag = 0;
            }
            break;
        #endif
            
    }
    return true;
}


uint8 divoom_sleep_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{    
    return 0;
}


uint8 divoom_sleep_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
    //睡眠和试听都不能打断，让其自动调用destroy
    return 0;
}

//结束16*48,voice
uint8 divoom_sleep_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
    divoom_sleep_disp_ctrl(DIVOOM_SLEEP_STATUS_STANDBY, 0);
    pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_STANDBY;
	pdivoom_sleep_handle->enable = 0;
    return 1;
}

uint8 divoom_sleep_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
    divoom_sys_msg_info model_msg = {0};
	//TODO
	divoom_print("sleep status: %d\n", pdivoom_sleep_handle->status);
	if(pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_SLEEP || pdivoom_sleep_handle->status == DIVOOM_SLEEP_STATUS_WAITING)
	{
        //将取消睡眠
        divoom_sleep_cancel();
            //恢复默认亮度
        model_msg.src_id = DIVOOM_MODEL_SLEEP;  
        model_msg.dest_id = DIVOOM_MODEL_MAIN;          
        model_msg.msg_id = DIVOOM_MAIN_EVENT_RESTORE_SYS_LIGHT; 
        divoom_sys_model_crl_msg_push(&model_msg);
	    return 1;
	}
	return 0;
}

void divoom_sleep_init(void)
{
    pdivoom_sleep_handle = divoom_malloc( sizeof(divoom_sleep_ctrl), DIVOOM_MODEL_SLEEP, __LINE__,__FILE__);

    memset(pdivoom_sleep_handle, 0, sizeof(divoom_sleep_ctrl));
    divoom_sleep_load();
    pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_STANDBY;
    divoom_sys_model_crl_register(DIVOOM_MODEL_SLEEP,divoom_sleep_event);
}

void divoom_sleep_save(void)
{
    FILE *divoom_file_handle; 
    divoom_system_tool_json_get_sleep save_config;
    uint16 check_sum;

    
	save_config.status = 0;
	save_config.time = pdivoom_sleep_handle->sleep_time;
	save_config.color_flag = 0;
	save_config.color[0] = pdivoom_sleep_handle->color.r_c;
	save_config.color[1] = pdivoom_sleep_handle->color.g_c;
	save_config.color[2] = pdivoom_sleep_handle->color.b_c;
	save_config.light = pdivoom_sleep_handle->light;
	save_config.vol = pdivoom_sleep_handle->app_vol;
	save_config.mode = pdivoom_sleep_handle->mode;
	save_config.scene = pdivoom_sleep_handle->music_type;
    check_sum = divoom_app_com_get_check_sum((uint8 *)&save_config, sizeof(save_config));
    divoom_file_handle = divoom_port_file_open(DIVOOM_SLEEP_SPIFFS_CONFIG, "wb");
    if(divoom_file_handle == NULL)
    {
        divoom_print("%s: err save sleep config!\n",__func__);
        return ;
    }
    divoom_port_file_write(&save_config, sizeof(save_config), 1, divoom_file_handle);
    divoom_port_file_write(&check_sum, sizeof(check_sum), 1, divoom_file_handle);
    divoom_port_file_close(divoom_file_handle);

}


void divoom_sleep_load(void)
{
    divoom_system_tool_json_get_sleep save_config;
	FILE *divoom_load_handle;
    uint16 check_sum = 0, raw_check_sum = 0;

	divoom_load_handle = divoom_port_file_open(DIVOOM_SLEEP_SPIFFS_CONFIG, "rb");
	if(divoom_load_handle == NULL)
	{
        divoom_sleep_load_default_table:
        divoom_app_com_del_file(DIVOOM_SLEEP_SPIFFS_CONFIG);
        divoom_print("%s: load default sleep config!\n",__func__);
    	pdivoom_sleep_handle->sleep_time = 10;
    	pdivoom_sleep_handle->color.r_c = 255 ;
    	pdivoom_sleep_handle->color.g_c = 128;
    	pdivoom_sleep_handle->color.b_c = 0;
    	pdivoom_sleep_handle->light = 50;
    	pdivoom_sleep_handle->app_vol = 50;
    	pdivoom_sleep_handle->mode = 0;
        pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_STANDBY;
    	pdivoom_sleep_handle->music_type = 1;
		return;
	}

    if(divoom_app_com_check_check_sum(divoom_load_handle) == 0)
    {
		divoom_port_file_close(divoom_load_handle);
        divoom_print("config is err!\n");
        goto divoom_sleep_load_default_table;
    }
    if(0 == divoom_port_file_read(&save_config, sizeof(save_config), 1, divoom_load_handle))
    {
        divoom_port_file_close(divoom_load_handle);
        divoom_print("%s:it fails to checksum err!\n",__FUNCTION__);
        goto divoom_sleep_load_default_table;
    }
    if(0 == divoom_port_file_read(&raw_check_sum, sizeof(raw_check_sum), 1, divoom_load_handle))
    {
        divoom_port_file_close(divoom_load_handle);
        divoom_print("%s:it fails to checksum err!\n",__FUNCTION__);
        goto divoom_sleep_load_default_table;
    }
	divoom_port_file_close(divoom_load_handle);
    check_sum = divoom_app_com_get_check_sum((uint8 *)&save_config, sizeof(save_config));
    if(check_sum != raw_check_sum)
    {
        divoom_print("%s:it fails to checksum err!\n",__FUNCTION__);
        goto divoom_sleep_load_default_table;
    }
    
	pdivoom_sleep_handle->sleep_time = save_config.time;
	pdivoom_sleep_handle->color.r_c = save_config.color[0] ;
	pdivoom_sleep_handle->color.g_c = save_config.color[1];
	pdivoom_sleep_handle->color.b_c = save_config.color[2];
	pdivoom_sleep_handle->light = save_config.light;
	pdivoom_sleep_handle->app_vol = save_config.vol;
	pdivoom_sleep_handle->mode = save_config.mode;
    pdivoom_sleep_handle->status = DIVOOM_SLEEP_STATUS_STANDBY;
	pdivoom_sleep_handle->music_type = save_config.scene;
}

void divoom_sleep_main(void)
{
    static uint8 flag = 0;
    long run_time;
    time_t cur_time;

    if(pdivoom_sleep_handle == NULL)
    {
        return;
    }
    time(&cur_time);

	//天花板项目才需要实时保存
	if(pdivoom_sleep_handle->update_flag)
	{
		divoom_sleep_save();
		pdivoom_sleep_handle->update_flag = 0;
	}

    switch(pdivoom_sleep_handle->status)
    {        
    	case DIVOOM_SLEEP_STATUS_TEST_MODE:
            if(cur_time >pdivoom_sleep_handle->end_time)
            {
                divoom_sleep_check_test_mode(0);
            }
            
            break;
            
    	case DIVOOM_SLEEP_STATUS_STANDBY:
            break;
            
    	case DIVOOM_SLEEP_STATUS_WAITING:
            if(cur_time > pdivoom_sleep_handle->end_time)
            {
                divoom_sleep_active_callback();
                flag = 0;
            }            
            break;
            
    	case DIVOOM_SLEEP_STATUS_SLEEP:
            run_time = cur_time - pdivoom_sleep_handle->end_time;
            if(run_time <= DIVOOM_SLEEP_TURN_OFF_TIME)
            {
                divoom_sleep_disp_ctrl(DIVOOM_SLEEP_STATUS_SLEEP,  run_time);
                flag = 0;
            }
            else if(flag == 0)
            {
                divoom_sleep_disp_ctrl(DIVOOM_SLEEP_STATUS_SLEEP,  DIVOOM_SLEEP_TURN_OFF_TIME);
                flag = 1;
            }
            break;
            
    }
}

void divoom_sleep_exit(void)
{
}

