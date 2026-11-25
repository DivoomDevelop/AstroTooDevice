#include "divoom_disp_com.h"



#define DIVOOM_SCREEN_LOW_POWER_MAX_LIGHT 30


/*灯板软件版本格式
使用一个无符号型数据来记录版本号，版本包含多个信息，
以十进制中多个位来表示各自含义
例：
十进制
高位<-------------------------------个位
	MCU版本				软件功能版本区分			主版本号		子版本号
	00					00					000			000

MCU版本：  0，GD32	F303CC	1,AT32F413CBT7
*/



typedef struct _divoom_screen_driver_ctrl
{
    uint8 max_power_flag;   //为０时控制在２Ａ,为１时在３Ａ
    uint8 low_power_flag;
    uint8 low_power_light;
	uint8 sw;	//灯板开关逻辑上移
    uint8 light;
	uint8 direction;		 //DIVOOM_SCREEN_DIRECTION_TYPE 屏幕方向  0:正常      1:顺时针90度    2:顺时针180度   3:顺时针270度  
	uint8 mirror_flag;	//镜像标志
	uint32 ledboard_version;		//灯板版本
}divoom_screen_driver_ctrl, *Pdivoom_screen_driver_ctrl;

Pdivoom_screen_driver_ctrl pdivoom_screen_driver_handle = NULL;


void divoom_screen_get_data(int off, divoom_screen_color_info *pcolor, uint8 *disp_buf)
{
    off = (off << 1) + off;
    pcolor->r_c = disp_buf[off];
    pcolor->g_c = disp_buf[off+1];
    pcolor->b_c = disp_buf[off+2];    
}


void divoom_screen_set_data(int off, divoom_screen_color_info *pcolor, uint8 *disp_buf)
{
    off = (off << 1) + off;
    disp_buf[off] = pcolor->r_c;
    disp_buf[off+1] = pcolor->g_c;
    disp_buf[off+2] = pcolor->b_c;    
}

void divoom_screen_set_rgba_data(int off, divoom_screen_color_info *pcolor, uint8 opa, uint8 *disp_buf)
{
    off = (off << 2);
    disp_buf[off] = pcolor->b_c;
    disp_buf[off+1] = pcolor->g_c;
    disp_buf[off+2] = pcolor->r_c;
	disp_buf[off+3] = opa;    
}

void divoom_screen_color_init_64(uint8 *disp_buf,divoom_screen_color_info *pcolor)
{
}

/**
* NAME         divoom_screen_disp_add_pecent_info
* @BRIEF	   增加百分比信息
* @PARAM	
* @RETURN	NONE
* @RETVAL
*/

void divoom_screen_disp_add_pecent_info(uint8 *disp_buf)
{
}


static void divoom_screen_driver_send_data(uint8 *disp_buf)
{    
}



/*********************************************************************
  Function:         divoom_screen_get_public_buf
  Description:      得到公共资源ＢＵＦ,这个ＢＵＦ的大小为rgb大小SCREEN_BUF_SIZE
  Input:             
  Output             NONE
**********************************************************************/

uint8  *divoom_screen_get_public_buf(void)
{
    return NULL;
}




void divoom_screen_driver_receive_msg(uint8 recv_cmd,uint8 *recv_data, uint16 data_len)
{
}




/*********************************************************************
  Function:     divoom_screen_init
  Description:     对屏的初始化
  Input:           
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_init(void)
{
     pdivoom_screen_driver_handle = divoom_system_malloc(sizeof(divoom_screen_driver_ctrl));
}

/*********************************************************************
  Function:     divoom_screen_set_power_mode
  Description:  设置电源模式
  Input:             mode
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_set_power_mode(uint8 mode)
{
}

/*********************************************************************
  Function:     divoom_screen_update_write_brance
  Description:     修改白平衡
  Input:             r
                     g
                     b
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_update_write_brance(uint8 r, uint8 g, uint8 b)
{
}

extern void divoom_screen_hw_backlight_set(int brightness);

/*********************************************************************
  Function:     divoom_screen_bright
  Description:     修改屏的亮度
  Input:             light   屏的亮度，这个是百分比
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_bright(uint8 light)
{
	if (pdivoom_screen_driver_handle == NULL)
		return;

#if defined(DIVOOM_TIME_FRAME_SPEAKER) && !defined(DIVOOM_PRODUCT_ASTRO_TOO)
#ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
	if(light > 0 && light < DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT)
	{
		light = DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT;
	}
#else
	if(light > 0 && light < 5)
	{
		light = 5;
	}
#endif
#endif
	pdivoom_screen_driver_handle->light = light;
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
	divoom_screen_hw_backlight_set(light);
#else
	divoom_led_control_control_white_birghness(light);
#endif
}


/*********************************************************************
  Function:     divoom_screen_low_power_flag
  Description:     设置低电标记
  Input:             flag 
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_low_power_flag(uint8 flag)
{
	if(NULL == pdivoom_screen_driver_handle)
		return;
    pdivoom_screen_driver_handle->low_power_flag = flag;
    
	pdivoom_screen_driver_handle->low_power_light = 0xFF;
}


/*****************************************************************************
 Prototype    : divoom_screen_set_direction
 Description  : 设置屏幕方向  0:正常      1:顺时针90度    2:顺时针180度
                3:顺时针270度
 Input        : DIVOOM_SCREEN_DIRECTION_TYPE direction  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/9/23
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_screen_set_direction(DIVOOM_SCREEN_DIRECTION_TYPE direction)
{
//改为在上位机旋转
	if(NULL == pdivoom_screen_driver_handle)
		return;
	pdivoom_screen_driver_handle->direction = direction;
/*
    uint8 cmd_buf[6] ={0};    
    cmd_buf[0] = direction;
    divoom_screen_queue_send_data(DIVOOM_SCREEN_CMD_SCREEN_DIRECTION, cmd_buf, 1);
*/
}


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
void divoom_screen_set_mirror_image(uint8 mirror_flag)
{
//在上位机镜像
    divoom_set_mirror_mode(mirror_flag);
	if(NULL == pdivoom_screen_driver_handle)
		return;
	pdivoom_screen_driver_handle->mirror_flag = mirror_flag;
    
/*
    uint8 cmd_buf[6] ={0};    
    cmd_buf[0] = direction;
    divoom_screen_queue_send_data(DIVOOM_SCREEN_CMD_SCREEN_DIRECTION, cmd_buf, 1);
*/
}



/*********************************************************************
  Function:     divoom_screen_switch
  Description:     对屏进行开关动作
  Input:           on_off    1:开；0:关
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_switch(uint8 on_off)
{
	if (pdivoom_screen_driver_handle == NULL)
		return;

	if (on_off)
	{
		divoom_screen_bright(pdivoom_screen_driver_handle->light);
	}
	else
	{
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
		divoom_screen_hw_backlight_set(0);
#else
		exec_command_system("echo 0 > /sys/class/backlight/backlight/brightness");
#endif
	}
}

/*********************************************************************
  Function:     divoom_screen_set_key_time
  Description:     设置双击与单击时间
  Input:             double_time
                        three_time
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_set_key_time(uint16 double_time, uint16 three_time)
{
}



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
void divoom_screen_set_rgb_iout(uint8 r_iout,uint8 g_iout,uint8 b_iout)
{
}

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
void divoom_screen_send_get_version_cmd(void)
{
}


/**
* NAME         divoom_screen_disp_divoom_data
* @BRIEF	将显示编码的数据，是DIVOOM_IMAGE_ENCODE数据
* @PARAM	
* @RETURN	NONE
* @RETVAL
*/
void divoom_screen_disp_divoom_data(uint8 *encode_buf)
{
}


void divoom_screen_disp_large_pic_64(uint8 *src_buf,uint8 src_size, uint8 *dest_buf)
{
    uint16 i, j , off, m, n;
    divoom_screen_color_info color;

	if (src_size == SCREEN_WIDTH_16)
	{
		for (i = 0; i < SCREEN_WIDTH_16; i++)
		{
	        for(j = 0; j < SCREEN_WIDTH_16; j++)
	        {
	            divoom_screen_get_data(i*SCREEN_WIDTH_16 + j,&color,src_buf);
                off = (i << 2) * SCREEN_HEIGHT_64 + (j << 2);
                for(m = 0; m < 4; m ++)
                {
                    for(n = 0; n < 4; n++)
                    {
				        divoom_screen_set_data(off + n, &color,dest_buf);
                    }
                    off += SCREEN_HEIGHT_64;
                }
	            off ++;
	        }
		}
        
	}
	else if (src_size == SCREEN_WIDTH_32)
	{
		for (i = 0; i < SCREEN_WIDTH_32; i++)
		{
	        for(j = 0; j < SCREEN_WIDTH_32; j++)
	        {
	            divoom_screen_get_data(i*SCREEN_WIDTH_32 + j,&color,src_buf);
                off = (i << 1) * SCREEN_HEIGHT_64 + (j << 1);
				divoom_screen_set_data(off + 0, &color,dest_buf);
				divoom_screen_set_data(off + 1, &color,dest_buf);
                off += SCREEN_HEIGHT_64;
				divoom_screen_set_data(off + 0, &color,dest_buf);
				divoom_screen_set_data(off + 1, &color,dest_buf);
	        }
		}        
	}
}

void divoom_screen_disp_set_grid_line_opa(uint8 *data_buf, int src_w, int dest_w, int opa)
{
	int i, j, off, line_sep = dest_w/src_w;
    divoom_screen_color_info color = {0x1e, 0x1e, 0x1e};

	if (src_w >= SCREEN_WIDTH_64 || dest_w <= 256)
		return;

	if(divoom_main_get_show_grid_flag() == 0)
		return;
	
	for(i = line_sep; i < dest_w; i+=line_sep)
	{
		off = i * dest_w;
		for(j = 0; j < dest_w; j++)
		{
			divoom_screen_set_rgba_data(off, &color, opa, data_buf);
			off ++;
		}
	}

	for(i = line_sep; i < dest_w; i+=line_sep)
	{
		off = i;
		for(j = 0; j < dest_w; j++)
		{
			divoom_screen_set_rgba_data(off, &color, opa, data_buf);
			off += dest_w;
		}
	}
}

void divoom_screen_disp_add_grid_line(uint8 *image_buf, int src_size, int dest_size)
{
    //这位置添加网格线
    int i, j, off, line_sep = dest_size/src_size;
    divoom_screen_color_info color = {0x1e, 0x1e, 0x1e};

	if (src_size >= SCREEN_WIDTH_128 )
		return;

    for(i = line_sep; i < dest_size; i+=line_sep)
    {
        off = i * dest_size;
        for(j = 0; j < dest_size; j++)
        {
			divoom_screen_set_data(off, &color,image_buf);
            off ++;
        }
    }

    for(i = line_sep; i < dest_size; i+=line_sep)
    {
        off = i;
        for(j = 0; j < dest_size; j++)
        {
			divoom_screen_set_data(off, &color,image_buf);
            off += dest_size;
        }
    }
}


//扩展bgra图像
void divoom_screen_disp_large_bgra_pic(uint8 *src_buf,int src_size, int dest_size, uint8 *dest_buf)
{
    uint32 i, j , off, m, n, k, value;
    uint32 *src_image_ptr = (uint32 *)src_buf, *dest_image_ptr = (uint32 *)dest_buf;

	k = dest_size/src_size;
	for (i = 0; i < src_size; i++)
	{
        for(j = 0; j < src_size; j++)
        {
            off = i*src_size + j;
            value = src_image_ptr[off];
            off = (i*k) * dest_size + (j*k);
            for(m = 0; m < k; m++)
            {
                for(n = 0; n < k; n++)
                {
                    dest_image_ptr[off + n] = value;
                }
                off += dest_size;
            }
        }
	}
}

//
//扩展图像，要将RGB转化为BGR
void divoom_screen_disp_large_rgb_bgra_pic(uint8 *src_buf,int src_size, int dest_size, uint8 *dest_buf)
{
    int r,g,b;
    uint32 i, j , off, m, n, k;

	k = dest_size/src_size;
	for (i = 0; i < src_size; i++)
	{
        for(j = 0; j < src_size; j++)
        {
            off = i*src_size + j;
            r = src_buf[off * 3 + 2];
            g = src_buf[off * 3 + 1];
            b = src_buf[off * 3 + 0];
            off = (i*k) * dest_size + (j*k);
            for(m = 0; m < k; m++)
            {
                for(n = 0; n < k; n++)
                {
                    dest_buf[(off + n) * 4] = r;
                    dest_buf[(off + n) * 4 + 1] = g;
                    dest_buf[(off + n) * 4 + 2] = b;
                    dest_buf[(off + n) * 4 + 3] = 0xff;
                }
                off += dest_size;
            }
        }
	}
}

//要将RGB转化为BGR
void divoom_screen_disp_large_pic(uint8 *src_buf,int src_size, int dest_size, uint8 *dest_buf)
{
    uint32 i, j , off, m, n, k;
    divoom_screen_color_info color;

	k = dest_size/src_size;
	for (i = 0; i < src_size; i++)
	{
        for(j = 0; j < src_size; j++)
        {
            divoom_screen_get_data(i*src_size + j,&color,src_buf);
            off = (i*k) * dest_size + (j*k);
            for(m = 0; m < k; m++)
            {
                for(n = 0; n < k; n++)
                {
			        divoom_screen_set_data(off + n, &color,dest_buf);
                }
                off += dest_size;
            }
        }
	}
}

/**
* NAME         divoom_screen_disp_rgb16_data
* @BRIEF	将显示无编码的数据，是RGB数据
* @PARAM	
* @RETURN	NONE
* @RETVAL
*/


void divoom_screen_disp_rgb16_data(uint8 *disp_buf)
{
}

/**
* NAME         divoom_screen_disp_rgb32_data
* @BRIEF	将显示无编码的数据，是RGB数据
* @PARAM	
* @RETURN	NONE
* @RETVAL
*/


void divoom_screen_disp_rgb32_data(uint8 *disp_buf)
{
}

/**
* NAME         divoom_screen_disp_rgb_data
* @BRIEF	将显示无编码的数据，是RGB数据
* @PARAM	
* @RETURN	NONE
* @RETVAL
*/

void divoom_screen_disp_rgb_data(uint8 *disp_buf)
{    
}

/*****************************************************************************
 Prototype    : divoom_screen_disp_send_rgb_data
 Description  : 调用此接口传入screen_width,由底层进行扩大转换
                
 Input        : uint8 *disp_buf  
                uint8 width     
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/12/23
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_screen_disp_send_rgb_data(uint8 *disp_buf,uint8 width)
{

}

/*********************************************************************
  Function:     divoom_screen_update_percent
  Description:  更新下载进度
  Input:           percent    
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_update_percent(uint8 percent)
{
}


void divoom_screen_fill_color(divoom_screen_color_info *pcolor)
{
}


void divoom_screen_back_screen(void)
{
}


/*********************************************************************
  Function:     divoom_screen_close
  Description:     关机前先关闭屏的电源
  Input:           offset    屏中的偏移0
                     color      色彩
  Author:           AIJUN
  Data:            2016-1-29
**********************************************************************/

void divoom_screen_close(void)
{
	if(pdivoom_screen_driver_handle != NULL)
	{
		//	  divoom_screen_back_screen();
		divoom_screen_bright(0);
		divoom_com_free(pdivoom_screen_driver_handle, __func__, __LINE__);
		pdivoom_screen_driver_handle = NULL;
	}
}


