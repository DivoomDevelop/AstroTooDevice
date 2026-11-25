#ifndef DIVOOM_PATHS_H
#define DIVOOM_PATHS_H

/*
 * AstroToo storage layout (F101 RTOS):
 *   /usr      — preset partition (read-mostly, data_usr.fex)
 *   /userdata — user/runtime data (UDISK; format on first boot, seed from /usr)
 *
 * No /tmp, /etc, /data on device; use DIVOOM_TMP_DIR and DIVOOM_USER_*_DIR below.
 *
 * DIVOOM_SD_* in divoom.h are legacy names (meant external SD on Linux); on AstroToo
 * they map to /userdata subtrees — see DIVOOM_USER_SYSTEM_DIR etc.
 */

#define DIVOOM_PRESET_ROOT          "/usr/share/divoom_app"
#define DIVOOM_USER_ROOT            "/userdata"
#define DIVOOM_USER_SYSTEM_DIR      DIVOOM_USER_ROOT "/system"
#define DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH DIVOOM_USER_SYSTEM_DIR "/base_light.cfg"
#define DIVOOM_BAK_SYSTEM_DIR      DIVOOM_PRESET_ROOT "/system"

#define DIVOOM_TMP_DIR              DIVOOM_USER_ROOT "/tmp"

#define DIVOOM_USER_WIFI_DIR        DIVOOM_USER_ROOT "/wifi"
#define DIVOOM_USER_BT_DIR          DIVOOM_USER_ROOT "/bluetooth"
#define DIVOOM_USER_SSL_DIR         DIVOOM_USER_ROOT "/ssl"
#define DIVOOM_USER_SSL_CERTS_DIR   DIVOOM_USER_SSL_DIR "/certs"
#define DIVOOM_USER_SSL_PRIVATE_DIR DIVOOM_USER_SSL_DIR "/private"

#define DIVOOM_WPA_SUPPLICANT_CONF      DIVOOM_USER_SYSTEM_DIR "/wifi.conf"
/* wifimanager freertos_sta.c legacy path — same file tree */

#define DIVOOM_BT_MAIN_CONF         DIVOOM_USER_BT_DIR "/main.conf"

#define DIVOOM_SSL_CERT_FILE        DIVOOM_USER_SSL_CERTS_DIR "/server.crt"
#define DIVOOM_SSL_KEY_FILE         DIVOOM_USER_SSL_PRIVATE_DIR "/server.key"
#define DIVOOM_SSL_CA_BUNDLE_FILE   DIVOOM_USER_SSL_CERTS_DIR "/ca-certificates.crt"

/* Legacy Linux /sdcard — on AstroToo map to userdata until external SD is enabled */
#define DIVOOM_LEGACY_SD_ROOT       DIVOOM_USER_ROOT

#define DIVOOM_PRESET_RECOMMEND_DIR DIVOOM_PRESET_ROOT "/divoom"
#define DIVOOM_WRITE_RECOMMEND_DIR DIVOOM_USER_ROOT "/divoom"

#endif /* DIVOOM_PATHS_H */
