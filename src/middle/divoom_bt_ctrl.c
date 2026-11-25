#include "divoom.h"


#include <fcntl.h>

#ifdef DIVOOM_USE_RK_IC
#include "utility.h"
#include <RkBtBase.h>
#include <RkBtSink.h>
#include <RkBtSource.h>
#include <RkBle.h>
//#include <RkBtSpp.h>
#include <RkBleClient.h>

//vendor code for broadcom
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/ioctl.h>
#include <glib.h>
#else
#include <bt_manager.h>
#include "bt_dev_list.h"
#include "bt_log.h"
#endif



enum{
	A2DP_SOURCE,
	A2DP_SINK
};

enum{
	ACL_NORMAL_PRIORITY,
	ACL_HIGH_PRIORITY
};
int vendor_set_high_priority(char *ba, uint8_t priority, uint8_t direction);



#if 0



/* AD SERVICE_UUID */
#define AD_SERVICE_UUID16	"1111"
//#define AD_SERVICE_UUID16	"AF30"

#define AD_SERVICE_UUID32	"00002222"
#define AD_SERVICE_UUID128	"00002222-0000-1000-8000-00805f9b34fb"

/* GAP/GATT Service UUID */
#define BLE_UUID_SERVICE	"00001111-0000-1000-8000-00805F9B34FB"
#define BLE_UUID_WIFI_CHAR	"00002222-0000-1000-8000-00805F9B34FB"

#define BLE_UUID_SEND		"49535343-1E4D-4BD9-BA61-23C647249616"
#define BLE_UUID_RECV		"49535343-8841-43F4-A8D4-ECBE34729BB3"

#define SERVICE_UUID		"49535343-FE7D-4AE5-8FA9-9FAFD205E455"


#define BLE_UUID_SERVICE1	"00001234-0000-1000-8000-00805F9B34FB"
#define BLE_UUID_WIFI_CHAR1	"00001235-0000-1000-8000-00805F9B34FB"
#define BLE_UUID_SEND1		"00001236-1810-47f7-8248-eb8be3dc47f9"
#define BLE_UUID_RECV1		"00001237-1810-4a24-94d3-b2c11a851fac"
#define SERVICE_UUID1		"00001238-0000-1000-8000-00805f9b34fb"


#ifdef DIVOOM_USE_RK_IC
static RkBtContent bt_content;
#endif
static struct timeval start, now;
static ssize_t totalBytes;

static volatile bool ble_direct_flag = false;




#define BT_FIFO_SEND DIVOOM_TMP_DIR "/bt_fifo_send"
#define BT_FIFO_ACCEPTE DIVOOM_TMP_DIR "/bt_fifo_accept"


//读写配置
#define BT_CTRL_NAME_MAX_LEN 26
typedef struct _divoom_blue_config_para
{
//    char bt_name[BT_CTRL_NAME_MAX_LEN];
//    char bt_br_name[BT_CTRL_NAME_MAX_LEN];
    char bt_ble_name[BT_CTRL_NAME_MAX_LEN];
    int  ssp_switch;
    char ssp_secret[6];
	char ble_mac_buf[7];	//ble的地址
}divoom_blue_config_para, *Pdivoom_blue_config_para;



typedef struct _divoom_bt_ctrl_info
{

    int bt_send_handle;
    int bt_accept_handle;

    int bond_state;
    int bt_state;
    int sink_state;
    int play_status;
    int system_vol;
	int wifi_config_flag;
	char title[256];
	char artist[256];
	void *lrc_para;
	void *lrc_mux;
	divoom_bt_lrc_callback lrc_callback;
	divoom_blue_config_para cfg;
    DIVOOM_BT_STATUS_CB pbt_state_callback;

    int run_flag;
    void *sem;
	void *queue;
}divoom_bt_ctrl_info, *Pdivoom_bt_ctrl_info;

Pdivoom_bt_ctrl_info pdivoom_bt_ctrl_handle = NULL;


extern void divoom_ble_recv_data_callback(const char *uuid, char *data, int len);




//----------------------------------------------------------------------------------


uint32 divoom_blue_get_random(void)
{
	srand(time(0));
	return rand();
}


int divoom_blue_addr_to_structure(uint8 *byteAddr, uint16 *NAP, uint8 *UAP, uint32 *LAP)
{
    if (AK_NULL != NAP)
    {
        // 16bit
        *NAP = (uint16)(((uint16)byteAddr[0] << 8) + byteAddr[1]);
    }

    if (AK_NULL != UAP)
    {
        // 8 bit
        *UAP = (uint8)byteAddr[2];
    }
    
    if (AK_NULL != LAP)
    {
        // 24 bit
        *LAP = ((uint32)byteAddr[3] << 16) + \
            ((uint32)byteAddr[4] << 8) + byteAddr[5];
    }    
    return 1;
}




int divoom_blue_check_ddr(uint8 *addr)
{
    uint16 NAP = 0;
    uint8  UAP = 0;
    uint32 LAP = 0;

    if (0 == divoom_blue_addr_to_structure(addr, &NAP, &UAP, &LAP))
    {
        return 0;
    }
    if ((0 == LAP) && (0 == UAP) && (0 == NAP))
    {
        return  0; 
    }
    else if (LAP >= 0x9E8B00 && LAP <= 0x9E8B3F)
    {
        return  0; 
    }
    else if (LAP >= 0xFFFFFF)
    {
        return  0; 
    }    
    return 1;
}


int divoom_bt_generate_default_addr(uint8 *byteAddr)
{
#if 0
    uint16 napAddr = 0xB121;//16bit[vendor]
	uint8  uapAddr = 0x81;//8bit[vendor]
	uint32 lapAddr = 0x007199;//24bit [user]
#else
    uint16_t napAddr = 0x7CC5;//16bit[vendor]
	uint8_t  uapAddr = 0x11;//8bit[vendor]
	uint32_t lapAddr = 0x007199;//24bit [user]
#endif
    uint32 timeOut = 200;

	byteAddr[0] = (napAddr>>8)&0xFF;
	byteAddr[1] = napAddr&0xFF;
 	byteAddr[2] = uapAddr;   

	while(timeOut--)
    {
        lapAddr = divoom_blue_get_random();
		byteAddr[3] = (uint8)(lapAddr>>16)&0xFF;
		byteAddr[4] = (uint8)(lapAddr>>8)&0xFF;
		byteAddr[5] = (uint8)lapAddr &0xFF;
        if(divoom_blue_check_ddr(byteAddr))
        {
            break;
        }
    }
	divoom_debug("");
	divoom_print_buf(byteAddr,6);
    return 1;
}



void divoom_bt_ctrl_save_cfg_info(void *data)
{
    int i, j;
    char *json_info;
    divoom_cJSON *root;

	Pdivoom_blue_config_para para = (Pdivoom_blue_config_para)data;
	if(NULL == data)   
	{
		return;
	}

	root = divoom_cJSON_CreateObject();

    if(root == NULL)
    {
        return;
    }
	divoom_cJSON_AddStringToObject(root , "bt_ble_name", para->bt_ble_name);  
    divoom_cJSON_AddStringToObject(root , "ble_mac_buf", para->ble_mac_buf);
	divoom_cJSON_AddStringToObject(root , "ssp_secret", para->ssp_secret);  
    divoom_cJSON_AddNumberToObject(root , "ssp_switch", para->ssp_switch);

    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_BLUE_INFO_CFG, (uint8 *)json_info, strlen(json_info));
    divoom_system_free(json_info);    
}



void divoom_bt_ctrl_load_default_config(void)
{
	divoom_blue_config_para info = {0};

	divoom_bt_generate_default_addr(info.ble_mac_buf);
//	info.bt_ble_name = DIVOOM_WIFI_CONFIG_DEVICE_NAME;
	//默认是配置名
	strcpy(info.bt_ble_name,DIVOOM_WIFI_CONFIG_DEVICE_NAME);
	info.ssp_switch = 0;
	memset(info.ssp_secret,0xff,5);
	divoom_bt_ctrl_save_cfg_info(&info);

	if(pdivoom_bt_ctrl_handle)
	{
		memcpy(&pdivoom_bt_ctrl_handle->cfg,&info,sizeof(divoom_blue_config_para));		
	}
}



int divoom_bt_ctrl_rename_ble_config(char *ble_name)
{
	char *file_name = DIVOOM_BT_MAIN_CONF;
	char *in_file_buf = divoom_app_com_read_file(file_name, NULL);	//读出配置文件所有字符串
	if(in_file_buf == NULL) {
		divoom_debug("no string");
		return 0;
	}

	char *in_cur_str = strstr(in_file_buf,"BleName =");

	if(in_cur_str) {
		
		char name_str[128] = {0};
		int len = 0, off = 0;
		int c;
		in_cur_str += strlen("BleName =");
		char *in_file_last_str = in_cur_str;
		//找出当前行名字
		while( (c = *in_cur_str++) != EOF && len < 128 ) {
			if(' ' == c) {
				off++;
				continue;
			}

			if('\n' == c) {
				break;
			}
			name_str[len++]= c;
			off++;
		}
		name_str[len] = '\0';
		in_file_last_str += off;	//ble名下一行的字串，含"/n"

//		比较名字是否一样，不一样时改名字
		if(strcmp(name_str,ble_name)) {
//			重新写文件
			divoom_print("rename ble,cur:%s,new:%s\n",name_str,ble_name);


			char *out_file_buf = (char *)divoom_system_malloc(strlen(in_file_buf)+256);
			if(out_file_buf)
				strcpy(out_file_buf,in_file_buf);

			char *out_cur_str = strstr(out_file_buf,"BleName =");
			strcpy(out_cur_str,"BleName = ");
			out_cur_str += strlen("BleName = ");

			//复制新的蓝牙名 
			strcpy(out_cur_str,ble_name);
			out_cur_str += strlen(ble_name);

			//复制后半部分的字符
			strcpy(out_cur_str,in_file_last_str);

			FILE *new_fb = divoom_port_file_open(file_name, "w"); // 创建或打开文件用于写入
//			FILE *new_fb = divoom_port_file_open("./example.txt", "w"); // 创建或打开文件用于写入
			if (new_fb == NULL) {
				return 0;
			}
			// 写入内容到文件
			fputs(out_file_buf, new_fb);
			// 关闭文件
			divoom_port_file_close(new_fb);
			divoom_system_free(out_file_buf);
		}

	}
	divoom_system_free(in_file_buf);
	return 1;	
}



void divoom_bt_ctrl_load_system_cfg(void)
{
#if 0
	if(NULL == pdivoom_bt_ctrl_handle)
	{
		divoom_bt_ctrl_rename_ble_config(DIVOOM_WIFI_CONFIG_DEVICE_NAME);
		return;
	}

	if(pdivoom_bt_ctrl_handle->wifi_config_flag)
	{
		//配置WIFI模式
		divoom_bt_ctrl_rename_ble_config(DIVOOM_WIFI_CONFIG_DEVICE_NAME);
	}
	else
#endif
	{
		divoom_bt_ctrl_rename_ble_config(DIVOOM_BLUE_BLE_NORMAL_NAME);
	}

#if 0
    char *json_info;
    json_info = divoom_app_com_read_file(DIVOOM_BT_MAIN_CONF, NULL);
    if(json_info == NULL)
    {
		//如果是空重新创建一个默认的
		divoom_debug("no file create default!");
//		divoom_bt_ctrl_load_default_config();
        return;
    }	
    char *result = strstr(json_info,"BleName =");
	result+=9;
	if(result) {
		char tmpbuf[128] = {0};
		int len = 0;
		int c;
		while( (c = *result++) != EOF && len < 128 ) 
		{
			if(' ' == c)
				continue;
			if('\n' == c)
				break;							
			tmpbuf[len++]= c;
		}
		tmpbuf[len] = '\0';
		divoom_debug("%s",tmpbuf);

		if(strcmp(tmpbuf,DIVOOM_WIFI_CONFIG_DEVICE_NAME))
		{
			//不同的先改成一样
			divoom_debug("name error");

		}
	}
    divoom_system_free(json_info);
#endif
}



int divoom_bt_ctrl_load_cfg(void)
{
	int ret = 0;
    uint8 mac_buf[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

    int valid = 0;
    char *json_info;
    divoom_cJSON *root;
	if(NULL == pdivoom_bt_ctrl_handle)
	{
		divoom_print("pdivoom_power_on_mange_hander null!");
		return 0;
	}
	divoom_bt_ctrl_load_system_cfg();

    json_info = divoom_app_com_read_file(DIVOOM_BLUE_INFO_CFG, NULL);
    if(json_info == NULL)
    {
		//如果是空重新创建一个默认的
		divoom_print("no file create default!");
		divoom_bt_ctrl_load_default_config();
        return 0;
    }
    root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    if(root == NULL)
    {
		divoom_print("no string create default!");
		divoom_bt_ctrl_load_default_config();
        return 0;
    }
    pdivoom_bt_ctrl_handle->cfg.ssp_switch = divoom_cJSON_GetObjectInt(root, "ssp_switch");
	divoom_cJSON_GetObjectString(root, "ble_name",pdivoom_bt_ctrl_handle->cfg.bt_ble_name);
	divoom_cJSON_GetObjectString(root, "ssp_secret",pdivoom_bt_ctrl_handle->cfg.ssp_secret);
	divoom_cJSON_GetObjectString(root, "ble_mac_buf",pdivoom_bt_ctrl_handle->cfg.ble_mac_buf);

	divoom_print("ssp_switch:%d,bt_ble_name:%s",
		pdivoom_bt_ctrl_handle->cfg.ssp_switch,
		pdivoom_bt_ctrl_handle->cfg.bt_ble_name);

    divoom_cJSON_Delete(root);
}

void divoom_bt_ctrl_save_cfg(void)
{
	if(NULL == pdivoom_bt_ctrl_handle)
	{
		return;
	}
	divoom_bt_ctrl_save_cfg_info(&pdivoom_bt_ctrl_handle->cfg);
}




void divoom_bt_ctrl_lrc_set_callback(divoom_bt_lrc_callback callback,void *para)
{
	if(pdivoom_bt_ctrl_handle)
	{
		divoom_system_get_lock(pdivoom_bt_ctrl_handle->lrc_mux);
		pdivoom_bt_ctrl_handle->lrc_para = para;
		pdivoom_bt_ctrl_handle->lrc_callback = callback;
		divoom_system_free_lock(pdivoom_bt_ctrl_handle->lrc_mux);
	}
}

void divoom_bt_ctrl_update_lrc(char *title, char *artist)
{
	if(pdivoom_bt_ctrl_handle)
	{
		divoom_system_get_lock(pdivoom_bt_ctrl_handle->lrc_mux);
		if(pdivoom_bt_ctrl_handle->lrc_callback)
		{
			pdivoom_bt_ctrl_handle->lrc_callback(pdivoom_bt_ctrl_handle->lrc_para,title,artist);
		}
		divoom_system_free_lock(pdivoom_bt_ctrl_handle->lrc_mux);
	}
}


//设置设备音量
void divoom_bt_ctrl_set_music_device_volume(int volume)
{
	int v100;

	/* 手机侧同步范围 0~127：只更新缓存供 UI 使用；听感已由 A2DP 数字音量变化，Soft Volume 保持最大 */
	if (volume < 0)
		volume = 0;
	else if (volume > 127)
		volume = 127;
	v100 = (volume * 100) / 127;
	divoom_print("divoom_bt_ctrl_set_music_device_volume: raw=%d -> %d%%\n", volume, v100);
	divoom_bt_music_set_play_vol_to_device((uint8)v100);
	/* 手机侧已同步数字音量：取消本地同步超时，Soft Volume 拉回 100（正常模式） */
	divoom_bt_music_reset_vol_sync_state();
}


//通知手机音量
void divoom_bt_ctrl_set_sync_volume(int volume)
{
    #ifdef  DIVOOM_USE_RK_IC
	//手机同步音频范围是0~127
	divoom_print("volume:%d",volume);
	if(volume)
	{
		volume = volume * 127 / 100;
	}
	rk_bt_sink_set_volume(volume);
    #endif
}


//----------------------------------------------------------------------------------


static void divoom_bt_ctrl_ble_request_data_callback(const char *uuid, char *data, int *len)
{
    *len = 0;
}

/*
static void divoom_bt_ctrl_bond_state_cb(const char *bd_addr, const char *name, RK_BT_BOND_STATE state)
{
    pdivoom_bt_ctrl_handle->bond_state = state;
    switch (state)
    {
    case RK_BT_BOND_STATE_NONE:
        divoom_print("++++++++++ BT BOND NONE: %s, %s ++++++++++\n", name, bd_addr);
        break;
    case RK_BT_BOND_STATE_BONDING:
        divoom_print("++++++++++ BT BOND BONDING: %s, %s ++++++++++\n", name, bd_addr);
        break;
    case RK_BT_BOND_STATE_BONDED:
        divoom_print("++++++++++ BT BONDED: %s, %s ++++++++++\n", name, bd_addr);
        break;
    }
}
*/

static uint16_t service_handle;

static btmg_callback_t *bt_callback;

#define BLE_UUID_SEND		"49535343-1E4D-4BD9-BA61-23C647249616"
#define BLE_UUID_RECV		"49535343-8841-43F4-A8D4-ECBE34729BB3"

#define SERVICE_UUID		"49535343-FE7D-4AE5-8FA9-9FAFD205E455"

#define CCCD_UUID ((char *)"2902")

static int bt_gatt_server_register_ble_uart_svc()
{
    gatts_add_svc_t svc;
    gatts_add_char_t chr_rx;
    gatts_add_char_t chr_tx;
    gatts_add_char_t chr3;
    gatts_add_desc_t desc;
    gatts_star_svc_t start_svc;

    divoom_print("add service,uuid:%s\n", SERVICE_UUID);
    svc.number = 10;
    svc.uuid = SERVICE_UUID;
    svc.primary = true;
    bt_manager_gatt_server_create_service(&svc);

    chr_rx.permissions = BT_GATT_PERM_READ;
    chr_rx.properties = BT_GATT_CHAR_PROPERTY_WRITE_NO_RESPONSE | BT_GATT_CHAR_PROPERTY_WRITE | BT_GATT_CHAR_PROPERTY_NOTIFY;
    chr_rx.svc_handle = service_handle;
    chr_rx.uuid = BLE_UUID_RECV;
    bt_manager_gatt_server_add_characteristic(&chr_rx);

    chr_tx.permissions = BT_GATT_PERM_WRITE;
    chr_tx.properties = BT_GATT_CHAR_PROPERTY_WRITE_NO_RESPONSE | BT_GATT_CHAR_PROPERTY_WRITE |BT_GATT_CHAR_PROPERTY_NOTIFY;
    chr_tx.svc_handle = service_handle;
    chr_tx.uuid = BLE_UUID_SEND;
    bt_manager_gatt_server_add_characteristic(&chr_tx);

    desc.permissions = BT_GATT_PERM_READ | BT_GATT_PERM_WRITE;
    desc.uuid = CCCD_UUID;
    desc.svc_handle = service_handle;
    bt_manager_gatt_server_add_descriptor(&desc);

    return 0;
}

static void set_adv_param(void)
{
    btmg_le_advertising_parameters_t adv_params;

    adv_params.min_interval = 0x0020;
    adv_params.max_interval = 0x01E0; /*range from 0x0020 to 0x4000*/
    adv_params.own_addr_type = BTMG_LE_RANDOM_ADDRESS;
    adv_params.adv_type = BTMG_LE_ADV_IND; /*ADV_IND*/
    adv_params.chan_map =
            BTMG_LE_ADV_CHANNEL_ALL; /*0x07, *bit0:channel 37, bit1: channel 38, bit2: channel39*/
    adv_params.filter = BTMG_LE_PROCESS_ALL_REQ;
    bt_manager_le_set_adv_param(&adv_params);
}

static void divoom_bt_ctrl_gatt_add_service_cb(gatts_add_svc_msg_t *msg)
{
    if (msg != NULL) {
        service_handle = msg->svc_handle;
        divoom_print("add service handle: %d, handle max number: %d\n",
                                service_handle, msg->handle_num);
    }
}


static int service_char_handle = 0;
static int send_char_handle = 0;
static int secv_char_handle = 0;

static void divoom_bt_ctrl_gatts_add_char_cb(gatts_add_char_msg_t *msg)
{
    if (msg != NULL) {
        divoom_print("add char,uuid: %s,chr handle is 0x%04x\n", msg->uuid, msg->char_handle);
        if (strcmp(msg->uuid, SERVICE_UUID) == 0) {
            service_char_handle = msg->char_handle;
        } else if (strcmp(msg->uuid, BLE_UUID_SEND) == 0) {
            send_char_handle = msg->char_handle;
        } else if (strcmp(msg->uuid, BLE_UUID_RECV) == 0) {
            secv_char_handle = msg->char_handle;
        } 
    }
}

static void divoom_bt_ctrl_gatt_connection_cb(char *addr, gatts_connection_event_t event, int err)
{
    if (event == BT_GATT_CONNECTION) {
        divoom_print("gatt server Connected: %s.\n", addr);
    } else if (event == BT_GATT_DISCONNECT) {
        divoom_print("gatt server Disconnected: %s (reason UNKONW)\n", addr);
    } else {
        divoom_print("gatt server event unkown.\n");
    }
}

static void divoom_bt_ctrl_gatt_char_read_request_cb(gatts_char_read_req_t *chr_read)
{
    char value[1];
    static unsigned char count = 0;
    char dev_name[] = "aw_ble_test_1149";

    divoom_print("trans_id:%d,attr_handle:0x%04x,offset:%d\n", chr_read->trans_id, chr_read->attr_handle,
           chr_read->offset);

    if (chr_read) {
        if (chr_read->attr_handle == send_char_handle) {
            gatts_send_read_rsp_t data;
            data.trans_id = chr_read->trans_id;
            data.attr_handle = chr_read->attr_handle;
            data.status = 0x0b;
            data.auth_req = 0x00;
            value[0] = count;
            data.value = dev_name;
            data.value_len = strlen(dev_name);
            bt_manager_gatt_server_send_read_response(&data);
            return;
        }
        gatts_send_read_rsp_t data;
        data.trans_id = chr_read->trans_id;
        data.attr_handle = chr_read->attr_handle;
        data.status = 0x0b;
        data.auth_req = 0x00;
        value[0] = count;
        data.value = value;
        data.value_len = 1;
        bt_manager_gatt_server_send_read_response(&data);
        count++;
    }
}

static void divoom_bt_ctrl_gatt_char_write_request_cb(gatts_char_write_req_t *char_write)
{
    divoom_print("enter_func %d\n", __LINE__);
    static unsigned char data_value[1] = { 0 };
    int ret = 0;

    if (char_write) {
        divoom_print("attr_handle: 0x%04x,tran_id: %d\n", char_write->attr_handle, char_write->trans_id);
        divoom_print("Value:");
    
        divoom_ble_recv_data_callback(NULL,(unsigned char *)char_write->value,char_write->value_len);
       // bt_manager_hex_dump((char *)" ", 20, (unsigned char *)char_write->value,
       //                     char_write->value_len);
    }

    /*
    gatts_notify_rsp_t data;
    data.attr_handle = char_write->attr_handle;
    data.value = data_value;
    data.value_len = 1;

    data_value[0] ++ ;
    divoom_print("send data value:%d\n",data_value[0]);
    bt_manager_gatt_send_notification(&data);
*/

    divoom_print("char_write->need_rsp: %d\n", char_write->need_rsp);
    if (char_write->need_rsp) {
        gatts_write_rsp_t data;
        data.trans_id = char_write->trans_id;
        data.attr_handle = char_write->attr_handle;
        data.state = BT_GATT_SUCCESS;
        ret = bt_manager_gatt_server_send_write_response(&data);
        if (ret != 0)
            divoom_print("send write response failed!\n");
        else
            divoom_print("send write response success!\n");
    }
}

static void divoom_bt_ctrl_gatt_desc_read_requset_cb(gatts_desc_read_req_t *desc_read)
{
    divoom_print("enter\n");
    char value[1];
    static unsigned char count = 0;
    divoom_print("enter\n");

    divoom_print("trans_id:%d,attr_handle:0x%04x,offset:%d\n", desc_read->trans_id, desc_read->attr_handle,
           desc_read->offset);

    if (desc_read) {
        gatts_send_read_rsp_t data;
        data.trans_id = desc_read->trans_id;
        data.attr_handle = desc_read->attr_handle;
        data.status = 0x0b;
        data.auth_req = 0x00;
        value[0] = count;
        data.value = value;
        data.value_len = 1;
        bt_manager_gatt_server_send_read_response(&data);
        count++;
    }
}

static void divoom_bt_ctrl_gatt_desc_write_request_cb(gatts_desc_write_req_t *desc_write)
{
    int ret = 0;

    divoom_print("enter\n");
    divoom_print("desc_write->need_rsp: %d\n", desc_write->need_rsp);
    divoom_print("desc_write->attr_handle: 0x%04x\n", desc_write->attr_handle);


    if (desc_write->need_rsp) {
        gatts_write_rsp_t data;
        data.trans_id = desc_write->trans_id;
        data.attr_handle = desc_write->attr_handle;
        data.state = BT_GATT_SUCCESS;
        ret = bt_manager_gatt_server_send_write_response(&data);
        if (ret != 0)
            divoom_print("send write response failed!\n");
        else
            divoom_print("send write response success!\n");
    }
}

void bt_gatt_server_register_callback(btmg_callback_t *cb)
{
    cb->btmg_gatt_server_cb.gatts_add_svc_cb = divoom_bt_ctrl_gatt_add_service_cb;
    cb->btmg_gatt_server_cb.gatts_add_char_cb = divoom_bt_ctrl_gatts_add_char_cb;
    cb->btmg_gatt_server_cb.gatts_connection_event_cb = divoom_bt_ctrl_gatt_connection_cb;
    cb->btmg_gatt_server_cb.gatts_char_read_req_cb = divoom_bt_ctrl_gatt_char_read_request_cb;
    cb->btmg_gatt_server_cb.gatts_char_write_req_cb = divoom_bt_ctrl_gatt_char_write_request_cb;
    cb->btmg_gatt_server_cb.gatts_desc_read_req_cb = divoom_bt_ctrl_gatt_desc_read_requset_cb;
    cb->btmg_gatt_server_cb.gatts_desc_write_req_cb = divoom_bt_ctrl_gatt_desc_write_request_cb;
}

static int le_set_adv_data(const char *ble_name, uint16_t uuid)
{
    int dd;
    uint8_t manuf_len;
    int index;
    char advdata[31] = { 0 };
    char uuid_buf[5] = { 0 };
    int advdata_len = 0;

    index = 0;

    advdata[index] = 0x02; /* flag len */
    advdata[index + 1] = 0x01; /* type for flag */
    advdata[index + 2] = 0x1A; //0x05

    index += advdata[index] + 1;

    advdata[index] = strlen(ble_name) + 1; /* name len */
    advdata[index + 1] = 0x09; /* type for local name */
    int name_len;
    name_len = strlen(ble_name);
    strcpy(&(advdata[index + 2]), ble_name);
    index += advdata[index] + 1;

    advdata[index] = 0x03; /* uuid len */
    advdata[index + 1] = 0x03; /* type for complete list of 16-bit uuid */
    advdata[index + 2] = (char)(uuid & 0xFF);
    advdata[index + 3] = (char)((uuid >> 8) & 0xFF);
    index += advdata[index] + 1;

    btmg_scan_rsp_data_t adv;
    adv.data_len = index;
    memcpy(adv.data, advdata, 31);

    BTMG_INFO("ble name = [%s]", ble_name);
    return bt_manager_le_set_scan_rsp_data(&adv);
}

void divoom_bt_enable_ble(void)
{
    
    //system("/etc/bluetooth/bt_init.sh ble_start");

    bt_manager_gatt_server_init();

    bt_gatt_server_register_ble_uart_svc();

    set_adv_param();
    bt_manager_le_set_random_address();
    le_set_adv_data(DIVOOM_WIFI_CONFIG_DEVICE_NAME, 0);
    bt_manager_le_enable_adv(true);

}

static void divoom_bt_ctrl_adapter_status_cb(btmg_adapter_state_t status)
{
    char bt_addr[18] = { 0 };
    char bt_name_buf[64] = { 0 };
    char bt_name[64] = { 0 };

    if (status == BTMG_ADAPTER_OFF) {
        divoom_print("BT is off");
    } else if (status == BTMG_ADAPTER_ON) {
        divoom_print("BT is ON");
        bt_manager_get_adapter_address(bt_addr);
        bt_manager_set_adapter_name(DIVOOM_BLUE_AUDIO_NORMAL_NAME);
        bt_manager_agent_set_io_capability(BTMG_IO_CAP_NOINPUTNOOUTPUT);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
    } else if (status == BTMG_ADAPTER_TURNING_ON) {
        divoom_print("bt is turnning on.");
        divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_BT_ON_EVENT);
    } else if (status == BTMG_ADAPTER_TURNING_OFF) {
        divoom_print("bt is turnning off.");
    }
}


static void divoom_bt_ctrl_avrcp_track_changed_cb(const char *bd_addr,
        btmg_track_info_t track_info) {
    const char *s;
    BTMG_DEBUG("BT playing device: %s", bd_addr ? bd_addr : "(null)");
    s = track_info.title ? track_info.title : "";
    divoom_print("BT playing music title: %s", s);
    s = track_info.artist ? track_info.artist : "";
    divoom_print("BT playing music artist: %s", s);
    s = track_info.album ? track_info.album : "";
    divoom_print("BT playing music album: %s", s);
    BTMG_DEBUG("BT playing music track number: %s", track_info.track_num ? track_info.track_num : "");
    BTMG_DEBUG("BT playing music total number of tracks: %s",
            track_info.num_tracks ? track_info.num_tracks : "");
    BTMG_DEBUG("BT playing music genre: %s", track_info.genre ? track_info.genre : "");
    BTMG_DEBUG("BT playing music duration: %s", track_info.duration ? track_info.duration : "");
}

static int _bt_init(int profile)
{
    btmg_set_log_file_path(DIVOOM_TMP_DIR "/btmg.log");
    if (bt_manager_preinit(&bt_callback) != 0) {
        divoom_print("bt preinit failed!\n");
        return -1;
    }
    
    bt_manager_enable_profile(profile);

    bt_callback->btmg_adapter_cb.adapter_state_cb = divoom_bt_ctrl_adapter_status_cb;
    //bt_callback->btmg_avrcp_cb.avrcp_track_changed_cb = divoom_bt_ctrl_avrcp_track_changed_cb;
    
    if (bt_manager_init(bt_callback) != 0) {
        divoom_print("bt manager init failed.\n");
        return -1;
    }

    bt_gatt_server_register_callback(bt_callback);
    /*BT*/
    bt_manager_enable(true);



    return 0;
Failed:
    bt_manager_deinit(bt_callback);
    return -1;
}
void *dm_bt_test_init(int ble_name_flag)
{
    _bt_init(BTMG_GATT_SERVER_ENABLE);
    return NULL;
}


//清掉蓝牙相关的配置信息
void divoom_bt_ctrl_clear_config(void)
{
	exec_command_system("rm /userdata/lib/bluetooth -rf");
}



//切换ble名
void divoom_bt_ctrl_switch_ble_name(int wifi_config_flag)
{
#ifdef DIVOOM_USE_RK_IC
	char mac[6];
	if(NULL == pdivoom_bt_ctrl_handle)
	{
		return;
	}
	rk_bt_deinit();
	divoom_port_delay_ms((1) * 1000U);
	mac[0] = 0x7C;
	mac[1] = 0xC5;
	mac[2] = 0x11;
	mac[3] = 0xC3;
	mac[4] = 0x9C;
	mac[5] = 0x7C;

//	divoom_bt_generate_default_addr(mac);

	if(pdivoom_bt_ctrl_handle)
	{
		memcpy(mac,pdivoom_bt_ctrl_handle->cfg.ble_mac_buf,6);
		pdivoom_bt_ctrl_handle->wifi_config_flag = wifi_config_flag;
	}
//	divoom_debug("bt_cur_flag:%d",ble_config_onoff);
#if 0
	//改ble设备名
	if(wifi_config_flag)
	{
		//清理蓝牙信息
		divoom_bt_ctrl_clear_config();
		//配置WIFI模式
		divoom_bt_ctrl_rename_ble_config(DIVOOM_WIFI_CONFIG_DEVICE_NAME);
	}
	else
	{
		divoom_bt_ctrl_rename_ble_config(DIVOOM_BLUE_BLE_NORMAL_NAME);
	}
#else
	if(wifi_config_flag)
	{
		//清理蓝牙信息
		divoom_bt_ctrl_clear_config();
	}
#endif
    bt_content.init = false;
	dm_bt_test_init(wifi_config_flag);
	while (bt_content.init == false) {
		divoom_port_delay_ms((1) * 1000U);
		divoom_print("BT TURNING ON ...\n");
	}

	//音频使能可见性
	if (bt_content.profile & PROFILE_A2DP_SINK_HF)
	{
		rk_bt_set_discoverable(1);
//		rk_bt_set_profile(PROFILE_A2DP_SINK_HF, 1);
	}


	if(wifi_config_flag)
	{
		ble_direct_set_adv_param(DIVOOM_WIFI_CONFIG_DEVICE_NAME, mac);
	}
	else
	{
		ble_direct_set_adv_param(DIVOOM_BLUE_BLE_NORMAL_NAME, mac);
	}
	ble_direct_enable_adv();	
    #endif
}




void *divoom_bt_ctrl_task_main(void *arg)
{
    divoom_bt_msg *task_queue_handle;
    Pdivoom_bt_ctrl_info divoom_task_handle = (Pdivoom_bt_ctrl_info)arg;
	int ble_wifi_config_flag = 0;

	if(DIVOOM_POWER_ON_WIFI_CONFIG_MODE == divoom_power_on_manage_get_power_mode())
	{
		ble_wifi_config_flag = 1;
	}

	dm_bt_test_init(ble_wifi_config_flag);
#ifdef DIVOOM_USE_RK_IC
	while (bt_content.init == false) {
		divoom_port_delay_ms((1) * 1000U);
		divoom_print("BT TURNING ON ...\n");
	}
	//音频使能可见性
	if (bt_content.profile & PROFILE_A2DP_SINK_HF)
	{
		rk_bt_set_discoverable(1);
//		rk_bt_set_profile(PROFILE_A2DP_SINK_HF, 1);
	}
//	rk_bt_set_pairable(0);
#endif

    while(divoom_task_handle->run_flag)
    {
        divoom_system_get_sem(divoom_task_handle->sem);
		if(divoom_task_handle->run_flag == 0)
		{
			break;
		}
		while(divoom_queue_items(divoom_task_handle->queue))
		{
			task_queue_handle = divoom_queue_pop(divoom_task_handle->queue);
			if(task_queue_handle == NULL)
			{
				break;
			}

			switch(task_queue_handle->msg_id)
			{
				case DIVOOM_BT_EVENT_SW_BLE_NAME:
					{
						divoom_bt_ctrl_switch_ble_name(task_queue_handle->value);
					}
					break;
				case DIVOOM_BT_EVENT_ENABLE_BLE:
					{
						divoom_bt_enable_ble();
					}
					break;

				default:
					break;
			}
			divoom_system_free(task_queue_handle);
		}

    }
	return NULL;
}





void divoom_bt_ctrl_init(DIVOOM_BT_STATUS_CB callback)
{
    int len, ble_name_len, remain_len;
	int ble_wifi_config_flag = 0;
    pdivoom_bt_ctrl_handle = divoom_system_malloc(sizeof(divoom_bt_ctrl_info));
    if(pdivoom_bt_ctrl_handle == NULL)
    {
        return;
    }
	memset(pdivoom_bt_ctrl_handle,0,sizeof(divoom_bt_ctrl_info));
    pdivoom_bt_ctrl_handle->pbt_state_callback = callback;
	pdivoom_bt_ctrl_handle->lrc_mux = divoom_system_create_lock();
    
    
	if(DIVOOM_POWER_ON_WIFI_CONFIG_MODE == divoom_power_on_manage_get_power_mode())
	{
		ble_wifi_config_flag = 1;
		pdivoom_bt_ctrl_handle->wifi_config_flag = 1;

	}
	divoom_bt_ctrl_load_cfg();


	pdivoom_bt_ctrl_handle->run_flag = 1;
	pdivoom_bt_ctrl_handle->queue = divoom_queue_init(100);
	if(pdivoom_bt_ctrl_handle->queue == NULL)
	{
		divoom_print("Queue creat fail!\n");
	}

	pdivoom_bt_ctrl_handle->sem = divoom_system_create_sem(1, 0);
	if(pdivoom_bt_ctrl_handle->sem == NULL)
	{
		divoom_print("Queue creat fail!\n");
	}
	divoom_system_create_task(divoom_bt_ctrl_task_main, "bt_ctrl", pdivoom_bt_ctrl_handle, DIVOOM_TASK_STACK_BT_CTRL);
#if 0
	dm_bt_test_init(ble_wifi_config_flag);

	while (bt_content.init == false) {
		divoom_port_delay_ms((1) * 1000U);
		divoom_print("BT TURNING ON ...\n");
	}


	//音频使能可见性
	if (bt_content.profile & PROFILE_A2DP_SINK_HF)
	{
		rk_bt_set_discoverable(1);
//		rk_bt_set_profile(PROFILE_A2DP_SINK_HF, 1);
	}

//	rk_bt_set_pairable(0);
#endif

#if 0
	//ble adv tests
	rk_ble_adv_start();
	
	while (bt_content.ble_content.ble_advertised == false) {
		divoom_port_delay_ms((1) * 1000U);
		divoom_print("BT ADV ON ...\n");
	}
#endif

}

void divoom_bt_ctrl_put_msg(divoom_bt_msg *pmsg)
{
    divoom_bt_msg *task_queue_handle;
	if(NULL == pdivoom_bt_ctrl_handle || NULL == pmsg)
	{
		return;
	}
    task_queue_handle = divoom_system_malloc(sizeof(divoom_bt_msg));
    if(task_queue_handle)
    {
        memcpy(task_queue_handle, pmsg, sizeof(divoom_bt_msg));
        divoom_queue_push(pdivoom_bt_ctrl_handle->queue,task_queue_handle);
		divoom_system_put_sem(pdivoom_bt_ctrl_handle->sem);
    }
}

//切换ble名
void divoom_bt_ctrl_set_wifi_config(int wifi_config_flag)
{
//	divoom_bt_ctrl_switch_ble_name(wifi_config_flag);
	divoom_bt_msg msg;
	msg.msg_id = DIVOOM_BT_EVENT_SW_BLE_NAME;
	msg.value = wifi_config_flag;
	divoom_bt_ctrl_put_msg(&msg);
}


int divoom_bt_ctrl_get_music_status(void)
{
	if(pdivoom_bt_ctrl_handle)
	{
		return pdivoom_bt_ctrl_handle->play_status;
	}
	return 0;
}

void divoom_bt_ctrl_music_play_ctrl(uint8 play)
{
#ifdef DIVOOM_USE_RK_IC
    if(play)
    {
		rk_bt_sink_media_control("play");
    }
    else
    {
		rk_bt_sink_media_control("pause");
    }
    #endif
}

void divoom_bt_ctrl_music_play_next(void)
{
#ifdef DIVOOM_USE_RK_IC
//	{"bt a2dp sink avrcp control	(num_index input play/pause/next/previous)", bt_test_sink_media_control},
	rk_bt_sink_media_control("next");
#endif
}

void divoom_bt_ctrl_music_play_prev(void)
{
#ifdef DIVOOM_USE_RK_IC
	rk_bt_sink_media_control("previous");
#endif
}

void divoom_bt_ctrl_start_ble(void)
{
	divoom_bt_msg msg;
	msg.msg_id = DIVOOM_BT_EVENT_ENABLE_BLE;
	divoom_bt_ctrl_put_msg(&msg);
}

void divoom_bt_ctrl_ble_send_data(uint8_t *data, uint8_t len)
{
	if(NULL == pdivoom_bt_ctrl_handle)
	{
		return;
	}
#ifdef DIVOOM_USE_RK_IC

	rk_ble_send_notify(BLE_UUID_SEND, data, len);
#else

    gatts_notify_data_t notify_data;
    notify_data.attr_handle = send_char_handle;
    notify_data.value = data;
    notify_data.value_len = len;

    if (data == NULL || len == 0) {
        notify_data.value = data;
        notify_data.value_len = len;
    }
    bt_manager_gatt_server_send_notify(&notify_data);
#endif
}


void divoom_bt_ctrl_stop_ble(void)
{
	if(NULL == pdivoom_bt_ctrl_handle)
	{
		return;
	}
//    rk_ble_disconnect();
//    rk_ble_stop();
    #ifdef DIVOOM_USE_RK_IC
	rk_ble_adv_stop();
    #else
    bt_manager_gatt_server_deinit();
    #endif
}





void divoom_bt_test(int flag)
{
	static int bt_cur_flag = 1;

	if(bt_cur_flag)
	{
		bt_cur_flag =  0;
	}
	else
	{
		bt_cur_flag = 1;
	}
//	divoom_bt_ctrl_reload(bt_cur_flag);
	divoom_print("key flag: %d\n", flag);

	switch(flag)
	{
		case 0:

			break;
		case 1:
			divoom_bt_ctrl_music_play_ctrl(0);

			break;

		case 2:
			divoom_bt_ctrl_music_play_ctrl(1);
			break;
		case 3:
			divoom_bt_ctrl_music_play_next();
			break;
		case 4:
			divoom_bt_ctrl_music_play_prev();
			break;
	}
}

#else

void divoom_bt_ctrl_init(DIVOOM_BT_STATUS_CB callback){}
void divoom_bt_ctrl_music_play_ctrl(uint8 play){}
void divoom_bt_ctrl_music_play_next(void){}
void divoom_bt_ctrl_music_play_prev(void){}
int divoom_bt_ctrl_get_music_status(void)
{
	return 0;
}

void divoom_bt_ctrl_set_wifi_config(int wifi_config_flag)
{
}


#define BT_FIFO_SEND DIVOOM_TMP_DIR "/bt_fifo_send"
#define BT_FIFO_ACCEPTE DIVOOM_TMP_DIR "/bt_fifo_accept"


typedef struct _divoom_bt_ctrl_info
{
    uint8 file_buf[512];
    int bt_send_handle;
    int bt_accept_handle;
    int run_flag;
    void *lock_handle;
}divoom_bt_ctrl_info, *Pdivoom_bt_ctrl_info;

Pdivoom_bt_ctrl_info pdivoom_bt_ctrl_handle = NULL;

void divoom_ble_recv_data_callback(const char *uuid, char *data, int len);

void* divoom_bt_ctrl_main_task(void *param)
{
    int read_len;

    for(int i = 0;i < 2; i++)
    {
        pdivoom_bt_ctrl_handle->bt_accept_handle = open(BT_FIFO_SEND, O_RDONLY | O_NONBLOCK);
        if(pdivoom_bt_ctrl_handle->bt_accept_handle > 0)
        {
            break;
        }
        if(i == 0)
        {
            divoom_system_delay_ms(1000);
        }
    }
    
    if(pdivoom_bt_ctrl_handle->bt_accept_handle <= 0)
    {
        divoom_print("open file err!\n");
        goto divoom_bt_ctrl_main_task_fail_table;
    }
    
    
    pdivoom_bt_ctrl_handle->bt_send_handle = open(BT_FIFO_ACCEPTE, O_WRONLY);
    if(pdivoom_bt_ctrl_handle->bt_send_handle <= 0)
    {
        divoom_print("open file err!\n");    
        goto divoom_bt_ctrl_main_task_fail_table;
    }
    
    while(pdivoom_bt_ctrl_handle->run_flag)
    {
        fd_set read_fds;
        struct timeval timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(pdivoom_bt_ctrl_handle->bt_accept_handle, &read_fds);
        timeout.tv_sec = 1;  // 1秒超时
        timeout.tv_usec = 0;
        
        int ret = select(pdivoom_bt_ctrl_handle->bt_accept_handle + 1, &read_fds, NULL, NULL, &timeout);
        if(ret < 0)
        {
            // select出错，退出循环
            divoom_debug("select error: %d\n", errno);
            break;
        }
        if(ret == 0 || !pdivoom_bt_ctrl_handle->run_flag)
        {
            // 超时或run_flag被清除，继续检查
            continue;
        }
        
        read_len = read(pdivoom_bt_ctrl_handle->bt_accept_handle ,pdivoom_bt_ctrl_handle->file_buf, 512);
        
        if(read_len <= 0)
        {
            if(read_len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                // 非阻塞模式下没有数据，继续等待
                continue;
            }
            break;
        }
        
        divoom_ble_recv_data_callback(NULL, pdivoom_bt_ctrl_handle->file_buf, read_len);
    }
    
    Pdivoom_bt_ctrl_info phandle = pdivoom_bt_ctrl_handle;
    divoom_system_get_lock(pdivoom_bt_ctrl_handle->lock_handle);
    pdivoom_bt_ctrl_handle = NULL;
    if(phandle->bt_accept_handle >= 0)
    {
        close(phandle->bt_accept_handle);
    }
    if(phandle->bt_send_handle >= 0)
    {
        close(phandle->bt_send_handle);
    }
    divoom_system_free_lock(phandle->lock_handle);
    divoom_system_destroy_lock(phandle->lock_handle);
    divoom_system_free(phandle);
    phandle = NULL;
    divoom_bt_ctrl_main_task_fail_table:
    divoom_main_push_model_event_id(DIVOOM_MODEL_CONFIG_WIFI, DIVOOM_APP_CONFIG_WIFI_BLE_EXIT);
    return NULL;
}
//
void divoom_bt_ctrl_start_ble(void)
{
    if(pdivoom_bt_ctrl_handle)
    {
        return;
    }
    pdivoom_bt_ctrl_handle = divoom_system_malloc(sizeof(divoom_bt_ctrl_info));
    pdivoom_bt_ctrl_handle->run_flag = 1;
    pdivoom_bt_ctrl_handle->lock_handle = divoom_system_create_lock();
    
    //system("/usr/bin/divoom_bt_manage > /dev/null 2>&1 &");
    divoom_system_create_task(divoom_bt_ctrl_main_task, "bt_main", pdivoom_bt_ctrl_handle, DIVOOM_TASK_STACK_BT_MAIN);
    
}
void divoom_bt_ctrl_ble_send_data(uint8_t *data, uint8_t len)
{
    if(pdivoom_bt_ctrl_handle && pdivoom_bt_ctrl_handle->bt_send_handle >= 0)
    {
        int ret = write(pdivoom_bt_ctrl_handle->bt_send_handle, data, len);
    }
}
void divoom_bt_ctrl_reset_ble(void)
{
    int try_times = 0;

    if(pdivoom_bt_ctrl_handle)
    {
        divoom_bt_ctrl_stop_ble();
        while(pdivoom_bt_ctrl_handle && try_times < 100)
        {
            divoom_system_delay_ms(50);
            try_times ++;
        }
        divoom_bt_ctrl_start_ble();
    }
}

void divoom_bt_ctrl_stop_ble(void)
{
    if(pdivoom_bt_ctrl_handle)
    {
        divoom_system_get_lock(pdivoom_bt_ctrl_handle->lock_handle);
        pdivoom_bt_ctrl_handle->run_flag = 0;
        //关闭句柄，触发退出
        if(pdivoom_bt_ctrl_handle->bt_send_handle >= 0)
        {
            close(pdivoom_bt_ctrl_handle->bt_send_handle);
            pdivoom_bt_ctrl_handle->bt_send_handle = -1;
        }
        if(pdivoom_bt_ctrl_handle->bt_accept_handle >= 0)
        {
            close(pdivoom_bt_ctrl_handle->bt_accept_handle);
            pdivoom_bt_ctrl_handle->bt_accept_handle = -1;
        }
        divoom_system_free_lock(pdivoom_bt_ctrl_handle->lock_handle);
    }
}



