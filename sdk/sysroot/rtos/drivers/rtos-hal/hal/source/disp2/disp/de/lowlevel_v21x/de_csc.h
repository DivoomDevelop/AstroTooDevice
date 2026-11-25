/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_CSC_H__
#define __DE_CSC_H__

#include "de_rtmx.h"
#define CSC_ENHANCE_MODE_NUM 3

int de_dcsc_apply(unsigned int sel, struct disp_csc_config *config);
int de_dcsc_set_colormatrix(unsigned int sel, long long *matrix4x3,
			    bool is_identity);
int de_dcsc_init(struct disp_bsp_init_para *para);
int de_dcsc_exit(void);
int de_dcsc_update_regs(unsigned int sel);
int de_dcsc_get_config(unsigned int sel, struct disp_csc_config *config);

int de_ccsc_apply(unsigned int sel, unsigned int ch_id,
		  struct disp_csc_config *config);
int de_ccsc_update_regs(unsigned int sel);
int de_ccsc_init(struct disp_bsp_init_para *para);
int de_ccsc_exit(void);
int de_csc_coeff_calc(unsigned int infmt, unsigned int incscmod, unsigned int in_color_range,
					unsigned int outfmt, unsigned int outcscmod, unsigned int out_color_range,
					unsigned int brightness, unsigned int contrast, unsigned int saturation, unsigned int hue,
					int *csc_coeff, bool use_user_matrix);
int de_csc2_coeff_calc(unsigned int infmt, unsigned int incscmod, unsigned int in_color_range,
					unsigned int outfmt, unsigned int outcscmod, unsigned int out_color_range,
					unsigned int brightness, unsigned int contrast, unsigned int saturation, unsigned int hue,
					int *csc_coeff, bool use_user_matrix);
s32 de_dcsc_get_reg_blocks(u32 disp, struct de_reg_block **blks,
		u32 *blk_num);
s32 de_ccsc_get_reg_blocks(u32 disp, struct de_reg_block **blks,
		u32 *blk_num);
__s64 IntRightShift64(__s64 datain, unsigned int shiftbit);

#endif
