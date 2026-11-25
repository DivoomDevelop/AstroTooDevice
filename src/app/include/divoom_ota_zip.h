#ifndef _divoom_ota_zip_h_
#define _divoom_ota_zip_h_

/* ZIP 本地文件头魔数 PK\x03\x04 */
#define DIVOOM_OTA_ZIP_MAGIC0  0x50
#define DIVOOM_OTA_ZIP_MAGIC1  0x4b
#define DIVOOM_OTA_ZIP_MAGIC2  0x03
#define DIVOOM_OTA_ZIP_MAGIC3  0x04

typedef void (*divoom_ota_zip_progress_fn)(int percent, void *user_data);

/* 读文件头判断是否为 ZIP */
int divoom_ota_is_zip_file(const char *path);

/*
 * 从 zip_path 解压根目录成员 DIVOOM_OTA_RTOS_PACKAGE_NAME 到 out_fex_path。
 * 返回 0 成功，负值失败。
 */
int divoom_ota_zip_extract_fex(const char *zip_path, const char *out_fex_path,
			       divoom_ota_zip_progress_fn progress, void *progress_ud);

#endif
