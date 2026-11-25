/* Durable upload queue: concatenated, independently CRC-checked crash records. */
#include <divoom_crash_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#ifndef CRASH_UPLOAD_DIR
#define CRASH_UPLOAD_DIR "/userdata/crash"
#endif
#define BUNDLE CRASH_UPLOAD_DIR "/upload.bin"
#define TEMP BUNDLE ".tmp"
#define PREVIOUS BUNDLE ".prev"
#define RECORD_MAX (512 + 4096)

struct pending_file {
    struct pending_file *next;
    time_t modified;
    char path[160];
};

static int read_exact(int fd, void *data, size_t bytes)
{
    char *p = data;
    while (bytes) {
        ssize_t n = read(fd, p, bytes);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        p += n; bytes -= n;
    }
    return 0;
}

/* 0=EOF, positive=one complete record, -1=I/O or invalid/truncated record. */
static int read_record(int fd, void *buffer, uint32_t *version)
{
    uint32_t length;
    ssize_t n;
    do { n = read(fd, buffer, 1); } while (n < 0 && errno == EINTR);
    if (n <= 0) return n;
    if (read_exact(fd, (char *)buffer + 1, 511)) return -1;
    memcpy(&length, (char *)buffer + 8, sizeof(length));
    if (length > RECORD_MAX - 512 || read_exact(fd, (char *)buffer + 512, length) ||
        divoom_crash_log_record_info(buffer, 512 + length, version)) return -1;
    return 512 + length;
}

static int write_record(int fd, const void *buffer, size_t bytes)
{
    const char *p = buffer;
    while (bytes) {
        ssize_t n = write(fd, p, bytes);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        p += n; bytes -= n;
    }
    return 0;
}

/* Validate the entire bundle, optionally copy it or locate an exact record. */
static int scan_bundle(const char *path, void *buffer, int output,
                       const void *match, int match_size, uint32_t *first_version)
{
    uint32_t version;
    int fd = open(path, O_RDONLY), n, count = 0, found = 0;
    if (fd < 0) return errno == ENOENT ? 0 : -1;
    while ((n = read_record(fd, buffer, &version)) > 0) {
        if (!count && first_version) *first_version = version;
        ++count;
        if (match && n == match_size && !memcmp(buffer, match, n)) found = 1;
        if (output >= 0 && write_record(output, buffer, n)) { n = -1; break; }
    }
    if (close(fd) || n < 0 || !count) return -1;
    return match ? found : count;
}

static int load_file(const char *path, void *buffer)
{
    uint32_t version;
    char extra;
    int fd = open(path, O_RDONLY), n;
    if (fd < 0) return -1;
    n = read_record(fd, buffer, &version);
    if (n <= 0 || read(fd, &extra, 1) != 0) n = -1;
    if (close(fd)) n = -1;
    return n;
}

static int list_files(struct pending_file **list)
{
    DIR *dir = opendir(CRASH_UPLOAD_DIR);
    struct dirent *entry;
    int ret = -1;
    if (!dir) return -1;
    for (;;) {
        struct stat st;
        struct pending_file *item, **at;
        uint32_t format[2];
        int fd, checked;
        size_t len;
        errno = 0;
        entry = readdir(dir);
        if (!entry) { ret = errno ? -1 : 0; break; }
        len = strlen(entry->d_name);
        if (len < 10 || strncmp(entry->d_name, "crash-", 6) ||
            strcmp(entry->d_name + len - 4, ".bin")) continue;
        item = malloc(sizeof(*item));
        if (!item) break;
        if (snprintf(item->path, sizeof(item->path), "%s/%s", CRASH_UPLOAD_DIR,
                     entry->d_name) >= (int)sizeof(item->path) || stat(item->path, &st) ||
            !S_ISREG(st.st_mode)) { free(item); break; }
        item->modified = st.st_mtime;
        fd = open(item->path, O_RDONLY);
        if (fd < 0) { free(item); break; }
        checked = read_exact(fd, format, sizeof(format));
        if (close(fd)) checked = -1;
        if (checked) { free(item); break; }
        if (format[0] == 0x31524344U && format[1] != 4) {
            /* Old formats have no firmware version and are outside this queue. */
            free(item); continue;
        }
        /* Oldest exported file first; stable filename order if RTC was unset. */
        for (at = list; *at && ((*at)->modified < item->modified ||
             ((*at)->modified == item->modified && strcmp((*at)->path, item->path) < 0));
             at = &(*at)->next) {}
        item->next = *at; *at = item;
    }
    if (closedir(dir)) ret = -1;
    return ret;
}

int divoom_crash_upload_prepare(uint32_t *version)
{
    struct pending_file *list = NULL, *item;
    struct stat st;
    void *record = NULL, *scratch = NULL;
    int fd = -1, result = -1, count, n, existing, contains;
    if (!version || !divoom_crash_log_available()) return 0;
    *version = 0;
    /* Export verifies each normal file before releasing its raw journal slot. */
    if (divoom_crash_log_export()) return -1;
    record = malloc(RECORD_MAX); scratch = malloc(RECORD_MAX);
    if (!record || !scratch) goto out;
    /* Recover a reset between the two renames; never discard the only copy. */
    if (stat(PREVIOUS, &st) == 0) {
        existing = scan_bundle(BUNDLE, scratch, -1, NULL, 0, version);
        if (existing < 0) goto out;
        if (!existing) {
            if (scan_bundle(PREVIOUS, scratch, -1, NULL, 0, version) <= 0 ||
                rename(PREVIOUS, BUNDLE)) goto out;
        } else if (unlink(PREVIOUS)) goto out;
    } else if (errno != ENOENT) goto out;
    if (list_files(&list)) goto out;
    existing = scan_bundle(BUNDLE, scratch, -1, NULL, 0, version);
    if (existing < 0) goto out;
    if (!list) { result = existing ? 1 : 0; goto out; }
    fd = open(TEMP, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) goto out;
    count = scan_bundle(BUNDLE, scratch, fd, NULL, 0, version);
    if (count < 0) goto out;
    for (item = list; item; item = item->next) {
        n = load_file(item->path, record);
        if (n < 0) goto out;
        contains = scan_bundle(BUNDLE, scratch, -1, record, n, NULL);
        if (contains < 0) goto out;
        if (!contains) {
            if (write_record(fd, record, n)) goto out;
            ++count;
        }
    }
    result = -1;
    if (fsync(fd)) goto out;
    n = close(fd); fd = -1;
    if (n || scan_bundle(TEMP, scratch, -1, NULL, 0, version) != count) goto out;
    /* FAT rename cannot replace an existing file. Keep a recoverable backup. */
    if (existing && rename(BUNDLE, PREVIOUS)) goto out;
    if (rename(TEMP, BUNDLE)) goto out;
    if (scan_bundle(BUNDLE, scratch, -1, NULL, 0, version) != count) goto out;
    if (existing && unlink(PREVIOUS)) goto out;
    for (item = list; item; item = item->next) {
        n = load_file(item->path, record);
        if (n < 0 || scan_bundle(BUNDLE, scratch, -1, record, n, NULL) != 1 ||
            unlink(item->path)) goto out;
    }
    printf("[crash] upload bundle ready: %d records, first firmware=%lu\n",
           count, (unsigned long)*version);
    result = 1;
out:
    if (fd >= 0) close(fd);
    while (list) { item = list->next; free(list); list = item; }
    free(record); free(scratch);
    if (result < 0) printf("[crash] bundle preparation failed; records retained\n");
    return result;
}

int divoom_crash_upload_finish(void)
{
    if (!divoom_crash_log_available()) return -1;
    return unlink(BUNDLE);
}
