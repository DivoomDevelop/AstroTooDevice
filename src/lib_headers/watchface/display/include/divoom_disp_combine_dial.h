#ifndef _divoom_disp_combine_dial_h_

#define _divoom_disp_combine_dial_h_

void *divoom_disp_combine_dial_init(void *data, uint8 *lcd_list, uint8 lcd_num);

void divoom_disp_combine_dial_update(void *handle, void *data, uint8 *lcd_list, uint8 lcd_num);

void divoom_disp_combine_dial_update_lcd_list(void *handle, uint8 *lcd_list, uint8 lcd_num);

void divoom_disp_combine_dial_exit(void *handle);
void divoom_disp_combine_dial_get_app_data(void *handle, int clock_id, int independ_index, Pdivoom_clock_control_disp_data pdisp_data);

#endif

