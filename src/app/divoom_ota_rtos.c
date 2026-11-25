#include "divoom.h"
#include "divoom_boot_health.h"
#include "divoom_ota_rtos.h"
#include "divoom_ota_zip.h"
#include "divoom_ota_server.h"
#include "divoom_product_test.h"
#include "divoom_port_fs.h"
#include "divoom_jl_amp_adapter.h"
#include "sunxi_update.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#if DIVOOM_BUILD_RTOS_DEVICE && defined(CONFIG_PROJECT_F101S3_ASTROTOO) && \
    defined(CONFIG_SUNXI_OTA_UPDATE_PART) && !defined(CONFIG_SUNXI_OTA_UPDATE_PART_BUFF) && \
    !defined(CONFIG_SUNXI_UPDATE_IMAGE_KEEP_MEMORY)
#define DIVOOM_OTA_HAS_UPDATE_SESSION 1
#endif

#ifdef DIVOOM_TIMES_VIBE
#include "divoom_ota_jl.h"
#endif
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_lvgl_device_update.h"
#endif

extern int cmd_reboot(int argc, char **argv);

typedef struct {
	char full_path[256];
	char staging_path[256];
	int prepare_from_staging;
} divoom_ota_rtos_task_param_t;

static volatile uint8 s_rtos_ota_running;
static volatile uint8 s_rtos_ota_flash_started;
static int s_ota_dl_last_percent = -1;

/* 仅压 iis_in（0x2A=0）。勿发 0x1e：F101 reboot 杰理不复位，开机不解功放 mute。
 * 失败不改 TONE。开机第一次 I2S（提示音 amp_tip_hold）会 prime 回 100。 */
static void divoom_ota_rtos_mute_iis_in_before_reboot(void)
{
	uint8_t tone[3];

	if (!divoom_btcore_uart_cmd_is_ready())
		return;
	tone[0] = 1;
	tone[1] = 0;
	tone[2] = 0;
	(void)divoom_btcore_uart_cmd_send(DM_BTCORE_TONE_VOLUME, tone, 3);
}

static void *divoom_ota_rtos_task_main(void *param);

#define DIVOOM_OTA_RTOS_COPY_BUF  DIVOOM_PORT_WRITE_CHUNK

static int divoom_ota_rtos_copy_file(const char *src, const char *dst)
{
	void *sfp = NULL;
	void *dfp = NULL;
	char *buf = NULL;
	size_t n;
	int ret = -1;

	divoom_port_remove_file(dst);
	sfp = divoom_port_file_open(src, "rb");
	dfp = divoom_port_file_open(dst, "wb");
	if (!sfp || !dfp)
		goto out;

	buf = divoom_system_malloc(DIVOOM_OTA_RTOS_COPY_BUF);
	if (!buf)
		goto out;

	while ((n = divoom_port_file_read(buf, 1, DIVOOM_OTA_RTOS_COPY_BUF, sfp)) > 0) {
		if (divoom_port_file_write(buf, 1, n, dfp) != n)
			goto out;
	}
	if (ferror((FILE *)sfp) || divoom_port_file_sync(dfp) != 0)
		goto out;
	ret = 0;

out:
	divoom_system_free(buf);
	if (dfp && divoom_port_file_close(dfp) != 0)
		ret = -1;
	if (sfp && divoom_port_file_close(sfp) != 0)
		ret = -1;
	if (ret != 0)
		divoom_port_remove_file(dst);
	return ret;
}

static int divoom_ota_rtos_verify_sha1(const char *path, const char *sha1_expect)
{
	char sha1sum_info[128];

	if (!sha1_expect || !sha1_expect[0])
		return 0;

	sha1sum_info[0] = '\0';
	divoom_app_com_get_file_sha1sum((char *)path, sha1sum_info);
	if (strcmp(sha1sum_info, sha1_expect) != 0) {
		divoom_print("ota_rtos: sha1 mismatch %s != %s (%s)\n",
			     sha1sum_info, sha1_expect, path);
		return -1;
	}
	return 0;
}

static void divoom_ota_rtos_report_progress(int percent)
{
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	if (divoom_product_test_is_active()) {
		divoom_product_test_on_ota_percent(percent);
		return;
	}

	if (divoom_ota_server_get_show_percent()) {
#if DIVOOM_BUILD_RTOS_DEVICE
		divoom_lvgl_device_update_set_ota_progress((uint8)percent);
#else
		divoom_disp_ctrl_set_mode(DIVOOM_DISP_DEVICE_UPDATE_MODE, &percent);
#endif
		return;
	}

	{
		divoom_sys_msg_info model_msg = {0};

		model_msg.src_id = DIVOOM_MODEL_OTA;
		model_msg.dest_id = DIVOOM_MODEL_PRODUCT_TEST;
		model_msg.msg_id = DIVOOM_PRODUCT_TEST_MODE_FIRMWARE_UPDATE_PERCENT;
		model_msg.data = (uint8 *)&percent;
		divoom_sys_model_crl_msg_push(&model_msg);
	}
}

static void divoom_ota_rtos_http_progress(void *user_data, uint32 read_len, uint32 tol_len)
{
	int percent;

	(void)user_data;
	if (tol_len == 0)
		return;
	percent = (int)((unsigned long long)read_len * 100ull / (unsigned long long)tol_len);
	if (percent > 100)
		percent = 100;
	if (percent == s_ota_dl_last_percent)
		return;
	s_ota_dl_last_percent = percent;
	divoom_ota_rtos_report_progress(percent);
}

#if DIVOOM_BUILD_RTOS_DEVICE
static void divoom_ota_rtos_ui_zip_progress(int percent, void *user_data)
{
	(void)user_data;

	if (divoom_product_test_is_active()) {
		divoom_product_test_on_ota_unzip_percent(percent);
		return;
	}
	if (divoom_ota_server_get_show_percent())
		divoom_lvgl_device_update_set_ota_progress((uint8)percent);
}
#endif

const char *divoom_ota_rtos_basename_from_url(const char *url, char *buf, int buf_len)
{
	const char *slash;
	char *query;

	if (!buf || buf_len <= 0)
		return NULL;

	buf[0] = '\0';
	if (!url || !url[0]) {
		strncpy(buf, "rtos_pkg_nor.fex", buf_len - 1);
		buf[buf_len - 1] = '\0';
		return buf;
	}

	slash = strrchr(url, '/');
	if (!slash || !slash[1]) {
		strncpy(buf, "rtos_pkg_nor.fex", buf_len - 1);
		buf[buf_len - 1] = '\0';
		return buf;
	}

	strncpy(buf, slash + 1, buf_len - 1);
	buf[buf_len - 1] = '\0';
	query = strchr(buf, '?');
	if (query)
		*query = '\0';
	if (!buf[0]) {
		strncpy(buf, "rtos_pkg_nor.fex", buf_len - 1);
		buf[buf_len - 1] = '\0';
	}
	return buf;
}

int divoom_ota_rtos_build_save_path(const char *url, char *save_path, int save_path_len)
{
	(void)url; /* CDN FileId 超长会截断 UPDATE 32B 缓冲，落盘固定短名 */

	if (!save_path || save_path_len <= 0)
		return -1;

	snprintf(save_path, (size_t)save_path_len, "%s/%s",
		 DIVOOM_OTA_RTOS_STAGING_DIR, DIVOOM_OTA_RTOS_PACKAGE_NAME);
	return 0;
}

int divoom_ota_rtos_build_staging_download_path(char *save_path, int save_path_len)
{
	if (!save_path || save_path_len <= 0)
		return -1;

	strncpy(save_path, DIVOOM_OTA_RTOS_STAGING_DOWNLOAD, (size_t)save_path_len - 1);
	save_path[save_path_len - 1] = '\0';
	return 0;
}

void divoom_ota_rtos_remove_staging_if_present(void)
{
	if (divoom_port_file_exists(DIVOOM_OTA_RTOS_STAGING_DOWNLOAD))
		divoom_port_remove_file(DIVOOM_OTA_RTOS_STAGING_DOWNLOAD);
}

int divoom_ota_rtos_prepare_package(const char *staging_path, const char *out_fex_path,
				    divoom_ota_prepare_progress_fn progress, void *progress_ud)
{
#if DIVOOM_BUILD_SIMULATOR
	(void)staging_path;
	(void)out_fex_path;
	(void)progress;
	(void)progress_ud;
	return -1;
#else
	if (!staging_path || !out_fex_path || !staging_path[0] || !out_fex_path[0])
		return -1;
	if (!divoom_port_file_exists(staging_path)) {
		divoom_print("ota_rtos: prepare missing staging %s\n", staging_path);
		return -1;
	}

	if (divoom_ota_is_zip_file(staging_path)) {
		divoom_print("ota_rtos: prepare unzip %s -> %s\n", staging_path, out_fex_path);
		return divoom_ota_zip_extract_fex(staging_path, out_fex_path, progress, progress_ud);
	}

	divoom_print("ota_rtos: prepare raw fex %s -> %s\n", staging_path, out_fex_path);
	divoom_port_remove_file(out_fex_path);
	if (divoom_port_rename_file(staging_path, out_fex_path) == 0) {
		divoom_port_sync();
		if (progress)
			progress(100, progress_ud);
		return 0;
	}

	if (divoom_ota_rtos_copy_file(staging_path, out_fex_path) != 0)
		return -1;

	divoom_port_sync();
	if (progress)
		progress(100, progress_ud);
	return 0;
#endif
}

static int divoom_ota_rtos_prepare_package_blocking(const char *staging_path,
						    const char *out_fex_path)
{
	return divoom_ota_rtos_prepare_package(staging_path, out_fex_path,
#if DIVOOM_BUILD_RTOS_DEVICE
					      divoom_ota_rtos_ui_zip_progress, NULL);
#else
					      NULL, NULL);
#endif
}

static int divoom_ota_rtos_spawn_task(divoom_ota_rtos_task_param_t *task)
{
	if (!__sync_bool_compare_and_swap(&s_rtos_ota_running, 0, 1)) {
		divoom_print("ota_rtos: reject duplicate task %s\n", task->full_path);
		divoom_system_free(task);
		return -1;
	}
	s_rtos_ota_flash_started = 0;
	divoom_ota_server_set_update_status(1);
	divoom_print("ota_rtos: start task prepare=%d fex=%s\n",
		     task->prepare_from_staging, task->full_path);
	if (!divoom_system_create_task(divoom_ota_rtos_task_main, "ota_rtos", task,
				       DIVOOM_TASK_STACK_OTA_SERVER_LOCAL)) {
		divoom_print("ota_rtos: create task fail %s\n", task->full_path);
		divoom_system_free(task);
		s_rtos_ota_running = 0;
		divoom_ota_server_set_update_status(0);
		return -1;
	}
	return 0;
}

int divoom_ota_rtos_normalize_sha1(const char *digest, char normalized[41])
{
	unsigned int i;
	if (!digest || !normalized)
		return 0;
	for (i = 0; i < 40; ++i) {
		char c = digest[i];
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
		      (c >= 'A' && c <= 'F')))
			return 0;
		normalized[i] = (c >= 'A' && c <= 'F') ? c + ('a' - 'A') : c;
	}
	if (digest[40] != '\0')
		return 0;
	normalized[40] = '\0';
	return 1;
}

int divoom_ota_rtos_download_staging(const char *url, const char *sha1_expect,
					      int show_percent,
					      void (*dl_progress)(void *user_data, uint32 read_len,
								  uint32 tol_len),
					      void *dl_progress_ud)
{
	char staging_path[256];
#if DIVOOM_BUILD_RTOS_DEVICE
	char recovery_sha1[41];
#endif

	if (!url || !url[0])
		return 0;

#if DIVOOM_BUILD_RTOS_DEVICE
	/* Every network entry point, including push/manual updates, passes here. */
	if (divoom_main_get_boot_ab_recovery_needed()) {
		if (!divoom_ota_rtos_normalize_sha1(sha1_expect, recovery_sha1)) {
			divoom_print("[BOOT AB] recovery firmware requires SHA-1\n");
			return 0;
		}
		sha1_expect = recovery_sha1;
	}
#endif
	divoom_port_mkdir_p(DIVOOM_OTA_RTOS_STAGING_DIR);
	divoom_ota_rtos_build_staging_download_path(staging_path, sizeof(staging_path));

	s_ota_dl_last_percent = -1;
	if (!dl_progress)
		dl_progress = divoom_ota_rtos_http_progress;
#if DIVOOM_BUILD_RTOS_DEVICE
	if (!divoom_http_client_down_ota_file(staging_path, url, show_percent, sha1_expect,
					      dl_progress, dl_progress_ud)) {
#else
	if (!divoom_http_client_down_file_no_delay(staging_path, (char *)url, show_percent,
						   NULL, dl_progress, dl_progress_ud)) {
#endif
		divoom_print("ota_rtos: download fail %s\n", url);
		return 0;
	}

	divoom_system_delay_ms(500);
	divoom_port_sync();

	if (divoom_ota_rtos_verify_sha1(staging_path, sha1_expect) != 0) {
		divoom_ota_rtos_remove_staging_if_present();
		return 0;
	}
	return 1;
}

static int divoom_ota_rtos_split_path(const char *full_path, char *dir, int dir_len,
				      char *base, int base_len)
{
	const char *slash;

	if (!full_path || !full_path[0] || !dir || !base)
		return -1;

	slash = strrchr(full_path, '/');
	if (!slash)
		return -1;

	if ((int)(slash - full_path + 2) > dir_len)
		return -1;
	memcpy(dir, full_path, (size_t)(slash - full_path + 1));
	dir[slash - full_path + 1] = '\0';
	if (dir[0] && dir[strlen(dir) - 1] != '/') {
		if ((int)(strlen(dir) + 1) >= dir_len)
			return -1;
		strcat(dir, "/");
	}

	strncpy(base, slash + 1, base_len - 1);
	base[base_len - 1] = '\0';
	if (!base[0])
		return -1;
	return 0;
}

static void divoom_ota_rtos_resume_after_flash_failure(void)
{
	divoom_jl_audio_gates_mute_set(0);
	if (!divoom_database_manage_init(DIVOOM_THUMB_DATA_BASE_FILE_NAME))
		divoom_print("ota_rtos: database reopen failed after update failure\n");
}

static int divoom_ota_rtos_apply_package(const char *full_path)
{
	char dir[192];
	char base[96];
	char *dir_buf;
	long ret;
	int status;
	int prog;
	int last_prog = -1;

	if (divoom_ota_rtos_split_path(full_path, dir, sizeof(dir), base, sizeof(base)) != 0) {
		divoom_print("ota_rtos: invalid path %s\n", full_path);
		return -1;
	}

	if (access(full_path, F_OK) != 0) {
		divoom_print("ota_rtos: file not found %s\n", full_path);
		return -1;
	}

#if DIVOOM_BUILD_RTOS_DEVICE
	if (divoom_ota_server_get_show_percent())
		divoom_lvgl_device_update_begin_flash();
#endif

	/* 刷片前 mute 杰理媒体/通话门，不 pause。走现有 UART 发送（已有 tx 锁），
	 * 成功会 reboot；失败再 unmute，避免一直静音。 */
	divoom_jl_audio_gates_mute_set(1);

	dir_buf = dir;
	sunxi_update_set_package_filename(base);
	divoom_database_manage_close(NULL);
	divoom_print("ota_rtos: UPDATE_CMD_START dir=%s file=%s\n", dir, base);
	ret = UPDATE_MIoctrl(UPDATE_CMD_START, dir_buf);
	if (ret != EPDK_OK) {
		divoom_print("ota_rtos: UPDATE_CMD_START fail ret=%ld for %s\n", ret, full_path);
		sunxi_update_reset_package_filename();
		divoom_ota_rtos_resume_after_flash_failure();
		return -1;
	}
	s_rtos_ota_flash_started = 1;

	while (1) {
		divoom_system_delay_ms(1000);
		status = (int)UPDATE_MIoctrl(UPDATE_CMD_CHCEK_STATUS, 0);
		prog = (int)UPDATE_MIoctrl(UPDATE_CMD_CHECK_PROG, 0);
		if (prog != last_prog) {
			last_prog = prog;
			divoom_print("ota_rtos: flash prog=%d status=%d\n", prog, status);
			divoom_ota_rtos_report_progress(prog);
		}
		if (status == UPDATE_STATUS_UPDATE_FINSH) {
			divoom_print("ota_rtos: flash finish, reboot\n");
			sunxi_update_reset_package_filename();
			divoom_ota_rtos_report_progress(100);
			divoom_ota_rtos_mute_iis_in_before_reboot();
			divoom_port_sync();
			divoom_system_delay_ms(500);
			if (divoom_port_sync() == 0) (void)divoom_boot_health_clean();
			cmd_reboot(1, NULL);
			divoom_print("ota_rtos: reboot returned\n");
			divoom_ota_rtos_resume_after_flash_failure();
			return -1;
		}
		if (status == UPDATE_STATUS_UPDATE_FAIL) {
			divoom_print("ota_rtos: flash failed status=%d\n", status);
			sunxi_update_reset_package_filename();
			divoom_ota_rtos_resume_after_flash_failure();
			return -1;
		}
	}
}

int divoom_ota_rtos_flash_has_started(void)
{
	return s_rtos_ota_flash_started ? 1 : 0;
}

int divoom_ota_rtos_is_running(void)
{
	return s_rtos_ota_running ? 1 : 0;
}

static void *divoom_ota_rtos_task_main(void *param)
{
	divoom_ota_rtos_task_param_t *task = (divoom_ota_rtos_task_param_t *)param;
	int ret;

	if (!task) {
		s_rtos_ota_running = 0;
		return NULL;
	}

#ifdef DIVOOM_OTA_HAS_UPDATE_SESSION
	/* Reserve before staging changes, database closure, or UI/audio effects. */
	if (sunxi_update_reserve_session() != EPDK_OK) {
		divoom_print("ota_rtos: another firmware update is active\n");
		ret = -1;
		goto complete;
	}
#endif
	if (task->prepare_from_staging) {
		int is_zip = divoom_ota_is_zip_file(task->staging_path);
#if DIVOOM_BUILD_RTOS_DEVICE
		divoom_ota_prepare_progress_fn prepare_progress = NULL;

		if (is_zip) {
			if (divoom_ota_server_get_show_percent())
				divoom_lvgl_device_update_begin_unzip();
			prepare_progress = divoom_ota_rtos_ui_zip_progress;
		}
		ret = divoom_ota_rtos_prepare_package(task->staging_path, task->full_path,
						      prepare_progress, NULL);
#else
		ret = divoom_ota_rtos_prepare_package(task->staging_path, task->full_path,
						      NULL, NULL);
#endif
		if (ret != 0) {
#if DIVOOM_BUILD_RTOS_DEVICE
			if (divoom_ota_server_get_show_percent())
				divoom_lvgl_device_update_end_ota_progress();
#endif
			divoom_port_remove_file(task->full_path);
			divoom_ota_rtos_remove_staging_if_present();
			divoom_ota_server_set_update_status(0);
			divoom_main_push_model_event_id(DIVOOM_MODEL_OTA,
							DIVOOM_OTA_SERVER_SRV_UPDATE_FAIL);
#ifdef DIVOOM_OTA_HAS_UPDATE_SESSION
			UPDATE_MIoctrl(UPDATE_CMD_STOP, NULL);
#endif
			divoom_system_free(task);
			s_rtos_ota_running = 0;
			return NULL;
		}
		divoom_ota_rtos_remove_staging_if_present();
		divoom_ota_server_set_staged_firmware(task->full_path);
	}

	ret = divoom_ota_rtos_apply_package(task->full_path);
#ifdef DIVOOM_OTA_HAS_UPDATE_SESSION
complete:
#endif
	if (ret != 0) {
#if DIVOOM_BUILD_RTOS_DEVICE
		if (divoom_ota_server_get_show_percent())
			divoom_lvgl_device_update_end_ota_progress();
#endif
		divoom_ota_server_set_update_status(0);
		if (divoom_product_test_is_active())
			divoom_product_test_on_ota_result(DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR);
		else
			divoom_main_push_model_event_id(DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_SRV_UPDATE_FAIL);
	}

#ifdef DIVOOM_OTA_HAS_UPDATE_SESSION
	UPDATE_MIoctrl(UPDATE_CMD_STOP, NULL);
#endif
	divoom_system_free(task);
	s_rtos_ota_running = 0;
	return NULL;
}

int divoom_ota_rtos_start_from_staging(const char *staging_path, const char *fex_path)
{
	divoom_ota_rtos_task_param_t *task;

#if DIVOOM_BUILD_SIMULATOR
	(void)staging_path;
	(void)fex_path;
	return -1;
#else
	if (!staging_path || !fex_path || !staging_path[0] || !fex_path[0])
		return -1;
	if (s_rtos_ota_running) {
		divoom_print("ota_rtos: start_from_staging skip, already running\n");
		return -1;
	}
#ifdef DIVOOM_TIMES_VIBE
	if (divoom_ota_jl_is_upgrading()) {
		divoom_print("ota_rtos: start_from_staging skip, JL upgrading\n");
		return -1;
	}
#endif

	divoom_port_mkdir_p(DIVOOM_OTA_RTOS_STAGING_DIR);

	task = divoom_system_malloc(sizeof(*task));
	if (!task)
		return -1;
	memset(task, 0, sizeof(*task));
	strncpy(task->staging_path, staging_path, sizeof(task->staging_path) - 1);
	strncpy(task->full_path, fex_path, sizeof(task->full_path) - 1);
	task->prepare_from_staging = 1;
	return divoom_ota_rtos_spawn_task(task);
#endif
}

int divoom_ota_rtos_start_file(const char *full_path)
{
	divoom_ota_rtos_task_param_t *task;

#if DIVOOM_BUILD_SIMULATOR
	(void)full_path;
	return -1;
#else
	if (!full_path || !full_path[0])
		return -1;
	/* 仅用 s_rtos_ota_running 防重入。update_status 在 PushUpdate 打开升级页时
	 * 已被置 1，不能再用来拦截 start_file，否则永远起不了刷写任务。 */
	if (s_rtos_ota_running) {
		divoom_print("ota_rtos: start_file skip, already running %s\n", full_path);
		return -1;
	}
#ifdef DIVOOM_TIMES_VIBE
	if (divoom_ota_jl_is_upgrading()) {
		divoom_print("ota_rtos: start_file skip, JL upgrading\n");
		return -1;
	}
#endif

	divoom_port_mkdir_p(DIVOOM_OTA_RTOS_STAGING_DIR);

	task = divoom_system_malloc(sizeof(*task));
	if (!task)
		return -1;
	memset(task, 0, sizeof(*task));
	strncpy(task->full_path, full_path, sizeof(task->full_path) - 1);
	task->prepare_from_staging = 0;
	return divoom_ota_rtos_spawn_task(task);
#endif
}

int divoom_ota_rtos_download_prepare_and_start(const char *url, const char *sha1_expect,
					       int show_percent,
					       void (*dl_progress)(void *user_data,
								   uint32 read_len,
								   uint32 tol_len),
					       void *dl_progress_ud)
{
#if DIVOOM_BUILD_SIMULATOR
	(void)url;
	(void)sha1_expect;
	(void)show_percent;
	(void)dl_progress;
	(void)dl_progress_ud;
	return -1;
#else
	char staging_path[256];
	char fex_path[256];

	if (!url || !url[0])
		return -1;

	divoom_ota_rtos_build_staging_download_path(staging_path, sizeof(staging_path));
	divoom_ota_rtos_build_save_path(url, fex_path, sizeof(fex_path));

	if (!divoom_ota_rtos_download_staging(url, sha1_expect, show_percent,
					      dl_progress, dl_progress_ud))
		return -1;

	return divoom_ota_rtos_start_from_staging(staging_path, fex_path);
#endif
}

int divoom_ota_rtos_download_and_prepare(const char *url, const char *sha1_expect,
					 int show_percent,
					 void (*dl_progress)(void *user_data, uint32 read_len,
							     uint32 tol_len),
					 void *dl_progress_ud)
{
	char staging_path[256];
	char fex_path[256];

#if DIVOOM_BUILD_SIMULATOR
	(void)url;
	(void)sha1_expect;
	(void)show_percent;
	(void)dl_progress;
	(void)dl_progress_ud;
	return 0;
#else
	if (!url || !url[0])
		return 0;

	divoom_ota_rtos_build_staging_download_path(staging_path, sizeof(staging_path));
	divoom_ota_rtos_build_save_path(url, fex_path, sizeof(fex_path));

	if (!divoom_ota_rtos_download_staging(url, sha1_expect, show_percent,
					      dl_progress, dl_progress_ud))
		return 0;

	if (divoom_ota_rtos_prepare_package_blocking(staging_path, fex_path) != 0) {
		divoom_port_remove_file(fex_path);
		divoom_ota_rtos_remove_staging_if_present();
		return 0;
	}

	divoom_ota_rtos_remove_staging_if_present();
	divoom_ota_server_set_staged_firmware(fex_path);
	divoom_print("ota_rtos: firmware staged %s\n", fex_path);
	return 1;
#endif
}

int divoom_ota_rtos_download_and_start(const char *url, const char *save_full_path,
				       const char *sha1_expect)
{
	(void)save_full_path;
	return divoom_ota_rtos_download_prepare_and_start(url, sha1_expect, 1, NULL, NULL);
}
