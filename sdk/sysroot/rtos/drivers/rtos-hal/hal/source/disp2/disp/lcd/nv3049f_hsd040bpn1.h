#ifndef __NV3049F_HSD040BPN1_H__
#define __NV3049F_HSD040BPN1_H__

#include "panels.h"

void nv3049f_hsd040bpn1_panel_init(u32 sel);
void nv3049f_hsd040bpn1_panel_exit(u32 sel);

#if defined(CONFIG_DISP2_LCD_ESD_DETECT)
s32 nv3049f_hsd040bpn1_set_esd_info(struct disp_lcd_esd_info *p_info);
s32 nv3049f_hsd040bpn1_esd_check(u32 sel);
s32 nv3049f_hsd040bpn1_reset_panel(u32 sel);
#endif

#endif
