#if !defined(_WIN32)
#include <sys/errno.h>
#endif
#include "divoom.h"
#include "divoom_device_error.h"
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_port_fs.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define ERROR_COUNT DIVOOM_SD_NAND_ERR_UBOOT_BACK
#define ERROR_MASK ((1U << ERROR_COUNT) - 1U)
#define ERROR_MAGIC 0x31524544U
static const char *error_paths[2] = {"/userdata/.device_errors.0", "/userdata/.device_errors.1"};
typedef struct { uint32_t magic, sequence, mask, crc; } error_record;
static pthread_mutex_t error_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t error_format_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned pending, retired;
static int loaded, dirty, media_ready, formatting;
static int active_slot = -1;
static uint32 sequence;

static uint32_t record_crc(const error_record *record)
{
    const unsigned char *p = (const unsigned char *)record;
    uint32_t crc = 0xffffffffU;
    for (unsigned i = 0; i < 12; ++i) {
        crc ^= p[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (crc & 1U ? 0xedb88320U : 0U);
    }
    return crc ^ 0xffffffffU;
}

/* 1 valid, 0 absent, 2 corrupt/truncated, -1 I/O error. */
static int read_slot(int slot, error_record *record)
{
    errno = 0;
    void *file = divoom_port_file_open(error_paths[slot], "rb");
    if (!file) return errno == ENOENT ? 0 : -1;
    size_t count = divoom_port_file_read(record, 1, sizeof(*record), file);
    unsigned char extra;
    size_t tail = divoom_port_file_read(&extra, 1, 1, file);
    int io_error = ferror((FILE *)file);
    if (divoom_port_file_close(file)) io_error = 1;
    if (io_error) return -1;
    if (count != sizeof(*record) || tail || record->magic != ERROR_MAGIC ||
        record->mask > ERROR_MASK || record->crc != record_crc(record)) return 2;
    return 1;
}

/* Caller owns error_lock; no file operation before mount or during mkfs. */
static int load_locked(void)
{
    error_record records[2];
    if (!media_ready || formatting) return 0;
    if (loaded) return 1;
    int a = read_slot(0, &records[0]), b = read_slot(1, &records[1]);
    /* An unreadable copy may be newer: do not truncate it on the next save. */
    if (a < 0 || b < 0) return 0;
    active_slot = a == 1 ? 0 : b == 1 ? 1 : -1;
    if (a == 1 && b == 1 && (int32_t)(records[1].sequence - records[0].sequence) > 0)
        active_slot = 1;
    sequence = active_slot >= 0 ? records[active_slot].sequence : 0;
    if (active_slot >= 0) pending |= records[active_slot].mask & ~retired;
    else if (a == 2 || b == 2) dirty = 1;
    loaded = 1;
    return 1;
}

static int save_locked(void)
{
    if (!load_locked()) return 0;
    if (!dirty) return 1;
    int next = active_slot == 0 ? 1 : 0;
    error_record record = { ERROR_MAGIC, sequence + 1U, pending, 0 }, verify;
    record.crc = record_crc(&record);
    void *file = divoom_port_file_open(error_paths[next], "wb");
    if (!file) return 0;
    int ok = divoom_port_file_write(&record, 1, sizeof(record), file) == sizeof(record);
    if (ok && divoom_port_file_sync(file)) ok = 0;
    if (divoom_port_file_close(file)) ok = 0;
    if (!ok || divoom_port_sync() != 0 || read_slot(next, &verify) != 1 ||
        memcmp(&record, &verify, sizeof(record))) return 0;
    active_slot = next; sequence = record.sequence;
    dirty = 0; retired = 0;
    return 1;
}

/* FAT calls this after the actual mount/unmount; publish state only. */
void elmfat_volume_state_notify(const char *path, int mounted)
{
    if (!path || strcmp(path, "/userdata")) return;
    pthread_mutex_lock(&error_lock);
    media_ready = !!mounted;
    loaded = 0; active_slot = -1; sequence = 0;
    pthread_mutex_unlock(&error_lock);
}

void divoom_device_error_userdata_ready(int ready)
{
    pthread_mutex_lock(&error_lock);
    if (media_ready != !!ready) {
        loaded = 0; active_slot = -1; sequence = 0;
    }
    media_ready = !!ready;
    if (ready) save_locked();
    pthread_mutex_unlock(&error_lock);
}

int divoom_device_error_userdata_repair_complete(void)
{
    pthread_mutex_lock(&error_lock);
    /* A structural repair can remove either file without an unmount. The
     * already loaded RAM mask remains authoritative and must be republished. */
    dirty = 1;
    int ok = save_locked();
    pthread_mutex_unlock(&error_lock);
    return ok;
}

void divoom_device_error_userdata_format_begin(void)
{
    /* Only userdata formatting is serialized; reporters can still queue RAM events. */
    pthread_mutex_lock(&error_format_lock);
    pthread_mutex_lock(&error_lock);
    if (!load_locked())
        divoom_print("device_error: old userdata reports unavailable before format\n");
    formatting = 1;
    loaded = 0; active_slot = -1; sequence = 0;
    dirty = 1;
    pthread_mutex_unlock(&error_lock);
}

void divoom_device_error_userdata_format_end(void)
{
    pthread_mutex_lock(&error_lock);
    formatting = 0;
    loaded = 0; active_slot = -1; sequence = 0;
    save_locked();
    pthread_mutex_unlock(&error_lock);
    pthread_mutex_unlock(&error_format_lock);
}

void divoom_device_error_record(int error_id)
{
    if (error_id < 1 || error_id > ERROR_COUNT) return;
    unsigned bit = 1U << (error_id - 1);
    pthread_mutex_lock(&error_lock);
    load_locked();
    if (!(pending & bit)) {
        pending |= bit;
        retired &= ~bit;
        dirty = 1;
        if (!save_locked())
            divoom_print("device_error: cannot persist error %d; retained in RAM\n", error_id);
    }
    pthread_mutex_unlock(&error_lock);
}

/* Called by SET_ALL_MODE: consume this batch and queue every error once. */
void divoom_device_error_report_all(void)
{
    unsigned errors;
    pthread_mutex_lock(&error_lock);
    load_locked();
    errors = pending;
    if (errors) {
        pending = 0;
        retired |= errors;
        dirty = 1;
    }
    if (!save_locked() && errors)
        divoom_print("device_error: cannot clear stored errors\n");
    pthread_mutex_unlock(&error_lock);

    for (int id = 1; id <= ERROR_COUNT; ++id) {
        if (!(errors & (1U << (id - 1)))) continue;
        int queued = divoom_net_app_send_packet_checked(
            DIVOOM_NET_COMM_SAVE_DEVICE_ERROR_RECORD, DIVOOM_SERVER_TOPIC, &id);
        if (!queued) divoom_print("device_error: cannot queue error %d\n", id);
    }
}

#else
void divoom_device_error_userdata_ready(int ready) { (void)ready; }
int divoom_device_error_userdata_repair_complete(void) { return 1; }
void divoom_device_error_userdata_format_begin(void) { }
void divoom_device_error_userdata_format_end(void) { }
void divoom_device_error_record(int id) { (void)id; }
void divoom_device_error_report_all(void) { }
#endif
