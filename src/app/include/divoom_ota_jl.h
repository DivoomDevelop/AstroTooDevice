#ifndef _DIVOOM_OTA_JL_H_
#define _DIVOOM_OTA_JL_H_

#ifdef DIVOOM_TIMES_VIBE

#include "divoom_general_json.h"

void divoom_ota_jl_init(void);

void divoom_ota_jl_on_version_notify(uint32_t jl_version);

uint32_t divoom_ota_jl_get_local_version(void);

/* 杰理上报版本是否可用（非 0、非主控 Hardware 前缀、三位硬件号合理） */
int divoom_ota_jl_version_is_valid(uint32_t jl_version);

/* 云端包是否主控 F101：Software/1000 == DIVOOM_HARDWARE_VERSION */
int divoom_ota_is_f101_cloud_package(uint32_t version);

/*
 * 云端包是否杰理：
 * 1) 排除 F101（DIVOOM_HARDWARE_VERSION）
 * 2) 用开机 BOOT_DONE 后杰理主动上报并缓存的版本前缀核对
 * 3) 尚未缓存时，用 Push 的 Hardware 与 Software 前缀互证（均非主控）
 */
int divoom_ota_jl_is_cloud_package(uint32_t version, int cloud_hardware);

int divoom_ota_jl_start_srv_update(Pdivoom_general_json_get_update_file_info_respone pinfo);

typedef void divoom_ota_jl_progress_callback(void *user_data, uint8 percent);
typedef void divoom_ota_jl_result_callback(void *user_data, int success);

/* 产测局域网升级：FileId 为完整 URL，不切换到普通系统 OTA 页面。 */
int divoom_ota_jl_start_local_update(
	Pdivoom_general_json_get_update_file_info_respone pinfo,
	divoom_ota_jl_progress_callback *progress_callback,
	divoom_ota_jl_result_callback *result_callback, void *user_data);

uint8 divoom_ota_jl_is_upgrading(void);

#endif /* DIVOOM_TIMES_VIBE */

#endif /* _DIVOOM_OTA_JL_H_ */
