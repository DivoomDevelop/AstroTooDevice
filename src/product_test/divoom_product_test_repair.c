#if !defined(_WIN32)
#include <sys/errno.h> /* Constants must not come from the wireless iw/errno.h. */
#endif
#include "divoom.h"
#include "divoom_device_error.h"
#include "divoom_product_test_format.h"
#include "divoom_product_test_repair.h"
#if defined(CONFIG_DRIVERS_AIC8800D80)
#include "divoom_product_test_wifi_fw.h"
#include <strings.h>
#endif
#include <errno.h>
#include <stdio.h>

#define REPAIR_LIST "/userdata/.usr_repair_files.bin"
#define REPAIR_BASE "/userdata/.usr_check_crc.bin"
#define LOCAL_LEDGER "/usr/file_crc.bin"
#define ORIGINAL_LEDGER "/userdata/.usr_original_crc.bin"
#define ORIGINAL_VERSION "/userdata/.usr_original_crc_version.txt"
#define ORIGINAL_READY "/userdata/.usr_original_crc_ready"
#define REPAIR_PLAN_MAX_FILES 512
#define REPAIR_PLAN_MAX_BYTES (256 * 1024)

typedef struct {
    divoom_cJSON *files;
    void *verifier;
    int done, total, failed, collect, temporary;
    int plan_bytes, plan_full;
    divoom_system_file_restore_progress progress;
} repair_scan;

static int s_repair_io_target = -1;
int divoom_product_test_repair_io_error_target(int clear)
{
    return clear ? __atomic_exchange_n(&s_repair_io_target, -1, __ATOMIC_ACQ_REL) :
        __atomic_load_n(&s_repair_io_target, __ATOMIC_ACQUIRE);
}
static void repair_io_error(const char *path, const char *operation, int error)
{
    if (error != EIO && error != ENOSPC && error != EROFS && error != EACCES) return;
    int target = !strncmp(path, "/userdata/", 10) ? 0 : !strncmp(path, "/usr/", 5) ? 1 : -1;
    if (target < 0) return;
    __atomic_store_n(&s_repair_io_target, target, __ATOMIC_RELEASE);
    divoom_print("usr_repair: local_io path=%s operation=%s errno=%d target=%d\n", path, operation, error, target);
}
void divoom_product_test_repair_record_io(const char *path, const char *operation, int error)
{
    repair_io_error(path, operation, error);
}
static int repair_stat(const char *path, divoom_port_stat_t *st)
{
    errno = 0;
    int rc = divoom_port_stat(path, st);
    if (rc) repair_io_error(path, "stat", errno);
    return rc;
}
static int repair_remove(const char *path)
{
    errno = 0;
    int rc = divoom_port_remove_file(path);
    if (rc) repair_io_error(path, "remove", errno);
    return rc;
}
static int repair_rename(const char *source, const char *destination)
{
    errno = 0;
    int rc = divoom_port_rename_file(source, destination);
    if (rc) repair_io_error(destination, "rename", errno);
    return rc;
}
static int repair_manifest(const char *path, const char *root, int create)
{
    errno = 0;
    int rc = divoom_port_dir_manifest_prepare_dirs(path, root, create);
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO) repair_io_error(path, "manifest_read", errno);
    return rc;
}

static int repair_sync(const char *path)
{
    void *fp = divoom_port_file_open(path, "r+b");
    int ok;
    if (!fp) { repair_io_error(path, "open_sync", errno); return 0; }
    ok = divoom_port_file_sync(fp) == 0;
    int error = errno;
    if (divoom_port_file_close(fp) != 0) { ok = 0; error = errno; }
    if (!ok) repair_io_error(path, "sync_close", error);
    return ok;
}

static int repair_copy_verified(const char *source, const char *destination)
{
    uint64_t size;
    uint32_t crc;
    errno = 0;
    int rc = divoom_port_file_crc32(source, &size, &crc);
    if (rc != 0) {
        if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) errno = ENOMEM;
        repair_io_error(source, "source_crc_read", errno); return 0;
    }
    unsigned char *buffer = divoom_system_malloc(65536);
    if (!buffer) { errno = ENOMEM; return 0; }
    errno = 0;
    void *in = divoom_port_file_open(source, "rb"), *out = NULL;
    int ok = in != NULL;
    if (!in) repair_io_error(source, "source_open", errno);
    if (ok) {
        errno = 0; out = divoom_port_file_open(destination, "wb"); ok = out != NULL;
        if (!ok) repair_io_error(destination, "destination_open", errno);
    }
    uint64_t copied = 0;
    while (ok) {
        errno = 0;
        size_t count = divoom_port_file_read(buffer, 1, 65536, in);
        if (ferror((FILE *)in)) {
            ok = 0; repair_io_error(source, "source_read", errno ? errno : EIO); break;
        }
        if (!count) break;
        errno = 0;
        size_t written = divoom_port_file_write(buffer, 1, count, out);
        if (written != count) {
            ok = 0; repair_io_error(destination, "destination_write", errno ? errno : EIO); break;
        }
        copied += count;
    }
    if (ok && copied != size) { ok = 0; errno = EAGAIN; } /* Source changed: recheck, not erase. */
    if (ok && divoom_port_file_sync(out) != 0) {
        ok = 0; repair_io_error(destination, "destination_sync", errno);
    }
    if (in && divoom_port_file_close(in) != 0) { ok = 0; repair_io_error(source, "source_close", errno); }
    if (out && divoom_port_file_close(out) != 0) { ok = 0; repair_io_error(destination, "destination_close", errno); }
    divoom_system_free(buffer);
    if (!ok) return 0;
    errno = 0;
    rc = divoom_port_file_verify(destination, size, crc, 0);
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) errno = ENOMEM;
    if (rc) repair_io_error(destination, "destination_verify", errno);
    if (rc == DIVOOM_PORT_DIR_COMPARE_DIFFERENT) {
        /* Local copy succeeded but its installed bytes differ. Re-read the
         * source only on this failure path to locate the faulty volume. */
        errno = 0;
        int source_rc = divoom_port_file_verify(source, size, crc, 0);
        if (!source_rc) repair_io_error(destination, "destination_crc_mismatch", EIO);
        else if (source_rc == DIVOOM_PORT_DIR_COMPARE_DIFFERENT)
            repair_io_error(source, "source_changed_crc", EIO);
        else repair_io_error(source, "source_recheck", errno);
    }
    return rc == 0;
}

static int repair_count(const divoom_port_dir_change_t *file, void *arg)
{
    repair_scan *s = arg;
    (void)file;
    if (s->total == 0x7fffffff) return 1;
    ++s->total;
    return 0;
}

static divoom_cJSON *repair_find(divoom_cJSON *files, const char *path)
{
    divoom_cJSON *item;
    for (item = files ? files->child : NULL; item; item = item->next) {
        divoom_cJSON *p = divoom_cJSON_GetObjectItem(item, "path");
        if (p && p->type == divoom_cJSON_String && !strcmp(p->valuestring, path)) return item;
    }
    return NULL;
}

static int repair_temporary_errno(void)
{
    return errno == ENOMEM || errno == EMFILE || errno == ENFILE ||
        errno == EBUSY || errno == ETIMEDOUT;
}

static int repair_check(const divoom_port_dir_change_t *file, void *arg)
{
    repair_scan *s = arg;
    char path[512];
    int rc;
    if (snprintf(path, sizeof(path), "/usr/%s", file->relative_path) >= sizeof(path)) return 1;
    /* During final validation repaired files are checked against the download
     * guide separately, since the original local ledger must not be changed. */
    rc = !s->collect && repair_find(s->files, file->relative_path) ? 0 :
        divoom_port_file_verifier_verify(s->verifier, path, file->old_size, file->old_crc32, 1);
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (rc && repair_temporary_errno())) {
        s->temporary = 1;
        return 1;
    }
    if (rc) {
        if (rc == DIVOOM_PORT_DIR_COMPARE_DIFFERENT)
            divoom_device_error_record(DIVOOM_SD_NAND_ERR_USR_DATA);
        divoom_port_stat_t st;
        long actual = repair_stat(path, &st) == 0 ? st.size : -1;
        ++s->failed;
        divoom_product_test_format_set_error_files(s->collect ? s->failed :
            divoom_cJSON_GetArraySize(s->files) + s->failed);
        divoom_print("usr_check: path=%s error=%d expected_size=%llu actual_size=%ld expected_crc=%08x\n",
            path, rc, (unsigned long long)file->old_size, actual, file->old_crc32);
        if (s->collect && !s->plan_full) {
            /* Include the object, separator and cJSON's print lookahead. */
            int bytes = 16;
            const unsigned char *p = (const unsigned char *)file->relative_path;
            for (; *p; ++p) {
                bytes += *p < 0x20 ? 6 : (*p == '"' || *p == '\\') ? 2 : 1;
            }
            if (s->failed > REPAIR_PLAN_MAX_FILES || bytes > REPAIR_PLAN_MAX_BYTES - s->plan_bytes) {
                s->plan_full = 1;
            } else {
                s->plan_bytes += bytes;
                if (s->files) {
                    divoom_cJSON *item = divoom_cJSON_CreateObject();
                    if (!item) { s->temporary = 1; return 1; }
                    divoom_cJSON_AddStringToObject(item, "path", file->relative_path);
                    if (!divoom_cJSON_GetObjectItem(item, "path")) {
                        s->temporary = 1; divoom_cJSON_Delete(item); return 1;
                    }
                    divoom_cJSON_AddItemToArray(s->files, item);
                }
            }
        }
    }
    ++s->done;
    if (s->progress && (rc || s->done == s->total || (s->done & 15) == 0))
        s->progress(s->done, s->total, -1);
    return 0;
}

static int repair_scan_manifest(const char *manifest, repair_scan *scan)
{
    scan->plan_bytes = 8; /* Array brackets, NUL and print lookahead. */
    scan->verifier = divoom_port_file_verifier_create();
    if (!scan->verifier) { scan->temporary = 1; return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM; }
    int rc = divoom_port_dir_manifest_visit(manifest, "/usr", repair_count, scan);
    if (!rc) rc = divoom_port_dir_manifest_visit(manifest, "/usr", repair_check, scan);
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) scan->temporary = 1;
    int error = errno;
    divoom_port_file_verifier_destroy(scan->verifier);
    scan->verifier = NULL;
    errno = error;
    return rc;
}

int divoom_product_test_repair_preview(divoom_product_test_repair_plan *plan,
    divoom_system_file_restore_progress progress)
{
    repair_scan scan = {.collect = 1, .progress = progress};
    divoom_product_test_format_set_phase(DIVOOM_PT_FORMAT_CHECK);
    divoom_product_test_format_set_error_files(0);
    int rc = repair_scan_manifest(LOCAL_LEDGER, &scan);
    int error = errno;
    divoom_print("usr_check: stage=preview crc checked=%d/%d bad=%d result=%d\n",
        scan.done, scan.total, scan.failed, rc);
    errno = error;
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT ||
        (rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO && errno == ENOENT && !scan.done)) {
        plan->check_system_files = 1;
        return 1;
    }
    if (rc) return 0;
    if (scan.plan_full) plan->format_mask |= 2U;
    else plan->download_files = scan.failed;
    return 1;
}

static uint32_t repair_list_checksum(const unsigned char *data, size_t size)
{
    uint32_t value = 2166136261U;
    while (size--) value = (value ^ *data++) * 16777619U;
    return value;
}

static int repair_save_list(divoom_cJSON *files, int budget)
{
    char *json = divoom_system_malloc(budget);
    void *fp;
    int ok;
    size_t len;
    uint32_t checksum;
    if (!json) { errno = ENOMEM; return 0; }
    if (!divoom_cJSON_PrintPreallocated(files, json, budget, 0)) {
        divoom_system_free(json);
        errno = EFBIG;
        return 0;
    }
    len = strlen(json);
    checksum = repair_list_checksum((unsigned char *)json, len);
    errno = 0;
    fp = divoom_port_file_open(REPAIR_LIST ".tmp", "wb");
    ok = fp && divoom_port_file_write(&checksum, 1, sizeof(checksum), fp) == sizeof(checksum) &&
        divoom_port_file_write(json, 1, len, fp) == len &&
        divoom_port_file_sync(fp) == 0;
    int write_error = !ok && fp ? (errno ? errno : EIO) : errno;
    if (fp && divoom_port_file_close(fp) != 0) { ok = 0; write_error = errno ? errno : EIO; }
    divoom_system_free(json);
    if (!ok) { repair_io_error(REPAIR_LIST, "save_list", write_error); errno = write_error; return 0; }
    if (divoom_port_sync() != 0) { repair_io_error(REPAIR_LIST, "save_list_sync", errno ? errno : EIO); return 0; }
    /* C state may replace a previous incomplete scan, never a live P task. */
    errno = 0;
    if (repair_remove(REPAIR_LIST) != 0 && errno != ENOENT) { repair_io_error(REPAIR_LIST, "list_remove", errno); return 0; }
    int committed = repair_rename(REPAIR_LIST ".tmp", REPAIR_LIST) == 0 && divoom_port_sync() == 0;
    if (!committed) repair_io_error(REPAIR_LIST, "list_commit", errno);
    return committed;
}

/* The persisted format is an array of flat objects. Bound parser node
 * allocations before parsing, including records written by older firmware. */
static int repair_list_within_limits(const char *json)
{
    unsigned depth = 0, tokens = 0;
    int quoted = 0;
    for (; *json; ++json) {
        if (quoted && *json == '\\' && json[1]) { ++json; continue; }
        if (*json == '"') quoted = !quoted;
        if (quoted) continue;
        if (*json == '[' || *json == '{') {
            if (++depth > 2) return 0;
            ++tokens;
        } else if (*json == ']' || *json == '}') {
            if (!depth) return 0;
            --depth;
        } else if (*json == ',') ++tokens;
        /* Old records can have six properties per file, including skip. */
        if (tokens > 7U * REPAIR_PLAN_MAX_FILES + 1U) return 0;
    }
    return !quoted && !depth;
}

static divoom_cJSON *repair_load_list(int *invalid)
{
    divoom_port_stat_t st;
    if (invalid) *invalid = 0;
    if (repair_stat(REPAIR_LIST, &st) != 0) {
        if (invalid && errno == ENOENT) *invalid = 1;
        return NULL;
    }
    if (st.is_dir || st.size <= sizeof(uint32_t) || st.size > REPAIR_PLAN_MAX_BYTES + (int)sizeof(uint32_t)) {
        if (invalid) *invalid = 1;
        return NULL;
    }
    int len;
    errno = 0;
    char *json = divoom_port_file_read_all_bounded(REPAIR_LIST, &len,
        REPAIR_PLAN_MAX_BYTES + sizeof(uint32_t));
    if (!json || len != st.size) {
        int error = errno;
        if (!json && error == EFBIG && invalid) *invalid = 1;
        if (json && !error) error = EIO;
        divoom_system_free(json);
        repair_io_error(REPAIR_LIST, "read_list", error);
        errno = error;
        return NULL;
    }
    uint32_t expected;
    memcpy(&expected, json, sizeof(expected));
    if (repair_list_checksum((unsigned char *)json + sizeof(expected), len - sizeof(expected)) != expected ||
        memchr(json + sizeof(expected), 0, len - sizeof(expected)) ||
        !repair_list_within_limits(json + sizeof(expected))) {
        divoom_system_free(json);
        if (invalid) *invalid = 1;
        return NULL;
    }
    const char *parse_end = NULL;
    divoom_cJSON *files = divoom_cJSON_ParseWithOpts(json + sizeof(expected), &parse_end, 1);
    int bad_syntax = !files && parse_end;
    divoom_system_free(json);
    if (!files) {
        if (invalid) *invalid = bad_syntax;
        errno = bad_syntax ? EINVAL : ENOMEM;
        return NULL;
    }
    if (files->type != divoom_cJSON_Array || !files->child) {
        divoom_cJSON_Delete(files);
        if (invalid) *invalid = 1;
        return NULL;
    }
    unsigned count = 0;
    for (divoom_cJSON *item = files->child; item; item = item->next) {
        divoom_cJSON *path = divoom_cJSON_GetObjectItem(item, "path");
        if (++count > REPAIR_PLAN_MAX_FILES || !path || path->type != divoom_cJSON_String ||
            !path->valuestring || !path->valuestring[0] || divoom_update_system_file_restore_path_is_reserved(path->valuestring)) {
            divoom_cJSON_Delete(files);
            if (invalid) *invalid = 1;
            return NULL;
        }
    }
    return files;
}

int divoom_product_test_repair_error_count(void)
{
    divoom_cJSON *files = repair_load_list(NULL);
    int count = files ? divoom_cJSON_GetArraySize(files) : -1;
    divoom_cJSON_Delete(files);
    return count;
}

static int repair_validate_targets(divoom_cJSON *files)
{
    divoom_cJSON *item;
    for (item = files->child; item; item = item->next) {
        divoom_cJSON *p = divoom_cJSON_GetObjectItem(item, "path");
        uint64_t size;
        uint32_t crc;
        /* Validate against the immutable scan baseline first. Missing server
         * entries must not bypass path validation or CRC-ledger protection. */
        if (!p || p->type != divoom_cJSON_String) return 0;
        int rc = divoom_port_dir_manifest_file_info(REPAIR_BASE, "/usr", p->valuestring, &size, &crc);
        if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) return DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT;
        if (rc || divoom_update_system_file_restore_path_is_reserved(p->valuestring)) return 0;
        rc = divoom_port_dir_manifest_file_info(DIVOOM_SYSTEM_FORMAT_MANIFEST,
            "/usr", p->valuestring, &size, &crc);
        if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOT_FOUND) {
            divoom_cJSON_AddNumberToObject(item, "skip", 1);
            if (!divoom_cJSON_GetObjectItem(item, "skip")) return DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT;
            divoom_print("usr_repair: skip absent server entry: %s\n", p->valuestring);
            continue;
        }
        if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) return DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT;
        if (rc != 0 || size > 0x7fffffffU) return 0;
    }
    return 1;
}

static int repair_delete_all_once(divoom_cJSON *files, int resume)
{
    divoom_cJSON *item;
    for (item = files->child; item; item = item->next) {
        char path[512];
        divoom_port_stat_t st;
        uint64_t expected_size;
        uint32_t expected_crc;
        const char *relative = divoom_cJSON_GetObjectItem(item, "path")->valuestring;
        if (snprintf(path, sizeof(path), "/usr/%s", relative) >= sizeof(path)) return 0;
        if (!divoom_cJSON_GetObjectItem(item, "skip")) {
            errno = 0;
            int rc = divoom_port_dir_manifest_file_info(DIVOOM_SYSTEM_FORMAT_MANIFEST, "/usr", relative,
                &expected_size, &expected_crc);
            if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (rc && repair_temporary_errno())) return DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT;
            if (rc != 0) return 0;
            if (resume) {
                errno = 0;
                rc = divoom_port_file_verify(path, expected_size, expected_crc, 0);
                if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (rc && repair_temporary_errno())) return DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT;
                if (!rc) continue;
            }
        }
        errno = 0;
        if (repair_stat(path, &st) != 0) {
            if (errno == ENOENT) continue;
            return repair_temporary_errno() ? DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT : 0;
        }
        if (st.is_dir) return 0;
        if (repair_remove(path) != 0 || divoom_port_sync() != 0)
            return repair_temporary_errno() ? DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT : 0;
        errno = 0;
        if (repair_stat(path, &st) == 0) return 0;
        if (errno != ENOENT)
            return repair_temporary_errno() ? DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT : 0;
    }
    errno = 0;
    if (divoom_port_sync() == 0) return 1;
    return repair_temporary_errno() ? DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT : 0;
}

static int repair_delete_all(divoom_cJSON *files, int resume)
{
    int attempt;
    for (attempt = 0; attempt <= 3; ++attempt) {
        int deleted = repair_delete_all_once(files, resume);
        if (deleted == DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT) return deleted;
        if (deleted) return 1;
        divoom_print("usr_repair: delete/confirm failed attempt=%d/4\n", attempt+1);
        if (attempt < 3) divoom_system_delay_ms(2000);
    }
    return 0;
}

static int repair_read_ready(const char *path)
{
    void *fp;
    char state, extra;
    int ok;
    errno = 0;
    fp = divoom_port_file_open(path, "rb");
    if (!fp) { repair_io_error(path, "ready_open", errno); return errno == ENOENT ? 0 : -1; }
    ok = divoom_port_file_read(&state, 1, 1, fp) == 1 &&
        divoom_port_file_read(&extra, 1, 1, fp) == 0 && !ferror((FILE *)fp);
    if (ferror((FILE *)fp)) repair_io_error(path, "ready_read", errno ? errno : EIO);
    if (divoom_port_file_close(fp) != 0) { ok = 0; repair_io_error(path, "ready_close", errno); }
    return ok && (state == '0' || state == '1') ? (state == '1' ? 2 : 1) : -1;
}

static int repair_write_ready(char state)
{
    void *fp = divoom_port_file_open(ORIGINAL_READY ".tmp", "wb");
    if (!fp) { repair_io_error(ORIGINAL_READY, "ready_create", errno); return 0; }
    errno = 0;
    int ok = divoom_port_file_write(&state, 1, 1, fp) == 1 && divoom_port_file_sync(fp) == 0;
    if (!ok) repair_io_error(ORIGINAL_READY, "ready_write", errno ? errno : EIO);
    if (divoom_port_file_close(fp) != 0) { ok = 0; repair_io_error(ORIGINAL_READY, "ready_close", errno); }
    if (!ok || divoom_port_sync() != 0 || repair_read_ready(ORIGINAL_READY ".tmp") != (state == '1' ? 2 : 1)) {
        repair_io_error(ORIGINAL_READY, "ready_flush_verify", errno); return 0;
    }
    errno = 0;
    if (repair_remove(ORIGINAL_READY) != 0 && errno != ENOENT) return 0;
    int committed = repair_rename(ORIGINAL_READY ".tmp", ORIGINAL_READY) == 0 && divoom_port_sync() == 0;
    if (!committed) repair_io_error(ORIGINAL_READY, "ready_commit", errno);
    return committed;
}

static int repair_recover_ready(void)
{
    errno = 0;
    int rc = repair_manifest(ORIGINAL_LEDGER, "/usr", 0);
    if (rc != 0) {
        if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) errno = ENOMEM;
        if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO) repair_io_error(ORIGINAL_LEDGER, "backup_manifest", errno);
        return rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM ||
            (rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO && errno != ENOENT) ? -1 : 0;
    }
    divoom_port_stat_t st;
    char version = '0';
    errno = 0;
    if (repair_stat(ORIGINAL_VERSION, &st) == 0) {
        char text[129] = {0};
        if (!st.is_dir && st.size > 0 && st.size <= 128) {
            errno = 0;
            void *fp = divoom_port_file_open(ORIGINAL_VERSION, "rb");
            if (!fp) { repair_io_error(ORIGINAL_VERSION, "version_open", errno); return -1; }
            int ok = divoom_port_file_read(text, 1, st.size, fp) == (size_t)st.size && !ferror((FILE *)fp);
            if (ferror((FILE *)fp)) repair_io_error(ORIGINAL_VERSION, "version_read", errno ? errno : EIO);
            if (divoom_port_file_close(fp) != 0) { ok = 0; repair_io_error(ORIGINAL_VERSION, "version_close", errno); }
            if (!ok) return -1;
            errno = 0;
            divoom_cJSON *root = divoom_cJSON_ParseWithOpts(text, NULL, 1);
            divoom_cJSON *item = divoom_cJSON_GetObjectItem(root, "version");
            ok = item && item->type == divoom_cJSON_Number && item->valueint > 0 &&
                item->valuedouble == (double)item->valueint;
            divoom_cJSON_Delete(root);
            if (!ok && errno == ENOMEM) return -1;
            if (ok) version = '1';
        }
        /* A corrupt optional version must not invalidate the usable original
         * CRC backup. Keep its bytes, but rebuild the marker without a version. */
    } else if (errno != ENOENT) { repair_io_error(ORIGINAL_VERSION, "version_stat", errno); return -1; }
    if (!repair_write_ready(version)) return -1;
    divoom_print("usr_repair: rebuilt original ledger ready marker version=%d\n", version == '1');
    return version == '1' ? 2 : 1;
}

static int repair_ledger_ready(void)
{
    int ready = repair_read_ready(ORIGINAL_READY);
    int error = errno;
    if (ready > 0) {
        int rc = repair_manifest(ORIGINAL_LEDGER, "/usr", 0);
        if (rc == 0) return ready;
        if (rc != DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT &&
            !(rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO && errno == ENOENT))
            return -1;
    } else {
        if (ready < 0 && (error == ENOMEM || error == EMFILE || error == ENFILE || error == EACCES || error == ETIMEDOUT))
            return -1;
        divoom_port_stat_t st;
        errno = 0;
        int temp = repair_stat(ORIGINAL_READY ".tmp", &st) == 0;
        if (!temp && errno != ENOENT) return -1;
        if (ready == 0 && !temp) return 0;
        int recovered = repair_recover_ready();
        if (recovered != 0) return recovered;
    }
    /* No trustworthy backup exists. Preserve backup bytes for diagnostics;
     * discard only the invalid control state and use the existing local/base
     * ledger path. Never promote the server's current download guide. */
    errno = 0;
    if (repair_remove(ORIGINAL_READY) != 0 && errno != ENOENT) return -1;
    errno = 0;
    if (repair_remove(ORIGINAL_READY ".tmp") != 0 && errno != ENOENT) return -1;
    int ok = divoom_port_sync() == 0;
    if (!ok) repair_io_error(ORIGINAL_READY, "ready_recovery_sync", errno);
    return ok ? 0 : -1;
}

static int repair_backup_ledger_impl(void)
{
    /* Preserve original accounting across mkfs, without ever promoting the
     * server guide to a local ledger or claiming the server resource version. */
    int ready = repair_ledger_ready();
    char version;
    divoom_port_stat_t st;
    if (ready < 0) return 0;
    if (ready) return ready != 2 || repair_recover_ready() > 0;
    if (repair_manifest(LOCAL_LEDGER, "/usr", 0) != 0) return 1;
    if (!repair_copy_verified(LOCAL_LEDGER, ORIGINAL_LEDGER) ||
        repair_manifest(ORIGINAL_LEDGER, "/usr", 0) != 0) return 0;
    errno = 0;
    version = repair_stat("/usr/file_crc_version.txt", &st) == 0 ? '1' : '0';
    if (version == '0' && errno != ENOENT) return 0;
    if (version == '1' && !repair_copy_verified("/usr/file_crc_version.txt", ORIGINAL_VERSION)) return 0;
    if (divoom_port_sync() != 0) { repair_io_error(ORIGINAL_LEDGER, "backup_sync", errno); return 0; }
    return repair_write_ready(version);
}

static int repair_restore_ledger_impl(void)
{
    int ready = repair_ledger_ready();
    /* Revalidate the optional version immediately before installing it too;
     * a previously committed marker does not prove its payload stayed intact. */
    if (ready != 1 && ready >= 0) ready = repair_recover_ready();
    if (ready < 0) return 0;
    if (!ready) return 1;
    if (!repair_copy_verified(ORIGINAL_LEDGER, LOCAL_LEDGER) ||
        repair_manifest(LOCAL_LEDGER, "/usr", 0) != 0) return 0;
    if (ready == 2 &&
        !repair_copy_verified(ORIGINAL_VERSION, "/usr/file_crc_version.txt")) return 0;
    int ok = divoom_port_sync() == 0;
    if (!ok) repair_io_error(LOCAL_LEDGER, "restore_sync", errno);
    return ok;
}

static int repair_cleanup_impl(void)
{
    const char *paths[] = { REPAIR_LIST, REPAIR_LIST ".tmp", REPAIR_BASE, ORIGINAL_LEDGER, ORIGINAL_VERSION, ORIGINAL_READY, ORIGINAL_READY ".tmp", DIVOOM_SYSTEM_FORMAT_MANIFEST, "/userdata/.usr_base_crc.bin", "/userdata/.usr_base_crc.bin.tmp" };
    int i;
    divoom_port_stat_t st;
    /* No request is active when completion/cancellation reaches cleanup. Keep
     * the durable state on failure so this dedicated tree can be retried. */
    errno = 0;
    if (repair_stat("/userdata/.format_restore", &st) == 0) {
        if (divoom_port_remove_tree("/userdata/.format_restore") != 0 || divoom_port_sync() != 0) { repair_io_error("/userdata/.format_restore", "cleanup_tree", errno); return 0; }
    } else if (errno != ENOENT) return 0;
    errno = 0;
    if (repair_stat("/userdata/.format_restore", &st) == 0 || errno != ENOENT) return 0;
    for (i = 0; i < (int)(sizeof(paths)/sizeof(paths[0])); ++i) {
        errno = 0;
        if (repair_remove(paths[i]) != 0 && errno != ENOENT) { repair_io_error(paths[i], "cleanup_remove", errno); return 0; }
    }
    int ok = divoom_port_sync() == 0;
    if (!ok) repair_io_error(REPAIR_LIST, "cleanup_sync", errno);
    return ok;
}

int divoom_product_test_repair_run(int (*network)(void), int (*retry)(int),
    divoom_system_file_restore_progress progress)
{
    divoom_product_test_repair_io_error_target(1);
    repair_scan scan = {0};
    divoom_cJSON *item;
    int result = 0, pending = divoom_product_test_recovery_state() == 'P';
    const char *stage = "prepare_baseline";
    scan.progress = progress;
    errno = 0;
    if (!pending) {
        const char *base = LOCAL_LEDGER;
        divoom_product_test_format_set_error_files(0);
        divoom_product_test_format_set_phase(DIVOOM_PT_FORMAT_CHECK);
        int prepared = repair_manifest(base, "/usr", 0);
        if (prepared == DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT ||
            (prepared == DIVOOM_PORT_DIR_COMPARE_ERR_IO && errno == ENOENT)) {
            divoom_product_test_repair_plan plan = {.check_system_files = 1};
            if (!divoom_product_test_format_allow_repair(&plan)) goto out;
            if (!divoom_product_test_repair_restore_ledger()) goto out;
            prepared = repair_manifest(base, "/usr", 0);
            if (prepared == DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT ||
                (prepared == DIVOOM_PORT_DIR_COMPARE_ERR_IO && errno == ENOENT)) {
                if (!network()) goto out;
                /* Only a proven corrupt ledger may be replaced. Download the
                 * original baseline, not the server's current resource guide. */
                if (prepared == DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT &&
                    (repair_remove(base) != 0 || divoom_port_sync() != 0)) goto out;
                divoom_product_test_format_set_phase(DIVOOM_PT_FORMAT_RESTORE);
                int restored = divoom_update_system_file_ensure_original_ledger(retry);
                if (restored == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT) { result = 2; goto out; }
                if (restored == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT) { result = restored; goto out; }
                if (restored != 1) goto out;
                divoom_product_test_format_set_phase(DIVOOM_PT_FORMAT_CHECK);
                prepared = repair_manifest(base, "/usr", 0);
            }
        }
        if (prepared == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) { scan.temporary = 1; goto out; }
        if (prepared != 0) goto out;
        /* Snapshot the original expectations; never write the local ledger. */
        errno = 0;
        if (!repair_copy_verified(base, REPAIR_BASE)) goto out;
        scan.files = divoom_cJSON_CreateArray();
        if (!scan.files) { scan.temporary = 1; goto out; }
        scan.collect = 1;
        stage = "check_files";
        uint32 scan_started = divoom_system_get_tickms();
        if (repair_scan_manifest(REPAIR_BASE, &scan) != 0) goto out;
        divoom_print("usr_check: scan checked=%d/%d bad=%d elapsed_ms=%u\n",
            scan.done, scan.total, scan.failed,
            (unsigned)(divoom_system_get_tickms() - scan_started));
        if (!scan.failed) { result = 1; goto out; }
        if (scan.plan_full) {
            divoom_print("usr_repair: partial repair plan limit exceeded bad=%d max_files=%d max_bytes=%d\n",
                scan.failed, REPAIR_PLAN_MAX_FILES, REPAIR_PLAN_MAX_BYTES);
            result = 2;
            goto out;
        }
        divoom_product_test_repair_plan plan = {.download_files = scan.failed};
        if (!divoom_product_test_format_allow_repair(&plan)) goto out;
        /* Commit the completed scan before any network/download operation.
         * Retrying or rebooting now resumes this list instead of rescanning. */
        stage = "save_repair_list";
        errno = 0;
        if (!repair_save_list(scan.files, scan.plan_bytes)) {
            if (errno == EFBIG) result = 2;
            goto out;
        }
        stage = "save_pending_state";
        if (!divoom_product_test_recovery_mark('P')) goto out;
    } else {
        stage = "load_repair_list";
        int invalid;
        scan.files = repair_load_list(&invalid);
        divoom_product_test_format_set_error_files(scan.files ? divoom_cJSON_GetArraySize(scan.files) : -1);
        if (scan.files) {
            int baseline = repair_manifest(REPAIR_BASE, "/usr", 0);
            invalid = baseline == DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT ||
                (baseline == DIVOOM_PORT_DIR_COMPARE_ERR_IO && errno == ENOENT);
            if (baseline == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) scan.temporary = 1;
            if (baseline != 0 && !invalid) goto out;
        }
        if (invalid) {
            /* A fresh CRC scan includes files already removed before power loss.
             * Publish C before replacing the disposable plan and snapshot. */
            divoom_print("usr_repair: invalid saved repair plan; rebuilding from original ledger\n");
            divoom_product_test_recovery_mark('C');
            goto out;
        }
        if (!scan.files) goto out;
    }
    stage = "connect_network";
    if (!network()) goto out;
    stage = "download_guide";
    divoom_product_test_format_set_phase(DIVOOM_PT_FORMAT_GUIDE);
    if (!divoom_update_system_file_ensure_format_manifest(retry)) goto out;
    stage = "validate_targets";
    int targets = repair_validate_targets(scan.files);
    if (targets == DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT) { scan.temporary = 1; goto out; }
    if (!targets) goto out;
#if defined(CONFIG_DRIVERS_AIC8800D80)
    for (item = scan.files->child; item; item = item->next) {
        const char *path = divoom_cJSON_GetObjectItem(item, "path")->valuestring;
        if (!strcasecmp(path, DIVOOM_WIFI_FW_SOURCE + sizeof("/usr/") - 1) &&
            divoom_product_test_wifi_fw_has_copy(1) != 0 && !divoom_product_test_wifi_fw_safe_to_erase(1)) {
            stage = "preserve_wifi_firmware";
            scan.temporary = 1;
            goto out;
        }
    }
#endif
    /* All bad paths are durable before the first unlink. A resumed task can
     * safely repeat deletion; no file outside this list is downloaded. */
    stage = "delete_files";
    int deleted = repair_delete_all(scan.files, pending);
    if (deleted == DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT) { scan.temporary = 1; goto out; }
    if (!deleted) { result = 2; goto out; }
    divoom_product_test_format_set_phase(DIVOOM_PT_FORMAT_RESTORE);
    scan.total = divoom_cJSON_GetArraySize(scan.files);
    scan.done = 0;
    stage = "download_files";
    for (item = scan.files->child; item; item = item->next) {
        const char *path = divoom_cJSON_GetObjectItem(item, "path")->valuestring;
        uint64_t size;
        uint32_t crc;
        char local[512];
        int errors = 0, local_errors = 0, userdata_errors = 0;
        if (divoom_cJSON_GetObjectItem(item, "skip")) {
            ++scan.done;
            if (progress) progress(scan.done, scan.total, -1);
            continue;
        }
        if (snprintf(local, sizeof(local), "/usr/%s", path) >= sizeof(local)) goto out;
        int info = divoom_port_dir_manifest_file_info(DIVOOM_SYSTEM_FORMAT_MANIFEST, "/usr", path, &size, &crc);
        if (info == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) { scan.temporary = 1; goto out; }
        if (info) goto out;
        for (;;) {
            int downloaded = 1;
            errno = 0;
            int verified = divoom_port_file_verify(local, size, crc, 0);
            if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (verified && (errno == ENOMEM || errno == EMFILE || errno == ENFILE))) {
                scan.temporary = 1; goto out;
            }
            if (verified != 0)
                downloaded = divoom_update_system_file_restore_one(path, size, crc,
                    scan.done, scan.total, progress, retry);
            if (downloaded == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT) { result = 2; goto out; }
            if (downloaded == DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT) { result = downloaded; goto out; }
            if (downloaded == DIVOOM_SYSTEM_FILE_RESTORE_USERDATA_RETRY) {
                if (++userdata_errors >= 4) { result = DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT; goto out; }
                downloaded = 0;
            }
            if (downloaded == DIVOOM_SYSTEM_FILE_RESTORE_LOCAL_RETRY) {
                if (++local_errors >= 4) { result = 2; goto out; }
                downloaded = 0;
            }
            if (downloaded == DIVOOM_SYSTEM_FILE_RESTORE_SKIPPED) {
                divoom_cJSON_AddNumberToObject(item, "skip", 1);
                if (!divoom_cJSON_GetObjectItem(item, "skip")) { scan.temporary = 1; goto out; }
                divoom_print("usr_repair: skip HTTP 404: %s\n", path);
                break;
            }
            if (downloaded) {
                errno = 0;
                verified = repair_sync(local) ? divoom_port_file_verify(local, size, crc, 0) : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
                if (!verified) break;
                if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (errno == ENOMEM || errno == EMFILE || errno == ENFILE)) {
                    scan.temporary = 1; goto out;
                }
                divoom_print("usr_repair: repaired final file failed verification; request system format path=%s\n", local);
                result = 2; goto out;
            } else if ((errno == ENOMEM || errno == EMFILE || errno == ENFILE)) { scan.temporary = 1; goto out; }
            /* A generic downloader stop (OOM/transport/cancellation) is not disk damage. */
            if (errors < 0x7fffffff) ++errors;
            if (retry && !retry(errors)) goto out;
            divoom_system_delay_ms(10000);
        }
        ++scan.done;
        if (progress) progress(scan.done, scan.total, 100);
    }
    /* The scan is immutable. Verify only repaired targets, never restart the
     * whole old-manifest check because a server entry/download was missing. */
    stage = "verify_repaired";
    for (item = scan.files->child; item; item = item->next) {
        char path[512];
        const char *relative = divoom_cJSON_GetObjectItem(item, "path")->valuestring;
        uint64_t size;
        uint32_t crc;
        if (divoom_cJSON_GetObjectItem(item, "skip")) continue;
        if (snprintf(path, sizeof(path), "/usr/%s", relative) >= sizeof(path)) goto out;
        int info = divoom_port_dir_manifest_file_info(DIVOOM_SYSTEM_FORMAT_MANIFEST, "/usr", relative, &size, &crc);
        if (info == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM) { scan.temporary = 1; goto out; }
        if (info) goto out;
        for (;;) {
            errno = 0;
            int verified = repair_sync(path) ? divoom_port_file_verify(path, size, crc, 0) : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
            if (!verified) break;
            if (verified == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM || (errno == ENOMEM || errno == EMFILE || errno == ENFILE)) {
                scan.temporary = 1; goto out;
            }
            divoom_print("usr_repair: repaired file failed final recheck; request system format path=%s\n", path);
            result = 2; goto out;
        }
    }
    result = 1;
    for (item = scan.files->child; item; item = item->next)
        if (divoom_cJSON_GetObjectItem(item, "skip")) {
            result = DIVOOM_PRODUCT_TEST_REPAIR_INCOMPLETE;
            break;
        }
out:
    if (result == 0 && (scan.temporary || (errno == ENOMEM || errno == EMFILE || errno == ENFILE))) result = DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT;
    if (result == 0 || result == DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT) {
        divoom_print("usr_repair: failed stage=%s pending=%d checked=%d/%d bad=%d\n",
            stage, pending, scan.done, scan.total, scan.failed);
    }
    if (result == 0 && (!strcmp(stage, "save_repair_list") || !strcmp(stage, "save_pending_state") ||
        !strcmp(stage, "load_repair_list") || !strcmp(stage, "validate_targets")))
        repair_io_error(REPAIR_LIST, stage, errno);
    if (result != 0) divoom_product_test_repair_io_error_target(1);
    divoom_port_file_verifier_destroy(scan.verifier);
    divoom_cJSON_Delete(scan.files);
    return result;
}

int divoom_product_test_repair_backup_ledger(void)
{
    divoom_product_test_repair_io_error_target(1);
    int ok = repair_backup_ledger_impl();
    if (ok) divoom_product_test_repair_io_error_target(1);
    return ok;
}

int divoom_product_test_repair_restore_ledger(void)
{
    divoom_product_test_repair_io_error_target(1);
    int ok = repair_restore_ledger_impl();
    if (ok) divoom_product_test_repair_io_error_target(1);
    return ok;
}

int divoom_product_test_repair_cleanup(void)
{
    divoom_product_test_repair_io_error_target(1);
    int ok = repair_cleanup_impl();
    if (ok) divoom_product_test_repair_io_error_target(1);
    return ok;
}
