#ifndef _divoom_light_env_h_
#define _divoom_light_env_h_

//typedef void DIVOOM_LIGHT_ENV_READ(uint8 mode, void *env_data);
typedef int DIVOOM_LIGHT_ENV_READ(uint8 mode, void *env_data);

enum
{
    DIVOOM_DISP_ENV_TIME,   //显示时间
    DIVOOM_DISP_ENV_WEATHER,  //显示天气
    DIVOOM_DISP_ENV_CELS,   //显示摄温度    
    DIVOOM_DISP_ENV_DATE,   //显示日期    
    DIVOOM_DISP_ENV_NEW_PIC,   //显示最新图库   
	DIVOOM_DISP_ENV_TIME_CELS_DATA,			//四横条显示时间,温度,日期
	DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER,	//四方块显示时间,温度,日期,天气
    DIVOOM_DISP_ENV_END_TYPE,
    DIVOOM_DISP_ENV_HUMI,   //显示湿度
    DIVOOM_DISP_ENV_BAT,   //显示湿度
};

#ifdef DIVOOM_HONGKONG_DEMO
enum
{
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_NORMAL,		//没有颜色，只有时分
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL,	//上下有个彩色条
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE,		//方形的
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_CIRCULAR,		//圆形的
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_MAX_NUMBER,
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR,		//底色为彩色
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GRAPH,		//图形的
};
#else
enum
{
	//DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_NORMAL,		//没有颜色，只有时分
	/*DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM,		//数字颜色，黑色底色
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL,	//上下有个彩色条
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE,		//方形的
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_CIRCULAR,		//圆形的
	//DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_BAK,		//黑色颜色，颜色底色
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR,		//底色为彩色
	//指针的阴影坐标是当前的指针,向右移一位,向下再移一位
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GRAPH,		//图形的
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LINE,			//四行显示
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLOCK,		//四个方块
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM,	//多彩的数字
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLUE_COLOURFUL,	//蓝色背景多彩数字.黑红指针
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND,	//金色背景盘,两根黑色指针
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_MAX_NUMBER,*/

	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP,		//数码管类型的显示
	DIVOOM_LIGHT_ENV_TIME_DUSP_EFFECT_ITALIC,		//斜的字体,底色蓝色
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM,	//多彩的数字
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL,	//上下有个彩色条
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND,	//金色背景盘,两根黑色指针
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLUE_COLOURFUL,	//蓝色背景多彩数字.黑红指针
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_CIRCULAR,		//圆形的
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GRAPH,		//图形的
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LINE,			//四行显示
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLOCK,		//四个方块
	DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_MAX_NUMBER,
};
#endif

enum{
    DIVOOM_LIGHT_ENV_MON,
    DIVOOM_LIGHT_ENV_TUE,        
    DIVOOM_LIGHT_ENV_WED,   
    DIVOOM_LIGHT_ENV_THU,   
    DIVOOM_LIGHT_ENV_FRI,   
    DIVOOM_LIGHT_ENV_SAT,       
    DIVOOM_LIGHT_ENV_SUN,  
};


#define DIVOOM_LIGHT_ENV_DISP_EFFECT_MAX	(DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_MAX_NUMBER-1)

typedef bool DIVOOM_DISP_ENV_GET_ACTIVE(uint8 x, uint8 y, uint8 user_data);

typedef struct _divoom_light_env_time_info
{
    uint8 hour;
    uint8 min;
}divoom_light_env_time_info, *Pdivoom_light_env_time_info;

typedef struct _divoom_light_env_date_info
{
    uint8 month;
    uint8 day;
    uint8 week;
}divoom_light_env_date_info, *Pdivoom_light_env_date_info;



/*********************************************************************
  Function:         divoom_light_env_update_mode
  Description:     更新环境显示模式
  Input:             mode   显示模式
  Return:           NONE
  Author:           divoom
**********************************************************************/


void divoom_light_env_update_mode(Pdivoom_disp_ctrl_env_info pevn_info);



/*********************************************************************
  Function:         divoom_light_env_play
  Description:     模块运行控制
  Input:             run_flag    1-->play; 0-->pause
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_play(uint8 run_flag);


/*********************************************************************
  Function:         divoom_light_env_close
  Description:     模块停止，释放资源
  Input:             void
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_close(void);

/*********************************************************************
  Function:         divoom_light_env_init
  Description:     启动环境显示模式
  Input:             mode   显示模式
                       pcallback??读环境数据的回调
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_init(Pdivoom_disp_ctrl_env_info penv_info, 
                                  DIVOOM_LIGHT_GET_ENV_INFO pcallback, uint8 flag);


/*********************************************************************
  Function:         divoom_light_env_update_hour_flag
  Description:     更新时间显示模式
  Input:             hour_flag  
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_update_hour_flag(uint8 hour_flag);


/*********************************************************************
  Function:         divoom_light_env_update_temp_flag
  Description:     更新时间温度制式
  Input:             hour_flag  
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_update_temp_flag(uint8 temp_flag);
bool  divoom_light_env_get_digit(uint8 x, uint8 y, uint8 user_data);

bool  divoom_light_env_noise_sep_digit(uint8 x, uint8 y, uint8 user_data);

void divoom_light_env_update_env_info(uint8 weather_type, int8 temp_value, uint32 humi_value);
uint8 divoom_light_env_disp_digit(uint8 x, uint8 y, uint16 value, uint8 disp_type, uint8 *buf);
void divoom_light_env_fill_area(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, uint8 user_data, 
        DIVOOM_DISP_ENV_GET_ACTIVE   *pget_call, uint8 *disp_buf, Pdivoom_light_color_info color);
Pdivoom_light_color_info divoom_light_env_get_back_color(void);
Pdivoom_light_color_info divoom_light_env_get_fill_color(void);
uint8 divoom_light_env_disp_big_digit(uint8 x, uint8 y, uint16 value, uint8 disp_type, uint8 *buf, uint8 effect_type, Pdivoom_light_color_info color);

/*********************************************************************
  Function:         divoom_light_env_gif_format_hot
  Description:     系统将对指定区域进行擦除动作
  Input:             
  Output             NONE
**********************************************************************/    
    

void divoom_light_env_gif_format_hot(uint32 file_index);
void divoom_disp_time_disp_led_time(uint8 *disp_buf,Pdivoom_light_env_time_info time_info, uint8 dot_flag);

#endif

