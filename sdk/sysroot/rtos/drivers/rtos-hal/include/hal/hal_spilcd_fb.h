/*
 * vendor/allwinner/r328/drivers/lichee/rtos/include/drivers/hal_spilcd_fb/hal_spilcd_fb.h
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#ifndef _HAL_SPILCD_FB_H
#define _HAL_SPILCD_FB_H

#ifdef CONFIG_KERNEL_FREERTOS
#include <aw_types.h>
#else
#include <typedef.h>
#endif
#include <sunxi_hal_common.h>

#ifdef __cplusplus
extern "C" {
#endif

enum spilcdfb_pixel_format {
	SPILCDFB_FORMAT_ARGB_8888 = 0x00,	/* MSB  A-R-G-B  LSB */
	SPILCDFB_FORMAT_ABGR_8888 = 0x01,
	SPILCDFB_FORMAT_RGBA_8888 = 0x02,
	SPILCDFB_FORMAT_BGRA_8888 = 0x03,
	SPILCDFB_FORMAT_XRGB_8888 = 0x04,
	SPILCDFB_FORMAT_XBGR_8888 = 0x05,
	SPILCDFB_FORMAT_RGBX_8888 = 0x06,
	SPILCDFB_FORMAT_BGRX_8888 = 0x07,
	SPILCDFB_FORMAT_RGB_888 = 0x08,
	SPILCDFB_FORMAT_BGR_888 = 0x09,
	SPILCDFB_FORMAT_RGB_565 = 0x0a,
	SPILCDFB_FORMAT_BGR_565 = 0x0b,
	SPILCDFB_FORMAT_ARGB_4444 = 0x0c,
	SPILCDFB_FORMAT_ABGR_4444 = 0x0d,
	SPILCDFB_FORMAT_RGBA_4444 = 0x0e,
	SPILCDFB_FORMAT_BGRA_4444 = 0x0f,
	SPILCDFB_FORMAT_ARGB_1555 = 0x10,
	SPILCDFB_FORMAT_ABGR_1555 = 0x11,
	SPILCDFB_FORMAT_RGBA_5551 = 0x12,
	SPILCDFB_FORMAT_BGRA_5551 = 0x13,
};

struct fb_var_info {
	unsigned int xres;	/* horizontal resolution */
	unsigned int yres;	/* vertical resolution */
	unsigned int xres_virtual;
	unsigned int yres_virtual;
	unsigned int xoffset;	/* horizontal resolution */
	unsigned int yoffset;	/* vertical resolution */
	enum spilcdfb_pixel_format spilcd_pixel_fmt;
	unsigned int bits_per_pixel;
};

struct fb_fix_info {
	unsigned int line_length;	/* line length*/
	unsigned int smem_len;	/* line length*/
	unsigned int smem_start;	/* line length*/
};

struct fb_info {
	struct fb_var_info var;
	struct fb_fix_info fix;
	void *screen_base;

};

/**
 * @name       :bsp_spi_disp_spilcd_blank
 * @brief      :blank display
 * @param[IN]  :disp: id of spilcd(return of disp_init_spilcd)
 * @param[IN]  :en: 1 to blank, 0 to unblank
 * @return     :0 if success
 */
int bsp_spi_disp_spilcd_blank(unsigned int disp, unsigned int en);

/**
 * @name       :bsp_spi_disp_get_screen_width
 * @brief      :get the spilcd width of spilcd panel
 * @param[IN]  :disp: id of spilcd(return of disp_init_spilcd)
 * @return     :spilcd width of spilcd panel
 */
int bsp_spi_disp_get_screen_width(unsigned int disp);

/**
 * @name       :bsp_spi_disp_get_screen_height
 * @brief      :get the spilcd height of spilcd panel
 * @param[IN]  :disp: id of spilcd(return of disp_init_spilcd)
 * @return     :spilcd height of spilcd panel
 */
int bsp_spi_disp_get_screen_height(unsigned int disp);

int bsp_spi_disp_spilcd_backlight_enable(unsigned int disp);

int bsp_spi_disp_spilcd_backlight_disable(unsigned int disp);

int bsp_spi_disp_spilcd_pwm_enable(unsigned int disp);

int bsp_spi_disp_spilcd_pwm_disable(unsigned int disp);

int bsp_spi_disp_spilcd_set_bright(unsigned int disp, unsigned int bright);

int bsp_spi_disp_spilcd_get_bright(unsigned int disp);

int bsp_spi_disp_spilcd_set_var(unsigned int disp, struct fb_info *p_info);

// int bsp_disp_spilcd_set_layer(unsigned int disp, struct fb_info *p_info);

int bsp_spi_disp_spilcd_set_layer(unsigned int spi_disp, struct fb_info *p_info);

s32 bsp_spi_disp_spilcd_wait_for_vsync(unsigned int disp);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/
