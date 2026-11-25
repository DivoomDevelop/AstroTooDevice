#include "divoom_build_config.h"
#include "divoom_jl_amp_adapter.h"
#include "divoom_print.h"
#include "divoom_system.h"
#include "divoom_system_task_conf.h"

#include <stdint.h>
#include <stdio.h>

#if DIVOOM_BUILD_RTOS_DEVICE
/* 不拉 divoom.h / 产品宏；仅声明 UART 所需符号 */
int divoom_btcore_uart_cmd_is_ready(void);
int divoom_btcore_uart_cmd_send(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len);
void divoom_main_set_jl_mcu_tone_volume_tag(int vol_0_100, const char *caller);
#define divoom_main_set_jl_mcu_tone_volume(vol) \
	divoom_main_set_jl_mcu_tone_volume_tag((vol), __func__)
#ifndef DM_BTCORE_MEDIA_VOLUME
#define DM_BTCORE_MEDIA_VOLUME ((uint8_t)0x6Cu)
#endif
#ifndef DM_BTCORE_CALL_VOLUME
#define DM_BTCORE_CALL_VOLUME ((uint8_t)0x6Eu)
#endif
#ifndef DIVOOM_BTCORE_MEDIA_VOL_OP_SET_MUTE
#define DIVOOM_BTCORE_MEDIA_VOL_OP_SET_MUTE  2
#endif

#define DIVOOM_JL_AMP_ADAPTER_ON_LEVEL   100
#define DIVOOM_JL_AMP_ADAPTER_SETTLE_MS  40
/* ~300ms 走完 0↔16（17 档 × ~18ms） */
#define DIVOOM_JL_MEDIA_VOL_STEP_MS      18

static uint8_t s_jl_primed;

static void *s_media_lock;
static int s_media_level = DIVOOM_JL_MEDIA_VOL_MAX;
static int s_media_target = DIVOOM_JL_MEDIA_VOL_MAX;
static uint8_t s_media_ramp_running;

static void divoom_jl_media_vol_ensure_lock(void)
{
	if (!s_media_lock)
	{
		s_media_lock = divoom_system_create_lock();
	}
}

static void divoom_jl_media_volume_send_unlocked(int level_0_16)
{
	uint8_t payload[2];

	if (level_0_16 < 0)
	{
		level_0_16 = 0;
	}
	else if (level_0_16 > DIVOOM_JL_MEDIA_VOL_MAX)
	{
		level_0_16 = DIVOOM_JL_MEDIA_VOL_MAX;
	}

	/* 仅 MEDIA_VOLUME：音乐总开关；勿发 MUSIC_VOLUME */
	payload[0] = 1; /* set */
	payload[1] = (uint8_t)level_0_16;
	if (divoom_btcore_uart_cmd_is_ready())
	{
		if (divoom_btcore_uart_cmd_send(DM_BTCORE_MEDIA_VOLUME, payload, 2) != 0)
		{
			divoom_print("media_vol: MEDIA_VOLUME send fail level=%d\n",
				     level_0_16);
		}
	}
	else
	{
		divoom_print("media_vol: btcore not ready level=%d\n", level_0_16);
	}
	s_media_level = level_0_16;
	divoom_print("media_duck: level=%d\n", level_0_16);
}

void divoom_jl_amp_adapter_prime_if_needed(void)
{
	if (s_jl_primed)
	{
		return;
	}
	divoom_main_set_jl_mcu_tone_volume(DIVOOM_JL_AMP_ADAPTER_ON_LEVEL);
	divoom_system_delay_ms(DIVOOM_JL_AMP_ADAPTER_SETTLE_MS);
	s_jl_primed = 1;
	divoom_print("jl_amp: primed level=%d (fixed)\n", DIVOOM_JL_AMP_ADAPTER_ON_LEVEL);
}

void divoom_jl_amp_adapter_on_session_active(void)
{
	divoom_jl_amp_adapter_prime_if_needed();
}

void divoom_jl_amp_adapter_on_session_idle(int keep_open_for_movie)
{
	(void)keep_open_for_movie;
	/* I2S 常开保活：不再 UART 拉 JL TONE 到 0，避免台阶啪 */
}

void divoom_jl_media_volume_set(int level_0_16)
{
	divoom_jl_media_vol_ensure_lock();
	divoom_system_get_lock(s_media_lock);
	s_media_target = level_0_16;
	divoom_jl_media_volume_send_unlocked(level_0_16);
	divoom_system_free_lock(s_media_lock);
}

static void *divoom_jl_media_volume_ramp_task(void *arg)
{
	(void)arg;

	divoom_jl_media_vol_ensure_lock();
	for (;;)
	{
		int cur;
		int tgt;
		int next;

		divoom_system_get_lock(s_media_lock);
		cur = s_media_level;
		tgt = s_media_target;
		if (cur == tgt)
		{
			s_media_ramp_running = 0;
			divoom_system_free_lock(s_media_lock);
			break;
		}
		next = (cur < tgt) ? (cur + 1) : (cur - 1);
		divoom_jl_media_volume_send_unlocked(next);
		divoom_system_free_lock(s_media_lock);
		divoom_system_delay_ms(DIVOOM_JL_MEDIA_VOL_STEP_MS);
	}
	return NULL;
}

void divoom_jl_media_volume_ramp_to(int level_0_16)
{
	if (level_0_16 < 0)
	{
		level_0_16 = 0;
	}
	else if (level_0_16 > DIVOOM_JL_MEDIA_VOL_MAX)
	{
		level_0_16 = DIVOOM_JL_MEDIA_VOL_MAX;
	}

	divoom_jl_media_vol_ensure_lock();
	divoom_system_get_lock(s_media_lock);
	s_media_target = level_0_16;
	if (s_media_level == s_media_target)
	{
		divoom_system_free_lock(s_media_lock);
		return;
	}
	if (s_media_ramp_running)
	{
		/* 已有 ramp：只改 target，任务会掉头 */
		divoom_system_free_lock(s_media_lock);
		return;
	}
	s_media_ramp_running = 1;
	divoom_system_free_lock(s_media_lock);
	divoom_system_create_task(divoom_jl_media_volume_ramp_task, "media_vol",
				  NULL, DIVOOM_TASK_STACK_MEDIA_VOL_RAMP);
}

int divoom_jl_media_volume_get_current(void)
{
	int level;

	divoom_jl_media_vol_ensure_lock();
	divoom_system_get_lock(s_media_lock);
	level = s_media_level;
	divoom_system_free_lock(s_media_lock);
	return level;
}

static void divoom_jl_audio_gate_mute_send(uint8_t cmd, const char *tag, int mute)
{
	uint8_t payload[2];

	payload[0] = (uint8_t)DIVOOM_BTCORE_MEDIA_VOL_OP_SET_MUTE;
	payload[1] = mute ? 1u : 0u;
	if (divoom_btcore_uart_cmd_is_ready())
	{
		if (divoom_btcore_uart_cmd_send(cmd, payload, 2) != 0)
		{
			divoom_print("%s: SET_MUTE send fail mute=%d\n", tag, mute);
		}
	}
	else
	{
		divoom_print("%s: btcore not ready mute=%d\n", tag, mute);
	}
}

void divoom_jl_audio_gates_mute_set(int mute)
{
	divoom_jl_audio_gate_mute_send(DM_BTCORE_MEDIA_VOLUME, "media_mute", mute);
	divoom_jl_audio_gate_mute_send(DM_BTCORE_CALL_VOLUME, "call_mute", mute);
	divoom_print("audio_gates: mute=%d\n", mute ? 1 : 0);
}

void divoom_jl_release_boot_audio_gates_once(void)
{
	static uint8_t s_boot_gates_released = 0;

	if (s_boot_gates_released)
		return;
	s_boot_gates_released = 1;
	divoom_jl_audio_gates_mute_set(0);
}

#else /* simulator */

void divoom_jl_amp_adapter_prime_if_needed(void) {}
void divoom_jl_amp_adapter_on_session_active(void) {}
void divoom_jl_amp_adapter_on_session_idle(int keep_open_for_movie)
{
	(void)keep_open_for_movie;
}

void divoom_jl_media_volume_set(int level_0_16)
{
	(void)level_0_16;
}

void divoom_jl_media_volume_ramp_to(int level_0_16)
{
	(void)level_0_16;
}

int divoom_jl_media_volume_get_current(void)
{
	return DIVOOM_JL_MEDIA_VOL_MAX;
}

void divoom_jl_audio_gates_mute_set(int mute)
{
	(void)mute;
}

void divoom_jl_release_boot_audio_gates_once(void)
{
}

#endif
