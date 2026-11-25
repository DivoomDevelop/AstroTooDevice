/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_SNR_H__
#define __DE_SNR_H__

#include "../include.h"
#include "de_rtmx.h"
#define SNR_REG_COUNT 11

typedef struct _snr_module_param_t {
	int id; // 25
	unsigned int value[SNR_REG_COUNT];
} snr_module_param_t;

int de_snr_init(unsigned int sel, unsigned int chno, uintptr_t reg_base);
int de_snr_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data);
int de_snr_update_regs(unsigned int sel, unsigned int chno);
int de_snr_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height);
s32 de_snr_get_reg_blocks(u32 disp, u32 chn, struct de_reg_block **blks, u32 *blk_num);
#endif
