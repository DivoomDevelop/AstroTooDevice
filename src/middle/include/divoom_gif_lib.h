#ifndef _divoom_gif_lib_h_
#define _divoom_gif_lib_h_
/**
 * NAME         gif_malloc
 * @BRIEF	    申请内存
                
 * @PARAM	    size : 
 
 * @RETURN	    ptr
 * @RETVAL	
 */

void *gif_malloc(int size);

/**
 * NAME         gif_free
 * @BRIEF	    释放内存
                
 * @PARAM	    ptr
 
 * @RETURN	    
 * @RETVAL	
 */

void gif_free(void *ptr);
/**
 * NAME         divoom_gif_lib_init
 * @BRIEF	    打开一个ＧＩＦ文件句柄
                
 * @PARAM	    file_name : name
                file_handle
                ext_buf_flag   1:使用外部的BUF;0：内部申请BUF
 
 * @RETURN	    gif handle
 * @RETVAL	
 */

void *divoom_gif_lib_init(char *file_name, void * file_handle, uint8 ext_buf_flag);

/**
 * NAME         divoom_gif_lib_get_pic_data
 * @BRIEF	    从句柄中得到多个图像
                
 * @PARAM	    handle : 
                pic_num
                rgb_buf
 
 * @RETURN	    1：重开成功，0：失败
 * @RETVAL	
 */

int divoom_gif_lib_get_pic_data(void *handle, uint8 *rgb_buf, uint8 *width, uint16 *speed ,int pic_num, int bgar_flag);

/**
 * NAME         divoom_gif_lib_reset
 * @BRIEF	    复位缓冲区
                
 * @PARAM	    handle : 
 
 * @RETURN	    none
 * @RETVAL	
 */

int divoom_gif_lib_reset(void *handle, int offset);

/**
 * NAME         divoom_gif_lib_get_pic_width
 * @BRIEF	    获取图像的宽度
                
 * @PARAM	    pdivoom_gif_handle : 
 
 * @RETURN	    图像的宽度
 * @RETVAL	
 */

int divoom_gif_lib_get_pic_width(void *pdivoom_gif_handle);

/**
 * NAME         divoom_gif_lib_get_pic_number
 * @BRIEF	    获取图像的张数
                
 * @PARAM	    pdivoom_gif_handle : 
 
 * @RETURN	    图像的张数
 * @RETVAL	
 */

int divoom_gif_lib_get_pic_number(void * pdivoom_gif_handle);

/**
 * NAME         divoom_gif_lib_get_pic_info
 * @BRIEF	    获取图像的信息
                
 * @PARAM	    pdivoom_gif_handle : 
 
 * @RETURN	    图像的数目
 * @RETVAL	
 */

int divoom_gif_lib_get_pic_info(void *handle, int *width, int *high, int *speed);

/**
 * NAME         divoom_gif_lib_exit
 * @BRIEF	    释放相关资源
                
 * @PARAM	    handle : 
                
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_gif_lib_exit(void *handle);


/**
 * NAME         divoom_gif_lib_bgra_get_bgra_data
 * @BRIEF	    从句柄中得到多个bgra图像,这个返回值不需要释放，是循环使用
                
 * @PARAM	    handle : 
                end_flag
 
 * @RETURN	    返回bgra缓冲区
 * @RETVAL	
 */

uint8 * divoom_gif_lib_bgra_get_bgra_data(void *handle, int *speed, int *end_flag);
#define divoom_gif_lib_get_frame_rgb565a8_data divoom_gif_lib_bgra_get_bgra_data


/**
 * NAME         divoom_gif_lib_bgra_pass_frame_num
 * @BRIEF	    跳过多少帧图像
                
 * @PARAM	    handle :
                pic_num
 
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_gif_lib_bgra_pass_frame_num(void *handle, int pic_num);
#define divoom_gif_lib_rgb565a8_pass_frame_num divoom_gif_lib_bgra_pass_frame_num

/**
 * NAME         divoom_gif_lib_bgra_get_bgra_data
 * @BRIEF	    从句柄中得到多个bgra图像,这个返回值需要释放
                
 * @PARAM	    handle :
                speed
                end_flag
 
 * @RETURN	    返回bgra缓冲区
 * @RETVAL	
 */

uint8 * divoom_gif_lib_bgra_get_malloc_bgra_data(void *handle, int *speed,int *end_flag);
#define divoom_gif_lib_get_malloc_rgb565a8_data divoom_gif_lib_bgra_get_malloc_bgra_data

#endif

