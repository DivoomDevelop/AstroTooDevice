#ifndef _divoom_light_sand_paint_h_
#define _divoom_light_sand_paint_h_

void divoom_light_sand_paint_init(uint8 type,uint32 file_index, uint32 disp_time);
void divoom_light_sand_paint_set_callback(DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback);

void divoom_light_sand_paint_play(uint8 play);


void divoom_light_sand_paint_exit(void);

#endif

