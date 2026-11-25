#ifndef __ASTROTOO_LCD_H__
#define __ASTROTOO_LCD_H__

#include "panels.h"

enum astrotoo_lcd_sel {
	ASTROTOO_LCD_NV3049F = 0,
	ASTROTOO_LCD_ST7701S = 1,
	ASTROTOO_LCD_AUTO = 2,
	ASTROTOO_LCD_JD9161 = 3,
	ASTROTOO_LCD_ST7701S_HSD = 4,
};

extern const struct __lcd_panel astrotoo_lcd_panel;
extern const struct __lcd_panel astrotoo_lcd_panel_legacy;

int astrotoo_lcd_get_selected(void);
void astrotoo_lcd_apply_timing(struct disp_panel_para *panel);
void astrotoo_lcd_commit_timing(u32 disp);

#endif
