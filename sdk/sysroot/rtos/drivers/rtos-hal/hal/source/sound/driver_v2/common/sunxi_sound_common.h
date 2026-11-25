/*
 * sound\snd_sunxi_rxsync.h
 * (C) Copyright 2025-2029
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * zhouxijing <zhouxijing@allwinnertech.com>
 * xudongpdc <xudongpdc@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SUNXI_SOUND_COMMON_H
#define __SUNXI_SOUND_COMMON_H

#include <hal_gpio.h>
#include <hal_workqueue.h>

/******* reg config *******/
#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct audio_reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};
/* EX:
 * static struct audio_reg_label reg_labels[] = {
 * 	REG_LABEL(SUNXI_REG_0),
 * 	REG_LABEL(SUNXI_REG_1),
 * 	REG_LABEL(SUNXI_REG_n),
 * 	REG_LABEL_END,
 * };
 */
int sunxi_sound_save_reg(struct audio_reg_label *reg_labels, void *data,
		       unsigned int (*snd_read_func)(void *data, unsigned int reg));
int sunxi_sound_echo_reg(struct audio_reg_label *reg_labels, void *data,
		       void (*snd_write_func)(void *data, unsigned int reg, unsigned int val));

/******* pinctrl config *******/
struct sunxi_sound_pinctrl {
	gpio_pin_t gpio_pin;
	gpio_muxsel_t mux;
	gpio_driving_level_t level;
	gpio_data_t data;
};

struct sunxi_sound_pinctrl *sunxi_sound_pinctrl_init(char *comp_name,
													 char *prefix,
													 unsigned int *pinctrl_num);
void sunxi_sound_pinctrl_exit(struct sunxi_sound_pinctrl *pinctrl);
int sunxi_sound_pin_state_set(char *comp_name, struct sunxi_sound_pinctrl *pinctrl,
							  unsigned int pinctrl_num, bool enable);

/******* ucfmt config *******/
struct sound_sunxi_dai_fmt {
	uint32_t fmt;
    uint32_t slots;
	uint32_t slot_width;

	bool tx_lsb_first;
	bool rx_lsb_first;
};

enum SUNXI_PACFG_MODE {
	SND_SUNXI_PA_CFG_NULL = -1,
	SND_SUNXI_PA_CFG_LEVEL = 0,
	SND_SUNXI_PA_CFG_PULSE,
	SND_SUNXI_PA_CFG_USER,
	SND_SUNXI_PA_CFG_CNT,
};

struct pacfg_level_trig {
	struct sunxi_sound_pinctrl pin;
	uint32_t msleep_0;
	uint32_t msleep_1;
};

struct pacfg_pulse_trig {
	struct sunxi_sound_pinctrl pin;
	uint32_t msleep_0;
	uint32_t msleep_1;

	uint32_t duty_us;
	uint32_t period_us;
	uint32_t period_cnt;
	bool polarity;
};

#define DEFAUT_PIN_MAX 			0
#define DEFAULT_PACFG_MODE 		0

struct pacfg_user_trig {
	uint32_t user_control;
};

struct sunxi_pacfg {
	struct pacfg_level_trig *level_trig;
	struct pacfg_pulse_trig *pulse_trig;
	struct pacfg_user_trig *user_trig;
	enum SUNXI_PACFG_MODE mode;
	bool used;
	uint32_t index;
	hal_work pa_en_work;
	hal_work pa_disa_work;

	hal_spinlock_t spinlock;

	void *param;
	void (*pa_disa_cb)(void *data);
};

struct sunxi_pacfg *sunxi_pa_pin_init(char *node, uint32_t *pa_pin_max);
void sunxi_pa_pin_exit(struct sunxi_pacfg *pa_cfg, uint32_t pa_pin_max);
int sunxi_pa_pin_enable(struct sunxi_pacfg *pa_cfg, uint32_t pa_pin_max);
void sunxi_pa_pin_disable(struct sunxi_pacfg *pa_cfg, uint32_t pa_pin_max);
void sunxi_pa_pin_disable_irp(struct sunxi_pacfg *pa_cfg, uint32_t pa_pin_max, void *param,
				  void (*pa_disa_cb)(void *data));

/******* misc config *******/
struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

/******* component load func *******/
#ifdef CONFIG_KERNEL_FREERTOS
struct sound_module {
	const char *name;
	int (*dev_probe)(void);
	void (*dev_remove)(void);
};

#define SECTION_USED(x)  __attribute__((used,section(x)))

#define REGISTER_MODULE(module) \
    const struct sound_module* sunxi_sound_##module SECTION_USED("sunxi_sound_module") = &(module)

void sunxi_sound_card_register_component(void);
void sunxi_sound_card_unregister_component(void);
#endif

#endif /* __SUNXI_SOUND_COMMON_H */