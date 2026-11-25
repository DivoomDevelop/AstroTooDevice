#include "aic_wifi.h"
#include "fhost.h"
#include "wlan_if.h"

#include <string.h>

int aw_wifi_sta_get_rssi(int8_t *rssi)
{
    struct fhost_vif_tag *fhost_vif;
    struct sta_info_tag *sta;
    struct station_info sta_info;

    if (rssi == NULL || !aw_wifi_is_connected()) {
        return -1;
    }

    fhost_vif = &fhost_env.vif[0];
    if (fhost_vif->mac_vif == NULL || fhost_vif->mac_vif->type != VIF_STA) {
        return -1;
    }
    if (fhost_vif->ap_id >= NX_REMOTE_STA_MAX) {
        return -1;
    }
    sta = vif_mgmt_get_sta_by_staid(fhost_vif->ap_id);
    if (sta == NULL) {
        return -1;
    }

    memset(&sta_info, 0, sizeof(sta_info));
    if (aicwf_get_station_info(sta, &sta_info, NULL, NULL, NULL) < 0) {
        return -1;
    }
    *rssi = sta_info.signal;
    return 0;
}
