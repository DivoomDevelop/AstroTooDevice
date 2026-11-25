#include "divoom.h"

uint32 divoom_twitch_get_play_time(char *time_str)
{
	uint32 current_time = 0;
	uint16 MonthToDays = 0;
	uint16 std_month_days[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	uint16 leap_month_days[13] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	uint16 TempYear;
	int year,month, day, hour, min, sec;
	uint16 i;

	if (NULL == time_str)
	{
		return 0;
	}
	sscanf(time_str, "%d-%d-%dT%d:%d:%dZ", &year, &month, &day, &hour, &min, &sec);

	for (TempYear = year - 1; TempYear >= 1970; TempYear--)
	{
		/* the case of leap year */
		if (0 == (TempYear % 4) && ((TempYear % 100) != 0 || 0 == (TempYear % 400)))
		{
			current_time += 31622400; // the seconds of a leap year
		}
		else
		{
			/* not a leap year */
			current_time += 31536000;  // the seconds of a common year(not a leap one)
		}
	}

	/* calculate the current year's seconds */

	if ((month < 1) || (month > 12))
	{
		/* get the default value. */
		month = 1;
		month = 1;
	}

	/* the current year is a leap one */
	if (0 == (year % 4) && ((year % 100) != 0 || 0 == (year % 400)))
	{
		for (i = 1; i < month; i++)
		{
			MonthToDays += leap_month_days[i];
		}
	}
	else
	{
		/* the current year is not a leap one */
		for (i = 1; i < month; i++)
		{
			MonthToDays += std_month_days[i];
		}
	}

	if ((day < 1) || (day > 31))
	{
		/* get the default value */
		day = 1;
	}
	MonthToDays += (day - 1);

	/* added the past days of this year(change to seconds) */
	current_time += MonthToDays * 24 * 3600;

	/* added the current day's time(seconds) */
	current_time += hour * 3600 + min * 60 + sec;

	return current_time;
}



char *divoom_twitch_get_data_request(char *url_addr, Pdivoom_http_client_url_head_info phead_info, int head_num, void *handle)
{
    char *ret_string = NULL, *cmd_string = NULL, *request_url, *user_id, *tmp_str, dest_id[32];
	divoom_cJSON *root = NULL, *cmd_obj = NULL, *cmd_list, *cmd_item, *ret_list, *ret_item;
    int i, j, user_list[10], user_num = 0, len, find_id;
    int views, viewers, flollowers,subscribes, points, status, play_time;
    int viewers_list[10], flollowers_list[10],subscribes_list[10], points_list[10], status_list[10], play_time_list[10];

    memset(viewers_list, 0, sizeof(viewers_list));
    memset(flollowers_list, 0, sizeof(flollowers_list));
    memset(subscribes_list, 0, sizeof(subscribes_list));
    memset(points_list, 0, sizeof(points_list));
    memset(status_list, 0, sizeof(status_list));
    memset(play_time_list, 0, sizeof(play_time_list));
    if(url_addr == NULL || phead_info == NULL)
    {
        divoom_print("err\n");
        return NULL;
    }
    
    request_url = divoom_system_malloc(1024 + 128);
    if(request_url == NULL)
    {
        divoom_print("err\n");
        return NULL;
    }
    user_id = divoom_system_malloc(512);
    if(user_id == NULL)
    {
        divoom_system_free(user_id);
        divoom_print("err\n");
        return NULL;
    }
    
    divoom_app_com_get_str_from_config(url_addr, "user_id", user_id);
    if(strlen(user_id) == 0)
    {
        divoom_system_free(request_url);
        divoom_system_free(user_id);
        divoom_print("err\n");
        return NULL;
    }
    tmp_str = user_id;
    do{
        user_list[user_num] = atoi(tmp_str);
        tmp_str = strchr(tmp_str, ',');
        if(tmp_str)
        {
            tmp_str ++;
        }
        user_num ++;
    }while(tmp_str && user_num < 10);
    divoom_app_com_get_int_from_config(url_addr, "Tviewers", &viewers);
    divoom_app_com_get_int_from_config(url_addr, "Tfollowers", &flollowers);
    divoom_app_com_get_int_from_config(url_addr, "Tsubscribes", &subscribes);
    divoom_app_com_get_int_from_config(url_addr, "Tpoints", &points);
    divoom_app_com_get_int_from_config(url_addr, "Tonline", &status);
    divoom_app_com_get_int_from_config(url_addr, "Tplay_time", &play_time);
    if(viewers || flollowers || subscribes || points || status || play_time)
    {
        if(viewers || play_time || status)
        {
            int cur_time = time(0), start_time;
            strcpy(request_url,"https://api.twitch.tv/helix/streams?");
            for(i = 0, len = 0; i < user_num; i++)
            {
                sprintf(&user_id[len], "user_id=%d", user_list[i]);
                len = strlen(user_id);
                if(i != user_num - 1)
                {
                    user_id[len] = '&';
                    user_id[len + 1] = 0;
                    len ++;
                }
            }
            strcat(request_url, user_id);
            cmd_string = divoom_http_client_url_request(request_url, phead_info, head_num, divoom_clock_control_check_exit_flag, handle);
            if(cmd_string == NULL)
            {
                divoom_print("err\n");
                goto err_divoom_twitch_get_data_request_table;
            }
            
            if(cmd_string)
            {
                cmd_obj = divoom_cJSON_Parse(cmd_string);
                if(cmd_obj == NULL)
                {
                    divoom_print("err\n");
                    goto err_divoom_twitch_get_data_request_table;
                }
                cmd_list = divoom_cJSON_GetObjectItem(cmd_obj, "data");
                if(cmd_list == NULL)
                {
                    divoom_print("err\n");
                    goto err_divoom_twitch_get_data_request_table;
                }
                for(i = 0; i < divoom_cJSON_GetArraySize(cmd_list); i++)
                {
                    cmd_item = divoom_cJSON_GetArrayItem(cmd_list, i);
                    if(cmd_item == NULL)
                    {
                        break;
                    }
                    divoom_cJSON_GetObjectString(cmd_item, "user_id", dest_id);
                    find_id = atoi(dest_id);
                    for(j = 0;  j < user_num; j ++)
                    {
                        if(find_id == user_list[j])
                        {
                            break;
                        }
                    }
                    if(j >= user_num)
                    {
                        continue;
                    }
                    divoom_cJSON_GetObjectString(cmd_item, "started_at", dest_id);
                    start_time = divoom_twitch_get_play_time(dest_id);
                    play_time_list[j] = cur_time - start_time;
                    viewers_list[j] = divoom_cJSON_GetObjectInt(cmd_item, "viewer_count");
                    status_list[j] = 1;
                }
                divoom_cJSON_Delete(cmd_obj);
                cmd_obj = NULL;
                divoom_system_free(cmd_string);
                cmd_string = NULL;
            }
            
            divoom_system_delay_ms(100);
        }
        
        if(flollowers)
        {
            for(i = 0; i < user_num; i++)
            {
                sprintf(request_url,"https://api.twitch.tv/helix/channels/followers?broadcaster_id=%d", user_list[i]);
                cmd_string = divoom_http_client_url_request(request_url, phead_info, head_num, divoom_clock_control_check_exit_flag, handle);
                if(cmd_string == NULL)
                {
                    divoom_print("err\n");
                    goto err_divoom_twitch_get_data_request_table;
                }
                if(cmd_string)
                {
                    cmd_obj = divoom_cJSON_Parse(cmd_string);
                    if(cmd_obj == NULL)
                    {
                        divoom_print("err\n");
                        goto err_divoom_twitch_get_data_request_table;
                    }
                    flollowers_list[i] = divoom_cJSON_GetObjectInt(cmd_obj, "total");
                    divoom_cJSON_Delete(cmd_obj);
                    cmd_obj = NULL;
                    divoom_system_free(cmd_string);
                    cmd_string = NULL;
                }
                divoom_system_delay_ms(100);
            }
        }
        if(subscribes || points)
        {
            //只能获取自己的，所有只可能有一个
            sprintf(request_url,"https://api.twitch.tv/helix/subscriptions?broadcaster_id=%d", user_list[0]);
            cmd_string = divoom_http_client_url_request(request_url, phead_info, head_num, divoom_clock_control_check_exit_flag, handle);
            if(cmd_string == NULL)
            {
                divoom_print("err\n");
                goto err_divoom_twitch_get_data_request_table;
            }
            if(cmd_string)
            {
                cmd_obj = divoom_cJSON_Parse(cmd_string);
                if(cmd_obj == NULL)
                {
                    divoom_print("err\n");
                    goto err_divoom_twitch_get_data_request_table;
                }
                points_list[0] = divoom_cJSON_GetObjectInt(cmd_obj, "points");
                subscribes_list[0] = divoom_cJSON_GetObjectInt(cmd_obj, "total");
                divoom_cJSON_Delete(cmd_obj);
                cmd_obj = NULL;
                divoom_system_free(cmd_string);
                cmd_string = NULL;
            }
            divoom_system_delay_ms(100);
        }
        
        strcpy(request_url,"https://api.twitch.tv/helix/users?");
        for(i = 0, len = 0; i < user_num; i++)
        {
            sprintf(&user_id[len], "id=%d", user_list[i]);
            len = strlen(user_id);
            if(i != user_num - 1)
            {
                user_id[len] = '&';
                user_id[len + 1] = 0;
                len ++;
            }
        }
        strcat(request_url, user_id);
        cmd_string = divoom_http_client_url_request(request_url, phead_info, head_num, divoom_clock_control_check_exit_flag, handle);
        if(cmd_string == NULL)
        {
            divoom_print("err\n");
            goto err_divoom_twitch_get_data_request_table;
        }
        if(cmd_string)
        {
            cmd_obj = divoom_cJSON_Parse(cmd_string);
            if(cmd_obj == NULL)
            {
                divoom_print("err\n");
                goto err_divoom_twitch_get_data_request_table;
            }
            cmd_list = divoom_cJSON_GetObjectItem(cmd_obj, "data");
            if(cmd_list == NULL)
            {
                divoom_print("err\n");
                goto err_divoom_twitch_get_data_request_table;
            }
            root = divoom_cJSON_CreateObject();
            divoom_cJSON_AddItemToObject(root, "data", ret_list = divoom_cJSON_CreateArray());
            
            for(i = 0; i < divoom_cJSON_GetArraySize(cmd_list); i++)
            {
                cmd_item = divoom_cJSON_GetArrayItem(cmd_list, i);
                if(cmd_item == NULL)
                {
                    divoom_print("err\n");
                    goto err_divoom_twitch_get_data_request_table;
                }
                divoom_cJSON_AddItemToArray(ret_list, ret_item = divoom_cJSON_CreateObject());
                divoom_cJSON_GetObjectString(cmd_item, "id", dest_id);
                find_id = atoi(dest_id);
                for(j = 0;  j < user_num; j ++)
                {
                    if(find_id == user_list[j])
                    {
                        break;
                    }
                }
                if(j >= user_num)
                {
                    continue;
                }
                
                divoom_cJSON_GetObjectString(cmd_item, "display_name", user_id);
                divoom_cJSON_AddStringToObject(ret_item, "display_name", user_id);
                views = divoom_cJSON_GetObjectInt(cmd_item, "view_count");
                divoom_cJSON_AddNumberToObject(ret_item, "view_count", views);
                divoom_cJSON_AddNumberToObject(ret_item, "total", flollowers_list[j]);
                divoom_cJSON_AddNumberToObject(ret_item, "viewer_count", viewers_list[j]);
                divoom_cJSON_AddNumberToObject(ret_item, "subscribe_count", subscribes_list[j]);
                divoom_cJSON_AddNumberToObject(ret_item, "points", points_list[j]);
                sprintf(dest_id, "%d:%02d", play_time_list[j]/3600,(play_time_list[j]%3600)/60);
                divoom_cJSON_AddStringToObject(ret_item, "play_time", dest_id);
                if(status_list[j])
                {
                    divoom_cJSON_AddStringToObject(ret_item, "type", "LIVE");
                }
                else
                {
                    divoom_cJSON_AddStringToObject(ret_item, "type", "OFF");
                }
            }
            if(play_time && user_num == 1)
            {
                divoom_cJSON_AddItemToArray(ret_list, ret_item = divoom_cJSON_CreateObject());;
                divoom_cJSON_AddStringToObject(ret_item, "display_name", "---");
                divoom_cJSON_AddNumberToObject(ret_item, "view_count", 0);
                divoom_cJSON_AddNumberToObject(ret_item, "total", 0);
                divoom_cJSON_AddNumberToObject(ret_item, "viewer_count", 0);
                divoom_cJSON_AddNumberToObject(ret_item, "subscribe_count", 0);
                divoom_cJSON_AddNumberToObject(ret_item, "points", 0);
                divoom_cJSON_AddStringToObject(ret_item, "play_time", "0:00");
                divoom_cJSON_AddStringToObject(ret_item, "type", "OFF");
            }
            divoom_cJSON_Delete(cmd_obj);
            cmd_obj = NULL;
            divoom_system_free(cmd_string);
            cmd_string = NULL;
        }
        ret_string = divoom_cJSON_Print(root);
    }
    
    err_divoom_twitch_get_data_request_table:
    if(request_url)
    {
        divoom_system_free(request_url);
    }
    if(user_id)
    {
        divoom_system_free(user_id);
    }

    
    if(root)
    {
        divoom_cJSON_Delete(root);
    }
    
    if(cmd_obj)
    {
        divoom_cJSON_Delete(cmd_obj);
    }
    
    if(cmd_string)
    {
        divoom_system_free(cmd_string);
    }
    return ret_string;
}

