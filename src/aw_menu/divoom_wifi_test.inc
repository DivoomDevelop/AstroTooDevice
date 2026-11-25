/* Included only from settings_wifi.c under DIVOOM_WIFI_TEST_ENABLE. */
#include <console.h>
#include <stdlib.h>
#include "divoom_user_key.h"

static volatile int g_wifi_test_api_running;
static int g_wifi_test_api_ret, g_wifi_test_api_count;
static uint32 g_wifi_test_request_seq;

typedef struct {
    uint32 seq;
    int action;
    char ssid[33];
    char password[64];
} divoom_wifi_test_request;

static void divoom_wifi_test_ui(void *arg)
{
    divoom_wifi_test_request *req = arg;
    Pdivoom_astrootoo_settings_ctrl ph = NULL;
    int menu = divoom_menu_control_get_active_menu_id();
    int i, found = -1;
    if (menu == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI
        || menu == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD
        || menu == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_PASSWORD)
        ph = divoom_menu_control_get_clock_mode_menu_handle();
    if (req->action == 1 || req->action == 2) {
        int target = req->action == 1 ? DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI
            : DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD;
        if (menu != target)
            divoom_menu_control_load_next(target, NULL, 0, 0);
    } else if (req->action == 3 && ph
        && ph->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD) {
        for (i = 0; i < ph->wifi_net_count; i++) {
            if (!strcmp(ph->wifi_ssid[i], req->ssid)) { found = i; break; }
        }
        if (found >= 0)
            divoom_astrootoo_settings_wifi_connect_begin(ph, found, req->password, 0);
    }
    divoom_print("[WIFI_TEST] UI seq=%u action=%d menu=%d found=%d\n",
        req->seq, req->action, menu, found);
    memset(req->password, 0, sizeof(req->password));
    divoom_system_free(req);
}

static void divoom_wifi_test_status(void)
{
    extern int divoom_wifi_test_sta_frequency(void);
    Pdivoom_astrootoo_settings_ctrl ph = NULL;
    int menu, locked;
    char ssid[33] = {0};
    divoom_wifi_get_connect_ssid_info(ssid);
    locked = divoom_get_lvgl_try_lock() == 0;
    menu = divoom_menu_control_get_active_menu_id();
    if (locked && (menu == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI
        || menu == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD
        || menu == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_PASSWORD))
        ph = divoom_menu_control_get_clock_mode_menu_handle();
    /* Fixed numeric fields are parseable without embedding unescaped SSIDs. */
    divoom_print("[WIFI_TEST] STATUS {\"menu\":%d,\"page\":%d,\"rows\":%d,"
        "\"finished\":%d,\"spinner\":%d,\"scan\":%d,\"scan_seq\":%u,"
        "\"cancel\":%d,\"flow\":%d,\"attempt\":%d,\"pending\":%d,"
        "\"link\":%d,\"ready\":%d,\"result\":%d,\"reason\":%d,"
        "\"api_running\":%d,\"api_ret\":%d,\"api_count\":%d,\"ui_busy\":%d,\"frequency\":%d}\n",
        menu, ph ? ph->page : -1, ph ? ph->wifi_row_count : -1,
        ph ? ph->wifi_scan_finished : -1, ph && ph->wifi_loading_gif_handle ? 1 : 0,
        g_wifi_scan_cache.status, g_wifi_diag_scan_seq, g_wifi_scan_cancel_requested,
        g_wifi_flow.connect_state, g_wifi_flow.connect_attempt,
        divoom_wifi_is_stop_connect_pending(), aw_wifi_is_connected() ? 1 : 0,
        divoom_wifi_model_is_network_ready(), divoom_wifi_get_manual_connect_result(),
        divoom_wifi_get_manual_connect_error_reason(), g_wifi_test_api_running,
        g_wifi_test_api_ret, g_wifi_test_api_count, !locked, divoom_wifi_test_sta_frequency());
    if (ph) {
        for (int i = 0; i < ph->wifi_net_count; i++)
            divoom_print("[WIFI_TEST] ROW index=%d rssi=%d ssid=%s\n",
                i, ph->wifi_rssi[i], ph->wifi_ssid[i]);
    }
    if (locked) divoom_free_lvgl_lock();
    divoom_print("[WIFI_TEST] CONNECTED ssid=%s\n", ssid);
}

static void *divoom_wifi_test_scan(void *arg)
{
    uint32 seq = (uint32)(uintptr_t)arg;
    uint32 start = divoom_system_get_tickms();
    uint16 count = 0;
    while (divoom_wifi_is_stop_connect_pending()
        && divoom_system_get_tickms() - start < 5000)
        divoom_system_delay_ms(20);
    g_wifi_test_api_ret = -1;
    if (!divoom_wifi_is_stop_connect_pending() && !divoom_wifi_ensure_sta_started())
        g_wifi_test_api_ret = aw_wifi_scan_start(NULL, true);
    if (!g_wifi_test_api_ret) aw_wifi_scan_get_ap_num(&count);
    g_wifi_test_api_count = count;
    divoom_print("[WIFI_TEST] SCAN_DONE seq=%u ret=%d count=%d elapsed_ms=%u\n",
        seq, g_wifi_test_api_ret, count, divoom_system_get_tickms() - start);
    g_wifi_test_api_running = 0;
    return NULL;
}

static int divoom_wifi_test_cli(int argc, char **argv)
{
    int base = argc > 0 && !strcmp(argv[0], "wifi_test") ? 1 : 0;
    const char *action = argc > base ? argv[base] : "help";
    uint32 seq = ++g_wifi_test_request_seq;
    divoom_wifi_test_request *req;
    if (!strcmp(action, "status")) {
        divoom_wifi_test_status();
    } else if (!strcmp(action, "pause")) {
        divoom_wifi_stop_connect_wifi();
    } else if (!strcmp(action, "resume")) {
        divoom_wifi_allow_saved_reconnect();
    } else if (!strcmp(action, "fault") && argc == base + 3) {
        char *end;
        long n = strtol(argv[base+2], &end, 10);
        if (*end || n < 0 || n > 65000 || g_wifi_test_api_running
            || g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_RUNNING) return -1;
        if (!strcmp(argv[base+1], "fail") && n <= 3) g_wifi_test_scan_failures = n;
        else if (!strcmp(argv[base+1], "empty") && n <= 3) g_wifi_test_scan_empty = n;
        else if (!strcmp(argv[base+1], "delay")) g_wifi_test_scan_delay_ms = n;
        else return -1;
    } else if (!strcmp(action, "connect") && argc == base + 3) {
        if (!argv[base+1][0] || strlen(argv[base+1]) > 32 || strlen(argv[base+2]) > 63)
            return -1;
        if (g_wifi_test_api_running || g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_RUNNING)
            return -1;
        divoom_wifi_connect_ssid(argv[base+1], argv[base+2]);
    } else if (!strcmp(action, "scan")) {
        if (g_wifi_test_api_running || g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_RUNNING) {
            divoom_print("[WIFI_TEST] BUSY seq=%u\n", seq);
            return -1;
        }
        g_wifi_test_api_running = 1;
        g_wifi_test_api_count = 0;
        divoom_wifi_stop_connect_wifi();
        if (!divoom_system_create_task(divoom_wifi_test_scan, "wifi_tscan",
                (void *)(uintptr_t)seq, 4096)) {
            g_wifi_test_api_running = 0;
            return -1;
        }
    } else if (!strcmp(action, "ui") && argc >= base + 2) {
        const char *sub = argv[base+1];
        if (!strcmp(sub, "key") && argc == base+3) {
            int key;
            if (!strcmp(argv[base+2], "esc")) key = DIVOOM_KEY_ESC;
            else if (!strcmp(argv[base+2], "enter")) key = DIVOOM_KEY_PLAY;
            else if (!strcmp(argv[base+2], "up")) key = DIVOOM_KEY_PREV;
            else if (!strcmp(argv[base+2], "down")) key = DIVOOM_KEY_NEXT;
            else return -1;
            divoom_user_key_send_key_info(key, DIVOOM_PRESS_SHORT);
        } else {
            int operation = !strcmp(sub,"wifi") ? 1 : !strcmp(sub,"add") ? 2
                : !strcmp(sub,"connect") && argc == base+4 ? 3 : 0;
            if (!operation || g_wifi_test_api_running) return -1;
            req = divoom_system_malloc(sizeof(*req));
            if (!req) return -1;
            memset(req, 0, sizeof(*req));
            req->seq = seq;
            req->action = operation;
            if (operation == 3) {
                if (strlen(argv[base+2]) > 32 || strlen(argv[base+3]) > 63) {
                    divoom_system_free(req); return -1;
                }
                strcpy(req->ssid, argv[base+2]); strcpy(req->password, argv[base+3]);
            }
            divoom_get_lvgl_lock();
            if (lv_async_call(divoom_wifi_test_ui, req) != LV_RESULT_OK) {
                divoom_system_free(req); divoom_free_lvgl_lock(); return -1;
            }
            divoom_free_lvgl_lock();
        }
    } else if (!strcmp(action, "stop")) {
        divoom_get_lvgl_lock();
        g_wifi_scan_cancel_requested = 1;
        divoom_free_lvgl_lock();
    } else {
        divoom_print("wifi_test status|pause|resume|scan|stop|connect SSID PASSWORD\n"
            "wifi_test fault fail/empty COUNT(0..3)|delay MS(0..65000)\n"
            "wifi_test ui wifi|add|key esc/enter/up/down|connect SSID PASSWORD\n");
        return 0;
    }
    divoom_print("[WIFI_TEST] ACK seq=%u action=%s\n", seq, action);
    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_wifi_test_cli, wifi_test, Laboratory WiFi whitebox control);
