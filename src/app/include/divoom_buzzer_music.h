#ifndef __divoom_buzzer_music_h
#define __divoom_buzzer_music_h


enum{
    DVIOOM_BUZZER_GOURD_BOY = 0,    //葫芦娃
    DIVOOM_BUZZER_SKY_CITY,     //天空之城
    DIVOOM_BUZZER_CLOCK_DIDIDI,   //闹钟dididi
	DIVOOM_BUZZER_ONLY_DI,
    DIVOOM_BUZZER_END,   //
};

#define DIVOOM_BUZZER_EN_GPIO -1

//#define BUZZER_PWM_ID      uiPWM3
//#define BUZZER_PWM_ID      uiPWM4
#define BUZZER_PWM_ID      uiPWM1

//#define BUZZER_PWM_FREQ    32768
#define BUZZER_PWM_DUTY_CYCLE    50		//duty_cycle duty_cycle of pwm [0--100]占空比


/*****************************************************************************
 Prototype    : divoom_buzzer_select_song
 Description  : 选择歌曲
 Input        : uint8 song  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_select_song(uint8 song);
/*****************************************************************************
 Prototype    : divoom_buzzer_init
 Description  : 蜂鸣器模块初始化
 Input        : uint8 *data  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_init(uint8 *data);
/*****************************************************************************
 Prototype    : divoom_buzzer_close
 Description  : 关闭蜂鸣器模块
 Input        : void  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_close(void);
/*****************************************************************************
 Prototype    : divoom_buzzer_en_io_set
 Description  : 设置蜂鸣器使能脚
 Input        : uint8 level  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/21
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_en_io_set(uint8 level);
#endif

