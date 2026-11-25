#ifndef __DIVOOM_ONELINE_LED_H
#define __DIVOOM_ONELINE_LED_H

#define DIVOOM_ONELINE_LED_NUM		DIVOOM_LED_CONTROL_MAX_NUM
#define ONELINE_SCREEN_WIDTH        DIVOOM_LED_CONTROL_MAX_NUM
#define ONELINE_SCREEN_HEIGHT       1
#define ONELINE_SCREEN_LED_NUM      (ONELINE_SCREEN_WIDTH*ONELINE_SCREEN_HEIGHT)
#define ONELINE_SCREEN_BUF_SIZE     (ONELINE_SCREEN_WIDTH*ONELINE_SCREEN_HEIGHT*3)

//灯光最高亮度等级
#define ONELINE_SCREEN_BRIGHT_MAX_LEV			100       
//默认值
#define ONELINE_SCREEN_BRIGHT_DEFAULT_LEV		100

/*****************************************************************************
 Prototype    : divoom_oneline_led_set_bright
 Description  : 设置亮度
 Input        : uint8 lev  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/8/3
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_oneline_led_set_bright(uint8 lev);
/*****************************************************************************
 Prototype    : divoom_oneline_led_set_switch
 Description  : 设置单线数据LED灯开关
 Input        : uint8 flag  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/8/3
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_oneline_led_on_off(uint8 flag);
void divoom_oneline_led_send_rgb_data(uint8 *disp_buf);
void divoom_oneline_led_init(int switch_off);
void divoom_oneline_led_close(void);

#endif


