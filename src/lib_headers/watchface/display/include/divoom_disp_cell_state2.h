#ifndef _divoom_disp_cell_state2_h_
#define _divoom_disp_cell_state2_h_

#include "lvgl.h"

int divoom_disp_cell_state2_is_clock(int clock_id);
void *divoom_disp_cell_state2_create(lv_obj_t *parent, int menu_id);
lv_obj_t *divoom_disp_cell_state2_get_obj(void *handle);
void divoom_disp_cell_state2_set_pos(void *handle, int x, int y);
void divoom_disp_cell_state2_set_visible(void *handle, int visible);
void divoom_disp_cell_state2_pause(void *handle, int pause);
void divoom_disp_cell_state2_destroy(void *handle);

#endif
