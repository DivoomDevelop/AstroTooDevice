// SPDX-License-Identifier: GPL-2.0
/*
 * ksc.h
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
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
#ifndef _KSC_H
#define _KSC_H
#ifdef CONFIG_KERNEL_FREERTOS
#include <aw_types.h>
#else
#include <typedef.h>
#endif
#include <rtthread.h>
#include <workqueue.h>
#include <hal_timer.h>
#include <stdbool.h>
#include <hal_sem.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <aw_list.h>
#include <hal_gpio.h>
#include <hal_atomic.h>
#include <hal_workqueue.h>
#include <hal_waitqueue.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_queue.h>
#include <hal_log.h>
#include <hal_cfg.h>
#include <stdbool.h>
#include <sunxi_hal_common.h>
#include <sunxi_hal_ksc.h>
#include <aw_common.h>
#include <finsh_api.h>
#include "ksc_macro_def.h"
#include "ksc_sys_intf.h"
#include "ksc_mem.h"

#define dma_addr_t unsigned long

#define FDT_KSC_PATH "ksc"
#define MAX_CLK_NUM 4

#define KSC_KSCBUG_LEVEL 1

#if KSC_KSCBUG_LEVEL == 1
#define KSC_INF(fmt, ...)        do {hal_log_info("ksc:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define KSC_MSG(fmt, ...)        do {hal_log_info("ksc:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define KSC_WRN(fmt, ...) 		 do {hal_log_warn("ksc:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define KSC_ERR(fmt, ...) 		 do {hal_log_err("ksc:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define KSC_DBG(fmt, ...)      do {hal_log_debug("ksc:[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#elif KSC_KSCBUG_LEVEL == 2
#define KSC_INF(...)  hal_log_info(__VA_ARGS__)
#define KSC_HERE(...) hal_log_info(__VA_ARGS__)
#define KSC_DBG(...)  hal_log_debug(__VA_ARGS__)
#else
#define KSC_INF(msg...) asm("nop")
#define KSC_MSG(msg...) asm("nop")
#define KSC_HERE asm("nop")
#define KSC_DBG(msg...) asm("nop")
#endif

#define KSC_IRQ_RETURN 0


extern struct ksc_drv_data ksc110_data;

enum ksc_irq_state {
	KSC_IRQ_INVALID = 0,
	KSC_IRQ_BE_FINISH = 0x1,
	KSC_IRQ_BE_ERR = 0x2,
	KSC_IRQ_FE_ERR = 0x4,
	KSC_IRQ_LUT_OVER_SPAN = 0x8,
	SVP2KSC_DMT_ERR = 0x10,
	KSC_IRQ_DE2KSC_DATA_VOLUME_ERR = 0x20,
};

struct ksc_drv_data {
	int version;
	bool support_offline_ksc;
	bool support_offline_up_scaler;
	bool support_offline_arbitrary_angel_rotation;
	bool support_offline_img_crop;
	bool support_offline_img_flip;
	bool support_bandwidth_control;
};

struct sunxi_ksc_data_match {
	const char *compatible;
	const struct ksc_drv_data *data;
};

struct dmabuf_item {
	struct list_head list;
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	uintptr_t dma_addr;
	struct device *p_device;
};

/**
 * description here
 */
struct ksc_freeing_buffers {
	void *mem0_virt;
	dma_addr_t mem0_phy_addr;
	void *mem1_virt;
	dma_addr_t mem1_phy_addr;
	unsigned int mem_size;
};

struct ksc_buffers {
	bool alloced;
	struct device *p_device;

	void *mem0_virt;
	dma_addr_t mem0_phy_addr;
	void *mem1_virt;
	dma_addr_t mem1_phy_addr;
	unsigned int mem_size;
	unsigned int chm_offset;
	unsigned int pix_2b_offset;

	void *lut_virt;
	dma_addr_t lut_phy_addr;
	unsigned int lut_size;

	void *lut_mem0_virt;
	dma_addr_t lut_mem0_phy_addr;
	void *lut_mem1_virt;
	dma_addr_t lut_mem1_phy_addr;

	struct dmabuf_item *src_item;
	struct dmabuf_item *dst_item;

	struct ksc_freeing_buffers buf_to_be_free;
};

/**
 * description here
 */
struct ksc_band_width_info {
	unsigned int bw_ctrl_en;
	unsigned int bw_ctrl_num;
	int fps;
	int clk_freq;
};

/**
 * ksc_para
 */
struct ksc_para {
	struct sunxi_ksc_para para;
	struct ksc_buffers buf;
	bool is_first_frame;
	struct ksc_band_width_info bw;
};

/**
 * ksc device
 */
struct ksc_device {
	bool enabled;
	hal_sem_t mlock;
	struct device *p_device;

	struct ksc_regs *p_reg;
	void *tv_ksc_top_reg;
	struct device *ksc_top;
	unsigned int irq_no;
	hal_clk_id_t clks[MAX_CLK_NUM];
	hal_reset_id_t rsts[MAX_CLK_NUM];
	hal_waitqueue_head_t event_wait;
	const struct ksc_drv_data *drv_data;
	unsigned int clk_freq[MAX_CLK_NUM];
	bool offline_finish_flag;
	int be_err_cnt;
	int fe_err_cnt;
	int svp2ksc_dmt_err_cnt;
	int irq_cnt;
	struct rt_delayed_work free_buffer_work;


	struct ksc_para ksc_para;

	int (*enable)(struct ksc_device *p_ksc, bool enable);
	int (*set_ksc_para)(struct ksc_device *p_ksc, struct sunxi_ksc_para *para);
	int (*ksc_set_pq_para)(struct ksc_device *p_ksc, struct sunxi_ksc_para *para);
	struct ksc_buffers *(*get_ksc_buffers)(struct ksc_device *p_ksc);
	struct sunxi_ksc_para *(*get_ksc_para)(struct ksc_device *p_ksc);
	int (*get_ksc_lut_buf)(struct ksc_device *p_ksc, bool enable, int dst_w, int dst_h);
};

struct ksc_device *ksc_dev_init(void);
bool is_reg_update_finish(struct ksc_regs *p_reg);
int ksc_reg_update_all(struct ksc_regs *p_reg, struct ksc_para *para);
int ksc_reg_get_version(struct ksc_regs *p_reg);
int ksc_reg_switch_lut_buf(struct ksc_regs *p_reg, struct ksc_para *para);
int ksc_reg_irq_query(struct ksc_regs *p_reg);
bool is_mem_sync_finish(struct ksc_regs *p_reg);
int tvksc_ksc_enable(struct ksc_device *p_ksc, bool enable);

#endif /*End of file*/
