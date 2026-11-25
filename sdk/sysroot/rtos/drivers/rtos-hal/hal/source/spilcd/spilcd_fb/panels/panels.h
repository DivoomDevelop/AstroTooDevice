/*
 * Allwinner SoCs spi_display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PANEL_H__
#define __PANEL_H__
#include "../include.h"
#include "spilcd_source.h"
#include "../spi_disp_display.h"
#include "../spi_disp_spilcd.h"

struct __spilcd_panel {
	char name[32];
	struct spi_disp_spilcd_panel_fun func;
};

extern struct __spilcd_panel *spi_panel_array[];

struct sunxi_spilcd_drv {
	struct sunxi_spi_disp_source_ops src_ops;
};

extern int sunxi_spi_disp_get_source_ops(struct sunxi_spi_disp_source_ops *src_ops);
int spilcd_init(void);
#ifdef CONFIG_SPILCD_SUPPORT_KLD35512
extern struct __spilcd_panel kld35512_panel;
#endif

#ifdef CONFIG_SPILCD_SUPPORT_KLD39501
extern struct __spilcd_panel kld39501_panel;
#endif

#ifdef CONFIG_SPILCD_SUPPORT_KLD2844B
extern struct __spilcd_panel kld2844b_panel;
#endif

#ifdef CONFIG_SPILCD_SUPPORT_NV3029S
extern struct __spilcd_panel nv3029s_panel;
#endif

#ifdef CONFIG_SPILCD_SUPPORT_ST7789T3
extern struct __spilcd_panel st7789t3_panel;
#endif

#ifdef CONFIG_SPILCD_SUPPORT_ST77916
extern struct __spilcd_panel st77916_panel;
#endif

#ifdef CONFIG_SPILCD_SUPPORT_ST77903
extern struct __spilcd_panel st77903_panel;
#endif

#endif
