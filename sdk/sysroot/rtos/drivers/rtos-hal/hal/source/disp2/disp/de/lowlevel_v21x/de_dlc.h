/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2025 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_DLC_H__
#define __DE_DLC_H__

#include "de_rtmx.h"

int de_dlc_init(unsigned int sel, unsigned int chno, uintptr_t reg_base);
int de_dlc_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data);
s32 de_dlc_tasklet(u32 disp, u32 chn, u32 frame_cnt, u32 frame_updata_cnt);
int de_dlc_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height);

#endif
