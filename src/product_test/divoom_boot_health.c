#if !defined(_WIN32)
#include <sys/errno.h> /* Constants must not come from the wireless iw/errno.h. */
#endif
#include "divoom.h"
#include "divoom_boot_health.h"
#include "divoom_device_error.h"
#include "divoom_product_test_format.h"
#include "divoom_product_test_repair.h"
#include <stdint.h>
#include <errno.h>

#define HEALTH_MAGIC 0x31484244U
#define HEALTH_PATH "/userdata/.boot_health."
#define HEALTH_CHECK_AFTER_FAILURES 3U
#define HEALTH_STABLE_MS (10U * 60U * 1000U)
typedef struct { uint32_t magic, sequence, dirty, failures, checksum; } health_record;
static void *health_lock;
static health_record health;
static int health_slot = -1, health_running;
static uint32_t health_clear_after;
static int health_userdata_formatting;
static volatile int mount_failed, userdata_mount_failed;
static volatile int mount_error[2], mount_storage_error[2];

void elmfat_mount_error_notify(const char *dev, int result, int storage_error)
{
    if (!dev) return;
    int target = !strcmp(dev, "usr") ? 1 : !strcmp(dev, "UDISK") ? 0 : -1;
    if (target < 0) return;
    mount_error[target] = result;
    mount_storage_error[target] = storage_error;
}

int divoom_boot_health_mount_reason(int target_usr) { return mount_error[!!target_usr]; }
int divoom_boot_health_mount_storage_failure(int target_usr) { return mount_storage_error[!!target_usr]; }

static int health_lock_take(void)
{
    void *lock = __atomic_load_n(&health_lock, __ATOMIC_ACQUIRE);
    if (!lock) {
        void *created = divoom_system_create_lock();
        if (!created) return 0;
        lock = __sync_val_compare_and_swap(&health_lock, NULL, created);
        if (lock) divoom_system_destroy_lock(created);
        else lock = created;
    }
    divoom_system_get_lock_direct(lock);
    return 1;
}

static uint32_t health_checksum(const health_record *r)
{
    const unsigned char *p = (const unsigned char *)r;
    uint32_t crc = 0xffffffffU;
    size_t i;
    for (i = 0; i < sizeof(*r)-sizeof(r->checksum); ++i) {
        unsigned bit;
        crc ^= p[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (crc & 1U ? 0xedb88320U : 0U);
    }
    return crc ^ 0xffffffffU;
}

static int health_io_failed(const char *path, const char *operation, int error)
{
    int saved_errno = error ? error : EIO;
    divoom_product_test_repair_record_io(path, operation, saved_errno);
    errno = saved_errno;
    return 0;
}

static int health_read(int slot, health_record *r)
{
    char path[64];
    unsigned char extra;
    void *fp;
    int read_error, close_error;
    size_t count, extra_count = 0;
    snprintf(path, sizeof(path), HEALTH_PATH "%d", slot);
    errno = 0;
    fp = divoom_port_file_open(path, "rb");
    if (!fp) {
        if (errno == ENOENT) return 0;
        health_io_failed(path, "health.read_open", errno);
        return -1;
    }
    errno = 0;
    count = divoom_port_file_read(r, 1, sizeof(*r), fp);
    if (count == sizeof(*r)) extra_count = divoom_port_file_read(&extra, 1, 1, fp);
    read_error = ferror((FILE *)fp) ? (errno ? errno : EIO) : 0;
    errno = 0;
    close_error = divoom_port_file_close(fp) != 0 ? (errno ? errno : EIO) : 0;
    if (read_error || close_error) {
        health_io_failed(path, read_error ? "health.read" : "health.read_close",
            read_error ? read_error : close_error);
        return -1;
    }
    /* Existing corrupt bytes are not proof of a persistent disk I/O failure. */
    errno = 0;
    return count == sizeof(*r) && !extra_count && r->magic == HEALTH_MAGIC &&
        r->dirty <= 1 && r->failures <= HEALTH_CHECK_AFTER_FAILURES &&
        r->checksum == health_checksum(r) ? 1 : -1;
}

static int health_save(void)
{
    char path[64];
    health_record verify;
    int next = health_slot == 0 ? 1 : 0, error = 0;
    const char *operation = NULL;
    void *fp;
    health.magic = HEALTH_MAGIC;
    ++health.sequence;
    health.checksum = health_checksum(&health);
    snprintf(path, sizeof(path), HEALTH_PATH "%d", next);
    errno = 0;
    fp = divoom_port_file_open(path, "wb");
    if (!fp) return health_io_failed(path, "health.write_open", errno);
    errno = 0;
    if (divoom_port_file_write(&health, 1, sizeof(health), fp) != sizeof(health)) {
        error = errno ? errno : EIO;
        operation = "health.write";
    } else {
        errno = 0;
        if (divoom_port_file_sync(fp) != 0) {
            error = errno ? errno : EIO;
            operation = "health.file_sync";
        }
    }
    errno = 0;
    if (divoom_port_file_close(fp) != 0 && !error) {
        error = errno ? errno : EIO;
        operation = "health.write_close";
    }
    if (error) return health_io_failed(path, operation, error);
    errno = 0;
    if (divoom_port_sync() != 0) return health_io_failed(path, "health.volume_sync", errno);
    int read_result = health_read(next, &verify);
    if (read_result != 1)
        return health_io_failed(path, "health.readback", read_result < 0 && errno ? errno : EIO);
    if (memcmp(&verify, &health, sizeof(health)) != 0)
        return health_io_failed(path, "health.readback_compare", EIO);
    health_slot = next;
    return 1;
}

static int health_begin_locked(void)
{
    health_record a = {0}, b = {0};
    int ra, rb;
    if (health_userdata_formatting) return -1;
    if (health_running) return 0;
    ra = health_read(0, &a); rb = health_read(1, &b);
    if (ra == 1 || rb == 1) {
        health_slot = rb == 1 && (ra != 1 || (int32_t)(b.sequence-a.sequence) > 0) ? 1 : 0;
        health = health_slot ? b : a;
    } else if (ra < 0 || rb < 0) {
        /* Do not silently accept corrupt/unreadable persistent state. */
        return divoom_product_test_recovery_mark('C') ? 1 : -1;
    }
    if (health.dirty && health.failures < HEALTH_CHECK_AFTER_FAILURES) ++health.failures;
    health.dirty = 1;
    if (!health_save()) return divoom_product_test_recovery_mark('C') ? 1 : -1;
    if (health.failures >= HEALTH_CHECK_AFTER_FAILURES) {
        if (!divoom_product_test_recovery_mark('C')) return -1;
        return 2; /* Release the health mutex before the intentional reboot. */
    }
    health_running = 1;
    health_clear_after = (uint32_t)divoom_system_get_tickms() + HEALTH_STABLE_MS;
    return 0;
}

int divoom_boot_health_begin(void)
{
    int result;
    if (!health_lock_take()) return -1;
    result = health_begin_locked();
    divoom_system_free_lock_direct(health_lock);
    if (result == 2) { divoom_system_reboot(); return 1; }
    return result;
}

int divoom_boot_health_restart_count(void)
{
    if (!health_lock_take()) return 0;
    int count = (int)health.failures;
    divoom_system_free_lock_direct(health_lock);
    return count;
}

static int health_clean_locked(void)
{
    /* Load the latest sequence even in a cold recovery boot. */
    health_record a = {0}, b = {0};
    int ra, rb;
    if (health_userdata_formatting) return 0;
    ra = health_read(0, &a); rb = health_read(1, &b);
    if (ra == 1 || rb == 1) {
        health_slot = rb == 1 && (ra != 1 || (int32_t)(b.sequence-a.sequence) > 0) ? 1 : 0;
        health = health_slot ? b : a;
    }
    health.dirty = health.failures = 0;
    if (!health_save()) return 0;
    health_running = 0;
    return 1;
}

int divoom_boot_health_clean(void)
{
    int result;
    if (!health_lock_take()) return 0;
    result = health_clean_locked();
    divoom_system_free_lock_direct(health_lock);
    return result;
}

int divoom_boot_health_userdata_format_begin(void)
{
    if (!health_lock_take()) return 0;
    /* Drain in-flight shutdown writes before the old FAT context is removed. */
    health_userdata_formatting = 1;
    divoom_system_free_lock_direct(health_lock);
    return 1;
}

void divoom_boot_health_userdata_format_end(void)
{
    if (!health_lock_take()) return;
    health_userdata_formatting = 0;
    divoom_system_free_lock_direct(health_lock);
}

void divoom_boot_health_tick(void)
{
    if (!health_lock_take()) return;
    uint32_t now = (uint32_t)divoom_system_get_tickms();
    if (health_running && !health_userdata_formatting &&
        (int32_t)(now - health_clear_after) >= 0) {
        /* Only boot history is cleared; durable repair intent stays intact.
         * Successful clean disables further writes for this run. */
        if (health_clean_locked())
            printf("boot_health: stable for 600s, restart history cleared\n");
        else
            health_clear_after = now + 10000U;
    }
    divoom_system_free_lock_direct(health_lock);
}

/* AstroToo keeps blank/corrupt system and user volumes untouched at boot.
 * Report the mount failure and let the factory recovery worker own mkfs.
 * Other boards keep the generic FAT hook's default first-boot behavior. */
int elmfat_autoformat_notify(const char *dev, int completed)
{
    (void)completed;
    if (!strcmp(dev, "usr")) {
        mount_failed = 1;
        return -1;
    }
    if (!strcmp(dev, "UDISK")) {
        userdata_mount_failed = 1;
        return -1;
    }
    return 0;
}

int divoom_boot_health_mount_failed(void) { return mount_failed; }
int divoom_boot_health_userdata_mount_failed(void) { return userdata_mount_failed; }

void divoom_boot_health_set_mount_failed(int target_usr, int failed)
{
    if (!target_usr) divoom_device_error_userdata_ready(!failed);
    if (failed && divoom_boot_health_mount_storage_failure(target_usr))
        divoom_device_error_record(target_usr ? DIVOOM_SD_NAND_ERR_USR_FORMAT : DIVOOM_SD_NAND_ERR_USERDATA);
    if (target_usr) mount_failed = !!failed;
    else userdata_mount_failed = !!failed;
}
