/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
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
#ifndef __G2D_DEBUG_H__
#define __G2D_DEBUG_H__

#include "g2d_driver_i.h"
#include "g2d_driver.h"
#include "g2d_platform.h"
#include "g2d_top.h"

#include <hal_log.h>
#if defined(CONFIG_KERNEL_FREERTOS)
#include <ktimer.h>
#else
#include <melis/ktimer.h>
#endif


#define REG_INTERVAL 0x04
#define HEXADECIMAL  0x10
#define REG_CL       0x0c

extern __u32 g2d200_loglevel;

#ifdef CONFIG_G2D200_DEBUG
#define G2D_DRV_DBG(fmt, ...) \
	do { \
		if (g2d200_loglevel & 0x1) { \
			hal_log_err("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} else { \
			hal_log_debug("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)

#define G2D_PARA_DBG(fmt, ...) \
	do { \
		if (g2d200_loglevel & 0x2) { \
			hal_log_err("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} else { \
			hal_log_debug("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#define G2D_REG_DBG(fmt, ...) \
	do { \
		if (g2d200_loglevel & 0x4) { \
			hal_log_err("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} else { \
			hal_log_debug("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#define G2D_INFO(fmt, ...) \
	hal_log_info("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#else

#define G2D_DRV_DBG(fmt, ...)
#define G2D_PARA_DBG(fmt, ...)
#define G2D_REG_DBG(fmt, ...)
#define G2D_INFO(fmt, ...)

#endif

#define G2D_ERR(fmt, ...) \
	hal_log_err("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define G2D_WARN(fmt, ...) \
	hal_log_warn("g2d:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

struct g2d_time_info {
	__u8 dump_time_info_en;
	uint64_t ctr_start_ts;
	uint64_t hw_proc_start_ts;
	uint64_t hw_proc_end_ts;
	uint64_t ctr_end_ts;
};

uint64_t __get_ts_diff(uint64_t ts_end, uint64_t ts_start);
void g2d_dump_reg(__u32 io_addr);
void dump_g2d_bld_info(const g2d_bld *g2d_bld_para);
void dump_mixer_para_info(const mixer_para *mixer_paras);
void dump_g2d_blt_h_info(const g2d_blt_h *g2d_blt_h_para);
void dump_g2d_fillrect_h_info(const g2d_fillrect_h *g2d_fillrect_h_para);
void dump_g2d_maskblt_info(const g2d_maskblt *g2d_maskblt_para);

#endif  /* __G2D_DEBUG_H__ */
