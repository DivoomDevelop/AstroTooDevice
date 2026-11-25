#include "divoom.h"
#if 0

typedef struct _divoom_weather_control_ctrl
{
    int active_num;
    void *lock;
	Pdivoom_disp_weather_info pweather_list;
}divoom_weather_control_ctrl, *Pdivoom_weather_control_ctrl;

Pdivoom_weather_control_ctrl pdivoom_weather_control_handle = NULL;

/**
 * NAME         divoom_weather_control_init
 * @BRIEF	    天气初始化
 * @PARAM	
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_init(void)
{
    pdivoom_weather_control_handle = divoom_system_malloc(sizeof(divoom_weather_control_ctrl));
    pdivoom_weather_control_handle->lock = divoom_system_create_lock();
}


/**
 * NAME         divoom_weather_control_get
 * @BRIEF	    获取天气信息，通过weather_info的城市名来获取
 * @PARAM	    weather_info
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_get(Pdivoom_disp_weather_info weather_info)
{
    int i;
    if(pdivoom_weather_control_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_weather_control_handle->lock);
    for(i = 0; i < pdivoom_weather_control_handle->active_num; i++)
    {
        if(strcmp(weather_info->city_name, pdivoom_weather_control_handle->pweather_list[i].city_name) == 0)
        {
            memcpy(weather_info, &pdivoom_weather_control_handle->pweather_list[i], sizeof(divoom_disp_weather_info));
            break;
        }
    }
    divoom_system_free_lock(pdivoom_weather_control_handle->lock);
}

/**
 * NAME         divoom_weather_control_add
 * @BRIEF	    增加一个城市的天气获取
 * @PARAM	    city_name
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_add(char *city_name)
{
    int i;
    Pdivoom_disp_weather_info pweather_list;
    
    if(pdivoom_weather_control_handle == NULL || strlen(city_name) == 0)
    {
        return;
    }
    divoom_print("add city: %s\n", city_name);
    divoom_system_get_lock(pdivoom_weather_control_handle->lock);
    for(i = 0; i < pdivoom_weather_control_handle->active_num; i++)
    {
        if(strcmp(pdivoom_weather_control_handle->pweather_list[i].city_name, city_name) == 0)
        {
            break;
        }
    }
    if(i < pdivoom_weather_control_handle->active_num)
    {
        divoom_system_free_lock(pdivoom_weather_control_handle->lock);
        return;
    }
    pweather_list = divoom_system_malloc(sizeof(divoom_disp_weather_info) * (pdivoom_weather_control_handle->active_num + 1));
    if(pweather_list)
    {
        if(pdivoom_weather_control_handle->active_num)
        {
            memcpy(pweather_list, pdivoom_weather_control_handle->pweather_list, sizeof(divoom_disp_weather_info) * pdivoom_weather_control_handle->active_num);
            divoom_system_free(pdivoom_weather_control_handle->pweather_list);
        }
        pdivoom_weather_control_handle->pweather_list = pweather_list;
        strcpy(pdivoom_weather_control_handle->pweather_list[pdivoom_weather_control_handle->active_num].city_name, city_name);
        pdivoom_weather_control_handle->active_num ++;
    }
    divoom_system_free_lock(pdivoom_weather_control_handle->lock);
}



/**
 * NAME         divoom_weather_control_remove
 * @BRIEF	    删除一个城市的天气获取
 * @PARAM	    city_name
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_remove(char *city_name)
{
    int i, find;
    Pdivoom_disp_weather_info pweather_list;
    
    if(pdivoom_weather_control_handle == NULL || strlen(city_name) == 0)
    {
        return;
    }
    if(city_name == NULL || strlen(city_name) == 0)
    {
        return;
    }
    divoom_print("remove city: %s\n", city_name);
    divoom_system_get_lock(pdivoom_weather_control_handle->lock);
    pweather_list = divoom_system_malloc(sizeof(divoom_disp_weather_info) * (pdivoom_weather_control_handle->active_num - 1));
    if(pdivoom_weather_control_handle->active_num <=1 )
    {
        divoom_system_free(pdivoom_weather_control_handle->pweather_list);
        pdivoom_weather_control_handle->pweather_list = NULL;
        pdivoom_weather_control_handle->active_num = 0;
    }
    else
    {
        if(pweather_list)
        {
            for(i = 0, find = 0; i < pdivoom_weather_control_handle->active_num && find < pdivoom_weather_control_handle->active_num - 1; i ++)
            {
                if(strcmp(pdivoom_weather_control_handle->pweather_list[i].city_name, city_name))
                {
                    memcpy(&pweather_list[find], &pdivoom_weather_control_handle->pweather_list[i], sizeof(divoom_disp_weather_info));
                    find ++;
                }
            }
            divoom_system_free(pdivoom_weather_control_handle->pweather_list);
            pdivoom_weather_control_handle->pweather_list = pweather_list;
            pdivoom_weather_control_handle->active_num --;
        }
    }
    divoom_system_free_lock(pdivoom_weather_control_handle->lock);
}


/**
 * NAME         divoom_weather_control_update_current
 * @BRIEF	    更新实时天气
 * @PARAM	    pweather_info
                num
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_update_current(Pdivoom_disp_weather_info pweather_info, int num)
{
    int i, j;

    divoom_system_get_lock(pdivoom_weather_control_handle->lock);
    for(i = 0; i < num; i++)
    {
        for(j = 0; j < pdivoom_weather_control_handle->active_num; j ++)
        {
            if(strcmp(pweather_info[i].city_name, pdivoom_weather_control_handle->pweather_list[j].city_name) == 0)
            {
                pdivoom_weather_control_handle->pweather_list[j].weather_pic_id = pweather_info[i].weather_pic_id;
                pdivoom_weather_control_handle->pweather_list[j].pressure = pweather_info[i].pressure;      //Atmospheric pressure (on the sea level, if there is no sea_level or grnd_level data), hPa
                pdivoom_weather_control_handle->pweather_list[j].humidity = pweather_info[i].humidity;      //Humidity, %
                pdivoom_weather_control_handle->pweather_list[j].visibility = pweather_info[i].visibility;    //Visibility, meter
                pdivoom_weather_control_handle->pweather_list[j].wind_speed = pweather_info[i].wind_speed;  //Wind speed. Unit Default: meter/sec, Metric: meter/sec, Imperial: miles/hour.
                pdivoom_weather_control_handle->pweather_list[j].temp = pweather_info[i].temp;        // Temperature. Unit Default: Kelvin, Metric: Celsius, Imperial: Fahrenheit.
                pdivoom_weather_control_handle->pweather_list[j].min_temp = pweather_info[i].min_temp;        // Temperature. Unit Default: Kelvin, Metric: Celsius, Imperial: Fahrenheit.
                pdivoom_weather_control_handle->pweather_list[j].max_temp = pweather_info[i].max_temp;        // Temperature. Unit Default: Kelvin, Metric: Celsius, Imperial: Fahrenheit.
                strcpy(pdivoom_weather_control_handle->pweather_list[j].weather_str,pweather_info[i].weather_str);
                pdivoom_weather_control_handle->pweather_list[j].update_real_time = time(0) + DIVOOM_MAIN_WEATHER_UPDATE_TIME;                
                break;
            }
        }
    }    
    divoom_system_free_lock(pdivoom_weather_control_handle->lock);
}


/**
 * NAME         divoom_weather_control_update_future
 * @BRIEF	    更新未来天气
 * @PARAM	    pweather_info
                num
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_update_future(Pdivoom_disp_weather_info pweather_info, int num)
{
    int i, j;

    divoom_system_get_lock(pdivoom_weather_control_handle->lock);
    for(i = 0; i < num; i++)
    {
        for(j = 0; j < pdivoom_weather_control_handle->active_num; j ++)
        {
            if(strcmp(pweather_info[i].city_name, pdivoom_weather_control_handle->pweather_list[j].city_name) == 0)
            {
                memcpy(pdivoom_weather_control_handle->pweather_list[j].futrue_table, pweather_info[i].futrue_table, sizeof(pweather_info[i].futrue_table));
                pdivoom_weather_control_handle->pweather_list[j].update_future_time = time(0) + DIVOOM_MAIN_WEATHER_UPDATE_TIME;
                break;
            }
            
        }
    }    
    divoom_system_free_lock(pdivoom_weather_control_handle->lock);
}


/**
 * NAME         divoom_weather_control_main_loop
 * @BRIEF	    控制天气请求
 * @PARAM	                        
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_main_loop(void)
{
    int i;
    divoom_weather_city_info city_list;
    if(pdivoom_weather_control_handle->active_num == 0)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_weather_control_handle->lock);
    city_list.num = 0;
    for(i = 0; i < pdivoom_weather_control_handle->active_num; i++)
    {
        if(pdivoom_weather_control_handle->pweather_list[i].update_real_time < time(0))
        {
            city_list.city_list[city_list.num] = pdivoom_weather_control_handle->pweather_list[i].city_name;
            pdivoom_weather_control_handle->pweather_list[i].update_real_time = time(0) + 10;
            city_list.num ++;
        }
    }
    if(city_list.num)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_SEND_CITY_CURRENT_WEATHER, DIVOOM_SERVER_TOPIC, &city_list);
    }
    #if 0  //暂时只要获取当前天气
    city_list.num = 0;
    for(i = 0; i < pdivoom_weather_control_handle->active_num; i++)
    {
        if(pdivoom_weather_control_handle->pweather_list[i].update_future_time < time(0))
        {
            city_list.city_list[city_list.num] = pdivoom_weather_control_handle->pweather_list[i].city_name;
            pdivoom_weather_control_handle->pweather_list[i].update_future_time = time(0) + 10;
            city_list.num ++;
        }
    }
    if(city_list.num)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_SEND_CITY_FUTURE_WEATHER, DIVOOM_SERVER_TOPIC, &city_list);
    }
    #endif
    divoom_system_free_lock(pdivoom_weather_control_handle->lock);
}
#endif



