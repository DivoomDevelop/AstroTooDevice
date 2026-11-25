/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.


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

#ifndef __SUNXI_HAL_PWM_BL_H__
#define __SUNXI_HAL_PWM_BL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_gpio.h>
#include <hal_atomic.h>
#include <aw_list.h>
#include <sunxi_hal_common.h>
#include <pwm_bl/common_pwm_bl.h>

//#define CONFIG_DRIVERS_PWM_BL_DEBUG
#ifdef CONFIG_DRIVERS_PWM_BL_DEBUG
#define PWM_BL_INFO(fmt, arg...) hal_log_info(fmt, ##arg)
#else
#define PWM_BL_INFO(fmt, arg...) do {}while(0)
#endif

#define PWM_BL_ERR(fmt, arg...) hal_log_err(fmt, ##arg)

/*****************************************************************************
 * Enums
 *****************************************************************************/

/* PWMBL Controller Count */
#define PWMBL_CTRL_NUM         2
#define PWMBL0                 0
#define PWMBL1                 1

/* Work Mode Enumeration */
typedef enum {
    PWMBL_ANA_ONLY    = 0,  /* Pure analog mode */
    PWMBL_DIG_ANA     = 1,  /* Digital + analog comparator mode */
    PWMBL_DIG_GPADC   = 2   /* Digital + GPADC mode */
} pwmbl_opt_mod_t;

/* Clock Source Selection */
typedef enum {
    PWMBL_SRC_24M = 0,     /* 24MHz clock */
    PWMBL_SRC_APB = 1,     /* APB clock */
    PWMBL_SRC_PERI = 2     /* Peripheral clock */
} pwmbl_clk_src_t;

/* CCU Clock Source Selection */
typedef enum {
    PWMBL_PERI2X = 0,      /* Peripheral 2X clock */
    PWMBL_AUDIO_PLL = 1    /* Audio PLL clock */
} pwmbl_ccu_clk_t;

/* Analog Mode Frequency Selection */
typedef enum {
    PWMBL_ANA_1M = 0,      /* 1MHz */
    PWMBL_ANA_750K = 1,    /* 750KHz */
    PWMBL_ANA_500K = 2,    /* 500KHz */
    PWMBL_ANA_250K = 3     /* 250KHz */
} pwmbl_ana_frq_t;

/* Reference Voltage Range Selection */
typedef enum {
    PWMBL_VF_200mV = 0,    /* 200mV */
    PWMBL_VF_300mV = 1,    /* 300mV */
    PWMBL_VF_400mV = 2,    /* 400mV */
    PWMBL_VF_500mV = 3,    /* 500mV */
    PWMBL_VF_600mV = 4,    /* 600mV */
    PWMBL_VF_700mV = 5,    /* 700mV */
    PWMBL_VF_800mV = 6     /* 800mV */
} pwmbl_vref_max_t;

/* PWMBL Status Enumeration */
typedef enum {
    HAL_PWMBL_STATUS_ERROR_PARAMETER = -3,
    HAL_PWMBL_STATUS_ERROR_CHANNEL = -2,
    HAL_PWMBL_STATUS_ERROR = -1,
    HAL_PWMBL_STATUS_OK = 0
} hal_pwmbl_status_t;

/* PWMBL Status Enumeration */
typedef enum {
    HAL_PWMBL_SW_CLK_8K = 0,
    HAL_PWMBL_SW_CLK_16K = 1,
    HAL_PWMBL_SW_CLK_31K = 2,
    HAL_PWMBL_SW_CLK_4K = 3
} hal_pwmbl_sw_clk_t;
/*****************************************************************************
 * Structures
 *****************************************************************************/

/* PWMBL Device Structure */
typedef struct {
    uint32_t reg_base;          /* Register base address */
    hal_clk_id_t clk_id;        /* Clock ID */
    hal_reset_id_t reset_id;    /* Reset ID */
    uint32_t irq_num;           /* Interrupt number */
    uint32_t initialized;       /* Device initialization flag */
    uint32_t port;              /* Device port number */
    pwmbl_opt_mod_t mode;       /* Current work mode */
    pwmbl_clk_src_t clk_src;    /* Clock source configuration */
    uint32_t clk_div_k;         /* Clock division factor K */
    uint32_t clk_div_m;         /* Clock division factor M */
    uint32_t output_freq;       /* Output frequency */
    uint32_t entire_cycle;      /* Total cycle count */
    uint32_t init_act_cycle;    /* Initial active cycle */
    uint32_t max_act_cycle;     /* Maximum active cycle */
    uint32_t act_step;          /* Duty cycle step value */
    uint32_t update_cycle;      /* Update cycle */
    pwmbl_ana_frq_t ana_freq_sel; /* Analog mode frequency selection */
    uint32_t gpadc_steps[4];    /* GPADC step values */
    uint32_t ocp_reinit;        /* OCP recovery enable */
    uint32_t ocp_enabled;      /* OCP disable flag */
    uint32_t bst_enabled;      /* OCP disable flag */
    uint32_t irq_enabled;       /* Interrupt enable flag */
    pwmbl_vref_max_t vref_sel;  /* Reference voltage selection */
    uint32_t dimm_value;       /* Dimming coefficient value */
    uint32_t adc_code;          /* ADC code value */
    uint32_t adc_voltages[3];  /* ADC voltage thresholds */
    bool output_enabled;    /* Output enable flag */
    pwmbl_opt_mod_t current_mode; /* Current work mode */
    uint32_t clk_gating;          /* clk_gating disable flag */
    uint32_t ana_module_enabled; /* ana module flag */
    hal_pwmbl_sw_clk_t sw_clk;   /*sw clk configuration*/
} pwmbl_dev_t;

/* PWMBL Configuration Structure */
typedef struct {
    uint32_t pwmbl_no;          /* Controller number */
    pwmbl_opt_mod_t mode;       /* Work mode */
    pwmbl_clk_src_t clk_src;    /* Clock source */
    uint32_t k;                 /* Division factor K */
    uint32_t m;                 /* Division factor M */
    uint32_t ent_cyl;           /* Total cycle count */
    uint32_t act_int;           /* Active cycle start */
    uint32_t act_max;           /* Active cycle maximum value */
    pwmbl_vref_max_t vref;      /* Reference voltage */
    uint32_t dimm;             /* Dimming coefficient */
} pwmbl_config_t;

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/* Initialization Functions */
hal_pwmbl_status_t hal_pwmbl_dev_init(uint32_t pwmbl_no);
hal_pwmbl_status_t hal_pwmbl_sys_ccmu_cfg(uint32_t pwmbl_no, pwmbl_ccu_clk_t clk_src, pwmbl_opt_mod_t mode);
hal_pwmbl_status_t hal_pwmbl_gpio_init(uint32_t pwm_index, uint32_t pwm_ch, uint32_t en);
hal_pwmbl_status_t hal_pwmbl_op_mod_sel(uint32_t pwmbl_no, uint32_t mode);

hal_pwmbl_status_t hal_pwmbl_output_en(uint32_t pwmbl_no, bool en);

/* Clock Configuration */
hal_pwmbl_status_t hal_pwmbl_refclk_cfg(uint32_t pwmbl_no, pwmbl_clk_src_t src, uint32_t k, uint32_t m, uint32_t *output_freq);

/* Period and Duty Cycle Configuration */
hal_pwmbl_status_t hal_pwmbl_digital_period_cfg(uint32_t pwmbl_port, uint32_t total_period, 
                                               uint32_t init_active_cycle, uint32_t max_active_cycle);
hal_pwmbl_status_t hal_pwmbl_duty_ud_cfg(uint32_t pwmbl_no, uint32_t step, uint32_t interval);
hal_pwmbl_status_t hal_pwmbl_ana_perio_cfg(uint32_t pwmbl_no, pwmbl_ana_frq_t sel);
hal_pwmbl_status_t hal_pwmbl_gpadc_duty_update_cfg(uint32_t pwmbl_no, uint32_t step1, uint32_t interval1,
                                                  uint32_t step2, uint32_t interval2,
                                                  uint32_t step3, uint32_t interval3,
                                                  uint32_t step4, uint32_t interval4);

/* Protection Functions */
hal_pwmbl_status_t hal_pwmbl_ocp_cfg(uint32_t pwmbl_port, uint32_t reinit, uint32_t fit_count,uint32_t dis);
hal_pwmbl_status_t hal_pwmbl_irq_en(uint32_t pwmbl_no, uint32_t en);

/* Voltage Regulation */
hal_pwmbl_status_t hal_pwmbl_voltage_reg_cfg(uint32_t pwmbl_no, pwmbl_vref_max_t vref, uint32_t dimm);
hal_pwmbl_status_t hal_pwmbl_adc_vol_tune(uint32_t pwmbl_no, uint32_t adc_code, uint32_t v0, uint32_t v1, uint32_t v2);

/* dig compare init */
hal_pwmbl_status_t hal_pwm_bl_dig_compare_init(uint32_t pwmbl_port);

/* analog init */
hal_pwmbl_status_t hal_pwm_bl_analog_init(uint32_t pwmbl_port);

/* dig compare disabled */
hal_pwmbl_status_t hal_pwm_bl_dig_compare_exit(uint32_t pwmbl_port);

/* analog disabled */
hal_pwmbl_status_t hal_pwm_bl_analog_exit(uint32_t pwmbl_port);

/* Device Management */
hal_pwmbl_status_t hal_pwmbl_deinit(uint32_t pwmbl_no);

/* Overcurrent Protection Interrupt Enable Configuration */
hal_pwmbl_status_t hal_pwmbl_ocp_int_en_cfg(uint32_t pwmbl_port, uint32_t enable);

/* Interrupt Enable Configuration */
hal_pwmbl_status_t hal_pwmbl_int_en_cfg(uint32_t pwmbl_no, uint32_t enable);

/* OCP Enable Configuration */
hal_pwmbl_status_t hal_pwmbl_ocp_en_cfg(uint32_t pwmbl_no, uint32_t enable);

/* Analog Maximum Brightness Reference Voltage Configuration */
hal_pwmbl_status_t hal_pwmbl_ana_max_brightness_cfg(uint32_t pwmbl_no, pwmbl_vref_max_t voltage_level);

/* Load Current Configuration */
hal_pwmbl_status_t hal_pwmbl_load_current_cfg(uint32_t pwmbl_no, uint32_t vref_value);

/* Analog Module Enable Configuration */
hal_pwmbl_status_t hal_pwmbl_ana_module_en_cfg(uint32_t pwmbl_no, uint32_t enable);

/* PWM Output Enable Switch Configuration */
hal_pwmbl_status_t hal_pwmbl_output_switch_cfg(uint32_t pwmbl_no, bool enable);

hal_pwmbl_status_t hal_pwmbl_clk_gating_mod_sel(uint32_t pwmbl_port, uint32_t enable);

/* OCP Enable Configuration */
hal_pwmbl_status_t hal_pwmbl_rsd_reg_cfg(uint32_t pwmbl_port, hal_pwmbl_sw_clk_t sw_clk);

/* OCP Voltage Calculation Interface */
void pwmbl_ocp_vol_count(uint32_t vref, pwmbl_vref_max_t* v_m_sel, uint32_t* dimm);

hal_pwmbl_status_t hal_pwmbl_enable(uint32_t pwmbl_port, uint32_t vref_value);

#ifdef __cplusplus
}
#endif

#endif /* __SUNXI_HAL_PWM_BL_H__ */
