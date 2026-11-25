#ifndef _divoom_light_word_h_
#define _divoom_light_word_h_

enum{
    DIVOOM_DISP_WORD_FONT_INFO,     //更新文字信息
    DIVOOM_DISP_WORD_FONT_SPEED,    //更新文字速度
    DIVOOM_DISP_WORD_BACK_PIC,      //更新背景图片
    DIVOOM_DISP_WORD_UPDATE_ALL,      //更新背景图片

};

#define DIVOOM_DISP_WORD_MAX_NUM    64

#define DIVOOM_DISP_WORD_BASE_PIC_LEN 263

typedef void DIVOOM_LIGHT_WORD_SEND_PIC(void *parm,uint8 *disp_buf, uint16 len);


typedef struct _divoom_light_word_disp_info
{
    uint16 font_info[16];
}divoom_light_word_disp_info, *Pdivoom_light_word_disp_info;

typedef struct _divoom_light_word_disp_font_info
{
    uint8 light;
    uint8 len;
    uint16 speed;
    uint16 word_info[DIVOOM_DISP_WORD_MAX_NUM];
    divoom_screen_color_info word_color;
    divoom_screen_color_info back_color;
}divoom_light_word_disp_font_info, *Pdivoom_light_word_disp_font_info;

typedef struct _divoom_light_word_word_info
{
    uint8 flag;
    uint16 len;
    uint16 *word_info;
}divoom_light_word_word_info, *Pdivoom_light_word_word_info;



/*********************************************************************
  Function:         divoom_light_word_init
  Description:     开始文字显示
   Input:            data   显示相关数据
  Author:           divoom
**********************************************************************/

void *divoom_light_word_init(void *data, uint8 mode);


/*********************************************************************
  Function:         divoom_light_word_update
  Description:     更新显示信息
   Input:            data  显示相关数据                
  Author:           divoom
**********************************************************************/

void divoom_light_word_update(Pdivoom_disp_ctrl_movie_ctrl data);


/*********************************************************************
  Function:         divoom_light_word_close
  Description:     关闭显示的文字
   Input:            NONE
  Author:           divoom
**********************************************************************/

uint8 divoom_light_word_close(void);

/*********************************************************************
  Function:         divoom_light_word_ctrl_task
  Description:     控制文字的微任务
   Input:            on_off   开始还是暂停
  Author:           divoom
**********************************************************************/

void divoom_light_word_ctrl_task(uint8 on_off);


/*********************************************************************
  Function:         divoom_light_word_set_cyc_flag
  Description:     更新显示信息
   Input:            data  循环标记
  Author:           divoom
**********************************************************************/

void divoom_light_word_set_cyc_flag(uint8 data, DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback);


uint8 divoom_light_word_get_mode(void);

void divoom_light_word_update_base_color(uint8 *buf, uint16 len, uint8 flag);
void divoom_light_word_update_word(uint16 len, uint16 *word_info, uint8 flag);
void divoom_light_word_update_speed(uint16 speed);
void divoom_light_word_update_callback(void *parm, DIVOOM_LIGHT_WORD_SEND_PIC *pcallback);

/*********************************************************************
  Function:         divoom_light_word_set_mul_flag
  Description:     设置多屏的标记
   Input:            data  0:pause;1:main;2:sub
  Author:           divoom
**********************************************************************/

void divoom_light_word_set_mul_flag(uint8 data);
uint8 divoom_light_word_get_work_status(void);

#endif


