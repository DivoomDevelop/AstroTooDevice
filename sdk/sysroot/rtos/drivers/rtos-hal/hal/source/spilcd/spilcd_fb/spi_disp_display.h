/*
 * Allwinner SoCs spi_display driver.
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "include.h"
#ifndef __DISP_DISPLAY_H__
#define __DISP_DISPLAY_H__

struct spi_disp_dev_t {
	u32 print_level;
	u32 spilcd_registered[3];
};

extern struct spi_disp_dev_t gspi_disp;
void SPILCD_OPEN_FUNC(u32 screen_id, SPILCD_FUNC func, u32 delay);
void SPILCD_CLOSE_FUNC(u32 screen_id, SPILCD_FUNC func, u32 delay);
s32 bsp_spi_disp_get_screen_physical_width(u32 spi_disp);
s32 bsp_spi_disp_get_screen_physical_height(u32 spi_disp);
s32 bsp_spi_disp_get_screen_width(u32 spi_disp);
s32 bsp_spi_disp_get_screen_height(u32 spi_disp);
/* lcd */
s32 bsp_spi_disp_spilcd_set_panel_funs(char *name, struct spi_disp_spilcd_panel_fun *spilcd_cfg);
s32 bsp_spi_disp_spilcd_backlight_enable(u32 spi_disp);
s32 bsp_spi_disp_spilcd_backlight_disable(u32 spi_disp);
s32 bsp_spi_disp_spilcd_pwm_enable(u32 spi_disp);
s32 bsp_spi_disp_spilcd_pwm_disable(u32 spi_disp);
s32 bsp_spi_disp_spilcd_power_enable(u32 spi_disp, u32 power_id);
s32 bsp_spi_disp_spilcd_power_disable(u32 spi_disp, u32 power_id);
s32 bsp_spi_disp_spilcd_set_bright(u32 spi_disp, u32 bright);
s32 bsp_spi_disp_spilcd_get_bright(u32 spi_disp);
s32 bsp_spi_disp_spilcd_pin_cfg(u32 spi_disp, u32 en);
s32 bsp_spi_disp_spilcd_gpio_set_value(u32 spi_disp, u32 io_index, u32 value);
s32 bsp_spi_disp_spilcd_gpio_set_direction(u32 spi_disp, unsigned int io_index,
				    u32 direction);
struct spi_disp_spilcd_flow *bsp_spi_disp_spilcd_get_open_flow(u32 spi_disp);
struct spi_disp_spilcd_flow *bsp_spi_disp_spilcd_get_close_flow(u32 spi_disp);
s32 bsp_spi_disp_get_panel_info(u32 spi_disp, struct spi_disp_panel_para *info);

int bsp_spi_disp_get_spi_display_size(u32 spi_disp, unsigned int *width,
			      unsigned int *height);
void *spilcd_fb_dma_malloc(u32 num_bytes);
void spilcd_fb_dma_free(void *addr);
struct spilcd_fb_device *spilcd_fb_device_get(int spi_disp);
s32 spilcd_fb_device_register(struct spilcd_fb_device *spi_dispdev);
s32 spilcd_fb_device_unregister(struct spilcd_fb_device *spi_dispdev);
s32 bsp_spi_disp_spilcd_set_layer(unsigned int spi_disp, struct fb_info *p_info);
s32 bsp_spi_disp_spilcd_blank(unsigned int spi_disp, unsigned int en);
s32 bsp_spi_disp_spilcd_set_var(unsigned int spi_disp, struct fb_info *p_info);
s32 bsp_spi_disp_spilcd_cmd_write(unsigned int screen_id,
					 unsigned char cmd);
s32 bsp_spi_disp_spilcd_para_write(unsigned int screen_id,
					 unsigned char para);
s32 bsp_spi_disp_spilcd_cmd_read(unsigned int screen_id, unsigned char cmd,
			  unsigned char *rx_buf, unsigned char len);
s32 bsp_spi_disp_spilcd_wait_for_vsync(unsigned int spi_disp);

#endif
