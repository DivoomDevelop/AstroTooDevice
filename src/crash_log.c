/* SD/MMC crash journal. The on-disk format is independent of SDK pstore. */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <blkpart.h>
#include <hal_time.h>
#include <sdmmc/sdmmc.h>
#include <sdmmc/panic.h>
#include <divoom_crash_log.h>
#include <context.h>
#include <arch/riscv/excep.h>
#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>
#endif

#define CRASH_MAGIC 0x31524344U /* DCR1 */
#define CRASH_VERSION 4U
#define CRASH_BYTES 8192U
#define CRASH_TEXT_BYTES 4096U
#define CRASH_STACK_BYTES 4096U
#define CRASH_SECTOR 512U
#define CRASH_SLOT_SECTORS (1U + CRASH_BYTES / CRASH_SECTOR)
#define CRASH_SLOTS 2U
#define CRASH_DIR "/userdata/crash"
#define CRASH_FLAG_SOFTWARE_FATAL 128U

struct crash_header {
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t sequence;
    uint64_t uptime_ns;
    uint32_t payload_crc;
    uint32_t header_crc;
    uint32_t text_length;
    uint32_t stack_address;
    uint32_t stack_length;
    uint32_t flags;
    /* Little-endian RV32 values; no formatted log in the file. */
    uint8_t build_id[16];
    uint32_t regs[32];
    uint32_t mepc, mcause, mtval, mstatus, mscratch;
    uint32_t task_handle, task_number, task_priority;
    uint32_t task_low, task_high, task_saved_sp;
    char task_name[32];
    uint32_t saved_regs[32];
    uint32_t saved_mepc, saved_mstatus, saved_mscratch;
    uint32_t software_version; /* v4: firmware at the time of the exception. */
    /* v4 reserved bytes: populated only when CRASH_FLAG_SOFTWARE_FATAL is set. */
    char fatal_reason[24];
    char fatal_file[72];
    uint32_t fatal_line;
};
typedef char crash_header_must_be_one_sector[(sizeof(struct crash_header) == 512) ? 1 : -1];

static struct part *crash_part;
#define CRASH_UDISK_RESERVE (64U * 1024U)
static struct part udisk_journal;
static struct part *reserved_udisk;
static struct mmc_panic_context panic_ctx;
static char panic_text[CRASH_BYTES] __attribute__((aligned(64)));
static struct crash_header panic_header __attribute__((aligned(64)));
static uint8_t empty_sector[CRASH_SECTOR] __attribute__((aligned(64)));
static unsigned int panic_length;
static volatile unsigned int next_slot = CRASH_SLOTS;
static uint32_t next_sequence;
static int armed, capturing, flushing, flushed;
static int overflow_pending, task_valid, saved_context_valid;
static struct divoom_crash_task_info task_info;
static switch_ctx_regs_t saved_context;
static uintptr_t stack_low, stack_high, stack_sp;
extern char __freertos_irq_stack_bottom[], __freertos_irq_stack_top[];
extern char __init_process_stack_start__[], __init_process_stack_end__[];
struct crash_build_note {
    uint32_t namesz, descsz, type;
    char name[4];
    uint8_t id[16];
};
extern const struct crash_build_note __crash_build_id_start;
extern const uint32_t divoom_crash_software_version;

static uint32_t crc_update(uint32_t crc, const void *buffer, size_t length)
{
    const uint8_t *p = buffer;
    size_t i;
    while (length--) {
        crc ^= *p++;
        for (i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    return crc;
}

static uint32_t crash_crc(const void *buffer, size_t length)
{
    return ~crc_update(0xffffffffU, buffer, length);
}

static uint32_t header_crc(const struct crash_header *h)
{
    uint32_t crc = crc_update(0xffffffffU, h, offsetof(struct crash_header, header_crc));
    if (h->version >= 2)
        crc = crc_update(crc, &h->text_length, sizeof(*h) - offsetof(struct crash_header, text_length));
    return ~crc;
}

static int header_valid(const struct crash_header *h)
{
    if (h->magic != CRASH_MAGIC || h->version != CRASH_VERSION || h->header_crc != header_crc(h))
        return 0;
    return h->text_length == 0 && h->length == h->stack_length &&
           h->stack_length <= CRASH_STACK_BYTES && h->stack_length % 4 == 0;
}

int divoom_crash_log_record_info(const void *record, size_t size, uint32_t *version)
{
    const struct crash_header *h = record;
    if (size < sizeof(*h) || !header_valid(h) || size != sizeof(*h) + h->length ||
        crash_crc((const char *)record + sizeof(*h), h->length) != h->payload_crc)
        return -1;
    *version = h->software_version;
    return 0;
}

int divoom_crash_log_available(void) { return crash_part && armed; }

/* Normal task-context access. Never call these callbacks from the trap. */
static int journal_io(int write_data, unsigned int slot, unsigned int sector,
                      void *buffer, unsigned int bytes)
{
    uint64_t off;
    if (!crash_part || slot >= CRASH_SLOTS || sector >= CRASH_SLOT_SECTORS ||
        bytes % CRASH_SECTOR || bytes > (CRASH_SLOT_SECTORS - sector) * CRASH_SECTOR)
        return -1;
    off = crash_part->off + (slot * CRASH_SLOT_SECTORS + sector) * CRASH_SECTOR;
    if (write_data)
        return crash_part->blk->program((unsigned int)off, buffer, bytes);
    return crash_part->blk->read((unsigned int)off, buffer, bytes);
}

static int choose_slot(void)
{
    struct crash_header h[CRASH_SLOTS] __attribute__((aligned(64)));
    int valid[CRASH_SLOTS];
    unsigned int i, newest;
    /* No writable slot until both headers have been read successfully. */
    next_slot = CRASH_SLOTS;
    for (i = 0; i < CRASH_SLOTS; ++i) {
        if (journal_io(0, i, 0, &h[i], sizeof(h[i])))
            return -1;
        valid[i] = header_valid(&h[i]);
    }
    newest = valid[1] && (!valid[0] || (int32_t)(h[1].sequence - h[0].sequence) > 0);
    next_sequence = valid[newest] ? h[newest].sequence + 1 : 1;
    /* Protect the first exception, even if the other legacy slot is free. */
    next_slot = valid[0] || valid[1] ? CRASH_SLOTS : 0;
    return 0;
}

/* Probe before mounting UDISK. Never shrink an existing FAT volume in place. */
static uint32_t crash_le16(const uint8_t *p)
{
    return p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t crash_le32(const uint8_t *p)
{
    return crash_le16(p) | (crash_le16(p + 2) << 16);
}

static int crash_part_valid(const struct part *p, uint64_t minimum)
{
    return p && p->blk && p->blk->name && !strcmp(p->blk->name, "emmc") &&
           p->blk->read && p->blk->program && !(p->off % CRASH_SECTOR) &&
           !(p->bytes % CRASH_SECTOR) && p->bytes >= minimum &&
           p->off < (1ULL << 32) && p->bytes <= (1ULL << 32) - p->off &&
           (uintptr_t)p->private < 3;
}

/* Only known FAT12/16/32 BPBs are accepted; unknown/corrupt layouts fail closed. */
static int crash_fat_fits(const uint8_t *b, uint32_t limit)
{
    uint32_t total = crash_le16(b + 19);
    uint32_t fat = crash_le16(b + 22);
    uint32_t reserved = crash_le16(b + 14);
    uint32_t roots = crash_le16(b + 17);
    uint32_t cluster = b[13];
    if (!total)
        total = crash_le32(b + 32);
    if (crash_le16(b + 510) != 0xaa55 ||
        (b[0] != 0xeb && b[0] != 0xe9) || crash_le16(b + 11) != 512 ||
        !cluster || cluster > 128 || (cluster & (cluster - 1)) ||
        !reserved || !b[16] || b[16] > 2 || !total || total > limit)
        return 0;
    if (fat) {
        if (!roots || memcmp(b + 54, "FAT", 3))
            return 0;
    } else {
        fat = crash_le32(b + 36);
        if (roots || memcmp(b + 82, "FAT32   ", 8) || crash_le16(b + 42))
            return 0;
    }
    return fat && (uint64_t)reserved + (uint64_t)b[16] * fat +
                  ((uint64_t)roots * 32 + 511) / 512 < total;
}

static struct part *crash_udisk_tail(void)
{
    uint8_t boot[CRASH_SECTOR] __attribute__((aligned(64)));
    uint8_t entries[64];
    struct part *p = get_part_by_name("UDISK");
    uint32_t limit;
    int volumes = 0;
    if (p && p == reserved_udisk && p->off + p->bytes == udisk_journal.off)
        return &udisk_journal; /* Re-init must not reserve another 64 KiB. */
    if (!crash_part_valid(p, CRASH_UDISK_RESERVE + CRASH_SECTOR))
        return NULL;
    limit = (p->bytes - CRASH_UDISK_RESERVE) / CRASH_SECTOR;
    if (p->blk->read((unsigned int)p->off, (char *)boot, sizeof(boot)))
        goto unsafe;
    if (!crash_fat_fits(boot, limit)) {
        /* FAT volume may live inside a DOS partition table (mkfs without SFD). */
        if (crash_le16(boot + 510) != 0xaa55 || boot[0] == 0xeb || boot[0] == 0xe9)
            goto unsafe;
        memcpy(entries, boot + 446, sizeof(entries));
        for (unsigned int i = 0; i < 4; ++i) {
            const uint8_t *e = entries + i * 16;
            uint32_t start = crash_le32(e + 8), count = crash_le32(e + 12);
            if (!e[4] && !start && !count)
                continue;
            /* No extended/GPT/unknown partitions whose extents we cannot verify. */
            if ((e[0] != 0 && e[0] != 0x80) ||
                (e[4] != 1 && e[4] != 4 && e[4] != 6 && e[4] != 0x0b &&
                 e[4] != 0x0c && e[4] != 0x0e) ||
                !start || start >= limit || !count || count > limit - start)
                goto unsafe;
            if (p->blk->read((unsigned int)(p->off + (uint64_t)start * CRASH_SECTOR),
                             (char *)boot, sizeof(boot)) || !crash_fat_fits(boot, count))
                goto unsafe;
            ++volumes;
        }
        if (!volumes)
            goto unsafe;
    }
    /* RAM-only partition view: GPT and the existing FAT metadata stay untouched.
     * Diskio uses p->bytes for formatting capacity and I/O bounds. */
    udisk_journal = *p;
    udisk_journal.off = p->off + p->bytes - CRASH_UDISK_RESERVE;
    udisk_journal.bytes = CRASH_UDISK_RESERVE;
    p->bytes -= CRASH_UDISK_RESERVE;
#ifdef CONFIG_COMPONENTS_AW_DEVFS
    p->node.size = p->bytes;
#endif
    reserved_udisk = p;
    printf("[crash] using UDISK tail 64 KiB at 0x%08lx; FAT capacity limited\n",
           (unsigned long)udisk_journal.off);
    return &udisk_journal;
unsafe:
    printf("[crash] UDISK tail not proven outside FAT; crash storage disabled\n");
    return NULL;
}

int divoom_crash_log_init(void)
{
    struct mmc_card *card;
    int ret;
    armed = 0;
    next_slot = CRASH_SLOTS;
    memset(&panic_ctx, 0, sizeof(panic_ctx));
    crash_part = get_part_by_name("pstore");
    if (!crash_part)
        crash_part = crash_udisk_tail();
    if (!crash_part_valid(crash_part, CRASH_SLOTS * CRASH_SLOT_SECTORS * CRASH_SECTOR)) {
        printf("[crash] no usable pstore/UDISK journal; capture disabled\n");
        crash_part = NULL;
        return -1;
    }
    card = mmc_card_open((uint8_t)(uintptr_t)crash_part->private);
    if (!card)
        return -1;
    ret = mmc_panic_prepare(&panic_ctx, card, crash_part->off / CRASH_SECTOR,
                           CRASH_SLOTS * CRASH_SLOT_SECTORS);
    mmc_card_close((uint8_t)(uintptr_t)crash_part->private);
    if (ret || choose_slot()) {
        printf("[crash] journal init failed; capture disabled\n");
        return -1;
    }
    armed = 1;
    printf("[crash] journal ready (2 x 8 KiB payloads)\n");
    if (next_slot == CRASH_SLOTS)
        printf("[crash] pending exception protected; new captures disabled until read\n");
    return 0;
}

/* Independent filesystem-recovery journal. Keep the crash ABI and both
 * original slots untouched. No FAT, mkdir, log or callback may run here. */
int divoom_recovery_storage_io(int write_data, uint32_t offset, void *data, uint32_t bytes)
{
    struct part *p = get_part_by_name("pstore");
    if (!p && reserved_udisk) p = &udisk_journal;
    if (!crash_part_valid(p, 65536U) || offset > 32768U || bytes > 32768U-offset ||
        offset % CRASH_SECTOR || bytes % CRASH_SECTOR) return -1;
    uint64_t address = p->off + 32768U + offset;
    int rc = write_data ? p->blk->program((unsigned int)address, data, bytes) :
        p->blk->read((unsigned int)address, data, bytes);
    if (!rc && write_data && p->blk->sync) rc = p->blk->sync();
    return rc;
}

static int crash_begin(const char *reason)
{
    if (capturing)
        return -1;
    capturing = 1;
    panic_length = 0;
    panic_header.uptime_ns = hal_gettime_ns();
    panic_header.software_version = divoom_crash_software_version;
    memcpy(panic_header.build_id, __crash_build_id_start.id, sizeof(panic_header.build_id));
    if (overflow_pending)
        panic_header.flags |= 1;
    divoom_crash_log_printf("AstroToo fatal exception\nELF Build ID: ");
    for (unsigned int i = 0; i < sizeof(__crash_build_id_start.id); ++i)
        divoom_crash_log_printf("%02x", __crash_build_id_start.id[i]);
    divoom_crash_log_printf("\nUptime(ns): %llu\nReason: %s\n",
                            (unsigned long long)panic_header.uptime_ns,
                            reason);
    return 0;
}

int divoom_crash_log_begin(void)
{
    return crash_begin(overflow_pending ?
        "STACK_OVERFLOW (trap registers show detection site)" : "CPU_EXCEPTION");
}

void divoom_crash_log_registers(const void *regs, unsigned long cause,
                                unsigned long pc, unsigned long value)
{
    const irq_regs_t *r = regs;
    panic_header.regs[0] = 0;
    panic_header.regs[1] = r->x1;
    panic_header.regs[2] = r->x2;
    panic_header.regs[3] = r->x3;
    panic_header.regs[4] = r->x4;
    panic_header.regs[5] = r->x5;
    panic_header.regs[6] = r->x6;
    panic_header.regs[7] = r->x7;
    panic_header.regs[8] = r->x8;
    panic_header.regs[9] = r->x9;
    panic_header.regs[10] = r->x10;
    panic_header.regs[11] = r->x11;
    panic_header.regs[12] = r->x12;
    panic_header.regs[13] = r->x13;
    panic_header.regs[14] = r->x14;
    panic_header.regs[15] = r->x15;
    panic_header.regs[16] = r->x16;
    panic_header.regs[17] = r->x17;
    panic_header.regs[18] = r->x18;
    panic_header.regs[19] = r->x19;
    panic_header.regs[20] = r->x20;
    panic_header.regs[21] = r->x21;
    panic_header.regs[22] = r->x22;
    panic_header.regs[23] = r->x23;
    panic_header.regs[24] = r->x24;
    panic_header.regs[25] = r->x25;
    panic_header.regs[26] = r->x26;
    panic_header.regs[27] = r->x27;
    panic_header.regs[28] = r->x28;
    panic_header.regs[29] = r->x29;
    panic_header.regs[30] = r->x30;
    panic_header.regs[31] = r->x31;
    panic_header.mepc = pc;
    panic_header.mcause = cause;
    panic_header.mtval = value;
    panic_header.mstatus = r->mstatus;
    panic_header.mscratch = r->mscratch;
    panic_header.flags |= 8; /* Complete trap register set. */
}

int divoom_crash_log_printf(const char *fmt, ...)
{
    va_list ap;
    int n;
    size_t remaining;
    if (!capturing || flushing || flushed)
        return -1;
    if (panic_length >= CRASH_TEXT_BYTES - 1)
        return 0;
    remaining = CRASH_TEXT_BYTES - panic_length;
    va_start(ap, fmt);
    n = vsnprintf(panic_text + panic_length, remaining, fmt, ap);
    va_end(ap);
    if (n > 0) {
        unsigned int actual = (size_t)n < remaining ? (unsigned int)n : remaining - 1;
        panic_length += actual;
        if ((size_t)n >= remaining) {
            static const char marker[] = "\n[crash log truncated]\n";
            memcpy(panic_text + CRASH_TEXT_BYTES - sizeof(marker), marker, sizeof(marker));
        }
        return actual;
    }
    return 0;
}

void divoom_crash_log_overflow_task(void *task)
{
    overflow_pending = 1;
    /* Save pxTopOfStack before ebreak: trap entry may replace the TCB context. */
    task_valid = divoom_crash_task_snapshot(task, &task_info) == 0;
}

static int within(uintptr_t addr, size_t size, uintptr_t low, uintptr_t high)
{
    return low >= CONFIG_ARCH_START_ADDRESS && high > low &&
           high <= CONFIG_ARCH_START_ADDRESS + CONFIG_ARCH_MEM_LENGTH &&
           addr >= low && addr < high && size <= high - addr;
}

void divoom_crash_log_task(unsigned long sp)
{
    if (!overflow_pending)
        task_valid = divoom_crash_task_snapshot(NULL, &task_info) == 0;
    stack_low = stack_high = 0;
    stack_sp = sp;
    if (task_valid) {
        panic_header.task_handle = task_info.handle;
        panic_header.task_number = task_info.number;
        panic_header.task_priority = task_info.priority;
        panic_header.task_low = task_info.stack_low;
        panic_header.task_high = task_info.stack_high;
        panic_header.task_saved_sp = task_info.saved_sp;
        memcpy(panic_header.task_name, task_info.name, sizeof(panic_header.task_name));
        panic_header.flags |= 2;
        divoom_crash_log_printf("Task: %s handle=0x%08lx number=%lu priority=%lu\n"
                               "Task stack: [0x%08lx,0x%08lx) size=%lu saved_context=0x%08lx\n",
            task_info.name, (unsigned long)task_info.handle, (unsigned long)task_info.number,
            (unsigned long)task_info.priority,
            (unsigned long)task_info.stack_low, (unsigned long)task_info.stack_high,
            (unsigned long)(task_info.stack_high - task_info.stack_low), (unsigned long)task_info.saved_sp);
        if (overflow_pending || within(sp, 1, task_info.stack_low, task_info.stack_high)) {
            stack_low = task_info.stack_low;
            stack_high = task_info.stack_high;
        }
    } else {
        divoom_crash_log_printf("Task metadata unavailable/invalid\n");
    }
    if (overflow_pending) {
        panic_header.flags |= 1;
        if (task_valid && !(task_info.saved_sp & 3) &&
            within(task_info.saved_sp, sizeof(saved_context), stack_low, stack_high)) {
            const volatile unsigned long *p = (const volatile unsigned long *)task_info.saved_sp;
            unsigned long *d = (unsigned long *)&saved_context;
            for (unsigned int i = 0; i < sizeof(saved_context) / sizeof(*d); ++i)
                d[i] = p[i];
            saved_context_valid = 1;
            panic_header.saved_regs[1] = saved_context.ra;
            panic_header.saved_regs[2] = saved_context.fp;
            panic_header.saved_regs[3] = saved_context.gp;
            panic_header.saved_regs[4] = saved_context.tp;
            for (unsigned int i = 0; i < 27; ++i)
                panic_header.saved_regs[i + 5] = saved_context.x5_x31[i];
            panic_header.saved_mepc = saved_context.mepc;
            panic_header.saved_mstatus = saved_context.mstatus;
            panic_header.saved_mscratch = saved_context.mscratch;
            panic_header.flags |= 4;
            stack_sp = saved_context.fp;
            divoom_crash_log_printf("Saved task registers (scheduler context, not hook registers):\n"
                "mepc=%08lx ra=%08lx sp=%08lx gp=%08lx tp=%08lx mstatus=%08lx mscratch=%08lx\n",
                saved_context.mepc, saved_context.ra, saved_context.fp, saved_context.gp,
                saved_context.tp, saved_context.mstatus, saved_context.mscratch);
            for (unsigned int i = 0; i < 27; ++i)
                divoom_crash_log_printf("x%u=%08lx%s", i + 5, saved_context.x5_x31[i], i % 4 == 3 ? "\n" : " ");
            divoom_crash_log_printf("\n");
        } else {
            stack_sp = task_info.saved_sp;
            divoom_crash_log_printf("Saved task context outside valid stack; registers/backtrace unavailable\n");
        }
        if (!within(stack_sp, 1, stack_low, stack_high)) {
            panic_header.flags |= 16; /* Raw snapshot falls back to stack base. */
            divoom_crash_log_printf("Overflow SP=0x%08lx; snapshot starts at valid task stack base\n", (unsigned long)stack_sp);
            stack_sp = stack_low;
        }
    } else if (!stack_high) {
        if (within(sp, 1, (uintptr_t)__freertos_irq_stack_bottom, (uintptr_t)__freertos_irq_stack_top)) {
            stack_low = (uintptr_t)__freertos_irq_stack_bottom;
            panic_header.flags |= 32;
            stack_high = (uintptr_t)__freertos_irq_stack_top;
            divoom_crash_log_printf("Fault SP is on IRQ stack; task above is the interrupted task\n");
        } else if (within(sp, 1, (uintptr_t)__init_process_stack_start__, (uintptr_t)__init_process_stack_end__)) {
            stack_low = (uintptr_t)__init_process_stack_start__;
            panic_header.flags |= 64;
            stack_high = (uintptr_t)__init_process_stack_end__;
            divoom_crash_log_printf("Fault SP is on boot stack\n");
        }
    }
}

void divoom_crash_log_stack(unsigned long sp)
{
    uintptr_t addr = stack_sp & ~(uintptr_t)3;
    uint32_t *dst = (uint32_t *)(panic_text + CRASH_TEXT_BYTES);
    (void)sp;
    panic_header.stack_address = addr;
    if (!within(addr, 4, stack_low, stack_high)) {
        divoom_crash_log_printf("No validated stack range; raw stack read skipped\n");
        return;
    }
    while (panic_header.stack_length < CRASH_STACK_BYTES && within(addr, 4, stack_low, stack_high)) {
        *dst++ = *(const volatile uint32_t *)addr;
        addr += 4;
        panic_header.stack_length += 4; /* Nested faults preserve completed words. */
    }
    divoom_crash_log_printf("Raw stack saved: address=0x%08lx bytes=%lu (limit 4096)\n",
        (unsigned long)panic_header.stack_address, (unsigned long)panic_header.stack_length);
}

void divoom_crash_log_backtrace(unsigned long status, unsigned long sp, unsigned long pc, unsigned long ra)
{
#ifdef CONFIG_DEBUG_BACKTRACE
    if (overflow_pending) {
        if (!saved_context_valid || !within(saved_context.fp, 4, stack_low, stack_high)) {
            divoom_crash_log_printf("Task backtrace unavailable: saved SP/context invalid\n");
            return;
        }
        status = saved_context.mstatus; sp = saved_context.fp;
        pc = saved_context.mepc; ra = saved_context.ra;
    }
    backtrace_exception(divoom_crash_log_printf, status, sp, pc, ra);
#else
    (void)status; (void)sp; (void)pc; (void)ra;
#endif
}

static void copy_fatal_field(char *dst, size_t capacity, const char *src)
{
    size_t length;
    if (!src)
        return;
    length = strlen(src);
    if (length >= capacity) {
        src += length - capacity + 1; /* Keep the filename, not a path prefix. */
        length = capacity - 1;
    }
    memcpy(dst, src, length);
    dst[length] = '\0';
}

int divoom_crash_log_capture_fatal(const char *reason, const char *file,
                                   unsigned int line, unsigned long pc,
                                   unsigned long sp)
{
    if (flushed)
        return 1; /* The CPU exception path already committed its record. */
    if (!crash_part || !armed || next_slot >= CRASH_SLOTS || !reason ||
        crash_begin(reason))
        return -1;

    panic_header.flags |= CRASH_FLAG_SOFTWARE_FATAL;
    panic_header.mepc = pc; /* Return address near the software failure site. */
    panic_header.regs[1] = pc;
    panic_header.regs[2] = sp;
    copy_fatal_field(panic_header.fatal_reason,
                     sizeof(panic_header.fatal_reason), reason);
    copy_fatal_field(panic_header.fatal_file,
                     sizeof(panic_header.fatal_file), file);
    panic_header.fatal_line = line;
    divoom_crash_log_task(sp);
    divoom_crash_log_stack(sp);
    return divoom_crash_log_flush();
}

int divoom_crash_log_flush(void)
{
    uint32_t sector = next_slot * CRASH_SLOT_SECTORS;
    if (flushed)
        return 0;
    if (!crash_part || !armed || !capturing || flushing || next_slot >= CRASH_SLOTS)
        return -1;
    flushing = 1; /* Nested faults must not reenter controller takeover. */
    panic_header.magic = CRASH_MAGIC;
    panic_header.version = CRASH_VERSION;
    panic_header.text_length = 0;
    panic_header.length = panic_header.stack_length;
    panic_header.sequence = next_sequence;
    panic_header.payload_crc = crash_crc(panic_text + CRASH_TEXT_BYTES, panic_header.length);
    panic_header.header_crc = header_crc(&panic_header);
    /* Invalidate only the destination; the other slot survives a failed write. */
    if (mmc_panic_begin(&panic_ctx) ||
        mmc_panic_write(&panic_ctx, sector, empty_sector, 1) ||
        (panic_header.length && mmc_panic_write(&panic_ctx, sector + 1, panic_text + CRASH_TEXT_BYTES,
                        (panic_header.length + CRASH_SECTOR - 1) / CRASH_SECTOR)) ||
        mmc_panic_write(&panic_ctx, sector, &panic_header, 1))
        return -1;
    flushed = 1;
    return 0;
}

void divoom_crash_log_print(void)
{
    printf("%.*s", (int)panic_length, panic_text);
}

static int write_all(int fd, const char *data, size_t length)
{
    while (length) {
        ssize_t n = write(fd, data, length);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        data += n;
        length -= n;
    }
    return 0;
}

static int file_matches(const char *path, const char *data, size_t length)
{
    char buf[256];
    int fd = open(path, O_RDONLY), result = -1;
    if (fd < 0)
        return -1;
    while (length) {
        size_t want = length < sizeof(buf) ? length : sizeof(buf);
        ssize_t n = read(fd, buf, want);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0 || memcmp(buf, data, n))
            goto out;
        data += n;
        length -= n;
    }
    if (read(fd, buf, 1) == 0)
        result = 0;
out:
    if (close(fd))
        result = -1;
    return result;
}

static int export_slot(unsigned int slot, struct crash_header *h, char *data)
{
    char path[128], tmp[136];
    struct stat st;
    int fd, ret, length;
    if (journal_io(0, slot, 0, h, sizeof(*h)))
        return -1;
    if (!header_valid(h))
        return 0;
    if ((h->length && journal_io(0, slot, 1, data,
                   (h->length + CRASH_SECTOR - 1) & ~(CRASH_SECTOR - 1))) ||
        crash_crc(data, h->length) != h->payload_crc) {
        printf("[crash] slot %u payload invalid; retained\n", slot);
        return -1;
    }
    /* Header and payload are one contiguous allocation; preserve exact bytes. */
    length = sizeof(*h) + h->length;
    snprintf(path, sizeof(path), CRASH_DIR "/crash-%08lx-%016llx-%08lx-%08lx.bin",
              (unsigned long)h->sequence, (unsigned long long)h->uptime_ns,
              (unsigned long)h->payload_crc, (unsigned long)h->header_crc);
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (stat(path, &st) != 0) {
        if (errno != ENOENT)
            return -1;
        /* A previous interrupted export may have left this same temporary file. */
        fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd < 0)
            return -1;
        ret = write_all(fd, (const char *)h, length);
        if (!ret)
            ret = fsync(fd);
        if (close(fd))
            ret = -1;
        if (ret || rename(tmp, path))
            return -1;
    }
    /* Covers both fresh exports and reset after rename but before invalidation. */
    if (file_matches(path, (const char *)h, length))
        return -1;
    if (journal_io(1, slot, 0, empty_sector, sizeof(empty_sector)) ||
        (crash_part->blk->sync && crash_part->blk->sync()))
        return -1;
    if (journal_io(0, slot, 0, h, sizeof(*h)) ||
        memcmp(h, empty_sector, sizeof(*h)))
        return -1;
    printf("[crash] saved %s; journal slot %u cleared\n", path, slot);
    return 1;
}

int divoom_crash_log_export(void)
{
    struct crash_header *h;
    char *data;
    unsigned int slot;
    int result = 0, exported = 0, ret;
    if (!crash_part || !armed)
        return -1;
    if (mkdir(CRASH_DIR, 0700) && errno != EEXIST) {
        printf("[crash] mkdir %s failed, errno=%d; journal retained\n", CRASH_DIR, errno);
        return -1;
    }
    h = malloc(sizeof(*h) + CRASH_STACK_BYTES);
    if (!h) {
        printf("[crash] export buffer allocation failed; journal retained\n");
        return -1;
    }
    data = (char *)(h + 1);
    /* A trap during export must not write while normal I/O changes slot state. */
    next_slot = CRASH_SLOTS;
    for (slot = 0; slot < CRASH_SLOTS; ++slot) {
        ret = export_slot(slot, h, data);
        if (ret < 0) {
            printf("[crash] slot %u export failed; journal retained for retry\n", slot);
            result = -1;
        } else
            exported += ret;
    }
    free(h);
    if (choose_slot()) {
        printf("[crash] slot refresh failed; capture disabled until retry\n");
        result = -1;
    }
    if (!result && !exported)
        printf("[crash] no valid pending records to export\n");
    return result;
}
