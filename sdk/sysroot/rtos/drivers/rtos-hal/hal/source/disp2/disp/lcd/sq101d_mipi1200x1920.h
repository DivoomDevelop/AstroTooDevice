/* drivers/video/sunxi/disp2/disp/lcd/sq101d_mipi1200x1920.h
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: lizeyu <lizeyu@allwinnertech.com>
 *
 * sq101d-mipi1200x1920 panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _SQ101D_MIPI_
#define _SQ101D_MIPI_

#include "panels.h"

extern struct __lcd_panel sq101d_panel;

extern s32 bsp_disp_get_panel_info(u32 screen_id, struct disp_panel_para *info);

#endif /*End of file*/

