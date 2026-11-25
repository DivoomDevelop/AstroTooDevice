
#ifndef _divoom_disp_ctrl_in_h_
#define _divoom_disp_ctrl_in_h_
#include "divoom_type_tivoo.h"
#include "divoom_disp_ctrl.h"

#define DIVOOM_DISP_CTRL_HOT_MAX_SIZE  (64*1024)


enum{
    DIVOOM_3X5_DIGIT,
	DIVOOM_3X7_DIGIT,
    DIVOOM_4X7_DIGIT,   		//方方正正的字模
	DIVOOM_5X7_DIGIT,
	DIVOOM_6X10_DIGIT,
	DIVOOM_6X11_DIGIT,
	//用于新加的时钟页面
	DIVOOM_7X9_DIGIT,
	DIVOOM_8X13_DIGIT,
	DIVOOM_6X10_FANG_DIGIT,
	//新的8*14,用于计分板
	DIVOOM_8X14_DIGIT,
	//32X32时间界面和倒计时秒表等
	DIVOOM_6X12_DIGIT,
	//斜体字体
	DIVOOM_4X13_ITALIC,
	//双倍8X14
	DIVOOM_8X14_BIG_DIGIT,
	//后面的貌似没有
    DIVOOM_4X7_DIGIT_SP,		//同样是4X7的字模，圆润一点

	//为了新加的始终界面所需要的数字
	DIVOOM_6X9_FOR_CLOCK,
};

/*********************************************************************
  Function:        divoom_disp_ctrl_set_data
  Description:   设置某个点的RGB
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/


void divoom_disp_ctrl_set_data(uint16 off,  const divoom_screen_color_info *  color, uint8 *disp_buf);
void divoom_disp_ctrl_small_set_data(uint16 off,  const divoom_screen_color_info *  color, uint8 *disp_buf);




/*********************************************************************
  Function:        divoom_disp_ctrl_get_data
  Description:    从显示缓冲区中得到某个点的RGB
  Input:            off    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/


void divoom_disp_ctrl_get_data(uint16 off, Pdivoom_screen_color_info color, uint8 *disp_buf);



/*********************************************************************
  Function:        divoom_disp_ctrl_set_dot
  Description:   设置某个点的RGB
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_set_dot(uint8 x, uint8 y, const divoom_screen_color_info *pcolor, uint8 *disp_buf);


/*********************************************************************
  Function:     divoom_disp_ctrl_set_dot_64
  Description:  把24bit的rgb转成15bit，设置某个点的RGB 
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           wangft
  Data:            2020-10-29
**********************************************************************/

void divoom_disp_ctrl_set_dot_64(uint8 x, uint8 y, const divoom_screen_color_info *pcolor, uint8 *disp_buf);


/*********************************************************************
  Function:        divoom_disp_ctrl_get_dot
  Description:    从显示缓冲区中得到某个点的RGB
  Input:            X    
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_get_dot(uint8 x, uint8 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf);

/*********************************************************************
  Function:     divoom_disp_ctrl_get_dot_64
  Description:    从显示缓冲区中得到某个点的RGB，此时的rgb是15bit长度
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           wangft
  Data:            2020-10-29
**********************************************************************/

void divoom_disp_ctrl_get_dot_64(uint8 x, uint8 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf);

/**
 * @function:    divoom_disp_ctrl_set_data_xy
 * @description: 设置(x,y)点的RGB
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        x              [x坐标]
 * @param        y              [y坐标]
 * @param        pcolor         [颜色缓冲区]
 * @param        disp_buf       [显示缓冲区]
 */
void divoom_disp_ctrl_set_data_xy(int16 x,int16 y,const divoom_screen_color_info *pcolor, uint8 *disp_buf);

/*****************************************************************************
 Prototype    : divoom_disp_ctrl_pic_extension
 Description  : 将边长为base_len的图片pic_data,扩大边长multi倍到disp_buf上
 Input        : uint8 *disp_buf   
                uint8 *pic_data  
                uint16 base_len  
                uint16 multi     
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/1/15
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_ctrl_pic_extension(uint8 *disp_buf,uint8 *pic_data,uint16 base_len,uint16 multi);


void divoom_disp_ctrl_clr_screen(void);

void divoom_disp_ctrl_fill(uint8 *disp_buf, const divoom_screen_color_info * color);
void divoom_disp_ctrl_fill_64(uint8 *disp_buf, const divoom_screen_color_info * color);

/*****************************************************************************
 Prototype    : divoom_disp_font_fill_area
 Description  : 画出字库      
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y                   
                uint8 size_x               字库宽               注意程序限制，size_x小于8             
                uint8 size_y               字库高     
                uint8 *digit_info          字库数组地址         
                uint8 *disp_buf            要填的buf    
                Pdivoom_screen_color_info color    字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/30
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_font_fill_area(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, uint8 *digit_info, 
        uint8 *disp_buf, Pdivoom_screen_color_info color);

/*****************************************************************************
 Prototype    : divoom_disp_digit_fill_area_shadow
 Description  : 填写一个数字,包含有阴影，要求字模最后一行全为0
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y     
                uint8 user_data            数字 0-9     
                uint8 type                 字体大小类型     DIVOOM_3X5_DIGIT   DIVOOM_4X7_DIGIT
                uint8 *disp_buf            要填的buf     
                Pdivoom_screen_color_info color      字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/11/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_digit_fill_area_shadow(uint8 start_x, uint8 start_y, uint8 user_data, 
        uint8 type, uint8 *disp_buf, Pdivoom_screen_color_info color);

/*****************************************************************************
 Prototype    : divoom_disp_digit_fill_area
 Description  : 填写一个数字
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y     
                uint8 user_data            数字 0-9     
                uint8 type                 字体大小类型     DIVOOM_3X5_DIGIT   DIVOOM_4X7_DIGIT
                uint8 *disp_buf            要填的buf     
                Pdivoom_screen_color_info color      字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_digit_fill_area(uint8 start_x, uint8 start_y, uint8 user_data, 
        uint8 type, uint8 *disp_buf, const divoom_screen_color_info * color);

/** 点阵格式：阴码；取模方式：逐行式；取模走向：逆向（低位在前） **/
void divoom_disp_font_fill_any_area(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, const uint8 *digit_info, 
        uint8 *disp_buf, Pdivoom_screen_color_info color);


void divoom_disp_digit_fill_area_with_bit(uint8 start_x, uint8 start_y, uint8 user_data, uint8 *disp_buf, uint16 start_dot, uint16 end_dot);

/*****************************************************************************
 Prototype    : divoom_disp_font_fill_area_shadow
 Description  : 画出含有阴影字库      ，要求字模最后一行全为0
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y                   
                uint8 size_x               字库宽               注意程序限制，size_x小于8             
                uint8 size_y               字库高     
                uint8 *digit_info          字库数组地址         
                uint8 *disp_buf            要填的buf    
                Pdivoom_screen_color_info color    字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/11/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_font_fill_area_shadow(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, uint8 *digit_info, 
        uint8 *disp_buf, Pdivoom_screen_color_info color);

void divoom_disp_watch_digit(uint8 x, uint8 y, uint8 width, uint8 height, uint8 digit,uint8 * screen_buf, const divoom_screen_color_info *color);



#if 0
void divoom_disp_ctrl_clr_screen(void);
void divoom_disp_ctrl_get_data(uint16 off, Pdivoom_screen_color_info color, uint8 *disp_buf);
void divoom_disp_ctrl_set_data(uint16 off,  const divoom_screen_color_info *  color, uint8 *disp_buf);

void divoom_disp_ctrl_fill(uint8 *disp_buf, const divoom_screen_color_info *color);


void divoom_disp_ctrl_set_light(uint8 light);


uint8 divoom_disp_ctrl_get_light(void);



/*********************************************************************
  Function:        divoom_disp_ctrl_set_dot
  Description:   设置某个点的RGB
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_set_dot(uint8 x, uint8 y, const divoom_screen_color_info *pcolor, uint8 *disp_buf);

/*********************************************************************
  Function:        divoom_disp_ctrl_set_dot_32
  Description:   设置某个点的RGB
  Input:            X   
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           wangft
  Data:            2019-04-18
**********************************************************************/

void divoom_disp_ctrl_set_dot_32(uint8 x, uint8 y, const divoom_screen_color_info *pcolor, uint8 *disp_buf);

/*********************************************************************
  Function:        divoom_disp_ctrl_get_dot
  Description:    从显示缓冲区中得到某个点的RGB
  Input:            X    
                       Y    
                       pcolor   颜色缓冲区
                       disp_buf     显示缓冲区
  Return:           void
  Author:           albert
  Data:            2016-02-25
**********************************************************************/

void divoom_disp_ctrl_get_dot(uint8 x, uint8 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf);

/**
 * @function:    divoom_disp_ctrl_set_data_xy
 * @description: 设置(x,y)点的RGB
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        x              [x坐标]
 * @param        y              [y坐标]
 * @param        pcolor         [颜色缓冲区]
 * @param        disp_buf       [显示缓冲区]
 */
void divoom_disp_ctrl_set_data_xy(int16 x,int16 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf);


/*****************************************************************************
 Prototype    : divoom_disp_digit_fill_area_shadow
 Description  : 填写一个数字,包含有阴影，要求字模最后一行全为0
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y     
                uint8 user_data            数字 0-9     
                uint8 type                 字体大小类型     DIVOOM_3X5_DIGIT   DIVOOM_4X7_DIGIT
                uint8 *disp_buf            要填的buf     
                Pdivoom_screen_color_info color      字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/11/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_digit_fill_area_shadow(uint8 start_x, uint8 start_y, uint8 user_data, 
        uint8 type, uint8 *disp_buf, Pdivoom_screen_color_info color);



/*****************************************************************************
 Prototype    : divoom_disp_digit_fill_area
 Description  : 填写一个数字
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y     
                uint8 user_data            数字 0-9     
                uint8 type                 字体大小类型     DIVOOM_3X5_DIGIT   DIVOOM_4X7_DIGIT
                uint8 *disp_buf            要填的buf     
                Pdivoom_screen_color_info color      字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_digit_fill_area(uint8 start_x, uint8 start_y, uint8 user_data, 
        uint8 type, uint8 *disp_buf, const divoom_screen_color_info * color);


/*****************************************************************************
 Prototype    : divoom_disp_font_fill_area
 Description  : 画出字库      
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y                   
                uint8 size_x               字库宽               注意程序限制，size_x小于8             
                uint8 size_y               字库高     
                uint8 *digit_info          字库数组地址         
                uint8 *disp_buf            要填的buf    
                Pdivoom_screen_color_info color    字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/30
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_font_fill_area(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, uint8 *digit_info, 
        uint8 *disp_buf, Pdivoom_screen_color_info color);


/*****************************************************************************
 Prototype    : divoom_disp_font_fill_area_shadow
 Description  : 画出含有阴影字库      ，要求字模最后一行全为0
 Input        : uint8 start_x              起始位置x     
                uint8 start_y              起始位置y                   
                uint8 size_x               字库宽               注意程序限制，size_x小于8             
                uint8 size_y               字库高     
                uint8 *digit_info          字库数组地址         
                uint8 *disp_buf            要填的buf    
                Pdivoom_screen_color_info color    字的颜色
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/11/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_font_fill_area_shadow(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, uint8 *digit_info, 
        uint8 *disp_buf, Pdivoom_screen_color_info color);

//void divoom_disp_ctrl_timer_set_self_light(T_TIMER timer_id, uint32 delay);

#endif

#endif


