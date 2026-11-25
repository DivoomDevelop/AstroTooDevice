#include "divoom.h"
#include "divoom_ota_rtos.h"
/* MQTT topic 前缀返回指针，调用前必须使用完整函数声明。 */
#include "divoom_open_interface.h"
#include "divoom_ha.h"
#include <unistd.h>
#include "divoom_game_runtime.h"
#include "divoom_store_clock_prefetch.h"
#include "divoom_ble_accept_data.h"
#include "divoom_port_fs.h"
#include "divoom_clock_component_index.h"
#include "divoom_watchface_local_api.h"

#define DIVOOM_DISP_WORD_MAX_NUM    64
#define	TASK_SIZE	(6<<10)

typedef struct{
    uint8 light_config_up;  //灯板配置是否更新
	void *cb_lock;
	void *recv_queue;
	void *recv_sem;
}divoom_net_app_ctrl, *Pdivoom_net_app_ctrl;

Pdivoom_net_app_ctrl pdivoom_net_app_handle = NULL;


static volatile int divoom_net_packet_flag = 0;
/* 用户信息查询当前按串行请求处理，回包只允许更新发起请求的模块。 */
static divoom_net_app_user_info_source g_divoom_net_app_pending_user_info_source =
    DIVOOM_NET_APP_USER_INFO_SOURCE_NONE;

static uint32 divoom_net_app_get_transfer_total(void)
{
    uint32 finished = 0;
    uint32 total = 0;

    divoom_manage_image_get_transfer_progress(&finished, &total);
    return total;
}

/* 用实际加入下载批次的数量判断“有更新”，重复文件不会误进进度页。 */
static void divoom_net_app_submit_reload_result(int command, uint32 before_total)
{
    uint32 after_total;
    int32 add_file_num = 0;

    after_total = divoom_net_app_get_transfer_total();
    if(after_total > before_total)
    {
        add_file_num = (int32)(after_total - before_total);
    }
    divoom_subscribe_manage_reload_result_add(command, add_file_num);
}

void divoom_net_app_free_packet(Pdivoom_net_command pcmd_info)
{
	if(pcmd_info)
	{
		if(pcmd_info->pfreefunc)
		{
			pcmd_info->pfreefunc(pcmd_info);
		}
		else
		{
			if(pcmd_info->cmd_data) //外部的cmd_data 都是局部变量，所以不能free
			{
				divoom_free(pcmd_info->cmd_data);
			}
			divoom_free(pcmd_info);
		}
	}
}

uint8 divoom_net_app_net_is_ok(void)
{
	return divoom_main_get_server_con_status();
	//Pdivoom_sys_ctrl_handle pdivoom_handle = divoom_main_get_sys_info(); 

	//return 1;//pdivoom_handle->dana_net_status;
}

bool divoom_net_app_command_ok(Pdivoom_net_command pcmd_info)
{ 
	if(pcmd_info)
	{
	    return true;
	}
	return false;
} 

//同步发送，没有异步操作，数据不需要释放
/* Keep packet admission out of callers: this RV32 compiler can ICE in cprop
 * when the guarded send path is inlined into the large callback dispatcher. */
#if defined(__GNUC__)
__attribute__((noinline))
#endif
uint32 divoom_net_app_send_packet_tracked(uint16 com, uint8 type, void *data)
{
    if ((com == DIVOOM_NET_COMM_DEVICE_UPDATE_CLOCK_INFO || com == DIVOOM_NET_COMM_DEVICE_UPLOAD_LOG_INFO)) return 0;

	divoom_net_command send_info = {0};
    uint8 topic_type[2] = {MQTT_SER ,MQTT_APP};
	
	if(divoom_net_app_net_is_ok() == 0 && divoom_power_on_manage_get_power_mode() == DIVOOM_POWER_ON_WIFI_MODE)
	{
		/* APP_TOPIC 的本机响应无需服务器 MQTT；LAN HTTP 仍要正常回包。 */
		if(type != DIVOOM_APP_TOPIC)
		{
			return 0;
		}
	}
    memset(&send_info, 0, sizeof(divoom_net_command));
    send_info.cmd = com;
	send_info.type = topic_type[type];		//发送到server还是app
    if(send_info.type == MQTT_SER)
    {
		char topic[128] = {0};
		sprintf(topic, "%s%d/set",divoom_open_get_topic(), divoom_main_get_device_id());
		strcpy((char *)send_info.topic, topic); 
    }
    else
    {
        strcpy((char *)send_info.topic, DIVOOM_MQTT_TO_APP_TOPIC); 
    }
    if(divoom_net_packet_flag == 0)
    {
        time_t cur_time;

        time(&cur_time);
        divoom_net_packet_flag = cur_time;
    }
    send_info.cmd_flag = divoom_net_packet_flag;        //命令标识，将为一个随机数
    send_info.cmd_data = data;
    divoom_net_packet_flag ++;
	divoom_subscribe_manage_transfer_request_add(com, send_info.cmd_flag);
    if (divoom_net_mqtt_manage_send_checked(&send_info))
    {
        return send_info.cmd_flag;
    }
    return 0;
}

int divoom_net_app_send_packet_checked(uint16 com, uint8 type, void *data)
{
    if ((com == DIVOOM_NET_COMM_DEVICE_UPDATE_CLOCK_INFO || com == DIVOOM_NET_COMM_DEVICE_UPLOAD_LOG_INFO)) return 0;

    return divoom_net_app_send_packet_tracked(com, type, data) != 0;
}

void divoom_net_app_send_packet(uint16 com, uint8 type, void *data)
{
    (void)divoom_net_app_send_packet_checked(com, type, data);
}

void divoom_net_app_request_user_info_list(divoom_net_app_user_info_source source, void *request_info)
{
    if(source <= DIVOOM_NET_APP_USER_INFO_SOURCE_NONE ||
       source > DIVOOM_NET_APP_USER_INFO_SOURCE_USER_HEAD ||
       request_info == NULL)
    {
        return;
    }

    if(divoom_net_app_net_is_ok() == 0 &&
       divoom_power_on_manage_get_power_mode() == DIVOOM_POWER_ON_WIFI_MODE)
    {
        return;
    }

    /* 串行约束下只需记住一个来源；下一次请求可覆盖已失效的未回包请求。 */
    g_divoom_net_app_pending_user_info_source = source;
    divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_USER_INFO_LIST,
        DIVOOM_SERVER_TOPIC, request_info);
}

void divoom_net_app_send_packet_lwt(uint16 com, uint8 type, void *data)
{
    if ((com == DIVOOM_NET_COMM_DEVICE_UPDATE_CLOCK_INFO || com == DIVOOM_NET_COMM_DEVICE_UPLOAD_LOG_INFO)) return;

	divoom_net_command send_info = {0};
	Pdivoom_net_command ret_data = NULL;
    uint8 topic_type[2] = {MQTT_SER ,MQTT_APP};
	
	if(divoom_net_app_net_is_ok() == 0)
	{
		return ;
	}
    memset(&send_info, 0, sizeof(divoom_net_command));
    send_info.cmd = com;
	send_info.type = topic_type[type];		//发送到server还是app
    if(send_info.type == MQTT_SER)
    {
		char topic[128] = {0};
        
        sprintf(topic, "%s%d/state",divoom_open_get_topic(), divoom_main_get_device_id());
		strcpy((char *)send_info.topic, topic); 
    }
    else
    {
		strcpy((char *)send_info.topic, DIVOOM_MQTT_APP_LWT_TOPIC); 
    }
    if(divoom_net_packet_flag == 0)
    {
        time_t cur_time;

        time(&cur_time);
        divoom_net_packet_flag = cur_time;
    }
    send_info.cmd_flag = divoom_net_packet_flag;        //命令标识，将为一个随机数
    send_info.cmd_data = data;
    send_info.wait_ret = 1;
    divoom_net_packet_flag ++;
	ret_data = divoom_net_http_manage_send(&send_info);
    if(ret_data)
    {
        divoom_net_app_free_packet(ret_data);
    }
}

void *divoom_net_app_send_packet_http(uint16 com, void *data, uint8 wait_flag)
{
    return divoom_net_app_send_packet_http_guarded(com, data, wait_flag, NULL, NULL);
}

/*
 * 歌词请求入队后可能尚未发送就发生换歌，仅在调用前检查会使旧请求继续发送，产生无用的付费调用。
 * 因此将 allow/user 传到 HTTP 队列，在实际发送前及处理响应时再次检查歌曲代次、退出和联网禁用状态。
 * 旧接口传入 NULL/NULL，普通请求保持原流程；校验不通过时取消请求并唤醒等待线程。
 * 携带 allow 时必须 wait_flag=1，调用者需保持 user 及其引用对象有效，直到本函数返回。
 */
void *divoom_net_app_send_packet_http_guarded(uint16 com, void *data, uint8 wait_flag, int (*allow)(void *, int), void *user)
{
    if ((com == DIVOOM_NET_COMM_DEVICE_UPDATE_CLOCK_INFO || com == DIVOOM_NET_COMM_DEVICE_UPLOAD_LOG_INFO)) return NULL;

    Pdivoom_net_command ret_data = NULL;
    divoom_net_command send_info = {0};
    
    if(divoom_main_get_wifi_con_status() == 0)
    {
        return NULL;
    }
    memset(&send_info, 0, sizeof(divoom_net_command));
    send_info.cmd = com;

    if(divoom_net_packet_flag == 0)
    {
        time_t cur_time;

        time(&cur_time);
        divoom_net_packet_flag = cur_time;
    }
    send_info.cmd_flag = divoom_net_packet_flag;        //命令标识，将为一个随机数
    send_info.cmd_data = data;
    send_info.wait_ret = wait_flag;
    send_info.type = MQTT_APP;
    divoom_net_packet_flag ++;

    if(wait_flag)
    {
        //设置等待时间
        send_info.cmd_time_out = time(0) + 10;    //命令超时时间
    }

    ret_data = divoom_net_http_manage_send_guarded(&send_info, allow, user);
    return ret_data;
}


/*
  Function:       divoom_net_app_send_packet
  Description:	   发送命令数据包
  Input:        com 命令
  				type 发给服务器或者app
  				data 命令所带的数据, 谁分配谁释放, 可以用局部变量
  				wait_flag 1等待返回数据 0不用等待数据
  Return:         ret_data 不为空时外部要释放
  Author:         wangft
  Data:           2020-12-18
*/
void *divoom_net_app_send_packet_wait(uint16 com, uint8 type, void *data)
{
    if ((com == DIVOOM_NET_COMM_DEVICE_UPDATE_CLOCK_INFO || com == DIVOOM_NET_COMM_DEVICE_UPLOAD_LOG_INFO)) return NULL;

	Pdivoom_net_command ret_data = NULL;
	divoom_net_command pcmd_info;
    uint8 topic_type[2] = {MQTT_SER ,MQTT_APP};
	uint8 wait_flag = 1;
	
	if(divoom_net_app_net_is_ok() == 0)
	{
		return NULL;
	}

    memset(&pcmd_info, 0, sizeof(divoom_net_command));
    pcmd_info.cmd = com;
    pcmd_info.type = topic_type[type];		//发送到server还是app
    if(pcmd_info.type == MQTT_SER)
    {
		char topic[128] = {0};
        
		sprintf(topic, "%s%d/set",divoom_open_get_topic(), divoom_main_get_device_id());
		strcpy((char *)pcmd_info.topic, topic); 
    }
    else
    {
        strcpy((char *)pcmd_info.topic, DIVOOM_MQTT_TO_APP_TOPIC); 
    }
    if(divoom_net_packet_flag == 0)
    {
        time_t cur_time;

        time(&cur_time);
        divoom_net_packet_flag = cur_time;
    }
    pcmd_info.cmd_flag = divoom_net_packet_flag; 	//命令标识，将为一个随机数
    pcmd_info.cmd_data = data; 					 
	pcmd_info.wait_ret = wait_flag;
	if (wait_flag)
	{
		//设置等待时间
		pcmd_info.cmd_time_out = time(0) + 10;    //命令超时时间
	}
	
    divoom_net_packet_flag ++;
	ret_data = divoom_net_mqtt_manage_send(&pcmd_info);
	return ret_data;
}

void *divoom_net_app_do_modify_plan_malloc(void *msg_data)
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


void *divoom_net_app_do_comm_malloc(void *msg_data)
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

void *divoom_net_app_do_comm_free(void *msg_data)
{
    Pdivoom_sys_msg_info msg_info = (Pdivoom_sys_msg_info)msg_data;
	
	if(msg_info->data)
		divoom_free(msg_info->data);
    
    divoom_free(msg_info);
    return NULL;
}

/*void divoom_net_app_set_clock_info(Pdivoom_lamp_json_clock_info clock_info, uint8 set_info, uint8 set_mode)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_clock_info clock_data;

	model_msg.src_id = DIVOOM_MODEL_NET;

	memset(&clock_data, 0, sizeof(divoom_light_clock_info));

	clock_data.Is_set_info = set_info;
	clock_data.Is_set_mode = set_mode;
	clock_data.clock_type = clock_info->disp_mode;
	clock_data.clock_colour_index = 0xFF;
	clock_data.clock_lightness = clock_info->clock_light;
	clock_data.clock_colour_info.r_c = clock_info->color[0];
	clock_data.clock_colour_info.g_c = clock_info->color[1];
	clock_data.clock_colour_info.b_c = clock_info->color[2];
	clock_data.clock_bit_use = 0x08;

	model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_CLOCK_INFO;
	model_msg.data = (uint8 *)&clock_data;
	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_net_app_set_lamp_info(Pdivoom_lamp_json_set_lamp lamp_info , uint8 set_info, uint8 set_mode)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_light_info light_info;

	memset(&light_info, 0, sizeof(divoom_light_light_info));
	light_info.Is_set_info = set_info;
	light_info.Is_set_mode = set_mode;
	light_info.light_brightness = lamp_info->light;
	light_info.light_colour_index = 0xFF;
	light_info.light_colour_info.r_c = lamp_info->color[0];
	light_info.light_colour_info.g_c = lamp_info->color[1];
	light_info.light_colour_info.b_c = lamp_info->color[2];
	light_info.light_open_flag = lamp_info->open_flag;
	light_info.light_type = lamp_info->mode;

	model_msg.src_id = DIVOOM_MODEL_NET;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_LIGHT_INFO;
	model_msg.data = (uint8 *)&light_info;
	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_net_app_set_eq_effect(int eq_num, uint8 set_info, uint8 set_mode)
{
	divoom_light_eq_ctrl eq_info;
	divoom_sys_msg_info model_msg = {0};
	
	eq_info.Is_set_mode = set_mode;
	eq_info.Is_set_info = set_info;
	eq_info.all_eq_num = 0;
	eq_info.cur_set_id = eq_num;
	eq_info.eq_info = NULL;

	model_msg.src_id = DIVOOM_MODEL_NET;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_EQ_INFO;
	model_msg.data = (uint8 *)&eq_info;
	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_net_app_set_get_sub(Pdivoom_lamp_json_expect_sub expect_sub_data, uint32 cmd, uint8 set_info, uint8 set_mode)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_expert_info expect_info;
	divoom_light_expert_ctrl expect_data;
	memset(&expect_info, 0, sizeof(divoom_light_expert_info));
	memset(&expect_data, 0, sizeof(divoom_light_expert_ctrl));

	expect_info.Is_set_info = set_info;
	expect_info.Is_set_mode = set_mode;
	expect_info.command = DIVOOM_LIGHT_CONTROL_EXPERT_SUB;
	expect_info.type = expect_sub_data->type;
	//频道订阅
	if(expect_info.type == 0)
	{
		expect_data.channel.channel_id = expect_sub_data->expect_info.channel.channel_id;
		//strcpy(expect_data.channel.file_path, expect_sub_data->expect_info.channel.file_path);
	}
	//达人订阅
	else if(expect_info.type == 1)
	{
		//expect_data.user.update_time = expect_sub_data->update_time;
		expect_data.user.user_id = expect_sub_data->expect_info.user.user_id;
		expect_data.user.user_type = expect_sub_data->expect_info.user.user_type;
		//(cmd == DIVOOM_NET_COMM_LAMP_SET_SUB)?(expect_data.user.update_time = expect_sub_data->expect_info.user.expert_update_time):(expect_data.user.update_time = 0 );
	}
	else
	{
		divoom_print("Some Wrong In %s:%d", __func__, __LINE__);
		return;
	}
	expect_info.expert_info = &expect_data;

	model_msg.src_id = DIVOOM_MODEL_NET;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
	if(cmd == DIVOOM_NET_COMM_LAMP_SET_SUB)
		model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_EXPERT_INFO;
	else
		model_msg.msg_id = DIVOOM_LIGHT_CONTROL_GET_EXPERT_INFO;
	model_msg.data = (uint8 *)&expect_info;
	divoom_sys_model_crl_msg_push(&model_msg);
}

//设置自定义相框
void divoom_net_app_set_get_frame(Pdivoom_lamp_json_frame_info frame_info, uint8 set_info, uint8 set_mode)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_user_info frame_data;
	Pdivoom_light_user_item user_list = NULL;
	//有就更新,没有就置NULL
	if(frame_info->list_num > 0)
	{
		user_list = divoom_malloc(sizeof(divoom_light_user_item)*frame_info->list_num, DIVOOM_MODEL_NET, __LINE__, __FILE__);

		memset(user_list, 0, sizeof(divoom_light_user_item)*frame_info->list_num);
		while(frame_info->list_num-- > 0)
		{
			strcpy((char *)user_list[frame_info->list_num].frame_addr, frame_info->frame_list[frame_info->list_num].frame_addr);
		}
	}
	memset(&frame_data, 0, sizeof(divoom_light_user_info));
	frame_data.update_time = frame_info->update_time;
	frame_data.Is_set_info = set_info;
	frame_data.Is_set_mode = frame_info->is_change;
	frame_data.frame_list_num = frame_info->list_num;
	frame_data.frame_list = user_list;
	//strcpy(frame_data.file_addr, frame_info->frame_addr);

	model_msg.src_id = DIVOOM_MODEL_NET;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_USER_DEFINE_INFO;
	model_msg.data = (uint8 *)&frame_data;
	divoom_sys_model_crl_msg_push(&model_msg);
	//删掉,后面会自己申请空间
	divoom_free(user_list);
}

void divoom_net_app_get_eq_list(Pdivoom_lamp_json_get_eq_list eq_list, uint8 set_info, uint8 set_mode)
{
	int i = 0;
	divoom_sys_msg_info model_msg = {0};
	divoom_light_eq_ctrl eq_info;
	Pdivoom_light_eq_info eq_addr_list = NULL;
	eq_info.Is_set_mode = set_mode;
	eq_info.Is_set_info = set_info;
	eq_info.eq_command = DIVOOM_LIGHT_CONTROL_ADD_ALL;
	eq_info.all_eq_num = eq_list->all_eq_num;
	//==0不接受这两个值
	eq_info.cur_set_id = 0;
	eq_info.eq_update_time = eq_list->update_time;
	eq_addr_list = (Pdivoom_light_eq_info)divoom_malloc(sizeof(divoom_light_eq_info)*eq_info.all_eq_num, DIVOOM_MODEL_NET, __LINE__, __FILE__);

	memset(eq_addr_list, 0, sizeof(divoom_light_eq_info)*eq_info.all_eq_num);

	for(i=0;i<eq_info.all_eq_num;i++)
	{
		eq_addr_list[i].id = i + DIVOOM_DISP_MUSIC_END_MODE;
		strcpy(eq_addr_list[i].file_addr, eq_list->all_eq_addr[i].eq_file_addr);
	}

	eq_info.eq_info = eq_addr_list;

	model_msg.src_id = DIVOOM_MODEL_NET;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_EQ_INFO;
	model_msg.data = (uint8 *)&eq_info;
	divoom_sys_model_crl_msg_push(&model_msg);
	divoom_free(eq_addr_list);
}*/

void *divoom_net_app_task_main(void *arg)
{
	Pdivoom_net_command pcmd_info = NULL;

	while (1)
	{
        
		divoom_system_get_sem(pdivoom_net_app_handle->recv_sem);
        
		while(divoom_queue_not_empty(pdivoom_net_app_handle->recv_queue))
		{
			pcmd_info = divoom_queue_pop(pdivoom_net_app_handle->recv_queue);
			if(pcmd_info != NULL)
			{
				if (divoom_ota_server_get_update_status() == 0
				    && !divoom_ota_jl_is_upgrading())
					divoom_net_app_callback(pcmd_info);
				divoom_net_app_free_packet(pcmd_info);
			}
		}
        if(pdivoom_net_app_handle->light_config_up)
        {
        	divoom_sys_msg_info model_msg = {0};

        	model_msg.src_id = DIVOOM_MODEL_NET;
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_SERVE_CHANNEL_INFO;
			divoom_sys_model_crl_msg_push(&model_msg);
            
            pdivoom_net_app_handle->light_config_up = 0;
        }
	}
    return NULL;
}

int divoom_net_app_sync_draw_cmd_compare(void *item1, void *item2)
{
    T_U16 cmd = *(uint16 *)item1;
    Pdivoom_net_command pcmd_info = (Pdivoom_net_command)item2;

    return cmd != pcmd_info->cmd;
}

int divoom_net_app_skip_cmd_compare(void *item1, void *item2)
{
    int i;
    T_U16 cmd = *(uint16 *)item1;
    Pdivoom_net_command pcmd_info = (Pdivoom_net_command)item2;
    T_U16 skip_cmd_table[] ={   //除了实时画板，需要跨过的命令
        DIVOOM_NET_COMM_CHANNEL_SET_PLAY_CUSTOM_ID,
        DIVOOM_NET_COMM_CHANNEL_SET_CLOCK_SELECT_ID,
        DIVOOM_NET_COMM_CHANNEL_SET_EQ_POS,
        DIVOOM_NET_COMM_CHANNEL_SET_CHANNEL_INDEX,
        DIVOOM_NET_COMM_CHANNEL_SET_SUBSCRIBE_CONFIG,
    	DIVOOM_NET_COMM_DRAWING,						
    	DIVOOM_NET_COMM_LOCAL_DRAWING,
    	DIVOOM_NET_COMM_REMOTE_DRAWING,
    	DIVOOM_NET_COMM_SYNC_DRAWING,			
    	DIVOOM_NET_COMM_EXIT_SYNC_DRAWING,	
    	DIVOOM_NET_COMM_NEED_SEND_DRAW,
    	DIVOOM_NET_COMM_DRAW_SET_INFO,
    	DIVOOM_NET_COMM_PIC_SCAN,				
    	DIVOOM_NET_COMM_SAND_PAINT,			
    };

    T_U16 skip_cmd_table2[] ={  //实时画板要跳过的命令
        DIVOOM_NET_COMM_CHANNEL_SET_PLAY_CUSTOM_ID,
        DIVOOM_NET_COMM_CHANNEL_SET_CLOCK_SELECT_ID,
        DIVOOM_NET_COMM_CHANNEL_SET_EQ_POS,
        DIVOOM_NET_COMM_CHANNEL_SET_CHANNEL_INDEX,
        DIVOOM_NET_COMM_CHANNEL_SET_SUBSCRIBE_CONFIG,
        DIVOOM_NET_COMM_CHANNEL_SET_EQ_POS,
    };

    if(cmd == DIVOOM_NET_COMM_SYNC_DRAWING || cmd == DIVOOM_NET_COMM_LOCAL_DRAWING)
    {
        for(i = 0; i < sizeof(skip_cmd_table2)/sizeof(skip_cmd_table2[0]); i++)
        {
            if(skip_cmd_table[i] == pcmd_info->cmd)
            {
                return 0;
            }
        }
        return 1;
    }
    else
    {
        for(i = 0; i < sizeof(skip_cmd_table)/sizeof(skip_cmd_table[0]); i++)
        {
            if(skip_cmd_table[i] == pcmd_info->cmd)
            {
                return 0;
            }
        }
        return 1;
    }
        
}

int divoom_net_app_copy_draw_info(Pdivoom_drawing_json_sync_image_info pold_draw_info, Pdivoom_drawing_json_sync_image_info pcur_draw_info)
{

    uint16 *pos_table;    
    divoom_print("combine cmd!\n");
    pos_table = divoom_system_malloc(sizeof(uint16) *(pold_draw_info->num + pcur_draw_info->num));
    
    if(pos_table)
    {
        memcpy(pos_table, pold_draw_info->dot_pos, pold_draw_info->num * sizeof(uint16)); 
        memcpy(&pos_table[pold_draw_info->num], pcur_draw_info->dot_pos, pcur_draw_info->num * sizeof(uint16)); 
        pold_draw_info->num = pold_draw_info->num + pcur_draw_info->num;
        divoom_system_free(pold_draw_info->dot_pos);
        pold_draw_info->dot_pos = pos_table;
        return 1;
    }
    return 0;
}


/*
  Function:       divoom_net_app_clear_cmd
  Description:	   把命令全部清除
  Input:          cmd_type
  Return:         NONE
*/

void divoom_net_app_clear_cmd(T_U16 cmd_type)
{
    Pdivoom_net_command pskip_cmd; 
    
    while(1)
    {
        pskip_cmd = divoom_queue_pop_cmp(pdivoom_net_app_handle->recv_queue, &cmd_type, divoom_net_app_sync_draw_cmd_compare);
        if(pskip_cmd == NULL)
        {
            break;
        }
        divoom_print("we skip the cmd: %d\n", pskip_cmd->cmd);
        if(pskip_cmd->type  == DIVOOM_APP_TOPIC)
        {
            divoom_net_app_send_packet(pskip_cmd->cmd, DIVOOM_APP_TOPIC, NULL);
        }
        divoom_net_app_free_packet(pskip_cmd);            
    }
}

/*
  Function:       divoom_net_app_push_cmd
  Description:	   把命令压入队列
  Input:          pnet_info 命令信息，外部分配，处理完之后内部释放
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_net_app_push_cmd(Pdivoom_net_command pnet_info)
{
    int i;
    Pdivoom_net_command pskip_cmd;    
    T_U16 skip_cmd_table[] = {
        DIVOOM_NET_COMM_CHANNEL_SET_PLAY_CUSTOM_ID,
        DIVOOM_NET_COMM_CHANNEL_SET_CLOCK_SELECT_ID,
        DIVOOM_NET_COMM_CHANNEL_SET_EQ_POS,
        DIVOOM_NET_COMM_CHANNEL_SET_CHANNEL_INDEX,
    	DIVOOM_NET_COMM_DRAWING,						
    	DIVOOM_NET_COMM_LOCAL_DRAWING,
    	DIVOOM_NET_COMM_REMOTE_DRAWING,
    	DIVOOM_NET_COMM_SYNC_DRAWING,			
    	DIVOOM_NET_COMM_EXIT_SYNC_DRAWING,	
    	DIVOOM_NET_COMM_NEED_SEND_DRAW,
    	DIVOOM_NET_COMM_DRAW_SET_INFO,
    	DIVOOM_NET_COMM_PIC_SCAN,				
    	DIVOOM_NET_COMM_SAND_PAINT,			
    };
        
    /* MyClockAdd 不等待 recv_queue 中正在同步请求的日历等业务。 */
    if(pnet_info && pnet_info->cmd == DIVOOM_NET_COMM_ADD_NEW_CLOCK_TO_MY_CLOCK &&
        pnet_info->type == DIVOOM_SERVER_TOPIC && pnet_info->cmd_data &&
        *(int *)pnet_info->cmd_data < DIVOOM_CUSTOM_START_CLOCK_ID &&
        divoom_ota_server_get_update_status() == 0 && !divoom_ota_jl_is_upgrading() &&
        !divoom_game_runtime_low_mem_guard_is_active())
    {
        divoom_sys_msg_info model_msg = {0};
        int *clock_id = divoom_malloc(sizeof(int), DIVOOM_MODEL_NET, __LINE__, __FILE__);

        if(clock_id)
        {
            *clock_id = *(int *)pnet_info->cmd_data;
            model_msg.msg_type = 1;
            model_msg.src_id = DIVOOM_MODEL_NET;
            model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
            model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ADD_CLOCK_RECEIVED;
            model_msg.data = (uint8 *)clock_id;
            model_msg.data_len = sizeof(int);
            divoom_main_reset_check_mqtt_server_heart();
            divoom_print("clock add dispatch: %d\n", *clock_id);
            divoom_sys_model_crl_msg_push(&model_msg);
            divoom_net_app_free_packet(pnet_info);
            return;
        }
    }

	divoom_system_get_lock(pdivoom_net_app_handle->cb_lock);
    #if 1
    
    for(i = 0; i < sizeof(skip_cmd_table)/sizeof(skip_cmd_table[0]); i++)
    {
        if(skip_cmd_table[i] == pnet_info->cmd)
        {
            break;
        }
    }
    if(i < sizeof(skip_cmd_table)/sizeof(skip_cmd_table[0]))
    {
        while(1)
        {
            pskip_cmd = divoom_queue_pop_cmp(pdivoom_net_app_handle->recv_queue, &pnet_info->cmd, divoom_net_app_skip_cmd_compare);
            if(pskip_cmd == NULL)
            {
                break;
            }
            if(pskip_cmd->type  == DIVOOM_APP_TOPIC)
            {
                divoom_net_app_send_packet(pskip_cmd->cmd, DIVOOM_APP_TOPIC, NULL);
            }
            divoom_print("we skip the cmd: %d\n", pskip_cmd->cmd);
            divoom_net_app_free_packet(pskip_cmd);            
        }
    }
    
    if(pnet_info->cmd == DIVOOM_NET_COMM_SYNC_DRAWING)
    {
        pskip_cmd = divoom_queue_pop_cmp(pdivoom_net_app_handle->recv_queue, &pnet_info->cmd, divoom_net_app_sync_draw_cmd_compare);
        if(pskip_cmd)
        {
            //将两个命令合成一个命令
            Pdivoom_drawing_json_sync_image_info pold_draw_info = (Pdivoom_drawing_json_sync_image_info)pskip_cmd->cmd_data;
            Pdivoom_drawing_json_sync_image_info pcur_draw_info = (Pdivoom_drawing_json_sync_image_info)pnet_info->cmd_data;

            if(memcmp(pold_draw_info->color, pcur_draw_info->color, 3) == 0)
            {
//                Pdivoom_drawing_json_sync_image_info pnew_draw_info;
                
                if(divoom_net_app_copy_draw_info(pold_draw_info, pcur_draw_info))
                {
                    divoom_net_app_free_packet(pnet_info);
                    pnet_info = NULL;
                }
            }
        	divoom_queue_push(pdivoom_net_app_handle->recv_queue, (void *)pskip_cmd);
        }
        
    }
    
    #endif
    if(pnet_info)
    {
	    divoom_queue_push(pdivoom_net_app_handle->recv_queue, (void *)pnet_info);
    }
	divoom_system_free_lock(pdivoom_net_app_handle->cb_lock);
	divoom_system_put_sem(pdivoom_net_app_handle->recv_sem);
    
    
}


/*
  Function:       divoom_net_app_push_quick_cmd
  Description:	   把命令压入队列头，加速处理
  Input:          pnet_info 命令信息，外部分配，处理完之后内部释放
  Return:         NONE
  Author:         aijun
  Data:           2020-12-18
*/
void divoom_net_app_push_quick_cmd(Pdivoom_net_command pnet_info)
{
	divoom_system_get_lock(pdivoom_net_app_handle->cb_lock);
    if(pnet_info)
    {
	    divoom_queue_push_head(pdivoom_net_app_handle->recv_queue, (void *)pnet_info);
    }
	divoom_system_free_lock(pdivoom_net_app_handle->cb_lock);
	divoom_system_put_sem(pdivoom_net_app_handle->recv_sem);
    
    
}



void divoom_net_app_init(void)
{
	if(pdivoom_net_app_handle != NULL)
	{
		return;
	}
	pdivoom_net_app_handle = divoom_malloc(sizeof(divoom_net_app_ctrl), DIVOOM_MODEL_NET, __LINE__, __FILE__);
	pdivoom_net_app_handle->cb_lock = divoom_system_create_lock();
	pdivoom_net_app_handle->recv_queue = divoom_queue_init(1000, __func__, __LINE__);
	pdivoom_net_app_handle->recv_sem = divoom_system_create_sem(2, 0);
	divoom_system_create_task_prio(divoom_net_app_task_main, "net_app", NULL, DIVOOM_TASK_STACK_NET_APP, DIVOOM_SYSTEM_TASK_MODEL_MSG_PRIO);
}


//这个函数没有完成，请求设备的各个模块的指令处理都从这里增加
void divoom_net_app_callback(Pdivoom_net_command pnet_info)	//请求设备的命令
{
	//divoom_system_get_lock(pdivoom_net_app_handle->cb_lock);
	divoom_sys_msg_info model_msg = {0};

    if(pnet_info == NULL)
    {
        return;
    }

    if(divoom_game_runtime_low_mem_guard_is_active())
    {
        return;
    }

	model_msg.src_id = DIVOOM_MODEL_NET;

    if(DIVOOM_NET_COMM_SYNC_DRAWING != pnet_info->cmd && DIVOOM_NET_COMM_LED_SEND_DATA  != pnet_info->cmd)
    {
	    divoom_print("com %d\n", pnet_info->cmd);	
    }

	if (pnet_info->type == DIVOOM_APP_TOPIC)
	{
		//divoom_main_manage_accept_app_hearbeat(pnet_info->userid);
	}
    else if (pnet_info->type == DIVOOM_SERVER_TOPIC)
    {
        divoom_main_reset_check_mqtt_server_heart();
    }
	
	switch(pnet_info->cmd)
	{
        case DIVOOM_NET_COMM_SAVE_DEVICE_TEST_FLAG:
        {
            /* 只将当前设备的成功回包转交 MAIN，标量随消息复制。 */
            if (pnet_info->type == DIVOOM_SERVER_TOPIC &&
                pnet_info->device_id == divoom_main_get_device_id() && pnet_info->cmd_flag)
            {
                model_msg.msg_type = 1;
                model_msg.dest_id = DIVOOM_MODEL_MAIN;
                model_msg.msg_id = DIVOOM_MAIN_EVENT_BETA_FEATURES_SAVED;
                model_msg.result = (int32)pnet_info->cmd_flag;
                divoom_sys_model_crl_msg_push(&model_msg);
            }
            break;
        }

        case DIVOOM_NET_COMM_HOME_ASSISTANT_LOGIN:
        case DIVOOM_NET_COMM_HOME_ASSISTANT_LOGOUT:
        {
            int result = 1;
            divoom_home_assistant_login_info *info = pnet_info->cmd_data;
            /* 服务器负责校验用户及设备归属；设备拒绝发给其他设备的配置。 */
            if(pnet_info->type == DIVOOM_SERVER_TOPIC && pnet_info->device_id == divoom_main_get_device_id())
            {
                if(pnet_info->cmd == DIVOOM_NET_COMM_HOME_ASSISTANT_LOGOUT)
                {
                    result = divoom_ha_clear() ? 1 : 0;
                }
                else if(info)
                {
                    result = divoom_ha_login(info->ha_url, info->ha_client_id, info->ha_refresh_token) ? 1 : 0;
                }
            }
            if(info)
                memset(info, 0, sizeof(*info));
            divoom_net_app_send_packet(pnet_info->cmd, pnet_info->type == DIVOOM_SERVER_TOPIC ? DIVOOM_SERVER_TOPIC : DIVOOM_APP_TOPIC, &result);
            break;
        }
	
        case DIVOOM_NET_COMM_APP_GET_NO_BAN_FLAG:
            if(pnet_info->cmd_data)
            {
                divoom_config_set_china_config(*(int *)pnet_info->cmd_data);
            }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_REMOVE_LOCAL_CLOCK:
            divoom_main_remove_custom_clcok((int *)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_LOCAL_CLOCK_LIST:
            divoom_main_save_custom_clock_list((void *)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_FONT_NAME_INFO:
            divoom_system_font_save_font_name((void *)pnet_info->cmd_data);
            break;
            
		case DIVOOM_NET_COMM_STORE_CLOCK_GET_LIST:
			divoom_store_clock_prefetch_on_list_response((Pdivoom_device_store_clock_list)pnet_info->cmd_data);
			break;
        
        case DIVOOM_NET_COMM_CLOUD_PHOTO_ADD_IMAGE_ITEM:
            divoom_cloud_photo_ctrl_add_item((Pdivoom_device_cloud_photo_item)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_CLOUD_PHOTO_DEL_IMAGE_ITEM:
        {
            Pdivoom_device_cloud_photo_delete_info pinfo = (Pdivoom_device_cloud_photo_delete_info)pnet_info->cmd_data;
            for(int i = 0; i < pinfo->number; i++)
            {
                divoom_cloud_photo_ctrl_del_item(pinfo->photo_list[i]);
            }
        }
            break;
            
    	case DIVOOM_NET_COMM_CLOUD_PHOTO_GET_IMAGE_LIST:
        {
            divoom_cloud_photo_ctrl_update_list((Pdivoom_device_cloud_photo_list_info)pnet_info->cmd_data);
        }
            break;
        
        case DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST:
        {
            uint32 before_total = divoom_net_app_get_transfer_total();

            divoom_subscribe_manage_transfer_response_begin(
                DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST, pnet_info->cmd_flag);
            divoom_userimagelist_manage_update_file_list((Pdivoom_devcice_image_list)pnet_info->cmd_data);
            divoom_subscribe_manage_transfer_response_end();
            divoom_net_app_submit_reload_result(
                DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST, before_total);
        }
            break;
            
        case DIVOOM_NET_COMM_PLAY_LIST_SEND:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_PLAY_LIST;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_ADD_VISION_ITEM:
    	{
            Pdivoom_device_vision_item pinfo = (Pdivoom_device_vision_item)pnet_info->cmd_data;

            if(pinfo)
            {
                divoom_vision_ctrl_add_item(pinfo->id, pinfo->oss_name, time(0));
            }
    	}
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_REMOVE_VISION_ITEM:
    	{
            Pdivoom_device_vision_item pinfo = (Pdivoom_device_vision_item)pnet_info->cmd_data;

            if(pinfo)
            {
                divoom_vision_ctrl_del_item(pinfo->id);
            }
    	}
            break;
            
            
    	case DIVOOM_NET_COMM_DEVICE_GET_VISION_ALARM_TIME:
    	case DIVOOM_NET_COMM_DEVICE_SET_VISION_ALARM_TIME:
            if(pnet_info->cmd_data)
            {
                //divoom_clock_schedule_update_divoom_cloud((Pdivoom_channel_user_define_schedule_info)pnet_info->cmd_data);
            }
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_VISION_PLAY_TIME:
    	case DIVOOM_NET_COMM_DEVICE_GET_VISION_PLAY_TIME:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_VISION_PLAY_TIME;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_GET_VISION_UPDATE_LIST:
            divoom_vision_ctrl_update_list((Pdivoom_device_vision_update_info)pnet_info->cmd_data);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_ENTER_CUSTOM_DIAL:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_ENTER_CUSTOM_DIAL, DIVOOM_APP_TOPIC, NULL);
            divoom_custom_dial_ctrl_create_handle(pnet_info->cmd_data);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_EXIT_CUSTOM_DIAL:
            divoom_custom_dial_ctrl_destory_handle();
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_EXIT_CUSTOM_DIAL, DIVOOM_APP_TOPIC, NULL);
            break;
            
            
    	case DIVOOM_NET_COMM_DEVICE_ADD_CUSTOM_DISP_ITEMS:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_ADD_CUSTOM_DISP_ITEMS, DIVOOM_APP_TOPIC, NULL);
            divoom_custom_dial_ctrl_update_disp_text_item(pnet_info->cmd_data);
            break;
            
            
        case DIVOOM_NET_COMM_DEVICE_ENTER_OUTLOOK_CALENDAR:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ENTER_OUTLOOK_CALENDAR;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_SET_OUTLOOK_CALENDAR_TOKEN:
        {
            Pdivoom_device_google_calendar_config_respone_info pinfo = (Pdivoom_device_google_calendar_config_respone_info)pnet_info->cmd_data;
            
            divoom_outlook_calendar_update_token(-1, -1, pinfo->token);
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_SET_OUTLOOK_CALENDAR_CONFIG:
        {
            Pdivoom_device_google_calendar_config_respone_info pinfo = (Pdivoom_device_google_calendar_config_respone_info)pnet_info->cmd_data;
            
            divoom_outlook_calendar_update_switch(pinfo->pop_flag, pinfo->event_flag);
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_OUTLOOK_CALENDAR_CONFIG:
        {
            Pdivoom_device_google_calendar_config_respone_info pinfo = (Pdivoom_device_google_calendar_config_respone_info)pnet_info->cmd_data;
            
            divoom_outlook_calendar_update_token(pinfo->pop_flag, pinfo->event_flag, pinfo->token);
        }
            break;
        
		case DIVOOM_NET_COMM_DEVICE_SEND_DANMAKU_FACE:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SEND_DANMAKU_FACE;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;

        case DIVOOM_NET_COMM_DEVICE_ENTER_GOOGLE_CALENDAR:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ENTER_GOOGLE_CALENDAR;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_GET_GOOGLE_CALENDAR_CONFIG:
    	{
            Pdivoom_device_google_calendar_config_respone_info pinfo = (Pdivoom_device_google_calendar_config_respone_info)pnet_info->cmd_data;
            
            divoom_google_calendar_update_token(pinfo->pop_flag, pinfo->event_flag, pinfo->token);
    	}
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_GOOGLE_CALENDAR_TOKEN:
    	{
            Pdivoom_device_google_calendar_config_respone_info pinfo = (Pdivoom_device_google_calendar_config_respone_info)pnet_info->cmd_data;
            
            divoom_google_calendar_update_token(-1, -1, pinfo->token);
    	}
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_GOOGLE_CALENDAR_CONFIG:
    	{
            Pdivoom_device_google_calendar_config_respone_info pinfo = (Pdivoom_device_google_calendar_config_respone_info)pnet_info->cmd_data;
            
            divoom_google_calendar_update_switch(pinfo->pop_flag, pinfo->event_flag);
    	}
            break;
        
        case DIVOOM_NET_COMM_DEVICE_UPDATE_TEST_FILE:
            divoom_main_update_test_file(pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_STORAGE_STATUS:
        {
            uint32 free_kb = divoom_port_fs_free_space(DIVOOM_USER_ROOT);
            int full_flag = free_kb < DIVOOM_ROOM_WARNING_MIN;

            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_STORAGE_STATUS, DIVOOM_APP_TOPIC, &full_flag);
        }
            break;

        case DIVOOM_NET_COMM_GET_SCREEN_SNAPSHOT:
        {
            int saved;

            unlink(DIVOOM_DEVICE_SCREEN_SNAPSHOT_PATH);
            saved = divoom_device_save_screen_snapshot() &&
                divoom_port_file_exists(DIVOOM_DEVICE_SCREEN_SNAPSHOT_PATH) &&
                divoom_app_com_get_file_size(DIVOOM_DEVICE_SCREEN_SNAPSHOT_PATH) > 0;
            if(!saved && pnet_info->type == DIVOOM_APP_TOPIC)
            {
                char *resp = divoom_watchface_local_http_error_json(
                    "Device/GetScreenSnapshot", "local resource or operation unavailable");
                if(resp)
                {
                    divoom_http_server_send_command(DIVOOM_NET_COMM_GET_SCREEN_SNAPSHOT, resp);
                    divoom_system_free(resp);
                    return;
                }
            }
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_SCREEN_SNAPSHOT, DIVOOM_APP_TOPIC,
                (void *)DIVOOM_DEVICE_SCREEN_SNAPSHOT_PATH);
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_CREATE_PRELOAD_ALBUM:
            divoom_pic_album_manage_add_type("Preload", *(int *)pnet_info->cmd_data, DIVOOM_PIC_ALBUM_TYPE_PRELOAD);
            divoom_pic_phone_manage_add_all_preload(*(int *)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_SET_NIGHT_VIEW:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_NIGHT_VIEW_MODE;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_EXIT_NIGHT_VIEW:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_EXIT_NIGHT_VIEW_MODE;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_GET_DEVICE_UPDATE_FILE_LIST:
            divoom_update_system_file_add_file((void *)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_DELETE_PIXEL_DIAL:
        {
            divoom_devcice_user_define_item pclean_item = {0};
            divoom_light_app_delete_clock_id(*(int *)pnet_info->cmd_data);
            pclean_item.clock_id = *(int *)pnet_info->cmd_data;
            divoom_user_define_ctrl_clean_item(&pclean_item);
        }
            break;
        
    	case DIVOOM_NET_COMM_MOVE_MY_CLOCK_POSITION:
    	{
            Pdivoom_channel_move_my_clock_info pinfo = (Pdivoom_channel_move_my_clock_info)pnet_info->cmd_data;
            
            divoom_light_app_move_clock_id(pinfo->clock_id, pinfo->position);
    	}
            break;
            
	case DIVOOM_NET_COMM_REMOVE_MY_CLOCK:
        {
            int32 clock_id = *(int *)pnet_info->cmd_data;

            divoom_light_app_delete_clock_id_direct(clock_id);
			divoom_anniversary_clock_config_del_reminder(clock_id);
            /* 移除命令只删目标主表盘片段，不重新解析其他表盘。 */
            (void)divoom_clock_component_index_remove_clock(clock_id);
        }
            break;
            
	case DIVOOM_NET_COMM_ADD_NEW_CLOCK_TO_MY_CLOCK:
        {
            /* 转移 int 参数给异步业务消息，避免阻塞网络接收线程或提前释放参数。 */
            model_msg.msg_type = 1;
            model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
            model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ADD_CLOCK_RECEIVED;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            pnet_info->cmd_data = NULL;
            divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;

	case DIVOOM_NET_COMM_ADD_MY_CLOCK_RECENT:
        {
            int32 clock_id = *(int *)pnet_info->cmd_data;

            if(clock_id > 0)
            {
                divoom_light_app_add_recent_clock(clock_id, 1);
            }
        }
            break;

	case DIVOOM_NET_COMM_SET_MY_CLOCK_FAVORITE:
        {
            Pdivoom_channel_my_clock_favorite_info pinfo = (Pdivoom_channel_my_clock_favorite_info)pnet_info->cmd_data;

            if(pinfo && pinfo->clock_id > 0 && (pinfo->favorite_flag == 0 || pinfo->favorite_flag == 1))
            {
                divoom_light_app_add_remove_fav_clock(pinfo->clock_id, pinfo->favorite_flag);
            }
        }
            break;

        case DIVOOM_NET_COMM_UPLOAD_VOICE:
            model_msg.dest_id = DIVOOM_MODEL_LEAVE_MSG;
            model_msg.msg_id = DIVOOM_LEAVE_MSG_CTRL_EVENT_SET_VOICE_DATA;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_PHOTO_SET_ALBUM_COVER:
        {
            Pdivoom_photo_album_cover_respone_info pinfo = (Pdivoom_photo_album_cover_respone_info)pnet_info->cmd_data;
            if(pinfo)
            {
                divoom_pic_album_manage_set_album_cover(pinfo->album_id, pinfo->pic_id, 1, 1);
            }
        }
            break;
            
        case DIVOOM_NET_COMM_GET_USER_INFO_LIST:
        {
            Pdivoom_device_get_user_info_list_respone_info puser_info =
                (Pdivoom_device_get_user_info_list_respone_info)
                    pnet_info->cmd_data;
            divoom_net_app_user_info_source source =
                g_divoom_net_app_pending_user_info_source;

            /* 先清空来源，确保更新函数后续发起的新请求不会被本次回包误清除。 */
            g_divoom_net_app_pending_user_info_source =
                DIVOOM_NET_APP_USER_INFO_SOURCE_NONE;

            switch(source)
            {
                case DIVOOM_NET_APP_USER_INFO_SOURCE_HOT:
                {
                    //divoom_hot_manage_update_author_list(puser_info);
                    break;
                }
                case DIVOOM_NET_APP_USER_INFO_SOURCE_FAVORITE:
                {
                    //divoom_favorite_manage_update_author_list(puser_info);
                    break;
                }
                case DIVOOM_NET_APP_USER_INFO_SOURCE_ALBUM:
                {
                    //divoom_album_manage_update_author_list(puser_info);
                    break;
                }
                case DIVOOM_NET_APP_USER_INFO_SOURCE_EXPERT:
                {
                    //divoom_expert_manage_update_author_list(puser_info);
                    break;
                }
                case DIVOOM_NET_APP_USER_INFO_SOURCE_USER_DEFINE:
                {
                    //divoom_user_define_ctrl_update_author_list(puser_info);
                    break;
                }
                case DIVOOM_NET_APP_USER_INFO_SOURCE_USER_IMAGE_LIST:
                {
                   // divoom_userimagelist_manage_update_author_list(puser_info);
                    break;
                }
                case DIVOOM_NET_APP_USER_INFO_SOURCE_USER_HEAD:
                {
                    divoom_user_head_manage_update_list(puser_info);
                    break;
                }
                case DIVOOM_NET_APP_USER_INFO_SOURCE_NONE:
                default:
                {
                    break;
                }
            }
            break;
        }
            
        case DIVOOM_NET_COMM_GET_SYS_PIXEL_STYLE:
        case DIVOOM_NET_COMM_SET_SYS_PIXEL_STYLE:
            divoom_light_app_set_divoom_animation_edge_clock_id(*((int *)pnet_info->cmd_data));
            break;
            
    	case DIVOOM_NET_COMM_DRAWING_PIXEL_LOCAL_HTTP:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DRAWING_PIXEL_LOCAL_HTTP, DIVOOM_APP_TOPIC, NULL);
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_APP_HTTP_LOCAL_IMAG;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
    	case DIVOOM_NET_COMM_DRAWING_EQ_LOCAL_HTTP:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DRAWING_EQ_LOCAL_HTTP, DIVOOM_APP_TOPIC, NULL);
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_APP_HTTP_LOCAL_EQ;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_PHOTO_TEST_LOACL:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_TEST_LOACL, DIVOOM_APP_TOPIC, NULL);
            break;
        
        case DIVOOM_NET_COMM_DEVICE_GET_WEATHER_INFO:
        {
            divoom_device_get_weather_info weather_info;
            divoom_main_get_http_weather_info(&weather_info);
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_WEATHER_INFO, DIVOOM_APP_TOPIC,&weather_info);
        }
            break;

        case DIVOOM_NET_COMM_PHOTO_DELETE_PHOTO:
        {
            Pdivoom_photo_album_app_photo_respone_info pinfo = (Pdivoom_photo_album_app_photo_respone_info)pnet_info->cmd_data;

            if(pinfo->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
            {
                //if(divoom_pic_album_manage_check_delete_pic(pinfo->album_id, pinfo->pic_list, pinfo->pic_num) == 0)
                {
                    divoom_pic_album_manage_del_list_from_id(pinfo->album_id, pinfo->pic_list, pinfo->pic_num);
                    /* del_list_from_id 已删除数据库记录并排队清理文件，不能再次读取已删除的路径。 */
                }
            }
            else
            {
                divoom_clock_id_info clock_info={0};

                clock_info.clock_id = pinfo->album_id;
                clock_info.clock_pos = pinfo->clock_pos;
                clock_info.parent_id = pinfo->parent_id;
                for(int i = 0; i < pinfo->pic_num; i++)
                {
                    divoom_custom_photo_delete_item(&clock_info, pinfo->pic_list[i]);
                }
            }
        }
            break;
        case DIVOOM_NET_COMM_PHOTO_REMOVE_PHOTO_FROM_ALBUM:
        {
            Pdivoom_photo_album_app_photo_respone_info pinfo = (Pdivoom_photo_album_app_photo_respone_info)pnet_info->cmd_data;
            if(divoom_pic_album_manage_check_remove(pinfo->album_id, pinfo->pic_list, pinfo->pic_num) == 0)
            {
                divoom_pic_album_manage_remove_list_from_id(pinfo->album_id, pinfo->pic_list, pinfo->pic_num);
            }
        }
            break;
            
        case DIVOOM_NET_COMM_PHOTO_ADD_PHOTO_TO_ALBUM:
        {
            Pdivoom_photo_album_app_photo_respone_info pinfo = (Pdivoom_photo_album_app_photo_respone_info)pnet_info->cmd_data;
            
            divoom_pic_album_manage_add_list_to_name(pinfo->album_id, pinfo->pic_list, pinfo->pic_num);
        }
            break;
            
        case DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST:
        {
            Pdivoom_photo_album_get_local_photo_respone_info pinfo = (Pdivoom_photo_album_get_local_photo_respone_info)pnet_info->cmd_data;

            if(pinfo->album_id >= DIVOOM_PHOTO_START_CLOCK_ID)
            {
                divoom_pic_album_manage_get_album_local_pic_list(pinfo->album_id, pinfo->start, pinfo->end);
            }
            else
            {
                divoom_clock_id_info clock_info;

                clock_info.clock_id = pinfo->album_id;
                clock_info.clock_pos = pinfo->clock_pos;
                clock_info.parent_id = pinfo->parent_id;
                divoom_custom_photo_get_album_local_pic_list(&clock_info, pinfo->start, pinfo->end);
            }
        }
            break;
            
        case DIVOOM_NET_COMM_APP_ADD_ALI_PHOTO:
            divoom_download_aliyun_file_add_item((Pdivoom_download_aliyun_file_item)pnet_info->cmd_data, 1, NULL, 0);
            break;
            
        case DIVOOM_NET_COMM_GET_ALI_PHOTO_LIST:
        {
            Pdivoom_photo_album_get_ali_photo_info pinfo= (Pdivoom_photo_album_get_ali_photo_info)pnet_info->cmd_data;

            if(pinfo)
            {
                divoom_download_aliyun_file_add_item(pinfo->plist, pinfo->num, NULL, 1);
            }
            else
            {
                divoom_download_aliyun_file_add_item(NULL, 0, NULL, 1);
            }
        }
            break;
            
    	case DIVOOM_NET_COMM_PHOTO_ADD_NEW_ALBUM: //新建相簿
    	{
            Pdivoom_photo_album_album_info palbum_info = (Pdivoom_photo_album_album_info)pnet_info->cmd_data;
            
        	divoom_pic_album_manage_add_type(palbum_info->album_name, palbum_info->album_id, DIVOOM_PIC_ALBUM_TYPE_NORMAL);
    	}
            break;
            
    	case DIVOOM_NET_COMM_PHOTO_ENTER_ALBUM:      //进入照片模式
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ENTER_ALBUM_PLAY;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
    	case DIVOOM_NET_COMM_PHOTO_GET_ALBUM_LIST: //得到相簿列表
    	    divoom_pic_album_manage_update_list((Pdivoom_photo_album_album_list)pnet_info->cmd_data);
            break;

		case DIVOOM_NET_COMM_STORE_CLOCK_GET_CLASSIFY: //获取表盘市场分类
			divoom_store_clock_prefetch_on_classify_response((Pdivoom_device_store_clock_classify_list)pnet_info->cmd_data);
			break;

		case DIVOOM_NET_COMM_GET_AI_CLASSFY_LIST:
			//divoom_ai_talk_classfy_list_respone((Pdivoom_device_ai_classfy_list)pnet_info->cmd_data);
			break;

		case DIVOOM_NET_COMM_GET_AI_TALK_LIST:
			divoom_ai_talk_msg_list_respone((Pdivoom_device_ai_talk_list)pnet_info->cmd_data);
			break;

        case DIVOOM_NET_COMM_APP_SHARE_DEVICE:
            if(pnet_info->cmd_data)
            {
                divoom_main_set_host_id(*(int *)pnet_info->cmd_data);
            }
            break;
		case DIVOOM_NET_COMM_DEVICE_GET_SHARE_CODE:
		{
			Pdivoom_device_share_code_info pshare_code;

			pshare_code = (Pdivoom_device_share_code_info)pnet_info->cmd_data;
			if (pshare_code != NULL)
			{
				divoom_main_update_device_share_code(pshare_code->share_code,
					pshare_code->expire_time);
			}
			break;
		}

    	case DIVOOM_NET_COMM_PHOTO_APP_ADD_PHOTO: //局域网添加照片到相簿
            divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_APP_ADD_PHOTO, DIVOOM_APP_TOPIC, NULL);
        	divoom_download_aliyun_file_add_local_pic_list((void *)pnet_info->cmd_data);
            break;
            
    	case DIVOOM_NET_COMM_PHOTO_PLAY_ALBUM: //播放相簿
    	{
            Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
            if(pslide_config->album_id != *(int *)pnet_info->cmd_data)
            {
                pslide_config->album_id = *(int *)pnet_info->cmd_data;
                divoom_pic_album_manage_set_refresh_album_info();
            }
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ENTER_ALBUM_PLAY;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
    	}
            break;
            
    	case DIVOOM_NET_COMM_PHOTO_RENAME_ALBUM: //重命名相簿
            {
                Pdivoom_photo_album_album_info palbum_info = (Pdivoom_photo_album_album_info)pnet_info->cmd_data;
                
                divoom_pic_album_manage_update_album_name(palbum_info->album_id, palbum_info->album_name);
            }
            break;
            
    	case DIVOOM_NET_COMM_PHOTO_DELETE_ALBUM: //删除相簿
            divoom_pic_album_manage_del_type(*(int *)pnet_info->cmd_data, 0);
            break;
            
    	case DIVOOM_NET_COMM_PHOTO_SET_ALBUM_CONFIG: //设置相簿配置
    	case DIVOOM_NET_COMM_PHOTO_GET_ALBUM_CONFIG: //获取相簿配置
    	    if(pnet_info->cmd_data)
    	    {
                divoom_pic_phone_manage_app_update_slide_config((Pdivoom_photo_album_album_config_info)pnet_info->cmd_data);
    	    }
            break;
            
    	case DIVOOM_NET_COMM_PHOTO_CHANGE_SCALING: //更改所有相片比例
    	{
            Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

            //pslide_config->skip_fill_flag = *(int *)pnet_info->cmd_data;
    	}
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_GET_CLOCK_CONFIG:
            divoom_commit_clock_get_config((Pdivoom_channel_app_clock_config_info)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_GET_PC_CLOCK_CONFIG_INFO:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_TEMP_CLOCK_PARA;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;

            
        case DIVOOM_NET_COMM_DEVICE_TEST_JSON_STR:
            if(pnet_info->cmd_data)
            {
                divoom_home_connect_get_data_from_json((char *)pnet_info->cmd_data);
            }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_PC_UPDATE_CLOCK_DATA:
            divoom_clock_control_update_pc_data(pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_TEST_SELF_RESET:
            if(*(int *)pnet_info->cmd_data == 0)
            {
                char *ptr = NULL;

                divoom_print("it is null: (null)\n");
            }
            else if(*(int *)pnet_info->cmd_data == 1)
            {
                int *test_free_malloc = divoom_system_malloc(100);

                divoom_system_free(test_free_malloc);
                divoom_system_free(test_free_malloc);
            }
            else
            {
                while(1);
            }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_UPDATE_CLOCK_CONFIG:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_LIST, DIVOOM_SERVER_TOPIC,NULL);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_SET_ON_OFF_MODE:
            if(pnet_info->type == DIVOOM_APP_TOPIC)
            {
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_SET_ON_OFF_MODE, DIVOOM_SERVER_TOPIC, pnet_info->cmd_data);
            }
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_NOTICE_UPDATE_ON_OFF_TIME;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_ON_OFF_MODE:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_NOTICE_INIT_ON_OFF_TIME;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;

		case DIVOOM_NET_COMM_DEVICE_GET_POWER_ON_LIST:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_GET_POWER_ON_LIST;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
			
	    case DIVOOM_NET_COMM_DEVICE_SET_POWER_ON:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_POWER_ON_MODE;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_SEND_DANMAKU_TEXT:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SEND_DANMAKU_TEXT;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_SEND_RANDOM_DANMAKU_FACE:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SEND_RANDOM_DANMAKU_FACE;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_GET_DANMAKU_CONFIG:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_GET_DANMAKU_CONFIG;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_SET_DANMAKU_CONFIG:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_DANMAKU_CONFIG;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

        case DIVOOM_NET_COMM_DEVICE_REQUEST_BOND_FALG:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_REQUEST_BOND_FALG, DIVOOM_APP_TOPIC, pnet_info);
            break;
            
		case DIVOOM_NET_COMM_DEVICE_GET_DEBUG_FALG:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_GET_DEBUG_FLAG;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

        case DIVOOM_NET_COMM_DEVICE_GET_CLOCK_LIST:
			{
				Pdivoom_devcice_get_clock_list_respone plistInfo = (Pdivoom_devcice_get_clock_list_respone)pnet_info->cmd_data;
				
				divoom_light_app_update_my_clock_list(plistInfo);
				divoom_anniversary_clock_config_deal_init();
			}
			break;

        case DIVOOM_NET_COMM_DEVICE_GET_CLOCK_RECENT:
			{
				Pdivoom_devcice_get_clock_list_respone plistInfo = (Pdivoom_devcice_get_clock_list_respone)pnet_info->cmd_data;

				if(plistInfo && plistInfo->clock_num > 0 && plistInfo->pclock_list)
				{
					divoom_light_app_replace_recent_clock_list(plistInfo->pclock_list, plistInfo->clock_num);
				}
			}
			break;

        case DIVOOM_NET_COMM_DEVICE_GET_CLOCK_FAVORITE:
			{
				Pdivoom_devcice_get_clock_list_respone plistInfo = (Pdivoom_devcice_get_clock_list_respone)pnet_info->cmd_data;

				if(plistInfo && plistInfo->clock_num > 0 && plistInfo->pclock_list)
				{
					divoom_light_app_replace_fav_clock_list(plistInfo->pclock_list, plistInfo->clock_num);
				}
			}
			break;
        case DIVOOM_NET_COMM_PUBLIC_SER_MQTT_LWT:
            if(*(int *)pnet_info->cmd_data == 0)
            {
                divoom_main_send_lwt_msg(1);
            }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_APP_MQTT_LWT:
            if(*(int *)pnet_info->cmd_data == 0)
            {
                divoom_net_app_send_packet_lwt(DIVOOM_NET_COMM_DEVICE_APP_MQTT_LWT, DIVOOM_APP_TOPIC, NULL);
            }
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_CON_APP, DIVOOM_APP_TOPIC, NULL);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_REBOOT:
            exec_command_system("reboot");
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_CUSTOM_PLAY_TIME:
        case DIVOOM_NET_COMM_DEVICE_GET_CUSTOM_PLAY_TIME:
        {
            divoom_clock_id_info clock_info;
            Pdivoom_channel_set_custom_play_time_info  play_time = (Pdivoom_channel_set_custom_play_time_info)pnet_info->cmd_data;

            clock_info.clock_id = play_time->clock_id;
            clock_info.clock_pos = play_time->clock_pos;
            clock_info.parent_id = play_time->parent_id;
            divoom_user_define_ctrl_update_play_time(&clock_info, play_time->play_time, play_time->show_time_flag);
            break;
        }
           
        case DIVOOM_NET_COMM_CHANNEL_SET_SUSCRIBE_PLAY_TIME: 
        case DIVOOM_NET_COMM_DEVICE_GET_SUSCRIBE_PLAY_TIME:
        {
            divoom_clock_id_info clock_info;
            Pdivoom_channel_set_custom_play_time_info  play_time = (Pdivoom_channel_set_custom_play_time_info)pnet_info->cmd_data;
        
            clock_info.clock_id  = play_time->clock_id;
            if(clock_info.clock_id == 0)
            {
                clock_info.clock_id = DIVOOM_LIGHT_APP_HOT_CLOCK_ID;
            }
            clock_info.clock_pos  = play_time->clock_pos;
            clock_info.parent_id  = play_time->parent_id;
            divoom_subscribe_manage_update_play_time(&clock_info, play_time->play_time, play_time->show_time_flag);
            break;
        }
        case DIVOOM_NET_COMM_PHOTO_SLIDE_GET_ALBUM_TIME_LIST:
        {
            int max_update_time = 0;
            Pdivoom_device_get_album_time_list_info pinfo = (Pdivoom_device_get_album_time_list_info)pnet_info->cmd_data;

            for(int i = 0; pinfo && i < pinfo->number; i++)
            {
                divoom_clock_schedule_update_divoom_cloud(&pinfo->plist[i]);
                if(max_update_time < pinfo->plist[i].update_time)
                {
                    max_update_time = pinfo->plist[i].update_time;
                }
            }
            divoom_pic_phone_manage_update_album_update_time(max_update_time);
        }
            break;
            
        case DIVOOM_NET_COMM_PHOTO_SLIDE_SET_ALBUM_TIME:
        {
            Pdivoom_channel_user_define_schedule_info pinfo = (Pdivoom_channel_user_define_schedule_info)pnet_info->cmd_data;
            divoom_clock_schedule_update_divoom_cloud(pinfo);
            divoom_pic_phone_manage_update_album_update_time(pinfo->update_time);
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_INIT_CUSTOM_SCHEDULE_TIME:
            divoom_clock_schedule_update_divoom_cloud((Pdivoom_channel_user_define_schedule_info)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_CUSTOM_SCHEDULE_TIME:
            divoom_clock_schedule_update_divoom_cloud((Pdivoom_channel_user_define_schedule_info)pnet_info->cmd_data);
        
            break;
          
        case DIVOOM_NET_COMM_CHANNEL_GET_EQ_SCHEDULE_TIME:
            if(pnet_info->cmd_data)
            {
                divoom_clock_schedule_update_divoom_cloud((Pdivoom_channel_user_define_schedule_info)pnet_info->cmd_data);
            }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_EQ_SCHEDULE_TIME:
            if(pnet_info->cmd_data)
            {
                divoom_clock_schedule_update_divoom_cloud((Pdivoom_channel_user_define_schedule_info)pnet_info->cmd_data);
            }
            break;
             
        case DIVOOM_NET_COMM_CHANNEL_GET_SUBSCRIBE_SCHEDULE_TIME:
            if(pnet_info->cmd_data)
            {
                divoom_clock_schedule_update_divoom_cloud((Pdivoom_channel_user_define_schedule_info)pnet_info->cmd_data);
            }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_SUBSCRIBE_SCHEDULE_TIME:
            if(pnet_info->cmd_data)
            {
                divoom_clock_schedule_update_divoom_cloud((Pdivoom_channel_user_define_schedule_info)pnet_info->cmd_data);
            }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_FORMAT_SD:
            divoom_main_start_format_sd();
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_DIS_TEMP_MODE:
            divoom_main_set_temp_flag(*(int *)pnet_info->cmd_data);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_SCREEN_DIR_AGE:
            divoom_main_set_screen_dir(*(int *)pnet_info->cmd_data);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_MIRROR_MODE:
            divoom_main_set_mirror_flag(*(int *)pnet_info->cmd_data);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_HIGH_LIGHT_MODE:
            divoom_main_set_high_light_mode(*(int *)pnet_info->cmd_data);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_WHITE_BALANCE:
            divoom_main_set_white_banlance((divoom_screen_color_info *)pnet_info->cmd_data);
            break;
        
        case DIVOOM_NET_COMM_DEVICE_PLAY_DEVICE_BUZZER:
            divoom_main_play_buzzer((Pdivoom_devcice_play_buzzer_respone)pnet_info->cmd_data);
            break;
            
    	case DIVOOM_NET_COMM_DEVICE_SET_TIME24_FLAG:
            divoom_main_set_time_flag(*(int *)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_SCREEN_CTRL:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_APP_CTRL;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_SCREEN_STATUS:
        {
            int status = divoom_light_app_get_screen_status(0xFF);
            
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_SCREEN_STATUS, DIVOOM_APP_TOPIC, &status);
        }
            break;
            
        case DIVOOM_NET_COMM_APP_PLAY_TF_FOLDER:
        {
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_PLAY_LOCAL_GIF_FOLDER;
            model_msg.data = (uint8 *)"divoom_gif";
			divoom_sys_model_crl_msg_push(&model_msg);
            
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_PLAY_TF_GIF:
        {
            Pdivoom_device_play_tf_gif_info pgif_info = (Pdivoom_device_play_tf_gif_info)pnet_info->cmd_data;
            
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
            if(pgif_info->flag == 2)
            {
			    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DOWNLOAD_NET_GIF_FILE;
            }
            else if(pgif_info->flag == 1)
            {
			    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_PLAY_LOCAL_GIF_FOLDER;
            }
            else if(pgif_info->flag == 0)
            {
			    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_PLAY_LOCAL_GIF_FILE;
            }
            else
            {
                break;
            }
            model_msg.data = (uint8 *)pgif_info->file_name;
			divoom_sys_model_crl_msg_push(&model_msg);
            
        }
            break;
            
        case DIVOOM_NET_COMM_GET_ALBUM_LIST:
        {
            uint32 before_total = divoom_net_app_get_transfer_total();

            divoom_subscribe_manage_transfer_response_begin(
                DIVOOM_NET_COMM_GET_ALBUM_LIST, pnet_info->cmd_flag);
            divoom_album_manage_update_file_list((Pdivoom_devcice_image_list)pnet_info->cmd_data);
            divoom_subscribe_manage_transfer_response_end();
            divoom_net_app_submit_reload_result(
                DIVOOM_NET_COMM_GET_ALBUM_LIST, before_total);
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_HTTP_RESET_GIF_ID:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_RESET_HTTP_GIF_ID;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            

        case DIVOOM_NET_COMM_DEVICE_HTTP_ClEAR_TEXT:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_CLEAR_HTTP_IMAGE_TEXT;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            


        case DIVOOM_NET_COMM_DEVICE_HTTP_SEND_TEXT:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_TEXT;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            

        case DIVOOM_NET_COMM_DEVICE_HTTP_SEND_ITEM_LIST:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_TEXT_LIST;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_HTTP_SEND_IMAGE:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_HTTP_IMAGE_GIF;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_SHARE_DEVICE:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_APP_ID, DIVOOM_SERVER_TOPIC, NULL);
            break;

        case DIVOOM_NET_COMM_DEVICE_RESET_SYSTEM:
        {
            model_msg.msg_type = 1;
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING;
			divoom_sys_model_crl_msg_push(&model_msg); 
        }
            break;
        case DIVOOM_NET_COMM_DEVICE_RESET_ALL:
        {
			int app_flag = *(int *)pnet_info->cmd_data;
		
            divoom_manage_image_up_netstatus(0);
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_CLEAR_RESET_FLAG, DIVOOM_SERVER_TOPIC, NULL);
			
			if(app_flag == 1)
			{
				model_msg.msg_type = 1;
				model_msg.dest_id = DIVOOM_MODEL_MAIN; 
				model_msg.msg_id = DIVOOM_MAIN_EVENT_CONFIG_NET_WORK;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_APP_HEARBEAT:
            divoom_main_manage_send_connect_app(pnet_info->userid);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_SET_LOG_LEVEL:
        {            
            divoom_main_update_log_level(*(int *)pnet_info->cmd_data);
        }
            break;

        case DIVOOM_NET_COMM_GET_ALL_EXCHANGE_RATE_LIST:
        case DIVOOM_NET_COMM_GET_EXCHANGE_RATE_LIST:
        {            
            Pdivoom_exchange_rate_pic_respone_para pret_info = (Pdivoom_exchange_rate_pic_respone_para)pnet_info->cmd_data;
            
            divoom_exchange_pic_update_list(pret_info->plist, pret_info->num);
        }
            break;

		case DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST:
			{
				Pdivoom_team_img_respone_para pret_info = (Pdivoom_team_img_respone_para)pnet_info->cmd_data;
	            divoom_nba_pic_update_list(pret_info->plist, pret_info->num, pret_info->team_type);
			}
			break;

        case DIVOOM_NET_COMM_CHANNEL_GET_CLOCK_HISTORY:
			break;
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_HISTORY;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_CHANNEL_CONFIG:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_CHANNEL_GLOBAL_CONFIG;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            if(pnet_info->type == DIVOOM_APP_TOPIC)
            {
                divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_CHANNEL_CONFIG, DIVOOM_SERVER_TOPIC, (void *)pnet_info->cmd_data);
            }
            break;
			
        case DIVOOM_NET_COMM_CHANNEL_GET_CHANNEL_CONFIG:
        {            
            divoom_channel_channel_cfg_info pconfig_info;

            divoom_light_app_get_change_clock_config(&pconfig_info);
            divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_GET_CHANNEL_CONFIG, DIVOOM_APP_TOPIC, (void *)&pconfig_info);
            break;

            
        }
			
            
        case DIVOOM_NET_COMM_CHANNEL_SET_SUBSCRIBE_CONFIG:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_SUBCRIBE_INFO;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_PLAY_CUSTOM_ID:
        {
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_PLAY_USER_DEFINE;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			//divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_START_UP_CHANNEL:
        {
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_START_UP_CHANNEL;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            if(pnet_info->type == DIVOOM_APP_TOPIC)
            {
                divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_START_UP_CHANNEL, DIVOOM_SERVER_TOPIC, (void *)pnet_info->cmd_data);
            }
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_GET_START_UP_CHANNEL:
        {
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_GET_START_UP_CHANNEL;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
            
        //音乐律动模块
        case DIVOOM_NET_COMM_CHANNEL_GET_EQ_LIST:
        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_EQ_LIST;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_ADD_EQ_ITEM:
        {
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ADD_EQ_ITEM;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_GET_EQ_POS:
        {
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_GET_EQ_POS;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_GET_EQ_POS, DIVOOM_APP_TOPIC, (void *)&model_msg.result);
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_EQ_POS:
        {
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_EQ_POS;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            if(pnet_info->type == DIVOOM_APP_TOPIC)
            {
                pdivoom_net_app_handle->light_config_up = 1;
				divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_EQ_POS, DIVOOM_APP_TOPIC, (void *)pnet_info->cmd_data);
            }
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_DEL_EQ:
        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DEL_EQ_POS;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;

        case DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_APP_DATA:
        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_APP;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_SYS_DATA:
        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_SYS;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA:
        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_ALL;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;

        case DIVOOM_NET_COMM_DEVICE_GET_LOCAL_CLOCK_INFO:
            if(pnet_info->cmd_data)
            {
                Pdivoom_clock_id_info pcid = (Pdivoom_clock_id_info)pnet_info->cmd_data;
                int cid = pcid->clock_id;
                char *resp;

                if(cid < 0)
                {
                    cid = divoom_light_app_get_cur_clock_id();
                    if(cid <= 0)
                    {
                        resp = divoom_watchface_local_http_error_json(
                            "Device/GetLocalClockInfo", "no active display clock");
                        if(resp)
                        {
                            divoom_http_server_send_command(
                                DIVOOM_NET_COMM_DEVICE_GET_LOCAL_CLOCK_INFO, resp);
                            divoom_system_free(resp);
                            return;
                        }
                        break;
                    }
                }
                resp = divoom_watchface_local_http_get_full_json(cid);
                if(resp)
                {
                    divoom_http_server_send_command(DIVOOM_NET_COMM_DEVICE_GET_LOCAL_CLOCK_INFO, resp);
                    divoom_system_free(resp);
                    return;
                }
            }
            break;

        case DIVOOM_NET_COMM_DEVICE_PATCH_LOCAL_CLOCK_INFO:
            if(pnet_info->cmd_data)
            {
                Pdivoom_patch_local_holder h = (Pdivoom_patch_local_holder)pnet_info->cmd_data;
                if(h->raw_json)
                {
                    char *resp = divoom_watchface_local_http_patch_response_json(h->raw_json, NULL, 0);
                    if(resp)
                    {
                        divoom_http_server_send_command(DIVOOM_NET_COMM_DEVICE_PATCH_LOCAL_CLOCK_INFO, resp);
                        divoom_system_free(resp);
                        return;
                    }
                }
            }
            break;

        case DIVOOM_NET_COMM_DEVICE_GET_LOCAL_FONT_LIST:
            {
                char *resp = divoom_watchface_local_http_get_font_list_full_json();
                if(resp)
                {
                    divoom_http_server_send_command(DIVOOM_NET_COMM_DEVICE_GET_LOCAL_FONT_LIST, resp);
                    divoom_system_free(resp);
                    return;
                }
            }
            break;

        case DIVOOM_NET_COMM_DEVICE_GET_STORE_CLOCK_MARKET_LIST:
            {
                char *resp = divoom_watchface_local_http_get_store_clock_market_list_json();
                if(resp)
                {
                    divoom_http_server_send_command(
                        DIVOOM_NET_COMM_DEVICE_GET_STORE_CLOCK_MARKET_LIST, resp);
                    divoom_system_free(resp);
                    return;
                }
            }
            break;

        case DIVOOM_NET_COMM_DEVICE_RESET_LOCAL_CLOCK_FROM_SERVER:
            if(pnet_info->cmd_data)
            {
                Pdivoom_clock_id_info pcid = (Pdivoom_clock_id_info)pnet_info->cmd_data;
                divoom_clock_id_info req = {0};
                int sync_ok = 0;
                char *resp;

                if(pcid->clock_id <= 0)
                {
                    resp = divoom_watchface_local_http_error_json(
                        "Device/ResetLocalClockFromServer", "invalid ClockId");
                    if(resp)
                    {
                        divoom_http_server_send_command(
                            DIVOOM_NET_COMM_DEVICE_RESET_LOCAL_CLOCK_FROM_SERVER, resp);
                        divoom_system_free(resp);
                        return;
                    }
                    break;
                }

                divoom_clock_manage_reset_local_sys_clock_disk(pcid->clock_id);

                req.clock_id = pcid->clock_id;
                req.clock_pos = pcid->clock_pos;
                req.parent_id = pcid->parent_id;

                if(divoom_net_app_net_is_ok())
                {
                    divoom_net_app_send_packet(
                        DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA,
                        DIVOOM_SERVER_TOPIC, &req);
                    sync_ok = 1;
                }

                resp = divoom_watchface_local_http_reset_from_server_response_json(
                    pcid->clock_id, sync_ok, NULL);
                if(resp)
                {
                    divoom_http_server_send_command(
                        DIVOOM_NET_COMM_DEVICE_RESET_LOCAL_CLOCK_FROM_SERVER, resp);
                    divoom_system_free(resp);
                    return ;
                }
            }
            break;

        case DIVOOM_NET_COMM_SEND_GET_DEVICE_CLOCK_CONFIG:
            divoom_clock_manage_get_all_default_clock_list_time((Pdivoom_devcice_time_dial_clock_list_respone)pnet_info->cmd_data);
            break;
        
        case DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_SYS_LIST:
            divoom_clock_manage_update_sys_clock_list_time((Pdivoom_devcice_time_dial_clock_list_respone)pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_LIST:
        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_CLOCK_LIST;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_CLOCK_SELECT_ID:
        {
            Pdivoom_channel_select_clock_index select_clock = (Pdivoom_channel_select_clock_index)pnet_info->cmd_data;
            divoom_clock_id_info clock_info = {0};
            
            clock_info.clock_id = select_clock->clock_id;

            #if 0
            if(clock_info.clock_id > 10000 && clock_info.clock_id < DIVOOM_PHOTO_START_CLOCK_ID)
            {
                //这个位置是用户自定义表盘,要重新获取一下表盘信息
                divoom_light_app_reload_all_clock_info(clock_info.clock_id);
            }
            #endif       
            divoom_cache_clock_data_remove(select_clock->clock_id, 0, 0);
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_NET_SET_CLOCK_INDEX;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            if(pnet_info->type == DIVOOM_APP_TOPIC)
            {
                pdivoom_net_app_handle->light_config_up=  1;
                divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_CLOCK_SELECT_ID, DIVOOM_SERVER_TOPIC, (void *)&select_clock->clock_id);
            }
            divoom_clock_control_request_quick(&clock_info);
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_CLOCK_CONFIG:
        {
            int *clock_id = (int *)pnet_info->cmd_data;

            divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_CLOCK_CONFIG, DIVOOM_SERVER_TOPIC, (void *)clock_id);
        }
            break;

            
        case DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_APP_PIC:
        {
            Pdivoom_devcice_time_dial_app_pic_respone plist = (Pdivoom_devcice_time_dial_app_pic_respone)pnet_info->cmd_data;
            
            divoom_app_pic_manage_update(plist->plist, plist->num);
        }
            break;
            
        case DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_FONT_LIST:
        {
            Pdivoom_devcice_time_dial_font_list_respone pfont_list = (Pdivoom_devcice_time_dial_font_list_respone)pnet_info->cmd_data;
            
            divoom_system_font_update(pfont_list->pfont_list, pfont_list->font_num);
        }
            break;
            
	case DIVOOM_NET_COMM_GET_FAVORITE_LIST:
        {
            uint32 before_total = divoom_net_app_get_transfer_total();

            divoom_subscribe_manage_transfer_response_begin(
                DIVOOM_NET_COMM_GET_FAVORITE_LIST, pnet_info->cmd_flag);
            divoom_favorite_manage_update_list((Pdivoom_devcice_image_list)pnet_info->cmd_data);
            divoom_subscribe_manage_transfer_response_end();
            divoom_net_app_submit_reload_result(
                DIVOOM_NET_COMM_GET_FAVORITE_LIST, before_total);
        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_GET_COLCK_INFO:
        {
            divoom_channel_get_clock_info clock_info;

            clock_info.index = divoom_light_app_get_cur_clock_id();
            if(divoom_light_app_get_screen_status(0xFF))
            {
                clock_info.bright = divoom_main_get_sys_light();
            }
            else
            {
                clock_info.bright = 0;
            }
            divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_GET_COLCK_INFO, DIVOOM_APP_TOPIC, (void *)&clock_info);
        }
            break;

        case DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS:
        case DIVOOM_NET_COMM_CHANNEL_SET_BRIGHTNESS:
        case DIVOOM_NET_COMM_IFFT_ACTION_SET_BRIGHTNESS:
		case DIVOOM_NET_COMM_LAMDA_BRIGHTNESS_ADJUST:
            if(pnet_info->cmd_data && *(int *)pnet_info->cmd_data >= 0)
	        {
				model_msg.dest_id = DIVOOM_MODEL_MAIN; 
				model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_SYS_LIGHT;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
                if(pnet_info->type == DIVOOM_APP_TOPIC)
                {
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)pnet_info->cmd_data);
                }
	        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_PRODUCE_TIME:
	        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_PRODUCE_TIME;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
                if(pnet_info->type == DIVOOM_APP_TOPIC)
                {
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_PRODUCE_TIME, DIVOOM_SERVER_TOPIC, (void *)pnet_info->cmd_data);
                }
	        }
            break;
            
           
        case DIVOOM_NET_COMM_CHANNEL_SET_CUSTOM:
	        {
                Pdivoom_devcice_user_define_item pimage_file = (Pdivoom_devcice_user_define_item)pnet_info->cmd_data;
                
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ADD_USER_DEFINE_INDEX;
	            model_msg.data = (uint8 *)pimage_file;
				divoom_sys_model_crl_msg_push(&model_msg);
	        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_DELETE_CUSTOM:
	        {
                Pdivoom_devcice_user_define_item pimage_file = (Pdivoom_devcice_user_define_item)pnet_info->cmd_data;
                
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DEL_USER_DEFINE_ONE;
	            model_msg.data = (uint8 *)pimage_file;
				divoom_sys_model_crl_msg_push(&model_msg);
	        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_CLEAN_CUSTOM:
	        {
                Pdivoom_devcice_user_define_item pimage_file = (Pdivoom_devcice_user_define_item)pnet_info->cmd_data;
                
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DEL_USER_DEFINE_INDEX;
	            model_msg.data = (uint8 *)pimage_file;
				divoom_sys_model_crl_msg_push(&model_msg);
	        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_GET_CUSTOM_TIME:
	        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_GET_CUSTOM_TIME;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
	            divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_GET_CUSTOM_TIME, DIVOOM_APP_TOPIC, (void *)&model_msg.result);
	        }
            break;
            
        case DIVOOM_NET_COMM_CHANNEL_SET_CUSTOM_TIME:
	        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CUSTOM_TIME;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
                if(pnet_info->type == DIVOOM_APP_TOPIC)
                {
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_CUSTOM_TIME, DIVOOM_SERVER_TOPIC, (void *)pnet_info->cmd_data);
                }
	        }
            break;
           
        case DIVOOM_NET_COMM_GET_TOOL_STOP_WATCH_STATUS:
	        {
				int watch_status;
				divoom_app_tool_time_watch_info info = {0};
				model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
				model_msg.msg_id = DIVOOM_APP_TIME_WATCH_GET;
				model_msg.data = (uint8 *)&info;
				divoom_sys_model_crl_msg_push(&model_msg);
				watch_status = info.ctrl_flag;
				divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_TOOL_STOP_WATCH_STATUS, DIVOOM_APP_TOPIC, (void *)&watch_status);			
	        }
            break;
        
        case DIVOOM_NET_COMM_SET_TOOL_STOP_WATCH_STATUS:
			{
				divoom_app_tool_time_watch_info info = {0};
				int *pstatus = (int *)pnet_info->cmd_data;
				info.ctrl_flag = *pstatus;
				model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
				model_msg.msg_id = DIVOOM_APP_TIME_WATCH_SET;
	            model_msg.data = (uint8 *)&info;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
            break;


		case DIVOOM_NET_COMM_GET_TOOL_TIMER_STATUS:
			{
				divoom_app_tool_count_time_down_info info = {0};
				divoom_system_tool_json_timer_info send_info = {0};
				model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
				model_msg.msg_id = DIVOOM_APP_COUNT_TIME_DOWN_GET;
	            model_msg.data = (uint8 *)&info;
				divoom_sys_model_crl_msg_push(&model_msg);
				send_info.min = (int)info.hour * 60 + info.minute;
				send_info.sec = info.second;
				send_info.status = info.status;
	            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_TOOL_TIMER_STATUS, DIVOOM_APP_TOPIC, (void *)&send_info);		
			}
			break;



		case DIVOOM_NET_COMM_SET_TOOL_TIMER_STATUS:
			{
				divoom_app_tool_count_time_down_info info = {0};
				Pdivoom_system_tool_json_timer_info ptmp_info = (Pdivoom_system_tool_json_timer_info)pnet_info->cmd_data;
				info.minute = ptmp_info->min;
				info.second = ptmp_info->sec;
				info.status = ptmp_info->status;
				model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
				model_msg.msg_id = DIVOOM_APP_COUNT_TIME_DOWN_SET;
	            model_msg.data = (uint8 *)&info;
				divoom_sys_model_crl_msg_push(&model_msg);			
			}
			break;
        
        case DIVOOM_NET_COMM_GET_TOOL_SCORE_BOARD_STATUS:
        {
            divoom_system_tool_json_score_board score_info = {0};
			divoom_app_tool_score_info info = {0};
			model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
			model_msg.msg_id = DIVOOM_APP_SCORE_GET;
			model_msg.data = (uint8 *)&info;
			divoom_sys_model_crl_msg_push(&model_msg);
			score_info.red_score = info.red_score;
			score_info.blue_score = info.blue_score;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_TOOL_SCORE_BOARD_STATUS, DIVOOM_APP_TOPIC, (void *)&score_info);
        }
            break;
        case DIVOOM_NET_COMM_SET_TOOL_SCORE_BOARD_STATUS:
			{
				divoom_app_tool_score_info msg_data = {0};
				divoom_system_tool_json_score_board *net_info = (divoom_system_tool_json_score_board *)pnet_info->cmd_data;
				msg_data.blue_score = net_info->blue_score;
				msg_data.red_score = net_info->red_score;
				model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
				model_msg.msg_id = DIVOOM_APP_SCORE_SET;
	            model_msg.data = (uint8 *)&msg_data;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
            break;

        
        case DIVOOM_NET_COMM_GET_TOOL_NOISE_STATUS:
        	{
				int noise_status;
				uint8 status;
				model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
				model_msg.msg_id = DIVOOM_APP_NOISE_GET;
				model_msg.data = (uint8 *)&status;
				divoom_sys_model_crl_msg_push(&model_msg);
				noise_status = status;
				divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_TOOL_NOISE_STATUS, DIVOOM_APP_TOPIC, (void *)&noise_status);
        	}
            break;
        
		case DIVOOM_NET_COMM_SET_TOOL_NOISE_STATUS:
			{
				divoom_app_tool_noise_info info = {0};
				int *statu = (int *)pnet_info->cmd_data;
				info.ctrl_flag = *statu;
				model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
				model_msg.msg_id = DIVOOM_APP_NOISE_SET;
	            model_msg.data = (uint8 *)&info;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
			break;

        case DIVOOM_NET_COMM_MIXER_START:
        {
            model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
            model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_MIXER_START;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;
        }

        case DIVOOM_NET_COMM_MIXER_STOP:
        {
            model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
            model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_MIXER_STOP;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;
        }

        case DIVOOM_NET_COMM_MIXER_SET:
        {
            Pdivoom_mixer_set_json_info mixer_info = (Pdivoom_mixer_set_json_info)pnet_info->cmd_data;
            uint8 mixer_data[3];

            if(mixer_info != NULL)
            {
                mixer_data[0] = mixer_info->mode;
                mixer_data[1] = mixer_info->button_select;
                mixer_data[2] = mixer_info->on_off;
                model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
                model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_MIXER_SET;
                model_msg.data = mixer_data;
                divoom_sys_model_crl_msg_push(&model_msg);
            }
            break;
        }

        case DIVOOM_NET_COMM_MIXER_SET_NEW:
        {
            Pdivoom_mixer_set_new_json_info mixer_info = (Pdivoom_mixer_set_new_json_info)pnet_info->cmd_data;
            uint8 mixer_data[2];

            if(mixer_info != NULL)
            {
                mixer_data[0] = mixer_info->mode;
                mixer_data[1] = mixer_info->music_index;
                model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
                model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_MIXER_SET_NEW;
                model_msg.data = mixer_data;
                divoom_sys_model_crl_msg_push(&model_msg);
            }
            break;
        }

        case DIVOOM_NET_COMM_GET_USER_DEFINE_LIST:
        {
            Pdivoom_devcice_user_define_respone plist = (Pdivoom_devcice_user_define_respone)pnet_info->cmd_data;
            
            divoom_user_define_ctrl_update(plist);
        }
            break;
            
            
        case DIVOOM_NET_COMM_GET_EXPERT_LAST:
        {
            divoom_expert_manage_update_last_image((Pdivoom_devcice_image_list)pnet_info->cmd_data);
        }
            break;

        case DIVOOM_NET_COMM_UPDATE_EXPERT_LAST:
        {
            divoom_expert_manage_update_last_image((Pdivoom_devcice_image_list)pnet_info->cmd_data);
        }
            break;
            
        case DIVOOM_NET_COMM_GET_EXPERT_LIST:
        {
            uint32 before_total = divoom_net_app_get_transfer_total();

            divoom_subscribe_manage_transfer_response_begin(
                DIVOOM_NET_COMM_GET_EXPERT_LIST, pnet_info->cmd_flag);
            divoom_expert_manage_update_list_image((Pdivoom_devcice_expert_list)pnet_info->cmd_data);
            divoom_subscribe_manage_transfer_response_end();
            divoom_net_app_submit_reload_result(
                DIVOOM_NET_COMM_GET_EXPERT_LIST, before_total);
        }
            break;

		case DIVOOM_NET_COMM_IFFT_ACTION_OPEN_COUNTDOWN:
		{
			divoom_app_tool_count_time_down_info info = {0};
			info.minute = *(int *)pnet_info->cmd_data;
			info.second = 0;
			info.status = 1;
			model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
			model_msg.msg_id = DIVOOM_APP_COUNT_TIME_DOWN_SET;
			model_msg.data = (uint8 *)&info;
			divoom_sys_model_crl_msg_push(&model_msg);	
		}
			break;
			
        case DIVOOM_NET_COMM_IFFT_ACTION_OPEN_CLOCK:
        {
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
        
        case DIVOOM_NET_COMM_IFFT_ACTION_OPEN_TIMER: 
        {
			divoom_app_tool_time_watch_info info = {0};
			info.ctrl_flag = 1; //打开
			model_msg.dest_id = DIVOOM_MODEL_APP_TOOL; 
			model_msg.msg_id = DIVOOM_APP_TIME_WATCH_SET;
            model_msg.data = (uint8 *)&info;
			divoom_sys_model_crl_msg_push(&model_msg);			
        }
            break;
        
        case DIVOOM_NET_COMM_IFFT_ACTION_SCREEN_CTRL:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;

        case DIVOOM_NET_COMM_IFFT_ACTION_OPEN_SCREEN: 
        {
			int switch_flag = 1;
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
			model_msg.data = (uint8 *)&switch_flag;
			divoom_sys_model_crl_msg_push(&model_msg);
        }
			break;
		
    	case DIVOOM_NET_COMM_IFFT_ACTION_CLOSE_SCREEN:
    	{
			int switch_flag = 0;
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
			model_msg.data = (uint8 *)&switch_flag;
			divoom_sys_model_crl_msg_push(&model_msg);
    	}
			break;
        
        case DIVOOM_NET_COMM_IFFT_ACTION_PLAY_NET_GIF:
        {
			divoom_draw_remote_json_image_info image_info;

            strcpy(image_info.file_id, pnet_info->cmd_data);
            image_info.flag = 0;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
  		    model_msg.data = (uint8 *)&image_info;	
  			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DRAW_REMOTE;
	  		divoom_sys_model_crl_msg_push(&model_msg);
        }
            break;
            
    	case DIVOOM_NET_COMM_IFFT_ACTION_DISPLAY_MESSAGE:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DISPLAY_IFFFT_TEXT;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;

		case DIVOOM_NET_COMM_IFFT_ACTION_DISPLAY_AMAZON_MUSIC:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DISPLAY_IFFFT_AMAZON_MUSIC;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
            
    	case DIVOOM_NET_COMM_IFFT_ACTION_PLAY_USER_DEFINE:
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_USER_DEFINE_INDEX;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
    	case DIVOOM_NET_COMM_IFFT_ACTION_PLAY_SUBSCRIBE:
    	{
            int channel_index = 1;
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CHANNEL_INDEX;
			model_msg.data = (uint8 *)&channel_index;
			divoom_sys_model_crl_msg_push(&model_msg);
    	}
            break;

		case DIVOOM_NET_COMM_IFFT_ACTION_SWITCH_CLOCK:
		{
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
		}
			break;	

		case DIVOOM_NET_COMM_LAMDA_SCREEN_CTRL:
		case DIVOOM_NET_COMM_LAMDA_DIVOOM_DEVICE_CONTROL:
		{
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
		}
			break;

		case DIVOOM_NET_COMM_LAMDA_LIGHT_CTRL:
		{
			int ctrl = *(int *)pnet_info->cmd_data;
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			if (ctrl == 1)
				model_msg.msg_id = DIVOOM_MAIN_EVENT_ADD_SYS_LIGHT;
			else
				model_msg.msg_id = DIVOOM_MAIN_EVENT_DEC_SYS_LIGHT;
			divoom_sys_model_crl_msg_push(&model_msg);
		}
			break;

		case DIVOOM_NET_COMM_LAMDA_SWITCH_CLOCK:
		{
			int clock_id = *(int *)pnet_info->cmd_data;
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			if (clock_id == -1)
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_NEXT_CLOCK;
			else
			{
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
            	model_msg.data = (uint8 *)pnet_info->cmd_data;
			}
			divoom_sys_model_crl_msg_push(&model_msg);
		}
			break;

		case DIVOOM_NET_COMM_LAMDA_DISPLAY_PIC:
		{
			divoom_draw_remote_json_image_info image_info;

            strcpy(image_info.file_id, pnet_info->cmd_data);
            image_info.flag = 0;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
  		    model_msg.data = (uint8 *)&image_info;	
  			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DRAW_REMOTE;
	  		divoom_sys_model_crl_msg_push(&model_msg);
		}
			break;
			
        case DIVOOM_NET_COMM_DEVICE_GET_USERLIST_NAME:
//            divoom_disp_divoom_subcribe_update_user_name(pnet_info->cmd_data);
            break;

        case DIVOOM_NET_COMM_GET_EXPERT_PIC_LIST:
        case DIVOOM_NET_COMM_GET_CATEGORY_FILE_LIST_V2:
//            divoom_disp_divoom_subcribe_update_file_list(pnet_info->cmd_data);
            break;
            
        case DIVOOM_NET_COMM_SET_SYS_NIGHT_VIEW:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_DEVICE_NIGHT_VIEW;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_GET_SYS_NIGHT_VIEW:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_INIT_DEVICE_NIGHT_VIEW;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_GET_DEVICE_PASSWORD:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_GET_DEVICE_PASSWORD;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
        case DIVOOM_NET_COMM_DEVICE_HOST_LIKE_GALLERY:
            divoom_favorite_manage_update_item((Pdivoom_devcice_image_file)pnet_info->cmd_data);
            break;
            
		case DIVOOM_NET_COMM_DEVICE_BOND:
			{
                int old_gif_mode;
                Pdivoom_bond_user_para bond_info = (Pdivoom_bond_user_para)pnet_info->cmd_data;
				#if 0
				//服务器回应绑定成功，通知app状态
				i = DIVOOM_WIFI_CONNECTED_SER;
				divoom_ble_wifi_status_notify(WIFI_STATUS_BLE_COMMAND, 1, (uint8 *)&i);
				//退出蓝牙模式
				#endif
                old_gif_mode = divoom_main_change_to_gif_mode(-1);
                divoom_main_change_to_gif_mode(bond_info->gif_mode);
                if(bond_info->gif_mode == 1)
                {
                }
                else
                {                    
                    if(bond_info->app_id == 1 && old_gif_mode == 0)
                    {
                        divoom_print("new host, we will clear favorite\n");                    
                        divoom_sys_model_crl_send_all_model(DIVOOM_DEVICE_SYSTEM_RESET_ALL);   
                    }
                    else
                    {                    
                        divoom_print("old host, we will pass to del favorite\n");
                    }
                }

                //divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_APP_ID, DIVOOM_SERVER_TOPIC,NULL);
			}
			break;

        case DIVOOM_NET_COMM_DEVICE_CON_SRV:
        {
            Pdivoom_devcice_con_respone_json pdevice_info = (Pdivoom_devcice_con_respone_json)pnet_info->cmd_data;
    		divoom_sys_model_crl_send_asyn_all_model(DIVOOM_DEVICE_CON_SRV_ID);

            if(pdevice_info && pdevice_info->version <= DIVOOM_SOFTWARE_VERSION)
            {
                //没有升级时获取一下杰理的
                divoom_main_check_jl_update_version();
            }
        }
            break;

		case DIVOOM_NET_COMM_GET_DEVICE_NAME:
		case DIVOOM_NET_COMM_SET_DEVICE_NAME:
			/* Device/SetName：与 SetBlueName 同路径，下发 JL 蓝牙名并落本地 blue_name */
		case DIVOOM_NET_COMM_SET_BLUE_NAME:
			model_msg.dest_id = DIVOOM_MODEL_MAIN;
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_BLUE_NAME;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_GET_BLUE_NAME:
			model_msg.dest_id = DIVOOM_MODEL_MAIN;          
            model_msg.msg_id = DIVOOM_MAIN_EVENT_GET_BLUE_NAME; 
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_GET_DEVICE_ID:
			model_msg.dest_id = DIVOOM_MODEL_MAIN;          
            model_msg.msg_id = DIVOOM_MAIN_EVENT_GET_DEVICE_ID; 
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_APP_DELUSER:
			break;
		case DIVOOM_NET_COMM_APP_UNBIND:
			model_msg.msg_type = 1;
			model_msg.dest_id = DIVOOM_MODEL_MAIN;
	  		model_msg.msg_id = DIVOOM_MAIN_EVENT_APP_UNBIND;
	  		divoom_sys_model_crl_msg_push(&model_msg);
			break;
            
    	case DIVOOM_NET_COMM_GET_APP_ID:
            model_msg.dest_id = DIVOOM_MODEL_MAIN;          
            model_msg.msg_id = DIVOOM_MAIN_EVENT_PUSH_APP_ID; 
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;
            
    	case DIVOOM_NET_COMM_PUSH_APP_ID:
            model_msg.dest_id = DIVOOM_MODEL_MAIN;          
            model_msg.msg_id = DIVOOM_MAIN_EVENT_UPDATE_APP_ID; 
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;

        
        case DIVOOM_NET_COMM_GET_ALARM:
            model_msg.dest_id = DIVOOM_MODEL_ALARM;
            model_msg.msg_id = DIVOOM_ALARM_UPDATE_ALARM;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            
            break;
            
            
        
        case DIVOOM_NET_COMM_SET_ALARM:
            model_msg.dest_id = DIVOOM_MODEL_ALARM;
            model_msg.msg_id = DIVOOM_ALARM_SET_ALARM;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            
            break;
            
        case DIVOOM_NET_COMM_DEL_ALARM:
            model_msg.dest_id = DIVOOM_MODEL_ALARM;          
            model_msg.msg_id = DIVOOM_ALARM_DEL_ALARM; 
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;

            
        case DIVOOM_NET_COMM_LISTEN_ALARM:
            model_msg.dest_id = DIVOOM_MODEL_ALARM;          
            model_msg.msg_id = DIVOOM_ALARM_LISTEN_ALARM; 
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;

		case DIVOOM_NET_COMM_LISTEN_ALARM_SET_VOLUME:
            model_msg.dest_id = DIVOOM_MODEL_ALARM;          
            model_msg.msg_id = DIVOOM_ALARM_LISTEN_ALARM_SET_VOLUMN; 
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;

		case DIVOOM_NET_COMM_GET_MEMORIAL:
            model_msg.dest_id = DIVOOM_MODEL_ALARM;
            model_msg.msg_id = DIVOOM_ALARM_UPDATE_MEMORIAL;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
			break;
            
		case DIVOOM_NET_COMM_SET_MEMORIAL:
            model_msg.dest_id = DIVOOM_MODEL_ALARM;
            model_msg.msg_id = DIVOOM_ALARM_SET_MEMORIAL;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
			break;
            
		case DIVOOM_NET_COMM_DEL_MEMORIAL:
            model_msg.dest_id = DIVOOM_MODEL_ALARM;
            model_msg.msg_id = DIVOOM_ALARM_DEL_MEMORIAL;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
			break;

        
            /*
		case DIVOOM_NET_COMM_LED_SEND_DATA:			
		{
			Pdivoom_led_edit_info pled_info = (Pdivoom_led_edit_info)pnet_info->cmd_data;

            if(divoom_disp_ctrl_get_mode() != DIVOOM_DISP_MOVIE_CTRL)
            {
                divoom_print("%s:we will pass the movie data!\n",__FUNCTION__);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_LED_STOP, DIVOOM_APP_TOPIC, &pled_info->user_id);
            }
            else
            {
                divoom_disp_word_update_base_color(pled_info->led_data, pled_info->led_len);
            }
			break;
		}
        
		case DIVOOM_NET_COMM_LED_SET_SPEED:
		{
			int speed = *(int *)pnet_info->cmd_data;
            
			divoom_disp_ctrl_word_update_speed(speed);
			break;
		}
        
		case DIVOOM_NET_COMM_LED_SET_TEXT:
		{			
			Pdivoom_led_edit_set_text_info text_info = (Pdivoom_led_edit_set_text_info)pnet_info->cmd_data;
            divoom_disp_word_disp_font_info word_info = {0};

            word_info.user_id = text_info->user_id;
            word_info.len = text_info->text_num;
            word_info.speed = text_info->speed;
            memcpy(word_info.word_info, text_info->text_list, word_info.len << 1);
            word_info.word_color.r_c = 0xFF;
            word_info.word_color.g_c = 0xFF;
            word_info.word_color.b_c = 0xFF;
            word_info.back_color.r_c = 0;
            word_info.back_color.g_c = 0;
            word_info.back_color.b_c = 0;
            word_info.light = divoom_disp_ctrl_get_light();
            word_info.cyc_flag = T_U32_MAX;
            
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			//根据是16*16,还是16*48决定使用 DIVOOM_LIGHT_APP_EVENT_SMALL_DRAW 或者 DIVOOM_LIGHT_APP_EVENT_BIG_PLAY;
	  		//model_msg.msg_id = DIVOOM_DISP_MOVIE_CTRL;			
	  		model_msg.data = (uint8 *)&word_info;
	  		divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}

    	case DIVOOM_NET_COMM_LED_STOP:
            if(divoom_disp_ctrl_get_mode() == DIVOOM_DISP_MOVIE_CTRL)
            {
                divoom_disp_word_reset_user_id();
                model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
				//根据是16*16,还是16*48决定使用 DIVOOM_LIGHT_APP_EVENT_SMALL_DRAW 或者 DIVOOM_LIGHT_APP_EVENT_BIG_PLAY;
                //model_msg.msg_id = DIVOOM_LIGHT_EXIT_CUR_MODE;
                //model_msg.msg_id = DIVOOM_LIGHT_EXIT_CUR_MODE;    
                //model_msg.attr = DIVOOM_LIGHT_APP_CTRL_NORMAL_1;
                divoom_sys_model_crl_msg_push(&model_msg);
            }
            break;

		//发送留言文字不要了
		case DIVOOM_NET_COMM_LED_SEND_TEXT:
        {
        }
            break;*/

        
        /*
		//音乐部分不需要了
		case DIVOOM_NET_COMM_SET_SOND_DIS_CFG:
        {
        }*/
        
		case DIVOOM_NET_COMM_SET_SYS_LOGO:
    	{	
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_ON_PIC;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
    	}
        

        case DIVOOM_NET_COMM_SET_DEVICE_LOCATION:
    	{	
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_DEVICE_LOCATION;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
    	}
        
        case DIVOOM_NET_COMM_SET_DEVICE_TIME_ZONE:
    	{	
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_TIMEZONE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
    	}

		case DIVOOM_NET_COMM_SET_DEVICE_TIME:
		{
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_TIME;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
		
        case DIVOOM_NET_COMM_GET_SYS_BRIGHTNESS:
        {
            int sys_light;

            if(divoom_light_app_get_screen_status(0xFF))
                sys_light = divoom_main_get_sys_light();
            else
                sys_light = 0;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_SYS_BRIGHTNESS, DIVOOM_APP_TOPIC, (void *)&sys_light);
        }
            break;
        /*
		case DIVOOM_NET_COMM_SET_SYS_AUTO_POWER_OFF:
		{	
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_TIME;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
		*/
        
        case DIVOOM_NET_COMM_GET_SYS_CONF:
        {
            divoom_channel_channel_cfg_info channel_info;
            Pdivoom_system_setting_json_system_cfg setting = (Pdivoom_system_setting_json_system_cfg)pnet_info->cmd_data;
            
            channel_info.CurClockId = setting->CurClockId;
            channel_info.RotationFlag = setting->RotationFlag;
            channel_info.ClockTime = setting->ClockTime;
            channel_info.GalleryTime = setting->GalleryTime;
            channel_info.SingleGalleyTime = setting->SingleGalleyTime;
            channel_info.ChannelIndex = setting->ChannelIndex;
            channel_info.GalleryShowTimeFlag = setting->GalleryShowTimeFlag;
            channel_info.PowerOnClockId = setting->PowerOnClockId;
            channel_info.StartUpClockOnOff = setting->StartUpClockOnOff;
            
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_CHANNEL_GLOBAL_CONFIG;
            model_msg.data = (uint8 *)&channel_info;
			divoom_sys_model_crl_msg_push(&model_msg);
            
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_ALL_MODE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;
        }
        case DIVOOM_NET_COMM_SET_SYS_CONF:
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_ALL_MODE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;

		case DIVOOM_NET_COMM_SET_SYS_POWERON_MODE:
			{
				uint8 *power_on_mode = (uint8 *)divoom_malloc(sizeof(uint8), DIVOOM_MODEL_NET, __LINE__, __FILE__);

				*power_on_mode = *((uint8 *)pnet_info->cmd_data);
				//做异步处理,避免阻塞这里
				model_msg.msg_type = 1;
				model_msg.dest_id = DIVOOM_MODEL_MAIN; 
				model_msg.msg_id = DIVOOM_MAIN_SET_POWERON_MODE;
				model_msg.data = power_on_mode;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
			break;

		case DIVOOM_NET_COMM_SEND_CURRENT_WEATHER:
		{
			Pdivoom_disp_weather_info weather_info = (Pdivoom_disp_weather_info)pnet_info->cmd_data;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
  		    model_msg.data = (uint8 *)weather_info;	
  			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_CURRENT_WEATHER;
	  		divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}

		case DIVOOM_NET_COMM_SEND_FUTURE_WEATHER:
		{
			Pdivoom_disp_weather_info weather_info = (Pdivoom_disp_weather_info)pnet_info->cmd_data;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
  		    model_msg.data = (uint8 *)weather_info;	
  			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_FUTURE_WEATHER;
	  		divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
			
		case DIVOOM_NET_COMM_GET_SLEEP_CONF:
		{
			divoom_system_tool_json_get_sleep psleep_info;

            divoom_sleep_get(&psleep_info);
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_SLEEP_CONF, DIVOOM_APP_TOPIC, (void *)&psleep_info);
            break;
		}
		case DIVOOM_NET_COMM_SET_SLEEP_CONF:
		{
            model_msg.msg_type = 1;
			model_msg.dest_id = DIVOOM_MODEL_SLEEP;
			model_msg.msg_id = DIVOOM_SLEEP_UPDATE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
            pnet_info->cmd_data = NULL;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
        
		case DIVOOM_NET_COMM_TEST_SLEEP:
		{
            model_msg.msg_type = 1;
			model_msg.dest_id = DIVOOM_MODEL_SLEEP;
			model_msg.msg_id = DIVOOM_SLEEP_TEST_MODE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
            pnet_info->cmd_data = NULL;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
		case DIVOOM_NET_COMM_EXIT_TEST_SLEEP:
		{
            model_msg.msg_type = 1;
			model_msg.dest_id = DIVOOM_MODEL_SLEEP;
			model_msg.msg_id = DIVOOM_SLEEP_EXIT_TEST_MODE;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
		
		case DIVOOM_NET_COMM_DRAWING:
		{
            /*
			Pdivoom_drawing_json_image_info image_info = (Pdivoom_drawing_json_image_info)pnet_info->cmd_data;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
  		    model_msg.data = (uint8 *)image_info;	
  			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_DATA_MODE;
	  		divoom_sys_model_crl_msg_push(&model_msg);
            //divoom_system_free(pic_data);
            */
			break;
		}

		case DIVOOM_NET_COMM_REAL_EQ_DRAWING_BLE:
        case DIVOOM_NET_COMM_REAL_EQ_DRAWING:
		{
			Pdivoom_draw_local_json_image_info image_info = (Pdivoom_draw_local_json_image_info)pnet_info->cmd_data;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
  		    model_msg.data = (uint8 *)image_info;	
  			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ADD_REAL_EQ_DATA;
	  		divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}

        case DIVOOM_NET_COMM_CHANNEL_SET_TMP_EQ:
		{
			Pdivoom_draw_remote_json_image_info image_info = (Pdivoom_draw_remote_json_image_info)pnet_info->cmd_data;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
  		    model_msg.data = (uint8 *)image_info;	
  			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_PLAY_REAL_EQ;
	  		divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
            break;
            
        case DIVOOM_NET_COMM_DEVICE_LOCAL_IMAGE_PLAY:
		case DIVOOM_NET_COMM_REMOTE_DRAWING: //显示服务器上的动画
		{
			Pdivoom_draw_remote_json_image_info image_info = (Pdivoom_draw_remote_json_image_info)pnet_info->cmd_data;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
  		    model_msg.data = (uint8 *)image_info;	
  			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DRAW_REMOTE;
	  		divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
        case DIVOOM_NET_COMM_GET_HOT_LIST:
        {
            int32 add_file_num = 0;

            if(divoom_hot_manage_submit_hot_list(
                (Pdivoom_devcice_image_list)pnet_info->cmd_data,
                &add_file_num))
            {
                /* HOT 管理器接管响应块，任务尾部释放网络包时跳过该指针。 */
                pnet_info->cmd_data = NULL;
            }
            divoom_subscribe_manage_reload_result_add(
                DIVOOM_NET_COMM_GET_HOT_LIST, add_file_num);
            break;
        }
            
		case DIVOOM_NET_COMM_GET_FILE_BY_APP:
		{
			Pdivoom_draw_remote_by_app_json_image_info image_info = (Pdivoom_draw_remote_by_app_json_image_info)pnet_info->cmd_data;
			model_msg.msg_type = 0;			
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
  		    model_msg.data = (uint8 *)image_info;	
  			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DRAW_REMOTE_BY_APP;
	  		divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
        
		case DIVOOM_NET_COMM_SYNC_DRAWING:            
    		{
    			Pdivoom_drawing_json_sync_image_info pnet_dot_info = (Pdivoom_drawing_json_sync_image_info)pnet_info->cmd_data;

    			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
    			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_ENTER_SYNC_MODE;			
    			model_msg.data = (uint8 *)pnet_dot_info;
    			divoom_sys_model_crl_msg_push(&model_msg);
				#if 0
				if(model_msg.result == 0)
                {
                    divoom_print("%s:DIVOOM_NET_COMM_SYNC_DRAWING: the device isn't in drawing!\n", __FUNCTION__);
					divoom_net_app_send_packet(DIVOOM_NET_COMM_NEED_SEND_DRAW, DIVOOM_APP_TOPIC, (void *)&pnet_dot_info->UserId);
                }
				#endif
    	    }
    		break;
            
		case DIVOOM_NET_COMM_EXIT_SYNC_DRAWING:
		{
			model_msg.msg_type = 1;
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_EXIT_SYNC_MODE;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}

		case DIVOOM_NET_COMM_DRAW_SET_INFO:
		{
			Pdivoom_draw_set_info_json_image_info set_image_info = (Pdivoom_draw_set_info_json_image_info)pnet_info->cmd_data;

			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DRAW_SET_INFO;			
			model_msg.data = (uint8 *)set_image_info;
			divoom_sys_model_crl_msg_push(&model_msg);
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DRAW_SET_INFO, DIVOOM_APP_TOPIC, NULL);
			break;
		}
        
        case DIVOOM_NET_COMM_DRAW_SET_SPEED_MODE:
		{
			Pdivoom_draw_set_speed_json_mode_info set_speed_mode_info = (Pdivoom_draw_set_speed_json_mode_info)pnet_info->cmd_data;

			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_DRAW_SET_SPEED_MODE_INFO;			
			model_msg.data = (uint8 *)set_speed_mode_info;
			divoom_sys_model_crl_msg_push(&model_msg);
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DRAW_SET_SPEED_MODE, DIVOOM_APP_TOPIC, NULL);
			break;
		}
		
        case DIVOOM_NET_COMM_GET_TIME_PLAN:
			model_msg.dest_id = DIVOOM_MODEL_TIME_PLAN; 
			model_msg.msg_id = DIVOOM_TIME_PLAN_INIT;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
            break;

        case DIVOOM_NET_COMM_GET_CLOCK_PLAY_PLAN_SWITCH:
        case DIVOOM_NET_COMM_SET_CLOCK_PLAY_PLAN_SWITCH:
            break;

        case DIVOOM_NET_COMM_GET_CLOCK_PLAY_PLAN_CYCLE:
        case DIVOOM_NET_COMM_SET_CLOCK_PLAY_PLAN_CYCLE:
            model_msg.dest_id = DIVOOM_MODEL_CLOCK_PLAY_PLAN;
            model_msg.msg_id = DIVOOM_CLOCK_PLAY_PLAN_INIT_CYCLE;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;

        case DIVOOM_NET_COMM_GET_CLOCK_PLAY_PLAN_DATE:
            model_msg.dest_id = DIVOOM_MODEL_CLOCK_PLAY_PLAN;
            model_msg.msg_id = DIVOOM_CLOCK_PLAY_PLAN_INIT_DATE;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
            break;

        case DIVOOM_NET_COMM_SET_CLOCK_PLAY_PLAN_DATE:
        {
            Pdivoom_clock_play_plan_json_date_info pdate_info = (Pdivoom_clock_play_plan_json_date_info)pnet_info->cmd_data;

            if(pdate_info != NULL && pdate_info->has_date_rule_list)
            {
                model_msg.dest_id = DIVOOM_MODEL_CLOCK_PLAY_PLAN;
                model_msg.msg_id = DIVOOM_CLOCK_PLAY_PLAN_INIT_DATE;
                model_msg.data = (uint8 *)pdate_info;
                divoom_sys_model_crl_msg_push(&model_msg);
            }
            break;
        }
            /*
		case DIVOOM_NET_COMM_ADD_TIME_PLAN:
		{
			model_msg.dest_id = DIVOOM_MODEL_TIME_PLAN; 
			model_msg.msg_id = DIVOOM_TIME_PLAN_MODIFY;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
		case DIVOOM_NET_COMM_DEL_TIME_PLAN:
		{	
			model_msg.dest_id = DIVOOM_MODEL_TIME_PLAN; 
			model_msg.msg_id = DIVOOM_TIME_PLAN_DELETE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
		case DIVOOM_NET_COMM_CLOSE_TIME_PLAN:
		{		
			model_msg.dest_id = DIVOOM_MODEL_TIME_PLAN; 
			model_msg.msg_id = DIVOOM_TIME_PLAN_CANCEL;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;
		}
        
        
        case DIVOOM_NET_COMM_CTRL_MIX_MUISC:
            {
                Pdivoom_ctrl_mixer_json_info mixer_info = (Pdivoom_ctrl_mixer_json_info)divoom_malloc(sizeof(divoom_ctrl_mixer_json_info),DIVOOM_MODEL_NET, __LINE__, __FILE__);
                if(mixer_info)                
                {
                    if(pnet_info->cmd_data)
                    {
                        memcpy(mixer_info,pnet_info->cmd_data,sizeof(divoom_ctrl_mixer_json_info));
                    }else
                    {
                        memset(mixer_info,0,sizeof(divoom_ctrl_mixer_json_info));       
                    }
                }else
                {
                    divoom_print("%s:DIVOOM_NET_COMM_CTRL_MIX_MUISC,MALLOC_FAIL\n", __FUNCTION__);
                }
                model_msg.msg_type = 1;             
                model_msg.dest_id = DIVOOM_MODEL_MAIN; 
                model_msg.msg_id = DIVOOM_MAIN_EVENT_START_MUSIC_MIXER;
                model_msg.data = (uint8 *)mixer_info;
                divoom_sys_model_crl_msg_push(&model_msg);
            }
            
            break;
            
		//留言部分删除
		case DIVOOM_NET_COMM_GET_USER_HEAD_MSG:
            break;
            
        case DIVOOM_NET_COMM_GET_SPIRIT_IMAGE:
            break;
            
            
        case DIVOOM_NET_COMM_GET_EXPERT_PIC_LIST:
		case DIVOOM_NET_COMM_GET_CATEGORY_FILE_LIST_V2:
			//收到信息，发给light模块，light模块存储处理信息
			//NEED_MODIFY_EXPECT
			//divoom_disp_expert_pic_update_pic_list(pnet_info->cmd_data);
			//divoom_disp_app_set_list_update_user_list(pnet_info->cmd_data);
			{
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
				model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_SUB_PIC;
				model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
            break;*/

		//case DIVOOM_NET_COMM_GET_CATEGORY_FILE_LIST_V2:
			//NEED_MODIFY_EXPECT
			//divoom_disp_app_set_list_update_user_list(pnet_info->cmd_data);
			//break;

        case DIVOOM_NET_COMM_UPDATE_DEVICE_UPGRADE:
        case DIVOOM_NET_COMM_GET_UPDATE_FILE_INFO:
        case DIVOOM_NET_COMM_DEVICE_SERVER_UPDATE_DEVICE:
#ifndef DIVOOM_SIMULATOR
#ifdef DIVOOM_TIMES_VIBE
            /* JL 仅手动升级 / Sys/PushUpdate，不走凌晨自动拉包 */

            if (pnet_info->cmd == DIVOOM_NET_COMM_UPDATE_DEVICE_UPGRADE || pnet_info->cmd == DIVOOM_NET_COMM_GET_UPDATE_FILE_INFO)
            {
                Pdivoom_general_json_get_update_file_info_respone push_info =
                    (Pdivoom_general_json_get_update_file_info_respone)pnet_info->cmd_data;

                if (push_info)
                {
                    /* 先按主控 Hardware 判 F101；非 F101 再用杰理上报版本前缀核对 */
                    if (divoom_ota_is_f101_cloud_package(push_info->version))
                    {
                        divoom_print("ota: PushUpdate route F101 ver=%u (hw=%u)\n",
                                     (unsigned)push_info->version,
                                     (unsigned)DIVOOM_HARDWARE_VERSION);
                        /* 落入下方主控 SRV_UPDATE */
                    }
                    else if (divoom_ota_jl_is_cloud_package(push_info->version,
                                                           push_info->hardware))
                    {
                        Pdivoom_general_json_get_update_file_info_respone update_info =
                            (Pdivoom_general_json_get_update_file_info_respone)
                            divoom_malloc(sizeof(divoom_general_json_get_update_file_info_respone),
                                          DIVOOM_MODEL_NET, __LINE__, __FILE__);

                        divoom_print("ota: PushUpdate route JL ver=%u hw=%d (jl=%u)\n",
                                     (unsigned)push_info->version,
                                     push_info->hardware,
                                     (unsigned)divoom_ota_jl_get_local_version());
                        if (update_info)
                        {
                            memcpy(update_info, push_info,
                                   sizeof(divoom_general_json_get_update_file_info_respone));
                            divoom_main_set_jl_device_update_info(update_info);
                            update_info->type = 2;
                            if (!divoom_update_system_file_start_firmware(update_info,
                                DIVOOM_OTA_SERVER_JL_SRV_UPDATE))
                                divoom_system_free(update_info);
                        }
                        break;
                    }
                    else
                    {
                        divoom_print("ota: PushUpdate reject ver=%u (not F101, JL invalid/mismatch)\n",
                                     (unsigned)push_info->version);
                        break;
                    }
                }
            }
            if (pnet_info->cmd == DIVOOM_NET_COMM_GET_UPDATE_FILE_INFO
                && divoom_main_is_jl_get_update_pending())
            {
                Pdivoom_general_json_get_update_file_info_respone jl_info =
                    (Pdivoom_general_json_get_update_file_info_respone)pnet_info->cmd_data;

                divoom_main_set_jl_device_update_info(jl_info);
                break;
            }
#endif
            if (pnet_info->cmd == DIVOOM_NET_COMM_GET_UPDATE_FILE_INFO &&
                divoom_main_get_boot_ab_recovery_needed())
            {
                Pdivoom_general_json_get_update_file_info_respone info =
                    (Pdivoom_general_json_get_update_file_info_respone)pnet_info->cmd_data;
                char sha1[41];
                if (!info || info->version <= 0 || !info->file_id[0] ||
                    !memchr(info->file_id, '\0', 128) ||
                    !divoom_ota_rtos_normalize_sha1(info->sha1_sum, sha1))
                {
                    divoom_print("[BOOT AB] invalid recovery firmware response\n");
                    break;
                }
                memcpy(info->sha1_sum, sha1, sizeof(sha1));
            }
            if(divoom_main_get_device_self_update_flag() == 0 && !divoom_main_get_boot_ab_recovery_needed() && pnet_info->cmd == DIVOOM_NET_COMM_GET_UPDATE_FILE_INFO)
            {
				Pdivoom_general_json_get_update_file_info_respone update_info = (Pdivoom_general_json_get_update_file_info_respone)pnet_info->cmd_data;

                divoom_main_set_device_update_info(update_info);
            }
            else
        	{
				Pdivoom_general_json_get_update_file_info_respone update_info = (Pdivoom_general_json_get_update_file_info_respone)divoom_malloc(sizeof(divoom_general_json_get_update_file_info_respone), DIVOOM_MODEL_NET, __LINE__, __FILE__);


                if (!update_info || !pnet_info->cmd_data)
                {
                    if (update_info) divoom_system_free(update_info);
                    break;
                }
				memcpy(update_info, pnet_info->cmd_data, sizeof(divoom_general_json_get_update_file_info_respone));
                if(pnet_info->cmd == DIVOOM_NET_COMM_DEVICE_SERVER_UPDATE_DEVICE)
                {
                    time_t cur_time;
                    struct tm cur_tm;    
                    
                    cur_time = time(0);
                    localtime_r((time_t *)&cur_time, &cur_tm);
                    
                    if(cur_tm.tm_hour == 2 || cur_tm.tm_hour == 3)
                    {
                        update_info->type  = 3;
                    }
                    else
                    {
                        divoom_print("pass the update event!\n");
                        update_info->type  = 0;
                    }
                }
                else
                {
                    update_info->type = pnet_info->cmd == DIVOOM_NET_COMM_UPDATE_DEVICE_UPGRADE?2:1;
                }
                if (update_info->type)
                {
                    /* Wait for active resource writes without requesting or starting file updates. */
                    if (!divoom_update_system_file_start_firmware(update_info, DIVOOM_OTA_SERVER_SRV_UPDATE))
                        divoom_system_free(update_info);
                }
                else
                {
                    divoom_system_free(update_info);
                }
        	}
            #endif
            break;

		case DIVOOM_NET_COMM_USERLOGIN:
			{
				Pdivoom_login_info login_info = NULL;
				login_info = divoom_malloc(sizeof(divoom_login_info), DIVOOM_MODEL_NET, __LINE__, __FILE__);

				memcpy(login_info, pnet_info->cmd_data, sizeof(divoom_login_info));
				//返回的accesstoken,发送到main模块
				model_msg.dest_id = DIVOOM_MODEL_MAIN;
				model_msg.msg_type = 1;
				model_msg.msg_id = DIVOOM_MAIN_SET_LOGIN_INFO;
				model_msg.data = (uint8 *)login_info;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
			break;
		case DIVOOM_NET_COMM_DEVICE_CON_APP:
			{
				model_msg.dest_id = DIVOOM_MODEL_MAIN;
				model_msg.msg_id = DIVOOM_MAIN_EVENT_APP_MQTT_CON;
				model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
			break;

		case DIVOOM_NET_COMM_DEVICE_GET_COMPONENT_UPDATE_TIME_LIST:
	        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_SYS_COMPONENT_LIST;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
	        }
            break;

		 case DIVOOM_NET_COMM_DEVICE_GET_COMPONENT_INFO:
	        {
				model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
				model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_UPDATE_SYS_COMPONENT_INFO;
	            model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
	        }
            break;

		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_SET:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_SET;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_GET_LIST:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_GET;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_SEND_LISTCOUNT:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_SEND_LIST_COUNT;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;


		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_SEND_ONEBYONE:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_SEND_ONE_BY_ONE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_START:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_START;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_FOCUS_DONE:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_FOCUS_DONE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_DELETE:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_DELETE;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_LISTEN:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_LISTEN;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_SET_LISTEN_VOLUME:
			model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			model_msg.msg_id = DIVOOM_APP_TOMATO_LISTEN_VOLUME;
			model_msg.data = (uint8 *)pnet_info->cmd_data;
			divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_SET_WHITE_NOISE:
			model_msg.dest_id = DIVOOM_MODEL_APP_WHITE_NOISE;
	        model_msg.msg_id = DIVOOM_WHITE_NOISE_SET;
	        model_msg.data = (uint8 *)pnet_info->cmd_data;
	        divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_GET_WHITE_NOISE:
			model_msg.dest_id = DIVOOM_MODEL_APP_WHITE_NOISE;
            model_msg.msg_id = DIVOOM_WHITE_NOISE_GET;
            model_msg.data = (uint8 *)pnet_info->cmd_data;
            divoom_sys_model_crl_msg_push(&model_msg);
			break;

		case DIVOOM_NET_COMM_AID_SLEEP_GET_ALL_LIST:
			if (pnet_info->cmd_data)
				divoom_sleep_aid_handle_get_all_list_resp(pnet_info->cmd_data);
			break;

		case DIVOOM_NET_COMM_AID_SLEEP_ADD:
			if (pnet_info->cmd_data)
				divoom_sleep_aid_handle_add_req(pnet_info->cmd_data);
			break;

		case DIVOOM_NET_COMM_AID_SLEEP_PLAY:
			if (pnet_info->cmd_data)
				divoom_sleep_aid_handle_play_req(pnet_info->cmd_data);
			break;

		case DIVOOM_NET_COMM_AID_SLEEP_EXIT:
			divoom_sleep_aid_handle_exit_play();
			break;

		case DIVOOM_NET_COMM_CHANNEL_SET_RGB_INFO:
            if(pnet_info->cmd_data)
    		{
    			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
    			model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_RGB_INFO;
                model_msg.data = (uint8 *)pnet_info->cmd_data;
    			divoom_sys_model_crl_msg_push(&model_msg);
                if(pnet_info->type == DIVOOM_APP_TOPIC)
                {
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_RGB_INFO, DIVOOM_SERVER_TOPIC, (void *)pnet_info->cmd_data);
                }
    		}
			break;

		case DIVOOM_NET_COMM_CHANNEL_GET_RGB_INFO:
			{
				model_msg.dest_id = DIVOOM_MODEL_MAIN; 
				model_msg.msg_id = DIVOOM_MAIN_EVENT_GET_RGB_INFO;
				model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
				break;
			}

		case DIVOOM_NET_COMM_DEVICE_GET_RGB_INFO:
			if(pnet_info->cmd_data)
			{
				model_msg.dest_id = DIVOOM_MODEL_MAIN; 
				model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_RGB_INIT_INFO;
				model_msg.data = (uint8 *)pnet_info->cmd_data;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
			break;
	}

    if(DIVOOM_NET_COMM_SYNC_DRAWING != pnet_info->cmd && DIVOOM_NET_COMM_LED_SEND_DATA  != pnet_info->cmd)
    {
	    divoom_print("end com %d\n", pnet_info->cmd);	
    }
	if (pnet_info->type == DIVOOM_APP_TOPIC)
	{
        divoom_net_app_send_packet(pnet_info->cmd, DIVOOM_APP_TOPIC, NULL);
	}
	//divoom_system_free_lock(pdivoom_net_app_handle->cb_lock);
}
