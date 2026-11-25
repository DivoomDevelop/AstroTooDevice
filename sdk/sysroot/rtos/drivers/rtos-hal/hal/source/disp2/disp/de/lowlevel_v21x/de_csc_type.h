/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*****************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *  File name   :       de_csc_type.h
 *
 *  Description :       display engine 2.0 csc struct declaration
 *
 *  History     :       2014/05/16  vito cheng  v0.1  Initial version
****************************************************************************/

#ifndef __DE_CSC_TYPE_H__
#define __DE_CSC_TYPE_H__

union csc2_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:31;
	} bits;
};

union csc2_const_reg {
	u32 dwval;
	struct {
		u32 d:10;
		u32 res0:22;
	} bits;
};

union csc2_coeff_reg {
	u32 dwval;
	struct {
		u32 c:17;
		u32 res0:15;
	} bits;
};

struct csc2_reg {
	union csc2_ctl_reg ctl;
	union csc2_const_reg d0;
	union csc2_const_reg d1;
	union csc2_const_reg d2;
	union csc2_coeff_reg c00;
	union csc2_coeff_reg c01;
	union csc2_coeff_reg c02;
	union csc2_const_reg c03;
	union csc2_coeff_reg c10;
	union csc2_coeff_reg c11;
	union csc2_coeff_reg c12;
	union csc2_const_reg c13;
	union csc2_coeff_reg c20;
	union csc2_coeff_reg c21;
	union csc2_coeff_reg c22;
	union csc2_const_reg c23;
};
//------------------------------------------------
/*offset:0x0000*/
union gamma_cm_en_reg {
	u32 dwval;
	struct {
		u32 cm_en:1;
		u32 res0:31;
	} bits;
};

/*offset:0x0004*/
union gamma_cm_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0010*/
union gamma_cm_c00_reg {
	u32 dwval;
	struct {
		u32 c00:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0014*/
union gamma_cm_c01_reg {
	u32 dwval;
	struct {
		u32 c01:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0018*/
union gamma_cm_c02_reg {
	u32 dwval;
	struct {
		u32 c02:20;
		u32 res0:12;
	} bits;
};

/*offset:0x001c*/
union gamma_cm_c03_reg {
	u32 dwval;
	struct {
		u32 c03:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0020*/
union gamma_cm_c10_reg {
	u32 dwval;
	struct {
		u32 c10:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0024*/
union gamma_cm_c11_reg {
	u32 dwval;
	struct {
		u32 c11:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0028*/
union gamma_cm_c12_reg {
	u32 dwval;
	struct {
		u32 c12:20;
		u32 res0:12;
	} bits;
};

/*offset:0x002c*/
union gamma_cm_c13_reg {
	u32 dwval;
	struct {
		u32 c13:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0030*/
union gamma_cm_c20_reg {
	u32 dwval;
	struct {
		u32 c20:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0034*/
union gamma_cm_c21_reg {
	u32 dwval;
	struct {
		u32 c21:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0038*/
union gamma_cm_c22_reg {
	u32 dwval;
	struct {
		u32 c22:20;
		u32 res0:12;
	} bits;
};

/*offset:0x003c*/
union gamma_cm_c23_reg {
	u32 dwval;
	struct {
		u32 c23:20;
		u32 res0:12;
	} bits;
};

/*color mode:
 * 0,use 0x44 color;
 * 1,vertical gradient;
 * 2,horizontal gradient;
 * 3,colorbar;
 * 4, 16 gray;
 * other,reserved*/
/*offset:0x0040*/
union gamma_ctl_reg {
	u32 dwval;
	struct {
		u32 gamma_en:1;
		u32 color_mode:3;
		u32 blue_en:1;
		u32 res0:27;
	} bits;
};

/*offset:0x0044*/
union gamma_blue_color_reg {
	u32 dwval;
	struct {
		u32 b:10;
		u32 g:10;
		u32 r:10;
		u32 res0:2;
	} bits;
};

/*offset:0x0050*/
union gamma_ctc_ctl {
	u32 dwval;
	struct {
		u32 ctc_en:1;
		u32 res0:31;
	} bits;
};

/*offset:0x0054*/
union gamma_r_gain {
	u32 dwval;
	struct {
		u32 r_gain:10;
		u32 res0:22;
	} bits;
};

/*offset:0x0058*/
union gamma_g_gain {
	u32 dwval;
	struct {
		u32 g_gain:10;
		u32 res0:22;
	} bits;
};

/*offset:0x005C*/
union gamma_b_gain {
	u32 dwval;
	struct {
		u32 b_gain:10;
		u32 res0:22;
	} bits;
};

/*offset:0x0060*/
union gamma_r_offset {
	u32 dwval;
	struct {
		u32 r_offset:9;
		u32 res0:23;
	} bits;
};

/*offset:0x0064*/
union gamma_g_offset {
	u32 dwval;
	struct {
		u32 g_offset:9;
		u32 res0:23;
	} bits;
};

/*offset:0x0068*/
union gamma_b_offset {
	u32 dwval;
	struct {
		u32 b_offset:9;
		u32 res0:23;
	} bits;
};

/*offset:0x0100+n*4*/
union gamma_tab_reg {
	u32 dwval;
	struct {
		u32 tab_b:10;
		u32 tab_g:10;
		u32 tab_r:10;
		u32 res0:2;
	} bits;
};

//=================================
union CSC_BYPASS_REG2 {
	unsigned int dwval;
	struct {
		unsigned int res0:1;
		unsigned int enable:1;
		unsigned int res1:30;
	} bits;
};

union CSC_COEFF_REG {
	unsigned int dwval;
	struct {
		unsigned int coeff:13;
		unsigned int res0:19;
	} bits;
};

union CSC_CONST_REG {
	unsigned int dwval;
	struct {
		unsigned int cnst:20;
		unsigned int res0:12;
	} bits;
};

union CSC_CONST_REG2 {
	unsigned int dwval;
	struct {
		unsigned int cnst:14;
		unsigned int res0:18;
	} bits;
};


/* CSC IN SMBL */
struct __csc2_reg_t {
	union CSC_BYPASS_REG2 bypass;
	unsigned int res[31];
	union CSC_COEFF_REG c00;
	union CSC_COEFF_REG c01;
	union CSC_COEFF_REG c02;
	union CSC_CONST_REG2 c03;
	union CSC_COEFF_REG c10;
	union CSC_COEFF_REG c11;
	union CSC_COEFF_REG c12;
	union CSC_CONST_REG2 c13;
	union CSC_COEFF_REG c20;
	union CSC_COEFF_REG c21;
	union CSC_COEFF_REG c22;
	union CSC_CONST_REG2 c23;
};


#endif
