#ifndef _divoom_light_breathing_light_h_
#define _divoom_light_breathing_light_h_

uint32 divoom_light_breathing_light_init(void);
// void divoom_light_breathing_light_play(uint8 play);
void divoom_light_breathing_light_close(void);
void divoom_light_breathing_light_main(void);
void divoom_light_breathing_light_set_refresh_time(void);
uint32 divoom_light_breathing_light_get_refresh_time(void);
void divoom_light_breathing_light_get_taskid(uint32 id);

/*****************************************************************************
 Prototype    : divoom_light_breathing_light_snow
 Description  : 雪花屏
 Input        : uint16 index        第N步  
                uint8 *start_pos    闪烁的起始位
                uint8 *disp_buf     显示BUF
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/11
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_breathing_light_snow(uint16 index,uint8 *start_pos,uint8 *disp_buf);

/*****************************************************************************
 Prototype    : divoom_light_breathing_light_get_random_start
 Description  : 获取256个点的随机数
 Input        : uint8 *start_pos  
                uint8 max         
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/12
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_breathing_light_get_random_start(uint8 *start_pos,uint8 max);
#endif


