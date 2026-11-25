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

#ifndef __COMMON_PWM_BL_H__
#define __COMMON_PWM_BL_H__

/* PWMBL Register Offset Definitions */
#define PWMBL_CTRL_REG              0x0000  /* PWMBL Control Register */
#define PWMBL_CLK_CFG_REG           0x0004  /* PWMBL Clock Configuration Register */
#define PWMBL_PRD_REG               0x0008  /* PWMBL Period Register */
#define PWMBL_ACT_CYCLE_REG         0x000C  /* PWMBL Active Cycle Register */
#define PWMBL_ACT_CYCLE_STEP_REG    0x0010  /* PWMBL Active Cycle Step Register */
#define PWMBL_ACT_UP_CYCLE_REG      0x0014  /* PWMBL Active Update Cycle Register */
#define PWMBL_OCP_FLT_V_REG         0x0018  /* PWMBL OCP Filter Value Register */
#define PWMBL_INT_EN_REG            0x0040  /* PWMBL Interrupt Enable Register */
#define PWMBL_INT_PENDING_REG       0x0044  /* PWMBL Interrupt Pending Register */
#define PWMBL_ANA0_REG              0x0050  /* PWMBL Analog 0 Register */
#define PWMBL_ANA1_REG              0x0054  /* PWMBL Analog 1 Register */
#define PWMBL_ANA2_REG              0x0058  /* PWMBL Analog 2 Register */
#define PWMBL_ANA3_REG              0x005C  /* PWMBL Analog 3 Register */
#define PWMBL_RSD_REG               0x0060  /* PWMBL Reserved Register */
#define PWMBL_GPADC_CTRL_REG        0x0100  /* PWMBL GPADC Control Register */
#define PWMBL_GPADC_VF_REG          0x0104  /* PWMBL GPADC Voltage Reference Register */
#define PWMBL_GPADC_V0_REG          0x0110  /* PWMBL GPADC Voltage 0 Register */
#define PWMBL_GPADC_V1_REG          0x0114  /* PWMBL GPADC Voltage 1 Register */
#define PWMBL_GPADC_V2_REG          0x0118  /* PWMBL GPADC Voltage 2 Register */
#define PWMBL_GPADC_AC_STEP0_REG    0x0120  /* PWMBL GPADC Active Cycle Step0 Register */
#define PWMBL_GPADC_AC_STEP1_REG    0x0124  /* PWMBL GPADC Active Cycle Step1 Register */
#define PWMBL_GPADC_AC_STEP2_REG    0x0128  /* PWMBL GPADC Active Cycle Step2 Register */
#define PWMBL_GPADC_AC_STEP3_REG    0x012C  /* PWMBL GPADC Active Cycle Step3 Register */

/*****************************************************************************
 * define  register bit field
 *****************************************************************************/

/* Control Register Bit Definitions */
#define PWMBL_ENABLE_SHIFT      0
#define PWMBL_ENABLE_WIDTH      1
#define PWMBL_CLK_GATING_SHIFT  1
#define PWMBL_CLK_GATING_WIDTH  1
#define PWMBL_MODE_SEL_SHIFT    2
#define PWMBL_MODE_SEL_WIDTH    1
#define PWMBL_OUTPUT_EN_SHIFT   3
#define PWMBL_OUTPUT_EN_WIDTH   1

/* Clock Configuration Register Bit Definitions */
#define PWMBL_CLK_SRC_SHIFT     30
#define PWMBL_CLK_SRC_WIDTH     2
#define PWMBL_DIV_K_SHIFT       8
#define PWMBL_DIV_K_WIDTH       7
#define PWMBL_DIV_M_SHIFT       0
#define PWMBL_DIV_M_WIDTH       4

/* Period Configuration Register Bit Definitions */
#define PWMBL_ENTIRE_CYCLE_SHIFT 0
#define PWMBL_ENTIRE_CYCLE_WIDTH 16
#define PWMBL_INIT_ACTIVE_CYCLE_SHIFT 16
#define PWMBL_INIT_ACTIVE_CYCLE_WIDTH 16
#define PWMBL_MAX_ACTIVE_CYCLE_SHIFT  0
#define PWMBL_MAX_ACTIVE_CYCLE_WIDTH  16

/* GPADC Duty Cycle Update Configuration */
#define PWMBL_GPADC_ACT_CYCLE_STEP0_SHIFT 0
#define PWMBL_GPADC_ACT_CYCLE_STEP0_WIDTH 16
#define PWMBL_GPADC_ACT_CYCLE_STEP1_SHIFT 0
#define PWMBL_GPADC_ACT_CYCLE_STEP1_WIDTH 16
#define PWMBL_GPADC_ACT_CYCLE_STEP2_SHIFT 0
#define PWMBL_GPADC_ACT_CYCLE_STEP2_WIDTH 16
#define PWMBL_GPADC_ACT_CYCLE_STEP3_SHIFT 0
#define PWMBL_GPADC_ACT_CYCLE_STEP3_WIDTH 16

/* GPADC Duty Cycle Update Time Interval Configuration */
#define PWMBL_ACT_UPDATE_CYCLE_SHIFT 0
#define PWMBL_ACT_UPDATE_CYCLE_WIDTH 8

/* Analog Configuration Register Bit Definitions */
#define PWMBL_ANA_FREQ_SEL_SHIFT 0
#define PWMBL_ANA_FREQ_SEL_WIDTH 2

/* Reference Voltage Configuration Register Bit Definitions */
#define PWMBL_VREF_SEL_SHIFT    0
#define PWMBL_VREF_SEL_WIDTH    3
#define PWMBL_DIMM_RATIO_SHIFT  3
#define PWMBL_DIMM_RATIO_WIDTH  8

/* Configure OCP Enable */
#define PWMBL_OCP_EN_SHIFT 31
#define PWMBL_OCP_EN_WIDTH 1

/* Configure Maximum Brightness Reference Voltage */
#define PWMBL_ANA_MAX_BRIGHTNESS_SHIFT 0
#define PWMBL_ANA_MAX_BRIGHTNESS_WIDTH 3

/* Configure Load Current Vref Value */
#define PWMBL_LOAD_CURRENT_SHIFT 16
#define PWMBL_LOAD_CURRENT_WIDTH 8

/* Configure Analog Module Enable */
#define PWMBL_ANA_MODULE_EN_SHIFT 0
#define PWMBL_ANA_MODULE_EN_WIDTH 1

/* Configure PWM Output Enable Switch */
#define PWMBL_OUTPUT_SWITCH_SHIFT 0
#define PWMBL_OUTPUT_SWITCH_WIDTH 1

/* Configure OCP Parameters */
#define PWMBL_OCP_INIT_EN_SHIFT 5
#define PWMBL_OCP_INIT_EN_WIDTH 1
#define PWMBL_OCP_FLT_V_SHIFT 0
#define PWMBL_OCP_FLT_V_WIDTH 8
#define PWMBL_OCP_FLT_EN_SHIFT 31
#define PWMBL_OCP_FLT_EN_WIDTH 1

/* Configure Reference Voltage */
#define PWMBL_REF_VOLTAGE_SHIFT 0
#define PWMBL_REF_VOLTAGE_WIDTH 11

/* Interrupt Enable Configuration */
#define PWMBL_ACT_CYCLE_MAX_INT_EN_WIDTH 1
#define PWMBL_ACT_CYCLE_MAX_INT_EN_SHIFT 3
#define PWMBL_ACT_CYCLE_ERR_INT_EN_WIDTH 1
#define PWMBL_ACT_CYCLE_ERR_INT_EN_SHIFT 2
#define PWMBL_OCP_TRIG_INT_EN_WIDTH 1
#define PWMBL_OCP_TRIG_INT_EN_SHIFT 0

#define PWMBL_RSD_REG_SW_CLK_SHIFT 7
#define PWMBL_RSD_REG_SW_CLK_WIDTH 2

#endif /* __COMMON_PWM_BL_H__ */