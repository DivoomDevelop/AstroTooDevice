#ifndef _DIVOOM_ACM8625P_AMP_H_
#define _DIVOOM_ACM8625P_AMP_H_

#include "divoom_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACM8625P_AMP_I2C_DEV                  "/dev/i2c-2"
/*
 * 规格书 Table 3 给出的是 8bit 写地址: 0x58/0x5A/0x5C/0x5E
 * Linux I2C_SLAVE 使用 7bit 地址，所以默认地址为 0x58 >> 1 = 0x2C。
 */
#define ACM8625P_AMP_I2C_ADDR_7BIT_4K7        0x2C
#define ACM8625P_AMP_I2C_ADDR_7BIT_15K        0x2D
#define ACM8625P_AMP_I2C_ADDR_7BIT_47K        0x2E
#define ACM8625P_AMP_I2C_ADDR_7BIT_120K       0x2F
#define ACM8625P_AMP_I2C_ADDR_7BIT_DEFAULT    ACM8625P_AMP_I2C_ADDR_7BIT_15K

/* Page0 register map */
#define ACM8625P_REG_AMP_CTRL1                0x01
#define ACM8625P_REG_AMP_CTRL2                0x02
#define ACM8625P_REG_AMP_CTRL3                0x03
#define ACM8625P_REG_STATE_CTRL               0x04
#define ACM8625P_REG_PROCESSING_CTRL1         0x05
#define ACM8625P_REG_PROCESSING_CTRL2         0x06
#define ACM8625P_REG_I2S_DATA_FORMAT1         0x07
#define ACM8625P_REG_I2S_DATA_FORMAT2         0x08
#define ACM8625P_REG_GPIO2_CTRL               0x0A
#define ACM8625P_REG_GPIO1_CTRL               0x0B
#define ACM8625P_REG_GPIO1_FAULT_SEL          0x0C
#define ACM8625P_REG_GPIO2_FAULT_SEL          0x0D
#define ACM8625P_REG_SS_CTRL                  0x0E
#define ACM8625P_REG_VOLUME_CTRL_L            0x0F
#define ACM8625P_REG_VOLUME_CTRL_R            0x10
#define ACM8625P_REG_MISC_CTRL                0x11
#define ACM8625P_REG_DIEID_RPT                0x15
#define ACM8625P_REG_STATE_RPT                0x16
#define ACM8625P_REG_FAULT_RPT1               0x17
#define ACM8625P_REG_FAULT_RPT2               0x18
#define ACM8625P_REG_FAULT_RPT3               0x19

/* STATE_CTRL[1:0] */
#define ACM8625P_STATE_DIGITAL_OFF            0x00
#define ACM8625P_STATE_ANALOG_OFF             0x01
#define ACM8625P_STATE_DRIVER_OFF             0x02
#define ACM8625P_STATE_PLAY                   0x03

/* volume register range: 0x00=-104dB ... 0xD0=0dB ... 0xFF=+24dB */
#define ACM8625P_VOL_REG_MIN                  0x00
#define ACM8625P_VOL_REG_0DB                  0xD0
#define ACM8625P_VOL_REG_MAX                  0xFF

#define ACM8625P_MUTE_OFF                     0
#define ACM8625P_MUTE_ON                      1

/* T113 GPIO: PE8/PE9 */
#define ACM8625P_AMP_GPIO_PDN_PE8         136
#define ACM8625P_AMP_GPIO_POWER_PE9       137

typedef struct {
    uint8 reg;
    uint8 val;
    uint16 delay_ms;
} divoom_acm8625p_reg_cfg_t;

typedef enum {
    ACM8625P_ADR_PULL_4K7 = 0,
    ACM8625P_ADR_PULL_15K,
    ACM8625P_ADR_PULL_47K,
    ACM8625P_ADR_PULL_120K,
} divoom_acm8625p_adr_cfg_t;

typedef enum {
    ACM8625P_FAULT_LEVEL_INFO = 0,
    ACM8625P_FAULT_LEVEL_WARN,
    ACM8625P_FAULT_LEVEL_FATAL,
} divoom_acm8625p_fault_level_t;

#define DIVOOM_ACM8625P_EQ_PRESET_T_DEFINED
typedef enum {
    ACM8625P_EQ_PRESET_FLAT = 0,
    ACM8625P_EQ_PRESET_BYPASS,
} divoom_acm8625p_eq_preset_t;

int divoom_acm8625p_amp_power_on(void);
int divoom_acm8625p_amp_power_off(void);

int divoom_acm8625p_amp_write_reg(uint8 reg, uint8 val);
int divoom_acm8625p_amp_read_reg(uint8 reg, uint8 *val);
int divoom_acm8625p_amp_set_i2c_addr(uint8 addr_7bit);
int divoom_acm8625p_amp_set_i2c_addr_by_adr(divoom_acm8625p_adr_cfg_t adr_cfg);

int divoom_acm8625p_amp_apply_cfg(const divoom_acm8625p_reg_cfg_t *cfg, uint32 cfg_num);
int divoom_acm8625p_amp_apply_default_cfg(void);

/* 直接设置寄存器值，范围 0x00~0xFF */
int divoom_acm8625p_amp_set_volume(uint8 vol_reg);
int divoom_acm8625p_amp_set_volume_db(int db_x2);
int divoom_acm8625p_amp_set_eq_preset(divoom_acm8625p_eq_preset_t preset);
int divoom_acm8625p_amp_set_auto_eq_by_volume(uint8 enable);
int divoom_acm8625p_amp_set_mute(uint8 mute);
int divoom_acm8625p_amp_set_state(uint8 state);
int divoom_acm8625p_amp_fault_clear(void);
int divoom_acm8625p_amp_get_fault(uint8 *fault1, uint8 *fault2, uint8 *fault3);
int divoom_acm8625p_amp_fault_to_string(uint8 fault1, uint8 fault2, uint8 fault3, char *buf, uint32 buf_len);
divoom_acm8625p_fault_level_t divoom_acm8625p_amp_get_fault_level(uint8 fault1, uint8 fault2, uint8 fault3);
const char *divoom_acm8625p_amp_fault_level_to_string(divoom_acm8625p_fault_level_t level);
int divoom_acm8625p_amp_self_check(uint8 *die_id, uint8 *state, uint8 *fault1, uint8 *fault2, uint8 *fault3);
int divoom_acm8625p_amp_dump_key_regs(void);
int divoom_acm8625p_amp_monitor_once_loop(uint32 duration_ms, uint32 interval_ms);
int divoom_acm8625p_amp_start_play(void);

int divoom_acm8625p_amp_init(void);
int divoom_acm8625p_amp_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_ACM8625P_AMP_H_ */
