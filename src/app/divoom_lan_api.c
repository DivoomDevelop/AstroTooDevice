/* Dedicated synchronous HTTP commands: replies stay on their originating connection. */
#include <stdio.h>
#include <string.h>
#include "divoom.h"
#include "divoom_lan_storage.h"
#include "divoom_watchface_local_api.h"
#include "divoom_clock_manage.h"
#include "divoom_light_app.h"
#include "divoom_database_manage.h"
static int lan_number(divoom_cJSON *r, const char *key, int min, int max, int *out) {
    divoom_cJSON *v = divoom_cJSON_GetObjectItem(r, key);
    if(!v || !(v->type & divoom_cJSON_Number) || v->valuedouble != v->valueint ||
       v->valueint < min || v->valueint > max) return 0;
    *out = v->valueint; return 1;
}
char *divoom_lan_dispatch(const char *json, int *handled) {
    divoom_cJSON *r = divoom_cJSON_Parse(json), *c, *out;
    const char *cmd;
    char *result = NULL;
    int cid;
    if(handled) *handled = 1;
    if(!r) return divoom_watchface_local_http_error_json("", "invalid JSON");
    c = divoom_cJSON_GetObjectItem(r, "Command");
    if(!c || !(c->type & divoom_cJSON_String) || !c->valuestring) {
        divoom_cJSON_Delete(r);
        return divoom_watchface_local_http_error_json("", "Command required");
    }
    cmd = c->valuestring;
    out = divoom_cJSON_CreateObject();
    if(!out) { divoom_cJSON_Delete(r); return NULL; }
    divoom_cJSON_AddStringToObject(out, "Command", cmd);
    divoom_cJSON_AddStringToObject(out, "DeviceType", "AstroToo");
    divoom_cJSON_AddNumberToObject(out, "ReturnCode", 0);
    if(!strcmp(cmd, "Device/GetLanCapabilities") || !strcmp(cmd, "Device/GetHardwareVersion")) {
        divoom_cJSON_AddStringToObject(out, "Model", "astrotoo");
        divoom_cJSON_AddNumberToObject(out, "FirmwareVersion", DIVOOM_SOFTWARE_VERSION);
        divoom_cJSON_AddNumberToObject(out, "Hardware", DIVOOM_HARDWARE_VERSION);
        divoom_cJSON_AddNumberToObject(out, "Software", DIVOOM_SOFTWARE_VERSION);
        divoom_cJSON_AddNumberToObject(out, "LanApiVersion", 1);
        divoom_cJSON_AddNumberToObject(out, "CanvasWidth", 480);
        divoom_cJSON_AddNumberToObject(out, "CanvasHeight", 480);
        /* TimesFrame 视频最长 30 分钟；512 MiB 包含媒体和视频预览图。 */
        divoom_cJSON_AddBoolToObject(out, "SupportsPhotoLibrary", 1);
        divoom_cJSON_AddNumberToObject(out, "PhotoUploadMaxBytes", 512 * 1024 * 1024);
        divoom_cJSON_AddNumberToObject(out, "PhotoVideoMaxSeconds", 30 * 60);
        divoom_cJSON_AddBoolToObject(out, "LocalOnly", 1);
        divoom_cJSON_AddNumberToObject(out, "JsonMaxBytes", 65536);
        divoom_cJSON_AddNumberToObject(out, "BackgroundMaxBytesExclusive", 512000);
        divoom_cJSON_AddNumberToObject(out, "BundleMaxBytes", 0);
        divoom_cJSON_AddBoolToObject(out, "SupportsAssetBundle", 0);
        divoom_cJSON_AddStringToObject(out, "AssetTransfer", "sequential");
        divoom_cJSON_AddStringToObject(out, "LocalAssetUploadPath", "/upload_local_asset");
        divoom_cJSON_AddStringToObject(out, "UploadPersistence", "temporary-until-bound");
        divoom_cJSON_AddBoolToObject(out, "OutboundFileUpload", 0);
        divoom_cJSON_AddBoolToObject(out, "SupportsOSS", 0);

        divoom_cJSON_AddBoolToObject(out, "SupportsClockShare", 0);
        divoom_cJSON_AddBoolToObject(out, "SupportsClockUploadVisibility", 0);
        divoom_cJSON_AddBoolToObject(out, "SupportsClockUpdateMessageInfo", 0);
        divoom_cJSON_AddBoolToObject(out, "SupportsClockUploadClassify", 0);
        divoom_cJSON_AddStringToObject(out, "AppPreviewUploadFormat", "webp");
        divoom_cJSON_AddNumberToObject(out, "AppPreviewMaxWidth", 480);
        divoom_cJSON_AddNumberToObject(out, "AppPreviewMaxHeight", 480);
        divoom_cJSON_AddStringToObject(out, "SnapshotPath", DIVOOM_DEVICE_SCREEN_SNAPSHOT_PATH);
    } else if(!strcmp(cmd, "Photo/GetLocalAlbumList")) {
        divoom_cJSON *list = divoom_database_manage_get_lan_album_list();
        if(list == NULL)
        {
            goto unavailable;
        }
        divoom_cJSON_AddItemToObject(out, "AlbumList", list);
    } else if(!strcmp(cmd, "Photo/GetPhotoList") && divoom_cJSON_GetObjectInt(r, "ClockId") >= DIVOOM_PHOTO_START_CLOCK_ID) {
        int start;
        int end;
        divoom_cJSON *list;
        if(!lan_number(r, "ClockId", DIVOOM_PHOTO_START_CLOCK_ID, 2147483647, &cid) ||
           !lan_number(r, "StartNum", 1, 2147483547, &start) ||
           !lan_number(r, "EndNum", start, start + 99, &end))
        {
            goto invalid;
        }
        list = divoom_database_manage_get_lan_photo_list(cid, start, end);
        if(list == NULL)
        {
            goto unavailable;
        }
        divoom_cJSON_AddItemToObject(out, "PhotoList", list);
    } else if(!strcmp(cmd, "Device/ShareLocalClock") ||
              !strcmp(cmd, "Device/DeleteLocalClock") ||
              !strcmp(cmd, "Channel/Upload_device_clock_info") ||
              (!strcmp(cmd, "Device/UploadLog"))) {
        result = divoom_watchface_local_http_error_json(cmd, "unsupported in open-source firmware");
    } else {
        /* 标准命令统一交给 divoom_net_app；本地专用命令才在此同步返回。 */
        if(handled) *handled = 0;
        goto done;
    }
    if(!result) result = divoom_cJSON_Print(out);
    goto done;
invalid:
    result = divoom_watchface_local_http_error_json(cmd, "invalid or missing parameter");
    goto done;
unavailable:
    result = divoom_watchface_local_http_error_json(cmd, "local resource or operation unavailable");
done:
    divoom_cJSON_Delete(out);
    divoom_cJSON_Delete(r);
    return result;
}
