/* RTOS-side boot package check; U-Boot and the on-flash package stay unchanged. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <blkpart.h>
#include <env.h>
#include "divoom_device_error.h"

#define AB_LOAD_ADDRESS 0x40800000UL
#define AB_SLOT_MAX     0x680000U
#define AB_CHUNK        4096U
#define AB_MAGIC        0x89119800U
#define AB_SUM_STAMP    0x5f0a6c39U

struct ab_head {
    char name[16];
    uint32_t magic, sum, serial, status, items, length, version_main;
    uint32_t version_sub, reserved[3], end;
};
struct ab_item {
    char name[64];
    uint32_t offset, length, encrypt, type, run_addr, index, reserved[69], end;
};

/* 0 = unavailable, 1 = valid, -1 = invalid. Set before heap_init. */
static int current_result;
static int recovery_pending;
static int partition_read_error;
static const char *running_partition;
static uint32_t io_words[AB_CHUNK / sizeof(uint32_t)] __attribute__((aligned(64)));
extern char __bss_end__[];
extern int cmd_reboot(int argc, char **argv);

static int name_is(const char *name, const char *expected)
{
    unsigned i;
    for (i = 0; i < 64 && expected[i]; ++i)
        if (name[i] != expected[i]) return 0;
    return i < 64 && name[i] == 0;
}

/* The complete item table must fit in the first chunk, before dereferencing it. */
static int valid_header(const void *data, uint32_t available, uint32_t capacity)
{
    const struct ab_head *h = data;
    const struct ab_item *it;
    uint32_t i, rtos = 0, config = 0;
    if (available < sizeof(*h) || h->magic != AB_MAGIC ||
        h->length < AB_CHUNK || (h->length & 511U) ||
        h->length > capacity || h->length > AB_SLOT_MAX || !h->items ||
        h->items > (available - sizeof(*h)) / sizeof(*it))
        return 0;
    it = (const struct ab_item *)(h + 1);
    for (i = 0; i < h->items; ++i, ++it) {
        uint32_t rounded;
        if (!it->length || it->length > h->length ||
            (it->offset & 511U) || it->offset > h->length ||
            it->length > h->length - it->offset)
            return 0;
        rounded = (it->length + 511U) & ~511U;
        if (rounded > h->length - it->offset) return 0;
        if (name_is(it->name, "rtos")) {
            if (++rtos != 1 || rounded > 0x7f0000U) return 0;
        } else if (name_is(it->name, "melis-config")) {
            if (++config != 1 || rounded > 0x10000U) return 0;
        }
    }
    return rtos == 1 && config == 1;
}

static uint32_t add_words(const uint32_t *p, uint32_t bytes, uint32_t offset)
{
    uint32_t i, sum = 0;
    for (i = 0; i < bytes / 4U; ++i)
        sum += (offset == 0 && i == 5) ? AB_SUM_STAMP : p[i];
    return sum;
}

void divoom_boot_ab_capture_early(void)
{
    const struct ab_head *h = (const void *)AB_LOAD_ADDRESS;
    /* Startup clears BSS before this hook. If it reached the package, do not
     * mistake overwritten RAM for flash corruption; check storage later. */
    if ((uintptr_t)__bss_end__ > AB_LOAD_ADDRESS) return;
    if (!valid_header(h, AB_CHUNK, AB_SLOT_MAX)) {
        current_result = -1;
        return;
    }
    current_result = add_words((const uint32_t *)h, h->length, 0) == h->sum ? 1 : -1;
}

int divoom_boot_ab_current_verified(void) { return current_result == 1; }
int divoom_boot_ab_recovery_pending(void) { return recovery_pending; }
const char *divoom_boot_ab_running_partition(void) { return running_partition; }
void divoom_boot_ab_storage_unavailable(void)
{
    if (current_result != 1) recovery_pending = 1;
    printf("[BOOT AB] storage unavailable\n");
    if (current_result < 0) divoom_device_error_record(5);
}

int divoom_boot_ab_verify_partition(const char *name)
{
    struct part *p = get_part_by_name(name);
    struct ab_head *h = (void *)io_words;
    uint32_t length, expected, sum = 0, offset = 0;
    partition_read_error = 1;
    if (!p || !p->blk || !p->blk->read || p->bytes < AB_CHUNK ||
        p->off > UINT32_MAX || p->bytes > UINT32_MAX ||
        p->off + p->bytes > (uint64_t)UINT32_MAX + 1U)
        return 0;
    if (p->blk->read((unsigned int)p->off, (char *)io_words, AB_CHUNK))
        return 0;
    partition_read_error = 0;
    if (!valid_header(io_words, AB_CHUNK, (uint32_t)p->bytes)) return 0;
    length = h->length;
    expected = h->sum;
    while (offset < length) {
        uint32_t count = length - offset;
        if (count > AB_CHUNK) count = AB_CHUNK;
        if (offset && p->blk->read((unsigned int)(p->off + offset),
                                   (char *)io_words, count)) {
            partition_read_error = 1;
            return 0;
        }
        sum += add_words(io_words, count, offset);
        offset += count;
    }
    return sum == expected;
}

/* Called once the SD-NAND partitions are registered, before mounting volumes. */
void divoom_boot_ab_check(void)
{
    const char *value, *current, *other;
    int saved = 0, confirmed_damage = current_result < 0;
    if (fw_env_open() != 0) {
        if (current_result != 1) recovery_pending = 1;
        printf("[BOOT AB] environment unavailable\n");
        if (current_result < 0) divoom_device_error_record(5);
        return;
    }
    value = fw_getenv("boot_partition");
    if (value && !strcmp(value, "bootA")) {
        current = "bootA"; other = "bootB";
    } else if (value && !strcmp(value, "bootB")) {
        current = "bootB"; other = "bootA";
    } else {
        recovery_pending = current_result != 1;
        printf("[BOOT AB] unknown boot partition\n");
        fw_env_close();
        if (current_result < 0) divoom_device_error_record(5);
        return;
    }
    /* Capture before changing boot_partition; it is immutable for this boot. */
    running_partition = current;
    if (!current_result) {
        current_result = divoom_boot_ab_verify_partition(current) ? 1 : -1;
        confirmed_damage = current_result < 0 && !partition_read_error;
    }
    if (current_result == 1) {
        fw_env_close();
        /* Reconstruct a failed-bank report after the previous boot's fast
         * fallback; that boot could not yet persist anything on /userdata. */
        if (!divoom_boot_ab_verify_partition(other) && !partition_read_error)
            divoom_device_error_record(5);
        return;
    }
    printf("[BOOT AB] %s invalid\n", current);
    if (confirmed_damage) divoom_device_error_record(5); /* RAM until mount. */
    if (divoom_boot_ab_verify_partition(other)) {
        printf("[BOOT AB] %s verified, switching\n", other);
        if (fw_env_write("boot_partition", (char *)other) == 0 &&
            fw_env_flush() == 0)
            saved = 1;
        fw_env_close();
        /* Verify the persisted value before reboot; otherwise continue recovery. */
        if (saved && fw_env_open() == 0) {
            value = fw_getenv("boot_partition");
            saved = value && !strcmp(value, other);
            fw_env_close();
        } else saved = 0;
        if (saved) {
            cmd_reboot(1, NULL);
            printf("[BOOT AB] reboot returned\n");
        }
        recovery_pending = 1;
        return;
    }
    recovery_pending = 1;
    printf("[BOOT AB] both slots invalid, continuing %s\n", current);
    if (fw_env_write("rtos_ab_failure", (char *)current) || fw_env_flush())
        printf("[BOOT AB] cannot persist recovery marker\n");
    fw_env_close();
}
