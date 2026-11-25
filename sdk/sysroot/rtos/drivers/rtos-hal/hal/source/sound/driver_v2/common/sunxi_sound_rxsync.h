/*
 * sound\snd_sunxi_rxsync.h
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * caiyongheng <caiyongheng@allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SUNXI_SOUND_RXSYNC_H
#define __SUNXI_SOUND_RXSYNC_H

typedef enum {
	RX_SYNC_SYS_DOMAIN,
	RX_SYNC_DSP_DOMAIN,
	RX_SYNC_DOMAIN_CNT
} rx_sync_domain_t;

int sunxi_rx_sync_probe(rx_sync_domain_t domain);
void sunxi_rx_sync_remove(rx_sync_domain_t domain);
int sunxi_rx_sync_register_cb(rx_sync_domain_t domain, int id,
			      void *data, void (*rx_enable)(void *data, bool enable));
void sunxi_rx_sync_unregister_cb(rx_sync_domain_t domain, int id);
void sunxi_rx_sync_startup(rx_sync_domain_t domain, int id);
void sunxi_rx_sync_shutdown(rx_sync_domain_t domain, int id);
void sunxi_rx_sync_control(rx_sync_domain_t domain, int id, bool enable);

#endif /* __SUNXI_SOUND_RXSYNC_H */
