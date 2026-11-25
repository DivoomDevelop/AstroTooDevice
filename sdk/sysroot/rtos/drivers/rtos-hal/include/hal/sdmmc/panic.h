#ifndef SDMMC_PANIC_H
#define SDMMC_PANIC_H

#include <stdint.h>
#include <stddef.h>
struct mmc_card;

/* Snapshot prepared in task context; panic I/O never follows card/host locks. */
struct mmc_panic_context {
    uintptr_t base;
    uint32_t rca;
    uint32_t first_sector;
    uint32_t sector_count;
    int block_addressed;
    uint64_t started_ns;
};

int mmc_panic_prepare(struct mmc_panic_context *ctx, struct mmc_card *card,
                      uint32_t first_sector, uint32_t sector_count);
/* Fatal exception only. Takes over the controller; never resume normal I/O. */
int mmc_panic_begin(struct mmc_panic_context *ctx);
int mmc_panic_write(struct mmc_panic_context *ctx, uint32_t offset_sector,
                    const void *buffer, uint32_t sectors);
#endif
