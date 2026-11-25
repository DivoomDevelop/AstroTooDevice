#ifndef __SMAC_WIFI_H__
#define __SMAC_WIFI_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <hal_sem.h>
#include <hal_cache.h>
#include <hal_msgbox.h>
#include <aw_list.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/rpmsg_master.h>


#define XRADIO_RPMSG_NAME "xradio_rpmsg"

#define XRADIO_FUNC_CALL_HOST_TO_SLAVE(n)                 (n<<15)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_ID_MASK            (3<<13)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_CONNECTED_ID       (1<<13)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_ACK_ID             (1<<14)
#define XRADIO_FUNC_CALL_INIT_ID                          (1)
#define XRADIO_FUNC_CALL_DEINIT_ID                        (2)
#define XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID              (3)
#define XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID        (4)
#define XRADIO_FUNC_CALL_VERSION_CHECK_ID                 (5)
#define XRADIO_FUNC_CALL_INIT_ID_ETF                      (6)
#define XRADIO_FUNC_CALL_DEINIT_ID_ETF                    (7)
#define XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID_ETF          (8)
#define XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID_ETF    (9)
#define XRADIO_FUNC_CALL_VERSION_CHECK_ID_ETF             (10)

#define XRADIO_FUNC_CALL_FUNCTION_ID_MASK                 (0xF)

struct wsm_hdr {
	u16 len;
	u16 id;
};

struct wsm_buf {
	u8 *begin;
	u8 *data;
	u8 *end;
};

#ifndef CONFIG_DRIVERS_XRADIO
#define PAS_MEMORY_ADDRESS              0x09000000
#define XRADIO_FWMEM_FW2NET(addr)       ((addr) | 0x60000000)
#define SILICON_SYS_BASE_ADDR           (XRADIO_FWMEM_FW2NET(0))
#define SILICON_AHB_MEMORY_ADDRESS      (SILICON_SYS_BASE_ADDR + 0x08000000)
#define SILICON_PAC_BASE_ADDRESS        (SILICON_SYS_BASE_ADDR + 0x08018000)

#define TX_QUEUE_MAX  32
#define RX_QUEUE_MAX  64
#define TX_QUEUE_MASK  (TX_QUEUE_MAX-1)
#define RX_QUEUE_MASK  (RX_QUEUE_MAX-1)
typedef struct __queue_ctrl {
	volatile u32 read_idx;
	volatile u32 write_idx;
	volatile u32 buffer_idx;
} QUEUE_CTRL;

typedef struct __hif_queue {
	QUEUE_CTRL tx_ctrl;
	QUEUE_CTRL rx_ctrl;
	u32 tx_queue[TX_QUEUE_MAX];
	u32 rx_queue[RX_QUEUE_MAX];
	volatile u32 host_state;
  #define HIF_BYPASS_HOST_STAT_HOST_HAS_DATA_WAIT_TO_TX   (1 << 0)
	volatile u32 exception;
	volatile u32 rx_fifo_addr;
	volatile u32 rx_fifo_len;
} HIF_QUEUE;
#endif

int xradio_func_call_init(void);
void xradio_func_call_deinit(void);
void xradio_fw_call_host_func_handle(void);
int xradio_func_call_info_indication_handle(struct wsm_buf *buf);
void xradio_func_call_version_check(char *fw_version);
int xradio_func_call_init_etf(void);
void xradio_func_call_deinit_etf(void);
void xradio_fw_call_host_func_handle_etf(void);
int xradio_func_call_info_indication_handle_etf(struct wsm_buf *buf);
void xradio_func_call_version_check_etf(char *fw_version);

int xradio_rpmsg_create(void);


#endif
