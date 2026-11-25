#include "divoom.h"
#include "divoom_wifi_con.h"
#include "divoom_ble_accept_data.h"

#define DIVOOM_BLE_ACCEPT_TAG "DIVOOM_BLE_ACCEPT"
#define GATTS_TABLE_TAG "DIVOOM_BLE_GATTS"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SVC_INST_ID                 0

#define GATTS_DIVOOM_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

//从目前看应该不会超过128
//#define DIVOOM_QUEUE_DATA_LEN		128
//增加到256把
#define DIVOOM_QUEUE_DATA_LEN		256

#define PACKET_HEAD_1_FLAG			0XFE
#define PACKET_HEAD_2_FLAG			0XEF
#define PACKET_HEAD_3_FLAG			0XAA
#define PACKET_HEAD_4_FLAG			0X55

#define PACKET_CHECK_SUM_START		4
#define PACKET_TAIL_FLAG			0X02 
#define POSITION_LEN_LOW			0
#define POSITION_LEN_HIGH			1
#define POSITION_CMD				0
#define POSITION_SET_CMD			1
#define DIVOOM_CKECK_PKG_LEN		11
//命令回应
#define BLE_COMMAND_CHECK			0X4
#define BLE_COMMAND_ACK				0X55

#define DIVOOM_BLE_TASK_NAME		"ble_task"
#define DIVOOM_BLE_TASK_PRI			ESP_TASKD_EVENT_PRIO
#define DIVOOM_BLE_TSK_QUEUE_NAME	3
#define DIVOOM_BLE_TASK_STACK		3072

enum{
	DIVOOM_BLE_PACKET_HEAD_1_STATUS,
	DIVOOM_BLE_PACKET_HEAD_2_STATUS,
	DIVOOM_BLE_PACKET_HEAD_3_STATUS,
	DIVOOM_BLE_PACKET_HEAD_4_STATUS,
	DIVOOM_BLE_PACKET_LEN_1_STATUS,
	DIVOOM_BLE_PACKET_LEN_2_STATUS,
	DIVOOM_BLE_PACKET_FLAG_STATUS,
	DIVOOM_BLE_PACKET_DATA_STATUS,
	DIVOOM_BLE_PACKET_CHECK_SUM_1_STATUS,
	DIVOOM_BLE_PACKET_CHECK_SUM_2_STATUS,
	DIVOOM_BLE_PACKET_TAIL_STATUS,
};

enum{
	//0-->没有收到fe,ef,aa,55的数据
	DIVOOM_BLE_MAYBE_NULL_STATUS = 0,
	DIVOOM_BLE_MAYBE_HEAD_OVER_STATUS,
	DIVOOM_BLE_MAYBE_HEAD_1_STATUS,
	DIVOOM_BLE_MAYBE_HEAD_2_STATUS,
	DIVOOM_BLE_MAYBE_HEAD_3_STATUS,
	DIVOOM_BLE_MAYBE_HEAD_4_STATUS,
};

uint8_t divoom_check_code[6] = {0x64, 0x69, 0x76, 0x6f, 0x6f, 0x6d};

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

typedef struct{
	uint8_t data_len;
	uint8_t data[DIVOOM_QUEUE_DATA_LEN];
}divoom_ble_queue_struct, *Pdivoom_ble_queue_struct;


typedef struct {
    uint8 run_flag;
    void *divoom_ble_queue;
    void *divoom_ble_sem;
	prepare_type_env_t	prepare_write_env;
	uint16_t			conn_id;
	uint16_t			heart_rate_handle_table[HRS_IDX_NB];
	uint8_t				adv_config_done;
	DIVOOM_BLE_CALLBACK callback;
	uint8_t				Is_first_check;//0,没有收到过数据，1，收到check数据并检验成功,收到ssid和pswd 2,收到退出BLE命令，可能还没有连接上WiFi，等WiFi那边连接上在退出other:other error
	int                 divoom_ble_connect_time;
	DIVOOM_WIFI_STATUS	wifi_status;

	//保存收到的数据，收到0x02之后开始处理
	//当前收数据的状态
	uint8_t				check_status;
	//如果在头处理之后收到fe,ef,aa,55,每次都置为下一个
	uint8_t				check_instand_head;
	//包标识长度计算，4个字节
	uint8_t				ble_packet_len;
	//收到的数据的checksum
	uint32_t			check_sum_data;
	//收到的数据算出来的checksum
	uint32_t			check_sum_get;
	//计算出来的包长度
	uint32_t			ble_data_len_sum;
	//得到出来的包长度
	uint32_t			ble_data_len_get;
	//需要处理的数据的长度
	uint16_t			ble_accept_len;
	//需要处理的数据
	uint8_t				ble_accept_data[DIVOOM_QUEUE_DATA_LEN];
    //uint8_t             work_mode_flag;
	
	//存储之前第一次绑定时的APPid
	uint32_t			app_id;
}divoom_ble_struct, *Pdivoom_ble_struct;

Pdivoom_ble_struct divoom_ble_handle = NULL;

static void divoom_ble_cle_queue(void);


//返回大于0为成功
//会在此函数内把所有的utf16的转成utf8的，并且申请空间
//utf16_le小端，低位在前，高位在后，每次取两个，拼成uint16的,小于0x80就是不需要转化的，大于就要用divoom_utf8_putc转化
int8 divoom_ble_utf16_to_utf8_string(uint8 *srcstring, uint32 srcstring_len, uint8 **deststring, uint32 *needstring_len)
{
	uint32 i = 0, j = 0;
	uint8 utf8_string[6];
	uint8 utf8_string_len = 0;
	uint16 utf16_int = 0;
	uint8 *needstring = NULL;
	//判断长度是否是偶数
	//ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "srcstring_len = %d, srcstring = %s", srcstring_len, srcstring);
	if(srcstring_len & 1)
	{
		return -1;
	}
	if(needstring)
	{
		divoom_free(needstring);
	}
	//得到utf8的数量
	//不考虑占用四个字节的扩展D区的汉字
	for(i=0;i<srcstring_len/2;i++)
	{
		utf16_int = srcstring[i*2] + (srcstring[i*2+1] << 8);
		if(utf16_int < 0x80)
		{
			(*needstring_len)++;
		}
		else if(utf16_int < 0x800)
		{
			*needstring_len += 2;
		}
		else
		{
			*needstring_len += 3;
		}
	}
	(*needstring_len)++;
	//ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "srcstring_len = %d, needstring_len = %d", srcstring_len, *needstring_len);
	//malloc空间
	needstring = (uint8 *)divoom_malloc(sizeof(uint8)*(*needstring_len), DIVOOM_MODEL_CONFIG_WIFI, __LINE__, __FILE__);
	//开始转换
	for(i=0;i<srcstring_len/2;i++)
	{
		utf16_int = srcstring[i*2] + (srcstring[i*2+1] << 8);
		utf8_string_len = divoom_utf8_putc(utf8_string, sizeof(utf8_string), utf16_int);
		if(utf8_string_len > 0 && j < *needstring_len)
		{
			memcpy(needstring+j, utf8_string, utf8_string_len);
			j += utf8_string_len;
		}
	}
	needstring[*needstring_len - 1] = 0;
	/*for(i=0;i<*needstring_len;i++)
	{
		ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "needstring[%d] = 0x%02x", i, needstring[i]);
	}*/

	*deststring = needstring;

	return 1;
}

/*
flag:1回复命令，0，主动发送
   */
void divoom_ble_send_data(uint8_t *data, uint8_t len, bool flag, uint8_t cmd, Pdivoom_ble_struct divoom_send_handle)
{
	uint8_t back_data[31];
	uint8_t send_len = 0;
	uint8_t i = 0;
	uint16_t check_sum = 0;

	back_data[send_len++] = 0x1;

	//len = 0x04(1)+0xf3(1)+回应数据(len)+check sum(2)
	back_data[send_len++] = (len+5) & 0XFF;
	back_data[send_len++] = (len+5) >> 8;
	back_data[send_len++] = BLE_COMMAND_CHECK;
	back_data[send_len++] = cmd;
	back_data[send_len++] = BLE_COMMAND_ACK;

	for(i=0;i<len;i++)
	{
		back_data[send_len++] = data[i];
	}
	for(i=1;i<send_len;i++)
	{
		check_sum += back_data[i];
	}

	back_data[send_len++] = check_sum & 0xFF;
	back_data[send_len++] = check_sum >> 8;
	back_data[send_len++] = PACKET_TAIL_FLAG;

	/**************test******************
	for(i=0;i<send_len;i++)
	{
		ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "back_data[%d] = 0x%02x", i, back_data[i]);
	}
	**************test******************/

	divoom_bt_ctrl_ble_send_data(back_data, send_len);
}

//处理收到的BLE数据
static void divoom_ble_command_do(uint8_t *buf, uint8_t len, Pdivoom_ble_struct divoom_cmd_handle)
{
	divoom_sys_msg_info model_msg = {0};

    if(divoom_ble_handle == NULL)
    {
        return;
    }
	//外面检测了
	divoom_print("The BLE COMMOND:0x%02x\n", buf[POSITION_CMD]);
	//buf的内容,第一位是cmd,然后是数据
	switch(buf[POSITION_CMD])
	{
		//过后再添加
		case WIFI_SEND_DATA_BLE_DIS_CONNECT_COMMAND:
            if(divoom_ble_handle->wifi_status != DIVOOM_WIFI_CONNECTED && divoom_ble_handle->wifi_status != DIVOOM_WIFI_CONNECTED_SER)
            {
                divoom_ble_handle->wifi_status = DIVOOM_WIFI_NOT_CONFIG;
                divoom_wifi_reset_ble();
            }
            break;
            
		case SERVER_TYPE_BLE_COMMAND:
            divoom_main_update_server_type(buf[POSITION_SET_CMD]);
            break;
		case SPP_SET_MIX_MUISE_MODE:
			{
				Pdivoom_wifi_ble_music_info music_mix_info = NULL;                    
                if((buf[POSITION_SET_CMD] == 7 || buf[POSITION_SET_CMD] == 8)
                    && buf[POSITION_SET_CMD+2] == 0)
                {                        
                    break;  //这两个命令不处理
                }
                music_mix_info  = divoom_malloc(sizeof(divoom_wifi_ble_music_info), DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
				memset(music_mix_info, 0, sizeof(divoom_wifi_ble_music_info));
				music_mix_info->music_mix_mode = buf[POSITION_SET_CMD];
				music_mix_info->music_mix_button_select = buf[POSITION_SET_CMD+1];
				music_mix_info->music_mix_onoff = buf[POSITION_SET_CMD+2];
                music_mix_info->app_flag = 1;
				divoom_cmd_handle->Is_first_check = DIVOOM_BLE_GET_MIX_MUSIC;
				if(divoom_cmd_handle && divoom_cmd_handle->callback)
					divoom_cmd_handle->callback(divoom_cmd_handle->Is_first_check, music_mix_info);
			}
			break;
        case SPP_SET_MUSICAL_INSTRUMENTS_MODE:
            {
                Pdivoom_wifi_ble_musical_instruments_info info = divoom_malloc(sizeof(divoom_wifi_ble_musical_instruments_info), DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
                memset(info, 0, sizeof(divoom_wifi_ble_musical_instruments_info));

                info->mode = buf[POSITION_SET_CMD];
                info->index = buf[POSITION_SET_CMD+1];
                divoom_cmd_handle->Is_first_check = DIVOOM_BLE_GET_MUSICAL_INSTRUMENTS;
                if(divoom_cmd_handle && divoom_cmd_handle->callback)
                    divoom_cmd_handle->callback(divoom_cmd_handle->Is_first_check, info);                    
            }
            break;
		//START_BOND_BLE_COMMAND:和EXIT_BOND_BLE_COMMAND:说是不要了
		//20191022,收到这条命令回复本机的MAC地址
		case START_BOND_BLE_COMMAND:
			{
				model_msg.msg_type = 1;
				model_msg.src_id = DIVOOM_MODEL_CONFIG_WIFI;
				model_msg.dest_id = DIVOOM_MODEL_CONFIG_WIFI;
				model_msg.msg_id = DIVOOM_APP_CONFIG_BACK_MAC_ADDR;
				//model_msg.data = (uint8 *)send_handle;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
			break;
		case EXIT_BOND_BLE_COMMAND:
			{
				divoom_print("EXIT_BOND_BLE_COMMAND:%d\n", buf[POSITION_SET_CMD]);
				//0是断开连接，不释放内存，1是OK了，把这部分的内存释放掉
				//0不断开，等APP断开就行了
				/*if(buf[POSITION_SET_CMD] == 0)
				{
					esp_ble_gap_disconnect(divoom_cmd_handle->ble_remote_bda);
				}
				else*/
				#if 1
				//if(buf[POSITION_SET_CMD] == 1)
				{
					//for test
					model_msg.msg_type = 1;
					model_msg.src_id = DIVOOM_MODEL_CONFIG_WIFI;
					model_msg.dest_id = DIVOOM_MODEL_CONFIG_WIFI;
					model_msg.msg_id = DIVOOM_APP_CONFIG_WIFI_EXIT_BLE;
					//model_msg.data = (uint8 *)send_handle;
					divoom_sys_model_crl_msg_push(&model_msg);

					//divoom_ble_stop(divoom_cmd_handle);
				}
                #endif
			}
			break;
		case WIFI_CONFIG_BLE_COMMAND:
			{
				//APP非要采用utf16le编码,转化成utf8编码的
				uint32 offset = 0;
				Pdivoom_wifi_info_struct wifi_info = divoom_malloc(sizeof(divoom_wifi_info_struct), DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
				memset(wifi_info, 0, sizeof(divoom_wifi_info_struct));
				wifi_info->ssid_len = buf[POSITION_SET_CMD];

				/*
				if(divoom_ble_utf16_to_utf8_string(buf+POSITION_SET_CMD+1, buf[POSITION_SET_CMD], &wifi_info->ssid, &wifi_info->ssid_len) < 0)
				{
					ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "Some Wrong In Here:%s--%d", __func__, __LINE__);
					break;
				}
				*/
				
				// +1--->\0
				wifi_info->ssid = (uint8_t *)divoom_malloc(wifi_info->ssid_len+1, DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
				memcpy(wifi_info->ssid, buf+POSITION_SET_CMD+1, wifi_info->ssid_len);
				wifi_info->ssid[wifi_info->ssid_len] = 0;

				/*
				if(divoom_ble_utf16_to_utf8_string(buf+POSITION_SET_CMD+1+buf[POSITION_SET_CMD]+1, buf[POSITION_SET_CMD+1+buf[POSITION_SET_CMD]], &wifi_info->pswd, &wifi_info->pswd_len) < 0)
				{
					ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "Some Wrong In Here:%s--%d", __func__, __LINE__);
					break;
				}
				*/
				
				wifi_info->pswd_len = buf[POSITION_SET_CMD+1+wifi_info->ssid_len];
				wifi_info->pswd = (uint8_t *)divoom_malloc(wifi_info->pswd_len+1, DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
				memcpy(wifi_info->pswd, buf+POSITION_SET_CMD+1+wifi_info->ssid_len+1, wifi_info->pswd_len);
				wifi_info->pswd[wifi_info->pswd_len] = 0;

				offset = POSITION_SET_CMD + 1 + buf[POSITION_SET_CMD] + 1 + buf[POSITION_SET_CMD+1+buf[POSITION_SET_CMD]];
				wifi_info->app_id = buf[offset++] & 0xFF;
				wifi_info->app_id += buf[offset++] << 8;
				wifi_info->app_id += buf[offset++] << 16;
				wifi_info->app_id += buf[offset++] << 24;

				//解析登录服务器的密码,32位的小写MD5加密密文,需要进行MD5解密
				//只需要保存下来,使用一次,为了获取accesstoken
				if(offset < len)
				{
					wifi_info->login_ps_len = buf[offset++];
					if(wifi_info->login_ps_len > 0 && wifi_info->login_ps_len <= (sizeof(wifi_info->login_ps)-1))
					{
						strcpy((char *)wifi_info->login_ps, (char *)(buf+offset));
					}
				}

				//检查是不是第一次配网的useid,不是的话主动断开
				//20191022,不需要在这样校验了
				divoom_cmd_handle->app_id = wifi_info->app_id;
				divoom_print("[WIFI_CFG] BLE 0xF2 recv ssid=%s pswd_len=%u app_id=%u\n",
					wifi_info->ssid, (unsigned)wifi_info->pswd_len, (unsigned)wifi_info->app_id);
				divoom_cmd_handle->Is_first_check = DIVOOM_BLE_GET_CONFIG;
				if(divoom_cmd_handle && divoom_cmd_handle->callback)
					divoom_cmd_handle->callback(divoom_cmd_handle->Is_first_check, wifi_info);
			}
			break;
		//用divoom_ble_wifi_status_notify来发送这条命令
		//第一位是command,第二,三位是长度,低位在前,后面跟数据
		case WIFI_SEND_DATA_BLE_COMMAND:
			{
				uint8 command = buf[POSITION_SET_CMD];
				uint32 size = buf[POSITION_SET_CMD+1];
				size += buf[POSITION_SET_CMD+2] << 8;
				uint32 i = 0;
				divoom_print("send command = %x, size = %d\n", command, size);
				for(i=0;i<size;i++)
				{
					divoom_print("data[%d] = 0x%02x,", i, buf[POSITION_SET_CMD+3+i]);
				}
				divoom_print("\n");
				divoom_ble_send_data(buf+POSITION_SET_CMD+3, size, 0, command, divoom_cmd_handle);
			}
			break;
		default:
			//ESP_LOGE(DIVOOM_BLE_ACCEPT_TAG, "error cmd:0x%x\n", buf[POSITION_CMD]);
			break;
	}
}
void divoom_ble_on_jl_connect_info(const uint8_t *cmd_data, uint16_t cmd_len)
{
	if (!cmd_data || cmd_len < 2u || !divoom_ble_handle)
		return;
	if (cmd_data[0] != 0u)
		return;
	if (cmd_data[1] == 1u)
	{
		divoom_ble_handle->divoom_ble_connect_time = (int)(divoom_system_get_tickms() / 1000);
		divoom_print("JL BLE connected\n");
	}
	else
	{
		divoom_print("JL BLE disconnected\n");
		if (divoom_ble_handle->wifi_status != DIVOOM_WIFI_CONNECTED
		    && divoom_ble_handle->wifi_status != DIVOOM_WIFI_CONNECTED_SER)
		{
			divoom_ble_handle->wifi_status = DIVOOM_WIFI_NOT_CONFIG;
		}
	}
}

void divoom_ble_recv_data_callback(const char *uuid, char *data, int len)
{
    divoom_ble_queue_struct *task_queue_handle;
	if(NULL == divoom_ble_handle)
	{
		return;
	}
    task_queue_handle = divoom_system_malloc(sizeof(divoom_ble_queue_struct));
    if(task_queue_handle)
    {
        task_queue_handle->data_len = len;
        memcpy(task_queue_handle->data, data, len);
        divoom_queue_push(divoom_ble_handle->divoom_ble_queue,task_queue_handle);
		divoom_system_put_sem(divoom_ble_handle->divoom_ble_sem);
    }
}

//收数据主线程,BLE会把数据放到队列,这边处理
void *divoom_ble_data_do(void *arg)
{
    divoom_ble_queue_struct *task_queue_handle;
    Pdivoom_ble_struct divoom_task_handle = (Pdivoom_ble_struct)arg;
    //uint8_t accept_len = 0;
    while(divoom_task_handle->run_flag)
    {
        divoom_system_get_sem(divoom_task_handle->divoom_ble_sem);
        if(divoom_task_handle->run_flag == 0)
        {
            break;
        }
        while(divoom_queue_items(divoom_task_handle->divoom_ble_queue))
        {
            uint32_t i = 0;
            uint8_t Wrong_flag = 1;

            task_queue_handle = divoom_queue_pop(divoom_task_handle->divoom_ble_queue);
            if(task_queue_handle == NULL)
            {
                break;
            }
            divoom_print("task_queue_handle.data_len = %d\n", task_queue_handle->data_len);
            while(i<task_queue_handle->data_len && Wrong_flag)
            {
                //头部检查完了之后开始检查数据流中是否有刚好是四个头fe,ef,aa,55,如果是，Wrong_flag置0，清除前面所有的数据
                //如果先收到一个0xFE,在收到0xFE,EF,AA,55,那没有办法了，新的这条命令不能相应
                //ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "task_queue_handle.data[%d] = 0x%x", i, task_queue_handle.data[i]);
                //ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "divoom_task_handle->check_instand_head = %d", divoom_task_handle->check_instand_head);
                //ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "divoom_task_handle->check_status = %d", divoom_task_handle->check_status);
                {
                    //根据数据进行当前检查的范围进行判断
                    //主要是为了某次数据没收完,但是后面的没收到的处理
                    if(task_queue_handle->data[i] == PACKET_HEAD_1_FLAG && divoom_task_handle->check_instand_head == DIVOOM_BLE_MAYBE_HEAD_OVER_STATUS)
                    {
                        divoom_task_handle->check_instand_head = DIVOOM_BLE_MAYBE_HEAD_1_STATUS;
                    }
                    else if(task_queue_handle->data[i] == PACKET_HEAD_2_FLAG && divoom_task_handle->check_instand_head == DIVOOM_BLE_MAYBE_HEAD_1_STATUS)
                    {
                        divoom_task_handle->check_instand_head = DIVOOM_BLE_MAYBE_HEAD_2_STATUS;
                    }
                    else if(task_queue_handle->data[i] == PACKET_HEAD_3_FLAG && divoom_task_handle->check_instand_head == DIVOOM_BLE_MAYBE_HEAD_2_STATUS)
                    {
                        divoom_task_handle->check_instand_head = DIVOOM_BLE_MAYBE_HEAD_3_STATUS;
                    }
                    else if(task_queue_handle->data[i] == PACKET_HEAD_4_FLAG && divoom_task_handle->check_instand_head == DIVOOM_BLE_MAYBE_HEAD_3_STATUS)
                    {
                        //连续检测到了四个头
                        divoom_task_handle->check_status = DIVOOM_BLE_PACKET_LEN_1_STATUS;
                        divoom_task_handle->check_instand_head = DIVOOM_BLE_MAYBE_HEAD_OVER_STATUS;
                        //divoom_task_handle->check_status = DIVOOM_BLE_PACKET_HEAD_1_STATUS;
                        //divoom_task_handle->check_instand_head = 0;
                        divoom_task_handle->check_sum_data = 0;
                        divoom_task_handle->check_sum_get = 0;
                        divoom_task_handle->ble_accept_len = 0;
                        divoom_task_handle->ble_data_len_get = 0;
                        divoom_task_handle->ble_data_len_sum = 0;
                        divoom_task_handle->ble_packet_len = 0;
                        memset((char *)divoom_task_handle->ble_accept_data, 0, DIVOOM_QUEUE_DATA_LEN);
                        i++;
                        continue;
                    }
                    else if(divoom_task_handle->check_instand_head != DIVOOM_BLE_MAYBE_NULL_STATUS)
                    {
                        divoom_task_handle->check_instand_head = DIVOOM_BLE_MAYBE_HEAD_OVER_STATUS;
                    }
                }
                switch(divoom_task_handle->check_status)
                {
                    case DIVOOM_BLE_PACKET_HEAD_1_STATUS:
                        {
                            if(task_queue_handle->data[i] == PACKET_HEAD_1_FLAG)
                            {
                                divoom_task_handle->check_status = DIVOOM_BLE_PACKET_HEAD_2_STATUS;
                            }
                        }
                        break;
                    case DIVOOM_BLE_PACKET_HEAD_2_STATUS:
                        {
                            if(task_queue_handle->data[i] == PACKET_HEAD_2_FLAG)
                            {
                                divoom_task_handle->check_status = DIVOOM_BLE_PACKET_HEAD_3_STATUS;
                            }
                            else
                            {
                                Wrong_flag = 0;
                            }
                        }
                        break;
                    case DIVOOM_BLE_PACKET_HEAD_3_STATUS:
                        {
                            if(task_queue_handle->data[i] == PACKET_HEAD_3_FLAG)
                            {
                                divoom_task_handle->check_status = DIVOOM_BLE_PACKET_HEAD_4_STATUS;
                            }
                            else
                            {
                                Wrong_flag = 0;
                            }
                        }
                        break;
                    case DIVOOM_BLE_PACKET_HEAD_4_STATUS:
                        {
                            if(task_queue_handle->data[i] == PACKET_HEAD_4_FLAG)
                            {
                                divoom_task_handle->check_status = DIVOOM_BLE_PACKET_LEN_1_STATUS;
                                divoom_task_handle->check_instand_head = DIVOOM_BLE_MAYBE_HEAD_OVER_STATUS;
                                divoom_task_handle->check_sum_data = 0;
                            }
                            else
                            {
                                Wrong_flag = 0;
                            }
                        }
                        break;
                    case DIVOOM_BLE_PACKET_LEN_1_STATUS:
                        {
                            divoom_task_handle->ble_data_len_get = task_queue_handle->data[i] & 0xFF;
                            divoom_task_handle->check_sum_data += task_queue_handle->data[i];
                            divoom_task_handle->check_status = DIVOOM_BLE_PACKET_LEN_2_STATUS;
                        }
                        break;
                    case DIVOOM_BLE_PACKET_LEN_2_STATUS:
                        {
                            divoom_task_handle->ble_data_len_get += task_queue_handle->data[i] << 8;
                            divoom_task_handle->check_sum_data += task_queue_handle->data[i];
                            divoom_task_handle->check_status = DIVOOM_BLE_PACKET_FLAG_STATUS;
                            divoom_task_handle->ble_data_len_sum = 0;
                            divoom_task_handle->ble_packet_len = 0;
                            divoom_task_handle->ble_accept_len = 0;
                            if(divoom_task_handle->ble_data_len_get > DIVOOM_QUEUE_DATA_LEN)
                            {
                                divoom_print("BLE ERR PATCK LEN:%d\n\n\n\n\n", divoom_task_handle->ble_data_len_get);
                            }
                            //ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "divoom_task_handle->ble_data_len_get = %d\n", divoom_task_handle->ble_data_len_get);
                        }
                        break;
                    case DIVOOM_BLE_PACKET_FLAG_STATUS:
                        {
                            //包标示，4个字节，忽略掉，暂时不用
                            //if(divoom_task_handle->ble_packet_len++ >= 3)
                            {
                                divoom_task_handle->check_status = DIVOOM_BLE_PACKET_DATA_STATUS;
                                divoom_task_handle->check_sum_data += task_queue_handle->data[i];
                            }
                            divoom_task_handle->ble_data_len_sum++;
                        }
                        break;
                    case DIVOOM_BLE_PACKET_DATA_STATUS:
                        {
                            //除去0Xfe, 0xef, 0xaa, 0x55四个校验，从第四位开始,去除最后一位尾部,和两位checksum
                            //if(i<task_queue_handle.data_len-3)
                            if(divoom_task_handle->ble_data_len_sum++ < divoom_task_handle->ble_data_len_get - 1)
                            {
                                //ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "task_queue_handle.data[%d] = 0x%x\n", i, task_queue_handle.data[i]);
                                divoom_task_handle->check_sum_data += task_queue_handle->data[i];
                                if(divoom_task_handle->ble_accept_len >= DIVOOM_QUEUE_DATA_LEN)
                                {
                                    divoom_print("DIVOOM_BLE_PACKET_DATA_STATUS LEN:%d\n\n\n\n\n", divoom_task_handle->ble_accept_len);
                                }
                                divoom_task_handle->ble_accept_data[divoom_task_handle->ble_accept_len] = task_queue_handle->data[i];
                                divoom_task_handle->ble_accept_len++;
                            }
                            //if(i == task_queue_handle.data_len-4)
                            if(divoom_task_handle->ble_data_len_sum == divoom_task_handle->ble_data_len_get - 2)
                                divoom_task_handle->check_status = DIVOOM_BLE_PACKET_CHECK_SUM_1_STATUS;
                        }
                        break;
                    case DIVOOM_BLE_PACKET_CHECK_SUM_1_STATUS:
                        {
                            divoom_task_handle->check_sum_get = task_queue_handle->data[i] & 0xFF;
                            divoom_task_handle->check_status = DIVOOM_BLE_PACKET_CHECK_SUM_2_STATUS;
                            divoom_task_handle->ble_data_len_sum++;
                        }
                        break;
                    case DIVOOM_BLE_PACKET_CHECK_SUM_2_STATUS:
                        {
                            divoom_task_handle->check_sum_get += task_queue_handle->data[i] << 8;
                            divoom_task_handle->check_status = DIVOOM_BLE_PACKET_TAIL_STATUS;
                            divoom_task_handle->ble_data_len_sum++;

                            //iOS没有发PACKET_TAIL_FLAG，放在这里对长度和checksum进行判断
                            //对数据进行处理
                            //判断check_sum和长度是否匹配
                            //ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "check_sum_get = %d, check_sum_data = %d\n", divoom_task_handle->check_sum_get, divoom_task_handle->check_sum_data);
                            //ESP_LOGI(DIVOOM_BLE_ACCEPT_TAG, "divoom_task_handle->ble_data_len_sum = %d, divoom_task_handle->ble_data_len_get = %d\n", 
                            //      divoom_task_handle->ble_data_len_sum, divoom_task_handle->ble_data_len_get);
                            if(divoom_task_handle->check_sum_get == divoom_task_handle->check_sum_data && (divoom_task_handle->ble_data_len_sum == divoom_task_handle->ble_data_len_get))
                            //if(divoom_task_handle->check_sum_get == divoom_task_handle->check_sum_data && (task_queue_handle.data[i] == PACKET_TAIL_FLAG))
                            {
                                
                                divoom_ble_command_do(divoom_task_handle->ble_accept_data,
                                                      divoom_task_handle->ble_accept_len,
                                                      divoom_task_handle);

                                //数据成功之后把这个置为DIVOOM_BLE_MAYBE_NULL_STATUS
                                //放到这里避免出现刚好剩余sum数据的时候，sum检验不对直接退出，没有办法检验全部的头
                                divoom_task_handle->check_instand_head = DIVOOM_BLE_MAYBE_NULL_STATUS;
                                Wrong_flag = 0;
                            }
                        }
                        break;
                }
                i++;
            }
            if(Wrong_flag == 0)
            {
                divoom_task_handle->check_status = DIVOOM_BLE_PACKET_HEAD_1_STATUS;
                divoom_task_handle->check_instand_head = DIVOOM_BLE_MAYBE_NULL_STATUS;
                divoom_task_handle->check_sum_data = 0;
                divoom_task_handle->check_sum_get = 0;
                divoom_task_handle->ble_accept_len = 0;
                divoom_task_handle->ble_data_len_get = 0;
                divoom_task_handle->ble_data_len_sum = 0;
                divoom_task_handle->ble_packet_len = 0;
                memset((char *)divoom_task_handle->ble_accept_data, 0, DIVOOM_QUEUE_DATA_LEN);
            }
            divoom_system_free(task_queue_handle);
        }
    }
	if(divoom_task_handle->divoom_ble_queue)
	{
		divoom_queue_destroy(divoom_task_handle->divoom_ble_queue, NULL);
	}
	if(divoom_task_handle->divoom_ble_sem)
	{
		divoom_system_destroy_sem(divoom_task_handle->divoom_ble_sem);
	}
    
	divoom_system_free(divoom_task_handle);//put del task before
	return NULL;
}

static void divoom_ble_cle_queue(void)
{
	if(divoom_ble_handle && divoom_ble_handle->divoom_ble_queue)
	{
		//不管是否成功,先暂停连接WiFi,就没有什么信息发过来
		//如果失败了,证明阻塞在接收,队列里面没有消息
		divoom_queue_pop_all(divoom_ble_handle->divoom_ble_queue, NULL);
	}
}

static bool BtDev_AddrToStructure(uint8_t *byteAddr, uint16_t *NAP, uint8_t *UAP, uint32_t *LAP)
{
    if (NULL != NAP)
    {
        // 16bit
        *NAP = (uint16_t)(((uint16_t)byteAddr[5] << 8) + byteAddr[4]);
    }

    if (NULL != UAP)
    {
        // 8 bit
        *UAP = (uint8_t)byteAddr[3];
    }
    
    if (NULL != LAP)
    {
        // 24 bit
        *LAP = ((uint32_t)byteAddr[2] << 16) + \
            ((uint32_t)byteAddr[1] << 8) + byteAddr[0];
    }
    
    return true;
}

/**
 * @BRIEF	check if btdev addr is valid
 * @AUTHOR	zhuangyuping
 * @DATE	2012-05-21
 * @PARAM	uint8_t *byteAddr
 * @RETURN	bool
 * @RETVAL	
 */
bool BtDev_AddrValid(uint8_t *byteAddr)
{
    uint16_t NAP = 0;
    uint8_t  UAP = 0;
    uint32_t LAP = 0;

    if (false == BtDev_AddrToStructure(byteAddr, &NAP, &UAP, &LAP))
    {
        return false;
    }

    if ((0 == LAP) && (0 == UAP) && (0 == NAP))
    {
        return  false; 
    }
    else if (LAP >= 0x9E8B00 && LAP <= 0x9E8B3F)
    {
        return  false; 
    }
    else if (LAP > 0xFFFFFF)
    {
        return  false; 
    }
    
    return true;
}

static bool BtDev_GetDefaultAddr(uint8_t *byteAddr, bool isResetLap)
{
	uint16_t napAddr = 0x51EB;//16bit[vendor]
	uint8_t uapAddr = 0xFA;//8bit[vendor]
	uint32_t  lapAddr = 0x0b11C0;//24bit [user]
	uint32_t timeOut = 200;

	if (isResetLap)
	{
		byteAddr[5] = (napAddr>>8)&0xFF;
		byteAddr[4] = napAddr&0xFF;
		byteAddr[3] = uapAddr;

		while(timeOut--)
		{
			lapAddr = rand();

			byteAddr[2] = (uint8_t)(lapAddr>>16)&0xFF;
			byteAddr[1] = (uint8_t)(lapAddr>>8)&0xFF;
			//byteAddr[0] = (uint8_t)lapAddr &0xFF;
			byteAddr[0] = 0xC0;
            if (true == BtDev_AddrValid(byteAddr))
            {
                break;
            }
		}
	}

	return true;
}

//做成收到WiFi那边返回的WiFi状态之后，发到task那边去处理一下
//void divoom_ble_wifi_status_notify(DIVOOM_WIFI_STATUS status)
//第一位是command,第二,三位是长度,低位在前,后面跟数据
void divoom_ble_wifi_status_notify(uint8_t command, uint32_t size, uint8_t *data)
{
	divoom_print("send ble: %d\n", command);
	if(divoom_ble_handle && divoom_ble_handle->divoom_ble_queue && divoom_ble_handle->divoom_ble_sem)
	{
		divoom_ble_queue_struct *temp_date;

        temp_date = divoom_system_malloc(sizeof(divoom_ble_queue_struct));
		memset(temp_date, 0, sizeof(divoom_ble_queue_struct));
		uint32_t check_sum = 0;
		uint32 off_set = 0;
		uint8_t i = 0;

		temp_date->data[off_set++] = PACKET_HEAD_1_FLAG;
		temp_date->data[off_set++] = PACKET_HEAD_2_FLAG;
		temp_date->data[off_set++] = PACKET_HEAD_3_FLAG;
		temp_date->data[off_set++] = PACKET_HEAD_4_FLAG;
		//len low
		//长度为数据长度+(检验和+WIFI_SEND_DATA_BLE_COMMAND+包标示(固定长度4)) + command + size(两个字节)
		temp_date->data[off_set++] = 4 + (uint8)size + 3;
		//len high
		temp_date->data[off_set++] = (uint8)(size >> 8);
		//包标示
		temp_date->data[off_set++] = 0;
		temp_date->data[off_set++] = WIFI_SEND_DATA_BLE_COMMAND;

		temp_date->data[off_set++] = command;
		temp_date->data[off_set++] = (uint8)size;
		temp_date->data[off_set++] = (uint8)(size >> 8);
		for (i=0;i<size;i++)
		{
			temp_date->data[off_set++] = data[i];
		}
		//checksum=len+命令类型+命令数据(NULL)+包标示(暂时为0)
		for (i=4; i<7+off_set; i++)
		{
			check_sum += temp_date->data[i];
		}
		temp_date->data[off_set++] = check_sum & 0xFF;
		temp_date->data[off_set++] = check_sum >> 8;
		temp_date->data[off_set] = PACKET_TAIL_FLAG;
		temp_date->data_len = off_set;
		if(command == WIFI_STATUS_BLE_COMMAND)
		{
			divoom_ble_handle->wifi_status = data[0];
			divoom_print("wifi status:%d!\n",divoom_ble_handle->wifi_status);
		}
        //divoom_print("send ble!\n");
        divoom_queue_push(divoom_ble_handle->divoom_ble_queue, temp_date);
		divoom_system_put_sem(divoom_ble_handle->divoom_ble_sem);
	}
}

//返回当前配网状态信息
int divoom_ble_get_wifi_status(void)
{
    if(divoom_ble_handle == NULL)
    {
        if(divoom_wifi_model_get_wifi_status() == 0)
        {
            return DIVOOM_WIFI_NOT_CONFIG;
        }
        else
        {
            return DIVOOM_WIFI_CONNECTED_SER;
        }
    }
    return divoom_ble_handle->wifi_status;
}

void divoom_ble_set_wifi_status(int wifi_status)
{
    if(divoom_ble_handle)
    {
        divoom_ble_handle->wifi_status = wifi_status;
    }
}


void *divoom_ble_start(DIVOOM_BLE_CALLBACK callback, uint32_t app_id)
{
	if(divoom_ble_handle)
	{
//		divoom_bt_ctrl_start_ble();
		return divoom_ble_handle;
	}
	divoom_ble_handle = (Pdivoom_ble_struct)divoom_malloc(sizeof(divoom_ble_struct), DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
	memset(divoom_ble_handle, 0, sizeof(divoom_ble_struct));
	divoom_ble_handle->adv_config_done = 0;
	divoom_ble_handle->Is_first_check = 0;
	divoom_ble_handle->callback = callback;
	divoom_ble_handle->wifi_status = DIVOOM_WIFI_NOT_CONFIG;
	divoom_ble_handle->app_id = app_id;
	divoom_ble_handle->run_flag = 1;

	divoom_bt_ctrl_start_ble();
	divoom_print("[WIFI_CFG] ble_start app_id=%u\n", (unsigned)app_id);

	divoom_ble_handle->divoom_ble_queue = divoom_queue_init(100, __func__, __LINE__);
	if(divoom_ble_handle->divoom_ble_queue == NULL)
	{
		divoom_print("Queue creat fail!\n");
		goto WRONG_EXIT;
	}
	divoom_ble_handle->divoom_ble_sem = divoom_system_create_sem(1, 0);
	if(divoom_ble_handle->divoom_ble_sem == NULL)
	{
		divoom_print("Queue creat fail!\n");
		goto WRONG_EXIT;
	}

	divoom_system_create_task(divoom_ble_data_do, "ble_data", divoom_ble_handle, DIVOOM_TASK_STACK_BLE_DATA);


	return (void *)divoom_ble_handle;

WRONG_EXIT:
	//如果任何地方失败了，就重启吧。。。
	return NULL;
}

uint8 divoom_ble_is_run(void)
{
	return (divoom_ble_handle==NULL)?0:1;
}

uint8 divoom_ble_check_time_out(void)
{
    if(divoom_ble_handle == NULL)
    {
        return 0;
    }
    if(divoom_ble_handle->Is_first_check == DIVOOM_BLE_NO_DATA && divoom_ble_handle->divoom_ble_connect_time &&
        divoom_ble_handle->divoom_ble_connect_time + 60 < divoom_system_get_tickms()/1000)
    {
        divoom_print("time out; %d\n", divoom_system_get_tickms()/1000 - divoom_ble_handle->divoom_ble_connect_time);
        return 1;
    }
    else
    {
        return 0;
    }
}

void divoom_ble_stop(uint8 need_release_mem)
{
	if(divoom_ble_handle == NULL)
		return;
	divoom_bt_ctrl_stop_ble();

	if(divoom_ble_handle)
	{
        divoom_ble_handle->run_flag = 0;
        divoom_system_put_sem(divoom_ble_handle->divoom_ble_sem);
		divoom_ble_handle = NULL;
	}



}

