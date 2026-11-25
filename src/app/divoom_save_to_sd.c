#include "divoom.h"

#if 0
/*
配置文件保存的格式：文件长度（四个字节）+文件数据
所有配置文件保存在一个文件里面
*/

#define DIVOOM_SD_LIGHT_APP_BASE_LIGHT_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/base_light.cfg"
#define DIVOOM_SD_ALL_CLOCK_LIST_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/all_clock.cfg"
#define DIVOOM_SD_USER_DEFINE_PIC_VERSION_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/clock_user.cfg"
#define DIVOOM_SD_ALL_FONT_LIST_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/font_list.cfg"
#define DIVOOM_SD_ALL_APP_PIC_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/app_pic.cfg"
#define DIVOOM_SD_DEIVCE_POWERON_PIC_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/poweron_pic.bin"
#define DIVOOM_SD_DEIVCE_CLOCK_USER_HEAD_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/user_head.bin"
#define DIVOOM_SD_TIME_PLAN_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/time_plan.cfg"
#define DIVOOM_SD_SYS_CFG_SAVE_PATH DIVOOM_LEGACY_SD_ROOT "/system_config.cfg"
#define DIVOOM_SD_SLEEP_SPIFFS_CONFIG  DIVOOM_LEGACY_SD_ROOT "/divoom_sleep_config.cfg"
#define DIVOOM_SD_ALARM_INFO_PATH    DIVOOM_LEGACY_SD_ROOT "/divoom_alarm_config.cfg"

#define DIVOOM_SD_CONFIG_FILE_DIR				DIVOOM_LEGACY_SD_ROOT "/divoom_user_config"
#define DIVOOM_SD_LOG_FILE_DIR					DIVOOM_LEGACY_SD_ROOT "/divoom_user_log"
#define DIVOOM_SD_MAC_FILE_DIR					DIVOOM_LEGACY_SD_ROOT "/divoom_user_mac"

#define LOG_FILE_SAVE_MAX (256)
#define LOG_FILE_WRITE_MAX (256 + 128)
#define LOG_FILE_WRITE_TIME (3)

typedef struct _divoom_save_to_sd_file_info
{
	FILE *log_fp;
	uint8 write_log_flag;
	uint8 log_on;
	void *log_lock;
    int     upload_time;    //上一次上传文件的时间
    int 	write_time;
    int     cache_size;     //缓冲区数据大小
    char    out_buf[LOG_FILE_SAVE_MAX + 10];
    char    buf[LOG_FILE_WRITE_MAX];
}divoom_save_to_sd_file_info, *Pdivoom_save_to_sd_file_info;

static Pdivoom_save_to_sd_file_info pdivoom_save_sd_handle = NULL;

char config_list[11][50] = {
	DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH,
	DIVOOM_ALL_CLOCK_LIST_SAVE_PATH,	
	DIVOOM_USER_DEFINE_PIC_VERSION_SAVE_PATH,
	DIVOOM_ALL_FONT_LIST_SAVE_PATH,
	DIVOOM_ALL_APP_PIC_SAVE_PATH,
	DIVOOM_DEIVCE_POWERON_PIC_SAVE_PATH,
	DIVOOM_DEIVCE_CLOCK_USER_HEAD_SAVE_PATH,
	DIVOOM_TIME_PLAN_SAVE_PATH,
	DIVOOM_SYS_CFG_SAVE_PATH,
	DIVOOM_SLEEP_SPIFFS_CONFIG,
	DIVOOM_ALARM_INFO_PATH
};

char sd_config_list[11][50] = {
	DIVOOM_SD_LIGHT_APP_BASE_LIGHT_SAVE_PATH,
	DIVOOM_SD_ALL_CLOCK_LIST_SAVE_PATH,	
	DIVOOM_SD_USER_DEFINE_PIC_VERSION_SAVE_PATH,
	DIVOOM_SD_ALL_FONT_LIST_SAVE_PATH,
	DIVOOM_SD_ALL_APP_PIC_SAVE_PATH,
	DIVOOM_SD_DEIVCE_POWERON_PIC_SAVE_PATH,
	DIVOOM_SD_DEIVCE_CLOCK_USER_HEAD_SAVE_PATH,
	DIVOOM_SD_TIME_PLAN_SAVE_PATH,
	DIVOOM_SD_SYS_CFG_SAVE_PATH,
	DIVOOM_SD_SLEEP_SPIFFS_CONFIG,
	DIVOOM_SD_ALARM_INFO_PATH
};

/*
  Function:       divoom_save_to_sd_read_config
  Description:    把配置文件读出来并写进sd卡
  Input:          NONE 
  Return:         1 成功  0失败
  Author:         wangft
  Data:           2021-11-4
*/

int divoom_save_to_sd_read_config(void)
{
	//解析数据
	FILE *fp = NULL, *fp_config = NULL;
	int length = 0, i = 0, ret = 0, read_len = 0;
	char *read_buf = divoom_system_malloc(1024);
	if (read_buf == NULL)
	{
		return 0;
	}

	fp = divoom_port_file_open(DIVOOM_SD_CONFIG_FILE_DIR, "r");

	if (!fp)
	{
		divoom_print("open failed\n");
		divoom_system_free(read_buf);
		return 0;
	}

	for (i = 0; i < 11; i++)
	{
		fp_config = divoom_port_file_open(&sd_config_list[i][0], "w");

		if (fp_config == NULL)
		{
			divoom_port_file_close(fp);
			divoom_system_free(read_buf);
			divoom_print("open failed\n");
			return 0;
		}

		divoom_port_file_read(&length, sizeof(length), 1, fp);
		divoom_print("length %d\n", length);
		while (length)
		{
			if (length > 1024)
			{
				read_len = 1024;
			}
			else
			{
				read_len = length;
			}
			
			ret = divoom_port_file_read(read_buf, 1, read_len, fp);
			divoom_port_file_write(read_buf, 1, ret, fp_config);
			divoom_print("length %d ret %d\n", length, ret);
			length -= ret;
		}
		divoom_print("length %d\n", length);
		divoom_port_file_close(fp_config);
	}
	divoom_port_file_close(fp);
	return 1;
}

/*
  Function:       divoom_save_to_sd_config
  Description:    保存用户配置文件
  Input:          NONE 
  Return:         1 成功  0失败
  Author:         wangft
  Data:           2021-11-4
*/

int divoom_save_to_sd_write_config2(void)
{
	//读取配置文件列表
	int i = 0, length = 0, ret = 0, read_len = 0;
	FILE *fp_conifg, *fp_sd;

	fp_sd = divoom_port_file_open(DIVOOM_SD_CONFIG_FILE_DIR, "w");
	if (fp_sd == NULL)
		return 0;
	
	char *read_buf = divoom_system_malloc(1024);
	if (read_buf == NULL)
	{
		divoom_port_file_close(fp_sd);
		return 0;
	}

	for (i = 0; i < 11; i++)
	{
		fp_conifg = divoom_port_file_open(&config_list[i][0], "r");
		divoom_print("config path: %s\n", &config_list[i][0]);
		if (fp_conifg == NULL)
		{
			divoom_print("open failed\n");
			continue;
		}

		divoom_port_file_seek(fp_conifg, 0, SEEK_END);
		length = divoom_port_file_tell(fp_conifg);
		divoom_port_file_seek(fp_conifg, 0, SEEK_SET);
		divoom_print("length %d\n", length);
		divoom_port_file_write(&length, sizeof(length), 1, fp_sd); //先写长度
		while (length) //写数据
		{
			if (length > 1024)
				read_len = 1024;
			else
				read_len = length;
		
			ret = divoom_port_file_read(read_buf, 1, read_len, fp_conifg);
			length -= ret;
			divoom_port_file_write(read_buf, 1, ret, fp_sd);
		}
		divoom_print("length %d\n", length);
		divoom_port_file_close(fp_conifg);
	}
	divoom_port_file_close(fp_sd);
	divoom_system_free(read_buf);
	return 1;
}

int divoom_save_to_sd_write_config(void)
{
	//读取配置文件列表
	int i = 0, length = 0, ret = 0, read_len = 0;
	FILE *fp_conifg, *fp_sd;

	char *read_buf = divoom_system_malloc(1024);
	if (read_buf == NULL)
	{
		return 0;
	}

	for (i = 0; i < 11; i++)
	{
		fp_conifg = divoom_port_file_open(&config_list[i][0], "r");
		divoom_print("config path: %s\n", &config_list[i][0]);

		if (fp_conifg == NULL)
		{
			divoom_print("open failed\n");
			continue;
		}

		fp_sd = divoom_port_file_open(&sd_config_list[i][0], "w");
		divoom_print("config path: %s\n", &sd_config_list[i][0]);
		if (fp_sd == NULL)
		{
			divoom_print("open failed\n");
			divoom_port_file_close(fp_conifg);
			continue;
		}

		divoom_port_file_seek(fp_conifg, 0, SEEK_END);
		length = divoom_port_file_tell(fp_conifg);
		divoom_port_file_seek(fp_conifg, 0, SEEK_SET);
		while (length) //写数据
		{
			if (length > 1024)
				read_len = 1024;
			else
				read_len = length;
		
			ret = divoom_port_file_read(read_buf, 1, read_len, fp_conifg);
            if(ret <= 0)
            {
                break;
            }
			length -= ret;
			divoom_port_file_write(read_buf, 1, ret, fp_sd);
			divoom_system_delay_ms(5);
		}
		divoom_port_file_close(fp_conifg);
		divoom_port_file_close(fp_sd);
	}
	divoom_system_free(read_buf);
	return 1;
}

int divoom_save_restore_flash_write_config(void)
{
	//读取配置文件列表
	int i = 0, length = 0, ret = 0, read_len = 0;
	FILE *fp_conifg, *fp_sd;

	char *read_buf = divoom_system_malloc(1024);
	if (read_buf == NULL)
	{
		return 0;
	}

	for (i = 0; i < 11; i++)
	{
		fp_conifg = divoom_port_file_open(&sd_config_list[i][0], "r");
		divoom_print("config path: %s\n", &config_list[i][0]);

		if (fp_conifg == NULL)
		{
			divoom_print("open failed\n");
			continue;
		}

		fp_sd = divoom_port_file_open(&config_list[i][0], "w");
		divoom_print("config path: %s\n", &sd_config_list[i][0]);
		if (fp_sd == NULL)
		{
			divoom_print("open failed\n");
			divoom_port_file_close(fp_conifg);
			continue;
		}

		divoom_port_file_seek(fp_conifg, 0, SEEK_END);
		length = divoom_port_file_tell(fp_conifg);
		divoom_port_file_seek(fp_conifg, 0, SEEK_SET);
		while (length) //写数据
		{
			if (length > 1024)
				read_len = 1024;
			else
				read_len = length;
		
			ret = divoom_port_file_read(read_buf, 1, read_len, fp_conifg);
			length -= ret;
			divoom_port_file_write(read_buf, 1, ret, fp_sd);
			divoom_system_delay_ms(5);
		}
		divoom_port_file_close(fp_conifg);
		divoom_port_file_close(fp_sd);
	}
	divoom_system_free(read_buf);
	return 1;
}

char *divoom_save_to_sd_get_buf(void)
{
	 if(pdivoom_save_sd_handle == NULL || pdivoom_save_sd_handle->log_on == 0)
        return NULL;

	 return pdivoom_save_sd_handle->out_buf;
}

void divoom_save_to_sd_log_write(char *buf, int cur_size)
{
    if(pdivoom_save_sd_handle == NULL)
    {
        return;
    }

	if (divoom_system_get_tickms() > 40000)
	{
		//大于40s之后
		if (divoom_power_on_manage_get_power_mode() != DIVOOM_POWER_ON_WIFI_CONFIG_MODE) //用户是否在配网状态
		{
			return;
		}
	}

	divoom_system_get_lock(pdivoom_save_sd_handle->log_lock);
	if (pdivoom_save_sd_handle->log_on == 0)
	{
		divoom_system_free_lock(pdivoom_save_sd_handle->log_lock);
		return ;
	}

	if (divoom_system_get_tickms() > 10000) //大于10s自动写
	{
		if (pdivoom_save_sd_handle->write_time < time(0))
		{
			pdivoom_save_sd_handle->write_time = time(0) + 10;
			if (pdivoom_save_sd_handle->log_fp != NULL)
			{
				divoom_port_file_close(pdivoom_save_sd_handle->log_fp);
				pdivoom_save_sd_handle->log_fp = divoom_port_file_open(DIVOOM_SD_LOG_FILE_DIR, "r+"); 
				if (pdivoom_save_sd_handle->log_fp == NULL)
				{
					//divoom_print_err("open failed\n");
					pdivoom_save_sd_handle->log_on = 0;
					divoom_system_free_lock(pdivoom_save_sd_handle->log_lock);
					return;
				}
				divoom_port_file_seek(pdivoom_save_sd_handle->log_fp, 0, SEEK_END);
			}
			
		}
	}
	
	if (pdivoom_save_sd_handle->write_log_flag == 0)
	{
		pdivoom_save_sd_handle->write_log_flag = 1;
		pdivoom_save_sd_handle->log_fp = divoom_port_file_open(DIVOOM_SD_LOG_FILE_DIR, "w");
		if (pdivoom_save_sd_handle->log_fp == NULL)
		{
			//divoom_print_err("open failed\n");
			pdivoom_save_sd_handle->log_on = 0;
			divoom_system_free_lock(pdivoom_save_sd_handle->log_lock);
			return;
		}
	}

    if (pdivoom_save_sd_handle->cache_size + cur_size > LOG_FILE_WRITE_MAX || pdivoom_save_sd_handle->upload_time < time(0))
    {
        if(pdivoom_save_sd_handle->cache_size)
        {
			divoom_port_file_write(pdivoom_save_sd_handle->buf, 1, pdivoom_save_sd_handle->cache_size, pdivoom_save_sd_handle->log_fp);
			//fflush(pdivoom_save_sd_handle->log_fp);//这个函数没有作用
			memset(pdivoom_save_sd_handle->buf, 0, LOG_FILE_WRITE_MAX);
        }
        pdivoom_save_sd_handle->cache_size = 0;
        pdivoom_save_sd_handle->upload_time = time(0) + LOG_FILE_WRITE_TIME;
    }
	
    strcpy(&pdivoom_save_sd_handle->buf[pdivoom_save_sd_handle->cache_size], buf);
    pdivoom_save_sd_handle->cache_size += strlen(buf);
	divoom_system_free_lock(pdivoom_save_sd_handle->log_lock);
}

void divoom_save_to_sd_write_mac(void)
{
	FILE *fp;
	uint8 mac[6], device_name[14];
    #ifdef DIVOOM_TEST_CUSTOM_DEVICE
    memcpy(mac, DIVOOM_CUSTOM_DEVICE_MAC_ADDR, 6);
    #else
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    #endif
	sprintf((char *)device_name, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	fp = divoom_port_file_open(DIVOOM_SD_MAC_FILE_DIR, "w");

	if (fp == NULL)
		return;

	divoom_port_file_write(device_name, 12, 1, fp);
	divoom_port_file_close(fp);
}

void divoom_save_to_sd_task(void *param)
{
	divoom_save_to_sd_write_mac();
	divoom_save_to_sd_write_config();
}

void divoom_save_to_sd_start(void)
{
    return;
	if (pdivoom_save_sd_handle)
		return;

	pdivoom_save_sd_handle = divoom_system_malloc(sizeof(divoom_save_to_sd_file_info));
	memset(pdivoom_save_sd_handle, 0, sizeof(divoom_save_to_sd_file_info));
	pdivoom_save_sd_handle->log_lock = divoom_system_create_lock();
	pdivoom_save_sd_handle->log_on = 1; //log 开关
	pdivoom_save_sd_handle->upload_time = time(0) + LOG_FILE_WRITE_TIME;
    #ifdef DIVOOM_TEST_CUSTOM_DEVICE
    divoom_save_restore_flash_write_config();
    #else
	divoom_system_create_task_prio(divoom_save_to_sd_task, "SaveSd", NULL, 2*1024, 1); 
    #endif
}

void divoom_save_to_sd_exit(void)
{
	if (pdivoom_save_sd_handle == NULL)
		return;

	divoom_system_get_lock(pdivoom_save_sd_handle->log_lock);
	if (pdivoom_save_sd_handle->log_fp)
	{
		divoom_port_file_close(pdivoom_save_sd_handle->log_fp);
		pdivoom_save_sd_handle->log_fp = NULL;
	}
	pdivoom_save_sd_handle->log_on = 0;
	divoom_system_free_lock(pdivoom_save_sd_handle->log_lock);
}
#endif



