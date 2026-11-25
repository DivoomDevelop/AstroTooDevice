#ifndef _divoom_screen_driver_h_
#define _divoom_screen_driver_h_

#define SCREEN_WIDTH_16		16
#define SCREEN_HEIGHT_16    16
#define BYTES_PRE_DOT       3
#define SCREEN_DOT_NUM_16	(SCREEN_WIDTH_16 * SCREEN_HEIGHT_16)
#define SCREEN_BUF_SIZE_16	(SCREEN_WIDTH_16 * SCREEN_HEIGHT_16 * BYTES_PRE_DOT)

#define SCREEN_WIDTH_32		32
#define SCREEN_HEIGHT_32    32
#define SCREEN_DOT_NUM_32	(SCREEN_WIDTH_32 * SCREEN_HEIGHT_32)
#define SCREEN_BUF_SIZE_32	(SCREEN_WIDTH_32 * SCREEN_HEIGHT_32 * BYTES_PRE_DOT)

#define SCREEN_WIDTH_64		64
#define SCREEN_HEIGHT_64    64
#define SCREEN_DOT_NUM_64	(SCREEN_WIDTH_64 * SCREEN_HEIGHT_64)
#define SCREEN_BUF_SIZE_64	(SCREEN_WIDTH_64 * SCREEN_HEIGHT_64 * BYTES_PRE_DOT)

//当前屏幕的大小
#define SCREEN_WIDTH		64
#define SCREEN_HEIGHT		64
#define SCREEN_DOT_NUM		(SCREEN_WIDTH * SCREEN_HEIGHT)
#define SCREEN_BUF_SIZE		(SCREEN_HEIGHT * SCREEN_WIDTH * BYTES_PRE_DOT)


#define SCREEN_WIDTH_128		128
#define SCREEN_HEIGHT_128		128
#define SCREEN_DOT_NUM_128	    (SCREEN_WIDTH_128 * SCREEN_HEIGHT_128)
#define SCREEN_BUF_SIZE_128 	(SCREEN_WIDTH_128 * SCREEN_HEIGHT_128 * BYTES_PRE_DOT)

#define SCREEN_WIDTH_256		256
#define SCREEN_HEIGHT_256		256
#define SCREEN_DOT_NUM_256	    (SCREEN_WIDTH_256 * SCREEN_HEIGHT_256)
#define SCREEN_BUF_SIZE_256 	(SCREEN_WIDTH_256 * SCREEN_HEIGHT_256 * BYTES_PRE_DOT)


#define SCREEN_WIDTH_LCD		128
#define SCREEN_HEIGHT_LCD		128
#define SCREEN_DOT_NUM_LCD	    (SCREEN_WIDTH_LCD * SCREEN_HEIGHT_LCD)
#define SCREEN_BUF_SIZE_LCD 	(SCREEN_WIDTH_LCD * SCREEN_HEIGHT_LCD * BYTES_PRE_DOT)


typedef enum
{
	DIVOOM_SCREEN_CLOCKWISE_0_DEGREES,			//正常
	DIVOOM_SCREEN_CLOCKWISE_90_DEGREES,			//顺时针转90度
	DIVOOM_SCREEN_CLOCKWISE_180_DEGREES,		//顺时针转180度
	DIVOOM_SCREEN_CLOCKWISE_270_DEGREES,		//顺时针转270度
}DIVOOM_SCREEN_DIRECTION_TYPE;

void divoom_screen_disp_set_grid_line_opa(uint8 *data_buf, int src_w, int dest_w, int opa);
void divoom_screen_disp_add_grid_line(uint8 *image_buf, int src_size, int dest_size);
void divoom_screen_disp_large_pic(uint8 *src_buf,int src_size, int dest_size, uint8 *dest_buf);
void divoom_screen_disp_large_pic_64(uint8 *src_buf,uint8 src_size, uint8 *dest_buf);
void divoom_screen_set_data(int off, divoom_screen_color_info *pcolor, uint8 *disp_buf);
//
//扩展图像，要将RGB转化为BGR
void divoom_screen_disp_large_rgb_bgra_pic(uint8 *src_buf,int src_size, int dest_size, uint8 *dest_buf);

//扩展bgra图像
void divoom_screen_disp_large_bgra_pic(uint8 *src_buf,int src_size, int dest_size, uint8 *dest_buf);

/*********************************************************************
  Function:     divoom_screen_init
  Description:     对屏的初始化
  Input:           
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_init(void);

/*********************************************************************
  Function:     divoom_screen_bright
  Description:     修改屏的亮度
  Input:             light   屏的亮度，这个是百分比
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_bright(uint8 light);

/*********************************************************************
  Function:     divoom_screen_update_write_brance
  Description:     修改白平衡
  Input:             r
                     g
                     b
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_update_write_brance(uint8 r, uint8 g, uint8 b);

/*********************************************************************
  Function:     divoom_screen_set_power_mode
  Description:  设置电源模式
  Input:             mode
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_set_power_mode(uint8 mode);


/*****************************************************************************
 Prototype    : divoom_screen_set_direction
 Description  : 设置屏幕方向  0:正常      1:顺时针90度    2:顺时针180度
                3:顺时针270度
 Input        : DIVOOM_SCREEN_DIRECTION_TYPE direction  	DIVOOM_SCREEN_DIRECTION_TYPE
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/9/23
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_screen_set_direction(DIVOOM_SCREEN_DIRECTION_TYPE direction);

/*****************************************************************************
 Prototype    : divoom_screen_set_mirror_image
 Description  : 图像镜像
 Input        : uint8 mirror_flag  		0：正常		1：镜像
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2021/10/14
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_screen_set_mirror_image(uint8 mirror_flag);


/*********************************************************************
  Function:     divoom_screen_switch
  Description:     对屏进行开关动作
  Input:           on_off    1:开；0:关
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_switch(uint8 on_off);

/*****************************************************************************
 Prototype    : divoom_screen_set_rgb_iout
 Description  : 设置RGB灯珠各自的电流，用于用户自定义调色
                
 Input        : uint8 r_iout  0~100
                uint8 g_iout  0~100  
                uint8 b_iout  0~100  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2021/1/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_screen_set_rgb_iout(uint8 r_iout,uint8 g_iout,uint8 b_iout);

/*****************************************************************************
 Prototype    : divoom_screen_send_get_version_cmd
 Description  : 发送获取灯板版本号命令
 Input        : void  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2021/2/8
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_screen_send_get_version_cmd(void);


/**
填充颜色
**/
void divoom_screen_fill_color(divoom_screen_color_info *pcolor);

void divoom_screen_back_screen(void);


/*********************************************************************
  Function:     divoom_screen_close
  Description:     关机前先关闭屏的电源
  Input:           offset    屏中的偏移0
                     color      色彩
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_close(void);


/**
* NAME         divoom_screen_disp_rgb_data
* @BRIEF	将显示无编码的数据，是RGB数据
* @PARAM	
* @RETURN	NONE
* @RETVAL
*/


void divoom_screen_disp_rgb_data(uint8 *disp_buf);


void divoom_screen_disp_rgb16_data(uint8 *disp_buf);

/**
* NAME         divoom_screen_disp_rgb32_data
* @BRIEF	将显示无编码的数据，是RGB数据,必须是32*32大小的数据
* @PARAM	
* @RETURN	NONE
* @RETVAL
*/
void divoom_screen_disp_rgb32_data(uint8 *disp_buf);


/*****************************************************************************
 Prototype    : divoom_screen_disp_send_rgb_data
 Description  : 调用此接口传入screen_width,由底层进行扩大转换
                
 Input        : uint8 *disp_buf  
                uint8 width     图像边长
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/12/23
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_screen_disp_send_rgb_data(uint8 *disp_buf,uint8 width);



/**
* NAME         divoom_screen_disp_divoom_data
* @BRIEF	将显示编码的数据，是DIVOOM_IMAGE_ENCODE数据
* @PARAM	
* @RETURN	NONE
* @RETVAL
*/
void divoom_screen_disp_divoom_data(uint8 *encode_buf);

/*********************************************************************
  Function:         divoom_screen_get_public_buf
  Description:      得到公共资源ＢＵＦ,这个ＢＵＦ的大小为rgb大小SCREEN_BUF_SIZE
  Input:             
  Output             NONE
**********************************************************************/
uint8  *divoom_screen_get_public_buf(void);


/*********************************************************************
  Function:     divoom_screen_update_percent
  Description:  更新下载进度
  Input:           percent    
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_update_percent(uint8 percent);


#endif


