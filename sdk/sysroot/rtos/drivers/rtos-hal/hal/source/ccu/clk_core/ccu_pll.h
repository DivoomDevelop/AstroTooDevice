/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
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

#ifndef _CCU_PLL_H_
#define _CCU_PLL_H_

#include "aw_ccu.h"
#include "ccu_multiplier.h"

#define MAX_PLL_CONTROL_BIT_OFFSET MAX_CONTROL_BIT_OFFSET
#define PLL_CTRL_BIT_ABSENT_BIT_OFFSET CTRL_BIT_ABSENT_BIT_OFFSET
#define PLL_STATE_BIT_ABSENT_BIT_OFFSET CTRL_BIT_ABSENT_BIT_OFFSET

#define PLL_DEFAULT_STABLE_TIME 20 /* Unit: us */


enum ccu_pll_var_type
{
	CCU_PLL_VAR_TYPE0 = 0, /* default variant, only one reg */
	CCU_PLL_VAR_TYPE1, /* pll with multiple reg and the lock state bit in a new reg */
	CCU_PLL_VAR_TYPE2, /* pll with spread spectrum clock(type 0, two pattern reg) */
	//CCU_PLL_VAR_TYPE3, /* pll with spread spectrum clock(type 1, two patter reg, one ssc reg) */
};

typedef struct ccu_pll_base_type0
{
	uint8_t var_type;
	uint8_t stable_time;

	uint8_t ldo_ctrl_bit_offset;
	uint8_t enable_bit_offset;
	uint8_t output_gate_bit_offset;

	uint8_t lock_ctrl_bit_offset;
	uint8_t lock_state_bit_offset;

	ccu_mult_base_t mult;
} ccu_pll_base_type0_t;

typedef ccu_pll_base_type0_t ccu_pll_base_t;

typedef struct ccu_pll_base_type1
{
	uint8_t var_type;
	uint8_t stable_time;

	uint8_t enable_bit_offset;

	uint8_t lock_ctrl_bit_offset;
	uint8_t lock_state_bit_offset;
	uint16_t lock_state_reg_offset;

	ccu_mult_base_t mult;
} ccu_pll_base_type1_t;

typedef ccu_pll_base_type1_t ccu_pll_base_with_lock_state_reg_t;

typedef struct ssc_pll_division_cfg
{
	uint16_t div_value;
	clk_number_t div_clk_num;
} ssc_pll_division_cfg_t;

/* spread spectrum clock configuration */
typedef struct type0_ssc_cfg
{
	uint32_t output_freq;
	uint32_t pattern0_data;
	uint32_t pattern1_data;
	uint16_t mult_value;
	uint8_t div_cfg_cnt;
	const ssc_pll_division_cfg_t *div_cfg_table;
} type0_ssc_cfg_t;

typedef struct ccu_pll_base_type2
{
	uint8_t var_type;
	uint8_t stable_time;

	uint8_t ldo_ctrl_bit_offset;
	uint8_t enable_bit_offset;
	uint8_t output_gate_bit_offset;

	uint8_t lock_ctrl_bit_offset;
	uint8_t lock_state_bit_offset;

	uint8_t sdm_enable_bit_offset;

	uint8_t ssc_cfg_cnt;
	uint8_t is_ssc_enable_in_reg1;
	uint8_t ssc_enable_bit_offset;
	uint16_t ssc_cfg_reg0_offset;
	uint16_t ssc_cfg_reg1_offset;

	ccu_mult_base_t mult;

	const type0_ssc_cfg_t *ssc_cfg_table;
} ccu_pll_base_type2_t;

typedef ccu_pll_base_type2_t ccu_pll_base_with_type0_ssc_t;


/* parent class */
typedef ccu_pll_base_t ccu_pll_base_min_t;
/* child class */
typedef ccu_pll_base_with_lock_state_reg_t ccu_pll_base_max_t;

#define CCU_PLL_BASE(_mult_var_type, _mult_field_offset, _mult_field_width, \
		_pll_var_type, _stable_time, _ldo_ctrl_bit_offset, \
		_enable_bit_offset, _output_gate_bit_offset, \
		_lock_ctrl_bit_offset, _lock_state_bit_offset) \
{ \
	.var_type = _pll_var_type, \
	.stable_time = _stable_time, \
	.ldo_ctrl_bit_offset = _ldo_ctrl_bit_offset, \
	.enable_bit_offset = _enable_bit_offset, \
	.output_gate_bit_offset = _output_gate_bit_offset, \
	.lock_ctrl_bit_offset = _lock_ctrl_bit_offset, \
	.lock_state_bit_offset = _lock_state_bit_offset, \
	.mult = CCU_MULT_BASE(_mult_var_type, _mult_field_offset, _mult_field_width), \
}

#define CCU_PLL_BASE_WITH_LOCK_STATE_REG(_mult_var_type, \
		_mult_field_offset, _mult_field_width, \
		_pll_var_type, _stable_time, \
		_enable_bit_offset, \
		_lock_ctrl_bit_offset, _lock_state_reg_offset, _lock_state_bit_offset) \
{ \
	.var_type = _pll_var_type, \
	.stable_time = _stable_time, \
	.enable_bit_offset = _enable_bit_offset, \
	.lock_ctrl_bit_offset = _lock_ctrl_bit_offset, \
	.lock_state_bit_offset = _lock_state_bit_offset, \
	.lock_state_reg_offset = _lock_state_reg_offset, \
	.mult = CCU_MULT_BASE(_mult_var_type, _mult_field_offset, _mult_field_width), \
}

#define CCU_PLL_BASE_WITH_TYPE0_SSC(_mult_var_type, _mult_field_offset, _mult_field_width, \
		_pll_var_type, _stable_time, _ldo_ctrl_bit_offset, \
		_enable_bit_offset, _output_gate_bit_offset, \
		_lock_ctrl_bit_offset, _lock_state_bit_offset, \
		_sdm_enable_bit_offset, _ssc_cfg_reg0_offset, _ssc_cfg_reg1_offset, \
		_ssc_enable_bit_offset, _is_ssc_enable_in_reg1, _ssc_cfg_map) \
{ \
	.var_type = _pll_var_type, \
	.stable_time = _stable_time, \
	.ldo_ctrl_bit_offset = _ldo_ctrl_bit_offset, \
	.enable_bit_offset = _enable_bit_offset, \
	.output_gate_bit_offset = _output_gate_bit_offset, \
	.lock_ctrl_bit_offset = _lock_ctrl_bit_offset, \
	.lock_state_bit_offset = _lock_state_bit_offset, \
	.sdm_enable_bit_offset = _sdm_enable_bit_offset, \
	.ssc_cfg_reg0_offset = _ssc_cfg_reg0_offset, \
	.ssc_cfg_reg1_offset = _ssc_cfg_reg1_offset, \
	.ssc_enable_bit_offset = _ssc_enable_bit_offset, \
	.is_ssc_enable_in_reg1 = _is_ssc_enable_in_reg1, \
	.ssc_cfg_cnt = ARRAY_SIZE(_ssc_cfg_map), \
	.ssc_cfg_table = _ssc_cfg_map, \
	.mult = CCU_MULT_BASE(_mult_var_type, _mult_field_offset, _mult_field_width), \
}


typedef struct ccu_pll
{
	struct aw_ccu_clk_hw ccu_clk_hw;
	ccu_pll_base_t pll;
} ccu_pll_t;

typedef struct ccu_pll_with_lock_state_reg
{
	struct aw_ccu_clk_hw ccu_clk_hw;
	ccu_pll_base_with_lock_state_reg_t pll;
} ccu_pll_with_lock_state_reg_t;

typedef struct ccu_pll_with_type0_ssc
{
	struct aw_ccu_clk_hw ccu_clk_hw;
	ccu_pll_base_with_type0_ssc_t pll;
} ccu_pll_with_type0_ssc_t;

#define AW_CCU_PLL_FULL(_struct_name, _clk_name, _flags, _parent, \
				_reg_offset, _mult_var_type, _mult_field_offset, _mult_field_width, \
				_stable_time, _ldo_ctrl_bit_offset, \
				_enable_bit_offset, _output_gate_bit_offset, \
				_lock_ctrl_bit_offset, _lock_state_bit_offset) \
ccu_pll_t _struct_name = \
{ \
	.pll = CCU_PLL_BASE(_mult_var_type, _mult_field_offset, _mult_field_width, \
		CCU_PLL_VAR_TYPE0, _stable_time, _ldo_ctrl_bit_offset, \
		_enable_bit_offset, _output_gate_bit_offset, \
		_lock_ctrl_bit_offset, _lock_state_bit_offset), \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = CLK_HW_INFO(_clk_name, _parent, &g_ccu_pll_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_PLL_WITH_LOCK_STATE_REG_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_var_type, _mult_field_offset, _mult_field_width, \
			_stable_time, \
			_enable_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_reg_offset, _lock_state_bit_offset) \
ccu_pll_with_lock_state_reg_t _struct_name = \
{ \
	.pll = CCU_PLL_BASE_WITH_LOCK_STATE_REG(_mult_var_type, \
		_mult_field_offset, _mult_field_width, \
		CCU_PLL_VAR_TYPE1, _stable_time, \
		_enable_bit_offset, \
		_lock_ctrl_bit_offset, _lock_state_reg_offset, _lock_state_bit_offset), \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = CLK_HW_INFO(_clk_name, _parent, &g_ccu_pll_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_SSC_PLL_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_var_type, _mult_field_offset, _mult_field_width, \
			_stable_time, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset, \
			_sdm_enable_bit_offset, _ssc_cfg_reg0_offset, _ssc_cfg_reg1_offset, \
			_ssc_enable_bit_offset, _is_ssc_enable_in_reg1, _ssc_cfg_map) \
ccu_pll_with_type0_ssc_t _struct_name = \
{ \
	.pll = CCU_PLL_BASE_WITH_TYPE0_SSC(_mult_var_type, _mult_field_offset, _mult_field_width, \
		CCU_PLL_VAR_TYPE2, _stable_time, _ldo_ctrl_bit_offset, \
		_enable_bit_offset, _output_gate_bit_offset, \
		_lock_ctrl_bit_offset, _lock_state_bit_offset, \
		_sdm_enable_bit_offset, _ssc_cfg_reg0_offset, _ssc_cfg_reg1_offset, \
		_ssc_enable_bit_offset, _is_ssc_enable_in_reg1, _ssc_cfg_map), \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = CLK_HW_INFO(_clk_name, _parent, &g_ccu_pll_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_PLL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_stable_time, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset) \
		AW_CCU_PLL_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_MULT_VAR_TYPE0, _mult_field_offset, _mult_field_width, \
			_stable_time, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset)

#define AW_CCU_PLL_WITH_DEFAULT_STABLE_TIME(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset) \
		AW_CCU_PLL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			PLL_DEFAULT_STABLE_TIME, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset)

#define AW_CCU_PLL_WITHOUT_LDO_CTRL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_stable_time, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset) \
		AW_CCU_PLL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_stable_time, PLL_CTRL_BIT_ABSENT_BIT_OFFSET, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset)


#define AW_CCU_PLL_ZERO_BASED(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_stable_time, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset) \
		AW_CCU_PLL_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_MULT_VAR_TYPE1, _mult_field_offset, _mult_field_width, \
			_stable_time, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset)

#define AW_CCU_PLL_ZERO_BASED_WITH_DEFAULT_STABLE_TIME(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset) \
		AW_CCU_PLL_ZERO_BASED(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			PLL_DEFAULT_STABLE_TIME, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset)

#define AW_CCU_PLL_ZERO_BASED_WITH_LOCK_STATE_REG(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_stable_time, \
			_enable_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_reg_offset, _lock_state_bit_offset) \
		AW_CCU_PLL_WITH_LOCK_STATE_REG_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_MULT_VAR_TYPE1, _mult_field_offset, _mult_field_width, \
			_stable_time, \
			_enable_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_reg_offset, _lock_state_bit_offset)

#define AW_CCU_PLL_ZERO_BASED_WITH_LOCK_STATE_REG_AND_DEFAULT_STABLE_TIME( \
			_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_enable_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_reg_offset, _lock_state_bit_offset) \
		AW_CCU_PLL_ZERO_BASED_WITH_LOCK_STATE_REG( \
			_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			PLL_DEFAULT_STABLE_TIME, \
			_enable_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_reg_offset, _lock_state_bit_offset)


#define AW_CCU_SSC_PLL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_stable_time, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset, \
			_sdm_enable_bit_offset, _ssc_cfg_reg_offset, \
			_ssc_enable_bit_offset, _ssc_cfg_map) \
		AW_CCU_SSC_PLL_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_MULT_VAR_TYPE0, _mult_field_offset, _mult_field_width, \
			_stable_time, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset, \
			_sdm_enable_bit_offset, _ssc_cfg_reg_offset, _ssc_cfg_reg_offset + 4, \
			_ssc_enable_bit_offset, 0, _ssc_cfg_map)

#define AW_CCU_SSC_PLL_WITH_DEFAULT_STABLE_TIME(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			_ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset, \
			_sdm_enable_bit_offset, _ssc_cfg_reg_offset, \
			_ssc_enable_bit_offset, _ssc_cfg_map) \
		AW_CCU_SSC_PLL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _mult_field_offset, _mult_field_width, \
			PLL_DEFAULT_STABLE_TIME, _ldo_ctrl_bit_offset, \
			_enable_bit_offset, _output_gate_bit_offset, \
			_lock_ctrl_bit_offset, _lock_state_bit_offset, \
			_sdm_enable_bit_offset, _ssc_cfg_reg_offset, \
			_ssc_enable_bit_offset, _ssc_cfg_map)


extern const struct clk_hw_ops g_ccu_pll_hw_ops;

int ccu_pll_init(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll);

int ccu_pll_set_enable_state(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll, int is_enabled);
int ccu_pll_get_enable_state(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll, int *is_enabled);

int ccu_pll_set_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll, uint32_t parent_freq, uint32_t freq);
int ccu_pll_get_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll, uint32_t parent_freq, uint32_t *freq);

#endif /* _CCU_PLL_H_ */
