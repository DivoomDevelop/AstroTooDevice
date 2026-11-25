#ifndef _divoom_lvgl_top_control_h_
#define _divoom_lvgl_top_control_h_
/**
 * NAME         divoom_lvgl_top_control_init
 * @BRIEF	    初始化
 * @PARAM	    none
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_init(void);


/**
 * NAME         divoom_lvgl_top_control_add_top
 * @BRIEF	    添加一个TOP层句柄，负责控件焦点的切换
 * @PARAM	    handle      TOP层的唯一标识
                active_obj  要设置为焦点的控件
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_add_top(void *handle, void *active_obj);

/**
 * NAME         divoom_lvgl_top_control_update_top
 * @BRIEF	    更新一个句柄的焦点，负责控件焦点的切换
 * @PARAM	    handle      TOP层的唯一标识
                active_obj  要设置为焦点的控件
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_update_top(void *handle, void *active_obj);

/**
 * NAME         divoom_lvgl_top_control_remove_top
 * @BRIEF	    移除一个TOP层句柄，负责控件焦点的还原
 * @PARAM	    handle      TOP层的唯一标识
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_remove_top(void *handle);


/**
 * NAME         divoom_lvgl_top_control_update_active
 * @BRIEF	    当系统焦点发生变化时，调用这个函数，一般在菜单DISP之后调用
 * @PARAM	    
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_update_active(void);

/**
 * 将 lv_layer_top() 上的瞬时弹窗抬到当前层最前（后调用者在上）。
 * 持久 overlay 定时刷新勿调用，避免盖住音量/亮度等 OSD。
 */
void divoom_lvgl_layer_top_raise(void *obj);

#endif


