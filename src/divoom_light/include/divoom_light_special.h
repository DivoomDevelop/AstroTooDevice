#ifndef _divoom_light_special_h_
#define _divoom_light_special_h_
#define DIVOOM_DISP_SPECIAL_DIVOOM_NUM 8

//enum
//{
//    DIVOOM_DISP_SPECIAL_RAINBOW_MODE,
//    DIVOOM_LIGHT_SPECIAL_BREATHING_LIGHT_MODE,
//    DIVOOM_DISP_SPECIAL_BIAS_MODE,          
//    DIVOOM_DISP_SPECIAL_DIAMONG_MODE,
//    DIVOOM_DISP_SPECIAL_BLOCK_MODE,
//    DIVOOM_LIGHT_SPECIAL_COUNT_DOWN_GRADIENT_MODE,  
//    DIVOOM_LIGHT_SPECIAL_PLASMA_MODE,
//    DIVOOM_LIGHT_SPECIAL_WINDMILL_MODE, 
//    DIVOOM_DISP_SPECIAL_DIVOOM_START_MODE, //8 
//    DIVOOM_DISP_SPECIAL_DIVOOM_END_MODE = (DIVOOM_DISP_SPECIAL_DIVOOM_START_MODE + DIVOOM_DISP_SPECIAL_DIVOOM_NUM - 1),    
//    DIVOOM_DISP_SPECIAL_END_MODE,
//    DIVOOM_LIGHT_SPECIAL_EXPLODE_MODE,    
//    DIVOOM_DISP_SPECIAL_RANDOM_MODE,    
//    DIVOOM_DISP_SPECIAL_COUNT_DOWN_MODE,  
//    DIVOOM_LIGHT_SPECIAL_NEON_MOVE_MODE,    
//};

enum
{
    DIVOOM_LIGHT_SPECIAL_BREATHING_LIGHT_MODE,        //0
    DIVOOM_LIGHT_SPECIAL_FLASH_BLACK_MODE,            //1
    DIVOOM_DISP_SPECIAL_BLOCK_MODE,                   //2
    DIVOOM_LIGHT_SPECIAL_FLASH_CIRCULAR_MODE,         //3
    DIVOOM_LIGHT_SPECIAL_WINDMILL_MODE,               //4
    DIVOOM_LIGHT_SPECIAL_FLASH_RAINBOW_MODE,          //5
    DIVOOM_LIGHT_SPECIAL_PLASMA_MODE,                 //6
    DIVOOM_LIGHT_SPECIAL_FLASH_BLOCK_MODE,            //7
    DIVOOM_LIGHT_SPECIAL_FLASH_ROCK_MODE,             //8
    DIVOOM_LIGHT_SPECIAL_FLASH_FIRE_MODE,             //9
    DIVOOM_LIGHT_SPECIAL_COUNT_DOWN_GRADIENT_MODE,    //10  
    DIVOOM_LIGHT_SPECIAL_FLASH_LINE_MODE,             //11
    DIVOOM_DISP_SPECIAL_DIAMONG_MODE,                 //12
    DIVOOM_LIGHT_SPECIAL_FLASH_WATER_MODE,            //13
    DIVOOM_DISP_SPECIAL_BIAS_MODE,                    //14
    DIVOOM_LIGHT_SPECIAL_NEON_MOVE_MODE,
    DIVOOM_DISP_SPECIAL_END_MODE,
    DIVOOM_LIGHT_SPECIAL_EXPLODE_MODE,
    DIVOOM_DISP_SPECIAL_RANDOM_MODE,
    DIVOOM_DISP_SPECIAL_COUNT_DOWN_MODE,
	DIVOOM_DISP_SPECIAL_RAINBOW_MODE,
};



/*********************************************************************
  Function:         divoom_light_special_update_mode
  Description:     ??Ч??
  Input:             mode  ??
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_special_update_mode(uint8 mode);




/*********************************************************************
  Function:         divoom_light_special_close
  Description:     ??Ч??
  Input:            NONE
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_special_close(void);


/*********************************************************************
  Function:         divoom_light_special_init
  Description:     ??Ч??
  Input:             mode  ??
  Return:           NONE
  Author:           divoom
**********************************************************************/
    
void divoom_light_special_init(Pdivoom_disp_ctrl_special_info data);




void divoom_light_special_main(void *data);



uint8 divoom_light_special_set_mode(Pdivoom_disp_ctrl_special_info data);


/**
 * @function:    divoom_light_special_init_mode
 * @description: 灯效中小模块的初始化
 * @Author:      coronaa
 * @date:        2017-07-07
 * @return                      [刷新时间]
 */
uint32 divoom_light_special_init_mode(void);
/**
 * @function:    divoom_light_special_close_mode
 * @description: 灯效中小模块的关闭
 * @Author:      coronaa
 * @date:        2017-06-29
 */
void divoom_light_special_close_mode(void);

/**
 * @function:    divoom_light_special_task_id
 * @description: 传递task_id
 * @Author:      coronaa
 * @date:        2017-07-03
 */
void divoom_light_special_task_id(void);




void divoom_light_special_play(uint8 status);

#endif


