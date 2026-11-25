#ifndef _divoom_light_windmill_h_
#define _divoom_light_windmill_h_

uint32 divoom_light_windmill_init(void);
// void divoom_light_windmill_play(uint8 play);
void divoom_light_windmill_close(void);
void divoom_light_windmill_main(void);
void divoom_light_windmill_set_refresh_time(void);

/**
 * @function:    divoom_light_windmill_get_taskid
 * @description: 获取ID
 * @Author:      coronaa
 * @date:        2017-07-07
 * @param        id             [task_id]
 */
void divoom_light_windmill_get_taskid(uint32 id);
#endif

