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
#ifndef __UNIVERSAL_INTERRUPT_FRAMEWORK_H__
#define __UNIVERSAL_INTERRUPT_FRAMEWORK_H__

#include "../../arch/common/uif/core/include/uif_common.h"

#include <hal_interrupt.h>

int init_universal_interrupt_framework(void);

void uif_dump_sys_ic_info(void);
void uif_dump_irq_handler_info(void);
void uif_dump_irq_info(void);

void uif_handle_root_ic_irq(uint32_t irq_id);

int uif_request_irq(uint32_t irq_num, hal_irq_handler_t func, void *data);
int uif_free_irq(uint32_t irq_num, hal_irq_handler_t func, void *data);

int uif_get_irq_enable_state(uint32_t irq_num, int *enabled);

int uif_enable_irq(uint32_t irq_num);
int uif_disable_irq(uint32_t irq_num);

int uif_disable_ic_irq(uint16_t ic_id);
static inline int uif_disable_root_ic_irq(void)
{
	return uif_disable_ic_irq(ROOT_IRQ_CONTROLLER_ID);
}
int uif_disable_all_ic_irq(void);

int uif_request_system_irq(uint32_t irq_num, hal_irq_handler_t func, void *data);

int uif_set_irq_trigger_type(uint32_t irq_num, irq_trigger_type_t trigger_type);

#ifdef CONFIG_COMMAND_IRQ_DEBUG
void uif_dump_irq_handler_time(void);
#endif

#endif /* __UNIVERSAL_INTERRUPT_FRAMEWORK_H__ */