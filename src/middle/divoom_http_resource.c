#if !defined(_WIN32)
#include <sys/errno.h>
#endif
#include "divoom.h"
#include <errno.h>
#include "divoom_http_client.h"
#include "divoom_port_fs.h"

/* Only explicit process resource pressure is transient; a backend's
 * already-classified storage result must not depend on a stale errno. */
static int divoom_http_resource_local_error(void)
{
    return errno == ENOMEM || errno == EMFILE || errno == ENFILE ? -5 : -4;
}

static int divoom_http_resource_matches(const char *path, uint32 size, uint32 crc)
{
    uint64_t actual_size;
    uint32_t actual_crc;
    int rc;
    errno = 0;
    rc = divoom_port_file_crc32(path, &actual_size, &actual_crc);
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM ||
        (rc != 0 && divoom_http_resource_local_error() == -5)) return -1;
    return rc == 0 && actual_size == size && actual_crc == crc;
}

/* Use the same identity for resume and exact cleanup after installation. */
static int divoom_http_resource_identity(const char *path, const char *url,
    uint32 expected_size, uint32 expected_crc, char *identity, size_t capacity,
    char *meta_path, size_t meta_capacity)
{
    uint32 identity_hash = 2166136261U;
    int i, length;
    if (!path || !url || expected_size > 0x7fffffffU) return 0;
    length = snprintf(identity, capacity, "RESOURCE-RESUME-1\n%s\n%s\n%u\n%08x\n",
        url, path, expected_size, expected_crc);
    if (length <= 0 || (size_t)length >= capacity) return 0;
    for (i = 0; i < length; ++i)
        identity_hash = (identity_hash ^ (unsigned char)identity[i]) * 16777619U;
    if (snprintf(meta_path, meta_capacity, "/userdata/.resource_resume/%08x.meta", identity_hash) >= meta_capacity)
        return 0;
    return length;
}

int divoom_http_client_resource_file_supported(const char *path, const char *url,
    uint32 size, uint32 crc)
{
    char identity[384], meta_path[320];
    return divoom_http_resource_identity(path, url, size, crc, identity, sizeof(identity),
        meta_path, sizeof(meta_path)) > 0;
}

int divoom_http_client_resource_file_owned(const char *path, const char *url,
    uint32 size, uint32 crc)
{
    char identity[384], meta_path[320], saved[384];
    int length = divoom_http_resource_identity(path, url, size, crc, identity,
        sizeof(identity), meta_path, sizeof(meta_path));
    if (!length) return 0;
    errno = 0;
    void *fp = divoom_port_file_open(meta_path, "rb");
    if (!fp) return errno == ENOENT ? 0 : -1;
    size_t count = divoom_port_file_read(saved, 1, sizeof(saved), fp);
    int error = ferror((FILE *)fp) ? (errno ? errno : EIO) : 0;
    errno = 0;
    if (divoom_port_file_close(fp) != 0 && !error) error = errno ? errno : EIO;
    if (error) { errno = error; return -1; }
    return count == (size_t)length && !memcmp(saved, identity, count);
}

static int divoom_http_resource_remove(const char *path, int *changed)
{
    divoom_port_stat_t st;
    errno = 0;
    if (divoom_port_stat(path, &st) != 0) return errno == ENOENT;
    if (st.is_dir || divoom_port_remove_file(path) != 0) return 0;
    *changed = 1;
    errno = 0;
    return divoom_port_stat(path, &st) != 0 && errno == ENOENT;
}

int divoom_http_client_cleanup_resource_file(const char *path, const char *url,
    uint32 size, uint32 crc)
{
    char identity[384], meta_path[320], saved[384];
    divoom_port_stat_t st;
    int changed = 0, length, own_meta = 0;
    void *fp;
    size_t count;
    length = divoom_http_resource_identity(path, url, size, crc, identity, sizeof(identity),
        meta_path, sizeof(meta_path));
    if (!length) return 0;
    errno = 0;
    fp = divoom_port_file_open(meta_path, "rb");
    if (fp) {
        errno = 0;
        count = divoom_port_file_read(saved, 1, sizeof(saved), fp);
        int error = ferror((FILE *)fp) ? (errno ? errno : EIO) : 0;
        errno = 0;
        if (divoom_port_file_close(fp) != 0 && !error) error = errno ? errno : EIO;
        if (error) { errno = error; return 0; }
        /* A hash collision belongs to a different request; preserve its data. */
        own_meta = count == (size_t)length && !memcmp(saved, identity, count);
    } else if (errno != ENOENT) return 0;
    if (!divoom_http_resource_remove(path, &changed) ||
        (own_meta && !divoom_http_resource_remove(meta_path, &changed)) ||
        (changed && divoom_port_sync() != 0)) return 0;
    errno = 0;
    if (divoom_port_stat(path, &st) == 0 || errno != ENOENT) return 0;
    if (!own_meta) return 1;
    errno = 0;
    return divoom_port_stat(meta_path, &st) != 0 && errno == ENOENT;
}

static int divoom_http_resource_attempt(const char *path, const char *url,
    uint32 expected_size, uint32 expected_crc,
    divoom_http_client_down_file_callback *progress, void *user_data, int checked)
{
    char meta_path[320], identity[384], saved[384];
    void *fp;
    long offset;
    int length, matches = 0, rc, changed = 0;
    uint64_t actual_size;
    uint32_t actual_crc;
    size_t count;

    length = divoom_http_resource_identity(path, url, expected_size, expected_crc,
        identity, sizeof(identity), meta_path, sizeof(meta_path));
    if (!length) return 0;
    errno = 0;
    if (divoom_port_mkdir_p("/userdata/.resource_resume") != 0) return divoom_http_resource_local_error();
    errno = 0;
    fp = divoom_port_file_open(meta_path, "rb");
    if (fp) {
        int failed, saved_errno;
        errno = 0;
        count = divoom_port_file_read(saved, 1, sizeof(saved), fp);
        failed = ferror((FILE *)fp);
        saved_errno = errno;
        errno = 0;
        if (divoom_port_file_close(fp) != 0) {
            if (!failed) saved_errno = errno;
            failed = 1;
        }
        if (failed) { errno = saved_errno; return divoom_http_resource_local_error(); }
        matches = count == length && memcmp(saved, identity, count) == 0;
    } else if (errno != ENOENT) return divoom_http_resource_local_error();
    /* A verified complete stage is reusable even after its resume record was
     * removed on the preceding download. Check it before discarding prefixes. */
    rc = divoom_http_resource_matches(path, expected_size, expected_crc);
    if (rc < 0) return -5; /* Keep valid/partial bytes during CRC allocation pressure. */
    if (rc == 1 && (!checked || matches)) {
        if (!checked && matches && !divoom_http_resource_remove(meta_path, &changed)) return divoom_http_resource_local_error();
        if (progress) progress(user_data, expected_size, expected_size);
        return 1;
    }
    /* 清单身份变化时丢弃旧前缀，禁止拼接不同 URL/大小/CRC 的资源。 */
    if (!matches) {
        if ((rc != 1 && !divoom_http_resource_remove(path, &changed)) ||
            !divoom_http_resource_remove(meta_path, &changed)) {
            return divoom_http_resource_local_error();
        }
        int saved_errno;
        errno = 0;
        fp = divoom_port_file_open(meta_path, "wb");
        if (!fp) return divoom_http_resource_local_error();
        errno = 0;
        matches = divoom_port_file_write(identity, 1, length, fp) == length;
        saved_errno = errno;
        errno = 0;
        if (divoom_port_file_close(fp) != 0) {
            if (matches) saved_errno = errno;
            matches = 0;
        }
        if (!matches) { errno = saved_errno; return divoom_http_resource_local_error(); }
        errno = 0;
        if (divoom_port_sync() != 0) return divoom_http_resource_local_error();
    }
    if (rc == 1) {
        if (progress) progress(user_data, expected_size, expected_size);
        return 1;
    }
    /* A new maintenance content attempt must not reuse previously bad bytes.
     * Network retries inside this same call retain ordinary Range resume. */
    if (checked == 1 && !divoom_http_resource_remove(path, &changed))
        return divoom_http_resource_local_error();
    errno = 0;
    offset = divoom_port_file_size(path);
    if (offset < 0) {
        if (errno != ENOENT) return divoom_http_resource_local_error();
        offset = 0;
    }
    if (offset >= expected_size) {
        if (!divoom_http_resource_remove(path, &changed)) {
            return divoom_http_resource_local_error();
        }
        offset = 0;
    }
    divoom_print("resource_http: download offset=%ld size=%u url=%s\n", offset, expected_size, url);
    if (progress) progress(user_data, (uint32)offset, expected_size);
    rc = divoom_http_client_resource_part(url, path, (uint32)offset, expected_size, progress, user_data);
    if (rc == -6 || (rc == -1 && divoom_http_client_last_http_status() == 404)) {
        divoom_print("resource_http: skip HTTP 404 url=%s\n", url);
        return 2; /* Missing server file: handled separately from verified data. */
    }
    if (rc == -2) return -4; /* Backend explicitly classified storage. */
    errno = 0;
    if (divoom_port_sync() != 0) return divoom_http_resource_local_error();
    if (rc != 0) {
        /* 网络中断保留已落盘前缀；协议异常不能作为下一次续传基础。 */
        if (rc == -3) {
            if (!divoom_http_resource_remove(path, &changed)) return divoom_http_resource_local_error();
        }
        divoom_print("resource_http: interrupted rc=%d saved=%ld\n", rc, divoom_port_file_size(path));
        return rc == -5 ? -3 : rc == -4 ? -2 : 0;
    }
    errno = 0;
    rc = divoom_port_file_crc32(path, &actual_size, &actual_crc);
    if (rc != 0) {
        return rc == DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM ? -5 : divoom_http_resource_local_error();
    }
    if (actual_size != expected_size || actual_crc != expected_crc) {
        /* Keep maintenance ownership through deferred rescue/cleanup, so an
         * interrupted run can safely reclaim its bounded alternate names. */
        if (!checked && (!divoom_http_resource_remove(path, &changed) ||
            !divoom_http_resource_remove(meta_path, &changed))) {
            return divoom_http_resource_local_error();
        }
        if (progress) progress(user_data, 0, expected_size);
        return checked ? DIVOOM_HTTP_RESOURCE_CONTENT_MISMATCH : -1;
    }
    if (!checked && !divoom_http_resource_remove(meta_path, &changed)) return divoom_http_resource_local_error();
    return 1;
}

static int divoom_http_resource_download(const char *path, const char *url,
    uint32 expected_size, uint32 expected_crc, int (*retry)(int errors),
    divoom_http_client_down_file_callback *progress, void *user_data, int checked)
{
    int errors = 0, storage_errors = 0, rc, checked_attempt = checked;
    if (retry && !retry(0)) return 0;
    for (;;) {
        rc = divoom_http_resource_attempt(path, url, expected_size, expected_crc, progress, user_data, checked_attempt);
        if (checked && (rc == 0 || rc == -2 || rc == -3)) checked_attempt = 2;
        if (rc == DIVOOM_HTTP_RESOURCE_CONTENT_MISMATCH) return rc;
        if (rc == 2) return 2;
        if (rc == 1) return !retry || retry(0);
        /* Maintenance may free the old target before retrying an alternate
         * destination; surface ENOSPC immediately rather than filling again. */
        if (checked && rc == -4 && errno == ENOSPC) return -1;
        /* Transport/CRC failures keep the unlimited HTTP retry policy. A
         * local create/write/read/sync failure has an initial attempt + 3 retries. */
        if ((rc == 0 || rc == -3 || rc == -4 || rc == -5) && !retry) return 0;
        if (rc == -4 && ++storage_errors >= 4) {
            int saved_errno = errno;
            divoom_print("resource_http: local storage failed attempts=4 path=%s; request format\n", path);
            errno = saved_errno;
            return -1;
        }
        if (errors < 0x7fffffff) ++errors;
        divoom_print("resource_http: Error Times:%d reason=%s url=%s\n",
            errors, rc == -5 ? "temporary resource/memory error" : rc == -4 ? "local storage error" : rc == -1 ? "CRC mismatch" : rc == -3 ? "network interrupted" : rc == 0 ? "HTTP/file error" : "temporary network error", url);
        if (retry && !retry(errors)) {
            return 0;
        }
        divoom_system_delay_ms(10000);
    }
}

int divoom_http_client_down_resource_file(const char *path, const char *url,
    uint32 expected_size, uint32 expected_crc, int (*retry)(int errors),
    divoom_http_client_down_file_callback *progress, void *user_data)
{
    return divoom_http_resource_download(path, url, expected_size, expected_crc, retry,
        progress, user_data, 0);
}

int divoom_http_client_down_resource_file_checked(const char *path, const char *url,
    uint32 expected_size, uint32 expected_crc, int (*retry)(int errors),
    divoom_http_client_down_file_callback *progress, void *user_data)
{
    return divoom_http_resource_download(path, url, expected_size, expected_crc, retry,
        progress, user_data, 1);
}
