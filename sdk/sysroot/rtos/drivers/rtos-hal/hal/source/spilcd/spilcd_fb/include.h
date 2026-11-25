/*
 * include/include.h
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
#ifndef _INCLUDE_H
#define _INCLUDE_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <sunxi_hal_spi.h>
#include <sunxi_hal_dbi.h>
#include <hal_spilcd_fb.h>
#include <hal_gpio.h>
#if defined(CONFIG_ARCH_SUN8IW18P1) || defined(CONFIG_ARCH_SUN8IW19) \
			|| defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW20) \
			|| defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN20IW3)
 #include <sunxi_hal_pwm.h>
#elif defined(CONFIG_ARCH_SUN251IW1) || defined(CONFIG_ARCH_SUN252IW2)
#include <sunxi_hal_pwm_ng.h>
#endif
#if defined(CONFIG_ARCH_SUN252IW2) && defined(CONFIG_DRIVERS_SUNXI_PWM_BL)
#include <sunxi_hal_pwm_bl.h>
#endif

#include <sunxi_hal_pwm.h>
#include <hal_workqueue.h>
#include <hal_interrupt.h>
#include <hal_time.h>
#include <hal_timer.h>
#include <hal_mem.h>
#include <aw_list.h>
#include <hal_log.h>
#include "spilcd_fb_feature.h"

#define SPILCD_GAMMA_TABLE_SIZE (256 * sizeof(unsigned int))
#define SPILCD_FB_DEBUG_LEVEL 0
#define SPILCD_FB_MAX 2 /*TODO:auto detect*/

#if SPILCD_FB_DEBUG_LEVEL == 1
#define spilcd_fb_inf(msg...) do { \
			hal_log_info("[SPILCD_FB] %s,line:%d:", __func__, __LINE__); \
			hal_log_info(msg); \
	} while (0)
#define spilcd_fb_msg(msg...) do { \
			hal_log_info("[SPILCD_FB] %s,line:%d:", __func__, __LINE__); \
			hal_log_info(msg); \
	} while (0)
#define spilcd_fb_here
#define spilcd_fb_dbg(msg...)
#elif SPILCD_FB_DEBUG_LEVEL == 2
#define spilcd_fb_inf(msg...) do { \
			hal_log_info("[SPILCD_FB] %s,line:%d:", __func__, __LINE__); \
			hal_log_info(msg); \
	} while (0)
#define spilcd_fb_here do { \
			hal_log_info("[SPILCD_FB] %s,line:%d\n", __func__, __LINE__);\
	} while (0)
#define spilcd_fb_dbg(msg...)  do { \
			hal_log_debug("[SPILCD_FB] %s,line:%d:", __func__, __LINE__); \
			hal_log_debug(msg); \
	} while (0)
#else
#define spilcd_fb_inf(msg...)
#define spilcd_fb_msg(msg...)
#define spilcd_fb_here
#define spilcd_fb_dbg(msg...)
#endif

#define spilcd_fb_wrn(msg...) do { \
			hal_log_warn("[SPILCD_FB] %s,line:%d:", __func__, __LINE__); \
			hal_log_warn(msg); \
		} while (0)


typedef void (*SPILCD_FUNC) (unsigned int sel);
struct spi_disp_spilcd_function {
	SPILCD_FUNC func;
	unsigned int delay;
};

#define SPILCD_MAX_SEQUENCES 7
struct spi_disp_spilcd_flow {
	struct spi_disp_spilcd_function func[SPILCD_MAX_SEQUENCES];
	unsigned int func_num;
	unsigned int cur_step;
};

struct panel_extend_para {
	unsigned int spilcd_gamma_en;
	unsigned int spilcd_gamma_tbl[256];
	unsigned int spilcd_cmap_en;
	unsigned int spilcd_cmap_tbl[2][3][4];
	unsigned int spilcd_bright_curve_tbl[256];
};

struct spi_disp_rect {
	int x;
	int y;
	unsigned int width;
	unsigned int height;
};


struct spi_disp_spilcd_panel_fun {
	void (*cfg_panel_info)(struct panel_extend_para *info);
	int (*cfg_open_flow)(unsigned int sel);
	int (*cfg_close_flow)(unsigned int sel);
	int (*spilcd_user_defined_func)(unsigned int sel, unsigned int para1,
				     unsigned int para2, unsigned int para3);
	int (*set_bright)(unsigned int sel, unsigned int bright);
	int (*blank)(unsigned int sel, unsigned int en);
	int (*set_var)(unsigned int sel, struct fb_info *info);
	int (*set_addr_win)(unsigned int sel, int x, int y, int width, int height);
	int (*panel_status_check)(unsigned int sel);
};


struct spi_disp_video_timings {
	unsigned int vic;	/* video information code */
	unsigned int tv_mode;
	unsigned int pixel_clk;
	unsigned int pixel_repeat; /* pixel repeat (pixel_repeat+1) times */
	unsigned int x_res;
	unsigned int y_res;
	unsigned int hor_total_time;
	unsigned int hor_back_porch;
	unsigned int hor_front_porch;
	unsigned int hor_sync_time;
	unsigned int ver_total_time;
	unsigned int ver_back_porch;
	unsigned int ver_front_porch;
	unsigned int ver_sync_time;
	unsigned int hor_sync_polarity;	/* 0: negative, 1: positive */
	unsigned int ver_sync_polarity;	/* 0: negative, 1: positive */
	bool b_interlace;
	unsigned int vactive_space;
	unsigned int trd_mode;
	unsigned long      dclk_rate_set; /*unit: hz */
	unsigned long long frame_period; /* unit: ns */
	int                start_delay; /* unit: line */
};

enum spi_disp_fb_mode {
	FB_MODE_SCREEN0 = 0,
	FB_MODE_SCREEN1 = 1,
	FB_MODE_SCREEN2 = 2,
	FB_MODE_DUAL_SAME_SCREEN_TB = 3,/* two screen, top buffer for screen0, bottom buffer for screen1 */
	FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS = 4,/* two screen, they have same contents; */
};

struct spi_disp_fb_create_info {
	enum spi_disp_fb_mode fb_mode;
	unsigned int buffer_num;
	unsigned int width;
	unsigned int height;

	unsigned int output_width;  /* used when scaler mode */
	unsigned int output_height; /* used when scaler mode */
};


struct spilcd_spi_master {
    int port;
    hal_spi_master_config_t cfg;
};

struct spilcd_dbi_master {
    hal_dbi_master_port_t port;
    hal_dbi_config_t cfg;
};

enum spi_disp_spilcd_if {
	SPILCD_FB_IF_SPI = 0,
	SPILCD_FB_IF_DBI = 1,
	SPILCD_FB_IF_CUSTOM = 2,
};

enum spilcd_fb_dbi_if {
	SPILCD_FB_L3I1 = 0x0,
	SPILCD_FB_L3I2 = 0x1,
	SPILCD_FB_L4I1 = 0x2,
	SPILCD_FB_L4I2 = 0x3,
	SPILCD_FB_D2LI = 0x4,
};

enum spilcdfb_dbi_fmt {
	SPILCDFB_DBI_RGB111 = 0x0,
	SPILCDFB_DBI_RGB444 = 0x1,
	SPILCDFB_DBI_RGB565 = 0x2,
	SPILCDFB_DBI_RGB666 = 0x3,
	SPILCDFB_DBI_RGB888 = 0x4,
};

typedef enum {
    PANEL_STATUS_NORMAL,
    PANEL_STATUS_ABNORMAL,
    PANEL_STATUS_UNKNOW,
} panel_status_e;

struct spi_disp_panel_para {
	enum spi_disp_spilcd_if spilcd_if;
	enum spilcd_fb_dbi_if dbi_if;
	unsigned int spilcd_spi_bus_num;

	unsigned int spilcd_data_speed;
	unsigned int spilcd_data_speed_read;
	unsigned int spilcd_x;	/* horizontal resolution */
	unsigned int spilcd_y;	/* vertical resolution */
	unsigned int spilcd_width;	/* width of spilcd in mm */
	unsigned int spilcd_height;	/* height of spilcd in mm */

	unsigned int spilcd_pwm_used;
	unsigned int spilcd_pwm_ch;
	unsigned int spilcd_pwm_freq;
	unsigned int spilcd_pwm_pol;
	enum spilcdfb_pixel_format spilcd_pixel_fmt;
	enum spilcdfb_dbi_fmt spilcd_dbi_fmt;
	unsigned int spilcd_dbi_clk_mode;
	unsigned int spilcd_dbi_te;
	unsigned int fb_buffer_num;

	dbi_source_format_t spilcd_rgb_order;
	unsigned int spilcd_fps;
	unsigned int spilcd_check_skip_frame;

	unsigned int spilcd_frm;
	unsigned int spilcd_gamma_en;
	unsigned int spilcd_bright_curve_en;
	unsigned int lines_per_transfer;
	unsigned int line_length;

	char spilcd_size[8];	/* e.g. 7.9, 9.7 */
	char spilcd_model_name[32];

};



struct spilcd_fb_device {
	struct list_head list;
	struct device *dev;
	char name[32];
	u32 spi_disp;

	void *priv_data;

	struct spi_disp_video_timings timings;
	s32 (*init)(struct spilcd_fb_device *p_spilcd);
	s32 (*exit)(struct spilcd_fb_device *p_spilcd);
	s32 (*enable)(struct spilcd_fb_device *p_spilcd);
	s32 (*fake_enable)(struct spilcd_fb_device *p_spilcd);
	s32 (*disable)(struct spilcd_fb_device *p_spilcd);
	s32 (*is_enabled)(struct spilcd_fb_device *p_spilcd);
	s32 (*get_resolution)(struct spilcd_fb_device *p_spilcd, u32 *xres,
			      u32 *yres);
	s32 (*get_dimensions)(struct spilcd_fb_device *spi_dispdev, u32 *width,
			      u32 *height);
	s32 (*set_color_temperature)(struct spilcd_fb_device *spi_dispdev, s32 color_temperature);
	s32 (*get_color_temperature)(struct spilcd_fb_device *spi_dispdev);
	s32 (*suspend)(struct spilcd_fb_device *p_spilcd);
	s32 (*resume)(struct spilcd_fb_device *p_spilcd);
	s32 (*set_bright)(struct spilcd_fb_device *p_spilcd, u32 bright);
	s32 (*get_bright)(struct spilcd_fb_device *p_spilcd);
	s32 (*backlight_enable)(struct spilcd_fb_device *p_spilcd);
	s32 (*backlight_disable)(struct spilcd_fb_device *p_spilcd);
	s32 (*pwm_enable)(struct spilcd_fb_device *p_spilcd);
	s32 (*pwm_disable)(struct spilcd_fb_device *p_spilcd);
	s32 (*power_enable)(struct spilcd_fb_device *p_spilcd, u32 power_id);
	s32 (*power_disable)(struct spilcd_fb_device *p_spilcd, u32 power_id);
	s32 (*pin_cfg)(struct spilcd_fb_device *p_spilcd, u32 bon);
	s32 (*set_gamma_tbl)(struct spilcd_fb_device *p_spilcd, u32 *tbl,
			      u32 size);
	s32 (*set_bright_dimming)(struct spilcd_fb_device *spi_dispdev, u32 dimming);
	s32 (*enable_gamma)(struct spilcd_fb_device *p_spilcd);
	s32 (*disable_gamma)(struct spilcd_fb_device *p_spilcd);
	s32 (*set_panel_func)(struct spilcd_fb_device *spilcd, char *name,
			      struct spi_disp_spilcd_panel_fun *spilcd_cfg);
	s32 (*set_open_func)(struct spilcd_fb_device *spilcd, SPILCD_FUNC func,
			     u32 delay);
	s32 (*set_close_func)(struct spilcd_fb_device *spilcd, SPILCD_FUNC func,
			      u32 delay);
	int (*gpio_set_value)(struct spilcd_fb_device *p_spilcd,
			      unsigned int io_index, u32 value);
	int (*gpio_set_direction)(struct spilcd_fb_device *p_spilcd,
				  unsigned int io_index, u32 direction);
	int (*get_panel_info)(struct spilcd_fb_device *p_spilcd,
			       struct spi_disp_panel_para *info);
	int (*set_layer)(struct spilcd_fb_device *p_spilcd, struct fb_info *p_info);
	int (*set_var)(struct spilcd_fb_device *p_spilcd, struct fb_info *p_info);
	int (*blank)(struct spilcd_fb_device *p_spilcd, unsigned int en);
	int (*cmd_write)(struct spilcd_fb_device *p_spilcd, unsigned char cmd);
	int (*para_write)(struct spilcd_fb_device *p_spilcd, unsigned char para);
	int (*cmd_read)(struct spilcd_fb_device *p_spilcd, unsigned char cmd,
			unsigned char *rx_buf, unsigned char len);
	int (*wait_for_vsync)(struct spilcd_fb_device *p_spilcd);
};

struct sunxi_spi_disp_source_ops {
	int (*sunxi_spilcd_delay_ms)(unsigned int ms);
	int (*sunxi_spilcd_delay_us)(unsigned int us);
	int (*sunxi_spilcd_backlight_enable)(unsigned int screen_id);
	int (*sunxi_spilcd_backlight_disable)(unsigned int screen_id);
	int (*sunxi_spilcd_pwm_enable)(unsigned int screen_id);
	int (*sunxi_spilcd_pwm_disable)(unsigned int screen_id);
	int (*sunxi_spilcd_power_enable)(unsigned int screen_id,
				       unsigned int pwr_id);
	int (*sunxi_spilcd_power_disable)(unsigned int screen_id,
					unsigned int pwr_id);
	int (*sunxi_spilcd_set_panel_funs)(char *drv_name,
					 struct spi_disp_spilcd_panel_fun *spilcd_cfg);
	int (*sunxi_spilcd_pin_cfg)(unsigned int screen_id, unsigned int bon);
	int (*sunxi_spilcd_gpio_set_value)(unsigned int screen_id,
					 unsigned int io_index, u32 value);
	int (*sunxi_spilcd_gpio_set_direction)(unsigned int screen_id,
					     unsigned int io_index,
					     u32 direction);
	int (*sunxi_spilcd_cmd_write)(unsigned int screen_id,
					 unsigned char cmd);
	int (*sunxi_spilcd_para_write)(unsigned int screen_id,
					 unsigned char para);
	int (*sunxi_spilcd_cmd_read)(unsigned int screen_id,
					 unsigned char cmd, unsigned char *rx_buf, unsigned char len);
};



#endif /*End of file*/
