#ifndef _divoom_weather_control_h_
#define _divoom_weather_control_h_

/**
 * NAME         divoom_weather_control_init
 * @BRIEF	    天气初始化
 * @PARAM	
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_init(void);


/**
 * NAME         divoom_weather_control_get
 * @BRIEF	    获取天气信息，通过weather_info的城市名来获取
 * @PARAM	    weather_info
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_get(Pdivoom_disp_weather_info weather_info);

/**
 * NAME         divoom_weather_control_add
 * @BRIEF	    增加一个城市的天气获取
 * @PARAM	    city_name
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_add(char *city_name);

/**
 * NAME         divoom_weather_control_remove
 * @BRIEF	    删除一个城市的天气获取
 * @PARAM	    city_name
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_remove(char *city_name);

/**
 * NAME         divoom_weather_control_update_current
 * @BRIEF	    更新实时天气
 * @PARAM	    pweather_info
                num
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_update_current(Pdivoom_disp_weather_info pweather_info, int num);


/**
 * NAME         divoom_weather_control_update_future
 * @BRIEF	    更新未来天气
 * @PARAM	    pweather_info
                num
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_update_future(Pdivoom_disp_weather_info pweather_info, int num);


/**
 * NAME         divoom_weather_control_main_loop
 * @BRIEF	    控制天气请求
 * @PARAM	                        
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_main_loop(void);

/**
 * NAME         divoom_weather_control_switch_ctrl
 * @BRIEF	    天气请求开关
 * @PARAM	                        
 * @RETURN	none
 * @RETVAL	
 */

void divoom_weather_control_switch_ctrl(uint8 flag);

#endif


