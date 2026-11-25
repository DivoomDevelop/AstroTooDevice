#if !defined(_WIN32)
#include <sys/errno.h>
#endif
#include "divoom.h"
#include "divoom_device_error.h"
#include "divoom_product_test_usr_check.h"
#include "divoom_product_test_format.h"
#include "divoom_update_system_file.h"
#include "divoom_port_fs_check.h"
#include <errno.h>
#include <stdio.h>

typedef struct {
    divoom_product_test_usr_check_status *status;
    void *verifier;
} usr_check_scan;

static void *check_lock;
static divoom_product_test_usr_check_status check_status;

static int usr_check_lock_take(void)
{
    void *lock = __atomic_load_n(&check_lock, __ATOMIC_ACQUIRE);
    if (!lock) {
        void *created = divoom_system_create_lock();
        if (!created) return 0;
        lock = __sync_val_compare_and_swap(&check_lock, NULL, created);
        if (lock) divoom_system_destroy_lock(created);
        else lock = created;
    }
    divoom_system_get_lock_direct(lock);
    return 1;
}

void divoom_product_test_usr_check_get_status(divoom_product_test_usr_check_status *status)
{
    if (!status) return;
    if (!usr_check_lock_take()) {
        memset(status, 0, sizeof(*status));
        status->phase = DIVOOM_PT_USR_CHECK_FAILED;
        status->error_code = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
        return;
    }
    *status = check_status;
    divoom_system_free_lock_direct(check_lock);
}

static void usr_check_publish(const divoom_product_test_usr_check_status *status)
{
    /* The lock exists before the worker is created. No file I/O under it. */
    divoom_system_get_lock_direct(check_lock);
    check_status = *status;
    divoom_system_free_lock_direct(check_lock);
}

static int usr_check_count(const divoom_port_dir_change_t *file, void *arg)
{
    divoom_product_test_usr_check_status *status = arg;
    (void)file;
    if (status->total == 0x7fffffff) return 1;
    ++status->total;
    return 0;
}

static int usr_check_file(const divoom_port_dir_change_t *file, void *arg)
{
    usr_check_scan *scan = arg;
    divoom_product_test_usr_check_status *status = scan->status;
    char path[512];
    int rc, saved_errno;
    if (snprintf(path, sizeof(path), "/usr/%s", file->relative_path) >= sizeof(path)) {
        status->error_code = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
        return 1;
    }
    rc = divoom_port_file_verifier_verify(scan->verifier, path,
        file->old_size, file->old_crc32, 1);
    saved_errno = errno;
    /* Other manifest entries remain required; the verifier handles the
     * optional font143 exception before returning a result here. */
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) {
        status->error_code = rc;
        return 1;
    }
    if (rc) {
        if (rc == DIVOOM_PORT_DIR_COMPARE_DIFFERENT)
            divoom_device_error_record(DIVOOM_SD_NAND_ERR_USR_DATA);
        ++status->error_files;
        divoom_print("pt_usr_check: bad path=%s error=%d errno=%d\n", path, rc, saved_errno);
    }
    ++status->done;
    if (rc || status->done == status->total || (status->done & 15) == 0)
        usr_check_publish(status);
    /* Keep other tasks responsive without adding a sleep for every file. */
    if ((status->done & 15) == 0) divoom_system_delay_ms(1);
    return 0;
}

/* About only probes whether maintenance is needed. Factory checks retain
 * their full scan so the final error count includes every bad file. */
static int usr_check_file_until_error(const divoom_port_dir_change_t *file, void *arg)
{
    usr_check_scan *scan = arg;
    divoom_product_test_usr_check_status *status = scan->status;
    int rc = usr_check_file(file, arg);
    return rc || status->error_files != 0;
}

static void usr_check_structure_progress(uint32_t stage, uint32_t done, uint32_t total, void *arg)
{
    divoom_product_test_usr_check_status *status = arg;
    status->fs_check_stage = stage;
    status->done = done > 0x7fffffffU ? 0x7fffffff : (int)done;
    status->total = total > 0x7fffffffU ? 0x7fffffff : (int)total;
    usr_check_publish(status);
}

static int usr_check_structure(divoom_product_test_usr_check_status *status)
{
    for (int target = 0; target < 2; ++target) {
        int rc = FF_CHECK_BUSY;
        ff_check_result result;
        status->fs_check_target = target;
        status->fs_check_stage = FF_CHECK_STAGE_FAT;
        status->done = status->total = 0;
        usr_check_publish(status);
        for (int attempt = 1; attempt <= 3; ++attempt) {
            memset(&result, 0, sizeof(result));
            rc = divoom_port_fs_check_volume(target ? "/dev/usr" : "/dev/UDISK",
                target ? "/usr" : "/userdata", &result, NULL, 0,
                usr_check_structure_progress, status);
            divoom_print("pt_fs_check: volume=%s attempt=%d/3 result=%d complete=%u damage=%08x affected=%u io=%u\n",
                target ? "/usr" : "/userdata", attempt, rc, result.complete,
                result.damage_flags, result.affected_files, result.fs_error);
            if (rc == FF_CHECK_CLEAN && result.complete) break;
            if (rc == FF_CHECK_STRUCTURAL || rc == FF_CHECK_UNSUPPORTED) break;
            if (attempt < 3) divoom_system_delay_ms(1000);
        }
        if (rc != FF_CHECK_CLEAN || !result.complete) {
            if (rc == FF_CHECK_STRUCTURAL && result.complete &&
                result.damage_flags == FF_CHECK_ORPHAN) {
                /* Factory checks delegate reclamation to the journaled
                 * maintenance worker without reporting a file error. */
                status->fs_damage_flags = FF_CHECK_ORPHAN;
                status->maintenance_required = 1;
                return 0;
            }
            status->error_code = rc ? rc : FF_CHECK_IO;
            status->fs_damage_flags = result.damage_flags;
            status->error_files = result.affected_files > 0x7fffffffU ? 0x7fffffff : (int)result.affected_files;
            status->maintenance_required = 1;
            return 0;
        }
    }
    status->fs_check_stage = 0;
    status->done = status->total = 0;
    usr_check_publish(status);
    return 1;
}

static void *usr_check_worker(void *arg)
{
    divoom_product_test_usr_check_status status = {0};
    usr_check_scan scan = { &status, NULL };
    uint32 started = divoom_system_get_tickms();
    divoom_port_stat_t st;
    int rc, saved_errno;
    (void)arg;
    status.phase = DIVOOM_PT_USR_CHECK_RUNNING;
    /* About runs in the live system: only its gated /usr CRC pass is safe.
     * Full FAT/directory checks belong to the factory/maintenance entries. */
    if (!arg && !usr_check_structure(&status)) { rc = status.error_code; goto finished; }
    memset(&st, 0, sizeof(st));
    errno = 0;
    rc = divoom_port_stat("/usr", &st);
    saved_errno = errno;
    divoom_print("pt_usr_check: root stat rc=%d errno=%d is_dir=%d\n", rc, saved_errno, st.is_dir);
    memset(&st, 0, sizeof(st));
    errno = 0;
    rc = divoom_port_stat("/usr/file_crc.bin", &st);
    saved_errno = errno;
    divoom_print("pt_usr_check: manifest stat rc=%d errno=%d size=%lld is_dir=%d\n",
        rc, saved_errno, (long long)st.size, st.is_dir);
    errno = 0;
    rc = divoom_port_dir_manifest_visit("/usr/file_crc.bin", "/usr", usr_check_count, &status);
    saved_errno = errno;
    divoom_print("pt_usr_check: manifest count rc=%d errno=%d total=%d\n", rc, saved_errno, status.total);
    if (!rc && !status.total) rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
    if (!rc) {
        scan.verifier = divoom_port_file_verifier_create();
        if (!scan.verifier) rc = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
    }
    if (!rc) {
        usr_check_publish(&status);
        errno = 0;
        rc = divoom_port_dir_manifest_visit("/usr/file_crc.bin", "/usr",
            arg ? usr_check_file_until_error : usr_check_file, &scan);
        saved_errno = errno;
        divoom_print("pt_usr_check: verify rc=%d errno=%d checked=%d/%d bad=%d\n",
            rc, saved_errno, status.done, status.total, status.error_files);
    }
    divoom_port_file_verifier_destroy(scan.verifier);
    (void)saved_errno; /* Diagnostics may be disabled by the log level. */
    /* A Settings probe intentionally stops at its first confirmed error.
     * Only that expected callback stop becomes a completed error result. */
    if (arg && status.error_files && !status.error_code &&
        rc == DIVOOM_PORT_DIR_COMPARE_ERR_CALLBACK) rc = 0;
    if (!rc && status.done != status.total && !(arg && status.error_files))
        rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
finished:
    if (rc && !status.error_code) status.error_code = rc;
    status.phase = rc ? DIVOOM_PT_USR_CHECK_FAILED : DIVOOM_PT_USR_CHECK_DONE;
    if (status.error_files || rc) status.maintenance_required = 1;
    divoom_print("pt_usr_check: complete checked=%d/%d bad=%d error=%d elapsed_ms=%u\n",
        status.done, status.total, status.error_files, status.error_code,
        (unsigned)(divoom_system_get_tickms() - started));
    int repair = arg && (status.error_files || status.phase == DIVOOM_PT_USR_CHECK_FAILED);
    if (repair) {
        repair = divoom_update_system_file_hold_health_check_for_repair();
        divoom_product_test_format_settings_request_set_status(repair ? 0 : -1);
    } else if (arg) {
        divoom_update_system_file_end_health_check();
    }
    usr_check_publish(&status); /* Last access to /usr precedes terminal status. */
    /* The worker owns the request, even if About is paused or has been closed.
     * Main owns queued updates from here; no LVGL object is accessed. */
    if (repair)
        divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CHECK_SYSTEM_FILES);
    return NULL;
}

static int usr_check_start(int settings)
{
    if (!usr_check_lock_take()) return 0;
    if (check_status.phase == DIVOOM_PT_USR_CHECK_RUNNING ||
        (!settings && check_status.phase != DIVOOM_PT_USR_CHECK_IDLE)) {
        divoom_system_free_lock_direct(check_lock);
        return 0;
    }
    memset(&check_status, 0, sizeof(check_status));
    check_status.phase = DIVOOM_PT_USR_CHECK_RUNNING;
    divoom_system_free_lock_direct(check_lock);
    int held = settings && divoom_update_system_file_begin_health_check();
    if ((!settings || held) && divoom_system_create_task(usr_check_worker, "pt_usr_check",
            settings ? (void*)1 : NULL, DIVOOM_TASK_STACK_16K)) return 1;
    if (held) divoom_update_system_file_end_health_check();
    divoom_product_test_usr_check_status failed = {0};
    failed.phase = DIVOOM_PT_USR_CHECK_FAILED;
    failed.error_code = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
    failed.maintenance_required = !settings;
    usr_check_publish(&failed);
    divoom_print("pt_usr_check: worker unavailable or resource update busy\n");
    return 0;
}

void divoom_product_test_usr_check_start(void)
{
    (void)usr_check_start(0);
}

int divoom_product_test_usr_check_start_settings(void)
{
    return usr_check_start(1);
}
