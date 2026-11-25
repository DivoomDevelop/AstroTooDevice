#ifndef _divoom_decode_pic_h_
#define _divoom_decode_pic_h_

/**
* NAME         divoom_decode_pic_init
* @BRIEF	初始化解码句柄，整个系统使用一个共用的解码句柄
* @PARAM	
* @RETURN	 NONE
* @RETVAL
*/

void divoom_decode_pic_init(void);



/**
* NAME         divoom_decode_pic_get_rgb
* @BRIEF	对数据进行解码，生成ＲＧＢ数据,此函数只输出３２*３２的图像
* @PARAM	decode_buf          创建的句柄；
                    rgb_buf               输出的ＲＧＢ缓冲区
                    time                   返回这一帧数据将停留的时间
                    image_width       图像宽度
* @RETURN	 fail:0
* @RETVAL
*/

uint16 divoom_decode_pic_get_rgb(uint8 *decode_buf, uint8 *rgb_buf, uint16 * time, uint16 image_width);


#endif

