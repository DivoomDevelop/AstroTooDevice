/*
* Allwinner Ethernet driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef __ETHERNETIF_H_
#define __ETHERNETIF_H_

#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_atomic.h>
#include <hal_thread.h>

#include <sunxi_hal_common.h>

#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/sys.h>
#include <lwip/netif.h>
#include <lwip/stats.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>
#include <lwip/inet.h>
#include <lwip/snmp.h>
#include <lwip/ethip6.h>
#include <lwip/etharp.h>
#include <lwip/netifapi.h>

#include "hal_gmac.h"
#include "hal_dwmac.h"

#define PHY_ADDR_MIM (1)
#define PHY_ADDR_MAX (32)
#define PHY_UNUSE_ID (0x1fffffff)

struct ethernetif {
	struct netif *netif;
	struct eth_addr *ethaddr;
	char task_name[32];
	hal_thread_t task;
	const struct sunxi_eth_platform *plat;
	struct sunxi_gmac *gmac;
	struct sunxi_dwmac *dwmac;

	bool start;
};

err_t ethernetif_init(struct netif *netif);
void ethernetif_exit(struct netif *netif);

#ifdef CONFIG_ETHIF_GMAC
int hal_gmac_netif_init(struct ethernetif *ethernetif);
void hal_gmac_netif_exit(struct ethernetif *ethernetif);
int hal_gmac_output(struct ethernetif *ethernetif, struct pbuf *p);
struct pbuf *hal_gmac_input(struct ethernetif *ethernetif);
#else
static inline int hal_gmac_netif_init(struct ethernetif *ethernetif)
{
	return -EINVAL;
}
static inline void hal_gmac_netif_exit(struct ethernetif *ethernetif)
{
}
static inline int hal_gmac_output(struct ethernetif *ethernetif, struct pbuf *p)
{
	return -EINVAL;
}
static inline struct pbuf *hal_gmac_input(struct ethernetif *ethernetif)
{
	return NULL;
}
#endif

#ifdef CONFIG_ETHIF_DWMAC
int hal_dwmac_netif_init(struct ethernetif *ethernetif);
void hal_dwmac_netif_exit(struct ethernetif *ethernetif);
int hal_dwmac_output(struct ethernetif *ethernetif, struct pbuf *p);
struct pbuf *hal_dwmac_input(struct ethernetif *ethernetif);
#else
static inline int hal_dwmac_netif_init(struct ethernetif *ethernetif)
{
	return -EINVAL;
}
static inline void hal_dwmac_netif_exit(struct ethernetif *ethernetif)
{
}
static inline int hal_dwmac_output(struct ethernetif *ethernetif, struct pbuf *p)
{
	return -EINVAL;
}
static inline struct pbuf *hal_dwmac_input(struct ethernetif *ethernetif)
{
	return NULL;
}
#endif

__attribute__((__unused__)) static void pkt_hex_dump(char *prefix_str, void *pkt, unsigned int len)
{
	int i;
	char buf[1024], *ptr = buf;
	unsigned char *data = (unsigned char *)pkt;

	printf("%s: dump pkt buffer %p, len %d\n", prefix_str, pkt, len);

	if (len > 256)
		len = 256;

	for (i = 0; i < len; i++) {
		if (!(i % 16))
			ptr += sprintf(ptr, "\n%s %08x:", prefix_str, i);
		ptr += sprintf(ptr, " %02x", data[i]);
	}
	printf("%s\n", buf);
}

#endif /* __ETHERNETIF_H_ */