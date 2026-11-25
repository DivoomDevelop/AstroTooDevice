/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_CLOCK_H__
#define __DE_CLOCK_H__

#include "../include.h"
#include "de_feat.h"

enum de_clk_id {
	DE_CLK_NONE = 0,

	DE_CLK_CORE0 = 1,
	DE_CLK_CORE1 = 2,
	DE_CLK_WB = 3,
};

struct de_clk_para {
	enum de_clk_id clk_no;
	unsigned int div;
	unsigned int ahb_gate_adr;
	unsigned int ahb_gate_shift;
	unsigned int ahb_reset_adr;
	unsigned int ahb_reset_shift;
	unsigned int mbus_gate_adr;
	unsigned int mbus_gate_shift;
	unsigned int glb_clk_gate_adr;
	unsigned int glb_clk_gate_shift;
};

extern s32 de_clk_enable(u32 clk_no);
extern s32 de_clk_disable(u32 clk_no);
extern s32 de_clk_set_reg_base(uintptr_t reg_base);
#if defined(CONFIG_ARCH_SUN50IW10)
extern s32 de1_clk_set_reg_base(uintptr_t reg_base);
#endif
#endif
