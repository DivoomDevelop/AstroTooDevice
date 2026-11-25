#ifndef _divoom_gif_font_h_
#define _divoom_gif_font_h_

/**
 * NAME         divoom_gif_font_init
 * @BRIEF	    申请相关资源
                
 * @PARAM	    none
                
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_gif_font_init(void);

/**
 * NAME         divoom_gif_font_create_font_cache
 * @BRIEF	    加载这个字符的所有数据数据
                
 * @PARAM	    font_id, 字体ID
                flag,     字体
                
 * @RETURN	    none
 * @RETVAL	
 */

void *divoom_gif_font_create_font_cache(int font_id, uint8 flag);

/**
 * NAME         divoom_gif_font_read_font_image
 * @BRIEF	    读取数据的某一帧
                
 * @PARAM	    font_handle, 字体句柄
                rgb_buf,     RGB缓冲区
                pic_width    宽度
                speed        速度
                
 * @RETURN	    1:结束; 0:还有其它的
 * @RETVAL	
 */

uint8 divoom_gif_font_read_font_image(void *font_handle, uint8 *rgb_buf, uint8 *pic_width,uint16 * speed);

/**
 * NAME         divoom_gif_font_destroy_font_cache
 * @BRIEF	    释放某个字符的句柄
                
 * @PARAM	    font_handle, 字体句柄
                
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_gif_font_destroy_font_cache(void * font_handle);

/**
 * NAME         divoom_gif_font_exit
 * @BRIEF	    释放模块资源
                
 * @PARAM	    none
                
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_gif_font_exit(void);

#endif



