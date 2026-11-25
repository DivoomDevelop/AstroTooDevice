#ifndef _divoom_light_start_blink_h_
#define _divoom_light_start_blink_h_

/**
 * NAME               divoom_light_start_blink_init
 * *  @brief       	星星闪烁初始化
 * *  @param[in] 		void
 * *	 @author         wangft               
 * *  @return        刷新时间
 * */
 
//uint32 divoom_light_start_blink_init(void);
uint32 divoom_light_start_blink_init(bool flag);

/**
 * NAME               divoom_light_start_blink_close
 * *  @brief       	星星闪烁退出
 * *  @param[in] 		void
 * *	 @author         wangft               
 * *  @return        void
 * */
 
void divoom_light_start_blink_close(void);

/**
 * NAME               divoom_light_start_blink_main
 * *  @brief       	星星闪烁主任务
 * *  @param[in] 		void
 * *	 @author         wangft               
 * *  @return        void
 * */

void divoom_light_start_blink_main(uint8 *disp_buf, uint8 music_power, uint32 task_id);
void divoom_light_start_blink_set_light(void);
void divoom_light_start_blink_set_data(uint8 *disp_buf);
#endif

