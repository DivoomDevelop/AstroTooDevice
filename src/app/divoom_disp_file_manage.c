#include "divoom.h"

#define DIVOOM_ALL_CONFIG_PATH  "divoom_all_config.cfg"

int divoom_disp_file_manage_get_all_config(void *buf, int size)
{
/*
	Pdivoom_light_config_info plight_info = (Pdivoom_light_config_info)buf;
	char file_name[256], *file_buf;
	divoom_cJSON *root;

	sprintf(file_name, "%s/%s",DIVOOM_APP_CONFIG_PATH, DIVOOM_ALL_CONFIG_PATH);
	file_buf = divoom_app_com_read_file(file_name, NULL);
	if (file_buf == NULL)
	{
		divoom_print("[%s:%d]open file failed\n", __FILE__, __LINE__);
		return 0;
	}
	root = divoom_cJSON_Parse(file_buf);
	divoom_free(file_buf);
	if(root == NULL)
	{		
		return 0;
	}
	plight_info->low_energy_ctrl = divoom_cJSON_GetObjectInt(root, "low_energy_ctrl");
	plight_info->time_manage_flag = divoom_cJSON_GetObjectInt(root, "time_manage_flag");
//	plight_info->leave_msg = divoom_cJSON_GetObjectInt(root, "leave_msg");
	plight_info->voice_ctrl = divoom_cJSON_GetObjectInt(root, "voice_ctrl");
	plight_info->sys_light = divoom_cJSON_GetObjectInt(root, "sys_light");
	//plight_info->time_light = divoom_cJSON_GetObjectInt(root, "time_light");
	//plight_info->light_light = divoom_cJSON_GetObjectInt(root, "light_light");
	plight_info->light_mode = divoom_cJSON_GetObjectInt(root, "light_mode");
	plight_info->special_mode = divoom_cJSON_GetObjectInt(root, "special_mode");
	plight_info->color_r = divoom_cJSON_GetObjectInt(root, "color_r");
	plight_info->color_g = divoom_cJSON_GetObjectInt(root, "color_g");
	plight_info->color_b = divoom_cJSON_GetObjectInt(root, "color_b");
	plight_info->time_color_r = divoom_cJSON_GetObjectInt(root, "time_color_r");
	plight_info->time_color_g = divoom_cJSON_GetObjectInt(root, "time_color_g");
	plight_info->time_color_b = divoom_cJSON_GetObjectInt(root, "time_color_b");
	plight_info->time_just_num_color_r = divoom_cJSON_GetObjectInt(root, "time_just_num_color_r");
	plight_info->time_just_num_color_g = divoom_cJSON_GetObjectInt(root, "time_just_num_color_g");
	plight_info->time_just_num_color_b = divoom_cJSON_GetObjectInt(root, "time_just_num_color_b");
	plight_info->muise_type = divoom_cJSON_GetObjectInt(root, "muise_type");
	plight_info->temp_type = divoom_cJSON_GetObjectInt(root, "temp_type");
	plight_info->time_type = divoom_cJSON_GetObjectInt(root, "time_type");
	plight_info->env_effect_mode = divoom_cJSON_GetObjectInt(root, "env_effect_mode");
	plight_info->env_type = divoom_cJSON_GetObjectInt(root, "env_type");
	//plight_info->leave_msg_size = divoom_cJSON_GetObjectInt(root, "leave_msg_size");
	plight_info->net_temp_disp_type = divoom_cJSON_GetObjectInt(root, "net_temp_disp_type");
	plight_info->net_temp_disp_time = divoom_cJSON_GetObjectInt(root, "net_temp_disp_time");
	
//	hot_item = divoom_cJSON_GetObjectItem(root, "hot_info");
	//plight_info->hot_info.tol_item = divoom_cJSON_GetObjectInt(hot_item, "tol_item");
	divoom_cJSON_GetObjectString(root, "rsv_info", (char *)plight_info->rsv_info);
	
	divoom_cJSON_Delete(root);
*/
	return 1;
}

int divoom_disp_file_manage_set_all_config(void *buf, int size)
{	
/*
	Pdivoom_light_config_info plight_info = (Pdivoom_light_config_info)buf;
	char file_name[128], *file_buf;
	divoom_cJSON *root, *hot_info;

	sprintf(file_name, "%s/%s",DIVOOM_APP_CONFIG_PATH, DIVOOM_ALL_CONFIG_PATH);

	root = divoom_cJSON_CreateObject();
	if(root == NULL)
	{
		return 0;
	}

	divoom_cJSON_AddNumberToObject(root, "low_energy_ctrl", plight_info->low_energy_ctrl);	
	divoom_cJSON_AddNumberToObject(root, "time_manage_flag", plight_info->time_manage_flag);
	//divoom_cJSON_AddNumberToObject(root, "leave_msg", plight_info->leave_msg);
	divoom_cJSON_AddNumberToObject(root, "voice_ctrl", plight_info->voice_ctrl);
	divoom_cJSON_AddNumberToObject(root, "sys_light", plight_info->sys_light);
	//divoom_cJSON_AddNumberToObject(root, "time_light", plight_info->time_light);
	//divoom_cJSON_AddNumberToObject(root, "light_light", plight_info->light_light);
	divoom_cJSON_AddNumberToObject(root, "light_mode", plight_info->light_mode);
	divoom_cJSON_AddNumberToObject(root, "special_mode", plight_info->special_mode);
	divoom_cJSON_AddNumberToObject(root, "color_r", plight_info->color_r);
	divoom_cJSON_AddNumberToObject(root, "color_g", plight_info->color_g);
	divoom_cJSON_AddNumberToObject(root, "color_b", plight_info->color_b);
	divoom_cJSON_AddNumberToObject(root, "time_color_r", plight_info->time_color_r);
	divoom_cJSON_AddNumberToObject(root, "time_color_g", plight_info->time_color_g);
	divoom_cJSON_AddNumberToObject(root, "time_color_b", plight_info->time_color_b);
	divoom_cJSON_AddNumberToObject(root, "time_just_num_color_r", plight_info->time_just_num_color_r);
	divoom_cJSON_AddNumberToObject(root, "time_just_num_color_g", plight_info->time_just_num_color_g);
	divoom_cJSON_AddNumberToObject(root, "time_just_num_color_b", plight_info->time_just_num_color_b);
	divoom_cJSON_AddNumberToObject(root, "muise_type", plight_info->muise_type);
	divoom_cJSON_AddNumberToObject(root, "temp_type", plight_info->temp_type);
	divoom_cJSON_AddNumberToObject(root, "time_type", plight_info->time_type);
	divoom_cJSON_AddNumberToObject(root, "env_effect_mode", plight_info->env_effect_mode);
	divoom_cJSON_AddNumberToObject(root, "env_type", plight_info->env_type);
	//divoom_cJSON_AddNumberToObject(root, "leave_msg_size", plight_info->leave_msg_size);
	divoom_cJSON_AddNumberToObject(root, "net_temp_disp_type", plight_info->net_temp_disp_type);
	divoom_cJSON_AddNumberToObject(root, "net_temp_disp_time", plight_info->net_temp_disp_time);
	divoom_cJSON_AddItemToObject(root, "hot_info", hot_info = divoom_cJSON_CreateObject());
	//divoom_cJSON_AddNumberToObject(hot_info, "tol_item", plight_info->hot_info.tol_item);
	divoom_cJSON_AddStringToObject(root, "rsv_info", (char *)plight_info->rsv_info); 
	file_buf = divoom_cJSON_Print(root);    
	divoom_cJSON_Delete(root);	

	divoom_app_com_write_file(file_name, (uint8 *)file_buf, strlen(file_buf));	
	divoom_free(file_buf);
*/
	return 1;
}

