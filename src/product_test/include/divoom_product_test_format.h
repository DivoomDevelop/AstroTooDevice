#ifndef DIVOOM_PRODUCT_TEST_FORMAT_H
#define DIVOOM_PRODUCT_TEST_FORMAT_H

typedef enum {
	DIVOOM_PT_FORMAT_IDLE,
	DIVOOM_PT_FORMAT_WAIT_NETWORK,
	DIVOOM_PT_FORMAT_GUIDE,
	DIVOOM_PT_FORMAT_VOLUME,
	DIVOOM_PT_FORMAT_RESTORE,
	DIVOOM_PT_FORMAT_FAILED,
	DIVOOM_PT_FORMAT_DONE,
	DIVOOM_PT_FORMAT_CANCELLED,
	DIVOOM_PT_FORMAT_CHECK,
	DIVOOM_PT_FORMAT_INPUT_WIFI,
    DIVOOM_PT_FORMAT_RETRY,
    DIVOOM_PT_FORMAT_CHECK_PASSED,
    DIVOOM_PT_FORMAT_REPAIR_CONFIRM
} divoom_product_test_format_phase;

typedef struct {
    unsigned format_mask; /* Bit 0 user partition, bit 1 system partition. */
    int structure_files[2];
    int download_files;
    int check_system_files;
} divoom_product_test_repair_plan;

typedef struct {
	divoom_product_test_format_phase phase;
	int target_usr;
	int busy;
	int stopped; /* Exhausted automatic recovery; keep durable intent for next boot. */
	int volume_started; /* 清卷开始后禁止离开页面，失败也只能重试。 */
	int formatted;
    int can_cancel; /* Only an uncommitted intent on mounted volumes may exit. */
	int done;
	int total;
	int error_times;
    int repair_retries; /* Whole check/repair retries; independent of file retries. */
    int error_files; /* Files identified by the check; -1 if no scan/list exists. */
    int fs_check_target;
    unsigned fs_check_stage;
    unsigned fs_clean_mask; /* Bit 0 user partition, bit 1 system partition. */
    divoom_product_test_repair_plan repair_plan;
    unsigned repair_request;
    int repair_declined;
	int file_percent; /* -1 until a resource file starts. */
	char wifi_ssid[33];
	char wifi_password[65];
	const char *error_key;
    divoom_product_test_format_phase failure_phase;
    int failure_target_usr, failure_line, failure_errno;
    int failure_has_code, failure_code;
    int failure_raw_errno;
    const char *failure_stage; /* Static stage identifier, never worker stack storage. */
} divoom_product_test_format_status;

#define DIVOOM_PT_WIFI_MAX_NETWORKS 32
typedef struct {
    char ssid[33];
    int rssi;
    int open;
} divoom_product_test_wifi_network;
typedef enum {
    DIVOOM_PT_WIFI_SCANNING,
    DIVOOM_PT_WIFI_READY,
    DIVOOM_PT_WIFI_EMPTY,
    DIVOOM_PT_WIFI_SCAN_FAILED
} divoom_product_test_wifi_scan_state;
typedef struct {
    unsigned int generation;
    divoom_product_test_wifi_scan_state state;
    int count;
    int connect_failed;
    divoom_product_test_wifi_network networks[DIVOOM_PT_WIFI_MAX_NETWORKS];
} divoom_product_test_wifi_scan;
void divoom_product_test_format_get_wifi_scan(divoom_product_test_wifi_scan *scan);
int divoom_product_test_format_rescan_wifi(void);

/* Exists until all /usr resources have been verified and installed. */
#define DIVOOM_SYSTEM_FORMAT_PENDING "/userdata/.usr_restore_pending"
/* C: check, P: partial repair, F: cancellable intent, M: erase committed,
 * R: restore, V: verified/cleanup. M/R are never cancellable. */
int divoom_product_test_recovery_mark(char state);
int divoom_product_test_recovery_state(void);
int divoom_product_test_format_submit_wifi(const char *ssid, const char *password);
void divoom_product_test_format_set_phase(divoom_product_test_format_phase phase);
void divoom_product_test_format_set_error_files(int count);
int divoom_product_test_format_needs_recovery(void);
int divoom_product_test_format_resume(void);
int divoom_product_test_format_check_start(void);
int divoom_product_test_format_check_continue(void);
/* Second marker byte preserves the Settings confirmation policy across power loss. */
int divoom_product_test_format_arm_settings_check(void);
/* Live About UI -> main worker handoff: 0 queued, 1 accepted, -1 rejected. */
void divoom_product_test_format_settings_request_set_status(int status);
int divoom_product_test_format_settings_request_status(void);
int divoom_product_test_format_is_settings_check(void); /* -1: unreadable; never auto-erase. */
int divoom_product_test_format_check_confirm_restart(void);
int divoom_product_test_format_submit_repair(unsigned request, int approve);
/* Worker only, before publishing a new destructive action. */
int divoom_product_test_format_allow_repair(const divoom_product_test_repair_plan *plan);
void divoom_product_test_format_init(void);
void divoom_product_test_format_poll(void);
void divoom_product_test_format_fs_progress(int target_usr, unsigned stage, unsigned done, unsigned total);
void divoom_product_test_format_fs_result(int target_usr, int clean);
int divoom_product_test_format_start(int target_usr);
void divoom_product_test_format_cancel(void);
void divoom_product_test_format_get_status(divoom_product_test_format_status *status);
#endif
