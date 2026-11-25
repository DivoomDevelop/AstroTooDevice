/*
 * sound\sound_sunxi_rxsync.h
 * (C) Copyright 2025-2029
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * zhouxijing <zhouxijing@allwinnertech.com>
 * xudongpdc <xudongpdc@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SUNXI_AAUDIO_H
#define __SUNXI_AAUDIO_H

#ifdef CONFIG_ARCH_SUN251IW1
#include "sun251iw1-codec.h"
#elif defined (CONFIG_ARCH_SUN252IW2)
#include "sun252iw2-codec.h"
/* #elif defined (CONFIG_ARCH_SUNXIWX)
 * #include "sound/driver_v2/codecs/sunxiwx-codec.h"
 */
#endif

struct sunxi_aaudio {
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
};

#endif /* __SUNXI_AAUDIO_H */
