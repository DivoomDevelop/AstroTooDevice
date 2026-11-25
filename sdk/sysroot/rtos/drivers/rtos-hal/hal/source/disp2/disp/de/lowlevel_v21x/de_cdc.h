/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_CDC_H_
#define _DE_CDC_H_

enum {
	CDC_REG_BLK_CTL = 0,
	CDC_REG_BLK_LUT0,
	CDC_REG_BLK_LUT1,
	CDC_REG_BLK_LUT2,
	CDC_REG_BLK_LUT3,
	CDC_REG_BLK_LUT4,
	CDC_REG_BLK_LUT5,
	CDC_REG_BLK_LUT6,
	CDC_REG_BLK_LUT7,
	CDC_REG_BLK_NUM,
};

typedef struct _cdc_module_param_t {
	unsigned int cmd;
	unsigned int CDC_LUT0[729];
	unsigned int CDC_LUT1[648];
	unsigned int CDC_LUT2[648];
	unsigned int CDC_LUT3[576];
	unsigned int CDC_LUT4[648];
	unsigned int CDC_LUT5[576];
	unsigned int CDC_LUT6[576];
	unsigned int CDC_LUT7[512];
} cdc_module_param_t; // 19656

int de_cdc_init(unsigned int sel, uintptr_t reg_base);
int de_cdc_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data);
int de_cdc_update_regs(unsigned int sel);
s32 de_cdc_get_reg_blocks(u32 disp, struct de_reg_block **blks, u32 *blk_num);
#endif /* #ifndef _DE_CDC_H_ */
