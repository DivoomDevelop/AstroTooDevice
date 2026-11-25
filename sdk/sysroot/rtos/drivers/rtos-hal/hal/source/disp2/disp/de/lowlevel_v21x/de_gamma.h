/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :       de_gamma_type.h
 *
 *  Description :       display engine 35x vep basic function declaration
 *
 *  History     :       2021/10/29 v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_GAMMA_H_
#define _DE_GAMMA_H_

#include "de_rtmx.h"

enum gamma_type {
	CHN_GAMMA_TYPE = 1 << 0,
	DISP_GAMMA_TYPE = 1 << 1,
};

extern short g_gamma_funcs[17][1024];
int de_ctc_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data);
s32 de_gamma_init(u32 disp, u32 chn, enum gamma_type type, uintptr_t reg_base);
s32 de_gamma_set_table(u32 disp, u32 chn, enum gamma_type type, u32 en, bool is_gamma_tbl_10bit, u32 *gamma_tbl);
s32 de_gamma_get_reg_blocks(u32 disp, struct de_reg_block **blks, u32 *blk_num);
s32 de_gamma_enable(u32 disp, u32 chn, enum gamma_type type, u32 en);
int de_gamma_apply_csc(unsigned int sel, struct disp_csc_config *config);
int _de_gamma_init(struct disp_bsp_init_para *para);

#endif /* #ifndef _DE_GAMMA_TYPE_H_ */
