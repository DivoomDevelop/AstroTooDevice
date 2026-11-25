#include "aic_wifi.h"
#include "wifimg.h"

#include "lwip/netif.h"
#include "lwip/ip_addr.h"

#include <stdint.h>
#include <string.h>

#ifndef DIVOOM_WIFI_RSSI_UNKNOWN
#define DIVOOM_WIFI_RSSI_UNKNOWN  (-100)
#endif

extern uint8_t *get_mac_address(void);

static wmg_status_t divoom_wifi_compat_ensure_sta(void)
{
    if (aw_wifi_init(NULL) != 0) {
        return WMG_STATUS_FAIL;
    }

    aw_wifi_set_mode(AW_WIFI_MODE_STA);
    if (aw_wifi_start() != 0) {
        return WMG_STATUS_FAIL;
    }
    set_sta_not_autoconnect(0);

    return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_sta_connect(wifi_sta_cn_para_t *cn_para)
{
    const char *ssid;
    const char *password;

    if (cn_para == NULL || cn_para->ssid == NULL || cn_para->ssid[0] == '\0') {
        return WMG_STATUS_INVALID;
    }

    if (divoom_wifi_compat_ensure_sta() != WMG_STATUS_SUCCESS) {
        return WMG_STATUS_FAIL;
    }

    ssid = cn_para->ssid;
    password = cn_para->password ? cn_para->password : "";

    return (aw_wifi_sta_join(ssid, password) == 0) ?
           WMG_STATUS_SUCCESS : WMG_STATUS_FAIL;
}

wmg_status_t wifi_sta_remove_networks(char *ssid)
{
    (void)ssid;
    return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_sta_disconnect(void)
{
    aw_wifi_disconnect();
    return WMG_STATUS_SUCCESS;
}

wmg_status_t wifi_sta_get_info(wifi_sta_info_t *sta_info)
{
    aw_wifi_config_t conf;
    struct netif *netif;

    if (sta_info == NULL) {
        return WMG_STATUS_INVALID;
    }

    memset(sta_info, 0, sizeof(*sta_info));

    if (!aw_wifi_is_connected()) {
        return WMG_STATUS_FAIL;
    }

    if (aw_wifi_get_config(AW_WIFI_IF_STA, &conf) == 0) {
        strncpy(sta_info->ssid, (const char *)conf.sta.ssid, sizeof(sta_info->ssid) - 1);
        switch (conf.sta.authmode) {
        case AW_WIFI_AUTH_OPEN:
            sta_info->sec = WIFI_SEC_NONE;
            break;
        case AW_WIFI_AUTH_WEP:
            sta_info->sec = WIFI_SEC_WEP;
            break;
        case AW_WIFI_AUTH_WPA_PSK:
            sta_info->sec = WIFI_SEC_WPA_PSK;
            break;
        case AW_WIFI_AUTH_WPA3_PSK:
            sta_info->sec = WIFI_SEC_WPA3_PSK;
            break;
        default:
            sta_info->sec = WIFI_SEC_WPA2_PSK;
            break;
        }
    }

    {
        uint8_t *m_addr = get_mac_address();

        if (m_addr != NULL) {
            memcpy(sta_info->mac_addr, m_addr, sizeof(sta_info->mac_addr));
        }
    }

    {
        int8_t rssi;

        if (aw_wifi_sta_get_rssi(&rssi) == 0) {
            sta_info->rssi = rssi;
        } else {
            sta_info->rssi = DIVOOM_WIFI_RSSI_UNKNOWN;
        }
    }

    netif = netif_find("wl1");
    if (netif != NULL) {
        uint32_t ip = ip4_addr_get_u32(netif_ip4_addr(netif));
        uint32_t gw = ip4_addr_get_u32(netif_ip4_gw(netif));

        sta_info->ip_addr[0] = (uint8_t)(ip & 0xff);
        sta_info->ip_addr[1] = (uint8_t)((ip >> 8) & 0xff);
        sta_info->ip_addr[2] = (uint8_t)((ip >> 16) & 0xff);
        sta_info->ip_addr[3] = (uint8_t)((ip >> 24) & 0xff);

        sta_info->gw_addr[0] = (uint8_t)(gw & 0xff);
        sta_info->gw_addr[1] = (uint8_t)((gw >> 8) & 0xff);
        sta_info->gw_addr[2] = (uint8_t)((gw >> 16) & 0xff);
        sta_info->gw_addr[3] = (uint8_t)((gw >> 24) & 0xff);
    }

    return WMG_STATUS_SUCCESS;
}
