#include "divoom.h"
#include "divoom_wifi_con.h"
#include "divoom_ble_accept_data.h"

#define DIVOOM_APP_CONFIG_TAG					"DIVOOM_APP_CONFIG_WIFI"
#define DIVOOM_APP_CONFIG_STACK					(3072)

typedef struct _divoom_app_config_wifi_struct_
{
    uint8 wifi_connected_ok;    //成功标记
    uint8 wifi_connect_flag;
    uint8 notice_wifi_time_out; //只提醒一次
	void *task_handle;
	void *lock_handle;
	uint32 app_id;
	char ssid[128];
}divoom_app_config_wifi_struct, *Pdivoom_app_config_wifi_struct;

Pdivoom_app_config_wifi_struct pdivoom_app_config_handle = NULL;


void divoom_ble_wifi_status_notify(uint8_t command, uint32_t size, uint8_t *data);

static uint8 divoom_app_config_wifi_event(Pdivoom_sys_msg_info msg)
{
	if(pdivoom_app_config_handle == NULL)
	{
		return DIVOOM_MODEL_NO_INIT;
	}
	divoom_print("%s:Get Event:%d\n", __func__, msg->msg_id);
	switch(msg->msg_id)
	{
        case DIVOOM_APP_CONFIG_WIFI_WIFI_START:
#if DIVOOM_BUILD_RTOS_DEVICE
            /* RTOS 真机：不在 STA_START 时自动连产测 AP；由 divoom_product_test_wifi_init() 显式触发 */
            divoom_print("[WIFI_CFG] skip OTA test AP on STA_START (use product test to connect)\n");
#else
            {
                wifi_sta_cn_para_t cn_para = {0};

                cn_para.ssid = DIVOOM_PRODUCT_TEST_WIFI_SSID;
                cn_para.password = DIVOOM_PRODUCT_TEST_WIFI_PASSWD;
                wifi_sta_connect(&cn_para);
            }
#endif
            break;
            
        case DIVOOM_APP_CONFIG_WIFI_WIFI_CONNECT:
        {
                divoom_print("[WIFI_CFG] wifi connected, save config app_id=%d\n",
                    pdivoom_app_config_handle->app_id);
                pdivoom_app_config_handle->wifi_connected_ok = 1;
				divoom_wifi_save_config();
        }
            break;

        case DIVOOM_APP_CONFIG_WIFI_WIFI_GET_IP:
        {
                divoom_sys_msg_info msg_send = {0};

                divoom_print("[WIFI_CFG] GOT_IP -> MAIN_EVENT_WIFI_CON app_id=%d\n",
                    pdivoom_app_config_handle->app_id);
				msg_send.msg_id = DIVOOM_MAIN_EVENT_WIFI_CON;
				msg_send.src_id = DIVOOM_MODEL_NULL;
				msg_send.dest_id = DIVOOM_MODEL_MAIN;
				msg_send.msg_type = 1;
                if(pdivoom_app_config_handle->app_id)
                {
                    int *app_id = (int *)divoom_system_malloc(sizeof(int));
                    msg_send.data = (uint8 *)app_id;
                    *app_id = pdivoom_app_config_handle->app_id;
                }
				divoom_sys_model_crl_msg_push(&msg_send);
        }
            break;
            
		case DIVOOM_APP_CONFIG_WIFI_GET_SSID:
			{
				divoom_system_get_lock(pdivoom_app_config_handle->lock_handle);
				//msg->result = (int)pdivoom_app_config_handle->ssid;
				divoom_system_free_lock(pdivoom_app_config_handle->lock_handle);
			}
			break;
		case DIVOOM_APP_CONFIG_WIFI_GET_APP_ID:
			{
				divoom_system_get_lock(pdivoom_app_config_handle->lock_handle);
				msg->result = (pdivoom_app_config_handle->app_id != 0)?(pdivoom_app_config_handle->app_id):(0);
				divoom_system_free_lock(pdivoom_app_config_handle->lock_handle);
			}
			break;
		case DIVOOM_APP_CONFIG_WIFI_EXIT_BLE:
			{
                //divoom_bt_ctrl_stop_ble();
			}
			break;
        case DIVOOM_APP_CONFIG_WIFI_BLE_EXIT:
            if(pdivoom_app_config_handle->wifi_connected_ok == 0)
            {
                divoom_bt_ctrl_start_ble();
            }
            break;
            
		case DIVOOM_APP_CONFIG_BACK_MAC_ADDR:
			{
				uint8_t mac[6];
				divoom_main_get_device_mac_addr(mac);
				//回复给手机,当前设备的MAC地址
				divoom_ble_wifi_status_notify(START_BOND_BLE_COMMAND, sizeof(mac), mac);
				break;
			}
		case DIVOOM_APP_CONFIG_WIFI_RESET_USERID:
			{
				divoom_power_on_manage_erase_app_id();
			}
			break;
		default:
			divoom_print("%s:No do this cmd:%d\n", __func__, msg->msg_id);
			break;
	}
	return 1;
}

static void divoom_app_config_wifi_cb(DIVOOM_WIFI_STATUS status, void *data)
{
	divoom_sys_msg_info msg = {0};
	divoom_print("[WIFI_CFG] app_config_cb status=%d\n", status);
	switch (status)
	{
        case DIVOOM_WIFI_RECONFIG:
            if(data)
            {
                pdivoom_app_config_handle->app_id = *(int *)data;
            }
            pdivoom_app_config_handle->notice_wifi_time_out = 1;
            break;
            
        case DIVOOM_WIFI_NO_SSID:
        case DIVOOM_WIFI_OTHER_ERR: 
           if(pdivoom_app_config_handle->notice_wifi_time_out && pdivoom_app_config_handle->wifi_connect_flag)
           {
            
                pdivoom_app_config_handle->notice_wifi_time_out = 0;
    			msg.msg_id = DIVOOM_MAIN_NOTICE_WIFI_NOTICE_MESSAGE;
    			msg.src_id = DIVOOM_MODEL_NULL;
    			msg.dest_id = DIVOOM_MODEL_MAIN;
    			msg.msg_type = 1;
    			divoom_sys_model_crl_msg_push(&msg);
           }
            break;
        
		case DIVOOM_WIFI_CONNECTED:
                pdivoom_app_config_handle->wifi_connect_flag = 0;
				msg.src_id = DIVOOM_MODEL_NULL;
				msg.dest_id = DIVOOM_MODEL_CONFIG_WIFI;
				msg.msg_id = DIVOOM_APP_CONFIG_WIFI_WIFI_CONNECT;
				msg.msg_type = 1;
				divoom_sys_model_crl_msg_push(&msg);
#ifdef DIVOOM_SIMULATOR
            {
                divoom_sys_msg_info msg_ip = {0};

                msg_ip.src_id = DIVOOM_MODEL_NULL;
                msg_ip.dest_id = DIVOOM_MODEL_CONFIG_WIFI;
                msg_ip.msg_id = DIVOOM_APP_CONFIG_WIFI_WIFI_GET_IP;
                msg_ip.msg_type = 1;
                divoom_sys_model_crl_msg_push(&msg_ip);
            }
#endif
			break;
		case DIVOOM_WIFI_GOT_IP:
				msg.src_id = DIVOOM_MODEL_NULL;
				msg.dest_id = DIVOOM_MODEL_CONFIG_WIFI;
				msg.msg_id = DIVOOM_APP_CONFIG_WIFI_WIFI_GET_IP;
				msg.msg_type = 1;
				divoom_sys_model_crl_msg_push(&msg);
			break;
		case DIVOOM_WIFI_DISCONNECTED:
			{
				msg.msg_id = DIVOOM_MAIN_EVENT_WIFI_DISCON;
				msg.src_id = DIVOOM_MODEL_NULL;
				msg.dest_id = DIVOOM_MODEL_MAIN;
				msg.msg_type = 1;
				divoom_sys_model_crl_msg_push(&msg);
			}
			break;
		case DIVOOM_WIFI_START:
			{
				msg.msg_id = DIVOOM_APP_CONFIG_WIFI_WIFI_START;
				msg.src_id = DIVOOM_MODEL_NULL;
				msg.dest_id = DIVOOM_MODEL_CONFIG_WIFI;
				msg.msg_type = 1;
				divoom_sys_model_crl_msg_push(&msg);
			}
			break;
		default :
			break;
	}
}

void *divoom_app_config_wifi_init_task_main(void *para)
{
    uint8 mac[12];
    char *wifi_info;
	int wlan_wait;
	uint8 cur_power_mode = divoom_power_on_manage_get_power_mode();

    divoom_main_get_device_mac_addr(mac);
    srand(divoom_com_unpack_uint32(&mac[2]));

	if (cur_power_mode == DIVOOM_POWER_ON_WIFI_MODE)
	{
        divoom_print("[WIFI_CFG] boot WIFI_MODE, auto reconnect\n");
		if(!divoom_wifi_model_init(divoom_app_config_wifi_cb, 0)) //如果打开之前存储的WiFi的nvs失败,重新配网
		{
			divoom_wifi_model_config();
//			打开WIFI失败，没有足够内存进蓝牙配置模式则进
			divoom_power_on_manage_set_power_mode(DIVOOM_POWER_ON_WIFI_CONFIG_MODE, 1);
		}
		//非配网，也启动BLE
		//divoom_wifi_model_config();
	}
	else if (cur_power_mode == DIVOOM_POWER_ON_WIFI_CONFIG_MODE) //配网 
	{
        divoom_print("[WIFI_CFG] boot WIFI_CONFIG_MODE, enter provisioning\n");
        pdivoom_app_config_handle->wifi_connect_flag = 1;
		divoom_wifi_model_init(divoom_app_config_wifi_cb, 1);
		divoom_wifi_model_config();
	}

#ifndef DIVOOM_SIMULATOR
    wifi_info = divoom_system_malloc(10*1024);
    if (wifi_info)
    {
	    for (wlan_wait = 0; wlan_wait < 30; wlan_wait++)
	    {
	        divoom_exec_cmd((char *)"ifconfig", wifi_info, 10*1024);
	        if(strstr(wifi_info, "wlan0"))
	        {
	            break;
	        }
	        divoom_print("get wifi info and wait: %s\n",wifi_info);
	        divoom_port_delay_ms((1) * 1000U);
	    }
	    if (wlan_wait < 30)
	    {
	        divoom_print("find wlan0 and start wifis!\n");
	    }
	    else
	    {
	        divoom_print("wlan0 not found after wifi init\n");
	    }
	    divoom_system_free(wifi_info);
    }
#else
    divoom_print("sim: wlan0 ready, start wifi init\n");
#endif

    return NULL;
}

//创建一个微任务，处理divoom_wifi_model_init的callback的返回值，避免阻塞底层处理
//不用微任务了,直接处理,不会阻塞很久的
//炫音工厂的不放在这里处理了，另外开个模块处理吧

#ifdef DIVOOM_SIMULATOR
static uint8 s_app_config_wifi_defer;

void divoom_app_config_wifi_sim_finish_init(void)
{
	if (!s_app_config_wifi_defer || pdivoom_app_config_handle == NULL)
	{
		return;
	}
	s_app_config_wifi_defer = 0;
	divoom_app_config_wifi_init_task_main(pdivoom_app_config_handle);
}
#endif

void divoom_app_config_wifi_init(void)
{
	if (pdivoom_app_config_handle != NULL)
	{
		divoom_print("Had been inited!\n");
		return;
	}
	pdivoom_app_config_handle = (Pdivoom_app_config_wifi_struct)divoom_malloc(sizeof(divoom_app_config_wifi_struct), DIVOOM_MODEL_CONFIG_WIFI, __LINE__, __FILE__);
	if (pdivoom_app_config_handle == NULL)
	{
		divoom_print("divoom_malloc error");
		return;
	}
	memset(pdivoom_app_config_handle, 0, sizeof(divoom_app_config_wifi_struct));
	divoom_sys_model_crl_register(DIVOOM_MODEL_CONFIG_WIFI, divoom_app_config_wifi_event);
	pdivoom_app_config_handle->lock_handle = divoom_system_create_lock();
    //创建一个线程查询WIFI正常再初始化WIFI信息
#if !defined(DIVOOM_SIMULATOR) || SIM_USE_REAL_THREADS
    
        uint8 cur_power_mode = divoom_power_on_manage_get_power_mode();
    
        if (cur_power_mode == DIVOOM_POWER_ON_WIFI_MODE)
        {
            divoom_print("[WIFI_CFG] init WIFI_MODE\n");
            if(!divoom_wifi_model_init(divoom_app_config_wifi_cb, 0)) //如果打开之前存储的WiFi的nvs失败,重新配网
            {
                divoom_wifi_model_config();
    //          打开WIFI失败，没有足够内存进蓝牙配置模式则进
                divoom_power_on_manage_set_power_mode(DIVOOM_POWER_ON_WIFI_CONFIG_MODE, 1);
            }
            //非配网，也启动BLE
            //divoom_wifi_model_config();
        }
        else if (cur_power_mode == DIVOOM_POWER_ON_WIFI_CONFIG_MODE) //配网 
        {
            divoom_print("[WIFI_CFG] init WIFI_CONFIG_MODE\n");
            pdivoom_app_config_handle->wifi_connect_flag = 1;
            divoom_wifi_model_init(divoom_app_config_wifi_cb, 1);
            divoom_wifi_model_config();
        }
#else
    {
		if(!divoom_wifi_model_init(divoom_app_config_wifi_cb, 0)) //如果打开之前存储的WiFi的nvs失败,重新配网
		{
			divoom_wifi_model_config();
//			打开WIFI失败，没有足够内存进蓝牙配置模式则进
			divoom_power_on_manage_set_power_mode(DIVOOM_POWER_ON_WIFI_CONFIG_MODE, 1);
		}
    }
#endif
	//只需要考虑配网和正常WiFi模式，蓝牙模式divoom_app_config_wifi_init不会init

}

//在VJ进行关机之后再开机运行此函数
void divoom_app_config_wifi_poweron_init(void)
{
	uint8 cur_poweron_mode = divoom_power_on_manage_get_power_mode();
	divoom_wifi_model_init(divoom_app_config_wifi_cb, (cur_poweron_mode == DIVOOM_POWER_ON_WIFI_MODE)?0:1);
	divoom_wifi_model_config();
}

//关闭WiFi,如果在ble运行状态,关闭ble
void divoom_app_config_wifi_deinit(void)
{
	divoom_wifi_model_deinit();
}

