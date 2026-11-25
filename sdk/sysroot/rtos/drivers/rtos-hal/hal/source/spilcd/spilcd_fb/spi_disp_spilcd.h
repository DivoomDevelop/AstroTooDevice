/*
 * Allwinner SoCs spi_display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#include "include.h"
#include "spilcd_fb_intf.h"
#include "dev_spilcd_fb.h"
#ifndef __DISP_SPILCD_H__
#define __DISP_SPILCD_H__


#define SUPPORT_MAX_SPILCD SPILCD_FB_MAX
#define SPILCD_GPIO_NUM 8
#define SPILCD_POWER_NUM 4
#define SPILCD_POWER_STR_LEN 32
#define SPILCD_GPIO_REGU_NUM 3
#define SPILCD_GPIO_SCL (SPILCD_GPIO_NUM-2)
#define SPILCD_GPIO_SDA (SPILCD_GPIO_NUM-1)

struct spi_disp_spilcd_cfg {
	bool spilcd_used;

	bool spilcd_bl_en_used;
	struct spi_disp_gpio_set_t spilcd_bl_en;
	int spilcd_bl_gpio_hdl;
	char spilcd_bl_en_power[SPILCD_POWER_STR_LEN];

	u32 spilcd_power_used[SPILCD_POWER_NUM];
	char spilcd_power[SPILCD_POWER_NUM][SPILCD_POWER_STR_LEN];

	/* 0: invalid, 1: gpio, 2: regulator */
	u32 spilcd_fix_power_used[SPILCD_POWER_NUM];
	char spilcd_fix_power[SPILCD_POWER_NUM][SPILCD_POWER_STR_LEN];

	bool spilcd_gpio_used[SPILCD_GPIO_NUM];
	struct spi_disp_gpio_set_t spilcd_gpio[SPILCD_GPIO_NUM];
	int gpio_hdl[SPILCD_GPIO_NUM];
	char spilcd_gpio_power[SPILCD_GPIO_REGU_NUM][SPILCD_POWER_STR_LEN];

	char spilcd_pin_power[SPILCD_GPIO_REGU_NUM][SPILCD_POWER_STR_LEN];
	struct spi_disp_gpio_set_t spilcd_spi_dc_pin;
	int spi_dc_pin_hdl;

	struct spi_disp_gpio_set_t spilcd_spi_te_pin;

	u32 backlight_bright;
	/*
	 * IEP-drc backlight dimming rate:
	 * 0 -256 (256: no dimming; 0: the most dimming)
	 */
	u32 backlight_dimming;
	u32 backlight_curve_adjust[101];

	u32 spilcd_bright;
	u32 spilcd_contrast;
	u32 spilcd_saturation;
	u32 spilcd_hue;
};

s32 spi_disp_init_spilcd(struct dev_spilcd_fb_t *p_info);
s32 spi_disp_exit_spilcd(void);
s32 spi_disp_spilcd_set_bright(struct spilcd_fb_device *spilcd, u32 bright);
s32 spi_disp_spilcd_get_bright(struct spilcd_fb_device *spilcd);
s32 spi_disp_spilcd_gpio_init(struct spilcd_fb_device *spilcd);
s32 spi_disp_spilcd_gpio_exit(struct spilcd_fb_device *spilcd);
s32 spi_disp_spilcd_gpio_set_direction(struct spilcd_fb_device *spilcd, u32 io_index,
				u32 direction);
s32 spi_disp_spilcd_gpio_get_value(struct spilcd_fb_device *spilcd, u32 io_index);
s32 spi_disp_spilcd_gpio_set_value(struct spilcd_fb_device *spilcd, u32 io_index, u32 data);
s32 spi_disp_spilcd_is_enabled(struct spilcd_fb_device *spilcd);
struct spilcd_fb_device *spi_disp_get_spilcd(u32 spi_disp);
void spilcd_fb_black_screen(unsigned int sel);

#endif
