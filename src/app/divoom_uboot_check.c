/* RTOS-only, read-only checks of the fixed F101S3 SD-NAND boot packages. */
#include "divoom.h"
#include "divoom_device_error.h"
#include <blkpart.h>
#include <stdint.h>
#include <stdio.h>

/* Match Brandy spare_head.h; these are raw sectors before GPT partitions. */
#define UBOOT_MAIN_OFFSET (32800U * 512U)
#define UBOOT_BACK_OFFSET (24576U * 512U)
#define UBOOT_MAX_BYTES   (4U * 1024U * 1024U)
#define UBOOT_CHUNK       4096U
#define UBOOT_MAGIC       0x89119800U
#define UBOOT_SUM_STAMP   0x5f0a6c39U
static uint32_t uboot_words[UBOOT_CHUNK / 4U] __attribute__((aligned(64)));

/* 1 valid, -1 confirmed bad header/checksum, 0 unreadable or unavailable. */
static int uboot_valid(struct part *disk, uint32_t base)
{
    uint32_t length, expected, sum = 0;
    if (!disk || !disk->blk || !disk->blk->read ||
        disk->bytes < (uint64_t)base + UBOOT_MAX_BYTES ||
        disk->blk->read(base, (char *)uboot_words, UBOOT_CHUNK)) return 0;
    if (uboot_words[4] != UBOOT_MAGIC) return -1;
    expected = uboot_words[5]; length = uboot_words[9];
    if (length < UBOOT_CHUNK || length > UBOOT_MAX_BYTES || (length & 511U)) return -1;
    for (uint32_t offset = 0; offset < length; offset += UBOOT_CHUNK) {
        uint32_t count = length - offset;
        if (count > UBOOT_CHUNK) count = UBOOT_CHUNK;
        if (offset && disk->blk->read(base + offset, (char *)uboot_words, count)) return 0;
        for (uint32_t i = 0; i < count / 4U; ++i)
            sum += (!offset && i == 5) ? UBOOT_SUM_STAMP : uboot_words[i];
    }
    return sum == expected ? 1 : -1;
}

/* Before filesystem mounts: the outbox retains confirmed errors in RAM. */
void divoom_uboot_check(void)
{
    struct part *disk = get_part_by_index("emmc", 0);
    const uint32_t offsets[] = { UBOOT_MAIN_OFFSET, UBOOT_BACK_OFFSET };
    const int errors[] = { DIVOOM_SD_NAND_ERR_UBOOT, DIVOOM_SD_NAND_ERR_UBOOT_BACK };
    for (unsigned i = 0; i < 2; ++i) {
        int valid = uboot_valid(disk, offsets[i]);
        if (valid < 0) {
            printf("[BOOT CHECK] uboot%s invalid, error=%d\n", i ? "-backup" : "", errors[i]);
            divoom_device_error_record(errors[i]);
        } else if (!valid) {
            printf("[BOOT CHECK] uboot%s check unavailable\n", i ? "-backup" : "");
        }
    }
}
