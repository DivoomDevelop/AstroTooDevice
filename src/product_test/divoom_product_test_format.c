#if !defined(_WIN32)
#include <sys/errno.h> /* Constants must not come from the wireless iw/errno.h. */
#endif
#include "divoom.h"
#include "divoom_wifi_con.h"
#include "divoom_product_test_format.h"
#include "divoom_product_test_wifi_fw.h"
#include "divoom_product_test_repair.h"
#include "divoom_boot_health.h"
#include "divoom_product_test_userdata_wifi.h"
#include "divoom_recovery_journal.h"
#include "divoom_recovery_integrity.h"
#include "divoom_port_fs_check.h"
#include <errno.h>
#include <stdio.h>
#ifdef DIVOOM_SIMULATOR
#include "sim_aic_wifi.h"
#else
#include "aic_wifi.h"
#endif
#ifdef DIVOOM_SIMULATOR
#include "divoom_product_test_ui.h"
#include "divoom_lvgl_product_test_menu.h"
#include "divoom_user_key.h"
#include <stdlib.h>
#include <stdio.h>
#include "sim_ui_visual.h"
#endif

typedef struct {
	void *lock;
	int cancel;
	int manual_check, reboot_confirmed;
    int manual_preview, repair_answer;
    divoom_product_test_repair_plan approved_plan;
    int reset_recovery, auto_userdata, worker_alive, explicit_usr_format, explicit_userdata_format;
    unsigned failed_format_mask, unrecorded_format_mask;
    unsigned preview_format_mask, launch_failures;
    uint32 last_progress_tick;
	void *wifi_submit;
	int wifi_submitted; /* 1: connect, 2: rescan; serialized by lock + wifi_submit. */
    divoom_product_test_wifi_scan wifi_scan;
	divoom_product_test_format_status status;
} divoom_product_test_format_ctrl;

enum { DIVOOM_PT_CHECK_MENU = 1, DIVOOM_PT_CHECK_SETTINGS, DIVOOM_PT_CHECK_AUTOMATIC };
static divoom_product_test_format_ctrl s_format;
static int s_settings_request_status;
static volatile int s_format_retry_requested;
static int s_format_retry_target, s_format_retry_mode = -1;
static uint32 s_format_retry_after;

#define DIVOOM_PT_AUTO_ATTEMPTS 3U
#define DIVOOM_PT_STALL_TIMEOUT_MS 120000U
#define DIVOOM_PT_INPUT_TIMEOUT_MS 300000U

/* Capture before taking locks or saving the journal can change errno. The
 * UI receives a value snapshot, never a pointer to a worker stack buffer. */
static void divoom_product_test_format_failure_locked(int line, int error_number, int has_code, int code)
{
    if (s_format.status.phase != DIVOOM_PT_FORMAT_FAILED)
        s_format.status.failure_phase = s_format.status.phase;
    s_format.status.failure_target_usr = s_format.status.fs_check_stage ?
        s_format.status.fs_check_target : s_format.status.target_usr;
    s_format.status.failure_line = line;
    s_format.status.failure_errno = error_number;
    s_format.status.failure_raw_errno = error_number;
    s_format.status.failure_stage = NULL;
    s_format.status.failure_has_code = has_code;
    s_format.status.failure_code = code;
}

/* Caller holds the UI lock (or initialization has not obtained one yet).
 * Do not release in-flight worker resources or erase its durable transaction. */
static void divoom_product_test_format_stop_locked(const char *reason, const char *stage,
    int error_number, int line)
{
    if (s_format.status.stopped) return;
    if (s_format.status.phase != DIVOOM_PT_FORMAT_FAILED || !s_format.status.failure_line)
        divoom_product_test_format_failure_locked(line, error_number, 0, 0);
    s_format.status.phase = DIVOOM_PT_FORMAT_FAILED;
    if (reason || !s_format.status.error_key)
        s_format.status.error_key = reason ? reason : "Retry limit reached";
    if (stage) s_format.status.failure_stage = stage;
    s_format.status.stopped = 1;
    s_format.status.busy = s_format.status.can_cancel = 0;
    s_format.cancel = 1;
    s_format_retry_requested = 0;
}

static void divoom_product_test_format_stop_at(const char *reason, const char *stage,
    int error_number, int line)
{
    divoom_system_get_lock_direct(s_format.lock);
    divoom_product_test_format_stop_locked(reason, stage, error_number, line);
    divoom_system_free_lock_direct(s_format.lock);
    divoom_print("recovery: stopped stage=%s errno=%d site=F%d\n", stage ? stage : "worker", error_number, line);
}
#define divoom_product_test_format_stop(reason, stage, error_number) \
    divoom_product_test_format_stop_at(reason, stage, error_number, __LINE__)

static void divoom_product_test_format_schedule_retry(int target_usr, int mode, int locked)
{
    if (!locked && s_format.lock) divoom_system_get_lock_direct(s_format.lock);
    s_format_retry_target = target_usr; s_format_retry_mode = mode;
    if (++s_format.launch_failures >= DIVOOM_PT_AUTO_ATTEMPTS)
        divoom_product_test_format_stop_locked(NULL, "worker_start", errno ? errno : ENOMEM, __LINE__);
    else {
        s_format_retry_after = divoom_system_get_tickms() + 10000;
        s_format_retry_requested = 1;
    }
    if (!locked && s_format.lock) divoom_system_free_lock_direct(s_format.lock);
}

void divoom_product_test_format_settings_request_set_status(int status)
{
    __atomic_store_n(&s_settings_request_status, status, __ATOMIC_RELEASE);
}

int divoom_product_test_format_settings_request_status(void)
{
    return __atomic_load_n(&s_settings_request_status, __ATOMIC_ACQUIRE);
}

int divoom_product_test_format_needs_recovery(void)
{
    if (divoom_recovery_journal_pending() != 0) return 1;
    divoom_port_stat_t st;
    errno = 0;
    if (divoom_boot_health_userdata_mount_failed() || divoom_boot_health_mount_failed() ||
        divoom_port_stat(DIVOOM_SYSTEM_FORMAT_PENDING, &st) == 0 || errno != ENOENT) return 1;
    /* A reset intent is flushed here before removing an invalid formal marker.
     * Also recover an interrupted initial publish instead of booting normally. */
    errno = 0;
    if (divoom_port_stat(DIVOOM_SYSTEM_FORMAT_PENDING ".tmp", &st) == 0 || errno != ENOENT) return 1;
    return divoom_product_test_userdata_wifi_pending() != 0;
}

static int divoom_product_test_format_marker_state(void)
{
    void *fp;
    char state = '?';
    int needed = divoom_product_test_format_needs_recovery();
    int saved_errno = errno;
    printf("[RECOVERY_MARKER] path=%s stage=state.probe needed=%d errno=%d usr_mount_failed=%d userdata_mount_failed=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, needed, saved_errno,
        divoom_boot_health_mount_failed(), divoom_boot_health_userdata_mount_failed());
    errno = saved_errno;
    if (!needed) return 0;
    errno = 0;
    fp = divoom_port_file_open(DIVOOM_SYSTEM_FORMAT_PENDING, "rb");
    saved_errno = errno;
    printf("[RECOVERY_MARKER] path=%s stage=state.open rc=%d errno=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, fp ? 0 : -1, saved_errno);
    errno = saved_errno;
    if (!fp) return '?';
    errno = 0;
    size_t count = divoom_port_file_read(&state, 1, 1, fp);
    saved_errno = errno;
    printf("[RECOVERY_MARKER] path=%s stage=state.read count=%u expected=1 byte0=%02x ferror=%d feof=%d errno=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, (unsigned)count, (unsigned char)state,
        ferror((FILE *)fp), feof((FILE *)fp), saved_errno);
    errno = saved_errno;
    if (count != 1) state = '?';
    errno = 0;
    int close_rc = divoom_port_file_close(fp);
    saved_errno = errno;
    printf("[RECOVERY_MARKER] path=%s stage=state.close rc=%d errno=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, close_rc, saved_errno);
    errno = saved_errno;
    if (close_rc != 0) state = '?';
    if (state == 'F' || state == 'M' || state == 'R' ||
        state == 'C' || state == 'P' || state == 'V') return state;
    printf("[RECOVERY_MARKER] path=%s stage=state.invalid byte0=%02x errno=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, (unsigned char)state, saved_errno);
    errno = saved_errno;
    return '?';
}

/* Publish only a fully flushed initial marker. A power cut while creating
 * the temporary file must not leave an empty formal recovery marker. */
static int divoom_product_test_format_prepare_marker(char state, int settings)
{
    const char *temp = DIVOOM_SYSTEM_FORMAT_PENDING ".tmp";
    char bytes[2] = { state, 'S' };
    size_t size = settings ? 2 : 1;
    void *fp = divoom_port_file_open(temp, "wb");
    if (!fp) { divoom_product_test_repair_record_io(temp,"marker.open",errno); return 0; }
    errno=0;
    int ok = divoom_port_file_write(bytes, 1, size, fp) == size &&
        divoom_port_file_sync(fp) == 0;
    if (!ok) divoom_product_test_repair_record_io(temp,"marker.write_sync",errno?errno:EIO);
    if (divoom_port_file_close(fp) != 0) {divoom_product_test_repair_record_io(temp,"marker.close",errno);ok = 0;}
    /* This may be the only reset intent after an interrupted rename. Keep
     * even a failed/empty temp file: the next boot must still enter recovery. */
    if (ok && divoom_port_sync() != 0) {divoom_product_test_repair_record_io(temp,"marker.sync",errno);ok=0;}
    return ok;
}

static int divoom_product_test_format_publish_marker(char state, int settings)
{
    if (!divoom_product_test_format_prepare_marker(state, settings)) return 0;
    int ok=divoom_port_rename_file(DIVOOM_SYSTEM_FORMAT_PENDING ".tmp",
        DIVOOM_SYSTEM_FORMAT_PENDING) == 0 && divoom_port_sync() == 0;
    if (!ok) divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.rename_sync",errno);
    return ok;
}

static int divoom_product_test_format_marker_write_checked(char state, int confirmed_storage_failure)
{
    int existing = 0;
    void *probe;
    errno = 0;
    probe = divoom_port_file_open(DIVOOM_SYSTEM_FORMAT_PENDING, "rb");
    if (probe) {
        char previous;
        if (divoom_port_file_read(&previous, 1, 1, probe) != 1) existing = '?';
        else existing = previous;
        if (divoom_port_file_close(probe) != 0) existing = '?';
    } else if (errno != ENOENT) existing = '?';
	int ok;
	void *fp;
	if (existing == state || (existing == 'M' && state == 'F')) {
        errno=0;int same_ok=divoom_port_sync()==0;
        if(!same_ok)divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.already_sync",errno?errno:EIO);
        return same_ok; /* Never downgrade an interrupted erase. */
    }
	if (existing == '?' || (existing == 'R' && state != 'R' && state != 'V' &&
        !((state == 'F' || state == 'M') && (divoom_boot_health_mount_failed() || confirmed_storage_failure))))
		return 0;
    if (!existing) return divoom_product_test_format_publish_marker(state, 0);
	fp = divoom_port_file_open(DIVOOM_SYSTEM_FORMAT_PENDING, "r+b");
	if (!fp) {divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.reopen",errno);return 0;}
    errno=0;
	ok = divoom_port_file_write(&state, 1, 1, fp) == 1 && divoom_port_file_sync(fp) == 0;
    if (!ok) divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.update_sync",errno?errno:EIO);
	if (divoom_port_file_close(fp) != 0) {divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.close",errno);ok = 0;}
    if (divoom_port_sync()!=0) {divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.sync",errno);ok=0;}
	return ok;
}

static int divoom_product_test_format_marker_write(char state)
{
    return divoom_product_test_format_marker_write_checked(state, 0);
}

int divoom_product_test_recovery_mark(char state)
{
    return divoom_product_test_format_marker_write(state);
}

/* Existing one-byte recovery markers retain their automatic policy. Settings
 * requests are atomically published as CS; all stage writes preserve byte 2. */
int divoom_product_test_format_is_settings_check(void)
{
    if (divoom_recovery_journal_get()->pending_mask &&
        (divoom_recovery_journal_get()->flags & 4U)) return 1;
    char marker[2] = {0};
    divoom_port_stat_t st = {0};
    errno = 0;
    int stat_rc = divoom_port_stat(DIVOOM_SYSTEM_FORMAT_PENDING, &st);
    int saved_errno = errno;
    printf("[RECOVERY_MARKER] path=%s stage=policy.stat rc=%d size=%lld errno=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, stat_rc, stat_rc ? -1LL : (long long)st.size, saved_errno);
    errno = saved_errno;
    if (stat_rc != 0) return saved_errno == ENOENT ? 0 : -1;
    if (st.size != 1 && st.size != 2) {
        printf("[RECOVERY_MARKER] path=%s stage=policy.invalid_size size=%lld expected=1_or_2\n",
            DIVOOM_SYSTEM_FORMAT_PENDING, (long long)st.size);
        errno = saved_errno;
        return -1;
    }
    errno = 0;
    void *fp = divoom_port_file_open(DIVOOM_SYSTEM_FORMAT_PENDING, "rb");
    saved_errno = errno;
    printf("[RECOVERY_MARKER] path=%s stage=policy.open rc=%d errno=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, fp ? 0 : -1, saved_errno);
    errno = saved_errno;
    if (!fp) return -1;
    errno = 0;
    size_t count = divoom_port_file_read(marker, 1, sizeof(marker), fp);
    saved_errno = errno;
    printf("[RECOVERY_MARKER] path=%s stage=policy.read count=%u size=%lld byte0=%02x byte1=%02x ferror=%d feof=%d errno=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, (unsigned)count, (long long)st.size,
        (unsigned char)marker[0], (unsigned char)marker[1],
        ferror((FILE *)fp), feof((FILE *)fp), saved_errno);
    errno = saved_errno;
    int ok = count == st.size;
    errno = 0;
    int close_rc = divoom_port_file_close(fp);
    saved_errno = errno;
    printf("[RECOVERY_MARKER] path=%s stage=policy.close rc=%d errno=%d\n",
        DIVOOM_SYSTEM_FORMAT_PENDING, close_rc, saved_errno);
    errno = saved_errno;
    if (close_rc != 0) ok = 0;
    if (!ok || !marker[0] || !strchr("CPFMRV", marker[0]) || (count == 2 && marker[1] != 'S')) {
        printf("[RECOVERY_MARKER] path=%s stage=policy.invalid count=%u byte0=%02x byte1=%02x errno=%d\n",
            DIVOOM_SYSTEM_FORMAT_PENDING, (unsigned)count,
            (unsigned char)marker[0], (unsigned char)marker[1], saved_errno);
        errno = saved_errno;
        return -1;
    }
    return count == 2;
}

int divoom_product_test_format_arm_settings_check(void)
{
    divoom_recovery_record *record = divoom_system_malloc(sizeof(*record));
    if (!record) return 0;
    memcpy(record, divoom_recovery_journal_get(), sizeof(*record));
    if (!record->pending_mask) memset(record, 0, sizeof(*record));
    record->pending_mask |= 3U; record->phase = DIVOOM_RECOVERY_SCAN;
    record->flags |= 4U; /* Settings requested a maintenance check. */
    int ok = divoom_recovery_journal_save(record);
    divoom_system_free(record); return ok;
}

int divoom_product_test_recovery_state(void)
{
    return divoom_product_test_format_marker_state();
}

static int divoom_product_test_format_marker_clear(void)
{
    errno = 0;
    if (divoom_port_remove_file(DIVOOM_SYSTEM_FORMAT_PENDING) != 0 && errno != ENOENT) {divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.remove",errno);return 0;}
    errno = 0;
    if (divoom_port_remove_file(DIVOOM_SYSTEM_FORMAT_PENDING ".tmp") != 0 && errno != ENOENT) {divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.remove_tmp",errno);return 0;}
    int ok=divoom_port_sync()==0;
    if (!ok) divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.clear_sync",errno);
    return ok;
}


#ifdef DIVOOM_SIMULATOR
static int divoom_product_test_format_fault(const char *name)
{
	const char *fault = getenv("DIVOOM_SIM_FORMAT_FAULT");
	return fault && strcmp(fault, name) == 0;
}
#else
#define divoom_product_test_format_fault(name) 0
#endif

void divoom_product_test_format_init(void)
{
	/* UI 创建时初始化一次，工作线程与 UI 快照共用此锁。 */
    if (!s_format.lock) {
        s_format.lock = divoom_system_create_lock();
        if (s_format.lock) s_format.status.error_files = -1;
    }
    if (!s_format.wifi_submit) s_format.wifi_submit = divoom_system_create_sem(1, 0);
}

void divoom_product_test_format_get_status(divoom_product_test_format_status *status)
{
	if (!s_format.lock) {
		memset(status, 0, sizeof(*status));
		status->phase = DIVOOM_PT_FORMAT_FAILED;
        status->error_files = -1;
		status->error_key = "Not enough memory";
        status->failure_line = __LINE__;
        status->failure_errno = ENOMEM;
        status->failure_raw_errno = ENOMEM;
        status->stopped = s_format.status.stopped;
        status->failure_stage = "worker_start";
		return;
	}
	divoom_system_get_lock_direct(s_format.lock);
	*status = s_format.status;
	divoom_system_free_lock_direct(s_format.lock);
}

void divoom_product_test_format_fs_progress(int target_usr, unsigned stage, unsigned done, unsigned total)
{
    if (!s_format.lock) return;
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.status.stopped) { divoom_system_free_lock_direct(s_format.lock); return; }
    if (target_usr != s_format.status.fs_check_target || stage != s_format.status.fs_check_stage ||
        done != (unsigned)s_format.status.done || total != (unsigned)s_format.status.total)
        s_format.last_progress_tick = divoom_system_get_tickms();
    s_format.status.fs_check_target = target_usr;
    s_format.status.fs_check_stage = stage;
    s_format.status.phase = DIVOOM_PT_FORMAT_CHECK;
    s_format.status.error_key = NULL;
    s_format.status.done = (int)done; s_format.status.total = (int)total;
    s_format.status.file_percent = -1;
    divoom_system_free_lock_direct(s_format.lock);
}

void divoom_product_test_format_fs_result(int target_usr, int clean)
{
    if (!s_format.lock) return;
    unsigned bit = 1U << !!target_usr;
    divoom_system_get_lock_direct(s_format.lock);
    s_format.status.fs_clean_mask &= ~bit;
    if (clean) s_format.status.fs_clean_mask |= bit;
    divoom_system_free_lock_direct(s_format.lock);
}

static void divoom_product_test_format_phase_set_at(divoom_product_test_format_phase phase,
    const char *error, int line, int error_number, int has_code, int code)
{
	divoom_system_get_lock_direct(s_format.lock);
    if (s_format.status.stopped) { divoom_system_free_lock_direct(s_format.lock); return; }
    if (phase != s_format.status.phase) s_format.last_progress_tick = divoom_system_get_tickms();
    if (phase == DIVOOM_PT_FORMAT_FAILED) {
        if (s_format.status.phase == DIVOOM_PT_FORMAT_FAILED &&
            s_format.status.failure_has_code && !has_code) {
            /* Saving/replaying a failed-format intent must not hide the
             * original format error behind a secondary generic failure. */
            error = s_format.status.error_key;
            line = s_format.status.failure_line;
            error_number = s_format.status.failure_errno;
            has_code = 1; code = s_format.status.failure_code;
        } else {
            divoom_product_test_format_failure_locked(line, error_number, has_code, code);
        }
    }
	s_format.status.phase = phase;
	s_format.status.error_key = error;
	if (phase == DIVOOM_PT_FORMAT_DONE || phase == DIVOOM_PT_FORMAT_CANCELLED)
		s_format.status.error_times = 0;
	divoom_system_free_lock_direct(s_format.lock);
	divoom_print("product_format: phase=%d error=%s\n", phase, error ? error : "none");
    if (phase == DIVOOM_PT_FORMAT_FAILED)
        divoom_print("product_format: failure site=F%d errno=%d has_code=%d code=%d\n",
            line, error_number, has_code, code);
}

#define divoom_product_test_format_phase_set(phase, error) \
    divoom_product_test_format_phase_set_at(phase, error, __LINE__, errno, 0, 0)
#define divoom_product_test_format_phase_code(phase, error, code, error_number) \
    divoom_product_test_format_phase_set_at(phase, error, __LINE__, error_number, 1, code)

static void divoom_product_test_format_integrity_failed_at(int result, int fallback_error, int line)
{
    divoom_recovery_integrity_failure failure;
    divoom_recovery_integrity_get_failure(&failure);
    int known = failure.kind != DIVOOM_RECOVERY_FAILURE_NONE;
    int error_number = known ? failure.error : fallback_error ? fallback_error : EIO;
    divoom_product_test_format_phase_set_at(DIVOOM_PT_FORMAT_FAILED,
        result == -2 ? "Storage failure after recovery. Checking automatically" :
            "Filesystem check waiting. Retrying automatically",
        line, error_number, 1, known ? failure.result : result);
    divoom_system_get_lock_direct(s_format.lock);
    if (!s_format.status.stopped) {
        s_format.status.failure_target_usr = known ? failure.target_usr : -1;
        s_format.status.failure_stage = known ? failure.stage : "integrity";
        s_format.status.failure_raw_errno = known ? failure.raw_error : fallback_error;
    }
    divoom_system_free_lock_direct(s_format.lock);
    errno = error_number;
}
#define divoom_product_test_format_integrity_failed(result, error_number) \
    divoom_product_test_format_integrity_failed_at(result, error_number, __LINE__)

void divoom_product_test_format_set_phase(divoom_product_test_format_phase phase)
{
    divoom_product_test_format_phase_set(phase, NULL);
}

void divoom_product_test_format_set_error_files(int count)
{
    if (!s_format.lock) return;
    divoom_system_get_lock_direct(s_format.lock);
    s_format.status.error_files = count < 0 ? -1 : count;
    divoom_system_free_lock_direct(s_format.lock);
}

static int divoom_product_test_format_is_cancelled(void)
{
	int cancel;
	divoom_system_get_lock_direct(s_format.lock);
	cancel = s_format.cancel;
	divoom_system_free_lock_direct(s_format.lock);
	return cancel;
}

static int divoom_product_test_format_finish_cancel(void)
{
    divoom_product_test_format_status status;
    divoom_product_test_format_get_status(&status);
    if (status.stopped) return 0;
    if (status.repair_declined) {
        /* Declining an escalation must not erase an already committed
         * deletion or format transaction, nor release its write barriers. */
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_CANCELLED, NULL);
        return 1;
    }
    if (!status.can_cancel || status.volume_started || status.formatted) return 0;
    if (status.target_usr && !divoom_product_test_format_marker_clear()) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot clear recovery marker");
        return 0;
    }
    if (!divoom_recovery_journal_clear()) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot clear recovery marker"); return 0;
    }
    divoom_port_fs_write_block("/dev/UDISK", 0);
    divoom_port_fs_write_block("/dev/usr", 0);
    divoom_product_test_leave_format_recovery();
    divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_CANCELLED, NULL);
    return 1;
}

void divoom_product_test_format_cancel(void)
{
    int busy;
    if (!s_format.lock) return;
    divoom_system_get_lock_direct(s_format.lock);
    if (!s_format.status.can_cancel || s_format.status.volume_started || s_format.status.formatted) {
        divoom_system_free_lock_direct(s_format.lock); return;
    }
    s_format.cancel = 1;
    busy = s_format.worker_alive || s_format.status.busy;
    divoom_system_free_lock_direct(s_format.lock);
    if (busy) divoom_system_put_sem(s_format.wifi_submit); /* Wake manual provisioning. */
    else if (divoom_product_test_format_finish_cancel()) {
        divoom_product_test_end_format();
        divoom_product_test_wifi_fw_recovery_resume();
    }
}

static void divoom_product_test_format_progress(int done, int total, int file_percent)
{
	divoom_system_get_lock_direct(s_format.lock);
	if (s_format.status.stopped) { divoom_system_free_lock_direct(s_format.lock); return; }
	if (done != s_format.status.done || total != s_format.status.total || file_percent != s_format.status.file_percent)
		s_format.last_progress_tick = divoom_system_get_tickms();
	if (done != s_format.status.done || file_percent < 0)
		s_format.status.error_times = 0;
	s_format.status.done = done;
	s_format.status.total = total;
	s_format.status.file_percent = file_percent;
	divoom_system_free_lock_direct(s_format.lock);
}

/* Publish exactly the saved AP being joined before starting its connection. */
static void divoom_product_test_format_connect_saved(
	const divoom_wifi_menu_wifi_info *network)
{
	char ssid[33] = {0}, password[65] = {0};
	snprintf(ssid, sizeof(ssid), "%.*s", (int)sizeof(network->ssid), (const char *)network->ssid);
	snprintf(password, sizeof(password), "%.*s", (int)sizeof(network->pswd), (const char *)network->pswd);
	divoom_system_get_lock_direct(s_format.lock);
	memcpy(s_format.status.wifi_ssid, ssid, sizeof(ssid));
	memcpy(s_format.status.wifi_password, password, sizeof(password));
	divoom_system_free_lock_direct(s_format.lock);
	divoom_wifi_connect_ssid(ssid, password);
}

void divoom_product_test_format_get_wifi_scan(divoom_product_test_wifi_scan *scan)
{
    memset(scan, 0, sizeof(*scan));
    if (!s_format.lock) return;
    divoom_system_get_lock_direct(s_format.lock);
    *scan = s_format.wifi_scan;
    divoom_system_free_lock_direct(s_format.lock);
}

int divoom_product_test_format_rescan_wifi(void)
{
    if (!s_format.lock || !s_format.wifi_submit) return 0;
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.status.phase != DIVOOM_PT_FORMAT_INPUT_WIFI || s_format.wifi_submitted ||
        s_format.wifi_scan.state == DIVOOM_PT_WIFI_SCANNING) {
        divoom_system_free_lock_direct(s_format.lock); return 0;
    }
    s_format.wifi_submitted = 2;
    s_format.wifi_scan.state = DIVOOM_PT_WIFI_SCANNING;
    divoom_system_free_lock_direct(s_format.lock);
    divoom_system_put_sem(s_format.wifi_submit);
    return 1;
}

/* Runs only on the existing recovery worker. Never hold the UI lock across
 * driver calls: stopping/joining Wi-Fi may synchronously deliver callbacks. */
static void divoom_product_test_format_scan_wifi(int connect_failed)
{
    divoom_product_test_wifi_scan next = {0};
    int round;
    uint32 started;
    next.connect_failed = connect_failed;
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.status.stopped || s_format.cancel) { divoom_system_free_lock_direct(s_format.lock); return; }
    s_format.last_progress_tick = divoom_system_get_tickms();
    next.generation = s_format.wifi_scan.generation + 1;
    s_format.wifi_scan = next;
    s_format.wifi_submitted = 0;
    s_format.status.phase = DIVOOM_PT_FORMAT_INPUT_WIFI;
    divoom_system_free_lock_direct(s_format.lock);
    next.state = DIVOOM_PT_WIFI_SCAN_FAILED;
    /* One initial attempt followed by at most five retries. */
    for (round = 0; round <= 5 && !divoom_product_test_format_is_cancelled(); ++round) {
        uint16_t count = 0, actual;
        aw_wifi_ap_record_t *records;
        int i, j, rc, pending;
        /* Retry the whole preparation, not only the scan command. A failed
         * STOP_CONNECT leaves pending=2 and needs a new stop request, exactly
         * what the user previously had to trigger through RESCAN. */
        if (round) divoom_system_delay_ms(3000);
        if (divoom_product_test_format_is_cancelled()) return;
        divoom_wifi_stop_connect_wifi();
        started = divoom_system_get_tickms();
        while (divoom_wifi_is_stop_connect_pending() == 1) {
            if (divoom_product_test_format_is_cancelled()) return;
            if ((uint32)(divoom_system_get_tickms() - started) >= 15000U) break;
            divoom_system_delay_ms(100);
        }
        pending = divoom_wifi_is_stop_connect_pending();
        if (pending) {
            divoom_print("product_wifi_scan: attempt=%d stage=stop pending=%d\n", round+1, pending);
            if (pending == 1) break; /* Do not race an unacknowledged stop. */
            continue;
        }
        rc = divoom_wifi_ensure_sta_started();
        if (rc) {
            divoom_print("product_wifi_scan: attempt=%d stage=start_sta rc=%d\n", round+1, rc);
            continue;
        }
        rc = aw_wifi_scan_start(NULL, true);
        if (!rc) rc = aw_wifi_scan_get_ap_num(&count);
        divoom_print("product_wifi_scan: attempt=%d stage=scan rc=%d aps=%u\n", round+1, rc, count);
        if (rc) continue;
        next.state = DIVOOM_PT_WIFI_EMPTY;
        if (!count) continue;
        if (count > 128) count = 128;
        records = divoom_system_malloc((size_t)count * sizeof(*records));
        if (!records) { next.state = DIVOOM_PT_WIFI_SCAN_FAILED; break; }
        actual = count;
        if (aw_wifi_scan_get_ap_records(&actual, records) != 0) {
            divoom_system_free(records); next.state = DIVOOM_PT_WIFI_SCAN_FAILED; continue;
        }
        if (actual > count) actual = count;
        for (i = 0; i < actual; ++i) {
            divoom_product_test_wifi_network ap = {0};
            memcpy(ap.ssid, records[i].ssid, 32);
            if (!ap.ssid[0]) continue;
            ap.rssi = records[i].rssi;
            ap.open = records[i].authmode == AW_WIFI_AUTH_OPEN;
            for (j = 0; j < next.count; ++j)
                if (!strcmp(next.networks[j].ssid, ap.ssid)) break;
            if (j < next.count && next.networks[j].rssi >= ap.rssi) continue;
            if (j == next.count) {
                if (next.count < DIVOOM_PT_WIFI_MAX_NETWORKS) ++next.count;
                else {
                    for (j = 1; j < next.count; ++j)
                        if (next.networks[j].rssi < next.networks[0].rssi) {
                            divoom_product_test_wifi_network tmp = next.networks[0];
                            next.networks[0] = next.networks[j]; next.networks[j] = tmp;
                        }
                    j = 0;
                    if (next.networks[j].rssi >= ap.rssi) continue;
                }
            }
            next.networks[j] = ap;
        }
        divoom_system_free(records);
        if (next.count) { next.state = DIVOOM_PT_WIFI_READY; break; }
    }
    for (round = 1; round < next.count; ++round) {
        divoom_product_test_wifi_network ap = next.networks[round];
        int j = round;
        while (j > 0 && next.networks[j-1].rssi < ap.rssi) {
            next.networks[j] = next.networks[j-1]; --j;
        }
        next.networks[j] = ap;
    }
    for (round = 0; round < next.count; ++round) {
        char hex[65];
        unsigned i, len = strlen(next.networks[round].ssid);
        for (i = 0; i < len; ++i) snprintf(hex + i*2, 3, "%02x", (unsigned char)next.networks[round].ssid[i]);
        hex[len*2] = 0;
        divoom_print("product_wifi_scan: ssid[%d] len=%u hex=%s rssi=%d\n",
            round, len, hex, next.networks[round].rssi);
    }
    divoom_system_get_lock_direct(s_format.lock);
    if (!s_format.status.stopped && !s_format.cancel) s_format.wifi_scan = next;
    divoom_system_free_lock_direct(s_format.lock);
}

int divoom_product_test_format_submit_wifi(const char *ssid, const char *password)
{
    if (!s_format.lock || !s_format.wifi_submit || !ssid || !password ||
        !ssid[0] || strlen(ssid) > 32 || strlen(password) > 64) return 0;
    divoom_system_get_lock_direct(s_format.lock);
    int i;
    for (i = 0; i < s_format.wifi_scan.count; ++i)
        if (!strcmp(ssid, s_format.wifi_scan.networks[i].ssid)) break;
    if (s_format.status.phase != DIVOOM_PT_FORMAT_INPUT_WIFI || s_format.wifi_submitted ||
        s_format.wifi_scan.state != DIVOOM_PT_WIFI_READY || i == s_format.wifi_scan.count) {
        divoom_system_free_lock_direct(s_format.lock); return 0;
    }
    snprintf(s_format.status.wifi_ssid, sizeof(s_format.status.wifi_ssid), "%s", ssid);
    snprintf(s_format.status.wifi_password, sizeof(s_format.status.wifi_password), "%s", password);
    s_format.wifi_submitted = 1;
    divoom_system_free_lock_direct(s_format.lock);
    divoom_system_put_sem(s_format.wifi_submit);
    return 1;
}

static int divoom_product_test_format_join(const divoom_wifi_menu_wifi_info *network)
{
    uint32 attempt;
    char ssid[128], expected[33];
    snprintf(expected, sizeof(expected), "%.*s", (int)sizeof(network->ssid), (const char *)network->ssid);
    divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_WAIT_NETWORK, NULL);
    divoom_product_test_format_connect_saved(network);
    attempt = divoom_system_get_tickms();
    do {
        if (divoom_product_test_format_is_cancelled()) return 0;
        ssid[0] = 0;
        if (divoom_wifi_model_is_network_ready() && !divoom_wifi_model_network_services_paused() &&
            !divoom_product_test_format_fault("wifi-timeout")) {
            divoom_wifi_get_connect_ssid_info(ssid);
            if (!strcmp(ssid, expected)) return 1;
        }
        divoom_system_delay_ms(100);
    } while ((uint32)(divoom_system_get_tickms()-attempt) < 15000U);
    divoom_wifi_stop_connect_wifi();
    return 0;
}

static int divoom_product_test_format_wait_network(void)
{
    divoom_product_test_format_status previous;
    Pdivoom_wifi_menu_wifi_info networks;
    char connected_ssid[128];
    int count = 0, i, connected = 0, connect_failed = 0;
    divoom_product_test_format_get_status(&previous);
    if (!previous.target_usr) return 1;
#ifndef DIVOOM_SIMULATOR
    if (!divoom_product_test_wifi_fw_prepare_network() || !divoom_product_test_ensure_wifi()) return 0;
#endif
    connected_ssid[0] = 0;
    if (previous.wifi_ssid[0] && divoom_wifi_model_is_network_ready() &&
        !divoom_wifi_model_network_services_paused()) {
        divoom_wifi_get_connect_ssid_info(connected_ssid);
        if (!strcmp(connected_ssid, previous.wifi_ssid)) {
            divoom_http_client_request_abort_clear(); return 1;
        }
    }
    divoom_wifi_allow_saved_reconnect();
    networks = divoom_wifi_menu_get_all_wifi_info(&count);
    if (!divoom_product_test_format_fault("wifi-none"))
        for (i = count-1; networks && i >= 0; --i) {
            if (divoom_product_test_format_join(&networks[i])) { connected = 1; break; }
            if (divoom_product_test_format_is_cancelled()) break;
        }
    if (networks) divoom_system_free(networks);
    while (!connected && !divoom_product_test_format_is_cancelled()) {
        divoom_wifi_menu_wifi_info manual = {0};
        int action;
        divoom_product_test_format_scan_wifi(connect_failed);
        if (divoom_product_test_format_is_cancelled()) return 0;
        if (!s_format.wifi_submit) return 0;
        uint32 input_started = divoom_system_get_tickms();
        while (divoom_system_get_sem_with_time(s_format.wifi_submit, 1) != 1) {
            if (divoom_product_test_format_is_cancelled()) return 0;
            if ((uint32)(divoom_system_get_tickms() - input_started) >= DIVOOM_PT_INPUT_TIMEOUT_MS) {
                divoom_product_test_format_stop("Operation timed out", "wifi_input", ETIMEDOUT);
                return 0;
            }
        }
        divoom_system_get_lock_direct(s_format.lock);
        action = s_format.wifi_submitted;
        /* Saved-network fields are fixed 32/64 byte arrays, without room for
         * a terminator at the maximum length. connect_saved adds it safely. */
        memcpy(manual.ssid, s_format.status.wifi_ssid, strlen(s_format.status.wifi_ssid));
        memcpy(manual.pswd, s_format.status.wifi_password, strlen(s_format.status.wifi_password));
        divoom_system_free_lock_direct(s_format.lock);
        if (divoom_product_test_format_is_cancelled()) return 0;
        if (action != 1) continue;
        connected = divoom_product_test_format_join(&manual);
        connect_failed = !connected;
    }
    if (!connected) return 0;
    divoom_http_client_request_abort_clear();
    divoom_product_test_format_phase_set(previous.phase, NULL);
    return 1;
}

static int divoom_product_test_format_retry(int errors)
{
	int cancel;
	int error = errno;
	divoom_system_get_lock_direct(s_format.lock);
	if (s_format.status.stopped) { divoom_system_free_lock_direct(s_format.lock); return 0; }
	s_format.status.error_times = errors > 0 ? errors : 0;
	if (errors > 0)
		divoom_print("product_format: Error Times:%d\n", errors);
	cancel = s_format.cancel;
	divoom_system_free_lock_direct(s_format.lock);
	if (!cancel && errors >= (int)DIVOOM_PT_AUTO_ATTEMPTS) {
		divoom_product_test_format_stop(NULL, "resource_guide", error ? error : ETIMEDOUT);
		return 0;
	}
	return !cancel && (errors == 0 || divoom_product_test_format_wait_network());
}

#define DIVOOM_PT_DOWNLOAD_RETRY_DELAY_MS 10000

/* Download retries stay in this stage. Do not run saved-AP selection or the
 * recovery Wi-Fi keyboard again for a failed resource request. */
static int divoom_product_test_format_download_retry(int errors)
{
    int cancel;
    int error = errno;
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.status.stopped) { divoom_system_free_lock_direct(s_format.lock); return 0; }
    s_format.status.error_times = errors > 0 ? errors : 0;
    s_format.status.error_key = errors > 0 ? "Download failed. Retrying in 10s..." : NULL;
    cancel = s_format.cancel;
    divoom_system_free_lock_direct(s_format.lock);
    if (!cancel && errors >= (int)DIVOOM_PT_AUTO_ATTEMPTS) {
        divoom_product_test_format_stop(NULL, "resource_download", error ? error : ETIMEDOUT);
        return 0;
    }
    return !cancel;
}

static int divoom_product_test_format_restore_with_retry(void)
{
    int errors = 0, local_errors = 0, userdata_errors = 0;
    for (;;) {
        if (divoom_product_test_format_is_cancelled()) return 0;
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_RESTORE, NULL);
        int restored = divoom_update_system_file_restore_format(divoom_product_test_format_progress,
            divoom_product_test_format_download_retry);
        if (restored == 1 || restored == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT ||
            restored == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT) return restored;
        /* A generic failure can be memory pressure or an invalid guide. Only
         * explicitly classified local I/O may authorize destructive recovery. */
        if (restored == DIVOOM_SYSTEM_FILE_RESTORE_LOCAL_RETRY && ++local_errors >= (int)DIVOOM_PT_AUTO_ATTEMPTS)
            return DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT;
        if (restored == DIVOOM_SYSTEM_FILE_RESTORE_USERDATA_RETRY && ++userdata_errors >= (int)DIVOOM_PT_AUTO_ATTEMPTS)
            return DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT;
        if (!divoom_product_test_format_download_retry(++errors)) return 0;
        divoom_print("product_format: restore retry=%d local_errors=%d/3 result=%d delay=10s\n",
            errors, local_errors, restored);
        divoom_system_delay_ms(DIVOOM_PT_DOWNLOAD_RETRY_DELAY_MS);
    }
}

#define DIVOOM_PT_REPAIR_RETRIES 3
#define DIVOOM_PT_REPAIR_RETRY_DELAY_MS 2000

static int divoom_product_test_format_repair_attempt_retry(int errors)
{
    divoom_product_test_format_status status;
    divoom_product_test_format_get_status(&status);
    /* Resource retries stay in the current download, with a finite budget. */
    if (status.phase == DIVOOM_PT_FORMAT_RESTORE)
        return divoom_product_test_format_download_retry(errors);
    return errors == 0 && divoom_product_test_format_retry(0);
}

static int divoom_product_test_format_repair_with_retry(void)
{
    int result, retries = 0, transient = 0;
    for (;;) {
        if (divoom_product_test_format_is_cancelled()) return 0;
        result = divoom_product_test_repair_run(divoom_product_test_format_wait_network,
            divoom_product_test_format_repair_attempt_retry, divoom_product_test_format_progress);
        if (divoom_product_test_format_is_cancelled()) return 0;
        if (result == DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT) {
            if (++transient >= (int)DIVOOM_PT_AUTO_ATTEMPTS) {
                divoom_product_test_format_stop("Not enough memory", "repair_resources", errno ? errno : ENOMEM);
                return 0;
            }
            divoom_system_delay_ms(DIVOOM_PT_DOWNLOAD_RETRY_DELAY_MS);
            continue;
        }
        /* Server-absent/404 targets were deleted and intentionally skipped.
         * They must not turn a completed repair into a request to erase /usr. */
        if (result == DIVOOM_PRODUCT_TEST_REPAIR_INCOMPLETE) return 1;
        if (result != 0) return result;
        if (++retries >= DIVOOM_PT_REPAIR_RETRIES) {
            divoom_print("product_format: check/repair exhausted after %d attempts\n", retries);
            return 0;
        }
        divoom_system_get_lock_direct(s_format.lock);
        s_format.status.repair_retries = retries;
        s_format.status.error_times = 0;
        divoom_system_free_lock_direct(s_format.lock);
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_RETRY,
            "Check/repair failed. Automatic retry...");
        /* Same worker, no UI lock held while waiting or invoking callbacks. */
        divoom_system_delay_ms(DIVOOM_PT_REPAIR_RETRY_DELAY_MS);
    }
}

static int divoom_product_test_format_reset_recovery(void)
{
    /* Discard only scan progress and disposable download staging. Preserve
     * original CRC/version backups too: /usr may already have been erased. */
    const char *paths[] = { "/userdata/.usr_repair_files.bin", "/userdata/.usr_repair_files.bin.tmp",
        "/userdata/.usr_check_crc.bin", DIVOOM_SYSTEM_FORMAT_MANIFEST,
        "/userdata/.usr_base_crc.bin", "/userdata/.usr_base_crc.bin.tmp" };
    divoom_port_stat_t st;
    errno = 0;
    if (divoom_port_stat("/userdata/.format_restore", &st) == 0) {
        if (divoom_port_remove_tree("/userdata/.format_restore") != 0) {divoom_product_test_repair_record_io("/userdata/.format_restore","reset.remove_tree",errno);return 0;}
    } else if (errno != ENOENT) {divoom_product_test_repair_record_io("/userdata/.format_restore","reset.stat",errno);return 0;}
    for (unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        errno = 0;
        if (divoom_port_remove_file(paths[i]) != 0 && errno != ENOENT) {divoom_product_test_repair_record_io(paths[i],"reset.remove",errno);return 0;}
    }
    if (!divoom_boot_health_clean()) return 0;
    /* Persist the conservative CS intent before removing the invalid marker.
     * needs_recovery() recognizes this temp file at every power-cut boundary. */
    if (!divoom_product_test_format_prepare_marker('C', 1)) return 0;
    errno = 0;
    if (divoom_port_remove_file(DIVOOM_SYSTEM_FORMAT_PENDING) != 0 && errno != ENOENT) {divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"marker.remove",errno);return 0;}
    if (divoom_port_sync() != 0) {divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"reset.sync",errno);return 0;}
    int ok=divoom_port_rename_file(DIVOOM_SYSTEM_FORMAT_PENDING ".tmp",
        DIVOOM_SYSTEM_FORMAT_PENDING) == 0 && divoom_port_sync() == 0;
    if (!ok) divoom_product_test_repair_record_io(DIVOOM_SYSTEM_FORMAT_PENDING,"reset.rename_sync",errno);
    return ok;
}

int divoom_product_test_format_submit_repair(unsigned request, int approve)
{
    if (!s_format.lock || !s_format.wifi_submit) return 0;
    divoom_system_get_lock_direct(s_format.lock);
    int accepted = s_format.status.phase == DIVOOM_PT_FORMAT_REPAIR_CONFIRM &&
        s_format.status.repair_request == request && !s_format.repair_answer;
    if (accepted) s_format.repair_answer = approve ? 1 : -1;
    divoom_system_free_lock_direct(s_format.lock);
    if (accepted) divoom_system_put_sem(s_format.wifi_submit);
    return accepted;
}

static int divoom_product_test_format_wait_repair(const divoom_product_test_repair_plan *plan)
{
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.status.stopped || s_format.cancel) { divoom_system_free_lock_direct(s_format.lock); return 0; }
    s_format.repair_answer = 0;
    s_format.status.repair_request++;
    s_format.status.repair_plan = *plan;
    s_format.status.phase = DIVOOM_PT_FORMAT_REPAIR_CONFIRM;
    s_format.status.error_key = NULL;
    divoom_system_free_lock_direct(s_format.lock);
    uint32 started = divoom_system_get_tickms();
    for (;;) {
        if (divoom_product_test_format_is_cancelled()) return 0;
        if ((uint32)(divoom_system_get_tickms() - started) >= DIVOOM_PT_INPUT_TIMEOUT_MS) {
            divoom_product_test_format_stop("Operation timed out", "repair_confirmation", ETIMEDOUT);
            return 0; /* Never treat lack of input as permission to erase. */
        }
        if (divoom_system_get_sem_with_time(s_format.wifi_submit, 1) != 1) continue;
        divoom_system_get_lock_direct(s_format.lock);
        if (s_format.status.stopped || s_format.cancel) { divoom_system_free_lock_direct(s_format.lock); return 0; }
        int answer = s_format.repair_answer;
        if (answer < 0) {
            s_format.status.repair_declined = 1;
            s_format.cancel = 1;
        } else if (answer > 0) {
            s_format.status.phase = DIVOOM_PT_FORMAT_CHECK;
        }
        divoom_system_free_lock_direct(s_format.lock);
        if (answer) return answer > 0;
    }
}

int divoom_product_test_format_allow_repair(const divoom_product_test_repair_plan *plan)
{
    const divoom_recovery_record *record = divoom_recovery_journal_get();
    if (!(record->flags & DIVOOM_RECOVERY_MENU_CHECK)) return 1;
    unsigned format_mask = (record->flags & DIVOOM_RECOVERY_FORMAT_APPROVAL(0) ? 1U : 0U) |
        (record->flags & DIVOOM_RECOVERY_FORMAT_APPROVAL(1) ? 2U : 0U);
    const divoom_product_test_repair_plan *approved = &s_format.approved_plan;
    if (!(plan->format_mask & ~format_mask) &&
        plan->structure_files[0] <= approved->structure_files[0] &&
        plan->structure_files[1] <= approved->structure_files[1] &&
        plan->download_files <= approved->download_files &&
        plan->check_system_files <= approved->check_system_files) return 1;
    return divoom_product_test_format_wait_repair(plan);
}

int divoom_product_test_format_check_confirm_restart(void)
{
    if (!s_format.lock || !s_format.wifi_submit) return 0;
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.status.phase != DIVOOM_PT_FORMAT_CHECK_PASSED || s_format.reboot_confirmed) {
        divoom_system_free_lock_direct(s_format.lock); return 0;
    }
    s_format.reboot_confirmed = 1;
    divoom_system_free_lock_direct(s_format.lock);
    divoom_system_put_sem(s_format.wifi_submit);
    return 1;
}

static int divoom_product_test_format_wait_check_restart(void)
{
    int confirmed;
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.status.stopped || s_format.cancel) { divoom_system_free_lock_direct(s_format.lock); return 0; }
    if (!s_format.manual_check || s_format.manual_check == DIVOOM_PT_CHECK_AUTOMATIC ||
        divoom_recovery_journal_pending() != 0) {
        divoom_system_free_lock_direct(s_format.lock); return 1;
    }
    s_format.reboot_confirmed = 0;
    s_format.status.file_percent = -1;
    s_format.status.error_key = NULL;
    s_format.status.phase = DIVOOM_PT_FORMAT_CHECK_PASSED;
    divoom_system_free_lock_direct(s_format.lock);
    /* Keep the maintenance owner and this worker; never block the LVGL task.
     * A stale Wi-Fi wakeup cannot acknowledge this separate result flag. */
    do {
        if (divoom_product_test_format_is_cancelled()) return 0;
        if (divoom_system_get_sem_with_time(s_format.wifi_submit, 1) != 1) continue;
        divoom_system_get_lock_direct(s_format.lock);
        confirmed = s_format.reboot_confirmed;
        divoom_system_free_lock_direct(s_format.lock);
        if (confirmed) return 1;
    } while (1);
}

/* 1 ready to repair, 2 clean, 0 retry, -1 user declined. */
static int divoom_product_test_format_preview(int target_usr)
{
    divoom_product_test_repair_plan plan = {.format_mask = s_format.preview_format_mask};
    if (!divoom_recovery_integrity_preview(&plan)) return 0;
    if (!(plan.format_mask & 2U)) {
        if (plan.structure_files[1]) plan.check_system_files = 1;
        else {
            divoom_system_get_lock_direct(s_format.lock);
            s_format.status.fs_check_stage = 0;
            s_format.status.done = s_format.status.total = 0;
            divoom_system_free_lock_direct(s_format.lock);
            if (!divoom_product_test_repair_preview(&plan, divoom_product_test_format_progress)) return 0;
        }
    }
    if (!(plan.format_mask & 2U) && s_format.reset_recovery)
        plan.check_system_files = 1;
    if (!plan.format_mask && !plan.structure_files[0] && !plan.structure_files[1] &&
        !plan.download_files && !plan.check_system_files) return 2;
    if (!divoom_product_test_format_wait_repair(&plan)) return -1;
    divoom_recovery_record *record = divoom_system_malloc(sizeof(*record));
    if (!record) return 0;
    memset(record, 0, sizeof(*record));
    record->pending_mask = 3;
    record->phase = DIVOOM_RECOVERY_SCAN;
    record->target_usr = target_usr;
    record->flags = DIVOOM_RECOVERY_MENU_CHECK;
    if (plan.format_mask & 1U) record->flags |= DIVOOM_RECOVERY_FORMAT_APPROVAL(0);
    if (plan.format_mask & 2U) record->flags |= DIVOOM_RECOVERY_FORMAT_APPROVAL(1);
    int saved = divoom_recovery_journal_save(record);
    divoom_system_free(record);
    if (!saved) return 0;
    s_format.approved_plan = plan;
    s_format.manual_preview = 0;
    if (s_format.preview_format_mask) {
        int target = (s_format.preview_format_mask & 1U) ? 0 : 1;
        if (divoom_recovery_integrity_request_format(target) < 0) return 0;
    }
    return 1;
}

static void *divoom_product_test_format_worker_once(void *arg)
{
	divoom_product_test_format_status status;
    int userdata_wifi_paused = 0, userdata_health_paused = 0;
    int preview_blocks = 0;
	int mount_recovery;
	int repair_result;
	(void)arg;
	divoom_product_test_format_get_status(&status);
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.cancel) {
        divoom_system_free_lock_direct(s_format.lock);
        goto cancelled;
    }
    s_format.status.can_cancel = 0;
    divoom_system_free_lock_direct(s_format.lock);
    mount_recovery = status.target_usr && divoom_boot_health_mount_failed();
#ifndef DIVOOM_SIMULATOR
	/* 仅真机产测会延后初始化 Wi-Fi；仿真没有设备产测启动状态，无需等待。 */
    uint32 wifi_boot_started = divoom_system_get_tickms();
	while (divoom_product_test_wifi_boot_pending())
	{
		if ((uint32)(divoom_system_get_tickms() - wifi_boot_started) >= 30000U) {
			divoom_product_test_format_stop("Operation timed out", "wifi_boot", ETIMEDOUT);
			goto finished;
		}
		if (divoom_product_test_format_is_cancelled())
		{
			goto cancelled;
		}
		divoom_system_delay_ms(20);
	}
#endif
	if (!divoom_product_test_wifi_fw_recovery_pause()) {
		divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "File operations are busy");
		goto finished;
	}
	if (!divoom_product_test_begin_format()) {
		divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Update is busy. Restart test mode");
		goto finished;
	}
    divoom_update_system_file_stop();
    /* stop 的等待有界，必须再次确认旧 worker 已退出，不能与清卷并行。 */
	if (divoom_update_system_file_is_busy()) {
		divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "File operations are busy");
		goto finished;
	}
    divoom_http_client_request_abort_clear();
    if (!divoom_wifi_userdata_format_begin()) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "File operations are busy"); goto finished;
    }
    userdata_wifi_paused = 1;
    if (!divoom_boot_health_userdata_format_begin()) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "File operations are busy"); goto finished;
    }
    userdata_health_paused = 1;
    divoom_database_manage_close(NULL); divoom_log_close();
    if (s_format.manual_preview) {
        int journal = divoom_recovery_journal_retry_read();
        if (journal < 0) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot read recovery journal. Retrying automatically");
            goto finished;
        }
        /* A failed commit readback can recover a real transaction here. */
        if (journal > 0) s_format.manual_preview = 0;
    }
    if (s_format.manual_preview) {
        preview_blocks = 1;
        int preview = 0;
        if (!divoom_port_fs_write_block("/dev/UDISK", 1) && !divoom_port_fs_write_block("/dev/usr", 1))
            preview = divoom_product_test_format_preview(status.target_usr);
        if (preview < 0) goto cancelled;
        if (preview == 2) {
            if (!divoom_product_test_format_wait_check_restart()) goto cancelled;
            divoom_port_fs_write_block("/dev/UDISK", 0);
            divoom_port_fs_write_block("/dev/usr", 0);
            preview_blocks = 0;
            divoom_boot_health_userdata_format_end(); userdata_health_paused = 0;
            if (!divoom_boot_health_clean()) {
                divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save recovery marker");
                goto finished;
            }
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_DONE, NULL);
            if (divoom_product_test_format_is_cancelled()) goto cancelled;
            divoom_system_reboot();
            goto finished;
        }
        if (!preview) {
            divoom_product_test_format_integrity_failed(-1, errno);
            goto finished;
        }
    }
    int previous_marker = divoom_product_test_recovery_state();
    int manual_usr = s_format.explicit_usr_format ||
        (status.target_usr && previous_marker == 'F' && !divoom_product_test_is_format_recovery());
    if (s_format.unrecorded_format_mask) {
        int usr = !!(s_format.unrecorded_format_mask & 2U);
        if (!(divoom_recovery_journal_get()->format_mask & (1U << usr)) &&
            !divoom_recovery_integrity_after_format(usr)) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save format completion. Retrying automatically");
            goto finished;
        }
        s_format.unrecorded_format_mask = 0;
    }
    if (s_format.failed_format_mask) {
        int usr = !!(s_format.failed_format_mask & 2U);
        if (!divoom_recovery_integrity_format_failed(usr)) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save format failure. Retrying automatically");
            goto finished;
        }
        s_format.failed_format_mask = 0;
    }
    int integrity = divoom_recovery_integrity_run(manual_usr ? 1 : s_format.explicit_userdata_format ? 2 : 0);
    int integrity_errno = errno;
    divoom_system_get_lock_direct(s_format.lock);
    s_format.status.fs_check_stage = 0;
    divoom_system_free_lock_direct(s_format.lock);
    if (divoom_product_test_format_is_cancelled()) goto cancelled;
    if (integrity < 0) {
        if (integrity == -2 && status.phase == DIVOOM_PT_FORMAT_FAILED &&
            status.failure_has_code && status.failure_phase == DIVOOM_PT_FORMAT_VOLUME) {
            /* The checker confirmed the same failed format. Keep its original
             * site/code through automatic retries, rather than replacing it. */
            divoom_system_get_lock_direct(s_format.lock);
            s_format.status.phase = DIVOOM_PT_FORMAT_FAILED;
            s_format.status.error_key = status.error_key;
            divoom_system_free_lock_direct(s_format.lock);
        } else {
            divoom_product_test_format_integrity_failed(integrity, integrity_errno);
        }
        goto finished;
    }
    if (integrity == 2) {
        status.target_usr = 0; status.formatted = 0; mount_recovery = 0;
        divoom_system_get_lock_direct(s_format.lock);
        s_format.status.target_usr = 0; s_format.status.formatted = 0;
        s_format.status.can_cancel = 0; s_format.manual_check = 0;
        s_format.auto_userdata = 1;
        divoom_system_free_lock_direct(s_format.lock);
        goto format_volume;
    }
    if (integrity == 4) {
        status.target_usr = 0; status.formatted = !!(divoom_recovery_journal_get()->format_mask & 1U);
        divoom_system_get_lock_direct(s_format.lock);
        s_format.status.target_usr = 0; s_format.status.formatted = status.formatted;
        s_format.auto_userdata = 1;
        divoom_system_free_lock_direct(s_format.lock);
        divoom_boot_health_userdata_format_end(); userdata_health_paused = 0;
        goto restore_userdata;
    }
    /* A clean/repaired userdata volume needs no mkfs. Proceed to the existing
     * system CRC/download flow, including a resumed transaction after reboot. */
    if (!status.target_usr) {
        status.target_usr = 1; status.formatted = 0;
        divoom_system_get_lock_direct(s_format.lock);
        s_format.status.target_usr = 1; s_format.status.formatted = 0;
        s_format.auto_userdata = 0;
        divoom_system_free_lock_direct(s_format.lock);
    }
    divoom_boot_health_userdata_format_end(); userdata_health_paused = 0;
    if (integrity == 3) {
        status.formatted = 0;
        divoom_system_get_lock_direct(s_format.lock);
        s_format.status.formatted = 0;
        const divoom_recovery_record *record = divoom_recovery_journal_get();
        s_format.status.can_cancel = (record->flags & 2U) && !(record->flags & 0x200U) &&
            !record->reserved && !record->plan_count && !record->format_mask &&
            !s_format.status.volume_started && !divoom_boot_health_mount_failed();
        divoom_system_free_lock_direct(s_format.lock);
    }
    if (s_format.reset_recovery || divoom_product_test_recovery_state() == '?') {
        int reset = 0;
        for (int attempt = 0; attempt < 3 && !reset; ++attempt) {
            errno = 0; reset = divoom_product_test_format_reset_recovery();
            if (!reset && errno != EIO) break;
            if (!reset && attempt < 2) divoom_system_delay_ms(1000);
        }
        if (!reset) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot reset recovery state. Retrying automatically"); goto finished;
        }
        s_format.reset_recovery = 0;
    }
    int marker = divoom_product_test_recovery_state();
    char next_marker = integrity == 3 ? 'F' :
        (divoom_recovery_journal_get()->format_mask & 2U) && marker != 'V' ? 'R' : 0;
    if (next_marker && !divoom_product_test_format_marker_write_checked(next_marker, integrity == 3)) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save recovery marker");
        goto finished;
    }
    if (!divoom_product_test_recovery_state() && !divoom_product_test_recovery_mark('C')) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save recovery marker"); goto finished;
    }
    divoom_wifi_userdata_format_end(); userdata_wifi_paused = 0;
    mount_recovery = divoom_boot_health_mount_failed();
    if (divoom_product_test_recovery_state() == 'V') goto completed;
	if (status.target_usr) {
		int marker = divoom_product_test_format_marker_state();
		if (marker == '?') {
			divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Recovery marker is unreadable");
			goto finished;
		}
        if (marker != 'C')
            divoom_product_test_format_set_error_files(divoom_product_test_repair_error_count());
        if (marker == 'M') {
            divoom_system_get_lock_direct(s_format.lock);
            s_format.status.volume_started = 1;
            s_format.status.can_cancel = 0;
            divoom_system_free_lock_direct(s_format.lock);
        }
		if (marker == 'R') {
			status.formatted = 1;
			divoom_system_get_lock_direct(s_format.lock);
			s_format.status.formatted = 1;
			s_format.status.volume_started = 1;
			divoom_system_free_lock_direct(s_format.lock);
		}
	}
    /* An unmounted /usr cannot host the firmware needed for provisioning.
     * Its durable F intent is already saved by resume_mode. */
    if (mount_recovery && !status.formatted) {
        goto format_volume;
    }
    if (status.target_usr && (divoom_product_test_recovery_state() == 'C' ||
        divoom_product_test_recovery_state() == 'P')) {
        repair_result = divoom_product_test_format_repair_with_retry();
        if (divoom_product_test_format_is_cancelled()) goto cancelled;
        if (repair_result == 1) {
            if (!divoom_product_test_wifi_fw_restore()) {
                divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Wi-Fi firmware restore failed");
                goto finished;
            }
            goto completed;
        }
        if (repair_result == 0) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Repair waiting. Retrying automatically"); goto finished;
        }
        if (repair_result == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT) goto local_userdata_format;
        if (divoom_recovery_integrity_request_format(1) < 0) {
            if (divoom_product_test_format_is_cancelled()) goto cancelled;
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Storage failure after recovery. Checking automatically"); goto finished;
        }
        if (!divoom_product_test_recovery_mark('F')) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save recovery marker");
            goto finished;
        }
    }
    if (status.target_usr) {
        if (!divoom_product_test_format_wait_network()) {
            if (divoom_product_test_format_is_cancelled()) goto cancelled;
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Wi-Fi connection failed. Retry");
            goto finished;
        }
        if (!status.formatted) {
            if (!divoom_product_test_repair_backup_ledger()) {
                divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save original resource ledger");
                goto finished;
            }
            if (!divoom_product_test_format_marker_write('F')) {
				divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save recovery marker");
				goto finished;
			}
			divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_GUIDE, NULL);
			if (!divoom_update_system_file_prepare_format(divoom_product_test_format_retry)) {
				if (divoom_product_test_format_is_cancelled()) goto cancelled;
				divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Resource guide download or validation failed");
				goto finished;
			}
		}
	}
format_volume:
	if (!status.formatted) {
        /* DHCP persistence runs on wifi_con, independently of this worker.
         * Drain it before unmounting; keep network callbacks available. */
        if (!status.target_usr) {
            if (!userdata_wifi_paused) {
                if (!divoom_wifi_userdata_format_begin()) {
                    divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "File operations are busy");
                    goto finished;
                }
                userdata_wifi_paused = 1;
            }
            if (!userdata_health_paused) {
                if (!divoom_boot_health_userdata_format_begin()) {
                    divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "File operations are busy");
                    goto finished;
                }
                userdata_health_paused = 1;
            }
        }
		divoom_system_get_lock_direct(s_format.lock);
		if (s_format.cancel) {
			divoom_system_free_lock_direct(s_format.lock);
			goto cancelled;
		}
		if (status.target_usr && !mount_recovery && !divoom_wifi_model_is_network_ready()) {
			int can_provision = !s_format.status.volume_started;
			divoom_system_free_lock_direct(s_format.lock);
			/* Before the first erase, use the same saved/manual Wi-Fi flow as
			 * boot recovery. Download-stage retries keep their existing policy. */
			if (can_provision && divoom_product_test_format_wait_network())
				goto format_volume;
			if (divoom_product_test_format_is_cancelled()) goto cancelled;
			divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Wi-Fi disconnected before format");
			goto finished;
		}
        divoom_system_free_lock_direct(s_format.lock);
        if (!divoom_recovery_integrity_before_format(status.target_usr)) {
            if (divoom_product_test_format_is_cancelled()) goto cancelled;
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Storage failure after recovery. Checking automatically"); goto finished;
        }
        if (!divoom_recovery_integrity_protect_wifi(status.target_usr)) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Wi-Fi firmware backup failed");
            goto finished;
        }
        divoom_system_get_lock_direct(s_format.lock);
        if (s_format.cancel) {
            divoom_system_free_lock_direct(s_format.lock);
            goto cancelled;
        }
        s_format.status.volume_started = 1;
        s_format.status.can_cancel = 0;
        divoom_system_free_lock_direct(s_format.lock);
        if (status.target_usr && !divoom_product_test_format_marker_write('M')) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save format start state");
            goto finished;
        }
		divoom_print("product_format: format begin target=%s\n", status.target_usr ? "/usr" : "/userdata");
		if (!status.target_usr) {
			divoom_database_manage_close(NULL);
			divoom_log_close();
		}
		divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_VOLUME, NULL);
        int format_result = divoom_port_fs_format_volume(status.target_usr ? "/dev/usr" : "/dev/UDISK",
            status.target_usr ? "/usr" : "/userdata");
        int format_errno = errno;
        if (format_result != 0 && format_result != FF_CHECK_FORMATTED_UNMOUNTED) {
            if (format_result != FF_CHECK_BUSY && format_result != FF_CHECK_NOMEM && format_result != FF_CHECK_UNSUPPORTED) {
                s_format.failed_format_mask |= 1U << status.target_usr;
                if (divoom_recovery_integrity_format_failed(status.target_usr))
                    s_format.failed_format_mask = 0;
            }
            divoom_product_test_format_phase_code(DIVOOM_PT_FORMAT_FAILED,
                format_result == FF_CHECK_BUSY ? "File operations are busy" :
                format_result == FF_CHECK_NOMEM ? "Not enough memory" : "Format failed",
                format_result, format_errno);
            goto finished;
        }
		divoom_system_get_lock_direct(s_format.lock);
		s_format.status.formatted = 1;
		divoom_system_free_lock_direct(s_format.lock);
		status.formatted = 1;
        s_format.unrecorded_format_mask = 1U << status.target_usr;
        if (!divoom_recovery_integrity_after_format(status.target_usr)) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED,
                "Post-format verification or state save failed"); goto finished;
        }
        s_format.unrecorded_format_mask = 0;
        divoom_boot_health_set_mount_failed(status.target_usr, 0);
        if (userdata_health_paused) {
            /* Remount succeeded: normal shutdown and completed may save health. */
            divoom_boot_health_userdata_format_end();
            userdata_health_paused = 0;
        }
	}
restore_userdata:
    if (!status.target_usr && s_format.auto_userdata) {
        if (!divoom_product_test_wifi_fw_safe_to_erase(1)) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Wi-Fi firmware backup failed");
            goto finished;
        }
        if (!divoom_product_test_userdata_wifi_restore()) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot restore user configuration");
            goto finished;
        }
    }
    if (!status.target_usr && divoom_boot_health_mount_failed() &&
        !divoom_product_test_recovery_mark('F')) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save recovery marker");
        goto finished;
    }
	if (status.target_usr) {
		/* No resource may be installed until the formatted state is durable. */
		if (!divoom_product_test_format_marker_write('R')) {
			divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save restore state");
			goto finished;
		}
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_RESTORE, NULL);
		if (!divoom_product_test_wifi_fw_restore() && !divoom_wifi_model_is_network_ready()) {
			divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Wi-Fi firmware restore failed");
			goto finished;
		}
        if (mount_recovery) {
            if (!divoom_product_test_format_wait_network()) {
                if (divoom_product_test_format_is_cancelled()) goto cancelled;
                divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Wi-Fi connection failed. Retry");
                goto finished;
            }
            mount_recovery = 0;
        }
		if (!divoom_update_system_file_ensure_format_manifest(divoom_product_test_format_retry)) {
			divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Resource guide is unavailable. Retry");
			goto finished;
		}
		divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_RESTORE, NULL);
        repair_result = divoom_product_test_format_restore_with_retry();
        if (!repair_result) goto cancelled;
reformat_local:
        if (repair_result == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT) goto local_userdata_format;
        if (repair_result == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT) {
            if (divoom_recovery_integrity_request_format(1) < 0) {
                if (divoom_product_test_format_is_cancelled()) goto cancelled;
                divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Storage failure after recovery. Checking automatically"); goto finished;
            }
            uint32 network_started = divoom_system_get_tickms();
            /* Keep the existing connection policy: wait here without entering
             * provisioning, and do not erase until the network is usable. */
            while (!divoom_wifi_model_is_network_ready()) {
                if ((uint32)(divoom_system_get_tickms() - network_started) >= 60000U) {
                    divoom_product_test_format_stop("Operation timed out", "reformat_network", ETIMEDOUT);
                    goto finished;
                }
                if (divoom_product_test_format_is_cancelled()) goto cancelled;
                divoom_system_delay_ms(DIVOOM_PT_DOWNLOAD_RETRY_DELAY_MS);
            }
            if (!divoom_product_test_format_marker_write_checked('F', 1)) {
                divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot save reformat state");
                goto finished;
            }
            divoom_print("product_format: confirmed local storage failure; reformat system volume\n");
            status.formatted = 0;
            divoom_system_get_lock_direct(s_format.lock);
            s_format.status.formatted = 0;
            divoom_system_free_lock_direct(s_format.lock);
            goto format_volume;
        }
        if (!divoom_product_test_repair_restore_ledger()) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot restore original resource ledger");
            goto finished;
        }
        repair_result = divoom_update_system_file_ensure_original_ledger(divoom_product_test_format_download_retry);
        if (repair_result == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT ||
            repair_result == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT) goto reformat_local;
        if (repair_result != 1) {
            if (divoom_product_test_format_is_cancelled()) goto cancelled;
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot restore original resource ledger");
            goto finished;
        }
        int driver_skipped = divoom_update_system_file_restore_driver_skipped();
        if (!driver_skipped && !divoom_product_test_wifi_fw_restore()) {
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Wi-Fi firmware restore failed");
            goto finished;
        }
        if (!driver_skipped && !divoom_product_test_wifi_fw_cleanup()) {
			divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot clear Wi-Fi firmware backup");
			goto finished;
		}
	}
completed:
    if (divoom_product_test_format_is_cancelled()) goto cancelled;
    if (divoom_port_sync() != 0 || !divoom_boot_health_clean() ||
        (status.target_usr && (!divoom_product_test_recovery_mark('V') ||
        !divoom_product_test_repair_cleanup() ||
        !divoom_product_test_format_marker_clear()))) {
		divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot finalize restored resources");
		goto finished;
	}
    if (!status.target_usr && s_format.auto_userdata && !divoom_product_test_userdata_wifi_finish()) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot finalize Wi-Fi recovery");
        goto finished;
    }
    if (!divoom_recovery_integrity_complete(status.target_usr)) {
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Cannot finalize recovery. Retrying automatically"); goto finished;
    }
    if (!divoom_product_test_format_wait_check_restart()) goto cancelled;
	divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_DONE, NULL);
    if (status.target_usr) {
        int skipped = 0;
        divoom_update_system_file_restore_status(NULL, NULL, &skipped);
        if (skipped) divoom_system_delay_ms(3000); /* Show the degraded result before restart. */
    }
    if (divoom_product_test_format_is_cancelled()) goto cancelled;
    divoom_system_reboot();
#if DIVOOM_BUILD_RTOS_DEVICE
    if (status.target_usr) divoom_product_test_recovery_mark(status.formatted ? 'R' : 'C');
    divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Restart failed. Retry");
#endif
    goto finished;
local_userdata_format:
    if (!userdata_wifi_paused) {
        if (!divoom_wifi_userdata_format_begin()) goto finished;
        userdata_wifi_paused = 1;
    }
    if (divoom_recovery_integrity_request_format(0) < 0) {
        if (divoom_product_test_format_is_cancelled()) goto cancelled;
        divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Storage failure after recovery. Checking automatically"); goto finished;
    }
    status.target_usr = 0; status.formatted = 0; mount_recovery = 0;
    divoom_system_get_lock_direct(s_format.lock);
    s_format.status.target_usr = 0; s_format.status.formatted = 0;
    s_format.status.can_cancel = 0; s_format.manual_check = 0; s_format.auto_userdata = 1;
    divoom_system_free_lock_direct(s_format.lock);
    goto format_volume;
cancelled:
    divoom_product_test_format_get_status(&status);
    if (!status.stopped) divoom_product_test_format_finish_cancel();
finished:
    if (preview_blocks && divoom_recovery_journal_pending() == 0) {
        divoom_port_fs_write_block("/dev/UDISK", 0);
        divoom_port_fs_write_block("/dev/usr", 0);
    }
	divoom_product_test_format_get_status(&status);
    if (userdata_health_paused && !status.volume_started)
        divoom_boot_health_userdata_format_end();
    if (userdata_wifi_paused && !status.volume_started)
        divoom_wifi_userdata_format_end();
    if (!status.volume_started && !divoom_product_test_is_format_recovery() &&
        !divoom_product_test_format_needs_recovery()) {
        divoom_product_test_end_format();
        divoom_product_test_wifi_fw_recovery_resume();
    }
	divoom_system_get_lock_direct(s_format.lock);
	s_format.status.busy = 0;
	divoom_system_free_lock_direct(s_format.lock);
	return NULL;
}

static void *divoom_product_test_format_worker(void *arg)
{
    unsigned local_errors[2] = {0, 0}, other_errors = 0;
    for (;;) {
        divoom_product_test_repair_io_error_target(1);
        divoom_recovery_integrity_clear_failure();
        errno = 0;
        divoom_product_test_format_worker_once(arg);
        divoom_product_test_format_status status;
        divoom_product_test_format_get_status(&status);
        if (status.stopped || status.phase == DIVOOM_PT_FORMAT_DONE || status.phase == DIVOOM_PT_FORMAT_CANCELLED ||
            status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED) break;
        divoom_recovery_integrity_failure failure;
        divoom_recovery_integrity_get_failure(&failure);
        if (failure.kind != DIVOOM_RECOVERY_FAILURE_NONE && status.phase == DIVOOM_PT_FORMAT_FAILED &&
            status.failure_phase != DIVOOM_PT_FORMAT_VOLUME) {
            divoom_system_get_lock_direct(s_format.lock);
            s_format.status.failure_target_usr = failure.target_usr;
            s_format.status.failure_stage = failure.stage;
            s_format.status.failure_errno = failure.error;
            s_format.status.failure_raw_errno = failure.raw_error;
            s_format.status.failure_has_code = 1;
            s_format.status.failure_code = failure.result;
            divoom_system_free_lock_direct(s_format.lock);
            divoom_product_test_format_get_status(&status);
        }
        int io_target = divoom_product_test_repair_io_error_target(0);
        if (failure.kind == DIVOOM_RECOVERY_FAILURE_NONE && io_target >= 0 && io_target <= 1) {
            failure.kind = DIVOOM_RECOVERY_FAILURE_VOLUME;
            failure.target_usr = io_target;
            failure.error = status.failure_errno ? status.failure_errno : EIO;
            failure.stage = "local_io";
        }
        int volume = failure.kind == DIVOOM_RECOVERY_FAILURE_VOLUME &&
            failure.target_usr >= 0 && failure.target_usr <= 1 ? failure.target_usr : -1;
        if (volume >= 0 && ++local_errors[volume] >= DIVOOM_PT_AUTO_ATTEMPTS) {
            if (s_format.manual_preview) {
                /* Build a format proposal; the manual menu must still obtain
                 * confirmation, never auto-approve a failed preview. */
                s_format.preview_format_mask |= 1U << volume;
                local_errors[volume] = 0;
            } else {
            divoom_system_get_lock_direct(s_format.lock);
            s_format.status.can_cancel = 0;
            s_format.cancel = 0;
            divoom_system_free_lock_direct(s_format.lock);
            divoom_print("recovery: exhausted volume=%s attempts=%u stage=%s rc=%d errno=%d raw=%d; request format\n",
                volume ? "/usr" : "/userdata", local_errors[volume], failure.stage ? failure.stage : "unknown",
                failure.result, failure.error, failure.raw_error);
            if (!volume) divoom_wifi_userdata_format_begin();
            int request = divoom_recovery_integrity_request_format(volume);
            if (request < 0 && !divoom_product_test_format_is_cancelled()) {
                divoom_product_test_format_integrity_failed(request, errno);
                divoom_product_test_format_stop(NULL, NULL, errno ? errno : EIO);
                break;
            }
            if (divoom_product_test_format_is_cancelled()) {
                divoom_product_test_format_finish_cancel();
                break;
            }
            local_errors[volume] = 0;
            }
        } else if (volume < 0 && ++other_errors >= DIVOOM_PT_AUTO_ATTEMPTS) {
            divoom_product_test_format_stop(NULL, status.failure_stage,
                status.failure_errno ? status.failure_errno : EIO);
            break;
        }
        divoom_system_get_lock_direct(s_format.lock);
        s_format.status.busy = 1;
        divoom_system_free_lock_direct(s_format.lock);
        divoom_system_delay_ms(10000);
    }
    divoom_system_get_lock_direct(s_format.lock);
    s_format.worker_alive = 0;
    s_format.manual_check = s_format.manual_preview = 0;
    divoom_system_free_lock_direct(s_format.lock);
    return NULL;
}

int divoom_product_test_format_start(int target_usr)
{
	if (s_format.status.stopped) return 0;
	divoom_product_test_format_init();
    if (!s_format.lock || !s_format.wifi_submit) {
        if (s_format.lock) {
            divoom_system_get_lock_direct(s_format.lock);
            s_format.status.target_usr = target_usr;
            divoom_system_free_lock_direct(s_format.lock);
            divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Not enough memory");
        }
        divoom_product_test_format_schedule_retry(target_usr, -1, 0);
        return 0;
    }
    divoom_system_get_lock_direct(s_format.lock);
    int wifi_intent = divoom_product_test_userdata_wifi_pending();
    if (wifi_intent > 0 || (!target_usr && divoom_boot_health_userdata_mount_failed())) target_usr = 0;
	if (s_format.worker_alive || s_format.status.busy || s_format.status.phase == DIVOOM_PT_FORMAT_DONE ||
        s_format.status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED ||
	    (s_format.status.formatted && target_usr != s_format.status.target_usr)) {
		divoom_system_free_lock_direct(s_format.lock);
		return 0;
	}
    s_format.explicit_usr_format = s_format.explicit_userdata_format = 0;
    s_format.preview_format_mask = 0;
    if (!target_usr && (wifi_intent > 0 || divoom_boot_health_userdata_mount_failed())) {
        s_format.auto_userdata = 1;
        unsigned origin = divoom_recovery_journal_get()->flags;
        if (origin & 4U) s_format.manual_check = 2;
        else if (origin & 8U) s_format.manual_check = 1;
    }
    int current_marker = target_usr ? divoom_product_test_recovery_state() : 0;
    if (target_usr && (current_marker == '?' || divoom_product_test_format_is_settings_check() < 0)) {
        s_format.reset_recovery = 1;
        s_format.status.formatted = s_format.status.volume_started = 0;
    }
    unsigned pending = divoom_recovery_journal_get()->pending_mask;
    int extend_pending = 0;
    if (pending == 1U) {
        divoom_port_stat_t st;
        errno = 0;
        extend_pending = divoom_boot_health_mount_failed() ||
            divoom_port_stat(DIVOOM_SYSTEM_FORMAT_PENDING, &st) == 0 || errno != ENOENT;
        if (!extend_pending) {
            errno = 0;
            extend_pending = divoom_port_stat(DIVOOM_SYSTEM_FORMAT_PENDING ".tmp", &st) == 0 || errno != ENOENT;
        }
    }
    if (divoom_recovery_journal_get()->flags & DIVOOM_RECOVERY_MENU_CHECK) s_format.manual_check = 1;
    s_format.manual_preview = s_format.manual_check == DIVOOM_PT_CHECK_MENU &&
        (divoom_recovery_journal_pending() < 0 ||
            (!pending && (!current_marker || current_marker == '?')));
    memset(&s_format.approved_plan, 0, sizeof(s_format.approved_plan));
    s_format.status.repair_declined = 0;
    if ((!pending || extend_pending) && !s_format.manual_preview) {
        divoom_recovery_record *record = divoom_system_malloc(sizeof(*record));
        if (!record) {
            divoom_product_test_format_failure_locked(__LINE__, ENOMEM, 0, 0);
            s_format.status.failure_target_usr = target_usr;
            s_format.status.phase = DIVOOM_PT_FORMAT_FAILED;
            s_format.status.error_key = "Not enough memory";
            divoom_product_test_format_schedule_retry(target_usr, -1, 1);
            divoom_system_free_lock_direct(s_format.lock); return 0;
        }
        if (pending) {
            /* A new system fault must extend, never replace, an in-flight plan. */
            memcpy(record, divoom_recovery_journal_get(), sizeof(*record));
            record->pending_mask |= 2U;
        } else {
            memset(record, 0, sizeof(*record)); record->pending_mask = 3; record->phase = DIVOOM_RECOVERY_SCAN;
            record->target_usr = target_usr;
            if (s_format.manual_check == 2) record->flags |= 4U;
            else if (s_format.manual_check == 1) record->flags |= 8U;
            else if (s_format.manual_check != DIVOOM_PT_CHECK_AUTOMATIC && !divoom_product_test_is_format_recovery()) {
                if (target_usr) { record->flags |= 2U; s_format.explicit_usr_format = 1; }
                else {
                    record->flags |= 16U; s_format.explicit_userdata_format = 1;
                    /* A fresh user-only erase must not invent a system recovery.
                     * Existing journal/legacy intents still keep both volumes. */
                    if (!divoom_product_test_format_needs_recovery()) record->pending_mask = 1U;
                }
            }
        }
        int saved = divoom_recovery_journal_save(record); divoom_system_free(record);
        if (!saved) {
            divoom_product_test_format_failure_locked(__LINE__, errno, 0, 0);
            s_format.status.failure_target_usr = target_usr;
            s_format.status.phase = DIVOOM_PT_FORMAT_FAILED;
            s_format.status.error_key = "Cannot save recovery marker";
            divoom_product_test_format_schedule_retry(target_usr, -1, 1);
            divoom_system_free_lock_direct(s_format.lock); return 0;
        }
    }
    s_format.status.error_times = 0;
    s_format.status.repair_retries = 0;
    s_format.status.fs_clean_mask = 0;
    s_format.status.file_percent = -1;
	s_format.status.wifi_ssid[0] = '\0';
	s_format.status.wifi_password[0] = '\0';
	s_format.cancel = 0;
	s_format.status.target_usr = target_usr;
    /* Offer cancellation only after validating the manual intent and both volumes. */
    s_format.status.can_cancel = 0;
	s_format.status.busy = 1;
    s_format.worker_alive = 1;
    s_format.last_progress_tick = divoom_system_get_tickms();
	s_format.status.phase = s_format.manual_preview ? DIVOOM_PT_FORMAT_CHECK : DIVOOM_PT_FORMAT_WAIT_NETWORK;
	s_format.status.error_key = NULL;
	divoom_system_free_lock_direct(s_format.lock);
	if (!divoom_system_create_task(divoom_product_test_format_worker, "pt_format", NULL, DIVOOM_TASK_STACK_OTA_SERVER_LOCAL)) {
		divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "File operations are busy");
		divoom_system_get_lock_direct(s_format.lock);
		s_format.status.busy = 0; s_format.worker_alive = 0;
        divoom_product_test_format_schedule_retry(target_usr, -1, 1);
		divoom_system_free_lock_direct(s_format.lock);
		return 0;
	}
	divoom_system_get_lock_direct(s_format.lock);
	s_format.launch_failures = 0;
	divoom_system_free_lock_direct(s_format.lock);
	return 1;
}

static int divoom_product_test_format_resume_mode(int manual_check)
{
	int marker;
	if (s_format.status.stopped) return 0;
	divoom_product_test_format_init();
    if (!s_format.lock || !s_format.wifi_submit) {
        if (s_format.lock) divoom_product_test_format_phase_set(DIVOOM_PT_FORMAT_FAILED, "Not enough memory");
        divoom_product_test_format_schedule_retry(1, manual_check, 0);
        return 0;
    }
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.worker_alive || s_format.status.busy || s_format.status.phase == DIVOOM_PT_FORMAT_DONE ||
        s_format.status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED) {
        divoom_system_free_lock_direct(s_format.lock); return 0;
    }
    int wifi_pending = divoom_product_test_userdata_wifi_pending();
    if (divoom_boot_health_userdata_mount_failed() || wifi_pending > 0) {
        unsigned origin = divoom_recovery_journal_get()->flags;
        s_format.manual_check = origin & 4U ? 2 : origin & 8U ? 1 : manual_check;
        s_format.auto_userdata = 1;
        s_format.reset_recovery = 0;
        divoom_system_free_lock_direct(s_format.lock);
        return divoom_product_test_format_start(0);
    }
    divoom_system_free_lock_direct(s_format.lock);
    marker = divoom_product_test_format_marker_state();
    divoom_print("product_format: resume marker=%c usr_mount_failed=%d action=%s\n",
        marker ? marker : '-', divoom_boot_health_mount_failed(),
        divoom_boot_health_mount_failed() ? "format_unmounted" :
        marker == 'R' ? "continue_download" : marker == 'P' ? "continue_repair" :
        marker == 'F' || marker == 'M' ? "format_pending" : "check_files");
    int settings_check = divoom_product_test_format_is_settings_check();
    divoom_port_stat_t temp_state;
    errno = 0;
    int reset_intent = divoom_port_stat(DIVOOM_SYSTEM_FORMAT_PENDING ".tmp", &temp_state) == 0 || errno != ENOENT;
    if (settings_check < 0 || (marker == '?' && (!divoom_boot_health_mount_failed() || reset_intent))) {
        divoom_system_get_lock_direct(s_format.lock);
        s_format.reset_recovery = 1;
        s_format.manual_check = manual_check;
        s_format.status.formatted = s_format.status.volume_started = 0;
        divoom_system_free_lock_direct(s_format.lock);
        return divoom_product_test_format_start(1);
    }
    if (settings_check) manual_check = 2;
	divoom_system_get_lock_direct(s_format.lock);
	s_format.manual_check = manual_check;
	s_format.status.target_usr = 1;
	s_format.status.formatted = marker == 'R';
	s_format.status.volume_started = marker == 'R' || marker == 'M';
	divoom_system_free_lock_direct(s_format.lock);
	return divoom_product_test_format_start(1);
}

void divoom_product_test_format_poll(void)
{
    if (s_format.lock) {
        divoom_system_get_lock_direct(s_format.lock);
        divoom_product_test_format_phase phase = s_format.status.phase;
        if (s_format.worker_alive && s_format.status.busy && !s_format.status.stopped &&
            !(phase == DIVOOM_PT_FORMAT_INPUT_WIFI && s_format.wifi_scan.state != DIVOOM_PT_WIFI_SCANNING) &&
            phase != DIVOOM_PT_FORMAT_REPAIR_CONFIRM &&
            phase != DIVOOM_PT_FORMAT_CHECK_PASSED &&
            (uint32)(divoom_system_get_tickms() - s_format.last_progress_tick) >= DIVOOM_PT_STALL_TIMEOUT_MS) {
            divoom_product_test_format_stop_locked("Operation timed out", "no_progress", ETIMEDOUT, __LINE__);
        }
        int stopped = s_format.status.stopped;
        divoom_system_free_lock_direct(s_format.lock);
        if (stopped) return;
    }
    if (!s_format_retry_requested || s_format.worker_alive ||
        (int32_t)(divoom_system_get_tickms() - s_format_retry_after) < 0) return;
    if (!__sync_bool_compare_and_swap(&s_format_retry_requested, 1, 0)) return;
    if (s_format_retry_mode >= 0) divoom_product_test_format_resume_mode(s_format_retry_mode);
    else divoom_product_test_format_start(s_format_retry_target);
}

int divoom_product_test_format_resume(void)
{
    return divoom_product_test_format_resume_mode(0);
}

int divoom_product_test_format_check_start(void)
{
    return divoom_product_test_format_resume_mode(DIVOOM_PT_CHECK_MENU);
}

int divoom_product_test_format_check_continue(void)
{
    return divoom_product_test_format_resume_mode(DIVOOM_PT_CHECK_AUTOMATIC);
}

#ifdef DIVOOM_SIMULATOR
static int s_format_sim_reboot;

void divoom_product_test_format_sim_reboot(void)
{
	/* 宿主不重启：记录重启请求，测试主循环正常退出并保留恢复结果。 */
	divoom_print("product_format: simulator reboot requested\n");
	if (s_format.lock) {
		divoom_system_get_lock_direct(s_format.lock);
		s_format_sim_reboot = 1;
		divoom_system_free_lock_direct(s_format.lock);
	}
}

int divoom_product_test_format_sim_tick(void)
{
	static int submitted;
	static int retries;
	static int cancel_sent;
	static uint32 start;
	const char *scenario = getenv("DIVOOM_SIM_PRODUCT_FORMAT");
	divoom_product_test_format_status status;
	void *screen;
	int i;
	int reboot_requested;
	if (!scenario) {
		return 0;
	}
	divoom_system_get_lock_direct(s_format.lock);
	reboot_requested = s_format_sim_reboot;
	divoom_system_free_lock_direct(s_format.lock);
	if (!strcmp(scenario, "manual")) {
		divoom_product_test_format_get_status(&status);
		return status.phase == DIVOOM_PT_FORMAT_DONE && !status.busy && reboot_requested ? 1 : 0;
	}
	if (!start) {
		start = divoom_system_get_tickms();
	}
	if (!submitted && (uint32)(divoom_system_get_tickms() - start) >= 5000) {
		screen = divoom_product_test_ui_lvgl_show();
		if (getenv("DIVOOM_SIM_PRODUCT_FORMAT_CAPTURE")) {
			lv_refr_now(NULL);
			sim_ui_visual_capture("product_format_home");
		}
		for (i = 0; i < (!strcmp(scenario, "userdata") ? 0 : 1); i++) {
			divoom_product_test_screen_on_key(screen, DIVOOM_KEY_ASTROTOO_KEY_DOWN, DIVOOM_PRESS_SHORT);
		}
		divoom_product_test_screen_on_key(screen, DIVOOM_KEY_ASTROTOO_KEY_ENTER, DIVOOM_PRESS_SHORT);
		if (getenv("DIVOOM_SIM_PRODUCT_FORMAT_CAPTURE")) {
			lv_refr_now(NULL);
			sim_ui_visual_capture("product_format_confirm");
		}
		if (!strcmp(scenario, "cancel")) {
			divoom_product_test_screen_on_key(screen, DIVOOM_KEY_ASTROTOO_KEY_ENTER, DIVOOM_PRESS_SHORT);
			divoom_product_test_format_get_status(&status);
			printf("FORMAT_TEST: default cancel phase=%d busy=%d\n", status.phase, status.busy);
			return !status.busy && status.phase == DIVOOM_PT_FORMAT_IDLE ? 1 : 2;
		}
		divoom_product_test_screen_on_key(screen, DIVOOM_KEY_ASTROTOO_KEY_RIGHT, DIVOOM_PRESS_SHORT);
		divoom_product_test_screen_on_key(screen, DIVOOM_KEY_ASTROTOO_KEY_ENTER, DIVOOM_PRESS_SHORT);
		submitted = 1;
	}
	divoom_product_test_format_get_status(&status);
	if (submitted && status.busy && !cancel_sent && !strcmp(scenario, "pre-cancel")) {
		divoom_product_test_format_cancel();
		cancel_sent = 1;
	}
	if (submitted && status.busy && !cancel_sent && !strcmp(scenario, "duplicate")) {
		printf("FORMAT_TEST: duplicate accepted=%d\n", divoom_product_test_format_start(1));
		cancel_sent = 1;
	}
	if (submitted && !status.busy && status.phase == DIVOOM_PT_FORMAT_CANCELLED) {
		printf("FORMAT_TEST: PASS pre-format cancel formatted=%d\n", status.formatted);
		return status.formatted ? 2 : 1;
	}
	if (submitted && !status.busy && status.phase == DIVOOM_PT_FORMAT_FAILED) {
		printf("FORMAT_TEST: failed formatted=%d error=%s\n", status.formatted, status.error_key);
		if (!strcmp(scenario, "retry") && status.formatted && retries++ < 8) {
			divoom_product_test_format_start(1);
			return 0;
		}
		return 2;
	}
	if (submitted && !status.busy && status.phase == DIVOOM_PT_FORMAT_DONE && reboot_requested) {
		printf("FORMAT_TEST: PASS done=%d total=%d reboot=1\n", status.done, status.total);
		return 1;
	}
	return 0;
}
#endif

#if DIVOOM_BUILD_RTOS_DEVICE
#include <console.h>
static int divoom_usr_restore_whitebox_cmd(int argc, char **argv)
{
    int first = 0, last = 11, passed = 0;
    if (argc != 2) {
        printf("Usage: usr_restore_test all|0..11 (dedicated fixture files only)\n");
        return -1;
    }
    if (strcmp(argv[1], "all")) {
        char *end = NULL;
        long number = strtol(argv[1], &end, 10);
        if (!argv[1][0] || !end || *end || number < 0 || number > 11) return -1;
        first = last = (int)number;
    }
    divoom_product_test_format_init();
    if (!s_format.lock || !s_format.wifi_submit) return -1;
    /* Reserve the same local worker slot as format_start/resume before taking
     * the updater owner, which is deliberately reentrant in factory mode. */
    divoom_system_get_lock_direct(s_format.lock);
    if (s_format.worker_alive || s_format.status.busy || s_format_retry_requested ||
        s_format.status.phase == DIVOOM_PT_FORMAT_DONE ||
        s_format.status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED ||
        divoom_product_test_format_needs_recovery()) {
        divoom_system_free_lock_direct(s_format.lock);
        printf("[USR_RESTORE_WB] BUSY recovery active\n"); return -1;
    }
    s_format.worker_alive = s_format.status.busy = 1;
    divoom_system_free_lock_direct(s_format.lock);
    int factory = divoom_product_test_is_active();
    int acquired = factory ? divoom_product_test_begin_format() : divoom_update_system_file_begin_health_check();
    if (!acquired) printf("[USR_RESTORE_WB] BUSY updater active\n");
    if (acquired && factory) divoom_update_system_file_stop();
    int ok = acquired;
    if (factory && divoom_update_system_file_is_busy()) ok = 0;
    for (int scenario = first; ok && scenario <= last; ++scenario) {
        int result = divoom_update_system_file_restore_whitebox(scenario);
        printf("[USR_RESTORE_WB] %s scenario=%d\n", result == 1 ? "PASS" : "FAIL", scenario);
        if (result == 1) ++passed;
        else ok = 0;
    }
    if (acquired) {
        if (factory) divoom_product_test_end_format();
        else divoom_update_system_file_end_health_check();
    }
    divoom_system_get_lock_direct(s_format.lock);
    s_format.worker_alive = s_format.status.busy = 0;
    divoom_system_free_lock_direct(s_format.lock);
    printf("[USR_RESTORE_WB] %s total=%d/%d\n", ok ? "PASS" : "FAIL", passed, last-first+1);
    return ok ? 0 : -1;
}
FINSH_FUNCTION_EXPORT_CMD(divoom_usr_restore_whitebox_cmd, usr_restore_test,
    Test resource restore in dedicated fixture directories);
#endif
