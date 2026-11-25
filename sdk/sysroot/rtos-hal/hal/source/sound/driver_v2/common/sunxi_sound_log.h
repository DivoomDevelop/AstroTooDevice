/*
 * sound\sound_sunxi_rxsync.h
 * (C) Copyright 2025-2029
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SUNXI_SOUND_LOG_H
#define __SUNXI_SOUND_LOG_H

/******* driver log func *******/
#define SOUNDRV_LOG_COLOR_RED		"\e[31m"
#define SOUNDRV_LOG_COLOR_GREEN		"\e[32m"
#define SOUNDRV_LOG_COLOR_NONE		"\e[0m"

#ifdef CONFIG_SUNXI_SOUND_DEBUG_ON
#define SOUND_LOG_DEBUG(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_GREEN "[SND_DEBUG][%s:%d]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
#else
#define SOUND_LOG_DEBUG(fmt, args...)
#endif
#define SOUND_LOG_ERR(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_RED "[SND_ERR][%s:%d]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)

#endif /* __SUNXI_SOUND_LOG_H */