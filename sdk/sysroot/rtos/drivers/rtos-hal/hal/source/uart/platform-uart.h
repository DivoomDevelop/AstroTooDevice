/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __UART_PLATFORM_H__
#define __UART_PLATFORM_H__

#if defined(CONFIG_ARCH_SUN8IW19)
#include "platform/uart-sun8iw19.h"
#endif
#if defined(CONFIG_ARCH_SUN8IW18P1)
#include "platform/uart-sun8iw18.h"
#endif
#if defined(CONFIG_ARCH_SUN8IW20) || defined (CONFIG_ARCH_SUN20IW1)
#include "platform/uart-sun8iw20.h"
#endif
#if defined(CONFIG_ARCH_SUN8IW21) || defined (CONFIG_ARCH_SUN20IW3)
#include "platform/uart-sun8iw21.h"
#endif
#if defined(CONFIG_ARCH_SUN50IW11)
#include "platform/uart-sun50iw11.h"
#endif
#if defined(CONFIG_ARCH_SUN20IW2)
#include "../ccmu/sunxi-ng/ccu-sun20iw2.h"
#include "platform/uart-sun20iw2p1.h"
#endif
#if defined(CONFIG_ARCH_SUN55IW3)
#include "../ccmu/sunxi-ng/ccu-sun55iw3.h"
#include "platform/uart-sun55iw3.h"
#endif
#if defined(CONFIG_ARCH_SUN55IW6)
#include "../ccmu/sunxi-ng/ccu-sun55iw6.h"
#include "platform/uart-sun55iw6.h"
#endif
#if defined(CONFIG_ARCH_SUN60IW1)
#include "../ccmu/sunxi-ng/ccu-sun60iw1.h"
#include "platform/uart-sun60iw1.h"
#endif
#if defined(CONFIG_ARCH_SUN300IW1)
#include "../ccmu/sunxi-ng/ccu-sun300iw1-app.h"
#include "platform/uart-sun300iw1.h"
#endif
#if defined(CONFIG_ARCH_SUN251IW1)
#include "platform/uart-sun251iw1.h"
#endif
#if defined(CONFIG_ARCH_SUN252IW1)
#include "platform/uart-sun252iw1.h"
#endif
#if defined(CONFIG_ARCH_SUN252IW2)
#include "platform/uart-sun252iw2.h"
#endif
#if defined(CONFIG_ARCH_SUN65IW1)
#include "platform/uart-sun65iw1.h"
#endif

#ifndef RESET_TYPE
#define RESET_TYPE      HAL_SUNXI_RESET;
#endif

#ifndef CLK_TYPE
#define CLK_TYPE        HAL_SUNXI_CCU;
#endif

#ifndef SCLK_TYPE
#define SCLK_TYPE       HAL_SUNXI_CCU
#endif

#ifndef PCLK_TYPE
#define PCLK_TYPE       HAL_SUNXI_AON_CCU
#endif

#ifndef	UART_MAILBOX_SIZE
#define UART_MAILBOX_SIZE	1024
#endif

/*
 * uart3 (JL BtCore @ 1.5Mbps)：一线缓冲已改由 hal_uart_rx_ring_* 的字节环承担，
 * mailbox 只作为字节环启用失败时的退路。每项 4 字节存 1 字节，8192 项要吃 32KB
 * 却只换来 8KB 缓冲，故降到 1024 项（4KB）。
 */
#ifndef UART3_MAILBOX_SIZE
#define UART3_MAILBOX_SIZE	1024
#endif

/*
 * RX 字节环的静态缓冲。给 uart3 预留而不走 hal_malloc，是因为回退路径（mailbox
 * 1024 项 = 1KB 缓冲）比启用环之前的 8KB 更弱：一旦启动期这块堆分配失败，
 * 无流控的 UART3 会立刻变得极易丢字节。静态预留把这条退化路径彻底去掉。
 * 容量必须是 2 的幂，见 hal_uart_rx_ring_enable。
 */
#ifndef UART_RX_RING_STATIC_PORT
#define UART_RX_RING_STATIC_PORT	3
#endif
#ifndef UART_RX_RING_STATIC_SIZE
#define UART_RX_RING_STATIC_SIZE	32768u
#endif

#ifndef RX_BUF_LEN
#define	RX_BUF_LEN	4096
#endif

#ifndef RX_TRIGGER_SIZE
#define	RX_TRIGGER_SIZE 1024
#endif

#endif /* __DMA_PLATFORM_H__ */
