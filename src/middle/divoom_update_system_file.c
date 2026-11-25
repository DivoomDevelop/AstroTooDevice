#if !defined(_WIN32)
#include <sys/errno.h>
#endif
#include "divoom.h"
#include "divoom_ota_server.h"
#include "divoom_ota_jl.h"
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_ota_rtos.h"
#endif
#include <errno.h>
#include <strings.h>
#if defined(DIVOOM_SIMULATOR) && defined(_WIN32)
#include <openssl/evp.h>
#endif

#define DIVOOM_SYSTEM_CRC_FILE_PATH          "/usr/file_crc.bin"
#define DIVOOM_SYSTEM_CRC_VERSION_PATH       "/usr/file_crc_version.txt"
#define DIVOOM_SYSTEM_CRC_BASE_TMP_PATH      DIVOOM_TMP_DIR "/file_crc_base.bin"
#define DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH    DIVOOM_TMP_DIR "/file_crc_online.bin"
#define DIVOOM_SYSTEM_CRC_CHECK_CACHE_PATH   DIVOOM_TMP_DIR "/file_crc_check.bin"
#define DIVOOM_SYSTEM_CRC_VERSION_TMP_PATH   DIVOOM_TMP_DIR "/file_crc_version_online.txt"
#define DIVOOM_SYSTEM_FILE_STAGE_PATH        "/userdata/.divoom_system_update"
#define DIVOOM_SYSTEM_FILE_ROOT_PATH         "/usr"
#define DIVOOM_SYSTEM_FILE_PATH_MAX          512
#define DIVOOM_SYSTEM_FILE_NET_PATH_MAX      256
#define DIVOOM_SYSTEM_FILE_RETRY_MS          5000
#define DIVOOM_SYSTEM_FILE_OFFLINE_MS        5000U
#define DIVOOM_SYSTEM_FILE_TIMEOUT_MS        (10U * 60U * 1000U)
#define DIVOOM_SYSTEM_FILE_BASE_PATH_FILE    "AstroToo/usr_original_guide.bin"
#define DIVOOM_SYSTEM_FILE_LAST_PATH_FILE    "AstroToo/usr/file_crc.bin"

typedef struct _divoom_update_system_file_ctrl {
	int local_version;
	int downloading;
    int health_check; /* 1: active reader, 2: finished reader awaiting maintenance. */
	int progress_stage;
	int progress_current;
	int progress_total;
	int pending_valid;
	void *firmware_pending; /* Owned original OTA payload from an upgrade command. */
	int firmware_event;
	int firmware_requires_update;
	volatile int stop_requested;
	int cancel_requested; /* Only a new settings-sync request may clear cancellation. */
	int response_expected; /* One response is authorized by SET_ALL_MODE. */
    uint32 round_started_ms; /* Budgets span all retries, only while downloading. */
    uint32 offline_started_ms;
    int offline_waiting;
	divoom_device_get_update_sys_file_respone_info pending;
	void *lock;
	void *download_sem;
	void *task_handle;
} divoom_update_system_file_ctrl, *Pdivoom_update_system_file_ctrl;

typedef enum _divoom_system_file_action {
	DIVOOM_SYSTEM_FILE_COUNT = 0,
	DIVOOM_SYSTEM_FILE_DOWNLOAD,
	DIVOOM_SYSTEM_FILE_DELETE,
} divoom_system_file_action;

typedef struct _divoom_system_file_apply_ctx {
	divoom_system_file_action action;
	const char *base_manifest;
	int file_total;
	int file_done;
	int failed;
} divoom_system_file_apply_ctx;

static Pdivoom_update_system_file_ctrl pdivoom_update_system_file_handle;



static int divoom_update_system_file_firmware_busy(void)
{
    if (divoom_ota_server_get_update_status())
        return 1;
#ifdef DIVOOM_TIMES_VIBE
    if (divoom_ota_jl_is_upgrading())
        return 1;
#endif
#if DIVOOM_BUILD_RTOS_DEVICE
    if (divoom_ota_rtos_is_running() || divoom_ota_rtos_flash_has_started())
        return 1;
#endif
    return 0;
}

/* Match the receiving OTA handlers; no-update replies keep their normal side effects. */
static int divoom_update_system_file_firmware_requires_update(void *firmware_info, int ota_event)
{
    const divoom_general_json_get_update_file_info_respone *info = firmware_info;
    if (ota_event == DIVOOM_OTA_SERVER_SRV_UPDATE)
        return divoom_main_get_boot_ab_recovery_needed() ||
               info->version > DIVOOM_SOFTWARE_VERSION;
#ifdef DIVOOM_TIMES_VIBE
    if (ota_event == DIVOOM_OTA_SERVER_JL_SRV_UPDATE) {
        uint32 local_version = divoom_ota_jl_get_local_version();
        return !divoom_ota_jl_version_is_valid(local_version) || info->version > local_version;
    }
#endif
    return 0;
}

/* Used only by this updater's HTTP requests and install boundaries. */
static int divoom_update_system_file_should_cancel(void *unused)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    int firmware_busy = divoom_update_system_file_firmware_busy();
#if DIVOOM_BUILD_RTOS_DEVICE
    /* STA_STOP/lost IP clears readiness even if the legacy connected flag stays set. */
    int wifi_online = divoom_wifi_model_is_network_ready();
#else
    int wifi_online = divoom_wifi_model_get_wifi_status();
#endif
    uint32 now;
    const char *reason = NULL;
    int cancel;
    (void)unused;
    if (!handle)
        return firmware_busy;
    divoom_system_get_lock(handle->lock);
    if (firmware_busy || (handle->firmware_pending && handle->firmware_requires_update)) {
        handle->cancel_requested = 1;
        handle->response_expected = 0;
    }
    if (handle->downloading && !handle->stop_requested && !handle->cancel_requested) {
        now = divoom_system_get_tickms();
        if (wifi_online) {
            handle->offline_waiting = 0;
        } else if (!handle->offline_waiting) {
            handle->offline_waiting = 1;
            handle->offline_started_ms = now;
        }
        if ((uint32)(now - handle->round_started_ms) >= DIVOOM_SYSTEM_FILE_TIMEOUT_MS)
            reason = "round timeout";
        else if (handle->offline_waiting &&
            (uint32)(now - handle->offline_started_ms) >= DIVOOM_SYSTEM_FILE_OFFLINE_MS)
            reason = "Wi-Fi offline";
        if (reason) {
            handle->cancel_requested = 1;
            handle->response_expected = 0;
            handle->pending_valid = 0;
        }
    }
    cancel = handle->stop_requested || handle->cancel_requested;
    divoom_system_free_lock(handle->lock);
    if (reason)
        divoom_print("system_file: cancel round: %s\n", reason);
    return cancel;
}

static int divoom_update_system_file_json_is_number(divoom_cJSON *item)
{
	return item && (item->type & 0xff) == divoom_cJSON_Number;
}

static int divoom_update_system_file_read_version(void)
{
	char *file_data;
	divoom_cJSON *root = NULL;
	divoom_cJSON *version_item;
	int file_len = 0;
	int version = 0;

	file_data = divoom_app_com_read_file(DIVOOM_SYSTEM_CRC_VERSION_PATH,
		&file_len);
	if (!file_data || file_len <= 0 || file_len > 128) {
		if (file_data)
			divoom_system_free(file_data);
		return 0;
	}
	root = divoom_cJSON_Parse(file_data);
	divoom_system_free(file_data);
	if (!root)
		return 0;
	version_item = divoom_cJSON_GetObjectItem(root, "version");
	if (divoom_update_system_file_json_is_number(version_item) &&
	    version_item->valueint > 0 &&
	    version_item->valuedouble == (double)version_item->valueint)
		version = version_item->valueint;
	divoom_cJSON_Delete(root);
	return version;
}

static int divoom_update_system_file_manifest_valid(const char *path)
{
	divoom_port_dir_compare_result_t result;

	memset(&result, 0, sizeof(result));
	return divoom_port_dir_manifest_compare(path, path, NULL, NULL,
		&result) >= 0;
}

static int divoom_update_system_file_mk_parent(const char *path)
{
	char parent[DIVOOM_SYSTEM_FILE_PATH_MAX];
	char *slash;
	size_t length;

	if (!path)
		return 0;
	length = strlen(path);
	if (length == 0 || length >= sizeof(parent))
		return 0;
	memcpy(parent, path, length + 1);
	slash = strrchr(parent, '/');
	if (!slash || slash == parent)
		return 1;
	*slash = '\0';
	return divoom_port_mkdir_p(parent) == 0;
}

static int divoom_update_system_file_write_empty(const char *path)
{
	void *file;

	if (!divoom_update_system_file_mk_parent(path))
		return 0;
	file = divoom_port_file_open(path, "wb");
	if (!file)
		return 0;
	return divoom_port_file_close(file) == 0;
}

static int divoom_update_system_file_hex_value(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

#if defined(DIVOOM_SIMULATOR) && defined(_WIN32)
/* Windows 仿真直接读映射文件，避免公共接口尝试调用 Linux sha1sum。 */
static int divoom_update_system_file_sha1_windows(const char *path, char *actual)
{
    EVP_MD_CTX *ctx;
    void *file;
    uint8 buffer[1024];
    uint8 digest[EVP_MAX_MD_SIZE];
    uint32 digest_size = 0;
    size_t length;
    uint32 i;
    int ok;

    file = divoom_port_file_open(path, "rb");
    if (!file)
    {
        return 0;
    }
    ctx = EVP_MD_CTX_new();
    ok = ctx && EVP_DigestInit_ex(ctx, EVP_sha1(), NULL) == 1;
    while (ok && (length = divoom_port_file_read(buffer, 1, sizeof(buffer), file)) > 0)
    {
        ok = EVP_DigestUpdate(ctx, buffer, length) == 1;
    }
    if (ok)
    {
        ok = EVP_DigestFinal_ex(ctx, digest, &digest_size) == 1 && digest_size == 20;
    }
    EVP_MD_CTX_free(ctx);
    divoom_port_file_close(file);
    if (ok)
    {
        for (i = 0; i < digest_size; i++)
        {
            snprintf(actual + i * 2, 3, "%02x", digest[i]);
        }
    }
    return ok;
}
#endif

static int divoom_update_system_file_check_sha1(const char *path,
	const char *expected)
{
	char actual[64];
	size_t i;

	if (!path || !expected || strlen(expected) != 40U)
		return 0;
	memset(actual, 0, sizeof(actual));
#if defined(DIVOOM_SIMULATOR) && defined(_WIN32)
    if (!divoom_update_system_file_sha1_windows(path, actual))
    {
        return 0;
    }
#else
    divoom_app_com_get_file_sha1sum(path, actual);
#endif
	if (strnlen(actual, sizeof(actual)) != 40U)
		return 0;
	for (i = 0; i < 40U; i++) {
		int expected_value = divoom_update_system_file_hex_value(expected[i]);
		int actual_value = divoom_update_system_file_hex_value(actual[i]);

		if (expected_value < 0 || actual_value < 0 ||
		    expected_value != actual_value)
			return 0;
	}
	return 1;
}

static int divoom_update_system_file_commit(const char *source,
	const char *destination)
{
	char new_path[DIVOOM_SYSTEM_FILE_PATH_MAX];
	char old_path[DIVOOM_SYSTEM_FILE_PATH_MAX];
	int had_destination;

	if (!source || !destination ||
	    snprintf(new_path, sizeof(new_path), "%s.divoom_new", destination) >=
		(int)sizeof(new_path) ||
	    snprintf(old_path, sizeof(old_path), "%s.divoom_old", destination) >=
		(int)sizeof(old_path) ||
	    !divoom_update_system_file_mk_parent(destination))
		return 0;
	/* A resource is downloaded beside its destination, so installation needs no copy. */
	if (strcmp(source, new_path) != 0) {
		divoom_port_remove_file(new_path);
		if (divoom_port_copy_file(source, new_path) != 0)
			return 0;
	}
	divoom_port_remove_file(old_path);
	had_destination = divoom_port_file_exists(destination);
	if (had_destination &&
	    divoom_port_rename_file(destination, old_path) != 0) {
		int saved_errno = errno ? errno : EIO;
		divoom_port_remove_file(new_path);
		errno = saved_errno;
		return 0;
	}
	if (divoom_port_rename_file(new_path, destination) != 0) {
		int saved_errno = errno ? errno : EIO;
		divoom_port_remove_file(new_path);
		if (had_destination)
			divoom_port_rename_file(old_path, destination);
		errno = saved_errno;
		return 0;
	}
	if (had_destination)
		divoom_port_remove_file(old_path);
	return 1;
}

static int divoom_update_system_file_write_version(int version)
{
	char text[32];
	void *file;
	int length;
	int ok;
	int actual_version;

	length = snprintf(text, sizeof(text), "{\"version\":%d}\n", version);
	if (length <= 0 || length >= (int)sizeof(text))
		return 0;
	divoom_port_remove_file(DIVOOM_SYSTEM_CRC_VERSION_TMP_PATH);
	file = divoom_port_file_open(DIVOOM_SYSTEM_CRC_VERSION_TMP_PATH, "wb");
	if (!file)
		return 0;
	ok = divoom_port_file_write(text, 1, (size_t)length, file) ==
		(size_t)length;
	if (ok && divoom_port_file_sync(file) != 0)
	{
		ok = 0;
	}
	if (divoom_port_file_close(file) != 0)
		ok = 0;
	if (!ok)
		return 0;
	if (!divoom_update_system_file_commit(DIVOOM_SYSTEM_CRC_VERSION_TMP_PATH,
		DIVOOM_SYSTEM_CRC_VERSION_PATH))
	{
		divoom_print("system_file: version commit failed path=%s expected=%d\n",
			DIVOOM_SYSTEM_CRC_VERSION_PATH, version);
		return 0;
	}
	/* 元数据永远最后单独提交，并在同步后回读，不能仅凭下载完成就报告升级成功。 */
	divoom_port_sync();
	actual_version = divoom_update_system_file_read_version();
	divoom_print("system_file: version committed path=%s expected=%d actual=%d\n",
		DIVOOM_SYSTEM_CRC_VERSION_PATH, version, actual_version);
	return actual_version == version;
}

static int divoom_update_system_file_bookkeeping_path(const char *path)
{
	return path && (!strcmp(path, "file_crc.bin") ||
		!strcmp(path, "file_crc_version.txt") ||
		!strcmp(path, "file_crc.bin.divoom_new") ||
		!strcmp(path, "file_crc.bin.divoom_old") ||
		!strcmp(path, "file_crc_version.txt.divoom_new") ||
		!strcmp(path, "file_crc_version.txt.divoom_old"));
}

static void divoom_update_system_file_set_progress(int stage, int current, int total)
{
	Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
	int changed;
	if (!handle)
		return;
	divoom_system_get_lock(handle->lock);
	changed = handle->progress_stage != stage;
	handle->progress_stage = stage;
	if (current >= 0)
		handle->progress_current = current;
	if (total >= 0)
		handle->progress_total = total;
	current = handle->progress_current;
	total = handle->progress_total;
	divoom_system_free_lock(handle->lock);
	if (changed)
		divoom_print("system_file: stage=%d progress=%d/%d\n", stage, current, total);
}

divoom_system_file_stage divoom_update_system_file_get_progress(int *current, int *total)
{
	Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
	divoom_system_file_stage stage = DIVOOM_SYSTEM_FILE_STAGE_IDLE;
	if (current) *current = 0;
	if (total) *total = 0;
	if (!handle)
		return stage;
	divoom_system_get_lock(handle->lock);
	stage = (divoom_system_file_stage)handle->progress_stage;
	if (current) *current = handle->progress_current;
	if (total) *total = handle->progress_total;
	divoom_system_free_lock(handle->lock);
	return stage;
}

static void divoom_update_system_file_progress(
	divoom_system_file_apply_ctx *ctx)
{
	int shown;

	ctx->file_done++;
	divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_DOWNLOAD,
		ctx->file_done, ctx->file_total);
	shown = ctx->file_done;
	/* Keep the page active until installation and version commit both finish. */
	if (shown >= ctx->file_total)
		shown = ctx->file_total - 1;
	if (shown < 0)
		shown = 0;
	divoom_download_aliyun_file_set_app_last_info(ctx->file_total, shown);
}

static int divoom_update_system_file_change(
	const divoom_port_dir_change_t *change, void *user_data)
{
	divoom_system_file_apply_ctx *ctx =
		(divoom_system_file_apply_ctx *)user_data;
	char local_path[DIVOOM_SYSTEM_FILE_PATH_MAX];
	char stage_path[DIVOOM_SYSTEM_FILE_PATH_MAX];
	char server_path[DIVOOM_SYSTEM_FILE_NET_PATH_MAX];
	divoom_port_stat_t stat_info;
	uint64_t expected_size = 0;
	int download_change;
	int delete_change;
	int staged = 0;

	/* 版本/清单元数据不作为资源下载、删除或安装，版本号由成功事务最后写入。 */
	if (!ctx || !change || divoom_update_system_file_bookkeeping_path(change->relative_path))
	{
		return 0;
	}
	if (divoom_update_system_file_should_cancel(NULL)) {
		ctx->failed = 1;
		return 1;
	}
	download_change = change->type == DIVOOM_PORT_DIR_CHANGE_ADDED ||
		change->type == DIVOOM_PORT_DIR_CHANGE_MODIFIED;
	delete_change = change->type == DIVOOM_PORT_DIR_CHANGE_REMOVED;
	if (ctx->action == DIVOOM_SYSTEM_FILE_COUNT) {
		if (download_change)
		{
			ctx->file_total++;
		}
		return 0;
	}
	if (snprintf(local_path, sizeof(local_path), "%s/%s",
		DIVOOM_SYSTEM_FILE_ROOT_PATH, change->relative_path) >=
	    (int)sizeof(local_path) ||
	    snprintf(stage_path, sizeof(stage_path), "%s.divoom_new", local_path) >=
	    (int)sizeof(stage_path))
		goto failed;

	if (ctx->action == DIVOOM_SYSTEM_FILE_DOWNLOAD && download_change) {
		uint32 started = divoom_system_get_tickms();
		uint32 downloaded;
		uint32 verified;
		expected_size = change->new_size;
		/* A retry/power loss may leave this file fully installed but no version yet. */
		if (divoom_port_file_verify(local_path, expected_size, change->new_crc32, 0) == 0) {
			divoom_update_system_file_progress(ctx);
			divoom_print("system_file: already installed %d/%d %s\n",
				ctx->file_done, ctx->file_total, change->relative_path);
			return 0;
		}
		divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_DOWNLOAD,
			ctx->file_done, ctx->file_total);
		if (!divoom_update_system_file_mk_parent(stage_path))
			goto failed;
		staged = 1;
		divoom_port_remove_file(stage_path);
		if (expected_size == 0) {
			if (!divoom_update_system_file_write_empty(stage_path))
				goto failed;
		} else if (snprintf(server_path, sizeof(server_path), "%s%s",
			DIVOOM_SYSTEM_NET_FILE_PATH, change->relative_path) >=
			   (int)sizeof(server_path) ||
			   divoom_http_client_down_file_with_exit(stage_path, server_path,
				1, NULL, divoom_update_system_file_should_cancel, NULL) != 1) {
			goto failed;
		}
		downloaded = divoom_system_get_tickms();
		if (divoom_update_system_file_should_cancel(NULL) ||
			divoom_port_file_verify(stage_path, expected_size, change->new_crc32, 0) != 0) {
			divoom_print("system_file: resource size/CRC failed %s\n", change->relative_path);
			goto failed;
		}
		verified = divoom_system_get_tickms();
		divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_INSTALL,
			ctx->file_done, ctx->file_total);
		if (!divoom_update_system_file_commit(stage_path, local_path))
			goto failed;
		divoom_update_system_file_progress(ctx);
		divoom_print("system_file: installed %d/%d %s download_ms=%u verify_ms=%u install_ms=%u\n",
			ctx->file_done, ctx->file_total, change->relative_path,
			(unsigned)(downloaded - started), (unsigned)(verified - downloaded),
			(unsigned)(divoom_system_get_tickms() - verified));
	} else if (ctx->action == DIVOOM_SYSTEM_FILE_DELETE && delete_change) {
		if (divoom_port_stat(local_path, &stat_info) == 0 &&
		    (stat_info.is_dir ? divoom_port_remove_tree(local_path) :
		     divoom_port_remove_file(local_path)) != 0)
			goto failed;
	}
	return 0;

failed:
	if (staged)
		divoom_port_remove_file(stage_path);
	divoom_print("system_file: action %d failed for %s\n",
		(int)ctx->action, change->relative_path);
	ctx->failed = 1;
	return 1;
}

static int divoom_update_system_file_compare(const char *new_manifest,
	divoom_system_file_apply_ctx *ctx)
{
	divoom_port_dir_compare_result_t result;
	int rc;

	memset(&result, 0, sizeof(result));
	rc = divoom_port_dir_manifest_compare(ctx->base_manifest,
		new_manifest, divoom_update_system_file_change, ctx, &result);
	if (rc < 0) {
		divoom_print("system_file: manifest compare failed rc=%d\n", rc);
		return 0;
	}
	return 1;
}

/* 仿真集成验收可指向本地 HTTP fixture；普通运行与真机继续使用服务器地址。 */
static int divoom_update_system_file_download_manifest(const char *local, const char *remote)
{
#ifdef DIVOOM_SIMULATOR
    const char *server = getenv("DIVOOM_SIM_SYSTEM_FILE_SERVER");
    char url[512];
    if (server && server[0])
    {
        if (snprintf(url, sizeof(url), "%s%s", server, remote) >= (int)sizeof(url))
        {
            return 0;
        }
        return divoom_http_client_down_file_with_exit((char *)local, url, 0, NULL,
            divoom_update_system_file_should_cancel, NULL);
    }
#endif
    return divoom_http_client_down_file_with_exit((char *)local, (char *)remote, 1, NULL,
        divoom_update_system_file_should_cancel, NULL);
}

/* 无可信版本时，只使用原始清单；检查不覆盖本地 CRC/版本，确认安装复用缓存。 */
static int divoom_update_system_file_prepare_base_crc(int allow_download)
{
	if (divoom_update_system_file_should_cancel(NULL))
		return 0;
	if (divoom_update_system_file_manifest_valid(DIVOOM_SYSTEM_CRC_BASE_TMP_PATH))
		return 1;
	if (!allow_download)
		return 0;
	divoom_print("system_file: no trusted local version, download base guide %s\n",
		DIVOOM_SYSTEM_FILE_BASE_PATH_FILE);
	divoom_port_remove_file(DIVOOM_SYSTEM_CRC_BASE_TMP_PATH);
	if (divoom_update_system_file_download_manifest(DIVOOM_SYSTEM_CRC_BASE_TMP_PATH,
		DIVOOM_SYSTEM_FILE_BASE_PATH_FILE) != 1 ||
		!divoom_update_system_file_manifest_valid(DIVOOM_SYSTEM_CRC_BASE_TMP_PATH))
	{
		divoom_port_remove_file(DIVOOM_SYSTEM_CRC_BASE_TMP_PATH);
		divoom_print("system_file: base crc download/check failed\n");
		return 0;
	}
	return 1;
}

/* 清单按内容 SHA1 校验后复用，保留磁盘缓存供重进页面和确认安装使用。仅 worker 调用。 */
static int divoom_update_system_file_prepare_check_manifest(
	const divoom_device_get_update_sys_file_respone_info *info)
{
	const char *path = DIVOOM_SYSTEM_CRC_CHECK_CACHE_PATH;
	const char *stage = "download";
	int result;

	if (divoom_update_system_file_should_cancel(NULL))
		return 0;
	if (divoom_port_file_exists(path) &&
		divoom_update_system_file_check_sha1(path, info->sha1_info) &&
		divoom_update_system_file_manifest_valid(path))
	{
		divoom_print("system_file: manifest cache hit version=%d\n", info->file_version);
		return 1;
	}
	divoom_port_remove_file(path);
	divoom_print("system_file: manifest download version=%d file=%s\n",
		info->file_version, info->file_name);
	result = divoom_update_system_file_download_manifest(path, info->file_name);
	if (result != 1)
	{
		divoom_print("system_file: manifest download result=%d http=%d\n",
			result, divoom_http_client_last_http_status());
		goto failed;
	}
	stage = "sha1";
	if (!divoom_update_system_file_check_sha1(path, info->sha1_info))
	{
		goto failed;
	}
	stage = "format";
	if (!divoom_update_system_file_manifest_valid(path))
	{
		goto failed;
	}
	divoom_print("system_file: manifest cached version=%d\n", info->file_version);
	return 1;
failed:
	divoom_print("system_file: manifest check failed version=%d stage=%s\n",
		info->file_version, stage);
	divoom_port_remove_file(path);
	return 0;
}

static int divoom_update_system_file_apply(
	const divoom_device_get_update_sys_file_respone_info *info)
{
	divoom_system_file_apply_ctx ctx;
	int local_version;
	int progress_total;
	uint32 commit_started;

	if (divoom_update_system_file_should_cancel(NULL) || !info ||
	    info->file_version <= 0 || !info->file_name[0])
		return 0;
	local_version = divoom_update_system_file_manifest_valid(
		DIVOOM_SYSTEM_CRC_FILE_PATH) ?
		divoom_update_system_file_read_version() : 0;
	divoom_print("system_file: compare version local=%d server=%d\n",
		local_version, info->file_version);
	if (info->file_version <= local_version)
	{
		divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_DONE, 1, 1);
		return 1;
	}

	/* The new guide remains temporary until every payload is installed. */
	divoom_port_remove_file(DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH);
	divoom_port_remove_tree(DIVOOM_SYSTEM_FILE_STAGE_PATH);
	divoom_download_aliyun_file_set_app_last_info(1, 0);
	divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_PREPARE, 0, 1);
	/* Worker prepares the checked manifest before installation; retry if it was lost. */
	if (!divoom_update_system_file_check_sha1(DIVOOM_SYSTEM_CRC_CHECK_CACHE_PATH, info->sha1_info) ||
		!divoom_update_system_file_manifest_valid(DIVOOM_SYSTEM_CRC_CHECK_CACHE_PATH))
	{
		divoom_print("system_file: install manifest unavailable, retry manifest preparation\n");
		divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_FAILED, -1, -1);
		return 0;
	}
	if (divoom_port_copy_file(DIVOOM_SYSTEM_CRC_CHECK_CACHE_PATH,
			DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH) != 0 ||
	    divoom_update_system_file_should_cancel(NULL) ||
	    !divoom_update_system_file_check_sha1(
		DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH, info->sha1_info) ||
	    !divoom_update_system_file_manifest_valid(
		DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH)) {
		divoom_print("system_file: crc guide download/check failed\n");
		goto failed;
	}

	memset(&ctx, 0, sizeof(ctx));
	ctx.action = DIVOOM_SYSTEM_FILE_COUNT;
	ctx.base_manifest = local_version > 0 ? DIVOOM_SYSTEM_CRC_FILE_PATH :
		DIVOOM_SYSTEM_CRC_BASE_TMP_PATH;
	if (local_version <= 0 && !divoom_update_system_file_prepare_base_crc(1))
	{
		divoom_print("system_file: base manifest unavailable, retry manifest preparation\n");
		divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_RETRY, -1, -1);
		return 0;
	}
	if (!divoom_update_system_file_compare(
		DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH, &ctx))
		goto failed;
	progress_total = ctx.file_total > 0 ? ctx.file_total : 1;
	ctx.file_total = progress_total;
	divoom_download_aliyun_file_set_app_last_info(progress_total, 0);
	divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_DOWNLOAD, 0, progress_total);

	/* Remove obsolete entries first, including file/directory type replacements. */
	divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_INSTALL, 0, progress_total);
	ctx.action = DIVOOM_SYSTEM_FILE_DELETE;
	if (divoom_update_system_file_should_cancel(NULL) ||
	    !divoom_update_system_file_compare(
		DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH, &ctx) || ctx.failed)
		goto failed;
	/* Each callback downloads, verifies and installs one file before the next HTTP. */
	ctx.action = DIVOOM_SYSTEM_FILE_DOWNLOAD;
	if (divoom_update_system_file_should_cancel(NULL) ||
	    !divoom_update_system_file_compare(
		DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH, &ctx) || ctx.failed)
		goto failed;
	/* Commit the guide first and the JSON version marker last. */
	commit_started = divoom_system_get_tickms();
	divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_COMMIT, progress_total, progress_total);
	if (divoom_update_system_file_should_cancel(NULL) ||
	    !divoom_update_system_file_commit(
		DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH,
		DIVOOM_SYSTEM_CRC_FILE_PATH) ||
	    !divoom_update_system_file_write_version(info->file_version)) {
		divoom_print("system_file: crc/version commit failed\n");
		goto failed;
	}
	divoom_port_sync();
	divoom_print("system_file: crc/version commit elapsed_ms=%u\n",
		(unsigned)(divoom_system_get_tickms() - commit_started));
	divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_DONE, progress_total, progress_total);
	divoom_download_aliyun_file_set_app_last_info(progress_total,
		progress_total);
	divoom_print("system_file: update complete version=%d files=%d\n",
		info->file_version, ctx.file_total);
	divoom_port_remove_file(DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH);
	divoom_port_remove_file(DIVOOM_SYSTEM_CRC_VERSION_TMP_PATH);
	divoom_port_remove_tree(DIVOOM_SYSTEM_FILE_STAGE_PATH);
	return 1;

failed:
	divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_RETRY, -1, -1);
	divoom_port_remove_file(DIVOOM_SYSTEM_CRC_UPDATE_TMP_PATH);
	divoom_port_remove_file(DIVOOM_SYSTEM_CRC_VERSION_TMP_PATH);
	divoom_port_remove_tree(DIVOOM_SYSTEM_FILE_STAGE_PATH);
	return 0;
}



/* SET_ALL_MODE arms the response before sending; no other entry may revive a cancelled round. */
int divoom_update_system_file_begin_settings_check(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    int firmware_busy;
    if (!handle) {
        divoom_update_system_file_init();
        handle = pdivoom_update_system_file_handle;
    }
    if (!handle)
        return 0;
    firmware_busy = divoom_update_system_file_firmware_busy();
    divoom_system_get_lock(handle->lock);
    if (handle->stop_requested || firmware_busy ||
        (handle->firmware_pending && handle->firmware_requires_update)) {
        handle->response_expected = 0;
        divoom_system_free_lock(handle->lock);
        return 0;
    }
    /* Do not reset a cancellation while the previous HTTP/install is draining. */
    if (handle->downloading || handle->pending_valid) {
        divoom_system_free_lock(handle->lock);
        return 0;
    }
    handle->cancel_requested = 0;
    handle->response_expected = 1;
    divoom_system_free_lock(handle->lock);
    return 1;
}

int divoom_update_system_file_start_firmware(void *firmware_info, int ota_event)
{
	Pdivoom_update_system_file_ctrl handle;
	void *previous;
	int version;
	int requires_update;
	if (!firmware_info)
		return 0;
	if (!pdivoom_update_system_file_handle)
		divoom_update_system_file_init();
	handle = pdivoom_update_system_file_handle;
	if (!handle)
		return 0;
    requires_update = divoom_update_system_file_firmware_requires_update(firmware_info, ota_event);
	version = divoom_update_system_file_read_version();
	divoom_system_get_lock(handle->lock);
	if (handle->stop_requested) {
		divoom_system_free_lock(handle->lock);
		return 0;
	}
    /* A late no-update query must not replace an already accepted real upgrade. */
    if (handle->firmware_pending && handle->firmware_requires_update && !requires_update) {
        divoom_system_free_lock(handle->lock);
        return 0;
    }
	previous = handle->firmware_pending;
	handle->firmware_pending = firmware_info;
	handle->firmware_event = ota_event;
	handle->firmware_requires_update = requires_update;
	if (requires_update) {
		handle->cancel_requested = 1;
		handle->response_expected = 0;
	}
	if (!handle->downloading && version <= 0)
		handle->local_version = 0;
	/* Firmware has priority: the worker drains this request and cancels resource work. */
	divoom_system_free_lock(handle->lock);
	if (previous && previous != firmware_info)
		divoom_system_free(previous);
	divoom_print("system_file: firmware queued event=%d requires_update=%d\n", ota_event, requires_update);
	divoom_system_put_sem(handle->download_sem);
	return 1;
}



static void *divoom_update_system_file_task_main(void *param)
{
	Pdivoom_update_system_file_ctrl handle = (Pdivoom_update_system_file_ctrl)param;
	divoom_device_get_update_sys_file_respone_info info;
	int superseded;

	while (1) {
		divoom_system_get_sem(handle->download_sem);
		while (1) {
			int cancelled = divoom_update_system_file_should_cancel(NULL);
			divoom_system_get_lock(handle->lock);
			if (handle->stop_requested || handle->health_check) {
				int stopped = handle->stop_requested;
				handle->downloading = 0;
				divoom_system_free_lock(handle->lock);
				if (stopped) return NULL;
				break;
			}
			if (cancelled && handle->cancel_requested) {
				handle->pending_valid = 0;
				handle->progress_stage = DIVOOM_SYSTEM_FILE_STAGE_IDLE;
			}
			if (!handle->pending_valid) {
				divoom_sys_msg_info firmware_msg = {0};
				firmware_msg.msg_type = 1;
				firmware_msg.dest_id = DIVOOM_MODEL_OTA;
				firmware_msg.msg_id = handle->firmware_event;
				firmware_msg.data = handle->firmware_pending;
				handle->firmware_pending = NULL;
				handle->firmware_requires_update = 0;
				handle->downloading = 0;
				divoom_system_free_lock(handle->lock);
				/* Dispatch unlocked: a synchronous OTA callback may stop this worker. */
				if (firmware_msg.data) divoom_sys_model_crl_msg_push(&firmware_msg);
				break;
			}
			if (handle->pending.file_version <= handle->local_version) {
				handle->pending_valid = 0;
				divoom_system_free_lock(handle->lock);
				continue;
			}
			info = handle->pending;
			handle->pending_valid = 0;
			handle->downloading = 1;
            handle->round_started_ms = divoom_system_get_tickms();
            handle->offline_waiting = 0;
			divoom_system_free_lock(handle->lock);

			divoom_update_system_file_set_progress(DIVOOM_SYSTEM_FILE_STAGE_PREPARE, 0, 1);
            /* Schedule callbacks may synchronously reenter this updater; no updater lock is held. */
            if (!divoom_ota_server_begin_system_file_update())
                divoom_update_system_file_cancel_current();
			/* Initial work and retries use the same prepare/apply path. */
			while (!divoom_update_system_file_should_cancel(NULL)) {
				if (divoom_update_system_file_prepare_check_manifest(&info) &&
					divoom_update_system_file_apply(&info) > 0) {
					int disk_version = divoom_update_system_file_read_version();
					divoom_system_get_lock(handle->lock);
					handle->local_version = disk_version;
					divoom_system_free_lock(handle->lock);
					break;
				}
				if (divoom_update_system_file_should_cancel(NULL)) break;
				divoom_system_get_lock(handle->lock);
				if (handle->stop_requested) {
					divoom_system_free_lock(handle->lock);
					break;
				}
				superseded = handle->pending_valid && handle->pending.file_version > info.file_version;
				divoom_system_free_lock(handle->lock);
				if (superseded) break;
				divoom_system_delay_ms(DIVOOM_SYSTEM_FILE_RETRY_MS);
			}
            /* Publish idle before cancel can synchronously restore another business event. */
            divoom_system_get_lock(handle->lock);
            handle->downloading = 0;
            divoom_system_free_lock(handle->lock);
            divoom_ota_server_end_system_file_update();
		}
	}
	return NULL;
}

int divoom_update_system_file_begin_health_check(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    int ok;
    if (!handle) return 0;
    divoom_system_get_lock(handle->lock);
    ok = !handle->health_check && !handle->stop_requested && !handle->downloading &&
        !handle->pending_valid && !handle->firmware_pending;
    if (ok) handle->health_check = 1;
    divoom_system_free_lock(handle->lock);
    return ok;
}

void divoom_update_system_file_end_health_check(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    if (!handle) return;
    divoom_system_get_lock(handle->lock);
    handle->health_check = 0;
    divoom_system_free_lock(handle->lock);
    divoom_system_put_sem(handle->download_sem);
}

/* The checker has finished all /usr I/O. Retain the local gate until main
 * either tears down for recovery or rejects the handoff. Queued OTA stays owned
 * by the existing updater and is released by its normal stop path. */
int divoom_update_system_file_hold_health_check_for_repair(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    int held = 0;
    if (!handle) return 0;
    divoom_system_get_lock(handle->lock);
    if (handle->health_check == 1) {
        held = !handle->stop_requested;
        handle->health_check = held ? 2 : 0;
    }
    divoom_system_free_lock(handle->lock);
    return held;
}

int divoom_update_system_file_health_check_repair_ready(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    int ready;
    if (!handle) return 0;
    divoom_system_get_lock(handle->lock);
    ready = handle->health_check == 2 && !handle->downloading && !handle->stop_requested;
    divoom_system_free_lock(handle->lock);
    return ready;
}

void divoom_update_system_file_cancel_health_check_repair(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    int released;
    if (!handle) return;
    divoom_system_get_lock(handle->lock);
    released = handle->health_check == 2;
    if (released) handle->health_check = 0;
    divoom_system_free_lock(handle->lock);
    if (released) divoom_system_put_sem(handle->download_sem);
}

/* Maintenance must exclude readers too; transfer UI keeps using is_downloading. */
int divoom_update_system_file_is_busy(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    if (!handle) return 0;
    divoom_system_get_lock(handle->lock);
    int busy = handle->health_check || handle->downloading || handle->pending_valid || handle->firmware_pending;
    divoom_system_free_lock(handle->lock);
    return busy;
}

void divoom_update_system_file_cancel_current(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    if (!handle) return;
    divoom_system_get_lock(handle->lock);
    handle->cancel_requested = 1;
    handle->response_expected = 0;
    handle->pending_valid = 0;
    divoom_system_free_lock(handle->lock);
    divoom_system_put_sem(handle->download_sem);
}

/* Resource-page ownership excludes queued firmware hints and cancelled work draining. */
int divoom_update_system_file_is_resource_updating(void)
{
    Pdivoom_update_system_file_ctrl handle = pdivoom_update_system_file_handle;
    int active;
    if (!handle) return 0;
    divoom_system_get_lock(handle->lock);
    active = !handle->stop_requested && !handle->cancel_requested &&
        (handle->downloading || handle->pending_valid);
    divoom_system_free_lock(handle->lock);
    return active;
}

int divoom_update_system_file_is_downloading(void)
{
	int downloading;

	if (!pdivoom_update_system_file_handle)
		return 0;
	divoom_system_get_lock(pdivoom_update_system_file_handle->lock);
	downloading = pdivoom_update_system_file_handle->downloading ||
		pdivoom_update_system_file_handle->pending_valid || pdivoom_update_system_file_handle->firmware_pending;
	divoom_system_free_lock(pdivoom_update_system_file_handle->lock);
	return downloading;
}

void divoom_update_system_file_stop(void)
{
	Pdivoom_update_system_file_ctrl handle =
		pdivoom_update_system_file_handle;
	int downloading;
	int i;
	void *firmware = NULL;

	if (handle) {
		divoom_system_get_lock(handle->lock);
		handle->stop_requested = 1;
		handle->response_expected = 0;
        /* A finished reader belongs to this teardown; a live reader stays busy. */
        if (handle->health_check == 2) handle->health_check = 0;
		handle->pending_valid = 0;
		firmware = handle->firmware_pending;
		handle->firmware_pending = NULL;
		handle->firmware_requires_update = 0;
		divoom_system_free_lock(handle->lock);
		divoom_system_put_sem(handle->download_sem);
	}
	if (firmware)
		divoom_system_free(firmware);
	divoom_http_client_request_abort();
	if (!handle)
		return;
	for (i = 0; i < 50; i++) {
		divoom_system_get_lock(handle->lock);
		downloading = handle->downloading;
		divoom_system_free_lock(handle->lock);
		if (!downloading)
			break;
		divoom_system_delay_ms(100);
	}
}

int divoom_update_system_file_start_pending(void)
{
	Pdivoom_update_system_file_ctrl handle;
	int active;
	int wake;

	if (!pdivoom_update_system_file_handle)
		divoom_update_system_file_init();
	handle = pdivoom_update_system_file_handle;
	if (!handle)
		return 0;
	divoom_system_get_lock(handle->lock);
	active = handle->downloading || handle->pending_valid;
	wake = handle->pending_valid && !handle->downloading;
	divoom_system_free_lock(handle->lock);
	if (wake) {
		divoom_print("system_file: boot screen finished, start pending update\n");
		divoom_system_put_sem(handle->download_sem);
	}
	return active;
}

void divoom_update_system_file_init(void)
{
	Pdivoom_update_system_file_ctrl handle;

	/* 保留既有 worker，收到设置同步事件的更新回包后直接执行安装。 */
	if (pdivoom_update_system_file_handle)
		return;
	handle = (Pdivoom_update_system_file_ctrl)
		divoom_system_malloc(sizeof(*handle));
	if (!handle)
		return;
	memset(handle, 0, sizeof(*handle));
	handle->lock = divoom_system_create_lock();
	handle->download_sem = divoom_system_create_sem(1, 0);
	if (!handle->lock || !handle->download_sem) {
		if (handle->download_sem)
			divoom_system_destroy_sem(handle->download_sem);
		if (handle->lock)
			divoom_system_destroy_lock(handle->lock);
		divoom_system_free(handle);
		return;
	}
	if (divoom_update_system_file_manifest_valid(
		DIVOOM_SYSTEM_CRC_FILE_PATH))
		handle->local_version = divoom_update_system_file_read_version();
	pdivoom_update_system_file_handle = handle;
	handle->task_handle = divoom_system_create_task(
		divoom_update_system_file_task_main, "sys_file_up", handle,
		DIVOOM_TASK_STACK_UPDATE_SYS_FILE);
	if (!handle->task_handle) {
		pdivoom_update_system_file_handle = NULL;
		divoom_system_destroy_sem(handle->download_sem);
		divoom_system_destroy_lock(handle->lock);
		divoom_system_free(handle);
	}
}

int divoom_update_system_file_get_file_version(void)
{
	int version;

	if (!pdivoom_update_system_file_handle)
		divoom_update_system_file_init();
	if (!pdivoom_update_system_file_handle)
		return 0;
	divoom_system_get_lock(pdivoom_update_system_file_handle->lock);
	version = pdivoom_update_system_file_handle->local_version;
	divoom_system_free_lock(pdivoom_update_system_file_handle->lock);
	return version;
}

void divoom_update_system_file_save_sys_file_version(int version)
{
	if (version <= 0 || !divoom_update_system_file_write_version(version))
		return;
	if (!pdivoom_update_system_file_handle)
		divoom_update_system_file_init();
	if (!pdivoom_update_system_file_handle)
		return;
	divoom_system_get_lock(pdivoom_update_system_file_handle->lock);
	pdivoom_update_system_file_handle->local_version = version;
	divoom_system_free_lock(pdivoom_update_system_file_handle->lock);
}

void divoom_update_system_file_add_file(void *para)
{
	Pdivoom_device_get_update_sys_file_respone_info info =
		(Pdivoom_device_get_update_sys_file_respone_info)para;
	Pdivoom_update_system_file_ctrl handle;
	size_t i;
	int valid;
	int local_version;
	int version;
	int firmware_busy;

	valid = info && info->file_version > 0 && info->file_name[0] &&
		strnlen(info->file_name, sizeof(info->file_name)) < sizeof(info->file_name) &&
		strnlen(info->sha1_info, sizeof(info->sha1_info)) == 40U;
	if (valid)
	{
		for (i = 0; i < 40U; i++)
		{
			if (divoom_update_system_file_hex_value(info->sha1_info[i]) < 0)
			{
				valid = 0;
				break;
			}
		}
	}
	handle = pdivoom_update_system_file_handle;
	if (!handle) {
		divoom_print("system_file: updater init failed\n");
		return;
	}
	version = divoom_update_system_file_read_version();
	firmware_busy = divoom_update_system_file_firmware_busy();
	divoom_system_get_lock(handle->lock);
    if (!handle->response_expected) {
        divoom_system_free_lock(handle->lock);
        return;
    }
    /* The first response consumes the request, including no-update/invalid responses. */
    handle->response_expected = 0;
	if (!handle->downloading && version <= 0)
		handle->local_version = 0;
	local_version = handle->local_version;
	/* Health checks defer writes to the same worker; their release wakes it again. */
	if (handle->stop_requested || firmware_busy ||
        (handle->firmware_pending && handle->firmware_requires_update) ||
		(handle->downloading && handle->cancel_requested) ||
		!valid || info->file_version <= local_version ||
		(handle->pending_valid && info->file_version <= handle->pending.file_version))
	{
		divoom_system_free_lock(handle->lock);
		return;
	}
	handle->pending = *info;
	handle->pending_valid = 1;
	divoom_system_free_lock(handle->lock);
	divoom_print("system_file: response queued local=%d server=%d\n", local_version, info->file_version);
	divoom_system_put_sem(handle->download_sem);
}

typedef struct divoom_restore_fault {
    uint64_t size;
    uint32_t crc;
    unsigned attempts, deferred, not_found, finalized;
    char path[DIVOOM_SYSTEM_FILE_PATH_MAX];
} divoom_restore_fault;
#define DIVOOM_RESTORE_FAULT_BYTES (64U * 1024U)
static int restore_status_stage, restore_status_failed, restore_status_skipped;
static int restore_driver_skipped;
int divoom_update_system_file_restore_driver_skipped(void)
{
    return __atomic_load_n(&restore_driver_skipped, __ATOMIC_ACQUIRE);
}
void divoom_update_system_file_restore_status(int *stage, int *failed, int *skipped)
{
    if (stage) *stage = __atomic_load_n(&restore_status_stage, __ATOMIC_ACQUIRE);
    if (failed) *failed = __atomic_load_n(&restore_status_failed, __ATOMIC_ACQUIRE);
    if (skipped) *skipped = __atomic_load_n(&restore_status_skipped, __ATOMIC_ACQUIRE);
}
typedef struct {
	int done;
	int total;
	divoom_system_file_restore_progress progress;
	uint32 current_file_size;
    divoom_cJSON *skipped; /* This pass only; a reboot may probe missing files again. */
    int skipped_count;
    int delete_failed; /* Latched reformat request after exhausted local I/O retries. */
    int stage_failed; /* Persistent staging I/O belongs to userdata, not usr. */
	int (*retry)(int errors);
    int failure; /* Preserve the callback's actual DCRC/I/O failure class. */
    divoom_restore_fault active;
    unsigned char *states;
    unsigned state_index, scan_index;
    unsigned fault_bytes;
    int full_restore, final_skipped;
    int whitebox, whitebox_downloads, whitebox_rescues, whitebox_space_retry;
} divoom_system_file_restore_ctx;

static int divoom_update_system_file_format_reserved(const char *path)
{
    static const char *names[] = {"file_crc.bin", "file_crc_version.txt",
        "file_crc.bin.divoom_new", "file_crc.bin.divoom_old",
        "file_crc_version.txt.divoom_new", "file_crc_version.txt.divoom_old", "file_crc.bin.divoom_base"};
    size_t length, i, j;
    char first[64];
    if (!path || !path[0]) return 1;
    length = strcspn(path, "/");
    /* FAT names are case insensitive and ignore trailing dots/spaces. */
    while (length && (path[length-1] == '.' || path[length-1] == ' ')) --length;
    if (length >= sizeof(first)) return 0;
    for (j = 0; j < length; ++j)
        first[j] = path[j] >= 'A' && path[j] <= 'Z' ? path[j] + ('a'-'A') : path[j];
    first[length] = 0;
    /* Reject short-name aliases of the long metadata/transaction names too. */
    if (!strncmp(first, "file_c~", 7) || !strncmp(first, ".userdata_wifi_recovery", 23) ||
        !strncmp(first, "userda~", 7)) return 1;
    for (i = 0; i < sizeof(names)/sizeof(names[0]); ++i)
        if (!strcmp(first, names[i])) return 1;
    return 0;
}

int divoom_update_system_file_restore_path_is_reserved(const char *relative)
{
    return divoom_update_system_file_format_reserved(relative);
}

static int divoom_update_system_file_format_resource_url(const char *remote, char *url, size_t capacity);

static int divoom_update_system_file_format_entry(const divoom_port_dir_change_t *entry, void *user_data)
{
    int *files = user_data;
    if (divoom_update_system_file_format_reserved(entry->relative_path)) return 1;
    if (entry->type != DIVOOM_PORT_DIR_CHANGE_ADDED) {
        char stage[DIVOOM_SYSTEM_FILE_PATH_MAX], remote[DIVOOM_SYSTEM_FILE_NET_PATH_MAX], url[512];
        /* Reject unsupported downloads before any erase, using the actual
         * downloader identity limit rather than a separate guessed bound. */
        if (*files >= DIVOOM_RESTORE_FAULT_BYTES || entry->old_size > 0x7fffffffU ||
            snprintf(stage, sizeof(stage), "/userdata/.format_restore/%s", entry->relative_path) >= sizeof(stage) ||
            snprintf(remote, sizeof(remote), "%s%s", DIVOOM_SYSTEM_NET_FILE_PATH, entry->relative_path) >= sizeof(remote) ||
            !divoom_update_system_file_format_resource_url(remote, url, sizeof(url)) ||
            !divoom_http_client_resource_file_supported(stage, url, (uint32)entry->old_size, entry->old_crc32)) return 1;
        ++*files;
    }
    return 0;
}

int divoom_update_system_file_validate_format_manifest(const char *manifest)
{
    int files = 0, rc = divoom_port_dir_manifest_prepare_dirs(manifest, "/usr", 0);
    if (rc) return rc;
    rc = divoom_port_dir_manifest_visit_paths(manifest, "/usr",
        divoom_update_system_file_format_entry, &files);
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_CALLBACK || (!rc && !files))
        return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
    return rc;
}

static int divoom_update_system_file_restore_sync(const char *path)
{
    void *fp = divoom_port_file_open(path, "r+b");
    int ok, error = 0;
    if (!fp) return 0;
    errno = 0;
    ok = divoom_port_file_sync(fp) == 0;
    if (!ok) error = errno ? errno : EIO;
    errno = 0;
    if (divoom_port_file_close(fp) != 0) {
        if (!error) error = errno ? errno : EIO;
        ok = 0;
    }
    errno = error;
    return ok;
}

static int divoom_update_system_file_format_download_once(char *local, char *remote)
{
#ifdef DIVOOM_SIMULATOR
	/* 仿真可指定测试 HTTP 源，仍经过同一下载器和清单/文件 CRC 校验。 */
	const char *server = getenv("DIVOOM_SIM_FORMAT_SERVER");
	if (server && server[0]) {
		char url[256];
		if (snprintf(url, sizeof(url), "%s/%s", server, remote) >= sizeof(url)) {
			return 0;
		}
		return divoom_http_client_down_file(local, url, 0, NULL);
	}
#endif
	return divoom_http_client_down_file(local, remote, 1, NULL);
}

static int divoom_update_system_file_format_download(char *local, char *remote, int (*retry)(int errors))
{
	int errors = 0;
	if (retry && !retry(0)) return 0;
	for (;;) {
		int rc = divoom_update_system_file_format_download_once(local, remote);
		int status = divoom_http_client_last_http_status();
		if (rc == 1) {
			rc = divoom_update_system_file_validate_format_manifest(local);
			if (rc == 0) return !retry || retry(0);
			if (rc != DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT) return 0;
		} else if (rc == 0 || (rc == -2 && divoom_wifi_model_get_wifi_status()) ||
		    (divoom_wifi_model_get_wifi_status() && status >= 400 && status != 408 && status != 429 && status < 500)) {
			return 0;
		}
		divoom_print("format_restore: guide retry reason=%s\n",
			rc == DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT ? "CRC/format mismatch" : "temporary network error");
		if (errors < 0x7fffffff) ++errors;
		if (retry && !retry(errors)) return 0;
		divoom_system_delay_ms(2000);
	}
}

static int divoom_update_system_file_format_resource_url(const char *remote, char *url, size_t capacity)
{
#ifdef DIVOOM_SIMULATOR
    const char *test_server = getenv("DIVOOM_SIM_FORMAT_SERVER");
    if (test_server && test_server[0])
        return snprintf(url, capacity, "%s/%s", test_server, remote) < capacity;
#endif
    return snprintf(url, capacity, "%s%s", DIVOOM_SERVER_URL, remote) < capacity;
}

static int divoom_update_system_file_format_resource_cleanup(const char *stage, const char *remote,
    uint32 size, uint32 crc)
{
    char url[512];
    return divoom_update_system_file_format_resource_url(remote, url, sizeof(url)) &&
        divoom_http_client_cleanup_resource_file(stage, url, size, crc);
}

static int divoom_update_system_file_format_resource(const char *local, const char *remote, uint32 size, uint32 crc, int (*retry)(int errors), divoom_http_client_down_file_callback *progress, void *user_data)
{
    char url[512];
    if (!divoom_update_system_file_format_resource_url(remote, url, sizeof(url))) return 0;
    return divoom_http_client_down_resource_file_checked(local, url, size, crc, retry, progress, user_data);
}

int divoom_update_system_file_prepare_format(int (*retry)(int errors))
{
	/* 在删除 /usr 前独立下载并完整解析清单，失败不能触碰资源卷。 */
	divoom_port_remove_file(DIVOOM_SYSTEM_FORMAT_MANIFEST);
	return divoom_update_system_file_format_download(DIVOOM_SYSTEM_FORMAT_MANIFEST,
		DIVOOM_SYSTEM_FILE_LAST_PATH_FILE, retry) == 1 &&
        divoom_update_system_file_restore_sync(DIVOOM_SYSTEM_FORMAT_MANIFEST);
}

int divoom_update_system_file_ensure_format_manifest(int (*retry)(int errors))
{
	/* A reboot keeps the original guide; redownload only if it is unreadable. */
	return divoom_update_system_file_validate_format_manifest(DIVOOM_SYSTEM_FORMAT_MANIFEST) == 0 ||
		divoom_update_system_file_prepare_format(retry);
}

/* Recovery never promotes the latest restore guide to the historical ledger.
 * The caller owns the maintenance window and all normal resource writers. */
static int divoom_update_system_file_original_state(void)
{
    divoom_port_stat_t st;
    errno = 0;
    if (divoom_port_stat(DIVOOM_SYSTEM_CRC_FILE_PATH, &st) == 0) {
        int rc;
        if (st.is_dir) return -1;
        rc = divoom_port_dir_manifest_prepare_dirs(DIVOOM_SYSTEM_CRC_FILE_PATH, "/usr", 0);
        return rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM ||
            (rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO && (errno == ENOMEM || errno == EMFILE || errno == ENFILE)) ?
            DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : rc == 0 ? 1 : -1;
    }
    return errno == ENOENT ? 0 : (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : -1;
}

static int divoom_update_system_file_original_clear_version(void)
{
    divoom_port_stat_t st;
    errno = 0;
    if (divoom_port_stat(DIVOOM_SYSTEM_CRC_VERSION_PATH, &st) != 0) return errno == ENOENT;
    if (st.is_dir || divoom_port_remove_file(DIVOOM_SYSTEM_CRC_VERSION_PATH) != 0 ||
        divoom_port_sync() != 0) return 0;
    errno = 0;
    return divoom_port_stat(DIVOOM_SYSTEM_CRC_VERSION_PATH, &st) != 0 && errno == ENOENT;
}

static int divoom_update_system_file_original_attempt(void)
{
    const char *candidate = "/userdata/.usr_base_crc.bin";
    const char *stage = DIVOOM_SYSTEM_CRC_FILE_PATH ".divoom_base";
    uint64_t size;
    uint32_t crc;
    int rc, state = divoom_update_system_file_original_state();
    if (state != 0) {
        /* Entry already rejected pre-existing invalid ledgers. A newly invalid
         * ledger here is a local publication failure; resource pressure is transient. */
        if (state < 0) return state == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM ? 0 : -1;
        errno = 0;
        if (divoom_update_system_file_restore_sync(DIVOOM_SYSTEM_CRC_FILE_PATH) && divoom_port_sync() == 0) return 1;
        return (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    }
    /* A complete, CRC-valid candidate survives power loss and can be reused. */
    rc = divoom_update_system_file_validate_format_manifest(candidate);
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) return 0;
    if (rc != 0) {
        int downloaded = divoom_update_system_file_format_download_once((char *)candidate,
            (char *)DIVOOM_SYSTEM_FILE_BASE_PATH_FILE);
        if (downloaded != 1)
            return downloaded == -2 && divoom_wifi_model_is_network_ready() ? -1 : 0;
        if (divoom_update_system_file_validate_format_manifest(candidate) != 0) return 0;
    }
    errno = 0;
    if (!divoom_update_system_file_restore_sync(candidate)) return (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    errno = 0;
    rc = divoom_port_file_crc32(candidate, &size, &crc);
    if (rc != 0) return rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    state = divoom_update_system_file_original_state();
    if (state != 0) {
        /* Entry already rejected pre-existing invalid ledgers. A newly invalid
         * ledger here is a local publication failure; resource pressure is transient. */
        if (state < 0) return state == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM ? 0 : -1;
        errno = 0;
        if (divoom_update_system_file_restore_sync(DIVOOM_SYSTEM_CRC_FILE_PATH) && divoom_port_sync() == 0) return 1;
        return (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    }
    errno = 0;
    if (divoom_port_copy_file(candidate, stage) != 0 ||
        !divoom_update_system_file_restore_sync(stage)) return (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    errno = 0;
    rc = divoom_port_file_verify(stage, size, crc, 0);
    if (rc != 0) return rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    state = divoom_update_system_file_original_state();
    if (state != 0) {
        /* Entry already rejected pre-existing invalid ledgers. A newly invalid
         * ledger here is a local publication failure; resource pressure is transient. */
        if (state < 0) return state == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM ? 0 : -1;
        errno = 0;
        if (divoom_update_system_file_restore_sync(DIVOOM_SYSTEM_CRC_FILE_PATH) && divoom_port_sync() == 0) return 1;
        return (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    }
    /* Invalidate any surviving version BEFORE making a base ledger visible.
     * An interrupted commit must never claim that the base is a newer guide. */
    errno = 0;
    if (!divoom_update_system_file_original_clear_version()) return (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    state = divoom_update_system_file_original_state();
    if (state != 0) {
        /* Entry already rejected pre-existing invalid ledgers. A newly invalid
         * ledger here is a local publication failure; resource pressure is transient. */
        if (state < 0) return state == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM ? 0 : -1;
        errno = 0;
        if (divoom_update_system_file_restore_sync(DIVOOM_SYSTEM_CRC_FILE_PATH) && divoom_port_sync() == 0) return 1;
        return (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    }
    errno = 0;
    if (divoom_port_rename_file(stage, DIVOOM_SYSTEM_CRC_FILE_PATH) != 0 ||
        !divoom_update_system_file_restore_sync(DIVOOM_SYSTEM_CRC_FILE_PATH) ||
        divoom_port_sync() != 0) return (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    errno = 0;
    rc = divoom_port_file_verify(DIVOOM_SYSTEM_CRC_FILE_PATH, size, crc, 0);
    if (rc != 0) return rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : -1;
    return 1;
}

int divoom_update_system_file_ensure_original_ledger(int (*retry)(int errors))
{
    int errors = 0, local_errors = 0, state = divoom_update_system_file_original_state();
    if (state < 0 && state != DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) return 0;
    if (state > 0 && divoom_update_system_file_restore_sync(DIVOOM_SYSTEM_CRC_FILE_PATH) && divoom_port_sync() == 0) return 1;
    if (retry && !retry(0)) return 0;
    for (;;) {
        int restored = divoom_update_system_file_original_attempt();
        if (restored == 1) return !retry || retry(0);
        if (!retry) return 0;
        if (restored < 0 && ++local_errors >= 4) {
            divoom_print("format_restore: original ledger local I/O failed attempts=4; request format\n");
            return DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT;
        }
        if (errors < 0x7fffffff) ++errors;
        divoom_print("format_restore: original ledger retry=%d delay=10s\n", errors);
        if (!retry(errors)) return 0;
        divoom_system_delay_ms(10000);
    }
}

/* Count only missing/damaged manifest files; unrelated live files are extras. */
static int divoom_update_system_file_restore_pending(
	const divoom_port_dir_change_t *change, void *user_data)
{
    divoom_system_file_restore_ctx *ctx = user_data;
    char path[DIVOOM_SYSTEM_FILE_PATH_MAX];
    ctx->failure = 0;
    if (change->type == DIVOOM_PORT_DIR_CHANGE_ADDED) return 0;
    if (ctx->total == 0x7fffffff ||
        divoom_update_system_file_format_reserved(change->relative_path) ||
        snprintf(path, sizeof(path), "/usr/%s", change->relative_path) >= sizeof(path)) return 1;
    ++ctx->total;
    errno = 0;
    int verified = divoom_port_file_verify(path, change->old_size, change->old_crc32, 0);
    if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (verified && (errno == ENOMEM || errno == EMFILE || errno == ENFILE))) {
        ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
    }
    if (!verified) ++ctx->done;
    return 0;
}

static void divoom_update_system_file_restore_file_progress(void *user_data,
	uint32 downloaded, uint32 total)
{
	divoom_system_file_restore_ctx *ctx = user_data;
	int percent;
	(void)total;
	if (!ctx || !ctx->progress) return;
	/* Manifest size also covers responses without a Content-Length header. */
	percent = ctx->current_file_size ?
		(int)((uint64_t)downloaded * 100 / ctx->current_file_size) : 100;
	if (percent > 100) percent = 100;
	ctx->progress(ctx->done, ctx->total, percent);
}

/* A 404 is not permission to keep corrupt data. Confirm unlink on /usr;
 * transient unlink/stat/fsync failures get three retries before reformat. */
static int divoom_update_system_file_remove_bad(const char *path)
{
    int attempt;
    for (attempt = 0; attempt <= 3; ++attempt) {
        divoom_port_stat_t st;
        int ok;
        errno = 0;
        if (divoom_port_stat(path, &st) == 0)
            ok = !st.is_dir && divoom_port_remove_file(path) == 0;
        else ok = errno == ENOENT;
        if (ok && divoom_port_sync() == 0) {
            errno = 0;
            if (divoom_port_stat(path, &st) != 0 && errno == ENOENT) return 1;
        }
        divoom_print("format_restore: delete/confirm failed attempt=%d/4 path=%s\n", attempt+1, path);
        if (attempt < 3) divoom_system_delay_ms(2000);
    }
    return 0;
}

static divoom_restore_fault *divoom_update_system_file_fault(divoom_system_file_restore_ctx *ctx,
    const char *relative, int create)
{
    if (strcmp(ctx->active.path, relative)) return NULL;
    if (!create) return ctx->active.attempts || ctx->active.deferred ? &ctx->active : NULL;
    if (!ctx->states) {
        unsigned count = ctx->full_restore ? (unsigned)ctx->total : 1U;
        if (!count || count > DIVOOM_RESTORE_FAULT_BYTES) goto nomem;
        ctx->states = calloc(count, 1);
        if (!ctx->states) goto nomem;
        ctx->fault_bytes = count;
    }
    if (ctx->state_index >= ctx->fault_bytes) goto nomem;
    return &ctx->active;
nomem:
    errno = ENOMEM; ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return NULL;
}
static void divoom_update_system_file_restore_select(divoom_system_file_restore_ctx *ctx,
    const divoom_port_dir_change_t *change)
{
    memset(&ctx->active, 0, sizeof(ctx->active));
    ctx->state_index = ctx->scan_index++;
    if (strlen(change->relative_path) < sizeof(ctx->active.path)) strcpy(ctx->active.path, change->relative_path);
    ctx->active.size = change->old_size; ctx->active.crc = change->old_crc32;
    unsigned state = ctx->states && ctx->state_index < ctx->fault_bytes ? ctx->states[ctx->state_index] : 0;
    ctx->active.attempts = state & 3U;
    ctx->active.deferred = !!(state & 4U); ctx->active.not_found = !!(state & 8U);
    ctx->active.finalized = !!(state & 16U);
}
static void divoom_update_system_file_restore_store(divoom_system_file_restore_ctx *ctx)
{
    if (ctx->states && ctx->state_index < ctx->fault_bytes)
        ctx->states[ctx->state_index] = ctx->active.attempts | (ctx->active.deferred ? 4U : 0U) |
            (ctx->active.not_found ? 8U : 0U) | (ctx->active.finalized ? 16U : 0U);
}
static void divoom_update_system_file_faults_clear(divoom_system_file_restore_ctx *ctx)
{
    free(ctx->states); ctx->states = NULL;
    divoom_cJSON_Delete(ctx->skipped);
}
static int divoom_update_system_file_was_skipped(divoom_system_file_restore_ctx *ctx,
    const char *relative)
{
    divoom_restore_fault *item = divoom_update_system_file_fault(ctx, relative, 0);
    return item && item->deferred;
}
static int divoom_update_system_file_record_skip(divoom_system_file_restore_ctx *ctx,
    const char *relative)
{
    divoom_restore_fault *item = divoom_update_system_file_fault(ctx, relative, 1);
    if (!item) return 0;
    if (!item->deferred) ++ctx->skipped_count;
    item->deferred = item->not_found = 1;
    __atomic_store_n(&restore_status_failed, ctx->skipped_count, __ATOMIC_RELEASE);
    return 1;
}
/* A private, explicitly selected white-box context writes only the caller's
 * test namespace. Ordinary recovery always uses the checked HTTP downloader. */
static int divoom_update_system_file_restore_fetch(divoom_system_file_restore_ctx *ctx,
    const char *local, const char *remote, uint32 size, uint32 crc, int rescue)
{
    if (!ctx->whitebox)
        return divoom_update_system_file_format_resource(local, remote, size, crc, ctx->retry,
            divoom_update_system_file_restore_file_progress, ctx);
    if (strncmp(local, "/usr/.restore_whitebox/", sizeof("/usr/.restore_whitebox/")-1) &&
        strncmp(local, "/userdata/.format_restore/.restore_whitebox/", sizeof("/userdata/.format_restore/.restore_whitebox/")-1)) return 0;
    int bad = strstr(local, "bad.bin") != NULL;
    if (bad) { if (rescue) ++ctx->whitebox_rescues; else ++ctx->whitebox_downloads; }
    if (bad && ctx->whitebox == 6) return DIVOOM_SYSTEM_FILE_RESTORE_SKIPPED;
    if (bad && rescue && ctx->whitebox == 3 && !ctx->whitebox_space_retry++) { errno = ENOSPC; return -1; }
    unsigned char data[16];
    for (unsigned i = 0; i < sizeof(data); ++i) data[i] = i;
    int corrupt = bad && (!rescue ? !(ctx->whitebox == 11 || ctx->whitebox == 12 || (ctx->whitebox == 9 && ctx->whitebox_downloads == 3)) : ctx->whitebox == 2 || ctx->whitebox == 7);
    if (corrupt) data[0] ^= 1;
    void *file = divoom_port_file_open(local, "wb");
    if (!file) return -1;
    int ok = divoom_port_file_write(data, 1, sizeof(data), file) == sizeof(data) && divoom_port_file_sync(file) == 0;
    if (divoom_port_file_close(file) != 0) ok = 0;
    return !ok ? -1 : corrupt ? DIVOOM_HTTP_RESOURCE_CONTENT_MISMATCH : 1;
}
static int divoom_update_system_file_restore_cleanup(divoom_system_file_restore_ctx *ctx,
    const char *stage, const char *remote, uint32 size, uint32 crc)
{
    return ctx->whitebox ? divoom_update_system_file_remove_bad(stage) :
        divoom_update_system_file_format_resource_cleanup(stage, remote, size, crc);
}
static int divoom_update_system_file_restore_owned(divoom_system_file_restore_ctx *ctx,
    const char *local, const char *remote, uint32 size, uint32 crc)
{
    if (ctx->whitebox) return 1; /* The explicit command owns its fixed test namespace. */
    char url[512];
    if (!divoom_update_system_file_format_resource_url(remote, url, sizeof(url))) return 0;
    return divoom_http_client_resource_file_owned(local, url, size, crc);
}

static int divoom_update_system_file_restore_change_entry(
	const divoom_port_dir_change_t *change, void *user_data)
{
	divoom_system_file_restore_ctx *ctx = user_data;
	char local_path[DIVOOM_SYSTEM_FILE_PATH_MAX];
	char server_path[DIVOOM_SYSTEM_FILE_NET_PATH_MAX];
	char stage_path[DIVOOM_SYSTEM_FILE_PATH_MAX];
	uint64_t size;
	uint32_t crc;
    int local_errors = 0, content_errors = 0, initially_valid = -1;
    const char *operation = "prepare";
    int verified;

    ctx->stage_failed = 0;
	/* checksum_compare 的 REMOVED/旧值代表清单期望的文件。 */
	if (change->type == DIVOOM_PORT_DIR_CHANGE_ADDED) {
		return 0;
	}
    if (divoom_update_system_file_format_reserved(change->relative_path)) return 1;
    if (divoom_update_system_file_was_skipped(ctx, change->relative_path)) return 0;
	if (snprintf(local_path, sizeof(local_path), "/usr/%s", change->relative_path) >= sizeof(local_path) ||
	    snprintf(stage_path, sizeof(stage_path), "/userdata/.format_restore/%s", change->relative_path) >= sizeof(stage_path) ||
	    snprintf(server_path, sizeof(server_path), "%s%s", DIVOOM_SYSTEM_NET_FILE_PATH, change->relative_path) >= sizeof(server_path)) {
		return 1;
	}
retry_local:
    ctx->failure = 0;
    ctx->stage_failed = 0;
    errno = 0;
    verified = divoom_port_file_verify(local_path, change->old_size, change->old_crc32, 0);
    if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (verified && (errno == ENOMEM || errno == EMFILE || errno == ENFILE))) {
        ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
    }
    if (verified == 0) {
        if (initially_valid < 0) initially_valid = 1;
        /* A renamed file is successful only after syncing AND reopening it for CRC. */
        operation = "installed_sync_or_crc";
        if (!divoom_update_system_file_restore_sync(local_path)) goto local_failed;
        verified = divoom_port_file_verify(local_path, change->old_size, change->old_crc32, 0);
        if (verified != 0) goto content_failed;
        operation = "stage_cleanup";
        if (!divoom_update_system_file_restore_cleanup(ctx, stage_path, server_path,
                (uint32)change->old_size, change->old_crc32)) goto local_failed;
        if (!initially_valid) {
            if (ctx->done < ctx->total) ++ctx->done;
            if (ctx->progress) ctx->progress(ctx->done, ctx->total, 100);
        }
        return 0;
    }
    if (initially_valid < 0) initially_valid = 0;
    operation = "create_stage_directory";
	if (!divoom_update_system_file_mk_parent(stage_path)) {
        goto local_failed;
	}
	if (change->old_size > 0x7fffffffU) return 1;
	ctx->current_file_size = (uint32)change->old_size;
	if (ctx->progress) ctx->progress(ctx->done, ctx->total, 0);
    if (ctx->full_restore) {
        divoom_restore_fault *attempt = divoom_update_system_file_fault(ctx, change->relative_path, 1);
        if (!attempt) return 1;
        if (attempt->attempts >= 3) goto content_failed;
        ++attempt->attempts;
    }
	if (change->old_size == 0) {
        operation = "create_empty_file";
		if (!divoom_update_system_file_write_empty(stage_path)) {
            goto local_failed;
		}
    } else {
        int downloaded = divoom_update_system_file_restore_fetch(ctx, stage_path, server_path,
            (uint32)change->old_size, change->old_crc32, 0);
        if (downloaded == DIVOOM_HTTP_RESOURCE_CONTENT_MISMATCH) goto content_failed;
        if (downloaded < 0) { ctx->delete_failed = 1; ctx->stage_failed = 1; return 1; }
        if (downloaded == DIVOOM_SYSTEM_FILE_RESTORE_SKIPPED) {
            if (!ctx->full_restore && !divoom_update_system_file_remove_bad(local_path)) {
                ctx->stage_failed = 0; /* This deletion belongs to /usr. */
                ctx->delete_failed = 1; return 1;
            }
            if (!divoom_update_system_file_record_skip(ctx, change->relative_path)) {
                ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
            }
            divoom_restore_fault *missing = divoom_update_system_file_fault(ctx, change->relative_path, 0);
            missing->size = change->old_size; missing->crc = change->old_crc32;
            divoom_print("format_restore: skipped HTTP 404 %s\n", change->relative_path);
            operation = "404_stage_cleanup";
            if (!ctx->full_restore && !divoom_update_system_file_restore_cleanup(ctx, stage_path, server_path,
                (uint32)change->old_size, change->old_crc32)) goto local_failed;
            if (ctx->done < ctx->total) ++ctx->done;
            if (ctx->progress) ctx->progress(ctx->done, ctx->total, -1);
            return 0;
        }
        if (!downloaded) {
            ctx->failure = (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : 0;
            divoom_print("format_restore: download failed %s\n", change->relative_path);
            return 1;
        }
    }
    operation = "stage_crc";
    errno = 0;
    verified = divoom_port_file_crc32(stage_path, &size, &crc);
    if (verified != 0) goto local_failed;
    if (size != change->old_size || crc != change->old_crc32) {
        /* Read succeeded: the source payload does not match its guide. Retry
         * this file only; never authorize an erase from a server CRC mismatch. */
        divoom_print("format_restore: source CRC mismatch file=%s size=%llu/%llu crc=%08x/%08x\n",
            change->relative_path, (unsigned long long)size, (unsigned long long)change->old_size,
            crc, change->old_crc32);
        goto content_failed;
    }
    operation = "copy_or_rename";
    errno = 0;
    if (!divoom_update_system_file_commit(stage_path, local_path)) goto local_failed;
    operation = "installed_sync";
    if (!divoom_update_system_file_restore_sync(local_path)) goto local_failed;
    /* Verify the FINAL pathname after rename. HTTP/stage CRC alone is insufficient. */
    operation = "installed_crc";
    if ((ctx->whitebox == 11 || ctx->whitebox == 12) && strstr(local_path, "/bad.bin")) {
        void *test_file = divoom_port_file_open(local_path, "r+b");
        unsigned char corrupt = 255;
        if (!test_file) goto local_failed;
        int ok = divoom_port_file_write(&corrupt, 1, 1, test_file) == 1 && divoom_port_file_sync(test_file) == 0;
        if (divoom_port_file_close(test_file) != 0 || !ok) goto local_failed;
    }
    verified = divoom_port_file_verify(local_path, change->old_size, change->old_crc32, 0);
    if (verified != 0) goto content_failed;
    operation = "stage_cleanup";
    if (!divoom_update_system_file_restore_cleanup(ctx, stage_path, server_path,
        (uint32)change->old_size, change->old_crc32)) goto local_failed;
	if (ctx->done < ctx->total) ctx->done++;
	if (ctx->progress) {
		ctx->progress(ctx->done, ctx->total, 100);
	}
	divoom_print("format_restore: installed %d/%d %s\n", ctx->done, ctx->total, change->relative_path);
	return 0;
content_failed:
    if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || errno == ENOMEM || errno == EMFILE || errno == ENFILE) {
        ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
    }
    if (!ctx->full_restore) { ctx->delete_failed = 1; ctx->stage_failed = 0; return 1; }
    divoom_restore_fault *bad = divoom_update_system_file_fault(ctx, change->relative_path, 1);
    if (!bad) return 1;
    bad->size = change->old_size; bad->crc = change->old_crc32;
    divoom_print("format_restore: content failure attempt=%u/3 file=%s\n", bad->attempts, local_path);
    if (bad->attempts >= 3) {
        if (!bad->deferred) ++ctx->skipped_count;
        bad->deferred = 1;
        __atomic_store_n(&restore_status_failed, ctx->skipped_count, __ATOMIC_RELEASE);
        ctx->failure = 0; if (ctx->done < ctx->total) ++ctx->done;
        if (ctx->progress) ctx->progress(ctx->done, ctx->total, -1);
        return 0;
    }
    operation = "stage_cleanup";
    if (!divoom_update_system_file_restore_cleanup(ctx, stage_path, server_path,
            (uint32)change->old_size, change->old_crc32)) goto local_failed;
    if (!divoom_update_system_file_remove_bad(local_path)) {
        ctx->stage_failed = 0; ctx->delete_failed = 1; return 1;
    }
    if (!ctx->retry || !ctx->retry(++content_errors)) return 1;
    divoom_system_delay_ms(10000);
    goto retry_local;
local_failed:
    if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (errno == ENOMEM || errno == EMFILE || errno == ENFILE)) {
        ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
    }
    ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_IO;
    ctx->stage_failed = !strcmp(operation, "create_stage_directory") ||
        !strcmp(operation, "create_empty_file") || !strcmp(operation, "stage_crc") ||
        !strcmp(operation, "stage_cleanup") || !strcmp(operation, "404_stage_cleanup");
    if (ctx->full_restore && !ctx->stage_failed) goto content_failed;
    ++local_errors;
    divoom_print("format_restore: local failure op=%s attempt=%d/4 errno=%d file=%s\n",
        operation, local_errors, errno, local_path);
    if (!ctx->retry) return 1;
    if (local_errors >= 4) { ctx->delete_failed = 1; return 1; }
    if (!ctx->retry(local_errors)) return 1;
    divoom_system_delay_ms(10000);
    goto retry_local;
}

static int divoom_update_system_file_restore_change(
    const divoom_port_dir_change_t *change, void *user_data)
{
    divoom_system_file_restore_ctx *ctx = user_data;
    if (change->type == DIVOOM_PORT_DIR_CHANGE_ADDED) return 0;
    divoom_update_system_file_restore_select(ctx, change);
    int rc = divoom_update_system_file_restore_change_entry(change, ctx);
    divoom_update_system_file_restore_store(ctx);
    return rc;
}
int divoom_update_system_file_restore_one(const char *relative, uint64_t size, uint32_t crc,
    int done, int total, divoom_system_file_restore_progress progress, int (*retry)(int))
{
    divoom_system_file_restore_ctx ctx = {0};
    divoom_port_dir_change_t change = {0};
    ctx.done = done; ctx.total = total; ctx.progress = progress; ctx.retry = retry;
    change.type = DIVOOM_PORT_DIR_CHANGE_REMOVED;
    change.relative_path = relative;
    change.old_size = size; change.old_crc32 = crc;
    int ok = divoom_update_system_file_restore_change(&change, &ctx) == 0;
    int result = ok ? (ctx.skipped_count ? DIVOOM_SYSTEM_FILE_RESTORE_SKIPPED : 1) :
        ctx.delete_failed ? (ctx.stage_failed ? DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT :
            DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT) : 0;
    divoom_update_system_file_faults_clear(&ctx);
    return result;
}

static int divoom_update_system_file_restore_finalize_entry(const divoom_port_dir_change_t *change, void *user_data)
{
    divoom_system_file_restore_ctx *ctx = user_data;
    if (change->type == DIVOOM_PORT_DIR_CHANGE_ADDED) return 0;
    divoom_update_system_file_restore_select(ctx, change);
    divoom_restore_fault *item = &ctx->active;
    char local[DIVOOM_SYSTEM_FILE_PATH_MAX], alternate[DIVOOM_SYSTEM_FILE_PATH_MAX];
    char stage[DIVOOM_SYSTEM_FILE_PATH_MAX], remote[DIVOOM_SYSTEM_FILE_NET_PATH_MAX];
        if (!item->deferred || item->finalized) return 0;
        if (snprintf(local, sizeof(local), "/usr/%s", item->path) >= sizeof(local) ||
            snprintf(stage, sizeof(stage), "/userdata/.format_restore/%s", item->path) >= sizeof(stage) ||
            snprintf(remote, sizeof(remote), "%s%s", DIVOOM_SYSTEM_NET_FILE_PATH, item->path) >= sizeof(remote)) return 1;
        __atomic_store_n(&restore_status_stage, item->not_found ? 2 : 1, __ATOMIC_RELEASE);
        if (ctx->progress) ctx->progress(ctx->done, ctx->total, -1);
        /* The completed bad staging payload is no longer needed, but the
         * final pathname is retained until a separately allocated copy passes. */
        if (!divoom_update_system_file_restore_cleanup(ctx, stage, remote, (uint32)item->size, item->crc)) {
            if (errno == ENOMEM || errno == EMFILE || errno == ENFILE) {
                ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
            }
            ctx->delete_failed = ctx->stage_failed = 1; return 1;
        }
        int restored = 0;
        alternate[0] = 0;
        if (!item->not_found) {
            divoom_port_stat_t st;
            int found = 0;
            for (unsigned name = 0; name < 32; ++name) {
                if (snprintf(alternate, sizeof(alternate), "%s.divoom_rescue%u", local, name) >= sizeof(alternate)) return 1;
                errno = 0;
                if (divoom_port_stat(alternate, &st) != 0 && errno == ENOENT) { found = 1; break; }
                if (errno && errno != ENOENT) return 1;
                int owned = divoom_update_system_file_restore_owned(ctx, alternate, remote, (uint32)item->size, item->crc);
                if (owned < 0) {
                    ctx->failure = errno == ENOMEM || errno == EMFILE || errno == ENFILE ? DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
                    return 1;
                }
                if (owned && divoom_update_system_file_restore_cleanup(ctx, alternate, remote, (uint32)item->size, item->crc)) {
                    found = 1; break;
                }
                if (owned) goto delete_failed;
            }
            /* Never overwrite an unrelated file occupying the candidate. */
            if (!found) { errno = ENOMEM; ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1; }
            uint64_t free_bytes = (uint64_t)divoom_port_fs_free_space("/usr") * 1024U;
            int removed_for_space = free_bytes < item->size;
            if (removed_for_space && !divoom_update_system_file_remove_bad(local)) goto delete_failed;
            divoom_print("format_restore: rescue begin file=%s alternate=%s free=%llu need=%llu\n", local, alternate,
                (unsigned long long)free_bytes, (unsigned long long)item->size);
            ctx->current_file_size = (uint32)item->size;
            int downloaded;
rescue_download:
            if (!divoom_update_system_file_mk_parent(alternate)) goto delete_failed;
            errno = 0;
            if (item->size == 0) downloaded = divoom_update_system_file_write_empty(alternate) ? 1 : -1;
            else downloaded = divoom_update_system_file_restore_fetch(ctx, alternate, remote, (uint32)item->size, item->crc, 1);
            if (downloaded < 0 && downloaded != DIVOOM_HTTP_RESOURCE_CONTENT_MISMATCH && errno == ENOSPC && !removed_for_space) {
                if (!divoom_update_system_file_restore_cleanup(ctx, alternate, remote, (uint32)item->size, item->crc) ||
                    !divoom_update_system_file_remove_bad(local)) goto delete_failed;
                removed_for_space = 1;
                divoom_print("format_restore: rescue ENOSPC, removed original before one restart file=%s\n", local);
                goto rescue_download;
            }
            if (!downloaded || errno == ENOMEM || errno == EMFILE || errno == ENFILE) {
                ctx->failure = errno == ENOMEM || errno == EMFILE || errno == ENFILE ? DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : 0;
                /* Cancellation/transport stop must not discard the old file. */
                if (!divoom_update_system_file_restore_cleanup(ctx, alternate, remote, (uint32)item->size, item->crc)) goto delete_failed;
                return 1;
            }
            errno = 0;
            int verified = downloaded == 1 && divoom_update_system_file_restore_sync(alternate) ?
                divoom_port_file_verify(alternate, item->size, item->crc, 0) : DIVOOM_PORT_DIR_COMPARE_DIFFERENT;
            if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || errno == ENOMEM || errno == EMFILE || errno == ENFILE) {
                ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
                if (!divoom_update_system_file_restore_cleanup(ctx, alternate, remote, (uint32)item->size, item->crc)) goto delete_failed;
                return 1;
            }
            if (!verified) {
                if (!divoom_update_system_file_remove_bad(local)) goto delete_failed;
                errno = 0;
                if (ctx->whitebox == 4) { errno = EIO; goto delete_failed; }
                if (divoom_port_rename_file(alternate, local) != 0) goto delete_failed;
                if (ctx->whitebox == 5) {
                    void *file = divoom_port_file_open(local, "r+b");
                    unsigned char bad = 255;
                    if (!file) goto delete_failed;
                    int ok = divoom_port_file_write(&bad, 1, 1, file) == 1 && divoom_port_file_sync(file) == 0;
                    if (divoom_port_file_close(file) != 0 || !ok) goto delete_failed;
                }
                verified = divoom_update_system_file_restore_sync(local) ?
                    divoom_port_file_verify(local, item->size, item->crc, 0) : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
                if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || errno == ENOMEM || errno == EMFILE || errno == ENFILE) {
                    ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
                }
                restored = verified == 0;
            }
        }
        __atomic_store_n(&restore_status_stage, 2, __ATOMIC_RELEASE);
        if (ctx->progress) ctx->progress(ctx->done, ctx->total, -1);
        if (!restored) {
            if (ctx->whitebox == 7) { errno = EIO; goto delete_failed; }
            if (!divoom_update_system_file_remove_bad(local)) goto delete_failed;
            ++ctx->final_skipped;
            __atomic_store_n(&restore_status_skipped, ctx->final_skipped, __ATOMIC_RELEASE);
        }
        if (alternate[0] && !divoom_update_system_file_restore_cleanup(ctx, alternate, remote, (uint32)item->size, item->crc)) goto delete_failed;
        item->finalized = 1;
        if (!restored && !strcasecmp(item->path, "aic/fmacfw_8800d80_h_u02.bin"))
            __atomic_store_n(&restore_driver_skipped, 1, __ATOMIC_RELEASE);
        divoom_update_system_file_restore_store(ctx);
        divoom_print("format_restore: rescue %s file=%s attempts=%u extra=%d\n", restored ? "verified" : "skipped", local, item->attempts, !item->not_found);
    return 0;
delete_failed:
    if (errno == ENOMEM || errno == EMFILE || errno == ENFILE) {
        ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
    }
    ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_IO;
    ctx->stage_failed = 0; ctx->delete_failed = 1;
    return 1;
}

static int divoom_update_system_file_restore_finalize(divoom_system_file_restore_ctx *ctx)
{
    if (!ctx->skipped_count) return 1;
    ctx->scan_index = 0;
    if (!ctx->whitebox)
        return divoom_port_dir_manifest_visit(DIVOOM_SYSTEM_FORMAT_MANIFEST, "/usr", divoom_update_system_file_restore_finalize_entry, ctx) == 0;
    divoom_port_dir_change_t change = {0};
    change.type = DIVOOM_PORT_DIR_CHANGE_REMOVED;
    change.old_size = 16; change.old_crc32 = 0xcecee288U;
    change.relative_path = ".restore_whitebox/bad.bin";
    if (divoom_update_system_file_restore_finalize_entry(&change, ctx)) return 0;
    change.relative_path = ".restore_whitebox/good.bin";
    return divoom_update_system_file_restore_finalize_entry(&change, ctx) == 0;
}

static int divoom_update_system_file_restore_verify_entry(const divoom_port_dir_change_t *change, void *user_data)
{
    char path[DIVOOM_SYSTEM_FILE_PATH_MAX];
    divoom_system_file_restore_ctx *ctx = user_data;
    ctx->failure = 0;
    ctx->stage_failed = 0;
    if (divoom_update_system_file_was_skipped(user_data, change->relative_path)) return 0;
    if (divoom_update_system_file_format_reserved(change->relative_path) ||
        snprintf(path, sizeof(path), "/usr/%s", change->relative_path) >= sizeof(path)) return 1;
    errno = 0;
    int verified = divoom_update_system_file_restore_sync(path) ?
        divoom_port_file_verify(path, change->old_size, change->old_crc32, 0) : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
    if (!verified) return 0;
    if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || errno == ENOMEM || errno == EMFILE || errno == ENFILE) {
        ctx->failure = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; return 1;
    }
    int done = ctx->done;
    int rc = divoom_update_system_file_restore_change_entry(change, ctx);
    ctx->done = done;
    return rc;
}
static int divoom_update_system_file_restore_verify(const divoom_port_dir_change_t *change, void *user_data)
{
    divoom_system_file_restore_ctx *ctx = user_data;
    if (change->type == DIVOOM_PORT_DIR_CHANGE_ADDED) return 0;
    divoom_update_system_file_restore_select(ctx, change);
    int rc = divoom_update_system_file_restore_verify_entry(change, ctx);
    divoom_update_system_file_restore_store(ctx);
    return rc;
}

int divoom_update_system_file_restore_format(divoom_system_file_restore_progress progress, int (*retry)(int errors))
{
    divoom_system_file_restore_ctx ctx = {0};
    int rc, failure;
    ctx.full_restore = 1;
    __atomic_store_n(&restore_driver_skipped, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_status_stage, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_status_failed, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_status_skipped, 0, __ATOMIC_RELEASE);
    errno = 0;
    rc = divoom_update_system_file_validate_format_manifest(DIVOOM_SYSTEM_FORMAT_MANIFEST);
    if (rc != 0) return rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO && !(errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? DIVOOM_SYSTEM_FILE_RESTORE_USERDATA_RETRY : 0;
    errno = 0;
    rc = divoom_port_dir_manifest_prepare_dirs(DIVOOM_SYSTEM_FORMAT_MANIFEST, "/usr", 1);
    if (rc != 0) return rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO && !(errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? DIVOOM_SYSTEM_FILE_RESTORE_LOCAL_RETRY : 0;
    errno = 0;
    rc = divoom_port_dir_manifest_visit(DIVOOM_SYSTEM_FORMAT_MANIFEST, "/usr",
        divoom_update_system_file_restore_pending, &ctx);
    if (rc != 0) {
        failure = ctx.failure ? ctx.failure : rc;
        return failure == DIVOOM_PORT_DIR_COMPARE_ERR_IO && !(errno == ENOMEM || errno == EMFILE || errno == ENFILE) ?
            (ctx.failure ? DIVOOM_SYSTEM_FILE_RESTORE_LOCAL_RETRY : DIVOOM_SYSTEM_FILE_RESTORE_USERDATA_RETRY) : 0;
    }
    divoom_print("format_restore: resume verified=%d/%d\n", ctx.done, ctx.total);
    ctx.progress = progress;
    ctx.retry = retry;
    if (progress) progress(ctx.done, ctx.total, -1);
    errno = 0;
    rc = divoom_port_dir_manifest_visit(DIVOOM_SYSTEM_FORMAT_MANIFEST, "/usr",
        divoom_update_system_file_restore_change, &ctx);
    if (!rc) {
        errno = 0;
        ctx.scan_index = 0;
        rc = divoom_port_dir_manifest_visit(DIVOOM_SYSTEM_FORMAT_MANIFEST, "/usr",
            divoom_update_system_file_restore_verify, &ctx);
    }
    if (!rc && !divoom_update_system_file_restore_finalize(&ctx)) rc = DIVOOM_PORT_DIR_COMPARE_ERR_CALLBACK;
    if (rc != 0) {
        int result;
        failure = ctx.failure ? ctx.failure : rc;
        result = ctx.delete_failed ? (ctx.stage_failed ? DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT :
            DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT) :
            (failure == DIVOOM_PORT_DIR_COMPARE_ERR_IO || failure == DIVOOM_PORT_DIR_COMPARE_DIFFERENT ||
             failure == DIVOOM_PORT_DIR_COMPARE_ERR_CHANGED) &&
            !(errno == ENOMEM || errno == EMFILE || errno == ENFILE) ?
                (ctx.stage_failed || (!ctx.failure && rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO) ?
                    DIVOOM_SYSTEM_FILE_RESTORE_USERDATA_RETRY : DIVOOM_SYSTEM_FILE_RESTORE_LOCAL_RETRY) : 0;
        divoom_update_system_file_faults_clear(&ctx);
        return result;
    }
    /* Never install the server guide as the local ledger or version. Extra
     * .divoom_new/.divoom_old files from an interrupted install are not inputs. */
    errno = 0;
    if (divoom_port_sync() != 0) {
        int result = (errno == ENOMEM || errno == EMFILE || errno == ENFILE) ? 0 : DIVOOM_SYSTEM_FILE_RESTORE_LOCAL_RETRY;
        divoom_update_system_file_faults_clear(&ctx);
        return result;
    }
    if (progress) progress(ctx.total, ctx.total, ctx.total ? 100 : -1);
    divoom_print("format_restore: complete files=%d deferred=%d skipped=%d\n", ctx.total, ctx.skipped_count, ctx.final_skipped);
    __atomic_store_n(&restore_status_stage, 3, __ATOMIC_RELEASE);
    divoom_update_system_file_faults_clear(&ctx);
    return 1;
}

static int divoom_update_system_file_whitebox_retry(int errors)
{
    (void)errors;
    return 1;
}

/* Called only by the explicitly selected factory command while it holds the
 * existing maintenance owner. It never invokes a format or recovery marker. */
int divoom_update_system_file_restore_whitebox(int scenario)
{
    if (scenario < 0 || scenario > 11) return -1;
    divoom_system_file_restore_ctx ctx = {0};
    ctx.full_restore = 1; ctx.whitebox = scenario + 1; ctx.total = 2;
    ctx.retry = divoom_update_system_file_whitebox_retry;
    __atomic_store_n(&restore_driver_skipped, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_status_stage, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_status_failed, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_status_skipped, 0, __ATOMIC_RELEASE);
    divoom_port_dir_change_t bad = {0}, good = {0};
    bad.type = good.type = DIVOOM_PORT_DIR_CHANGE_REMOVED;
    bad.relative_path = ".restore_whitebox/bad.bin";
    good.relative_path = ".restore_whitebox/good.bin";
    bad.old_size = good.old_size = 16;
    bad.old_crc32 = good.old_crc32 = 0xcecee288U;
    char path[DIVOOM_SYSTEM_FILE_PATH_MAX];
    int ok = 1;
    const char *names[] = {"/usr/.restore_whitebox/bad.bin", "/usr/.restore_whitebox/good.bin",
        "/userdata/.format_restore/.restore_whitebox/bad.bin", "/userdata/.format_restore/.restore_whitebox/good.bin"};
    if (scenario == 9) {
        ctx.total = DIVOOM_RESTORE_FAULT_BYTES;
        divoom_update_system_file_restore_select(&ctx, &bad);
        ok = divoom_update_system_file_fault(&ctx, bad.relative_path, 1) != NULL && ctx.fault_bytes == DIVOOM_RESTORE_FAULT_BYTES;
        divoom_update_system_file_faults_clear(&ctx); ctx.fault_bytes = 0;
        ctx.total = DIVOOM_RESTORE_FAULT_BYTES + 1;
        ok = ok && !divoom_update_system_file_fault(&ctx, bad.relative_path, 1) && ctx.failure == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
        divoom_print("[RESTORE_WHITEBOX] scenario=9 state_limit=%u bytes=%u %s\n", DIVOOM_RESTORE_FAULT_BYTES,
            DIVOOM_RESTORE_FAULT_BYTES, ok ? "PASS" : "FAIL");
        divoom_update_system_file_faults_clear(&ctx);
        __atomic_store_n(&restore_status_stage, 0, __ATOMIC_RELEASE);
        __atomic_store_n(&restore_status_failed, 0, __ATOMIC_RELEASE);
        __atomic_store_n(&restore_status_skipped, 0, __ATOMIC_RELEASE);
        return ok;
    }
    for (unsigned i = 0; i < sizeof(names)/sizeof(names[0]); ++i) {
        if (!divoom_update_system_file_mk_parent(names[i]) || !divoom_update_system_file_remove_bad(names[i])) ok = 0;
    }
    if (!ok) goto finish;
    divoom_print("[RESTORE_WHITEBOX] scenario=%d checkpoint=download_begin\n", scenario);
    if (divoom_update_system_file_restore_change(&bad, &ctx) != 0) { ok = 0; goto finish; }
    if ((scenario == 10 || scenario == 11) && (divoom_port_file_verify(names[0], 16, bad.old_crc32, 0) == 0 ||
            !divoom_port_file_exists(names[0]))) { ok = 0; goto finish; }
    divoom_print("[RESTORE_WHITEBOX] scenario=%d checkpoint=deferred downloads=%d failed=%d\n",
        scenario, ctx.whitebox_downloads, ctx.skipped_count);
    if (divoom_update_system_file_restore_change(&good, &ctx) != 0 ||
        divoom_port_file_verify(names[1], 16, good.old_crc32, 0) != 0) { ok = 0; goto finish; }
    if (scenario == 8) {
        ok = ctx.whitebox_downloads == 3 && !ctx.skipped_count && !ctx.whitebox_rescues;
    } else {
        if (ctx.whitebox_downloads != (scenario == 5 ? 1 : 3) || ctx.skipped_count != 1) ok = 0;
        if (scenario == 7) {
            int downloads = ctx.whitebox_downloads;
            ctx.scan_index = 0;
            if (divoom_update_system_file_restore_change(&bad, &ctx) || ctx.whitebox_downloads != downloads) ok = 0;
        }
    }
    if (!ok) goto finish;
    if (scenario == 11) {
        divoom_update_system_file_faults_clear(&ctx);
        memset(&ctx, 0, sizeof(ctx));
        ctx.full_restore = 1; ctx.whitebox = 12; ctx.total = 2; ctx.done = 1;
        ctx.retry = divoom_update_system_file_whitebox_retry;
        divoom_print("[RESTORE_WHITEBOX] scenario=11 checkpoint=cold_context_resume files_retained=1\n");
        if (divoom_update_system_file_restore_change(&bad, &ctx) ||
            divoom_update_system_file_restore_change(&good, &ctx) || ctx.whitebox_downloads != 3) { ok = 0; goto finish; }
    }
    divoom_print("[RESTORE_WHITEBOX] scenario=%d checkpoint=rescue_begin\n", scenario);
    int finalized = divoom_update_system_file_restore_finalize(&ctx);
    if (scenario == 3 || scenario == 6) {
        ok = !finalized && ctx.delete_failed;
    } else {
        ok = finalized;
        if (scenario == 1 || scenario == 4 || scenario == 5) {
            divoom_port_stat_t st; errno = 0;
            if (divoom_port_stat(names[0], &st) == 0 || errno != ENOENT || ctx.final_skipped != 1) ok = 0;
        } else if (divoom_port_file_verify(names[0], 16, bad.old_crc32, 0) != 0 || ctx.final_skipped) ok = 0;
        if (scenario == 2 && ctx.whitebox_rescues != 2) ok = 0;
        if (scenario == 7) {
            int rescues = ctx.whitebox_rescues;
            if (!divoom_update_system_file_restore_finalize(&ctx) || ctx.whitebox_rescues != rescues) ok = 0;
        }
    }
    if (divoom_port_file_verify(names[1], 16, good.old_crc32, 0) != 0) ok = 0;
finish:
    divoom_print("[RESTORE_WHITEBOX] scenario=%d checkpoint=cleanup downloads=%d rescue=%d skipped=%d delete_failed=%d %s\n",
        scenario, ctx.whitebox_downloads, ctx.whitebox_rescues, ctx.final_skipped, ctx.delete_failed, ok ? "PASS" : "FAIL");
    /* Only the fixed white-box namespace is owned by this command. */
    for (unsigned i = 0; i < sizeof(names)/sizeof(names[0]); ++i)
        if (!divoom_update_system_file_remove_bad(names[i])) ok = 0;
    for (unsigned i = 0; i < 32; ++i) {
        snprintf(path, sizeof(path), "/usr/.restore_whitebox/bad.bin.divoom_rescue%u", i);
        if (!divoom_update_system_file_remove_bad(path)) ok = 0;
    }
    divoom_update_system_file_faults_clear(&ctx);
    __atomic_store_n(&restore_status_stage, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_status_failed, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_status_skipped, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_driver_skipped, 0, __ATOMIC_RELEASE);
    return ok;
}
