#ifndef _divoom_disp_clock_ctrl_
#define _divoom_disp_clock_ctrl_

typedef struct _divoom_disp_obj_info
{
	int x;
	int y;
	int w;
	int h;
	void *obj; //menu_obj
}divoom_disp_obj_info, *Pdivoom_disp_obj_info;

void *divoom_disp_clock_main_get_main_clock_handle(void);
void *divoom_disp_clock_main_get_sub_clock_obj(int component_type, int clock_pos);
int divoom_disp_clock_main_is_ha(int clock_id);
void *divoom_disp_clock_main_get_ha_slot_obj(unsigned slot);
int divoom_lvgl_dial_menu_ha_editing(void);

void divoom_disp_clock_main_update_disp_data(Pdivoom_clock_id_info clock_id, Pdivoom_clock_control_disp_data pdisp_data);

void divoom_disp_clock_main_update_china_day_info(void);

int divoom_disp_clock_main_get_play_clock(void);

void divoom_disp_clock_main_update(void *data);
void divoom_disp_clock_main_clear_pending_font(void);
//更新表盘显示
void divoom_disp_clock_main_refresh_all(void);

int divoom_disp_clock_main_init(void *data, int menu_id);

void divoom_disp_clock_main_close(void);
void divoom_disp_clock_main_pause(void);

#if defined(DIVOOM_SIMULATOR)
/* 仅白盒故障注入：强制 V3 显示事务失败，以验证 cancel/不消费语义。 */
void divoom_disp_clock_main_whitebox_force_display_failure(uint8 enabled);
#endif

#endif
