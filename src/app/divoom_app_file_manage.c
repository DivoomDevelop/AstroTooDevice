#include "divoom.h"

#if 0
typedef struct _divoom_file_list
{
	int file_nums;
	void *file_list_queue;
}divoom_file_list, *Pdivoom_file_list;

typedef struct _divoom_app_file_manage_info
{
	divoom_file_list music_file_list;
	void *event_lock;
}divoom_app_file_manage_info, *Pdivoom_app_file_manage_info;

static Pdivoom_app_file_manage_info pdivoom_app_file_mange_handle = NULL;

void divoom_app_file_manage_free(void *data)
{
	if (data)
		divoom_free(data);
}

int divoom_app_file_manage_search(void)
{	
	char *cbuf = "*.mp3;*.aac;*.wav;*.flac;*.ogg;*.ape;*.wma";
	char str[20] = {0};	
	
	if (pdivoom_app_file_mange_handle->music_file_list.file_list_queue)
	{
		divoom_queue_destroy(pdivoom_app_file_mange_handle->music_file_list.file_list_queue, divoom_app_file_manage_free); //不为空销毁
		pdivoom_app_file_mange_handle->music_file_list.file_list_queue = NULL;
	}
	
	pdivoom_app_file_mange_handle->music_file_list.file_list_queue = divoom_file_manage_get_list(DIVOOM_MUSIC_DIR, cbuf, NULL);		
	pdivoom_app_file_mange_handle->music_file_list.file_nums = divoom_queue_items(pdivoom_app_file_mange_handle->music_file_list.file_list_queue);
	sprintf(str, "%d", pdivoom_app_file_mange_handle->music_file_list.file_nums);	
	divoom_file_manage_set_file_var(DIVOOM_FILE_KEY_INFO, DIVOOM_MUSIC_NUMS, str);
	divoom_file_manage_list_save(DIVOOM_FILE_MUSIC_LIST, DIVOOM_PATH_FILE_MUSIC_LIST, pdivoom_app_file_mange_handle->music_file_list.file_list_queue);				
	return 0;
}

void divoom_app_file_manage_sd_det_update_list(void)
{
	uint32 checksuma = 0, checksumb = 0;
	char var_buf[20] = {0};
	int i = 0, start_index = 0, end_index = -1;
	divoom_app_file_manage_search();//搜索文件		
	checksuma = divoom_file_manage_checksum(DIVOOM_FILE_MUSIC_LIST);
	//divoom_net_app_send_sd_card_status(true);
	divoom_file_manage_get_file_var(DIVOOM_FILE_KEY_INFO, DIVOOM_MUSIC_CHECKSUM, var_buf);
	checksumb = atoi(var_buf);
	if (checksuma != checksumb && divoom_net_app_net_is_ok())
	{	
		memset(var_buf, 0, 20);
		sprintf(var_buf, "%u", checksuma);
		divoom_file_manage_set_file_var(DIVOOM_FILE_KEY_INFO, DIVOOM_MUSIC_CHECKSUM, var_buf);
		if (pdivoom_app_file_mange_handle->music_file_list.file_nums == 0)
		{
			start_index = 0;
			end_index = 0;
			//divoom_net_app_send_music_list(pdivoom_app_file_mange_handle->music_file_list.file_list_queue, start_index, end_index);
			return;
		}
		for (i = 0; i < (pdivoom_app_file_mange_handle->music_file_list.file_nums)/10; i++)
		{
			start_index = end_index + 1;
			end_index = start_index + 9;
			//divoom_net_app_send_music_list(pdivoom_app_file_mange_handle->music_file_list.file_list_queue, start_index, end_index);
		}
		if ((pdivoom_app_file_mange_handle->music_file_list.file_nums)%10 > 0)
		{
			start_index = end_index + 1;
			end_index = start_index + (pdivoom_app_file_mange_handle->music_file_list.file_nums)%10 - 1;
			//divoom_net_app_send_music_list(pdivoom_app_file_mange_handle->music_file_list.file_list_queue, start_index, end_index);
		}
	}   
}

uint8 divoom_app_file_manage_event(Pdivoom_sys_msg_info msg)
{
	divoom_system_get_lock(pdivoom_app_file_mange_handle->event_lock);
	switch(msg->msg_id)
	{
		case DIVOOM_APP_SD_STATUS_OUT_ID:
			//divoom_net_app_send_sd_card_status(false);
			divoom_queue_destroy(pdivoom_app_file_mange_handle->music_file_list.file_list_queue, divoom_app_file_manage_free);
			pdivoom_app_file_mange_handle->music_file_list.file_list_queue = NULL;
			break;
		case DIVOOM_APP_SD_STATUS_IN_ID:
			divoom_app_file_manage_sd_det_update_list();
			break;
		case DIVOOM_APP_SD_CARD_UPDATE_FILE_LIST:
			divoom_app_file_manage_sd_det_update_list();
			break;
	}	
	divoom_system_free_lock(pdivoom_app_file_mange_handle->event_lock);
	return 0;	
}

void divoom_app_file_manage_detect(void)
{
	divoom_port_stat_t buf;
	if (divoom_port_stat(DIVOOM_APP_FILE_HOT, &buf) == 0)//存在
	{
		char *file_buf = divoom_app_com_read_file(DIVOOM_APP_FILE_HOT, NULL);
		if (file_buf != NULL)
		{
			divoom_free(file_buf);
		}
	}
}


void divoom_app_file_manage_init(void)
{
	if (pdivoom_app_file_mange_handle == NULL)
	{
		pdivoom_app_file_mange_handle = (Pdivoom_app_file_manage_info)divoom_malloc(sizeof(divoom_app_file_manage_info), DIVOOM_MODEL_FILE, __LINE__, __FILE__);
		if (pdivoom_app_file_mange_handle == NULL)
			return;
	}
	pdivoom_app_file_mange_handle->event_lock = divoom_system_create_lock();
	divoom_app_file_manage_detect();
	divoom_sys_model_crl_register(DIVOOM_MODEL_FILE, divoom_app_file_manage_event);
}
#else







uint32 divoom_app_file_manage_get_next_file_index(uint8 type, uint32 file_index)
{
    return T_U32_MAX;
}


uint32 divoom_app_file_manage_seek_off(void *handle, uint32 pos)
{
    return 1;
}

uint32 divoom_app_file_manage_get_file_size(void *handle)
{
    return 1;
}

void *divoom_app_file_manage_open_read(uint8 type, uint32 file_index, int8 dir)
{
    return NULL;
}
uint32 divoom_app_file_manage_read_data(void *handle, uint32 read_len, uint8 *buf)
{
    return 1;
}
uint8 divoom_app_file_manage_check_file_exist(uint8 type, uint32 file_index)
{
    return 1;
}




void divoom_app_file_manage_init(void)
{

}
#endif

