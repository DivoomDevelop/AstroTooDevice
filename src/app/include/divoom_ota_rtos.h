#ifndef _divoom_ota_rtos_h_
#define _divoom_ota_rtos_h_

/* 路径见 divoom.h：DIVOOM_OTA_RTOS_STAGING_DIR / STAGING_DOWNLOAD */
/* UPDATE 侧 g_ota_dl_filename 仅 32 字节；CDN 名常超长，落地一律用短名 */
#define DIVOOM_OTA_RTOS_PACKAGE_NAME "rtos_pkg_nor.fex"

int divoom_ota_rtos_normalize_sha1(const char *digest, char normalized[41]);

typedef void (*divoom_ota_prepare_progress_fn)(int percent, void *user_data);

/* 从 URL 取最后一段（仅诊断用）；落盘/刷写请用 DIVOOM_OTA_RTOS_PACKAGE_NAME */
const char *divoom_ota_rtos_basename_from_url(const char *url, char *buf, int buf_len);

/* 固定落地路径：/userdata/ota/rtos_pkg_nor.fex（忽略 URL 超长 basename） */
int divoom_ota_rtos_build_save_path(const char *url, char *save_path, int save_path_len);

int divoom_ota_rtos_build_staging_download_path(char *save_path, int save_path_len);

/*
 * staging（ZIP 或裸 fex）转为可刷写 fex：ZIP 解压根目录 rtos_pkg_nor.fex，裸 fex 则 rename/copy。
 * 返回 0 成功。
 */
int divoom_ota_rtos_prepare_package(const char *staging_path, const char *out_fex_path,
				    divoom_ota_prepare_progress_fn progress, void *progress_ud);

void divoom_ota_rtos_remove_staging_if_present(void);

/*
 * 下载并 prepare 为 fex，不自动 start_file（产测/局域网暂存用）。
 * 返回 1 成功，0 失败。
 */
int divoom_ota_rtos_download_and_prepare(const char *url, const char *sha1_expect, int show_percent,
				       void (*dl_progress)(void *user_data, uint32 read_len,
							   uint32 tol_len),
				       void *dl_progress_ud);

/*
 * 刷写本地已有 RTOS 包（完整路径，如 /userdata/ota/foo.fex）。
 * 在后台任务中执行 UPDATE PART + 轮询进度；成功则重启。
 * 返回 0 表示任务已启动，负值表示失败。
 */
int divoom_ota_rtos_start_file(const char *full_path);

/*
 * 本地 staging（ZIP 或裸 fex）prepare 后刷写。
 * staging_path 如 /userdata/ota/tf_update.zip；fex 用 build_save_path。
 * 返回 0 表示任务已启动，负值表示失败。
 */
int divoom_ota_rtos_start_from_staging(const char *staging_path, const char *fex_path);

/* UPDATE_CMD_START 成功后返回 1，用于区分启动前失败与真实 flash 失败。 */
int divoom_ota_rtos_flash_has_started(void);
int divoom_ota_rtos_is_running(void);

/* Download and verify only (1=success). Progress callbacks finish before return.
 * The caller decides whether its still-live session may hand off to flashing. */
int divoom_ota_rtos_download_staging(const char *url, const char *sha1_expect,
    int show_percent,
    void (*dl_progress)(void *user_data, uint32 read_len, uint32 tol_len),
    void *dl_progress_ud);

/*
 * 下载 URL 到 staging，SHA1 校验，prepare 后刷写。
 */
int divoom_ota_rtos_download_prepare_and_start(const char *url, const char *sha1_expect,
					       int show_percent,
					       void (*dl_progress)(void *user_data,
								   uint32 read_len,
								   uint32 tol_len),
					       void *dl_progress_ud);

/*
 * 下载 URL 到 save_full_path，可选 SHA1 校验后刷写。
 * save_full_path 为 NULL 时自动生成到 DIVOOM_OTA_RTOS_STAGING_DIR。
 */
int divoom_ota_rtos_download_and_start(const char *url, const char *save_full_path,
				       const char *sha1_expect);

#endif
