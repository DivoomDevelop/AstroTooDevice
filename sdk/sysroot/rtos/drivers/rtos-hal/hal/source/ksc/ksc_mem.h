// SPDX-License-Identifier: GPL-2.0
/*
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
#ifndef _KSC_MEM_H
#define _KSC_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ksc.h"

void *ksc_dma_malloc(u32 num_bytes, void *phys_addr);

void ksc_dma_free(void *virt_addr, void *phys_addr, u32 num_bytes);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/
