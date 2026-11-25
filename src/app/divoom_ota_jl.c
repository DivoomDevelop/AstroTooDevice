#include "divoom.h"

#ifdef DIVOOM_TIMES_VIBE

#include "divoom_ota_jl.h"
#include "divoom_ota_server.h"
#include "divoom_btcore_ota_tx.h"
#include "divoom_lvgl_device_update.h"
#include "divoom_menu_control.h"

#define DIVOOM_OTA_JL_DL_TRY        2

static uint32_t s_jl_local_version = 0;
static volatile uint8_t s_jl_upgrading = 0;

typedef struct {
	divoom_general_json_get_update_file_info_respone info;
	divoom_ota_jl_progress_callback *progress_callback;
	divoom_ota_jl_result_callback *result_callback;
	void *user_data;
	uint8 show_system_ui;
	uint8 file_server_flag;
} divoom_ota_jl_task_ctx;

int divoom_ota_jl_version_is_valid(uint32_t jl_version)
{
	uint32_t hw;

	if (jl_version == 0u)
		return 0;

	hw = jl_version / 1000u;
	/* 排除未初始化、以及误把主控版本当杰理版本 */
	if (hw == 0u || hw == (uint32_t)DIVOOM_HARDWARE_VERSION)
		return 0;
	/* 云端 Hardware 一般为三位数 */
	if (hw < 100u || hw > 999u)
		return 0;
	return 1;
}

int divoom_ota_is_f101_cloud_package(uint32_t version)
{
	return (version / 1000u) == (uint32_t)DIVOOM_HARDWARE_VERSION ? 1 : 0;
}

int divoom_ota_jl_is_cloud_package(uint32_t version, int cloud_hardware)
{
	uint32_t pkg_hw = version / 1000u;
	uint32_t jl_ver = s_jl_local_version;
	uint32_t jl_hw;

	if (divoom_ota_is_f101_cloud_package(version))
		return 0;

	/* 依赖开机 BOOT_DONE 后杰理主动上报缓存的版本，Push 时不再主动查询 */
	if (divoom_ota_jl_version_is_valid(jl_ver)) {
		jl_hw = jl_ver / 1000u;
		if (pkg_hw != jl_hw) {
			divoom_print("ota jl: pkg hw=%u != jl hw=%u (pkg=%u jl=%u)\n",
				     (unsigned)pkg_hw, (unsigned)jl_hw,
				     (unsigned)version, (unsigned)jl_ver);
			return 0;
		}
		return 1;
	}

	/*
	 * 尚未收到杰理上报时：用云端 Hardware 与 Software 前缀互证（均非主控）。
	 * 正常应在 BOOT_DONE 后已缓存；此处仅作兜底，避免误刷 F101。
	 */
	if (cloud_hardware > 0 &&
	    cloud_hardware != DIVOOM_HARDWARE_VERSION &&
	    (uint32_t)cloud_hardware == pkg_hw) {
		divoom_print("ota jl: cached ver missing, accept by cloud Hardware=%d Software=%u\n",
			     cloud_hardware, (unsigned)version);
		return 1;
	}

	divoom_print("ota jl: no cached local ver, reject pkg=%u cloud_hw=%d\n",
		     (unsigned)version, cloud_hardware);
	return 0;
}

static void divoom_ota_jl_show_ui(void)
{
	divoom_app_schedule_item_info channel_schedule = {0};
	divoom_sys_msg_info model_msg = {0};

	divoom_manage_image_up_netstatus(0);
	model_msg.src_id = DIVOOM_MODEL_OTA;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_DELETE_APP_ID;
	divoom_sys_model_crl_msg_push(&model_msg);

	channel_schedule.model_id = DIVOOM_MODEL_OTA;
	channel_schedule.event_id = DIVOOM_OTA_SERVER_JL_UPGRADE;
	channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	divoom_app_schedule_push(&channel_schedule);
	divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_JL_OTA_UPDATE, NULL,
					  DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_JL_UPGRADE);
}

static void divoom_ota_jl_hide_ui(void)
{
	divoom_app_schedule_item_info channel_schedule = {0};

	channel_schedule.model_id = DIVOOM_MODEL_OTA;
	channel_schedule.event_id = DIVOOM_OTA_SERVER_JL_UPGRADE;
	channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&channel_schedule);
	divoom_menu_control_load_next_pop_cur(divoom_menu_control_menu_get_first_menu_id(), NULL, 0, 0);
}

static void divoom_ota_jl_show_percent(uint8_t percent)
{
	divoom_lvgl_jl_ota_update_set_percent(percent);
}

static void divoom_ota_jl_report_progress(divoom_ota_jl_task_ctx *ctx,
	uint8 percent)
{
	if (!ctx)
		return;
	if (ctx->show_system_ui)
		divoom_ota_jl_show_percent(percent);
	if (ctx->progress_callback)
		ctx->progress_callback(ctx->user_data, percent);
}

static void divoom_ota_jl_download_progress(void *user_data, uint32_t received,
	uint32_t total)
{
	divoom_ota_jl_task_ctx *ctx = (divoom_ota_jl_task_ctx *)user_data;
	uint8 percent;

	if (!ctx || total == 0u)
		return;
	percent = (uint8)((received * 39u) / total);
	if (percent > 39u)
		percent = 39u;
	divoom_ota_jl_report_progress(ctx, percent);
}

static void divoom_ota_jl_tx_progress(void *user_data, uint32_t sent, uint32_t total)
{
	divoom_ota_jl_task_ctx *ctx = (divoom_ota_jl_task_ctx *)user_data;
	uint8_t percent;

	if (total == 0u)
		return;
	percent = (uint8_t)(40u + (sent * 59u) / total);
	if (percent > 99u)
		percent = 99u;
	divoom_ota_jl_report_progress(ctx, percent);
}

static uint8 divoom_ota_jl_download_and_verify(divoom_ota_jl_task_ctx *ctx)
{
	Pdivoom_general_json_get_update_file_info_respone pinfo;
	char sha1sum_info[128];
	int i;

	if (!ctx)
		return 0;
	pinfo = &ctx->info;
	if (!pinfo->file_id[0] || !pinfo->sha1_sum[0])
		return 0;

	for (i = 0; i < DIVOOM_OTA_JL_DL_TRY; i++)
	{
		if (!divoom_http_client_down_file_no_delay(DIVOOM_OTA_JL_FW_TMP_PATH,
							   pinfo->file_id,
							   ctx->file_server_flag, NULL,
							   divoom_ota_jl_download_progress,
							   ctx))
			continue;

		divoom_system_delay_ms(500);
		exec_command_system("sync");
		sha1sum_info[0] = '\0';
		divoom_app_com_get_file_sha1sum(DIVOOM_OTA_JL_FW_TMP_PATH, sha1sum_info);
		if (strcmp(sha1sum_info, pinfo->sha1_sum) == 0)
			return 1;

		divoom_print("ota jl: sha1 mismatch local=%s expect=%s\n",
			     sha1sum_info, pinfo->sha1_sum);
	}
	return 0;
}

static void *divoom_ota_jl_upgrade_task(void *param)
{
	divoom_ota_jl_task_ctx *ctx = (divoom_ota_jl_task_ctx *)param;
	Pdivoom_general_json_get_update_file_info_respone pinfo;
	divoom_ota_jl_result_callback *result_callback;
	void *user_data;
	uint8 show_system_ui;
	int ok = 0;

	if (!ctx)
		goto out;
	pinfo = &ctx->info;

	divoom_print("ota jl: start upgrade srv=%u local=%u\n",
		     (unsigned)pinfo->version, (unsigned)s_jl_local_version);

	divoom_ota_server_set_update_status(1);
	if (ctx->show_system_ui)
		divoom_ota_jl_show_ui();
	divoom_system_delay_ms(300);
	divoom_ota_jl_report_progress(ctx, 0);

	if (!divoom_ota_jl_download_and_verify(ctx))
	{
		divoom_print("ota jl: download or sha1 failed\n");
		goto out;
	}
	divoom_print("ota jl: download+sha1 ok, start uart tx\n");

	divoom_ota_jl_report_progress(ctx, 40);
	divoom_local_movie_play_stop_movie();
	divoom_system_delay_ms(500);
	if (divoom_btcore_ota_tx_send_firmware(DIVOOM_OTA_JL_FW_TMP_PATH, pinfo->version, 1,
					       divoom_ota_jl_tx_progress, ctx) != 0)
	{
		divoom_print("ota jl: uart ota tx failed\n");
		goto out;
	}

	divoom_ota_jl_report_progress(ctx, 100);
	divoom_system_delay_ms(800);
	if (divoom_btcore_uart_cmd_get_jl_device_version(&s_jl_local_version) == 0)
		divoom_print("ota jl: done, jl version=%u\n", (unsigned)s_jl_local_version);
	ok = 1;

out:
	divoom_ota_server_set_update_status(0);
	s_jl_upgrading = 0;
	show_system_ui = ctx ? ctx->show_system_ui : 0;
	result_callback = ctx ? ctx->result_callback : NULL;
	user_data = ctx ? ctx->user_data : NULL;
	if (show_system_ui)
		divoom_ota_jl_hide_ui();
	if (ctx)
		divoom_system_free(ctx);
	if (!ok && show_system_ui)
		divoom_main_push_model_event_id(DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_SRV_UPDATE_FAIL);
	if (result_callback)
		result_callback(user_data, ok);
	return NULL;
}

void divoom_ota_jl_init(void)
{
	/* 勿清 s_jl_local_version：check_link/BOOT_DONE 上报可能早于 ota_server_init */
	s_jl_upgrading = 0;
}

void divoom_ota_jl_on_version_notify(uint32_t jl_version)
{
	if (!divoom_ota_jl_version_is_valid(jl_version)) {
		divoom_print("ota jl: ignore invalid version=%u\n", (unsigned)jl_version);
		return;
	}
	s_jl_local_version = jl_version;
	divoom_print("ota jl: cached version=%u (hw=%u)\n",
		     (unsigned)jl_version, (unsigned)(jl_version / 1000u));
}

uint32_t divoom_ota_jl_get_local_version(void)
{
	return s_jl_local_version;
}

uint8 divoom_ota_jl_is_upgrading(void)
{
	return s_jl_upgrading;
}

static int divoom_ota_jl_start_update(
	Pdivoom_general_json_get_update_file_info_respone pinfo,
	uint8 show_system_ui, uint8 file_server_flag,
	divoom_ota_jl_progress_callback *progress_callback,
	divoom_ota_jl_result_callback *result_callback, void *user_data)
{
	divoom_ota_jl_task_ctx *ctx;

	if (!pinfo || s_jl_upgrading || divoom_ota_server_get_update_status())
		return -1;

	/* 仅用已缓存版本比较；不在此处主动查 0x50 */
	if (divoom_ota_jl_version_is_valid(s_jl_local_version) &&
	    pinfo->version <= s_jl_local_version)
	{
		divoom_print("ota jl: skip srv=%u local=%u\n",
			     (unsigned)pinfo->version, (unsigned)s_jl_local_version);
		return 0;
	}

	ctx = (divoom_ota_jl_task_ctx *)divoom_system_malloc(sizeof(*ctx));
	if (!ctx)
		return -1;
	memset(ctx, 0, sizeof(*ctx));
	memcpy(&ctx->info, pinfo, sizeof(ctx->info));
	ctx->progress_callback = progress_callback;
	ctx->result_callback = result_callback;
	ctx->user_data = user_data;
	ctx->show_system_ui = show_system_ui;
	ctx->file_server_flag = file_server_flag;

	s_jl_upgrading = 1;
	if (!divoom_system_create_task(divoom_ota_jl_upgrade_task, "ota_jl", ctx,
				       DIVOOM_TASK_STACK_OTA_JL)) {
		s_jl_upgrading = 0;
		divoom_system_free(ctx);
		return -1;
	}
	return 1;
}

int divoom_ota_jl_start_srv_update(Pdivoom_general_json_get_update_file_info_respone pinfo)
{
	return divoom_ota_jl_start_update(pinfo, 1, 1, NULL, NULL, NULL);
}

int divoom_ota_jl_start_local_update(
	Pdivoom_general_json_get_update_file_info_respone pinfo,
	divoom_ota_jl_progress_callback *progress_callback,
	divoom_ota_jl_result_callback *result_callback, void *user_data)
{
	return divoom_ota_jl_start_update(pinfo, 0, 0, progress_callback,
		result_callback, user_data);
}

#endif /* DIVOOM_TIMES_VIBE */
