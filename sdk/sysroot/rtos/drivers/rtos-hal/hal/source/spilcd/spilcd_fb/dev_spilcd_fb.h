/*
 * drivers/rtos-hal/hal/source/spilcd/spilcd_fb/dev_spilcd_fb.h
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
#ifndef _DEV_SPILCD_FB_H
#define _DEV_SPILCD_FB_H
#include "include.h"

extern struct dev_spilcd_fb_t g_drv_info;

struct dev_spilcd_fb_t {
	struct device *device;
	hal_work start_work;
	unsigned char spilcd_fb_num;
};

#endif /*End of file*/
