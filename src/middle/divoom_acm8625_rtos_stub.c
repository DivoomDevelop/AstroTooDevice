#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom_type.h"
#include "divoom_acm8625_app_init.h"
#include "divoom_acm8625p_amp.h"

int divoom_acm8625_init(void) { return 0; }
int divoom_acm8625_power_on(void) { return 0; }
int divoom_acm8625_power_off(void) { return 0; }
int divoom_acm8625_set_mode(uint8 mode) { (void)mode; return 0; }
int divoom_acm8625_set_volume(uint8 vol) { (void)vol; return 0; }
int divoom_acm8625_set_mute(uint8 mute) { (void)mute; return 0; }

int divoom_acm8625p_amp_init(void) { return 0; }
int divoom_acm8625p_amp_deinit(void) { return 0; }
int divoom_acm8625p_amp_power_on(void) { return 0; }
int divoom_acm8625p_amp_power_off(void) { return 0; }
int divoom_acm8625p_amp_write_reg(uint8 reg, uint8 val)
{
	(void)reg;
	(void)val;
	return 0;
}
int divoom_acm8625p_amp_read_reg(uint8 reg, uint8 *val)
{
	(void)reg;
	if (val)
		*val = 0;
	return 0;
}
int divoom_acm8625p_amp_set_i2c_addr(uint8 addr_7bit)
{
	(void)addr_7bit;
	return 0;
}
int divoom_acm8625p_amp_set_i2c_addr_by_adr(divoom_acm8625p_adr_cfg_t adr_cfg)
{
	(void)adr_cfg;
	return 0;
}
int divoom_acm8625p_amp_apply_cfg(const divoom_acm8625p_reg_cfg_t *cfg, uint32 cfg_num)
{
	(void)cfg;
	(void)cfg_num;
	return 0;
}
int divoom_acm8625p_amp_apply_default_cfg(void) { return 0; }
int divoom_acm8625p_amp_set_volume(uint8 vol) { (void)vol; return 0; }
int divoom_acm8625p_amp_set_volume_db(int db_x2) { (void)db_x2; return 0; }
int divoom_acm8625p_amp_set_eq_preset(divoom_acm8625p_eq_preset_t preset)
{
	(void)preset;
	return 0;
}
int divoom_acm8625p_amp_set_auto_eq_by_volume(uint8 enable)
{
	(void)enable;
	return 0;
}
int divoom_acm8625p_amp_set_mute(uint8 mute) { (void)mute; return 0; }
int divoom_acm8625p_amp_set_state(uint8 state) { (void)state; return 0; }
int divoom_acm8625p_amp_fault_clear(void) { return 0; }
int divoom_acm8625p_amp_get_fault(uint8 *fault1, uint8 *fault2, uint8 *fault3)
{
	if (fault1)
		*fault1 = 0;
	if (fault2)
		*fault2 = 0;
	if (fault3)
		*fault3 = 0;
	return 0;
}
int divoom_acm8625p_amp_fault_to_string(uint8 fault1, uint8 fault2, uint8 fault3,
					char *buf, uint32 buf_len)
{
	(void)fault1;
	(void)fault2;
	(void)fault3;
	if (buf && buf_len)
		buf[0] = '\0';
	return 0;
}
int divoom_acm8625p_amp_self_check(uint8 *die_id, uint8 *state, uint8 *fault1,
				   uint8 *fault2, uint8 *fault3)
{
	if (die_id)
		*die_id = 0;
	if (state)
		*state = 0;
	if (fault1)
		*fault1 = 0;
	if (fault2)
		*fault2 = 0;
	if (fault3)
		*fault3 = 0;
	return 0;
}
int divoom_acm8625p_amp_dump_key_regs(void) { return 0; }
int divoom_acm8625p_amp_monitor_once_loop(uint32 duration_ms, uint32 interval_ms)
{
	(void)duration_ms;
	(void)interval_ms;
	return 0;
}
int divoom_acm8625p_amp_start_play(void) { return 0; }
