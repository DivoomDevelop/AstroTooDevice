#include "divoom.h"
#include "divoom_white_noise_menu.h"

typedef struct _divoom_white_noise_ctrl 
{
	divoom_white_noise_info white_noise_info;
}divoom_white_noise_ctrl, *Pdivoom_white_noise_ctrl;

static Pdivoom_white_noise_ctrl pdivoom_white_noise_handle = NULL;

void divoom_white_noise_set_default(void)
{
	int i = 0;

	pdivoom_white_noise_handle->white_noise_info.on_off = 1;
	pdivoom_white_noise_handle->white_noise_info.time = 5;
	pdivoom_white_noise_handle->white_noise_info.time *= 60;
	pdivoom_white_noise_handle->white_noise_info.power_off = 0;

	for (i = 0; i < 8; i++)
	{
		pdivoom_white_noise_handle->white_noise_info.volume[i] = 50;
	}
}

void divoom_white_noise_save_cfg(void)
{
	FILE *fd = NULL;
	Pdivoom_tomato_time_info data_read;
	divoom_cJSON *root, *tomato_list, *tomato_item;

    divoom_print("save config!\n");
	if((fd = divoom_port_file_open(DIVOOM_WHITE_NOISE_SAVE_PATH, "w+")) == NULL)
	{
		divoom_print("Some Wrong in %d, maybe no this file:%s\n", __LINE__, DIVOOM_WHITE_NOISE_SAVE_PATH);
		return;
	}

	root = divoom_cJSON_CreateObject();
	divoom_cJSON_AddNumberToObject(root, "OnOff", pdivoom_white_noise_handle->white_noise_info.on_off);
	divoom_cJSON_AddNumberToObject(root, "Time", pdivoom_white_noise_handle->white_noise_info.time/60);
	divoom_cJSON_AddNumberToObject(root, "EndStatus", pdivoom_white_noise_handle->white_noise_info.power_off);
    divoom_cJSON_AddItemToObject(root , "Volume", divoom_cJSON_CreateIntArray(pdivoom_white_noise_handle->white_noise_info.volume, 8));

	char *string = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);

	if (string)
	{
		divoom_port_file_write(string, strlen(string), 1, fd);
		divoom_system_free(string);
	}
	divoom_port_file_close(fd);
}

void divoom_white_noise_load_cfg(void)
{
	int i, file_len = 0, ret = 0, voice_num;
	divoom_cJSON *root, *list, *item;
	FILE *fd = NULL;
	uint8 *read_buf = NULL;

	if ((fd = divoom_port_file_open(DIVOOM_WHITE_NOISE_SAVE_PATH, "r")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_WHITE_NOISE_SAVE_PATH);
		divoom_white_noise_set_default();
		return ;
	}

	divoom_port_file_seek(fd, 0, SEEK_END);
	file_len = divoom_port_file_tell(fd);
	divoom_port_file_seek(fd, 0, SEEK_SET);
	read_buf = divoom_system_malloc(file_len + 1);
	ret = divoom_port_file_read(read_buf, file_len, 1, fd);
	root = divoom_cJSON_Parse(read_buf);
	divoom_system_free(read_buf);

	if (root == NULL)
	{
		divoom_port_file_close(fd);
		return ;
	}

	pdivoom_white_noise_handle->white_noise_info.on_off = divoom_cJSON_GetObjectInt(root, "OnOff");
	pdivoom_white_noise_handle->white_noise_info.time = divoom_cJSON_GetObjectInt(root, "Time");
	pdivoom_white_noise_handle->white_noise_info.time *= 60;
	pdivoom_white_noise_handle->white_noise_info.power_off = divoom_cJSON_GetObjectInt(root, "EndStatus");
	list = divoom_cJSON_GetObjectItem(root, "Volume");
	voice_num = divoom_cJSON_GetArraySize(list);

	for (i = 0; i < voice_num; i++)
	{
		item = divoom_cJSON_GetArrayItem(list, i);
		if(item && item->valueint)
		{
			pdivoom_white_noise_handle->white_noise_info.volume[i] = item->valueint;
		}
	}
	divoom_port_file_close(fd);
	divoom_cJSON_Delete(root);
}

void *divoom_white_noise_get_base_info(void)
{
	return &pdivoom_white_noise_handle->white_noise_info;
}

uint8 divoom_white_noise_event(Pdivoom_sys_msg_info msg)
{
	divoom_app_schedule_item_info event_info = {0};
	divoom_debug("event:%d\n", msg->msg_id);
	
	if(msg)
	{
		switch (msg->msg_id)
		{
			case DIVOOM_WHITE_NOISE_SET:
				divoom_white_noise_menu_app_ctrl(1, msg->data);
				break;

			case DIVOOM_WHITE_NOISE_GET:
				divoom_white_noise_menu_app_ctrl(0, msg->data);
				break;
		}	
	}
	return 0;
}

void divoom_white_noise_start(void)
{
	//申请声音优先级
	divoom_app_schedule_item_info event_info = {0};
	event_info.model_id = DIVOOM_MODEL_APP_WHITE_NOISE;
	event_info.event_id = DIVOOM_WHITE_NOISE_ACTIVE;
	event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
//	if (divoom_app_schedule_push(&event_info))
	{
		divoom_mix_pcm_ctrl_init();
		divoom_snd_pcm_init();
	}
}

void divoom_white_noise_stop(void)
{
	divoom_snd_pcm_close();
	divoom_mix_pcm_ctrl_close();
}

void divoom_white_noise_init(void)
{
	if (pdivoom_white_noise_handle)
		return;

	pdivoom_white_noise_handle = divoom_system_malloc(sizeof(divoom_white_noise_ctrl));
	divoom_white_noise_load_cfg();

	divoom_sys_model_crl_register(DIVOOM_MODEL_APP_WHITE_NOISE, divoom_white_noise_event);
}

void divoom_white_noise_exit(void)
{
	if (pdivoom_white_noise_handle == NULL)
		return;
	
	divoom_white_noise_save_cfg();
	divoom_system_free(pdivoom_white_noise_handle);
	pdivoom_white_noise_handle = NULL;
}

