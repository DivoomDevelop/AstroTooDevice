#if !defined(_WIN32)
#include <sys/errno.h>
#endif
#include "divoom_port_fs.h"
#include "divoom_product_test_wifi_fw.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "aic_fw.h"
#if defined(CONFIG_DRIVERS_AIC8800D80)
#include "divoom.h"
#include "divoom_wifi_con.h"
#include "divoom_port_os.h"
#endif

extern void divoom_product_test_repair_record_io(const char *, const char *, int);
extern int divoom_product_test_repair_io_error_target(int);
static void *wifi_fw_open(const char *path, const char *mode)
{
    errno = 0;
    void *file = divoom_port_file_open(path, mode);
    if (!file) divoom_product_test_repair_record_io(path, "wifi_fw_open", errno);
    return file;
}
static int wifi_fw_close(const char *path, void *file)
{
    int prior_error = errno;
    errno = 0;
    int rc = divoom_port_file_close(file);
    if (rc) divoom_product_test_repair_record_io(path, "wifi_fw_close", errno);
    else errno = prior_error;
    return rc;
}
#if defined(CONFIG_DRIVERS_AIC8800D80)
static int wifi_fw_remove(const char *path)
{
    errno = 0;
    int rc = divoom_port_remove_file(path);
    if (rc) divoom_product_test_repair_record_io(path, "wifi_fw_remove", errno);
    return rc;
}
#endif

static int wifi_fw_mkdir(const char *path)
{
    errno = 0;
    int rc = divoom_port_mkdir_p(path);
    if (rc) divoom_product_test_repair_record_io(path, "wifi_fw_mkdir", errno);
    return rc;
}
/* Unlike generic copy_file, retain whether source reading or destination
 * writing failed. Content/SUM32 mismatch and allocation failure are separate. */
static int wifi_fw_copy(const char *source, const char *destination, const char *mode,
    unsigned *unsynced, unsigned slot_bit)
{
    unsigned char *buffer = malloc(65536);
    if (!buffer) { errno = ENOMEM; return -1; }
    void *in = wifi_fw_open(source, "rb"), *out = NULL;
    int ok = in != NULL;
    if (ok) {
        out = wifi_fw_open(destination, mode);
        ok = out != NULL;
        if (ok && unsynced) __atomic_fetch_or(unsynced, slot_bit, __ATOMIC_RELEASE);
    }
    size_t copied = 0;
    while (ok) {
        errno = 0;
        size_t count = divoom_port_file_read(buffer, 1, 65536, in);
        if (ferror((FILE *)in)) {
            divoom_product_test_repair_record_io(source, "wifi_fw_copy_read", errno ? errno : EIO);
            ok = 0; break;
        }
        if (!count) break;
        if (count > AIC8800D80_H_U02_FW_SIZE - copied) {
            errno = EINVAL;
            ok = 0;
            break;
        }
        copied += count;
        errno = 0;
        if (divoom_port_file_write(buffer, 1, count, out) != count) {
            divoom_product_test_repair_record_io(destination, "wifi_fw_copy_write", errno ? errno : EIO);
            ok = 0; break;
        }
    }
    if (ok && copied != AIC8800D80_H_U02_FW_SIZE) {
        errno = EINVAL;
        ok = 0;
    }
    if (ok && divoom_port_file_sync(out) != 0) {
        divoom_product_test_repair_record_io(destination, "wifi_fw_copy_sync", errno ? errno : EIO);
        ok = 0;
    }
    if (in && wifi_fw_close(source, in)) ok = 0;
    if (out && wifi_fw_close(destination, out)) ok = 0;
    free(buffer);
    return ok ? 0 : -1;
}

#define WIFI_FW_META_MAGIC 0x32465741U /* AWF2, byte-sum verification record */
typedef struct {
    uint32_t magic;
    uint32_t size;
    uint32_t sum32;
} wifi_fw_meta;

static const wifi_fw_meta wifi_fw_expected = {WIFI_FW_META_MAGIC,
    AIC8800D80_H_U02_FW_SIZE, AIC8800D80_H_U02_FW_SUM32};

/* Match the CRC checked by the driver's file loader in the same read pass as
 * the persisted SUM32. The nibble table needs only 64 bytes of read-only data. */
static uint32_t wifi_fw_crc32_update(uint32_t crc, const uint8_t *data, size_t size)
{
    static const uint32_t table[16] = {
        0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU,
        0x76dc4190U, 0x6b6b51f4U, 0x4db26158U, 0x5005713cU,
        0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
        0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
    };
    while (size--) {
        crc ^= *data++;
        crc = (crc >> 4) ^ table[crc & 15U];
        crc = (crc >> 4) ^ table[crc & 15U];
    }
    return crc;
}

static volatile int wifi_fw_download_needed;
#if defined(CONFIG_DRIVERS_AIC8800D80)
static volatile int wifi_fw_recovery_stopped;
#endif
static void wifi_fw_recovery_start(void);

static void wifi_fw_mark_download(void)
{
    void *fp;
    int ok = 0;
    wifi_fw_download_needed = 1;
    fp = wifi_fw_open(DIVOOM_WIFI_FW_DOWNLOAD_PENDING, "wb");
    if (fp) {
        ok = divoom_port_file_write("1", 1, 1, fp) == 1 &&
            divoom_port_file_sync(fp) == 0;
        if (!ok) divoom_product_test_repair_record_io(DIVOOM_WIFI_FW_DOWNLOAD_PENDING, "wifi_fw_pending_write_sync", errno ? errno : EIO);
        if (wifi_fw_close(DIVOOM_WIFI_FW_DOWNLOAD_PENDING, fp) != 0) ok = 0;
    }
    printf("[wifi_fw] download_pending record=%s saved=%d\n", DIVOOM_WIFI_FW_DOWNLOAD_PENDING, ok);
    wifi_fw_recovery_start();
}

#if defined(CONFIG_DRIVERS_AIC8800D80)
static int wifi_fw_sync_file(const char *path)
{
    void *fp = wifi_fw_open(path, "r+b");
    int ok;
    if (!fp) {
        printf("[wifi_fw] sync_fail path=%s op=open errno=%d\n", path, errno);
        return 0;
    }
    errno = 0;
    ok = divoom_port_file_sync(fp) == 0;
    if (!ok) divoom_product_test_repair_record_io(path, "wifi_fw_sync", errno);
    if (wifi_fw_close(path, fp) != 0) ok = 0;
    if (!ok) printf("[wifi_fw] sync_fail path=%s errno=%d\n", path, errno);
    return ok;
}

#endif

/* Read a bounded amount even when a damaged directory entry claims a huge size. */
static int wifi_fw_file_checksums(const char *path, uint64_t *size_out, uint32_t *sum_out, uint32_t *crc_out)
{
    void *fp = wifi_fw_open(path, "rb");
    uint8_t *buffer;
    size_t count;
    int ok;
    if (!fp) {
        if (errno != ENOENT)
            printf("[wifi_fw] check_io_fail path=%s op=open errno=%d\n", path, errno);
        return -1;
    }
    buffer = malloc(4096);
    if (!buffer) {
        printf("[wifi_fw] check_io_fail path=%s op=alloc bytes=4096\n", path);
        wifi_fw_close(path, fp);
        errno = ENOMEM;
        return -1;
    }
    *size_out = 0;
    *sum_out = 0;
    uint32_t crc = 0xffffffffU;
    while ((count = divoom_port_file_read(buffer, 1, 4096, fp)) > 0) {
        *size_out += count;
        if (*size_out > wifi_fw_expected.size) break;
        *sum_out = aic8800_fw_sum32_update(*sum_out, buffer, count);
        crc = wifi_fw_crc32_update(crc, buffer, count);
    }
    *crc_out = crc ^ 0xffffffffU;
    ok = !ferror((FILE *)fp);
    if (!ok) {
        if (!errno) errno = EIO;
        divoom_product_test_repair_record_io(path, "wifi_fw_read", errno);
    }
    if (!ok) printf("[wifi_fw] check_io_fail path=%s op=read size=%llu sum32=%08x errno=%d\n",
        path, (unsigned long long)*size_out, *sum_out, errno);
    if (wifi_fw_close(path, fp) != 0) {
        printf("[wifi_fw] check_io_fail path=%s op=close errno=%d\n", path, errno);
        ok = 0;
    }
    free(buffer);
    return ok ? 0 : -1;
}

static int wifi_fw_matches(const char *path, const wifi_fw_meta *meta)
{
    uint64_t size;
    uint32_t sum32, crc32;
    if (meta->magic != WIFI_FW_META_MAGIC || meta->size != wifi_fw_expected.size ||
        meta->sum32 != wifi_fw_expected.sum32) {
        printf("[wifi_fw] meta_fail path=%s magic=%08x size=%u sum32=%08x\n",
            path, meta->magic, meta->size, meta->sum32);
        return 0;
    }
    if (wifi_fw_file_checksums(path, &size, &sum32, &crc32) != 0) return 0;
    if (size != meta->size || sum32 != meta->sum32 || crc32 != AIC8800D80_H_U02_FW_CRC32) {
        printf("[wifi_fw] check_fail path=%s size=%llu sum32=%08x crc32=%08x expected_size=%u\n",
            path, (unsigned long long)size, sum32, crc32, meta->size);
        return 0;
    }
    return 1;
}

/* Only a locally copied destination may turn a silent content mismatch into
 * storage evidence. Recheck its already verified source first; arbitrary bad
 * source images/downloads remain content errors, and resource errors wait. */
static int wifi_fw_verify_copy(const char *source, const char *destination, const wifi_fw_meta *meta)
{
    errno = 0;
    if (wifi_fw_matches(destination, meta)) return 1;
    if (errno) return 0;
    errno = 0;
    if (wifi_fw_matches(source, meta))
        divoom_product_test_repair_record_io(destination, "wifi_fw_copy_readback_mismatch", EIO);
    else if (!errno) divoom_product_test_repair_record_io(source, "wifi_fw_copy_source_changed", EIO);
    return 0;
}

#define WIFI_FW_PATH_SIZE 64
static unsigned wifi_fw_unsynced[2];
static pthread_mutex_t wifi_fw_backup_lock[2] = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER
};

static const char *wifi_fw_primary(int usr)
{
    return usr ? DIVOOM_WIFI_FW_SOURCE : AIC8800D80_H_U02_FW_BACKUP_FILE;
}

static void wifi_fw_alt_path(char path[WIFI_FW_PATH_SIZE], int usr, unsigned slot)
{
    const char *prefix = usr ? AIC8800D80_H_U02_FW_USR_ALT : AIC8800D80_H_U02_FW_USERDATA_ALT;
    snprintf(path, WIFI_FW_PATH_SIZE, "%s%u", prefix, slot);
}

static int wifi_fw_probe(const char *path)
{
    errno = 0;
    if (wifi_fw_matches(path, &wifi_fw_expected)) return 1;
    return errno && errno != ENOENT && errno != ENOTDIR ? -1 : 0;
}

/* A previous format backup is usable as a source but is not a driver path.
 * Distinguish a missing/bad file from one that cannot currently be read. */
static int wifi_fw_find_copy(int usr, char path[WIFI_FW_PATH_SIZE], int include_legacy)
{
    int uncertain = 0;
    for (unsigned slot = 0; slot <= AIC8800D80_H_U02_FW_ALT_COUNT; ++slot) {
        if (__atomic_load_n(&wifi_fw_unsynced[usr], __ATOMIC_ACQUIRE) & (1U << slot)) {
            uncertain = 1;
            continue;
        }
        if (slot) wifi_fw_alt_path(path, usr, slot);
        else snprintf(path, WIFI_FW_PATH_SIZE, "%s", wifi_fw_primary(usr));
        int state = wifi_fw_probe(path);
        if (__atomic_load_n(&wifi_fw_unsynced[usr], __ATOMIC_ACQUIRE) & (1U << slot)) {
            uncertain = 1;
            continue;
        }
        if (state > 0) return 1;
        if (state < 0) uncertain = 1;
    }
    if (!usr && include_legacy) {
        snprintf(path, WIFI_FW_PATH_SIZE, "%s", DIVOOM_WIFI_FW_BACKUP);
        int state = wifi_fw_probe(path);
        if (state > 0) return 1;
        if (state < 0) uncertain = 1;
    }
    return uncertain ? -1 : 0;
}

int divoom_product_test_wifi_fw_has_copy(int usr)
{
    char path[WIFI_FW_PATH_SIZE];
    int result = wifi_fw_find_copy(usr, path, 1);
    divoom_product_test_repair_io_error_target(1);
    return result;
}

/* For boot maintenance and system erase, replace a confirmed bad standard
 * userdata backup first. A read error leaves its contents unknown; use a new name. */
static int wifi_fw_write_backup(int usr, const char *source, int overwrite_bad_primary)
{
    char path[WIFI_FW_PATH_SIZE];
    int error = pthread_mutex_lock(&wifi_fw_backup_lock[usr]);
    if (error) { errno = error; return 0; }
    int ok = 0;
    for (unsigned attempt = 0; attempt <= AIC8800D80_H_U02_FW_ALT_COUNT; ++attempt) {
        if (attempt) wifi_fw_alt_path(path, usr, attempt);
        else snprintf(path, sizeof(path), "%s", wifi_fw_primary(usr));

        unsigned bit = 1U << attempt;
        int uncommitted = !!(__atomic_load_n(&wifi_fw_unsynced[usr], __ATOMIC_ACQUIRE) & bit);
        int state = wifi_fw_probe(path);
        int missing = state == 0 && errno == ENOENT;
        int bad_primary = !attempt && overwrite_bad_primary && state == 0 && errno == 0;
        if (state > 0 && !uncommitted) { ok = 1; break; }
        if ((missing || bad_primary || (uncommitted && state >= 0)) &&
            (attempt || !usr || wifi_fw_mkdir("/usr/aic") == 0) &&
            wifi_fw_copy(source, path, missing ? "wbx" : "wb", &wifi_fw_unsynced[usr], bit) == 0 &&
            wifi_fw_verify_copy(source, path, &wifi_fw_expected)) {
            __atomic_fetch_and(&wifi_fw_unsynced[usr], ~bit, __ATOMIC_RELEASE);
            printf("[wifi_fw] backup_verified src=%s dst=%s attempt=%u\n", source, path, attempt);
            ok = 1;
            break;
        }
        printf("[wifi_fw] backup_attempt_failed dst=%s attempt=%u errno=%d\n", path, attempt, errno);
    }
    pthread_mutex_unlock(&wifi_fw_backup_lock[usr]);
    return ok;
}

int divoom_product_test_wifi_fw_maintain_backup(void)
{
    char source[WIFI_FW_PATH_SIZE];
    if (!(__atomic_load_n(&wifi_fw_unsynced[0], __ATOMIC_ACQUIRE) & 1U) &&
        wifi_fw_probe(AIC8800D80_H_U02_FW_BACKUP_FILE) > 0 &&
        !(__atomic_load_n(&wifi_fw_unsynced[0], __ATOMIC_ACQUIRE) & 1U)) return 1;
    if (wifi_fw_find_copy(1, source, 0) <= 0 && wifi_fw_find_copy(0, source, 1) <= 0) {
        printf("[wifi_fw] cannot repair backup: no verified firmware file\n");
        wifi_fw_mark_download();
        return 0;
    }
    return wifi_fw_write_backup(0, source, 1);
}

void aic8800d80_h_u02_fw_recovery_notify(int backup_loaded, int download_needed)
{
#if defined(CONFIG_DRIVERS_AIC8800D80)
    if (wifi_fw_recovery_stopped) return; /* The maintenance worker owns writes. */
#endif
    if (download_needed) {
        wifi_fw_mark_download();
    } else if (backup_loaded) {
        /* A server-installed firmware may target a different revision. The
         * verified backup just loaded by the driver must not replace it. */
        char source[WIFI_FW_PATH_SIZE];
        if (wifi_fw_find_copy(1, source, 0) > 0) return;
        if (wifi_fw_find_copy(0, source, 1) <= 0 || !wifi_fw_write_backup(1, source, 0)) {
            printf("[wifi_fw] loaded backup, but /usr repair failed\n");
            wifi_fw_mark_download();
        }
    }
}

#if defined(CONFIG_DRIVERS_AIC8800D80)
static volatile int wifi_fw_recovery_running;

static int wifi_fw_download_cancel(void *arg)
{
    uint32_t started = *(const uint32_t *)arg;
    return wifi_fw_recovery_stopped || (uint32_t)(divoom_port_tick_ms() - started) >= 300000U;
}

static int wifi_fw_download_repair(uint32_t *started)
{
    const char *step = "mkdir_tmp";
    int rc = 0, bytes = 0;
    if (wifi_fw_recovery_stopped || !divoom_wifi_model_is_network_ready()) return 0;
    /* A failed transfer or wrong server image must never replace either copy. */
    printf("[wifi_fw] download_begin remote=%s expected_size=%u expected_sum32=%08x\n",
        DIVOOM_WIFI_FW_REMOTE, wifi_fw_expected.size, wifi_fw_expected.sum32);
    if (wifi_fw_mkdir("/userdata/tmp") != 0) goto failed;
    step = "transport";
    rc = divoom_http_client_down_file_with_exit(DIVOOM_WIFI_FW_DOWNLOAD_STAGE,
        DIVOOM_WIFI_FW_REMOTE, 1, &bytes, wifi_fw_download_cancel, started);
    if (rc != 1) goto failed;
    step = "paused";
    if (wifi_fw_download_cancel(started)) goto failed;
    step = "sync_download";
    if (!wifi_fw_sync_file(DIVOOM_WIFI_FW_DOWNLOAD_STAGE)) goto failed;
    step = "verify_download";
    if (!wifi_fw_matches(DIVOOM_WIFI_FW_DOWNLOAD_STAGE, &wifi_fw_expected)) goto failed;
    step = "install_usr";
    if (!wifi_fw_write_backup(1, DIVOOM_WIFI_FW_DOWNLOAD_STAGE, 0)) goto failed;
    step = "rebuild_backup";
    if (!divoom_product_test_wifi_fw_maintain_backup()) goto failed;
    step = "clear_pending";
    if (divoom_port_file_exists(DIVOOM_WIFI_FW_DOWNLOAD_PENDING) &&
        wifi_fw_remove(DIVOOM_WIFI_FW_DOWNLOAD_PENDING) != 0)
        goto failed;
    wifi_fw_download_needed = 0;
    wifi_fw_remove(DIVOOM_WIFI_FW_DOWNLOAD_STAGE);
    printf("[wifi_fw] download_ok usr_and_backup_verified size=%u sum32=%08x\n",
        wifi_fw_expected.size, wifi_fw_expected.sum32);
    return 1;
failed:
    printf("[wifi_fw] download_fail step=%s rc=%d bytes=%d pending=keep errno=%d\n", step, rc, bytes, errno);
    return 0;
}

static void *wifi_fw_recovery_worker(void *arg)
{
    int wait, waiting = 0, attempts = 0;
    uint32_t started = divoom_port_tick_ms();
    (void)arg;
    while (!wifi_fw_recovery_stopped && wifi_fw_download_needed) {
        if ((uint32_t)(divoom_port_tick_ms() - started) >= 300000U || attempts >= 3) {
            printf("[wifi_fw] download_deferred attempts=%d pending=keep\n", attempts);
            break;
        }
        if (divoom_wifi_model_is_network_ready()) {
            waiting = 0;
            ++attempts;
            if (wifi_fw_download_repair(&started)) break;
            if (wifi_fw_recovery_stopped) break;
            if (attempts >= 3) continue;
            printf("[wifi_fw] download_retry delay_s=30\n");
            for (wait = 0; wait < 60 && !wifi_fw_recovery_stopped &&
                    (uint32_t)(divoom_port_tick_ms() - started) < 300000U; ++wait)
                divoom_port_delay_ms(500);
        } else {
            if (!waiting) printf("[wifi_fw] download_wait reason=network_not_ready\n");
            waiting = 1;
            divoom_port_delay_ms(500);
        }
    }
    __sync_lock_release(&wifi_fw_recovery_running);
    return NULL;
}

static void wifi_fw_recovery_start(void)
{
    if (wifi_fw_recovery_stopped ||
        !__sync_bool_compare_and_swap(&wifi_fw_recovery_running, 0, 1)) return;
    if (!divoom_port_thread_create_ex(wifi_fw_recovery_worker, NULL,
            "wifi_fw_fix", 8192, 0)) {
        __sync_lock_release(&wifi_fw_recovery_running);
        printf("[wifi_fw] cannot start network repair worker; pending record retained\n");
    }
}

int divoom_product_test_wifi_fw_recovery_pause(void)
{
    int wait;
    wifi_fw_recovery_stopped = 1;
    for (wait = 0; wait < 50 && wifi_fw_recovery_running; ++wait)
        divoom_port_delay_ms(100);
    if (wifi_fw_recovery_running) printf("[wifi_fw] pause_timeout action=deny_format\n");
    return !wifi_fw_recovery_running;
}

void divoom_product_test_wifi_fw_recovery_resume(void)
{
    if (wifi_fw_recovery_running) return;
    wifi_fw_recovery_stopped = 0;
    if (divoom_port_file_exists(DIVOOM_WIFI_FW_DOWNLOAD_PENDING)) {
        wifi_fw_download_needed = 1;
        printf("[wifi_fw] download_resume record=%s\n", DIVOOM_WIFI_FW_DOWNLOAD_PENDING);
    }
    if (wifi_fw_download_needed) wifi_fw_recovery_start();
}
#else
static void wifi_fw_recovery_start(void) {}
int divoom_product_test_wifi_fw_recovery_pause(void) { return 1; }
void divoom_product_test_wifi_fw_recovery_resume(void) {}
#endif

int divoom_product_test_wifi_fw_safe_to_erase(int target_usr)
{
    char source[WIFI_FW_PATH_SIZE];
    int peer = !target_usr;
    divoom_product_test_repair_io_error_target(1);
    int safe = wifi_fw_find_copy(peer, source, 1) > 0;
    if (!safe && wifi_fw_find_copy(target_usr, source, 1) > 0)
        safe = wifi_fw_write_backup(peer, source, target_usr);
    printf("[wifi_fw] erase_guard target=%s peer_file=%s\n",
        target_usr ? "/usr" : "/userdata", safe ? "verified" : "unavailable");
    /* Failing to save a copy is a reason to defer erase, never to erase the peer. */
    divoom_product_test_repair_io_error_target(1);
    return safe;
}

int divoom_product_test_wifi_fw_prepare_network(void)
{
    char source[WIFI_FW_PATH_SIZE];
    divoom_product_test_repair_io_error_target(1);
    int ok = wifi_fw_find_copy(1, source, 0) > 0 || wifi_fw_find_copy(0, source, 0) > 0 ||
        (wifi_fw_find_copy(0, source, 1) > 0 && wifi_fw_write_backup(0, source, 0));
    /* A missing/unwritable firmware copy must not trigger another erase. */
    divoom_product_test_repair_io_error_target(1);
    return ok;
}

static int wifi_fw_restore_impl(void)
{
    char backup[WIFI_FW_PATH_SIZE];
    /* Keep a server-installed file and use a numbered path if needed. */
    if (wifi_fw_find_copy(1, backup, 0) > 0) return 1;
    if (wifi_fw_find_copy(0, backup, 1) <= 0 || !wifi_fw_write_backup(1, backup, 0)) {
        printf("[wifi_fw] missing firmware: backup restore failed\n");
        return 0;
    }
    printf("[wifi_fw] restored system firmware from %s\n", backup);
    return 1;
}

static int wifi_fw_cleanup_impl(void)
{
    /* The worker calls this only after every resource passed final validation. */
    if (!divoom_port_file_exists(DIVOOM_WIFI_FW_BACKUP_DIR)) return 1;
    char driver_copy[WIFI_FW_PATH_SIZE];
    if (wifi_fw_find_copy(1, driver_copy, 0) <= 0 && wifi_fw_find_copy(0, driver_copy, 0) <= 0) {
        printf("[wifi_fw] cleanup_defer reason=no_driver_file keep_format_backup=1\n");
        return 0;
    }
    if (divoom_port_remove_tree(DIVOOM_WIFI_FW_BACKUP_DIR) != 0) {
        divoom_product_test_repair_record_io(DIVOOM_WIFI_FW_BACKUP_DIR, "wifi_fw_cleanup", errno);
        printf("[wifi_fw] cleanup_fail path=%s errno=%d\n", DIVOOM_WIFI_FW_BACKUP_DIR, errno);
        return 0;
    }
    printf("[wifi_fw] completed restore: backup removed\n");
    return 1;
}

int divoom_product_test_wifi_fw_restore(void)
{
    int ok = wifi_fw_restore_impl();
    divoom_product_test_repair_io_error_target(1);
    return ok;
}

int divoom_product_test_wifi_fw_cleanup(void)
{
    int ok = wifi_fw_cleanup_impl();
    if (ok) divoom_product_test_repair_io_error_target(1);
    return ok;
}
