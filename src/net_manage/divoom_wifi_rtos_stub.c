#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "wifimg.h"
#include <string.h>

wmg_status_t wifimanager_init(void)
{
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_on(wifi_mode_t mode)
{
	(void)mode;
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_sta_connect(wifi_sta_cn_para_t *cn_para)
{
	(void)cn_para;
	return WMG_STATUS_NOT_READY;
}

wmg_status_t wifi_sta_disconnect(void)
{
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_sta_auto_reconnect(wmg_bool_t enable)
{
	(void)enable;
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_sta_get_info(wifi_sta_info_t *sta_info)
{
	if (sta_info)
		memset(sta_info, 0, sizeof(*sta_info));
	return WMG_STATUS_NOT_READY;
}

wmg_status_t wifi_sta_remove_networks(char *ssid)
{
	(void)ssid;
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_register_msg_cb(wifi_msg_cb_t msg_cb, void *msg_cb_arg)
{
	(void)msg_cb;
	(void)msg_cb_arg;
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_get_scan_results(wifi_scan_result_t *result, uint32_t *bss_num, uint32_t arr_size)
{
	(void)result;
	(void)arr_size;
	if (bss_num)
		*bss_num = 0;
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_get_wmg_state(wifi_wmg_state_t *wmg_state)
{
	if (wmg_state)
		memset(wmg_state, 0, sizeof(*wmg_state));
	return WMG_STATUS_SUCCESS;
}
