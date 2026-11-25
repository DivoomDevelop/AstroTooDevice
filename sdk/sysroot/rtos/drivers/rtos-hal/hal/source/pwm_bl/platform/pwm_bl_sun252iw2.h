
/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


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

#ifndef __PWM_BL_SUN252IW2_H__
#define __PWM_BL_SUN252IW2_H__

#include <hal_gpio.h>
#include <sunxi_hal_common.h>

/* PWMBL0 register base address */
#define PWMBL0_BASE             0x0200A000UL
#define PWMBL_NUM   4
#define PWMBL_PORT  2
#define PWMBL_PORT_OFFSET   0x200

#define PWMBL_PORT0             0
#define PWMBL_PORT1             1
/* PWMBL1 register base address */
#define PWMBL1_BASE             0x0200A200UL

/****************************************************************************
 * define  clock and reset configuration
 *****************************************************************************/

/* PWMBL0 clock and reset configuration */
#define SUNXI_PWMBL0_CLK_TYPE     HAL_SUNXI_CCU
#define SUNXI_PWMBL0_CLK_BUS_ID   CLK_BUS_PWM_BL
#define SUNXI_PWMBL0_CLK_ID       CLK_PWM_BL
#define SUNXI_PWMBL0_RESET_TYPE   HAL_SUNXI_RESET
#define SUNXI_PWMBL0_RESET_ID     RST_BUS_PWM_BL
#define SUNXI_IRQ_PWMBL0          MAKE_IRQn(36, 0)

/*****************************************************************************
 * define  device parameters
 *****************************************************************************/

#define PWMBL0_PARAMS \
{   .pwmbl_port = 0, \
    .reg_base = PWMBL0_BASE, \
    .irq_num = SUNXI_IRQ_PWMBL0, \
    .bus_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_BUS_PWM_BL, \
    .clk_type = HAL_SUNXI_CCU, .clk_id = CLK_PWM_BL, \
    .reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_PWM_BL, \
}

#define PWMBL1_PARAMS \
{   .pwmbl_port = 1, \
    .reg_base = PWMBL1_BASE, \
    .irq_num = SUNXI_IRQ_PWMBL0, \
    .bus_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_BUS_PWM_BL, \
    .clk_type = HAL_SUNXI_CCU, .clk_id = CLK_PWM_BL, \
    .reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_PWM_BL, \
}

/*****************************************************************************
 * define  gpio
 *****************************************************************************/
typedef struct pwmbl_gpio_t
{
    gpio_pin_t pwmbl_pin;
    int pwmbl_function;
} pwmbl_gpio_t;

static pwmbl_gpio_t pwmbl_gpio[PWMBL_NUM] =
{
    {
        .pwmbl_pin = GPIO_PB0,
        .pwmbl_function = 2,
    },
    {
        .pwmbl_pin = GPIO_PB1,
        .pwmbl_function = 2,
    },
    {
        .pwmbl_pin = GPIO_PB2,
        .pwmbl_function = 2,
    },
    {
        .pwmbl_pin = GPIO_PB3,
        .pwmbl_function = 2,
    },
};

#endif /* __PWM_BL_SUN252IW2_H__ */


