#include "divoom.h"

#if 0
enum
{
    DIVOOM_LEAVE_MSG_STATUS_EMPTY,
    DIVOOM_LEAVE_MSG_STATUS_WAIT,
    DIVOOM_LEAVE_MSG_STATUS_PLAY,
};

typedef struct _divoom_leave_msg_ctrl
{
    uint8   status;
    uint32  resource_type;
    void    *msg_file_list;   
    char    img_id[64];
}divoom_leave_msg_ctrl, *Pdivoom_leave_msg_ctrl;

static Pdivoom_leave_msg_ctrl  pdivoom_leave_msg_handle = NULL;

uint32 divoom_leave_msg_request_run(uint32 resource_type, int event_id);

/**
 * NAME         divoom_leave_msg_play_over
 * @BRIEF      留言播放完成后销毁资源
 * @PARAM   data 无意义
                    
 * @RETURN  none
 * @RETVAL  
 */

void divoom_leave_msg_play_over(uint8 flag)
{
    Pdivoom_leave_msg_voice_item msg_item;
    divoom_app_schedule_item_info event_info = {0};

    if(flag == DIVOOM_MUSIC_PLAY_STATUS_ERR)
    {
		divoom_print("%s,DIVOOM_MUSIC_PLAY_STATUS_ERR\n",__func__);
//        pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_WAIT;
//        return;
    }
    if(flag == DIVOOM_MUSIC_PLAY_STATUS_PLAY)
    {
        return;
    }
    
    divoom_print("divoom_leave_msg_play_over: play voer!: %d\n",flag); 
    msg_item = divoom_queue_pop(pdivoom_leave_msg_handle->msg_file_list);
    
    divoom_net_app_send_packet(DIVOOM_NET_COMM_MARKED_VOICE, DIVOOM_SERVER_TOPIC, msg_item->file_name);
    divoom_free(msg_item);
    
	if(divoom_queue_items(pdivoom_leave_msg_handle->msg_file_list))
	{
        divoom_disp_ctrl_leave_notice_info msg_info = {0};
        
        //如果还有元素，将播放指令结果
        msg_info.number = divoom_queue_items(pdivoom_leave_msg_handle->msg_file_list);
        msg_info.stats = 0;
        msg_info.image_id = pdivoom_leave_msg_handle->img_id;
        pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_WAIT;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_LEAVE_MSG_MODE, &msg_info);
	}
    else
    {
        //释放所有资源
        event_info.model_id = DIVOOM_MODEL_LEAVE_MSG;
        event_info.event_id = DIVOOM_LEAVE_MSG_APP_EVENT_NOTICE_PLAY;
        event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
        pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_EMPTY;
        event_info.resource_type = DIVOOM_SCHEDULE_ALL_RESOUCE_TYPE;
        divoom_app_schedule_cancel(&event_info);
        event_info.event_id = DIVOOM_LEAVE_MSG_APP_EVENT_RUN_PLAY;
        divoom_app_schedule_cancel(&event_info);
		divoom_mix_screen_set_status_led1(0);
    }
}


/**
 * NAME         divoom_leave_msg_play_start
 * @BRIEF      启动留言播放
 * @PARAM     NONE
                    
 * @RETURN  none
 * @RETVAL  
 */

void divoom_leave_msg_play_start(uint8 status)
{
    char record_url[128];
    Pdivoom_leave_msg_voice_item msg_id;

    divoom_print("divoom_leave_msg_play_start:test start  %d, %d!\n", divoom_queue_items(pdivoom_leave_msg_handle->msg_file_list), status);

    
    divoom_disp_ctrl_leave_notice_info msg_info = {0};
    if(status == 0)
    {
        if(divoom_leave_msg_request_run(DIVOOM_SCHEDULE_ALL_RESOUCE_TYPE, DIVOOM_LEAVE_MSG_APP_EVENT_RUN_PLAY) == 0)
        {
            return;
        }
    }
    msg_id = divoom_queue_get_head_note(pdivoom_leave_msg_handle->msg_file_list);  
    msg_info.number = divoom_queue_items(pdivoom_leave_msg_handle->msg_file_list);
    msg_info.stats = 1;
    msg_info.pinfo = msg_id;
    msg_info.image_id = pdivoom_leave_msg_handle->img_id;
    pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_PLAY;
    divoom_disp_ctrl_set_mode(DIVOOM_DISP_LEAVE_MSG_MODE, &msg_info);

    strcpy(record_url, DIVOOM_SERVER_URL);
    strcat(record_url, msg_id->file_name);    
    divoom_print("divoom_leave_msg_play_start:device will start to play the leave msg: %s\n", record_url);
    divoom_local_music_play(record_url, divoom_main_get_sys_volume(), 0, divoom_leave_msg_play_over, 0);
    
}



uint32 divoom_leave_msg_request_run(uint32 resource_type, int event_id)
{

    divoom_app_schedule_item_info event_info = {0};

    //点亮丝印
	divoom_mix_screen_set_status_led1(1);

    event_info.model_id = DIVOOM_MODEL_LEAVE_MSG;
    event_info.event_id = event_id;
    event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
    event_info.resource_type = resource_type;

    return divoom_app_schedule_push(&event_info);
}

void divoom_leave_msg_over_notice(uint8 mode)
{
    //发异步通知开机事件完成
    divoom_sys_msg_info model_msg = {0};

    model_msg.src_id = DIVOOM_MODEL_LEAVE_MSG;
    model_msg.dest_id = DIVOOM_MODEL_LEAVE_MSG;  
    model_msg.msg_id = DIVOOM_LEAVE_MSG_NOTICE_OVER_EVENT;
    model_msg.msg_type = 1;
    divoom_sys_model_crl_msg_push(&model_msg);
    
}

void divoom_leave_msg_load_new_message_notice(void)
{
    char disp_buf[10];
    divoom_disp_event_disp_info disp_info = {0};

    divoom_print("%s: disp_mode: %d, %d\n", __func__,divoom_disp_ctrl_get_mode(), DIVOOM_DISP_LEAVE_MSG_MODE );
    if(divoom_disp_ctrl_get_mode() == DIVOOM_DISP_LEAVE_MSG_MODE)
    {
        divoom_disp_ctrl_leave_notice_info msg_info = {0}; 
        msg_info.number = divoom_queue_items(pdivoom_leave_msg_handle->msg_file_list);
        msg_info.stats = 0;
        msg_info.pinfo = NULL;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_LEAVE_MSG_MODE, &msg_info);
        return;
    }
    disp_info.on_time = 5;
    disp_info.period_time = 5;
    disp_info.tot_time = 30;
    disp_info.tot_pic = 2;
    disp_info.disp_id[0] = DIVOOM_DISP_DIVOOM_DISP_MESSAGE_NOTIC1_PIC_00;
    disp_info.disp_id[1] = DIVOOM_DISP_DIVOOM_DISP_MESSAGE_NOTIC1_PIC_01;
    
    disp_info.noice_id = DIVOOM_MAIN_PLAY_LOW_POWER_EVENT;
    disp_info.pover_callback = divoom_leave_msg_over_notice;
    sprintf(disp_buf, "Msg: %02d", divoom_queue_items(pdivoom_leave_msg_handle->msg_file_list));
    disp_info.disp_title = disp_buf;
    divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&disp_info);
}
void divoom_leave_msg_load_add(Pdivoom_voice_json_voice_item_info pitem_info)
{
    uint32 resource_type;
    Pdivoom_leave_msg_voice_item msg_item;

    msg_item = (Pdivoom_leave_msg_voice_item)divoom_malloc(sizeof(divoom_leave_msg_voice_item), DIVOOM_MODEL_LEAVE_MSG, __LINE__, __FILE__);

    strcpy(msg_item->file_name, pitem_info->path);
    strcpy(msg_item->user_name, pitem_info->UserName);
    divoom_queue_push(pdivoom_leave_msg_handle->msg_file_list, msg_item);
    if(pdivoom_leave_msg_handle->status != DIVOOM_LEAVE_MSG_STATUS_PLAY)
    {
        if(divoom_disp_ctrl_get_mode()== DIVOOM_DISP_LEAVE_MSG_MODE)
        {
            divoom_leave_msg_load_new_message_notice();
        }
        else
        {
            resource_type = divoom_leave_msg_request_run(DIVOOM_SCHEDULE_ALL_RESOUCE_TYPE, DIVOOM_LEAVE_MSG_APP_EVENT_NOTICE_PLAY);
            if(resource_type  & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE)
            {
                //新留言来的时候，给出一个声音提醒
                divoom_main_play_notice_voice(DIVOOM_DEFAULT_ALL_MUSIC_FILE, NULL, 0);
                pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_WAIT;
            }
            if(resource_type  & DIVOOM_SCHEDULE_WHITE_RESOUCE_TYPE)
            {
                //新留言来的时候，给出一个声音提醒
                divoom_leave_msg_load_new_message_notice();
            }
        }
    }
}

void divoom_leave_msg_load_srv(Pdivoom_devcice_con_ret_json plist_info)
{
    int i;
    Pdivoom_leave_msg_voice_item msg_info;

    strcpy(pdivoom_leave_msg_handle->img_id, plist_info->voice_img_id);
    divoom_queue_pop_all(pdivoom_leave_msg_handle->msg_file_list, NULL);
    for(i = 0; i < plist_info->voice_num; i++)
    {
        msg_info = (Pdivoom_leave_msg_voice_item)divoom_malloc(sizeof(divoom_leave_msg_voice_item), DIVOOM_MODEL_LEAVE_MSG, __LINE__, __FILE__);
        if(msg_info == NULL)
        {
            divoom_print("%s:it fails to malloc to save msg!\n",__FUNCTION__);
            break;
        }
        strcpy(msg_info->file_name, plist_info->pvoice_table[i].path);
        strcpy(msg_info->user_name, plist_info->pvoice_table[i].UserName);
        divoom_queue_push(pdivoom_leave_msg_handle->msg_file_list, msg_info);
    }
    if(plist_info->voice_num)
    {
        divoom_leave_msg_request_run(DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE, DIVOOM_LEAVE_MSG_APP_EVENT_NOTICE_PLAY);
        pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_WAIT;
    }
}

uint8 divoom_leave_msg_event(Pdivoom_sys_msg_info msg)
{
    divoom_print("divoom_leave_msg_event: msg id: %d\n", msg->msg_id);
    switch(msg->msg_id)
    {  
        case DIVOOM_LEAVE_MSG_NOTICE_OVER_EVENT:
            {
                divoom_app_schedule_item_info event_info = {0};

                event_info.model_id = DIVOOM_MODEL_LEAVE_MSG;
                event_info.event_id = DIVOOM_LEAVE_MSG_APP_EVENT_NOTICE_PLAY;
                event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
            
                event_info.resource_type = DIVOOM_SCHEDULE_SCREEN_RESOUCE_TYPE;
                divoom_app_schedule_cancel(&event_info);
            }
            break;
            
        case DIVOOM_LEAVE_MSG_LIST_EVENT:
            divoom_leave_msg_load_srv((Pdivoom_devcice_con_ret_json)msg->data);
            break;

            
        case DIVOOM_LEAVE_MSG_ADD_EVENT:
			divoom_mix_screen_set_status_led1(1);
            divoom_leave_msg_load_add((Pdivoom_voice_json_voice_item_info)msg->data);
            break;
            
     }
    return 1;
}

uint8 divoom_leave_msg_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{
    
    switch(event_id)
    {
            
        case DIVOOM_LEAVE_MSG_APP_EVENT_NOTICE_PLAY:
            //显示留言
            if(pdivoom_leave_msg_handle->status == DIVOOM_LEAVE_MSG_STATUS_EMPTY)
            {
                if(resouce_type & DIVOOM_SCHEDULE_SCREEN_RESOUCE_TYPE)
                {
                    divoom_leave_msg_load_new_message_notice();
                }
                if(resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE) 
                {
                    divoom_main_play_notice_voice(DIVOOM_DEFAULT_ALL_MUSIC_FILE, NULL, 0);
                }
                pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_WAIT;
            }
            break;
        case DIVOOM_LEAVE_MSG_APP_EVENT_RUN_PLAY:
        {
            divoom_disp_ctrl_leave_notice_info msg_info = {0};
            
            //如果还有元素，将播放指令结果
            msg_info.number = divoom_queue_items(pdivoom_leave_msg_handle->msg_file_list);
            msg_info.stats = 0;
            msg_info.image_id = pdivoom_leave_msg_handle->img_id;
            pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_WAIT;
            divoom_disp_ctrl_set_mode(DIVOOM_DISP_LEAVE_MSG_MODE, &msg_info);
        }
            break;
            
    }
    return 1;
}


uint8 divoom_leave_msg_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
    divoom_print("%s: %d\n", __func__, __LINE__);
    switch(event_id)
    {
        case DIVOOM_LEAVE_MSG_APP_EVENT_NOTICE_PLAY:
        case DIVOOM_LEAVE_MSG_APP_EVENT_RUN_PLAY:
            if(resouce_type & DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE)
            {
                divoom_local_music_stop();
            }
            if(pdivoom_leave_msg_handle->status == DIVOOM_LEAVE_MSG_STATUS_PLAY)
            {
                pdivoom_leave_msg_handle->status = DIVOOM_LEAVE_MSG_STATUS_WAIT;
            }
            break;
            
    }
    return 1;
}


uint8 divoom_leave_msg_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
    divoom_print("%s: %d\n", __func__, __LINE__);
    switch(event_id)
    {
        case DIVOOM_LEAVE_MSG_APP_EVENT_NOTICE_PLAY:
        case DIVOOM_LEAVE_MSG_APP_EVENT_RUN_PLAY:
            if(resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE)
            {
                divoom_local_music_stop();
            }
            break;
            
    }
    return 1;
}

uint8 divoom_leave_msg_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
    divoom_print("%s: key : %d, %d, %d\n", __func__, key_id, key_type, pdivoom_leave_msg_handle->status);
    
    switch(key_id)
    {
        case kbFUNC:
            if(pdivoom_leave_msg_handle->status == DIVOOM_LEAVE_MSG_STATUS_WAIT)
            {
                divoom_leave_msg_play_start(0);
                return 1;
            }
            break;
                    
        case kbPRE:
            if(pdivoom_leave_msg_handle->status == DIVOOM_LEAVE_MSG_STATUS_PLAY && (key_type == PRESS_SHORT || key_type == PRESSING))
            {
                divoom_main_change_sys_volumn(0);
                return 1;
            }
            break;
            

        case kbNEXT:
            if(pdivoom_leave_msg_handle->status == DIVOOM_LEAVE_MSG_STATUS_PLAY && (key_type == PRESS_SHORT || key_type == PRESSING))
            {
                divoom_main_change_sys_volumn(1);
                return 1;
            }
            break;
            
    }
    return 0;
}


void divoom_leave_msg_init(void)
{
   // divoom_app_schedule_model_info model_info;

    if(pdivoom_leave_msg_handle)
    {    
        divoom_print("[%s:%d]divoom_leave_msg_init have been inited\n", __func__, __LINE__);
        return;
    }

    pdivoom_leave_msg_handle = (Pdivoom_leave_msg_ctrl)divoom_malloc(sizeof(divoom_leave_msg_ctrl), DIVOOM_MODEL_LEAVE_MSG, __LINE__, __FILE__);
	DIVOOM_MEM_CHECK("LEAVE_MSG", pdivoom_leave_msg_handle, return);
    memset(pdivoom_leave_msg_handle, 0, sizeof(divoom_leave_msg_ctrl));


    pdivoom_leave_msg_handle->msg_file_list = divoom_queue_init(50);
    divoom_sys_model_crl_register(DIVOOM_MODEL_LEAVE_MSG ,divoom_leave_msg_event);
    /*
    model_info.model_id = DIVOOM_MODEL_LEAVE_MSG;
    model_info.play_callback = divoom_leave_msg_app_schedule_play;
    model_info.pause_callback = divoom_leave_msg_app_schedule_pause;
    model_info.destroy_callback = divoom_leave_msg_app_schedule_destroy;
    model_info.key_callback = divoom_leave_msg_app_schedule_key_handle;
    divoom_app_schedule_add_model(&model_info);
    */
}

#endif

