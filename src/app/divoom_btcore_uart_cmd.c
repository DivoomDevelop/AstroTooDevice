#include "divoom.h"
#include "divoom_ha.h"
#include "divoom_ai_voice_dial_test.h"
#include "divoom_print.h"

#ifdef DIVOOM_TIMES_VIBE
#include "btcore_uart.h"
#include "divoom_ble_accept_data.h"
#include "divoom_btcore_file_rx.h"
#include "divoom_btcore_file_tx.h"
#include "divoom_btcore_rec_stream.h"
#include "divoom_ota_jl.h"
#include "divoom_com.h"
#include "divoom_btcore_key.h"
#include "divoom_bt_bip.h"
#include "divoom_music_ctrl.h"
#include "divoom_product_test.h"
#include "divoom_onoff.h"
#include "divoom_tws_host.h"
#include "divoom_lvgl_device_update.h"
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
#include "divoom_astrootoo_source.h"
#include "divoom_lvgl_top_radio_wait.h"
#include "divoom_lvgl_top_bright_volume_menu.h"
#include "divoom_lvgl_top_phone_call_menu.h"
#endif
#include <stddef.h>
#include <string.h>
#include <time.h>

static uint8_t s_noise_meter_db;
static uint32_t s_noise_meter_last_rx_ms;

static uint8_t s_host_boot_cnt = 0; /* 0=尚未收到 CHECK_LINK 应答；有效值从 1 起 */

static void *s_btcore_cmd_reply_sem;
static void *s_btcore_cmd_uart_lock;
static uint8_t s_btcore_cmd_reply_expect;
static int s_btcore_cmd_reply_waiting;
static uint8_t s_btcore_reply_buf[BTCORE_MAX_CMD_DATA_LEN];
static uint16_t s_btcore_reply_saved_len;
static volatile int s_btcore_rx_suppressed_after_power_off;
static volatile uint8_t s_btcore_poweroff_pending;
static uint8_t s_aic_host_audio_ready_sent;

static uint16_t divoom_btcore_get_le16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/* JL→AIC：DM_BTCORE_GET_FFT_FREQ_DATA 应答，data_size(2) + 频谱数据 */
static void divoom_btcore_uart_handle_fft_freq_data(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint16_t npoints;
	uint16_t payload_len;

	if (!cmd_data || cmd_len < 2u)
	{
		divoom_spectrum_update_from_jl(NULL, 0);
		return;
	}
	npoints = divoom_btcore_get_le16(cmd_data);
	payload_len = (uint16_t)(cmd_len - 2u);
	if (payload_len < npoints)
		npoints = payload_len;
	divoom_spectrum_update_from_jl(cmd_data + 2u, (int)npoints);
}

void divoom_btcore_uart_cmd_set_fft_freq_stream(uint8 enable)
{
	(void)enable;
}

static void divoom_btcore_uart_handle_mic_noise_meter(const uint8_t *cmd_data, uint16_t cmd_len)
{
	if (!cmd_data || cmd_len < 1u)
		return;

	s_noise_meter_db = cmd_data[0];
	if (s_noise_meter_db > 99u)
		s_noise_meter_db = 99u;
	s_noise_meter_last_rx_ms = divoom_system_get_tickms();
}

void divoom_btcore_uart_cmd_set_noise_meter_stream(uint8 enable)
{
	(void)enable;
}

/* 0x65 等纯 mic 消费：DisableMic 即屏蔽（产测 get_mic_switch 恒 0） */
static int divoom_btcore_mic_consumer_disabled(void)
{
	return divoom_main_get_mic_switch() != 0;
}

/*
 * 0x3b FFT：JL 有 music_pcm 时走解码器 PCM，无音乐时才走 mic。
 * F101 用 music_ctrl playing 粗对齐；播放中不因 DisableMic 关 EQ。
 */
static int divoom_btcore_mic_fft_consumer_disabled(void)
{
	if (!divoom_btcore_mic_consumer_disabled())
		return 0;
	if (divoom_music_ctrl_is_playing())
		return 0;
	return 1;
}

int divoom_btcore_uart_cmd_mic_fft_suppressed(void)
{
	return divoom_btcore_mic_fft_consumer_disabled();
}

int divoom_btcore_uart_cmd_get_mic_noise_db(void)
{
	uint32_t now;
	uint32_t elapsed;

	if (divoom_btcore_mic_consumer_disabled())
		return 0;

	if (s_noise_meter_last_rx_ms == 0u)
		return 0;

	now = divoom_system_get_tickms();
	elapsed = now - s_noise_meter_last_rx_ms;
	if (elapsed > (uint32_t)DIVOOM_BTCORE_MIC_NOISE_UART_SILENCE_MS)
		return 0;
	return (int)s_noise_meter_db;
}

/*
 * 电源键 0x4c：不映射 kbPOWER（main 里 LONG 为 legacy 关机，关机走 0x19）。
 * 仅短按切换关屏/亮屏（SCREEN_CTRL 同时关/开环境光与按键灯）；丢弃 LONG。
 */
static void divoom_btcore_uart_handle_power_key(uint8_t btcore_action)
{
	int *screen_on;
	divoom_sys_msg_info model_msg = {0};

	if (btcore_action == DIVOOM_BTCORE_KEY_ACTION_LONG)
		return;
	if (btcore_action != DIVOOM_BTCORE_KEY_ACTION_CLICK)
		return;

#ifdef DIVOOM_TIME_FRAME_SPEAKER
	/* RTC 闹钟唤醒黑屏：短按电源先脱黑屏，勿与关屏切换叠加（原 T113 handler 逻辑） */
	if (divoom_main_try_dismiss_clock_black_screen())
		return;
#endif

	screen_on = (int *)divoom_system_malloc(sizeof(int));
	if (!screen_on)
		return;
	*screen_on = divoom_light_app_get_screen_status(0xFF) ? 0 : 1;

	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_MAIN;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
	model_msg.data = (uint8 *)screen_on;
	model_msg.data_len = (uint16)sizeof(int);
	divoom_sys_model_crl_msg_push(&model_msg);
}

/* JL→F101：DM_BTCORE_SEND_KEY_SYSTEM_MSG，payload [msg.value, msg.event] */
static void divoom_btcore_uart_handle_key_system_msg(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint8_t key_value;
	uint8_t key_action;
	uint8_t ui_key;
	uint16_t ui_press;
	int screen_off, ha_body;

	if (!cmd_data || cmd_len < 2u)
		return;

	key_value = cmd_data[0];
	key_action = cmd_data[1];
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
    /* 在电源、语音与导航分发前通知浮层；原按键仍走原有分支。 */
    if (key_action == DIVOOM_BTCORE_KEY_ACTION_CLICK || key_action == DIVOOM_BTCORE_KEY_ACTION_LONG ||
        key_action == DIVOOM_BTCORE_KEY_ACTION_DOUBLE_CLICK)
    {
        divoom_lvgl_top_radio_wait_dismiss_on_key();
    }
#endif

	if (key_value == DIVOOM_BTCORE_HW_KEY_POWER) {
		divoom_btcore_uart_handle_power_key(key_action);
		return;
	}

	ui_key = divoom_btcore_key_map(key_value, key_action);
	if (ui_key == 0xFFu)
		return;

	/* 关屏：电源短按仍开关屏；音量±/PP/上一曲下一曲进 main；导航键丢弃 */
	screen_off = (divoom_light_app_get_screen_status(0xFF) == 0);
    ha_body = !screen_off &&
        (ui_key == DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_SUB || ui_key == DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_ADD ||
         ui_key == DIVOOM_KEY_ASTROTOO_KEY_MUSIC_PLAY) && divoom_ha_clock_control_enabled();
    if (!ha_body && (ui_key == DIVOOM_KEY_ASTROTOO_KEY_MUSIC_PLAY || ui_key == DIVOOM_KEY_ASTROTOO_KEY_ENTER) &&
        (!screen_off || key_action == DIVOOM_BTCORE_KEY_ACTION_UP))
    {
        /* 松开先到语音状态机，即使此时页面已退出也要清掉旧手势。 */
        if (divoom_ai_voice_dial_test_device_key(key_action))
        {
            return;
        }
    }
	if (screen_off && !divoom_btcore_key_allowed_when_screen_off(ui_key))
		return;

	ui_press = divoom_btcore_press_map(key_action);
    if((ui_key == DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_SUB || ui_key == DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_ADD) &&
        ui_press != DIVOOM_PRESSING)
        divoom_print("ha key: uart hw=%u raw_action=%u key=%u press=%u to_ha=%d screen_off=%d\n",
            (unsigned)key_value, (unsigned)key_action, (unsigned)ui_key, (unsigned)ui_press, ha_body, screen_off);
	if (ui_press == DIVOOM_NO_PRESS)
		return;

    /* 电台拥有播放/收藏键；在UART发布点确定归属，不在MAIN延迟消费时猜页面。 */
    int32 radio_owned = 0;
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
    radio_owned = !screen_off && (ui_key == DIVOOM_KEY_MUSIC_PLAY ||
        ui_key == DIVOOM_KEY_MUSIC_FAVORITE) && divoom_astrootoo_source_net_radio_is_open();
#endif
	if (radio_owned || ha_body || (!screen_off && divoom_btcore_key_is_lvgl_nav(ui_key)))
		divoom_user_key_send_key_info(ui_key, ui_press);
	else
		divoom_main_keypad_send_key_info(ui_key, ui_press);
}

static int divoom_btcore_uart_take_sync_reply(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint16_t copy_n;

	if (!s_btcore_cmd_reply_sem || !s_btcore_cmd_reply_waiting || cmd != s_btcore_cmd_reply_expect)
		return 0;

	s_btcore_cmd_reply_waiting = 0;
	s_btcore_reply_saved_len = cmd_len;
	if (cmd_len && cmd_data)
	{
		copy_n = cmd_len;
		if (copy_n > BTCORE_MAX_CMD_DATA_LEN)
			copy_n = BTCORE_MAX_CMD_DATA_LEN;
		memcpy(s_btcore_reply_buf, cmd_data, copy_n);
	}
	divoom_system_put_sem(s_btcore_cmd_reply_sem);
	return 1;
}

int divoom_btcore_uart_cmd_try_take_sync_reply(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	return divoom_btcore_uart_take_sync_reply(cmd, cmd_data, cmd_len);
}

/*
 * 执行上下文声明表：本函数是「哪个命令在哪个线程跑」的唯一真源。
 * middle 层查表投递，不再自带一份 cmd 列表；新增命令只改这里与对应 handler。
 */
divoom_btcore_ctx_t divoom_btcore_uart_cmd_ctx(uint8_t cmd)
{
	switch (cmd) {
	/*
	 * 专辑封面分片。几十包后就停，放普通命令线程拼文件。
	 * 不进录音流，也不进 TF 文件那条慢队列。
	 */
	case DM_BTCORE_BT_BIP_DATA:
		return DIVOOM_BTCORE_CTX_WORK;

	case DM_BTCORE_REC_STREAM_DATA:
	case DM_BTCORE_REC_STREAM_END:
		return DIVOOM_BTCORE_CTX_STREAM;

	/* 均为纯定长变量写入，满足 CTX_FAST 契约，见 rx_handle_inline */
	case DM_BTCORE_GET_FFT_FREQ_DATA:
	case DM_BTCORE_MIC_NOISE_METER:
	case DM_BTCORE_BAT_VOLT_AVR:
	case DM_BTCORE_BAT_VOLT_LEVE:
	case DM_BTCORE_SD_PLAY_INFO:
		return DIVOOM_BTCORE_CTX_FAST;

	/*
	 * 落盘与 sqlite 写入，单次可占用很久。这一族必须整体同属一个上下文：
	 * 文件传输与歌单同步最终都改 tf_music 状态，而 tf_music / file_rx 都没有内部锁，
	 * 靠「UART 侧只有一个线程访问」这一约定；同时 HEAD→BLOCK→END 有顺序依赖，
	 * 跨队列会失序。新增碰 tf_music 或 file_rx 的命令请一并归到 CTX_SLOW。
	 */
	case DM_BTCORE_FILE_HEAD_TRANSMIT:
	case DM_BTCORE_FILE_BLOCK_TRANSMIT:
	case DM_BTCORE_FILE_END:
	case DM_BTCORE_FILE_MISSED_PACKETS:
	case DM_BTCORE_SONGLIST_INFO:
	case DM_BTCORE_SCAN_MUSIC_LIST:
	case DM_BTCORE_SD_TOTAL_MUSIC:
	case DM_BTCORE_SD_MUSIC_LIST:
	case DM_BTCORE_SD_MUSIC_LIST_ITEM:
	/*
	 * 这两条不落盘，但都会读写 pdivoom_tf_music_handle 的歌单队列
	 * （0x5e 经 jl_tf_ready_and_play 走播放路径，0x5f 遍历队列取下一首），
	 * 与上面的 sync 系列同属一份无锁状态，必须同线程串行。
	 */
	case DM_BTCORE_SD_MUSIC_NOTIFY_FINISHED:
	case DM_BTCORE_SD_MUSIC_READY:
		return DIVOOM_BTCORE_CTX_SLOW;

	default:
		return DIVOOM_BTCORE_CTX_WORK;
	}
}

void divoom_btcore_uart_cmd_rx_handle_inline(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	if (s_btcore_rx_suppressed_after_power_off)
		return;

	switch (cmd) {
	case DM_BTCORE_GET_FFT_FREQ_DATA:
		divoom_btcore_uart_handle_fft_freq_data(cmd_data, cmd_len);
		break;
	case DM_BTCORE_MIC_NOISE_METER:
		divoom_btcore_uart_handle_mic_noise_meter(cmd_data, cmd_len);
		break;
	case DM_BTCORE_BAT_VOLT_AVR:
		if (cmd_len >= 4u) {
			uint16_t bat_mv = divoom_btcore_get_le16(cmd_data);
			uint16_t usb_mv = divoom_btcore_get_le16(cmd_data + 2u);

			divoom_power_on_jl_bat_volt_avr(bat_mv, usb_mv);
		}
		break;
	case DM_BTCORE_BAT_VOLT_LEVE:
		if (cmd_len >= 1u)
			divoom_power_set_level(cmd_data[0]);
		break;
	case DM_BTCORE_SD_PLAY_INFO:
		if (cmd_len >= 10u) {
			uint16_t cur_time = divoom_btcore_get_le16(cmd_data);
			uint16_t totle_time = divoom_btcore_get_le16(cmd_data + 2u);

			divoom_music_ctrl_on_jl_sd_play_info(cur_time, totle_time, cmd_data[9]);
		}
		break;
	default:
		break;
	}
}

static void divoom_btcore_uart_handle_phone_call_alert(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint8_t action;
	uint8_t call_type;
	uint8_t flags;
	uint16_t off;
	uint8_t number_len;
	uint8_t name_len;
	const char *number;
	const char *name;

	if (cmd_data == NULL || cmd_len < 1u)
		return;

	action = cmd_data[0];
	if (action == DIVOOM_BTCORE_PHONE_ALERT_ACTION_DISMISS)
	{
		/* 仿真暂未接入来电页面，仍解析协议；真机照常通知 UI。 */
#if defined(DIVOOM_PRODUCT_ASTRO_TOO) && (!defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR))
		divoom_lvgl_top_phone_call_menu_dismiss();
#endif
		return;
	}
	if (action != DIVOOM_BTCORE_PHONE_ALERT_ACTION_SHOW)
		return;
	if (cmd_len < DIVOOM_BTCORE_PHONE_ALERT_SHOW_HDR_LEN)
		return;

	call_type = cmd_data[1];
	flags = cmd_data[2];
	off = DIVOOM_BTCORE_PHONE_ALERT_SHOW_HDR_LEN;
	number = "";
	number_len = 0u;
	name = "";
	name_len = 0u;

	if (flags & DIVOOM_BTCORE_PHONE_ALERT_FLAG_HAS_NUMBER)
	{
		if (off >= cmd_len)
			return;
		number_len = cmd_data[off++];
		if (number_len > DIVOOM_BTCORE_PHONE_ALERT_NUMBER_MAX)
			return;
		if ((uint16_t)(off + number_len) > cmd_len)
			return;
		number = (const char *)(cmd_data + off);
		off += number_len;
	}
	if (flags & DIVOOM_BTCORE_PHONE_ALERT_FLAG_HAS_NAME)
	{
		if (off >= cmd_len)
			return;
		name_len = cmd_data[off++];
		if (name_len > DIVOOM_BTCORE_PHONE_ALERT_NAME_MAX)
			return;
		if ((uint16_t)(off + name_len) > cmd_len)
			return;
		name = (const char *)(cmd_data + off);
	}

#if defined(DIVOOM_PRODUCT_ASTRO_TOO) && (!defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR))
	if (call_type == DIVOOM_BTCORE_PHONE_ALERT_CALL_INCOMING
	    || call_type == DIVOOM_BTCORE_PHONE_ALERT_CALL_OUTGOING
	    || call_type == DIVOOM_BTCORE_PHONE_ALERT_CALL_ACTIVE) {
		divoom_lvgl_top_phone_call_menu_notify(call_type, number, number_len,
						       name, name_len);
	}
#endif
}

static void divoom_btcore_uart_handle_music_tag_info(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint8_t source;
	uint16_t music_id;
	uint8_t flags;
	uint8_t title_len;
	uint8_t artist_len;
	uint16_t artist_off;

	if (cmd_data == NULL || cmd_len < DIVOOM_BTCORE_MUSIC_TAG_HDR_LEN)
		return;

	source = cmd_data[0];
	music_id = divoom_btcore_get_le16(cmd_data + 1u);
	flags = cmd_data[3];
	title_len = cmd_data[4];
	if ((uint16_t)(5u + title_len + 1u) > cmd_len)
		return;
	artist_len = cmd_data[5u + title_len];
	artist_off = (uint16_t)(6u + title_len);
	if ((uint16_t)(artist_off + artist_len) > cmd_len)
		return;

	/* 协议先通：只校验长度并打日志，暂不交给 TF/表盘 */
	divoom_print("btcore 0x70 src=%u id=%u flags=0x%02x title=\"%.*s\" artist=\"%.*s\"\n",
		     (unsigned)source, (unsigned)music_id, (unsigned)flags,
		     (int)title_len, (const char *)(cmd_data + 5u),
		     (int)artist_len, (const char *)(cmd_data + artist_off));
}

static void divoom_btcore_uart_dispatch(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	if (divoom_btcore_uart_take_sync_reply(cmd, cmd_data, cmd_len))
		return;
	if (divoom_product_test_is_active()) {
		divoom_product_test_on_btcore_cmd(cmd, cmd_data, cmd_len);
		return;
	}
	switch (cmd)
	{
		/*
		 * 只处理 divoom_btcore_uart_cmd_ctx() 判为 CTX_WORK / CTX_SLOW 的命令；
		 * CTX_FAST / CTX_STREAM / CTX_DROP 不会走到这里，实现分别在
		 * divoom_btcore_uart_cmd_rx_handle_inline 与录音流模块。
		 */
		case DM_BTCORE_SPPBLE_CONNECT_INFO:
			divoom_ble_on_jl_connect_info(cmd_data, cmd_len);
			break;

		case DM_BTCORE_APP_DATA_TRANSMIT:
		{
			uint16_t app_len;

			if (!cmd_data || cmd_len < 3u || cmd_data[0] != 0u)
				break;
			app_len = divoom_com_unpack_uint16((uint8_t *)&cmd_data[1]);
			if (app_len == 0u || (uint16_t)(3u + app_len) > cmd_len)
				break;
			divoom_ble_recv_data_callback(NULL, (char *)&cmd_data[3], (int)app_len);
		}
			break;


		case DM_BTCORE_SD_TOTAL_MUSIC:
			divoom_tf_music_sync_set_reason("jl_uart_sd_total");
			divoom_tf_music_sync_begin(cmd_data, cmd_len);
			break;

		case DM_BTCORE_SD_MUSIC_LIST:
			break;

		case DM_BTCORE_SD_MUSIC_LIST_ITEM:
			divoom_tf_music_sync_set_reason("jl_uart_sd_item");
    		if (divoom_tf_music_sync_store_item(cmd_data, cmd_len))
    		{
        		if (divoom_tf_music_sync_is_complete())
        		{
         		   divoom_tf_music_sync_commit();
        		}
    		}
			break;

		case DM_BTCORE_SD_MUSIC_NOTIFY_FINISHED:
			/* status=1：0/1 正常切歌，5 解码失败跳过；其它 err 由切歌函数丢弃 */
			if (cmd_len >= 2u && cmd_data[0] == 1u)
				divoom_tf_music_play_finished_send_play_next((int)cmd_data[1]);
			break;

		case DM_BTCORE_SD_MUSIC_READY:
			if (!divoom_product_test_is_active())
				divoom_music_ctrl_on_jl_sd_music_ready();
			break;

		case DM_BTCORE_SONGLIST_INFO:
			divoom_btcore_file_rx_on_songlist_reply(cmd_data, cmd_len);
			break;

		case DM_BTCORE_SCAN_MUSIC_LIST:
			divoom_tf_music_on_scan_music_list_status(cmd_data[0]);
			break;

		case DM_BTCORE_FILE_HEAD_TRANSMIT:
			divoom_btcore_file_rx_on_head(cmd_data, cmd_len);
			break;

		case DM_BTCORE_FILE_BLOCK_TRANSMIT:
			divoom_btcore_file_rx_on_block(cmd_data, cmd_len);
			break;

		case DM_BTCORE_FILE_END:
			divoom_btcore_file_rx_on_end();
			break;

		case DM_BTCORE_FILE_MISSED_PACKETS:
			if (divoom_btcore_file_tx_is_busy())
				divoom_btcore_file_tx_on_missed_packets(cmd_data, cmd_len);
			break;
		
		case DM_BTCORE_SD_INSERT_STATUS:
			divoom_main_tf_det_cb(cmd_data[0]);
			break;

		case DM_BTCORE_LINEIN_INSERT_STATUS:
			divoom_main_linein_det_cb(cmd_data[0]);
			break;

		case DM_BTCORE_UAC_INSERT_STATUS:
			divoom_main_usb_det_cb(cmd_data[0]);
			break;

		case DM_BTCORE_WORK_MODE:
			if (cmd_len >= 1u && !divoom_product_test_is_active())
				divoom_music_ctrl_on_jl_work_mode_notify(cmd_data[0]);
			break;

		case DM_BTCORE_MUSIC_STATUS:
			divoom_music_ctrl_post_play_status(cmd_data[0]);
			break;

		case DM_BTCORE_MUSIC_VOLUME:
			divoom_bt_music_handle_play_vol(cmd_data[0]);
			break;

		case DM_BTCORE_HFP_CALL_VOLUME:
			if (cmd_len >= 1u)
			{
				divoom_music_ctrl_on_hfp_call_volume_notify(cmd_data[0]);
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
				if (divoom_music_ctrl_is_phone_call())
					divoom_lvgl_top_bright_volume_menu_on_hfp_call_volume(
						divoom_music_ctrl_get_hfp_call_volume_ui());
#endif
			}
			break;

		case DM_BTCORE_PLAY_BLUE_MUSIC: //手机端暂停蓝牙音乐
			divoom_bt_music_handle_play_ctrl(cmd_data[0]);
			break;

		case DM_BTCORE_DEVICE_VERSION:
			if (cmd_len >= 4u)
				divoom_ota_jl_on_version_notify(btcore_get_le32(cmd_data));
			break;

		case DM_BTCORE_SERVICE_A2DP_STATUS: 
//			divoom_bt_music_handle_connect_status(cmd_data[0]);
			break;

		case DM_BTCORE_BT_CONNECT_STATUS:
			if (cmd_len >= 1u)
				divoom_bt_music_handle_connect_status(cmd_data[0]);

//				divoom_bt_music_handle_bt_connect_status(cmd_data[0]);
			break;

		case DM_BTCORE_MUSIC_LAST_NEXT://对tf卡做同步显示
			if (cmd_data[0])
				divoom_music_ctrl_play_next(); 
			else
				divoom_music_ctrl_play_pre();
			break;

		case DM_BTCORE_LYRIC_DATA_TRANSMIT: /* 0x04 仅蓝牙歌词：type1=逐句歌词；type2=歌名+歌手。TF 标签走 0x70 */
			if (cmd_len < 1)
				break;
			if (cmd_data[0] == 1)
			{
				/* type=1 歌词 → song_info.lyric */
				divoom_bt_music_set_title_lyric((uint8_t *)&cmd_data[1], 1);
			}
			else if (cmd_data[0] == 2)
			{
				/* type=2 标题(歌名及歌手) → 拆分写入 song_name / singer */
				divoom_bt_music_set_title_lyric((uint8_t *)&cmd_data[1], 2);
			}
			break;

		case DM_BTCORE_MUSIC_TAG_INFO:
			divoom_btcore_uart_handle_music_tag_info(cmd_data, cmd_len);
			break;

		case DM_BTCORE_PHONE_CALL_ALERT:
			divoom_btcore_uart_handle_phone_call_alert(cmd_data, cmd_len);
			break;

		case DM_BTCORE_REMOTE_STATUS:
		case DM_BTCORE_REMOTE_KEY:
		case DM_BTCORE_REMOTE_BATTERY:
		case DM_BTCORE_REMOTE_INFO:
			/* HOGP 遥控占位，AstroToo 暂不处理 */
			break;

		case DM_BTCORE_TF_OTA_PROGRESS:
			if (cmd_data && cmd_len >= 1)
				divoom_lvgl_device_update_tf_set_percent(cmd_data[0]);
			break;

		case DM_BTCORE_TWS_STATUS:
		case DM_BTCORE_TWS_PAIR_CTRL:
		case DM_BTCORE_TWS_EVENT:
			divoom_tws_host_on_snap(cmd_data, cmd_len);
			break;

		case DM_BTCORE_BT_INFO: //设备本身蓝牙信息
			divoom_bt_music_parse_device_info(cmd_data, cmd_len);
			break;

		case DM_BTCORE_BT_AUTO_CONNECT:
			divoom_bt_music_parse_bt_auto_connect_info(cmd_data, cmd_len);
			break;

		case DM_BTCORE_BT_CONNECTED_PHONE_INFO:
			divoom_bt_music_parse_connected_phone_info(cmd_data, cmd_len);
			break;

		case DM_BTCORE_BT_BIP_DATA:
			divoom_bt_bip_on_packet(cmd_data, cmd_len);
			break;

		case DM_BTCORE_BT_MUSIC_TIME:
			if (cmd_len >= 5u)
			{
				uint32_t time_ms = (uint32_t)btcore_get_le32(cmd_data + 1u);

				divoom_bt_music_handle_music_time(cmd_data[0], time_ms);
			}
			break;

		case DM_BTCORE_USB_IN_OUT_STA:
			divoom_power_jl_set_usb_status(cmd_data[0]);
			break;

		case DM_BTCORE_REMIND_BAT_LOW_POWER:
			divoom_power_monitor_event_to_user(cmd_data[0]);
			break;

		case DM_BTCORE_SYSTEM_POWER_OFF:
			if (!divoom_main_is_btcore_poweroff_ready()) {
				s_btcore_poweroff_pending = 1;
				divoom_print("btcore: defer SYSTEM_POWER_OFF (0x19)\n");
				break;
			}
			if (!divoom_onoff_is_poweroff_active()) {
				s_btcore_poweroff_pending = 0;
				divoom_onoff_poweroff(0);
			}
			break;

		case DM_BTCORE_SYSTEM_TIME:
			/* JL回复/主动推送：ctrl=0 + year LE + mon/day/hour/min/sec */
			if (cmd_len >= DIVOOM_BTCORE_SYSTEM_TIME_PAYLOAD_REPLY_LEN && cmd_data &&
			    cmd_data[0] == DIVOOM_BTCORE_SYSTEM_TIME_CTRL_GET) {
				uint16 year = divoom_btcore_get_le16(cmd_data + 1);

				divoom_onoff_on_jl_system_time(year, cmd_data[3], cmd_data[4],
							       cmd_data[5], cmd_data[6], cmd_data[7]);
			}
			break;

		case DM_BTCORE_ENTER_BOOT_MODE:
			divoom_print("btcore: enter boot mode (0x67)\n");
			divoom_product_test_enter_efex_boot();
			break;

		case DM_BTCORE_SEND_KEY_SYSTEM_MSG:
			divoom_btcore_uart_handle_key_system_msg(cmd_data, cmd_len);
			break;

		case DM_BTCORE_OTA_START:
		case DM_BTCORE_OTA_DATA:
		case DM_BTCORE_OTA_END:
			/* 正常应在 send_wait 路径消费；走到此处说明已超时或未在等应答 */
			divoom_print("btcore ota rx cmd=0x%02x len=%u ignored (not waiting)\n",
				     (unsigned)cmd, (unsigned)cmd_len);
			break;
	}
}

static void divoom_btcore_uart_rx_bridge(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	if (s_btcore_rx_suppressed_after_power_off)
		return;
	divoom_btcore_uart_dispatch(cmd, cmd_data, cmd_len);
}

#ifdef DIVOOM_BTCORE_UART_CLI
void divoom_btcore_uart_cmd_debug_simulate_rx(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	divoom_btcore_uart_rx_bridge(cmd, cmd_data, cmd_len);
}
#endif

int divoom_btcore_uart_cmd_init(const divoom_btcore_uart_cmd_init_param_t *param)
{
	btcore_uart_config_t cfg;
	int ret;

	btcore_uart_set_rx_callback(divoom_btcore_uart_rx_bridge);
	if (!param)
		ret = btcore_uart_init(NULL);
	else
	{
		cfg.device = param->device;
		cfg.baudrate = param->baudrate > 0 ? param->baudrate : BTCORE_UART_BAUD_DEFAULT;
		ret = btcore_uart_init(&cfg);
	}
	if (ret != 0)
		return ret;
	if (!s_btcore_cmd_reply_sem)
		s_btcore_cmd_reply_sem = divoom_system_create_sem(1, 0);
	if (!s_btcore_cmd_uart_lock)
		s_btcore_cmd_uart_lock = divoom_system_create_lock();
	if (!s_btcore_cmd_reply_sem || !s_btcore_cmd_uart_lock)
		return -1;
	return 0;
}

void divoom_btcore_uart_cmd_deinit(void)
{
	btcore_uart_set_rx_callback(NULL);
	btcore_uart_deinit();
	if (s_btcore_cmd_reply_sem)
	{
		divoom_system_destroy_sem(s_btcore_cmd_reply_sem);
		s_btcore_cmd_reply_sem = NULL;
	}
	if (s_btcore_cmd_uart_lock)
	{
		divoom_system_destroy_lock(s_btcore_cmd_uart_lock);
		s_btcore_cmd_uart_lock = NULL;
	}
	s_btcore_cmd_reply_waiting = 0;
	s_btcore_reply_saved_len = 0;
	s_btcore_rx_suppressed_after_power_off = 0;
	divoom_spectrum_reset();
}

void divoom_btcore_uart_cmd_try_pending_poweroff(void)
{
	if (!s_btcore_poweroff_pending)
		return;
	if (!divoom_main_is_btcore_poweroff_ready())
		return;
	if (divoom_onoff_is_poweroff_active())
		return;
	s_btcore_poweroff_pending = 0;
	divoom_onoff_poweroff(0);
}

int divoom_btcore_uart_cmd_is_ready(void)
{
	return btcore_uart_is_ready();
}

int divoom_btcore_uart_cmd_is_rx_suppressed(void)
{
	return s_btcore_rx_suppressed_after_power_off ? 1 : 0;
}

int divoom_btcore_uart_cmd_send(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	int ret;

	if (cmd == DM_BTCORE_SYSTEM_POWER_OFF)
		s_btcore_rx_suppressed_after_power_off = 1;
	if (cmd == DM_BTCORE_RECORD && cmd_data && cmd_len >= 1u &&
	    divoom_btcore_rec_stream_on_tx_record(cmd_data[0], cmd_len >= 2u ? cmd_data[1] : 0u) != 0)
		return -1;
	ret = btcore_uart_send(cmd, cmd_data, cmd_len);
	return ret;
}

int divoom_btcore_uart_cmd_send_wait(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len,
				     uint8_t *reply_out, uint16_t reply_out_cap, uint16_t *reply_len_out)
{
	return divoom_btcore_uart_cmd_send_wait_timeout(cmd, cmd_data, cmd_len,
							reply_out, reply_out_cap, reply_len_out, 1);
}

int divoom_btcore_uart_cmd_send_wait_timeout(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len,
					     uint8_t *reply_out, uint16_t reply_out_cap,
					     uint16_t *reply_len_out, int timeout_s)
{
	int send_err;
	int got;
	uint16_t give_len;
	uint16_t copy_n;

	if (!s_btcore_cmd_reply_sem || !s_btcore_cmd_uart_lock)
		return -1;

	if (timeout_s <= 0)
		timeout_s = 1;

	divoom_system_get_lock(s_btcore_cmd_uart_lock);
	if (reply_len_out)
		*reply_len_out = 0u;

	s_btcore_cmd_reply_expect = cmd;
	s_btcore_cmd_reply_waiting = 1;
	s_btcore_reply_saved_len = 0;
	send_err = btcore_uart_send(cmd, cmd_data, cmd_len);
	if (send_err != 0)
	{
		s_btcore_cmd_reply_waiting = 0;
		divoom_system_free_lock(s_btcore_cmd_uart_lock);
		return send_err;
	}

	got = divoom_system_get_sem_with_time(s_btcore_cmd_reply_sem, timeout_s);
	s_btcore_cmd_reply_waiting = 0;

	if (!got)
	{
		divoom_print("btcore uart wait timeout cmd=0x%02x timeout_s=%d\n",
			     (unsigned)cmd, timeout_s);
		divoom_system_free_lock(s_btcore_cmd_uart_lock);
		return -1;
	}

	give_len = s_btcore_reply_saved_len;
	if (reply_len_out)
		*reply_len_out = give_len;

	if (reply_out && reply_out_cap > 0u && give_len > 0u)
	{
		copy_n = give_len;
		if (copy_n > BTCORE_MAX_CMD_DATA_LEN)
			copy_n = BTCORE_MAX_CMD_DATA_LEN;
		if (copy_n > reply_out_cap)
			copy_n = reply_out_cap;
		memcpy(reply_out, s_btcore_reply_buf, copy_n);
	}

	divoom_system_free_lock(s_btcore_cmd_uart_lock);
	return 0;
}

int divoom_btcore_uart_cmd_notify_aic_host_audio_ready(void)
{
	uint8_t reply = 0;
	uint16_t reply_len = 0;
	int ret, attempt;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	if (divoom_btcore_uart_cmd_send(DM_BTCORE_MAIN_SOC_BOOT_DONE, NULL, 0) == 0)
	{
		divoom_print("btcore aic host audio ready sent (no ack)\n");
		return 0;
	}

	divoom_print("btcore aic host audio ready send failed\n");
	return -1;
}

int divoom_btcore_uart_cmd_try_notify_aic_host_audio_ready(void)
{
	if (s_aic_host_audio_ready_sent)
		return 0;
	if (divoom_btcore_uart_cmd_notify_aic_host_audio_ready() != 0)
		return -1;
	s_aic_host_audio_ready_sent = 1;
	return 0;
}

int divoom_btcore_uart_cmd_is_boot_done_notified(void)
{
	return s_aic_host_audio_ready_sent ? 1 : 0;
}

int divoom_btcore_uart_cmd_send_ble_ctrl(uint8_t enable)
{
	uint8_t payload = enable ? DIVOOM_BTCORE_BLE_CTRL_ON : DIVOOM_BTCORE_BLE_CTRL_OFF;
	int ret;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send(DM_BTCORE_BLE_CTRL, &payload, 1);
	if (ret == 0)
		divoom_print("btcore ble ctrl sent: %u\n", (unsigned)payload);
	else
		divoom_print("btcore ble ctrl send failed: enable=%u ret=%d\n",
			     (unsigned)payload, ret);
	return ret;
}

int divoom_btcore_uart_cmd_send_check_link(void)
{
	const char *fw_path = DIVOOM_BTCORE_JL_FW_UPDATE_PATH;
	uint8_t payload[4 + 1 + 255];
	size_t path_len;
	uint16_t payload_len;
	uint8_t reply = 0;
	uint16_t reply_len;
	int ret;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	if (!fw_path)
		fw_path = "";

	path_len = strlen(fw_path);
	if (path_len > 255u)
		path_len = 255u;

	btcore_put_le32(payload, (int32_t)DIVOOM_SOFTWARE_VERSION);
	payload[4] = (uint8_t)path_len;
	if (path_len > 0u)
		memcpy(payload + 5, fw_path, path_len);
	payload_len = (uint16_t)(5u + path_len);

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_CHECK_LINK, payload, payload_len,
					       &reply, 1, &reply_len);
	if (ret == 0) {
		if (reply_len >= 1u)
			s_host_boot_cnt = reply;
		divoom_print("btcore check link ok, cnt=%u ver=%u path=%.*s\n",
			     (unsigned)s_host_boot_cnt,
			     (unsigned)DIVOOM_SOFTWARE_VERSION, (int)path_len, fw_path);
	} else {
		divoom_print("btcore check link failed, ver=%u path=%.*s ret=%d\n",
			     (unsigned)DIVOOM_SOFTWARE_VERSION, (int)path_len, fw_path, ret);
	}
	return ret;
}

int divoom_btcore_uart_cmd_get_host_boot_cnt(uint8_t *out_cnt)
{
	if (!s_host_boot_cnt)
		return -1;
	if (out_cnt)
		*out_cnt = s_host_boot_cnt;
	return 0;
}

int divoom_btcore_uart_cmd_send_scan_music_list(void)
{
	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	return divoom_btcore_uart_cmd_send(DM_BTCORE_SCAN_MUSIC_LIST, NULL, 0);
}

int divoom_btcore_uart_cmd_get_jl_device_version(uint32_t *out_version)
{
	uint8_t reply[4];
	uint16_t reply_len = 0;
	uint32_t ver;
	int ret;

	if (!out_version || !divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_DEVICE_VERSION, NULL, 0,
					       reply, sizeof(reply), &reply_len);
	if (ret != 0 || reply_len < 4u)
		return -1;

	ver = btcore_get_le32(reply);
	/* 先校验再写回，避免失败/非法值冲掉已缓存的杰理版本 */
	divoom_ota_jl_on_version_notify(ver);
	if (!divoom_ota_jl_version_is_valid(ver))
		return -1;
	*out_version = ver;
	return 0;
}

static void divoom_btcore_put_le16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v & 0xffu);
	p[1] = (uint8_t)((v >> 8) & 0xffu);
}

static uint16_t divoom_btcore_clamp_year(uint16_t year)
{
	if (year < 1u)
		return 1u;
	if (year > 9999u)
		return 9999u;
	return year;
}

static void divoom_btcore_pack_calendar_payload(uint8_t *payload, uint8_t ctrl,
						const divoom_btcore_alarm_time_t *t)
{
	uint16_t year = divoom_btcore_clamp_year(t->year);

	payload[0] = ctrl;
	divoom_btcore_put_le16(payload + 1, year);
	payload[3] = t->month;
	payload[4] = t->day;
	payload[5] = t->hour;
	payload[6] = t->minute;
	payload[7] = t->second;
}

int divoom_btcore_uart_cmd_send_set_system_time(const divoom_btcore_alarm_time_t *t)
{
	uint8_t payload[DIVOOM_BTCORE_SYSTEM_TIME_PAYLOAD_SET_LEN];

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (!t)
		return -1;

	divoom_btcore_pack_calendar_payload(payload, DIVOOM_BTCORE_SYSTEM_TIME_CTRL_SET, t);

	return divoom_btcore_uart_cmd_send(DM_BTCORE_SYSTEM_TIME, payload,
					   DIVOOM_BTCORE_SYSTEM_TIME_PAYLOAD_SET_LEN);
}

int divoom_btcore_uart_cmd_send_set_system_time_now(void)
{
	time_t now;
	struct tm tm_local;
	divoom_btcore_alarm_time_t t;

	now = time(NULL);
	if (now == (time_t)-1 || localtime_r(&now, &tm_local) == NULL)
		return -1;

	t.year = (uint16_t)(tm_local.tm_year + 1900);
	t.month = (uint8_t)(tm_local.tm_mon + 1);
	if (t.month > 12)
		t.month = 12;
	t.day = (uint8_t)tm_local.tm_mday;
	t.hour = (uint8_t)(tm_local.tm_hour <= 23 ? tm_local.tm_hour : 0);
	t.minute = (uint8_t)(tm_local.tm_min <= 59 ? tm_local.tm_min : 0);
	t.second = (uint8_t)(tm_local.tm_sec <= 59 ? tm_local.tm_sec : 0);

	return divoom_btcore_uart_cmd_send_set_system_time(&t);
}

int divoom_btcore_uart_cmd_send_get_system_time(uint8_t *reply_out, uint16_t reply_out_cap,
						uint16_t *reply_len_out)
{
	uint8_t payload[DIVOOM_BTCORE_SYSTEM_TIME_PAYLOAD_GET_LEN];

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	payload[0] = DIVOOM_BTCORE_SYSTEM_TIME_CTRL_GET;

	return divoom_btcore_uart_cmd_send_wait(DM_BTCORE_SYSTEM_TIME, payload,
						DIVOOM_BTCORE_SYSTEM_TIME_PAYLOAD_GET_LEN,
						reply_out, reply_out_cap, reply_len_out);
}

int divoom_btcore_uart_cmd_send_set_alarm_time(const divoom_btcore_alarm_time_t *t)
{
	uint8_t payload[DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_SET_ENABLE_LEN];
	uint16_t year;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (!t)
		return -1;

	year = t->year;
	if (year < 1u)
		year = 1u;
	else if (year > 9999u)
		year = 9999u;

	payload[0] = DIVOOM_BTCORE_ALARM_TIME_CTRL_SET;
	payload[1] = DIVOOM_BTCORE_ALARM_TIME_ENABLE;
	divoom_btcore_put_le16(payload + 2, year);
	payload[4] = t->month;
	payload[5] = t->day;
	payload[6] = t->hour;
	payload[7] = t->minute;
	payload[8] = t->second;

	return divoom_btcore_uart_cmd_send(DM_BTCORE_SET_ALARM_TIME, payload,
					   DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_SET_ENABLE_LEN);
}

int divoom_btcore_uart_cmd_send_disable_alarm_time(void)
{
	uint8_t payload[DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_SET_DISABLE_LEN];

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	payload[0] = DIVOOM_BTCORE_ALARM_TIME_CTRL_SET;
	payload[1] = DIVOOM_BTCORE_ALARM_TIME_DISABLE;

	return divoom_btcore_uart_cmd_send(DM_BTCORE_SET_ALARM_TIME, payload,
					   DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_SET_DISABLE_LEN);
}

int divoom_btcore_uart_cmd_send_get_alarm_time(uint8_t *reply_out, uint16_t reply_out_cap,
					       uint16_t *reply_len_out)
{
	uint8_t payload[DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_GET_LEN];

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	payload[0] = DIVOOM_BTCORE_ALARM_TIME_CTRL_GET;

	return divoom_btcore_uart_cmd_send_wait(DM_BTCORE_SET_ALARM_TIME, payload,
						DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_GET_LEN,
						reply_out, reply_out_cap, reply_len_out);
}

int divoom_btcore_uart_cmd_query_boot_mode(uint8_t *out_mode, int timeout_s)
{
	int ret;
	uint8_t recv = 0;
	uint16_t ret_len = 0;

	if (!out_mode)
		return -1;
	*out_mode = DIVOOM_BTCORE_BOOT_MODE_NORMAL;
	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (timeout_s <= 0)
		timeout_s = DIVOOM_BTCORE_BOOT_MODE_QUERY_TIMEOUT_S;

	ret = divoom_btcore_uart_cmd_send_wait_timeout(DM_BTCORE_GET_BOOT_MODE, NULL, 0,
						       &recv, 1, &ret_len, timeout_s);
	if (ret != 0 || ret_len < 1u)
		return -1;

	*out_mode = recv;
	return 0;
}

static int divoom_btcore_uart_cmd_audio_effect_to_jl_mode(uint8_t audio_effect_idx)
{
	switch (audio_effect_idx) {
	case DIVOOM_AUDIO_EFFECT_ORIGINAL:
		return (int)DIVOOM_BTCORE_SOUND_EFFECT_JL_ORIGINAL;
	case DIVOOM_AUDIO_EFFECT_GAMING:
		return (int)DIVOOM_BTCORE_SOUND_EFFECT_JL_GAMING;
	case DIVOOM_AUDIO_EFFECT_HIFI:
		return (int)DIVOOM_BTCORE_SOUND_EFFECT_JL_HIFI;
	case DIVOOM_AUDIO_EFFECT_OUTDOOR:
		return (int)DIVOOM_BTCORE_SOUND_EFFECT_JL_OUTDOOR;
	case DIVOOM_AUDIO_EFFECT_RETRO:
		return (int)DIVOOM_BTCORE_SOUND_EFFECT_JL_RETRO;
	default:
		return -1;
	}
}

static int divoom_btcore_uart_cmd_jl_mode_to_audio_effect(uint8_t jl_mode)
{
	switch (jl_mode) {
	case DIVOOM_BTCORE_SOUND_EFFECT_JL_GAMING:
		return (int)DIVOOM_AUDIO_EFFECT_GAMING;
	case DIVOOM_BTCORE_SOUND_EFFECT_JL_HIFI:
		return (int)DIVOOM_AUDIO_EFFECT_HIFI;
	case DIVOOM_BTCORE_SOUND_EFFECT_JL_ORIGINAL:
		return (int)DIVOOM_AUDIO_EFFECT_ORIGINAL;
	case DIVOOM_BTCORE_SOUND_EFFECT_JL_OUTDOOR:
		return (int)DIVOOM_AUDIO_EFFECT_OUTDOOR;
	case DIVOOM_BTCORE_SOUND_EFFECT_JL_RETRO:
		return (int)DIVOOM_AUDIO_EFFECT_RETRO;
	default:
		return -1;
	}
}

static int divoom_btcore_uart_cmd_send_set_sound_effect_mode(uint8_t jl_mode)
{
	uint8_t payload[2];

	payload[0] = 1u;
	payload[1] = jl_mode;
	return divoom_btcore_uart_cmd_send(DM_BTCORE_SOUND_EFFECT_MODE, payload, sizeof(payload));
}

int divoom_btcore_uart_cmd_apply_sound_effect(uint8_t audio_effect_idx)
{
	int jl_mode;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	jl_mode = divoom_btcore_uart_cmd_audio_effect_to_jl_mode(audio_effect_idx);
	if (jl_mode < 0)
		return -1;

	return divoom_btcore_uart_cmd_send_set_sound_effect_mode((uint8_t)jl_mode) != 0 ? -1 : 0;
}

int divoom_btcore_uart_cmd_send_get_sound_effect_mode(void)
{
	uint8_t payload = 0u;
	uint8_t reply[1];
	uint16_t reply_len = 0;
	int ret;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_SOUND_EFFECT_MODE, &payload, 1u,
					       reply, (uint16_t)sizeof(reply), &reply_len);
	if (ret != 0 || reply_len < 1u)
		return -1;

	return divoom_btcore_uart_cmd_jl_mode_to_audio_effect(reply[0]);
}

int divoom_btcore_uart_cmd_set_iisin_eq(uint8_t level)
{
	uint8_t payload[2];

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (level > DIVOOM_BTCORE_IISIN_EQ_LEVEL_MAX)
		level = (uint8_t)DIVOOM_BTCORE_IISIN_EQ_LEVEL_MAX;

	payload[0] = (uint8_t)DIVOOM_BTCORE_IISIN_EQ_OP_SET;
	payload[1] = level;
	return divoom_btcore_uart_cmd_send(DM_BTCORE_IISIN_EQ, payload, sizeof(payload));
}

int divoom_btcore_uart_cmd_get_iisin_eq(uint8_t *out_level)
{
	uint8_t payload = (uint8_t)DIVOOM_BTCORE_IISIN_EQ_OP_GET;
	uint8_t reply[1];
	uint16_t reply_len = 0;
	int ret;

	if (!out_level || !divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_IISIN_EQ, &payload, 1u,
					       reply, (uint16_t)sizeof(reply), &reply_len);
	if (ret != 0 || reply_len < 1u)
		return -1;

	*out_level = reply[0];
	if (*out_level > DIVOOM_BTCORE_IISIN_EQ_LEVEL_MAX)
		*out_level = (uint8_t)DIVOOM_BTCORE_IISIN_EQ_LEVEL_MAX;
	return 0;
}


#endif
