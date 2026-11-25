#ifndef _divoom_clock_gif_control_h_
#define _divoom_clock_gif_control_h_


/**
 * NAME         divoom_clock_gif_control_init
 * @BRIEF	    初始化模块
                
 * @PARAM	    none
 
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_clock_gif_control_init(void);
typedef uint8 DIVOOM_CLOCK_GIF_CONTROL_DISP_END(void *handle, uint8 lcd_id);
typedef uint8 DIVOOM_CLOCK_GIF_CONTROL_READ_END(void *handle, uint8 lcd_id, char *new_file_name);

/**
 * NAME         divoom_clock_gif_control_add
 * @BRIEF	    增加 一组显示,显示是以LCD为基准，一个LCD只能显示一个字
                
 * @PARAM	    lcd_id
                font_id
                disp_value
 
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_clock_gif_control_add(uint8 lcd_id, int font_id, int disp_value, DIVOOM_CLOCK_GIF_CONTROL_DISP_END pend_call, void *para);

/**
 * NAME         divoom_clock_gif_control_play_ctrl
 * @BRIEF	    启动或暂停播放,为了做全局协同播放
                
 * @PARAM	    flag
 
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_clock_gif_control_play_ctrl(uint8 flag);

/**
 * NAME         divoom_clock_gif_control_add_gif_file
 * @BRIEF	    增加 一组显示,显示是以LCD为基准，一个LCD一个GIF文件
                
 * @PARAM	    lcd_list
                lcd_num
                file_name
 
 * @RETURN	    none
 * @RETVAL	
 */

int divoom_clock_gif_control_add_gif_file(uint8 *lcd_list, uint8 lcd_num,char *file_name, DIVOOM_CLOCK_GIF_CONTROL_DISP_END pend_call, void *para, DIVOOM_CLOCK_GIF_CONTROL_READ_END pread_call);

/**
 * NAME         diovom_clock_gif_control_remove
 * @BRIEF	    移除 一组显示
                
 * @PARAM	    lcd_id
 
 * @RETURN	    none
 * @RETVAL	
 */

void diovom_clock_gif_control_remove(uint8 lcd_id);

#endif

