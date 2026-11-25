#include "divoom.h"
#include "divoom_show_mode_ctrl.h"
#include "divoom_bt_music.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_menu_control.h"

#if DIVOOM_SHOW_MODE

typedef struct _divoom_show_mode_ctrl {
	uint32 pause_ms;
	uint32 disc_sec;
	uint8 a2dp_last;
	uint8 a2dp_last_valid;
	uint8 esc_fail;
	uint8 idle_checked;
} divoom_show_mode_ctrl;
static divoom_show_mode_ctrl s_show_mode_ctrl = {0};

void divoom_show_mode_ctrl_notice(void)
{
	s_show_mode_ctrl.pause_ms = divoom_system_get_tickms();
	if (s_show_mode_ctrl.pause_ms == 0)
		s_show_mode_ctrl.pause_ms = 1;
	s_show_mode_ctrl.disc_sec = 0;
	s_show_mode_ctrl.esc_fail = 0;
	s_show_mode_ctrl.idle_checked = 0;
}

static int divoom_show_mode_ctrl_idle_since(uint32 need_ms)
{
	if (s_show_mode_ctrl.pause_ms == 0)
		return 1;
	if ((divoom_system_get_tickms() - s_show_mode_ctrl.pause_ms) < need_ms)
		return 0;
	return 1;
}

int divoom_show_mode_ctrl_idle_ready(void)
{
	return divoom_show_mode_ctrl_idle_since(DIVOOM_SHOW_MODE_CTRL_IDLE_MS);
}

static void divoom_show_mode_ctrl_enter_tf(void)
{
	Pdivoom_sys_setting_info psys;
	uint8 vol;

	if (!divoom_music_ctrl_is_ready())
	{
		divoom_print("show_mode_ctrl: enter TF skip, music not ready\n");
		return;
	}
	/*
	 * 必须先写入本地音量再切模式。set_mode 起播时会把当时的
	 * music_volume_local 带进播放队列，sink 打开时按这个值出声。
	 * 起播后再 set_volume，会被这次打开盖掉，要等下一次音量键才生效。
	 */
	vol = divoom_com_convert_vol(DIVOOM_SHOW_MODE_CTRL_TF_VOL);
	psys = divoom_main_get_system_config();
	if (psys)
		psys->music_volume_local = vol;
	divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
	divoom_main_save_config();
	s_show_mode_ctrl.disc_sec = 0;
	divoom_print("show_mode_ctrl: enter TF vol=%d\n", DIVOOM_SHOW_MODE_CTRL_TF_VOL);
}

static void divoom_show_mode_ctrl_on_bt(int a2dp_on)
{
	if (a2dp_on)
	{
		s_show_mode_ctrl.disc_sec = 0;
		s_show_mode_ctrl.a2dp_last = 1;
		s_show_mode_ctrl.a2dp_last_valid = 1;
		return;
	}
	if (s_show_mode_ctrl.a2dp_last_valid && s_show_mode_ctrl.a2dp_last)
		s_show_mode_ctrl.disc_sec = 0;
	s_show_mode_ctrl.a2dp_last = 0;
	s_show_mode_ctrl.a2dp_last_valid = 1;
	s_show_mode_ctrl.disc_sec++;
	if (s_show_mode_ctrl.disc_sec >= DIVOOM_SHOW_MODE_CTRL_DISC_SEC)
	{
		s_show_mode_ctrl.disc_sec = 0;
		divoom_show_mode_ctrl_enter_tf();
	}
}

void divoom_show_mode_ctrl_tick(void)
{
	uint8 mode;
	int a2dp_on;

	if (divoom_lvgl_poweron_is_boot_pending())
		return;
	if (divoom_product_test_is_active())
		return;
	if (divoom_onoff_is_poweroff_active())
		return;
	if (!divoom_music_ctrl_is_ready())
		return;

	mode = divoom_music_ctrl_get_mode();
	if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		s_show_mode_ctrl.disc_sec = 0;
		return;
	}
	if (!divoom_show_mode_ctrl_idle_since(DIVOOM_SHOW_MODE_CTRL_TF_IDLE_MS))
	{
		s_show_mode_ctrl.disc_sec = 0;
		s_show_mode_ctrl.a2dp_last = divoom_bt_music_is_connected() ? 1 : 0;
		s_show_mode_ctrl.a2dp_last_valid = 1;
		return;
	}
	if (mode != DIVOOM_MUSIC_CTRL_MODE_BT)
		return;

	a2dp_on = divoom_bt_music_is_connected() ? 1 : 0;
	divoom_show_mode_ctrl_on_bt(a2dp_on);
}

int divoom_show_mode_ctrl_try_return_dial(void)
{
	int menu;
	int old_menu;

	menu = divoom_menu_control_get_active_menu_id();
	if (menu == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
	{
		s_show_mode_ctrl.esc_fail = 0;
		return 1;
	}
	if (s_show_mode_ctrl.idle_checked)
		return 0;
	s_show_mode_ctrl.idle_checked = 1;

	while (s_show_mode_ctrl.esc_fail < DIVOOM_SHOW_MODE_CTRL_ESC_MAX)
	{
		old_menu = menu;
		divoom_user_key_inject_key_info(DIVOOM_KEY_ESC, DIVOOM_PRESS_SHORT);
		divoom_system_delay_ms(DIVOOM_SHOW_MODE_CTRL_ESC_WAIT_MS);
		menu = divoom_menu_control_get_active_menu_id();
		if (menu == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
		{
			s_show_mode_ctrl.esc_fail = 0;
			divoom_print("show_mode_ctrl: ESC back to dial from %d\n", old_menu);
			return 1;
		}
		if (menu == old_menu)
		{
			s_show_mode_ctrl.esc_fail++;
			divoom_print("show_mode_ctrl: ESC invalid menu=%d fail=%u\n",
				menu, (unsigned)s_show_mode_ctrl.esc_fail);
		}
		else
		{
			s_show_mode_ctrl.esc_fail = 0;
			divoom_print("show_mode_ctrl: ESC %d -> %d\n", old_menu, menu);
		}
	}
	return 0;
}

#else

void divoom_show_mode_ctrl_notice(void)
{
}

int divoom_show_mode_ctrl_idle_ready(void)
{
	return 0;
}

void divoom_show_mode_ctrl_tick(void)
{
}

int divoom_show_mode_ctrl_try_return_dial(void)
{
	return 1;
}

#endif
