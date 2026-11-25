/*
 * sound\sound_sunxi_rxsync.h
 * (C) Copyright 2025-2029
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * caiyongheng <caiyongheng@allwinnertech.com>
 * xudongpdc <xudongpdc@allwinnertech.com>
 * zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SUNXI_MACHINE_H
#define __SUNXI_MACHINE_H

#include <sound_v2/sound_adf_core.h>
#include <sound_v2/sound_adf_dai.h>
#include <sound_v2/sound_pcm_common.h>

struct sound_mach_bind {
	unsigned int sysclk;
	int clk_direction;
	int slots;
	int slot_width;
	unsigned int tx_slot_mask;
	unsigned int rx_slot_mask;
};

struct sound_mach_bind_props {
	struct sound_adf_dai_bind_component cpus;   /* single cpu */
	struct sound_adf_dai_bind_component *codecs; /* multi codec */
	struct sound_adf_dai_bind_component platforms;
	unsigned int mclk_fp[2];
	unsigned int mclk_fs;
	unsigned int cpu_pll_fs;
	unsigned int codec_pll_fp[2];
	unsigned int codec_pllin_mode;
	unsigned int codec_pllin_fs;
	unsigned int codec_pllout_mode;
	unsigned int codec_pllout_fs;
};

struct sound_mach_priv {
	struct sound_adf_card card;
	struct sound_mach_bind_props *bind_props;
	struct sound_mach_bind *binds;
	struct sound_adf_dai_bind *dai_bind;
	enum sound_platform_type plat_type;

	struct list_head list;
	
};

int sound_mach_probe(const char* name, enum sound_platform_type plat_type);
void sound_mach_remove(const char* name);

#endif /* __SUNXI_MACHINE_H */
