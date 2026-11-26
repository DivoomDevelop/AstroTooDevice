/* Private implementation: included once by ff.c to reuse its volume/dir core.
 * Read-only checks never call sync_fs/getfree or change volume metadata.
 * Journaled maintenance operations below are explicit mutation exceptions. */
#include "ffcheck.h"
#include <stdlib.h>
#include <string.h>
#define CK_BUDGET (256U * 1024U)
#define CK_DEPTH 64U
#define CK_MIRROR_SECTORS 32U
/* F101 SD DMA rejects unaligned reads larger than its 64 KiB bounce buffer. */
#define CK_DMA_ALIGN 64U
#define CK_SCAN 1
#define CK_COLLECT 2
#define CK_VERIFY 3
#define CK_GROUP 4
#define CK_NONE 0xffffffffU

#if FF_FS_REENTRANT
#define ck_unlock(fs) unlock_fs(fs, FR_OK)
#else
#define ck_unlock(fs) ((void)(fs))
#endif
typedef struct {
    FF_DIR dir;
    DWORD cluster, parent, lfn_start, lfn_end;
    BYTE lfn_ord, lfn_sum, lfn_bad, lfn_length, dot_mask, ended, root;
    WCHAR lfn[260];
} CK_DIR;
#define CK_OPEN_MAX 32
typedef struct {
    void *object;
    DWORD parent, offset;
    int writer;
} CK_OPEN;
typedef struct {
    FATFS *fs;
    DWORD overflow;
    int quiescing;
    CK_OPEN files[CK_OPEN_MAX];
} CK_OPEN_VOLUME;
static CK_OPEN_VOLUME ck_open_volumes[FF_VOLUMES];
static CK_OPEN_VOLUME *ck_open_volume(FATFS *fs, int create)
{
    UINT i;
    for (i = 0; i < FF_VOLUMES; i++)
        if (FatFs[i] == fs) {
            CK_OPEN_VOLUME *v = &ck_open_volumes[i];
            if (v->fs != fs) {
                if (!create)
                    return 0;
                memset(v, 0, sizeof(*v));
                v->fs = fs;
            }
            return v;
        }
    if (!create)
        for (i = 0; i < FF_VOLUMES; i++)
            if (ck_open_volumes[i].fs == fs)
                return &ck_open_volumes[i];
    return 0;
}
static int ck_new_object_allowed(FATFS *fs)
{
    CK_OPEN_VOLUME *v = ck_open_volume(fs, 0);
    return !v || !v->quiescing;
}
static void ck_opened(void *object, FATFS *fs, DWORD parent, DWORD offset, int writer)
{
    CK_OPEN_VOLUME *v = ck_open_volume(fs, 1);
    UINT i;
    if (!v)
        return;
    for (i = 0; i < CK_OPEN_MAX; i++)
        if (!v->files[i].object) {
            v->files[i].object = object;
            v->files[i].parent = parent;
            v->files[i].offset = offset;
            v->files[i].writer = writer;
            return;
        }
    v->overflow++;
}
static void ck_closed(void *object, FATFS *fs)
{
    CK_OPEN_VOLUME *v = ck_open_volume(fs, 0);
    UINT i;
    if (!v)
        return;
    for (i = 0; i < CK_OPEN_MAX; i++)
        if (v->files[i].object == object) {
            v->files[i].object = 0;
            return;
        }
    if (v->overflow)
        v->overflow--;
}
static void ck_volume_gone(FATFS *fs)
{
    CK_OPEN_VOLUME *v = ck_open_volume(fs, 0);
    if (v)
        memset(v, 0, sizeof(*v));
}
static int ck_any_open(FATFS *fs)
{
    CK_OPEN_VOLUME *v = ck_open_volume(fs, 0);
    UINT i;
    if (v) {
        if (v->overflow)
            return 1;
        for (i = 0; i < CK_OPEN_MAX; i++)
            if (v->files[i].object)
                return 1;
    }
#if FF_FS_LOCK
    if (file_lock_present(fs, 0) != 0)
        return 1;
#endif
    return 0;
}
void f_check_forget_file(void *opaque)
{
    FFOBJID *object = (FFOBJID *)opaque;
    FATFS *fs;
    if (!object || !(fs = object->fs))
        return;
#if FF_FS_REENTRANT
    /* VFS permanently releases this object after a failed close. A timeout
     * must not leave a dangling observer pointer: retain ownership and wait
     * until the existing blocking volume grant can be obtained. */
    while (!lock_fs(fs)) { }
#endif
    ck_closed(opaque, fs);
#if FF_FS_LOCK
    file_lock_forget(object);
#endif
    object->fs = 0;
    ck_unlock(fs);
}
int f_check_volume_idle(const char *drive)
{
    FATFS *fs;
    int busy;
    if (!drive || drive[0] < '0' || drive[0] >= '0' + FF_VOLUMES || drive[1] != ':' || drive[2])
        return FF_CHECK_UNSUPPORTED;
    fs = FatFs[drive[0] - '0'];
    if (!fs)
        return FF_CHECK_CLEAN;
#if FF_FS_REENTRANT
    if (!lock_fs(fs))
        return FF_CHECK_BUSY;
#endif
    busy = ck_any_open(fs);
    ck_unlock(fs);
    return busy ? FF_CHECK_BUSY : FF_CHECK_CLEAN;
}
/* Close the idle-probe/unmount race without changing global scheduling.
 * Only maintenance uses this local guard; live readers may close normally. */
int f_check_volume_quiesce(const char *drive, int enable)
{
    FATFS *fs;
    CK_OPEN_VOLUME *v;
    int busy;
    if (!drive || drive[0] < '0' || drive[0] >= '0' + FF_VOLUMES || drive[1] != ':' || drive[2])
        return FF_CHECK_UNSUPPORTED;
    fs = FatFs[drive[0] - '0'];
    if (!fs)
        return FF_CHECK_CLEAN;
#if FF_FS_REENTRANT
    if (!lock_fs(fs))
        return FF_CHECK_BUSY;
#endif
    v = ck_open_volume(fs, 1);
    busy = enable && ck_any_open(fs);
    if (v && !busy)
        v->quiescing = !!enable;
    ck_unlock(fs);
    return busy || !v ? FF_CHECK_BUSY : FF_CHECK_CLEAN;
}
typedef struct {
    FATFS *fs;
    ff_check_result *out;
    ff_check_entry *entries;
    const ff_check_entry *plan;
    DWORD capacity, plan_count, map_bytes, cache_sectors, cache_sector, used_fat_bytes, used_fat_sectors;
    BYTE *allocation, *owned, *duplicate, *local, *cache, *scratch;
    CK_DIR *stack;
    ff_check_progress progress;
    void *context;
    int error, mode, foreign, unsafe, mirror_enabled, screen, live_writers, live_file;
    unsigned diagnostics;
    int (*visitor)(void *, DWORD, DWORD, DWORD, DWORD, DWORD, const BYTE *, int);
    void *visitor_context;
    ff_check_patch *patch;
    const ff_check_patch *verify_patch;
    int repair_scan, patch_verified, directory_uncertain;


} CK_CTX;

__attribute__((weak)) int ff_check_write_allowed(uint8_t drive)
{
    (void)drive;
    return 1;
}
static int ck_bit(const BYTE *map, DWORD n) { return (map[n >> 3] >> (n & 7)) & 1; }
static void ck_set(BYTE *map, DWORD n) { map[n >> 3] |= (BYTE)(1U << (n & 7)); }
/* At most 16 fault records per pass; no normal per-file logging. The raw
 * directory identity remains resolvable even for invalid names/LFN data. */
static void ck_diagnostic(CK_CTX *c, DWORD parent, DWORD offset, DWORD cluster, DWORD reason)
{
    if (c->diagnostics++ < 16)
        printf("[fat_check] drive=%u parent=%lu offset=%lu cluster=%lu reason=0x%lx fs_error=%lu\n",
               (unsigned)c->fs->pdrv, (unsigned long)parent, (unsigned long)offset,
               (unsigned long)cluster, (unsigned long)reason, (unsigned long)c->out->fs_error);
}
static int ck_io(CK_CTX *c, FRESULT error)
{
    c->out->fs_error = (uint32_t)error;
    ck_diagnostic(c, CK_NONE, CK_NONE, CK_NONE, 0);
    c->error = error == FR_TIMEOUT || error == FR_LOCKED ? FF_CHECK_BUSY
               : error == FR_NOT_ENOUGH_CORE             ? FF_CHECK_NOMEM
                                                         : FF_CHECK_IO;
    return c->error;
}
static void ck_notice(CK_CTX *c, DWORD stage, DWORD done, DWORD total)
{
    if (c->progress)
        c->progress(stage, done, total, c->context);
}
static BYTE *ck_align_dma(void *buffer)
{
    uintptr_t offset = (uintptr_t)buffer % CK_DMA_ALIGN;
    return (BYTE *)buffer + (CK_DMA_ALIGN - offset) % CK_DMA_ALIGN;
}
static int ck_setup(CK_CTX *c, FATFS *fs, ff_check_result *out, int screen)
{
    size_t maps, fixed, available;
    memset(c, 0, sizeof(*c));
    c->fs = fs;
    c->out = out;
    if (fs->fs_type != FS_FAT12 && fs->fs_type != FS_FAT16 && fs->fs_type != FS_FAT32)
        return FF_CHECK_UNSUPPORTED;
    if (fs->wflag)
        return FF_CHECK_BUSY;
    c->screen = screen;
    {
        CK_OPEN_VOLUME *v = ck_open_volume(fs, 0);
        UINT i;
        if (v) {
            if (v->overflow)
                return FF_CHECK_BUSY;
            for (i = 0; i < CK_OPEN_MAX; i++)
                if (v->files[i].object && v->files[i].writer) {
                    if (!screen)
                        return FF_CHECK_BUSY;
                    if (((FFOBJID *)v->files[i].object)->fs != fs ||
                        ((FFOBJID *)v->files[i].object)->id != fs->id)
                        return FF_CHECK_BUSY;
                    c->live_writers = 1;
                }
        }
    }
#if FF_FS_LOCK
    /* Writable objects have private dirty buffers that cannot be included in
     * an authoritative metadata snapshot. Readers are harmless under the lock. */
    {
        if (!screen && file_lock_present(fs, 1) != 0)
            return FF_CHECK_BUSY;
    }
#endif
    if (fs->n_fatent < 3 || !fs->csize)
        return FF_CHECK_UNSUPPORTED;
    uint64_t fat_bytes = fs->fs_type == FS_FAT12 ? ((uint64_t)fs->n_fatent * 3U + 1U) / 2U
                                                : (uint64_t)fs->n_fatent * (fs->fs_type == FS_FAT16 ? 2U : 4U);
    if (fat_bytes > (uint64_t)fs->fsize * SS(fs)) {
        out->damage_flags |= FF_CHECK_INVALID_LINK;
        out->unsafe = 1;
        return FF_CHECK_STRUCTURAL;
    }
    c->map_bytes = fs->n_fatent / 8U + (fs->n_fatent % 8U != 0);
    maps = (size_t)c->map_bytes * 3U;
    /* Align directory frames on RISC-V without rounding the bitmap spans. */
    maps = (maps + _Alignof(CK_DIR) - 1U) & ~(_Alignof(CK_DIR) - 1U);
    fixed = maps + sizeof(CK_DIR) * CK_DEPTH + SS(fs) + CK_DMA_ALIGN - 1U;
    if (fixed + SS(fs) > CK_BUDGET || fat_bytes > UINT32_MAX)
        return FF_CHECK_NOMEM;
    c->used_fat_bytes = (DWORD)fat_bytes;
    available = CK_BUDGET - fixed;
    if (available > 128U * 1024U)
        available = 128U * 1024U;
    c->cache_sectors = (DWORD)(available / SS(fs));
    c->used_fat_sectors = (DWORD)((fat_bytes + SS(fs) - 1U) / SS(fs));
    if (c->cache_sectors > c->used_fat_sectors)
        c->cache_sectors = c->used_fat_sectors;
    available = (size_t)c->cache_sectors * SS(fs);
    c->allocation = malloc(fixed + available);
    if (!c->allocation)
        return FF_CHECK_NOMEM;
    memset(c->allocation, 0, maps);
    c->owned = c->allocation;
    c->duplicate = c->owned + c->map_bytes;
    c->local = c->duplicate + c->map_bytes;
    c->stack = (CK_DIR *)(void *)(c->allocation + maps);
    c->cache = ck_align_dma(c->allocation + maps + sizeof(CK_DIR) * CK_DEPTH);
    c->scratch = c->cache + available;
    c->cache_sector = CK_NONE;
    c->mirror_enabled = fs->n_fats == 2;
    return FF_CHECK_CLEAN;
}
static void ck_release(CK_CTX *c)
{
    free(c->allocation);
    c->allocation = 0;
}
static int ck_cache_fat(CK_CTX *c, DWORD sector)
{
    FATFS *fs = c->fs;
    if (sector >= c->cache_sector && sector - c->cache_sector < c->cache_sectors)
        return 0;
    DWORD page = sector - sector % c->cache_sectors;
    UINT count = (UINT)c->cache_sectors;
    if (count > c->used_fat_sectors - page)
        count = (UINT)(c->used_fat_sectors - page);
    if (disk_read(fs->pdrv, c->cache, fs->fatbase + page, count) != RES_OK)
        return ck_io(c, FR_DISK_ERR);
    c->cache_sector = page;
    return 0;
}
static int ck_byte(CK_CTX *c, DWORD offset, BYTE *value)
{
    if (offset >= c->used_fat_bytes)
        return ck_io(c, FR_INT_ERR);
    int rc = ck_cache_fat(c, offset / SS(c->fs));
    if (rc)
        return rc;
    *value = c->cache[offset - c->cache_sector * SS(c->fs)];
    return 0;
}
static DWORD ck_fat(CK_CTX *c, DWORD cluster)
{
    BYTE a, b, d, e;
    DWORD offset;
    if (cluster >= c->fs->n_fatent)
        return 1;
    offset = c->fs->fs_type == FS_FAT12 ? cluster + cluster / 2U
                                        : cluster * (c->fs->fs_type == FS_FAT16 ? 2U : 4U);
    if (ck_byte(c, offset, &a) || ck_byte(c, offset + 1, &b))
        return 0xffffffffU;
    if (c->fs->fs_type == FS_FAT12)
        return cluster & 1U ? ((DWORD)a | ((DWORD)b << 8)) >> 4 : ((DWORD)a | ((DWORD)b << 8)) & 0xfffU;
    if (c->fs->fs_type == FS_FAT16)
        return (DWORD)a | ((DWORD)b << 8);
    if (ck_byte(c, offset + 2, &d) || ck_byte(c, offset + 3, &e))
        return 0xffffffffU;
    return ((DWORD)a | ((DWORD)b << 8) | ((DWORD)d << 16) | ((DWORD)e << 24)) & 0x0fffffffU;
}
static DWORD ck_eoc(CK_CTX *c)
{
    return c->fs->fs_type == FS_FAT12 ? 0xff8U : c->fs->fs_type == FS_FAT16 ? 0xfff8U : 0x0ffffff8U;
}
static DWORD ck_bad(CK_CTX *c) { return ck_eoc(c) - 1U; }
static int ck_geometry(CK_CTX *c)
{
    FATFS *fs = c->fs;
    ff_check_geometry *g = &c->out->geometry;
    if (disk_read(fs->pdrv, c->scratch, fs->volbase, 1) != RES_OK)
        return ck_io(c, FR_DISK_ERR);
    memset(g, 0, sizeof(*g));
    g->serial = ld_dword(c->scratch + (fs->fs_type == FS_FAT32 ? 67 : 39));
    g->fat_type = fs->fs_type == FS_FAT12 ? 12U : fs->fs_type == FS_FAT16 ? 16U : 32U;
    g->sector_size = SS(fs);
    g->cluster_sectors = fs->csize;
    g->fat_entries = fs->n_fatent;
    g->fat_sectors = fs->fsize;
    g->volume_base = fs->volbase;
    g->fat_base = fs->fatbase;
    g->directory_base = fs->dirbase;
    g->data_base = fs->database;
    if (fs->fs_type == FS_FAT32 && (ld_word(c->scratch + 40) & 0x80)) {
        /* Non-mirrored FAT32 copies are permitted by the format. This FatFs
         * version uses FAT0; do not misclassify another active FAT as damage. */
        if (ld_word(c->scratch + 40) & 0xf)
            return FF_CHECK_UNSUPPORTED;
        c->mirror_enabled = 0;
    }
    return 0;
}
static int ck_mirrors(CK_CTX *c)
{
    FATFS *fs = c->fs;
    DWORD sector = 0, offset = 0;
    DWORD skip = fs->fs_type == FS_FAT12 ? 3U : fs->fs_type == FS_FAT16 ? 4U : 8U;
    /* Directory frames are unused here and initialized by ck_init_dir. */
    BYTE *mirror = ck_align_dma(c->stack);
    UINT limit = (UINT)((sizeof(CK_DIR) * CK_DEPTH - (CK_DMA_ALIGN - 1U)) / SS(fs));
    if (!c->mirror_enabled)
        return 0;
    if (limit > CK_MIRROR_SECTORS)
        limit = CK_MIRROR_SECTORS;
    while (offset < c->used_fat_bytes) {
        DWORD left = c->used_fat_bytes - offset;
        UINT count = (UINT)(c->used_fat_sectors - sector);
        int rc = ck_cache_fat(c, sector);
        if (rc)
            return rc;
        DWORD in_page = sector - c->cache_sector;
        if (count > c->cache_sectors - in_page)
            count = (UINT)(c->cache_sectors - in_page);
        if (count > limit)
            count = limit;
        if (disk_read(fs->pdrv, mirror, fs->fatbase + fs->fsize + sector, count) != RES_OK)
            return ck_io(c, FR_DISK_ERR);
        const BYTE *first = c->cache + in_page * SS(fs);
        UINT n = count * SS(fs);
        if (n > left)
            n = (UINT)left;
        if (memcmp(first, mirror, n) != 0) {
            for (UINT i = 0; i < n; i++) {
                DWORD index = offset + i;
                BYTE mask = 0xff;
                if (index < skip)
                    continue;
                if (fs->fs_type == FS_FAT32 && (index & 3U) == 3U)
                    mask = 0x0f;
                if (fs->fs_type == FS_FAT12 && (fs->n_fatent & 1U) && index + 1U == c->used_fat_bytes)
                    mask = 0x0f;
                if ((first[i] & mask) != (mirror[i] & mask)) {
                    c->out->damage_flags |= FF_CHECK_MIRROR;
                    c->out->unsafe = 1;
                    return 0;
                }
            }
        }
        sector += count;
        offset += n;
    }
    return 0;
}
static DWORD ck_walk(CK_CTX *c, DWORD start, DWORD size, int directory)
{
    DWORD cl = start, count = 0, reason = 0, value;
    if (!cl)
        return size || directory ? FF_CHECK_INVALID_LINK : 0;
    if (cl < 2 || cl >= c->fs->n_fatent)
        return FF_CHECK_INVALID_LINK;
    memset(c->local, 0, c->map_bytes);
    while (cl >= 2 && cl < c->fs->n_fatent) {
        if (ck_bit(c->local, cl)) {
            reason |= FF_CHECK_CYCLE;
            if (c->mode == CK_SCAN)
                ck_set(c->duplicate, cl);
            break;
        }
        ck_set(c->local, cl);
        if (c->mode == CK_SCAN) {
            if (ck_bit(c->owned, cl)) {
                ck_set(c->duplicate, cl);
                reason |= FF_CHECK_CROSS_LINK;
                break;
            }
            ck_set(c->owned, cl);
        } else if (c->mode == CK_COLLECT) {
            if (ck_bit(c->duplicate, cl)) {
                reason |= FF_CHECK_CROSS_LINK;
                break;
            }
        } else if (c->mode == CK_VERIFY) {
            if (c->foreign && ck_bit(c->duplicate, cl))
                c->unsafe = 1;
            if (ck_bit(c->owned, cl) && !ck_bit(c->duplicate, cl))
                c->unsafe = 1;
            ck_set(c->owned, cl);
        }
        count++;
        value = ck_fat(c, cl);
        if (c->error)
            break;
        if (value >= ck_eoc(c))
            break;
        if (value == 0 || value == 1 || value == ck_bad(c) || value >= c->fs->n_fatent) {
            reason |= FF_CHECK_INVALID_LINK;
            break;
        }
        cl = value;
    }
    if (!directory && reason == 0 && !(c->screen && c->live_file)) {
        uint64_t cluster_bytes = (uint64_t)c->fs->csize * SS(c->fs);
        if (count != ((uint64_t)size + cluster_bytes - 1U) / cluster_bytes)
            reason |= FF_CHECK_FILE_SIZE;
    }
    if (c->mode == CK_SCAN) {
        c->out->damage_flags |= reason;
        if (reason & FF_CHECK_CROSS_LINK)
            c->out->cross_links++;
        if (reason & FF_CHECK_CYCLE)
            c->out->cycles++;
    } else if (c->mode == CK_VERIFY && reason && c->foreign)
        c->unsafe = 1;
    return reason;
}
static BYTE ck_sum(const BYTE *entry)
{
    BYTE sum = 0;
    UINT i;
    for (i = 0; i < 11; i++)
        sum = (BYTE)((sum >> 1) | ((sum & 1) << 7)) + entry[i];
    return sum;
}
static int ck_init_dir(CK_CTX *c, CK_DIR *d, DWORD cluster, DWORD parent, int root)
{
    FRESULT res;
    memset(d, 0, sizeof(*d));
    d->cluster = cluster;
    d->parent = parent;
    d->root = (BYTE)root;
    d->lfn_start = CK_NONE;
    d->dir.obj.fs = c->fs;
    d->dir.obj.id = c->fs->id;
    d->dir.obj.sclust = cluster;
    res = dir_sdi(&d->dir, 0);
    return res == FR_OK ? 0 : ck_io(c, res);
}
static int ck_is_plan(CK_CTX *c, DWORD parent, DWORD offset, const BYTE *entry)
{
    DWORD i;
    for (i = 0; i < c->plan_count; i++)
        if (c->plan[i].parent_cluster == parent && c->plan[i].entry_offset == offset &&
            !memcmp(c->plan[i].original, entry, 32))
            return 1;
    return 0;
}
static int ck_live_file(CK_CTX *c, DWORD parent, DWORD offset, DWORD *first, DWORD *size)
{
    CK_OPEN_VOLUME *v = ck_open_volume(c->fs, 0);
    UINT i;
    int found = 0;
    if (!c->screen || !v)
        return 0;
    for (i = 0; i < CK_OPEN_MAX; i++)
        if (v->files[i].object && v->files[i].writer && v->files[i].parent == parent &&
            v->files[i].offset == offset) {
            FFOBJID *obj = (FFOBJID *)v->files[i].object;
            if (found && *first != obj->sclust)
                return -1;
            *first = obj->sclust;
            if (!found || obj->objsize > *size)
                *size = (DWORD)obj->objsize;
            found = 1;
        }
    return found;
}
static int ck_lfn_valid(const CK_DIR *d)
{
    UINT i, limit = (UINT)d->lfn_length * 13U;
    int ended = 0;
    for (i = 0; i < limit; i++) {
        WCHAR ch = d->lfn[i];
        if (ended) {
            if (ch != 0xffff)
                return 0;
            continue;
        }
        if (ch == 0) {
            ended = 1;
            continue;
        }
        if (ch == 0xffff || i >= 255 || ch < 32 || ch == '"' || ch == '*' || ch == '/' || ch == ':' ||
            ch == '<' || ch == '>' || ch == '?' || ch == 0x5c || ch == '|')
            return 0;
        if (ch >= 0xd800 && ch <= 0xdbff) {
            if (i + 1 >= limit || d->lfn[i + 1] < 0xdc00 || d->lfn[i + 1] > 0xdfff)
                return 0;
            i++;
        } else if (ch >= 0xdc00 && ch <= 0xdfff)
            return 0;
    }
    return limit > 0;
}
static int ck_sfn_valid(const BYTE *e)
{
    UINT i;
    if (e[0] == ' ')
        return 0;
    for (i = 0; i < 11; i++) {
        BYTE ch = e[i];
        if (i == 0 && ch == 0x05)
            continue;
        if (ch < 32 || ch == '"' || ch == '*' || ch == '+' || ch == ',' || ch == '/' || ch == ':' ||
            ch == ';' || ch == '<' || ch == '=' || ch == '>' || ch == '?' || ch == '[' || ch == ']' ||
            ch == '|' || ch == 0x5c)
            return 0;
    }
    return 1;
}
/* Propose one directory identity at a time. A sector record keeps unrelated
 * bytes exact; only a single 32-byte entry can differ in this patch kind. */
static int ck_patch_entry(CK_CTX *c,DWORD parent,DWORD offset,const BYTE *before,const BYTE *after)
{
    FF_DIR dir;
    FRESULT res;
    if(!memcmp(before,after,32))return 0;
    if(c->verify_patch) {
        const ff_check_patch *p=c->verify_patch;
        if(parent==p->parent_cluster && offset==p->entry_offset &&
                !memcmp(before,p->identity,32) &&
                !memcmp(after,p->after+offset%SS(c->fs),32))c->patch_verified=1;
        return 0;
    }
    if(!c->patch || c->patch->kind)return 0;
    memset(&dir,0,sizeof(dir));dir.obj.fs=c->fs;dir.obj.id=c->fs->id;dir.obj.sclust=parent;
    res=dir_sdi(&dir,offset);
    if(res!=FR_OK)return ck_io(c,res);
    if(disk_read(c->fs->pdrv,c->scratch,dir.sect,1)!=RES_OK)return ck_io(c,FR_DISK_ERR);
    ff_check_patch *p=c->patch;
    memset(p,0,sizeof(*p));p->geometry=c->out->geometry;p->kind=FF_CHECK_PATCH_DIRECTORY;
    p->parent_cluster=parent;p->entry_offset=offset;memcpy(p->identity,before,32);
    memcpy(p->before,c->scratch,SS(c->fs));memcpy(p->after,p->before,SS(c->fs));
    memcpy(p->after+offset%SS(c->fs),after,32);
    return 0;
}
static int ck_patch_lfn(CK_CTX *c,DWORD parent,DWORD begin,DWORD end)
{
    for(DWORD offset=begin;offset<end;offset+=32) {
        FF_DIR dir;BYTE before[32],after[32];FRESULT res;
        memset(&dir,0,sizeof(dir));dir.obj.fs=c->fs;dir.obj.id=c->fs->id;dir.obj.sclust=parent;
        res=dir_sdi(&dir,offset);if(res!=FR_OK)return ck_io(c,res);
        res=move_window(c->fs,dir.sect);if(res!=FR_OK)return ck_io(c,res);
        memcpy(before,c->fs->win+dir.dptr%SS(c->fs),32);
        if(c->verify_patch && parent==c->verify_patch->parent_cluster && offset==c->verify_patch->entry_offset)
            memcpy(before,c->verify_patch->identity,32);
        if(before[0]==0 || before[0]==DDEM || before[DIR_Attr]!=AM_LFN)continue;
        memcpy(after,before,32);after[0]=DDEM;
        if(ck_patch_entry(c,parent,offset,before,after))return c->error;
    }
    return 0;
}
static int ck_dot_name(const BYTE *entry,int dot)
{
    UINT i;
    if(entry[0]!='.' || entry[1]!=(dot?' ':'.'))return 0;
    for(i=dot?1U:2U;i<11;i++)if(entry[i]!=' ')return 0;
    return 1;
}
static int ck_tree(CK_CTX *c)
{
    UINT depth = 0;
    FRESULT res;
    DWORD reason;
    if (c->mode == CK_SCAN)
        c->out->directories = 1;
    if (c->fs->fs_type == FS_FAT32) {
        c->foreign = 1;
        reason = ck_walk(c, c->fs->dirbase, 0, 1);
        if (reason) {
            c->out->damage_flags |= reason | FF_CHECK_DIRECTORY;
            c->out->unsafe = 1;c->directory_uncertain=1;
            return 0;
        }
    }
    if (c->visitor && c->visitor(c->visitor_context, 0, CK_NONE, CK_NONE,
            c->fs->fs_type == FS_FAT32 ? c->fs->dirbase : 0, 0, 0, 1))
        return c->error ? c->error : FF_CHECK_STRUCTURAL;
    if (ck_init_dir(c, &c->stack[0], 0, 0, 1))
        return c->error;
    for (;;) {
        CK_DIR *d = &c->stack[depth];
        BYTE e[32], attr;
        DWORD offset, first, size, lfn, base_reason = 0;
        if (d->ended || d->dir.sect == 0) {
            if (d->lfn_start != CK_NONE) {
                if(c->repair_scan && ck_patch_lfn(c,d->cluster,d->lfn_start,d->lfn_end))return c->error;
                c->out->damage_flags |= FF_CHECK_LFN | FF_CHECK_DIRECTORY;
                c->out->unsafe = 1;
            }
            if (!d->root && d->dot_mask != 3) {
                c->directory_uncertain=1;
                c->out->damage_flags |= FF_CHECK_DIRECTORY;
                c->out->unsafe = 1;
            }
            if (!depth)
                break;
            depth--;
            continue;
        }
        res = move_window(c->fs, d->dir.sect);
        if (res != FR_OK)
            return ck_io(c, res);
        memcpy(e, c->fs->win + d->dir.dptr % SS(c->fs), 32);
        offset = d->dir.dptr;
        if(c->verify_patch && d->cluster==c->verify_patch->parent_cluster && offset==c->verify_patch->entry_offset)
            memcpy(e,c->verify_patch->identity,32);
        res = dir_next(&d->dir, 0);
        if (res == FR_NO_FILE)
            d->ended = 1;
        else if (res != FR_OK)
            return ck_io(c, res);
        if (e[0] == 0) {
            d->ended = 1;
            continue;
        }
        if (e[0] == DDEM) {
            if(c->repair_scan && d->lfn_start!=CK_NONE && ck_patch_lfn(c,d->cluster,d->lfn_start,offset))return c->error;
            d->lfn_start = CK_NONE;
            d->lfn_bad = 0;
            d->lfn_ord = 0;
            continue;
        }
        attr = e[DIR_Attr];
        if(c->repair_scan && !d->root && (offset==0 || offset==32) && ck_dot_name(e,offset==0)) {
            BYTE after[32];DWORD expected=offset==0?d->cluster:d->parent;
            memcpy(after,e,32);after[DIR_Attr]=AM_DIR;
            st_clust(c->fs,after,expected);st_dword(after+DIR_FileSize,0);
            if(ck_patch_entry(c,d->cluster,offset,e,after))return c->error;
            memcpy(e,after,32);attr=e[DIR_Attr];
        }
        if (attr == AM_LFN) {
            d->lfn_end=offset+32;
            BYTE ord = e[0] & 0x1f;
            if (d->lfn_start == CK_NONE) {
                d->lfn_start = offset;
                d->lfn_bad = (e[0] & 0x40) ? 0 : 1;
                d->lfn_ord = ord;
                d->lfn_length = ord;
                d->lfn_sum = e[13];
                memset(d->lfn, 0xff, sizeof(d->lfn));
            } else if ((e[0] & 0x40) != 0)
                d->lfn_bad = 1;
            if (!ord || ord > 20 || ord != d->lfn_ord || e[13] != d->lfn_sum || e[12] || ld_word(e + 26))
                d->lfn_bad = 1;
            if (ord && ord <= 20) {
                static const BYTE offsets[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
                UINT k;
                for (k = 0; k < 13; k++)
                    d->lfn[(ord - 1) * 13 + k] = ld_word(e + offsets[k]);
            }
            d->lfn_ord = ord ? ord - 1 : 0;
            continue;
        }
        lfn = d->lfn_start;
        if (lfn != CK_NONE && (d->lfn_bad || d->lfn_ord || d->lfn_sum != ck_sum(e) || !ck_lfn_valid(d))) {
            base_reason |= FF_CHECK_LFN;
            if(c->repair_scan && ck_patch_lfn(c,d->cluster,lfn,offset))return c->error;
        }
        d->lfn_start = CK_NONE;
        d->lfn_bad = 0;
        d->lfn_ord = 0;
        first = ld_clust(c->fs, e);
        size = ld_dword(e + DIR_FileSize);
        if (e[0] == '.' && (e[1] == ' ' || (e[1] == '.' && e[2] == ' '))) {
            int dot = e[1] == ' ';
            DWORD expected = dot ? d->cluster : d->parent;
            if (d->root || !(attr & AM_DIR) || size || first != expected || offset != (dot ? 0U : 32U)) {
                c->out->damage_flags |= FF_CHECK_DIRECTORY;
                c->out->unsafe = 1;
                if(c->repair_scan && !d->root && ck_dot_name(e,dot) && offset==(dot?0U:32U)) {
                    BYTE after[32];memcpy(after,e,32);after[DIR_Attr]=AM_DIR;
                    st_clust(c->fs,after,expected);st_dword(after+DIR_FileSize,0);
                    if(ck_patch_entry(c,d->cluster,offset,e,after))return c->error;
                    d->dot_mask|=(BYTE)(dot?1:2);
                } else c->directory_uncertain=1;
            } else
                d->dot_mask |= (BYTE)(dot ? 1 : 2);
            continue;
        }
        if (attr & AM_VOL) {
            if ((attr & AM_DIR) || size || first) {
                c->directory_uncertain=1;
                c->out->damage_flags |= FF_CHECK_DIRECTORY;
                c->out->unsafe = 1;
            }
            continue;
        }
        if ((attr & 0xc0) || !ck_sfn_valid(e))
            base_reason |= FF_CHECK_LFN;
        if (attr & AM_DIR) {
            if (c->mode == CK_SCAN)
                c->out->directories++;
            c->foreign = 1;
            if(c->repair_scan && !(attr&0xc0) && (first<2 || first>=c->fs->n_fatent)) {
                BYTE after[32];memcpy(after,e,32);after[0]=DDEM;
                if(lfn!=CK_NONE && ck_patch_lfn(c,d->cluster,lfn,offset))return c->error;
                if(ck_patch_entry(c,d->cluster,offset,e,after))return c->error;
                c->out->damage_flags|=FF_CHECK_DIRECTORY;
                continue; /* No valid child chain exists; never infer children. */
            }
            reason = ck_walk(c, first, 0, 1) |
                (c->repair_scan && ck_sfn_valid(e) && !(attr&0xc0)?0:base_reason);
            if(c->repair_scan && size && !reason) {
                BYTE after[32];memcpy(after,e,32);st_dword(after+DIR_FileSize,0);
                if(ck_patch_entry(c,d->cluster,offset,e,after))return c->error;
                size=0;
            }
            if (size || reason) {
                c->directory_uncertain=1;
                if (c->mode == CK_SCAN)
                    ck_diagnostic(c, d->cluster, offset, first, reason | FF_CHECK_DIRECTORY);
                c->out->damage_flags |= reason | FF_CHECK_DIRECTORY;
                c->out->unsafe = 1;
                continue;
            }
            if (depth + 1 >= CK_DEPTH) {
                c->error = FF_CHECK_UNSUPPORTED;
                return c->error;
            }
            if (c->visitor && c->visitor(c->visitor_context, d->cluster, offset, lfn, first, size, e, 1))
                return c->error ? c->error : FF_CHECK_STRUCTURAL;
            depth++;
            if (ck_init_dir(c, &c->stack[depth], first, d->cluster, 0))
                return c->error;
        } else {
            if (c->mode == CK_SCAN)
                c->out->files++;
            c->foreign = c->mode == CK_VERIFY ? !ck_is_plan(c, d->cluster, offset, e) : 1;
            c->live_file = ck_live_file(c, d->cluster, offset, &first, &size);
            if (c->live_file < 0) {
                c->error = FF_CHECK_BUSY;
                return c->error;
            }
            reason = ck_walk(c, first, size, 0) | base_reason;
            if (c->mode == CK_SCAN)
                c->out->damage_flags |= reason;
            if (c->mode == CK_VERIFY && base_reason && c->foreign)
                c->unsafe = 1;
            if (c->visitor && c->visitor(c->visitor_context, d->cluster, offset, lfn, first, size, e, 0))
                return c->error ? c->error : FF_CHECK_STRUCTURAL;
            if (c->mode == CK_COLLECT && reason) {
                ck_diagnostic(c, d->cluster, offset, first, reason);
                DWORD index = c->out->affected_files++;
                if (index < c->capacity && c->entries) {
                    ff_check_entry *item = &c->entries[index];
                    memset(item, 0, sizeof(*item));
                    item->parent_cluster = d->cluster;
                    item->entry_offset = offset;
                    item->lfn_offset = lfn;
                    item->reason = reason;
                    memcpy(item->original, e, 32);
                } else
                    c->out->plan_complete = 0;
            }
        }
        if (c->error)
            return c->error;
        if (c->mode == CK_SCAN && ((c->out->files & 31U) == 0))
            ck_notice(c, FF_CHECK_STAGE_DIRECTORY, c->out->files, 0);
    }
    return 0;
}
static int ck_all_fat(CK_CTX *c, int verify)
{
    DWORD n, value;
    for (n = 2; n < c->fs->n_fatent; n++) {
        value = ck_fat(c, n);
        if (c->error)
            return c->error;
        if (value == 0 || value == ck_bad(c))
            continue;
        if (!ck_bit(c->owned, n)) {
            if ((!verify || !ck_bit(c->duplicate, n)) && !c->live_writers) {
                c->out->orphan_clusters++;
                ck_diagnostic(c, CK_NONE, CK_NONE, n, FF_CHECK_ORPHAN);
                c->out->damage_flags |= FF_CHECK_ORPHAN;
                c->out->unsafe = 1;
            }
        }
        if (value == 1 || (value < ck_eoc(c) && value >= c->fs->n_fatent)) {
            c->out->damage_flags |= FF_CHECK_INVALID_LINK;
            if (!ck_bit(c->owned, n) && (!verify || !ck_bit(c->duplicate, n)))
                c->out->unsafe = 1;
        }
        if ((n & 1023U) == 0)
            ck_notice(c, FF_CHECK_STAGE_FAT, n, c->fs->n_fatent);
    }
    return 0;
}
static int ck_open(const char *drive, FATFS **fs)
{
    TCHAR logical[4];
    const TCHAR *path;
    FRESULT res;
    if (!drive || drive[0] < '0' || drive[0] > '9' || drive[1] != ':' || drive[2])
        return FF_CHECK_UNSUPPORTED;
    logical[0] = (TCHAR)drive[0];
    logical[1] = ':';
    logical[2] = 0;
    path = logical;
    res = find_volume(&path, fs, 0);
    return res == FR_OK                            ? 0
           : res == FR_TIMEOUT || res == FR_LOCKED ? FF_CHECK_BUSY
           : res == FR_NOT_ENOUGH_CORE             ? FF_CHECK_NOMEM
           : res == FR_NO_FILESYSTEM               ? FF_CHECK_STRUCTURAL
                                                   : FF_CHECK_IO;
}
static int ck_check_volume(const char *drive, ff_check_result *out, ff_check_entry *entries,
                           uint32_t capacity, ff_check_progress progress, void *context, int screen)
{
    FATFS *fs = 0;
    CK_CTX c;
    int status;
    if (!out)
        return FF_CHECK_UNSUPPORTED;
    memset(out, 0, sizeof(*out));
    status = ck_open(drive, &fs);
    if (status) {
        out->fs_error = status == FF_CHECK_STRUCTURAL ? FR_NO_FILESYSTEM : FR_DISK_ERR;
        if (status == FF_CHECK_STRUCTURAL) {
            out->damage_flags = FF_CHECK_INVALID_LINK;
            out->unsafe = 1;
        }
        if (fs)
            ck_unlock(fs);
        return status;
    }
    out->screen_only = !!screen;
    status = ck_setup(&c, fs, out, screen);
    if (!status) {
        c.entries = entries;
        c.capacity = capacity;
        c.progress = progress;
        c.context = context;
        out->plan_complete = 1;
        status = ck_geometry(&c);
        if (!status)
            status = ck_mirrors(&c);
        c.mode = CK_SCAN;
        if (!status)
            status = ck_tree(&c);
        if (!status)
            status = ck_all_fat(&c, 0);
        /* A clean first pass has no affected owners to collect. */
        if (!status && out->damage_flags) {
            c.mode = CK_COLLECT;
            status = ck_tree(&c);
        }
        if (!status) {
            out->complete = 1;
            if (out->unsafe)
                out->plan_complete = 0;
            status = out->damage_flags ? FF_CHECK_STRUCTURAL : FF_CHECK_CLEAN;
        }
    }
    if (screen)
        out->plan_complete = 0;
    ck_release(&c);
    ck_unlock(fs);
    return status;
}
int f_check_volume(const char *drive, ff_check_result *out, ff_check_entry *entries, uint32_t capacity,
                   ff_check_progress progress, void *context)
{
    return ck_check_volume(drive, out, entries, capacity, progress, context, 0);
}
int f_screen_volume(const char *drive, ff_check_result *out, ff_check_progress progress, void *context)
{
    return ck_check_volume(drive, out, 0, 0, progress, context, 1);
}
/* The primary FAT is already cached in CK_CTX. Reuse its DMA-aligned scratch
 * sector to inspect the second copy without another allocation. */
static int ck_orphan_mirror_byte(CK_CTX *c, DWORD offset, DWORD *cached, BYTE *value)
{
    DWORD sector = offset / SS(c->fs);
    if (sector != *cached) {
        if (disk_read(c->fs->pdrv, c->scratch,
                c->fs->fatbase + c->fs->fsize + sector, 1) != RES_OK)
            return ck_io(c, FR_DISK_ERR);
        *cached = sector;
    }
    *value = c->scratch[offset % SS(c->fs)];
    return 0;
}
static int ck_orphan_plan(CK_CTX *c, int resume)
{
    DWORD cl, cached = CK_NONE;
    for (cl = 2; cl < c->fs->n_fatent; ++cl) {
        DWORD primary = ck_fat(c, cl), mirror = primary;
        if (c->error) return c->error;
        if (c->mirror_enabled) {
            BYTE a, b, d, e;
            DWORD offset = c->fs->fs_type == FS_FAT12 ? cl + cl / 2U :
                cl * (c->fs->fs_type == FS_FAT16 ? 2U : 4U);
            if (ck_orphan_mirror_byte(c, offset, &cached, &a) ||
                ck_orphan_mirror_byte(c, offset + 1U, &cached, &b)) return c->error;
            mirror = (DWORD)a | ((DWORD)b << 8);
            if (c->fs->fs_type == FS_FAT12)
                mirror = cl & 1U ? mirror >> 4 : mirror & 0xfffU;
            else if (c->fs->fs_type == FS_FAT32) {
                if (ck_orphan_mirror_byte(c, offset + 2U, &cached, &d) ||
                    ck_orphan_mirror_byte(c, offset + 3U, &cached, &e)) return c->error;
                mirror = (mirror | ((DWORD)d << 16) | ((DWORD)e << 24)) & 0x0fffffffU;
            }
            /* A durable interrupted reclaim can only affect unowned entries.
             * FAT12 entries crossing sectors may be partly zeroed, so do not
             * require either copy to contain a complete old/new value. */
            if (primary != mirror && (!resume || ck_bit(c->owned, cl) ||
                    primary == ck_bad(c) || mirror == ck_bad(c))) {
                c->out->damage_flags |= FF_CHECK_MIRROR;
                c->out->unsafe = 1;
                return FF_CHECK_STRUCTURAL;
            }
        }
        if (!ck_bit(c->owned, cl) && (primary || mirror) && primary != ck_bad(c))
            ck_set(c->duplicate, cl);
    }
    return 0;
}
static BYTE ck_orphan_mask(CK_CTX *c, DWORD offset)
{
    DWORD cl;
    BYTE mask;
    if (c->fs->fs_type == FS_FAT12) {
        cl = offset / 3U * 2U;
        mask = 0;
        if (offset % 3U != 2U && cl >= 2 && cl < c->fs->n_fatent && ck_bit(c->duplicate, cl))
            mask |= offset % 3U == 0 ? 0xffU : 0x0fU;
        ++cl;
        if (offset % 3U != 0 && cl < c->fs->n_fatent && ck_bit(c->duplicate, cl))
            mask |= offset % 3U == 1 ? 0xf0U : 0xffU;
        return mask;
    }
    cl = offset / (c->fs->fs_type == FS_FAT16 ? 2U : 4U);
    if (cl < 2 || cl >= c->fs->n_fatent || !ck_bit(c->duplicate, cl)) return 0;
    return c->fs->fs_type == FS_FAT32 && (offset & 3U) == 3U ? 0x0fU : 0xffU;
}
/* FAT32 FSInfo is an optional allocation hint, not ownership evidence. Raw
 * reclamation invalidates both known counters; never publish a guessed count
 * or create an FSInfo sector where no valid structure already exists. */
static int ck_orphan_fsinfo(CK_CTX *c)
{
    FATFS *fs=c->fs;
    DWORD offsets[2],reserved,info,backup;
    if(fs->fs_type!=FS_FAT32)return 0;
    fs->free_clst=fs->last_clst=0xffffffffU;
    fs->winsect=(DWORD)0-1U;
    if(disk_read(fs->pdrv,c->scratch,fs->volbase,1)!=RES_OK)return ck_io(c,FR_DISK_ERR);
    reserved=ld_word(c->scratch+BPB_RsvdSecCnt);
    info=ld_word(c->scratch+BPB_FSInfo32);
    backup=ld_word(c->scratch+BPB_BkBootSec32);
    if(!info || info==0xffffU || info>=reserved)return 0;
    offsets[0]=info;offsets[1]=0;
    if(backup && backup!=0xffffU && backup<reserved && backup+info<reserved)
        offsets[1]=backup+info;
    for(UINT copy=0;copy<2;copy++) {
        DWORD offset=offsets[copy],address;
        if(!offset || offset>0xffffffffU-fs->volbase)continue;
        address=fs->volbase+offset;
        if(disk_read(fs->pdrv,c->scratch,address,1)!=RES_OK)return ck_io(c,FR_DISK_ERR);
        if(ld_word(c->scratch+BS_55AA)!=0xaa55 ||
                ld_dword(c->scratch+FSI_LeadSig)!=0x41615252 ||
                ld_dword(c->scratch+FSI_StrucSig)!=0x61417272)continue;
        if(ld_dword(c->scratch+FSI_Free_Count)==0xffffffffU &&
                ld_dword(c->scratch+FSI_Nxt_Free)==0xffffffffU)continue;
        st_dword(c->scratch+FSI_Free_Count,0xffffffffU);
        st_dword(c->scratch+FSI_Nxt_Free,0xffffffffU);
        /* Preserve all signatures, reserved bytes and sector padding. */
        if(disk_write(fs->pdrv,c->scratch,address,1)!=RES_OK ||
                disk_ioctl(fs->pdrv,CTRL_SYNC,0)!=RES_OK)return ck_io(c,FR_DISK_ERR);
        c->cache_sector=CK_NONE;
        if(disk_read(fs->pdrv,c->cache,address,1)!=RES_OK ||
                memcmp(c->cache,c->scratch,SS(fs)))return ck_io(c,FR_DISK_ERR);
    }
    fs->fsi_flag&=0x80; /* No dirty cached count may overwrite the durable unknown hint. */
    return 0;
}
int f_reclaim_orphans(const char *drive, const ff_check_geometry *geometry, int resume,
                     ff_check_result *out, ff_check_progress progress, ff_check_commit commit,
                     void *context)
{
    FATFS *fs = 0;
    CK_CTX c;
    DWORD cl, sector, count = 0;
    int status;
    if (!geometry || !out || !commit) return FF_CHECK_UNSUPPORTED;
    memset(out, 0, sizeof(*out));
    status = ck_open(drive, &fs);
    if (status) {
        out->fs_error = status == FF_CHECK_IO ? FR_DISK_ERR :
            status == FF_CHECK_STRUCTURAL ? FR_NO_FILESYSTEM : 0;
        if (fs) ck_unlock(fs);
        return status;
    }
    status = ck_setup(&c, fs, out, 0);
    if (!status && (ff_check_write_allowed(fs->pdrv) || ck_any_open(fs)))
        status = FF_CHECK_BUSY;
    if (!status) status = ck_geometry(&c);
    if (!status && memcmp(geometry, &out->geometry, sizeof(*geometry)))
        status = FF_CHECK_STRUCTURAL;
    c.progress = progress;
    c.context = context;
    c.mode = CK_SCAN;
    if (!status) status = ck_tree(&c);
    /* Never infer ownership from a broken directory or referenced chain. */
    if (!status && out->damage_flags) status = FF_CHECK_STRUCTURAL;
    if (!status) status = ck_all_fat(&c, 0);
    if (!status && (out->damage_flags & ~(FF_CHECK_ORPHAN | FF_CHECK_INVALID_LINK))) status = FF_CHECK_STRUCTURAL;
    if (!status) status = ck_orphan_plan(&c, resume);
    for (cl = 2; !status && cl < fs->n_fatent; ++cl)
        if (ck_bit(c.duplicate, cl)) ++count;
    if (!status && (count || (resume && fs->fs_type==FS_FAT32)) &&
            !commit(FF_CHECK_ORPHAN_PENDING, context))
        status = FF_CHECK_IO; /* The independent journal is not target-volume I/O. */
    for (sector = 0; !status && count && sector < c.used_fat_sectors; ++sector) {
        UINT copy, copies = c.mirror_enabled ? 2U : 1U;
        for (copy = 0; !status && copy < copies; ++copy) {
            DWORD address = fs->fatbase + copy * fs->fsize + sector;
            UINT i;
            int changed = 0;
            if (disk_read(fs->pdrv, c.scratch, address, 1) != RES_OK) {
                status = ck_io(&c, FR_DISK_ERR);
                break;
            }
            for (i = 0; i < SS(fs); ++i) {
                BYTE mask = ck_orphan_mask(&c, sector * SS(fs) + i);
                if (c.scratch[i] & mask) { c.scratch[i] &= (BYTE)~mask; changed = 1; }
            }
            if (changed && disk_write(fs->pdrv, c.scratch, address, 1) != RES_OK)
                status = ck_io(&c, FR_DISK_ERR);
        }
        ck_notice(&c, FF_CHECK_STAGE_VERIFY, sector + 1U, c.used_fat_sectors);
    }
    if (count) {
        /* Discard any pre-reclaim FAT window, including after an I/O failure. */
        fs->winsect = (DWORD)0 - 1U;
        fs->free_clst = 0xffffffffU;
        fs->last_clst = 0xffffffffU;
    }
    /* A previous attempt may have cleared every entry before CTRL_SYNC failed.
     * A no-op resume must still make those earlier writes durable. */
    if (!status && (count || resume) && disk_ioctl(fs->pdrv, CTRL_SYNC, 0) != RES_OK)
        status = ck_io(&c, FR_DISK_ERR);
    /* CTRL_SYNC success alone cannot prove the medium retained the clear. */
    for (sector=0; !status && count && sector<c.used_fat_sectors; sector++) {
        UINT copies=c.mirror_enabled?2U:1U;
        for(UINT copy=0;!status && copy<copies;copy++) {
            if(disk_read(fs->pdrv,c.scratch,fs->fatbase+copy*fs->fsize+sector,1)!=RES_OK) {
                status=ck_io(&c,FR_DISK_ERR);break;
            }
            for(UINT i=0;i<SS(fs);i++)if(c.scratch[i]&ck_orphan_mask(&c,sector*SS(fs)+i)) {
                status=ck_io(&c,FR_DISK_ERR);break;
            }
        }
    }
    /* Resume also repairs a hint left stale after the final FAT write. The
     * same durable ORPHAN_PENDING checkpoint covers these idempotent hints. */
    if(!status && (count || resume))status=ck_orphan_fsinfo(&c);
    if (!status) {
        out->damage_flags = out->unsafe = 0;
        out->orphan_clusters = count;
        out->complete = 1;
    }
    ck_release(&c);
    ck_unlock(fs);
    return status;
}
static int ck_plan_entry(CK_CTX *c, const ff_check_entry *item, FF_DIR *d, int *present)
{
    FRESULT res;
    BYTE *entry;
    if ((item->entry_offset & 31U) ||
        (item->lfn_offset != CK_NONE && ((item->lfn_offset & 31U) || item->lfn_offset > item->entry_offset ||
                                         item->entry_offset - item->lfn_offset > 20U * 32U)))
        return FF_CHECK_STRUCTURAL;
    memset(d, 0, sizeof(*d));
    d->obj.fs = c->fs;
    d->obj.id = c->fs->id;
    d->obj.sclust = item->parent_cluster;
    res = dir_sdi(d, item->entry_offset);
    if (res != FR_OK)
        return ck_io(c, res);
    res = move_window(c->fs, d->sect);
    if (res != FR_OK)
        return ck_io(c, res);
    entry = c->fs->win + d->dptr % SS(c->fs);
    if (entry[0] == DDEM) {
        if (memcmp(entry + 1, item->original + 1, 31))
            return FF_CHECK_STRUCTURAL;
        *present = 0;
    } else {
        if (memcmp(entry, item->original, 32) || (entry[DIR_Attr] & AM_DIR))
            return FF_CHECK_STRUCTURAL;
        *present = 1;
    }
    if (item->lfn_offset != CK_NONE) {
        DWORD offset;
        FF_DIR part;
        for (offset = item->lfn_offset; offset < item->entry_offset; offset += 32) {
            memset(&part, 0, sizeof(part));
            part.obj.fs = c->fs;
            part.obj.sclust = item->parent_cluster;
            res = dir_sdi(&part, offset);
            if (res != FR_OK)
                return ck_io(c, res);
            res = move_window(c->fs, part.sect);
            if (res != FR_OK)
                return ck_io(c, res);
            entry = c->fs->win + part.dptr % SS(c->fs);
            if (entry[0] != DDEM && entry[DIR_Attr] != AM_LFN)
                return FF_CHECK_STRUCTURAL;
        }
    }
    return 0;
}
static int ck_detach(CK_CTX *c, const ff_check_entry *item)
{
    DWORD offset = item->lfn_offset == CK_NONE ? item->entry_offset : item->lfn_offset;
    FF_DIR part;
    FRESULT res;
    for (; offset <= item->entry_offset; offset += 32) {
        memset(&part, 0, sizeof(part));
        part.obj.fs = c->fs;
        part.obj.sclust = item->parent_cluster;
        res = dir_sdi(&part, offset);
        if (res != FR_OK)
            return ck_io(c, res);
        res = move_window(c->fs, part.sect);
        if (res != FR_OK)
            return ck_io(c, res);
        c->fs->win[part.dptr % SS(c->fs)] = DDEM;
        c->fs->wflag = 1;
    }
    return 0;
}
static int ck_union(CK_CTX *c, const ff_check_entry *entry)
{
    DWORD cl = ld_clust(c->fs, entry->original), value;
    /* The locked volume is immutable here. A visited cluster's chain has
     * already been covered, including shared tails and cycles. */
    while (cl >= 2 && cl < c->fs->n_fatent && !ck_bit(c->duplicate, cl)) {
        value = ck_fat(c, cl);
        if (c->error)
            return c->error;
        if (value == ck_bad(c))
            return FF_CHECK_STRUCTURAL;
        ck_set(c->duplicate, cl);
        if (value >= ck_eoc(c) || value <= 1 || value >= c->fs->n_fatent)
            break;
        cl = value;
    }
    return 0;
}
int f_delete_corrupt_batch(const char *drive, const ff_check_geometry *geometry,
                           const ff_check_entry *entries, uint32_t count, ff_check_commit commit,
                           void *context)
{
    FATFS *fs = 0;
    CK_CTX c;
    ff_check_result result;
    DWORD i, cl, value;
    int status, present;
    FF_DIR dir;
    FRESULT res;
    if (!geometry || !entries || !count || count > 1024 || !commit)
        return FF_CHECK_UNSUPPORTED;
    memset(&result, 0, sizeof(result));
    status = ck_open(drive, &fs);
    if (status) {
        if (fs)
            ck_unlock(fs);
        return status;
    }
    status = ck_setup(&c, fs, &result, 0);
    if (!status && (ff_check_write_allowed(fs->pdrv) || ck_any_open(fs)))
        status = FF_CHECK_BUSY;
    if (!status)
        status = ck_geometry(&c);
    if (!status && memcmp(geometry, &result.geometry, sizeof(*geometry)))
        status = FF_CHECK_STRUCTURAL;
    for (i = 0; !status && i < count; i++) {
        DWORD j;
        for (j = 0; j < i; j++)
            if (entries[j].parent_cluster == entries[i].parent_cluster &&
                entries[j].entry_offset == entries[i].entry_offset) {
                status = FF_CHECK_STRUCTURAL;
                break;
            }
        if (!status)
            status = ck_plan_entry(&c, &entries[i], &dir, &present);
        if (!status)
            status = ck_union(&c, &entries[i]);
    }
    if (!status) {
        c.plan = entries;
        c.plan_count = count;
        c.mode = CK_VERIFY;
        status = ck_mirrors(&c);
        if (!status)
            status = ck_tree(&c);
        if (!status)
            status = ck_all_fat(&c, 1);
        if (c.unsafe || result.unsafe)
            status = FF_CHECK_STRUCTURAL;
    }
    if (!status && !commit(FF_CHECK_DETACH_PENDING, context))
        status = FF_CHECK_IO;
    for (i = 0; !status && i < count; i++) {
        status = ck_plan_entry(&c, &entries[i], &dir, &present);
        if (!status && present)
            status = ck_detach(&c, &entries[i]);
    }
    if (!status) {
        res = sync_fs(fs);
        if (res != FR_OK)
            status = ck_io(&c, res);
    }
    if (!status && !commit(FF_CHECK_FREE_PENDING, context))
        status = FF_CHECK_IO;
    for (cl = 2; !status && cl < fs->n_fatent; cl++)
        if (ck_bit(c.duplicate, cl)) {
            value = ck_fat(&c, cl);
            if (c.error) {
                status = c.error;
                break;
            }
            if (value) {
                res = put_fat(fs, cl, 0);
                if (res != FR_OK) {
                    status = ck_io(&c, res);
                    break;
                }
                if (fs->free_clst < fs->n_fatent - 2)
                    fs->free_clst++;
                fs->fsi_flag |= 1;
            }
        }
    if (!status) {
        res = sync_fs(fs);
        if (res != FR_OK)
            status = ck_io(&c, res);
    }
    ck_release(&c);
    ck_unlock(fs);
    return status;
}


/* A group never changes FAT. The persisted seed therefore still describes the
 * complete original chain after any number of directory-only batches. */
#define CK_GROUP_MAX 240U
typedef struct {
    CK_CTX *c;
    const ff_check_group *group;
    const ff_check_entry *plan;
    DWORD plan_count;
    BYTE seen[CK_GROUP_MAX], parents[CK_GROUP_MAX];
} CK_GROUP_VISIT;
static int ck_group_chain(CK_CTX *c, DWORD first, int build)
{
    DWORD cl=first;
    int hit=0;
    memset(c->local,0,c->map_bytes);
    while(cl>=2 && cl<c->fs->n_fatent && !ck_bit(c->local,cl)) {
        DWORD next;
        ck_set(c->local,cl);
        if(build)ck_set(c->duplicate,cl);
        else if(ck_bit(c->duplicate,cl))hit=1;
        next=ck_fat(c,cl);
        if(c->error)return -1;
        if(next<2 || next>=c->fs->n_fatent || next>=ck_eoc(c) || next==ck_bad(c))break;
        cl=next;
    }
    return hit;
}
static int ck_group_visit(void *opaque,DWORD parent,DWORD offset,DWORD lfn,
    DWORD first,DWORD size,const BYTE *entry,int directory)
{
    CK_GROUP_VISIT *v=opaque;
    CK_CTX *c=v->c;
    DWORD i;
    int hit;
    (void)size;
    if(directory) {
        DWORD cluster=entry?first:0;
        for(i=0;i<v->plan_count;i++)
            if(v->plan[i].parent_cluster==cluster)v->parents[i]=1;
        if(!(v->group->flags&FF_CHECK_GROUP_IDENTITY) && ck_group_chain(c,first,0))
            return FF_CHECK_STRUCTURAL;
        return c->error;
    }
    hit=(v->group->flags&FF_CHECK_GROUP_IDENTITY)?
        parent==v->group->identity.parent_cluster && offset==v->group->identity.entry_offset &&
        !memcmp(entry,v->group->identity.original,32):ck_group_chain(c,first,0);
    if(c->error)return c->error;
    for(i=0;i<v->plan_count;i++)
        if(parent==v->plan[i].parent_cluster && offset==v->plan[i].entry_offset) {
            if(!hit || memcmp(entry,v->plan[i].original,32))return FF_CHECK_STRUCTURAL;
            v->seen[i]=1;
        }
    if(hit) {
        DWORD index=c->out->affected_files++;
        if(c->entries && index<c->capacity) {
            ff_check_entry *item=&c->entries[index];
            memset(item,0,sizeof(*item));item->parent_cluster=parent;item->entry_offset=offset;
            item->lfn_offset=lfn;item->reason=FF_CHECK_CROSS_LINK;
            memcpy(item->original,entry,32);
            c->out->batch_files++;
        }
    }
    return 0;
}
static int ck_group_proof(CK_CTX *c)
{
    int status;
    c->mode=CK_SCAN;
    status=ck_mirrors(c);
    if(!status)status=ck_tree(c);
    if(!status && (c->out->damage_flags&(FF_CHECK_DIRECTORY|FF_CHECK_MIRROR)))
        status=FF_CHECK_STRUCTURAL;
    /* Unreferenced chains stay allocated until all directory groups finish. */
    return status;
}
int f_check_group_next(const char *drive,ff_check_group *group,ff_check_entry *entries,
    uint32_t capacity,ff_check_result *out,ff_check_progress progress,void *context)
{
    FATFS *fs=0;
    CK_CTX c;
    CK_GROUP_VISIT visit;
    ff_check_entry seed;
    int status;
    if(!group || !out || !entries || !capacity || capacity>CK_GROUP_MAX)return FF_CHECK_UNSUPPORTED;
    memset(out,0,sizeof(*out));status=ck_open(drive,&fs);
    if(status){if(fs)ck_unlock(fs);return status;}
    status=ck_setup(&c,fs,out,0);
    if(!status && (ff_check_write_allowed(fs->pdrv)||ck_any_open(fs)))status=FF_CHECK_BUSY;
    if(!status)status=ck_geometry(&c);
    if(!status && (group->flags&FF_CHECK_GROUP_ACTIVE) && memcmp(&group->geometry,&out->geometry,sizeof(group->geometry)))status=FF_CHECK_STRUCTURAL;
    c.progress=progress;c.context=context;
    if(!status)status=ck_group_proof(&c);
    if(!status && !(group->flags&FF_CHECK_GROUP_ACTIVE)) {
        c.mode=CK_COLLECT;c.entries=&seed;c.capacity=1;out->plan_complete=1;
        status=ck_tree(&c);
        if(!status && out->affected_files) {
            memset(group,0,sizeof(*group));group->geometry=out->geometry;group->identity=seed;
            group->seed_cluster=ld_clust(fs,seed.original);group->flags=FF_CHECK_GROUP_ACTIVE;
            if(group->seed_cluster<2 || group->seed_cluster>=fs->n_fatent) {
                group->seed_cluster=0;group->flags|=FF_CHECK_GROUP_IDENTITY;
            }
        }
    }
    if(!status && (group->flags&FF_CHECK_GROUP_ACTIVE)) {
        memset(c.duplicate,0,c.map_bytes);
        if(!(group->flags&FF_CHECK_GROUP_IDENTITY))ck_group_chain(&c,group->seed_cluster,1);
        if(c.error)status=c.error;
        memset(&visit,0,sizeof(visit));visit.c=&c;visit.group=group;
        c.mode=CK_GROUP;c.entries=entries;c.capacity=capacity;c.visitor=ck_group_visit;c.visitor_context=&visit;
        out->affected_files=out->batch_files=0;
        if(!status)status=ck_tree(&c);
    }
    if(!status) {
        out->complete=1;out->plan_complete=out->affected_files==out->batch_files;
        status=out->batch_files?FF_CHECK_STRUCTURAL:FF_CHECK_CLEAN;
    }
    ck_release(&c);ck_unlock(fs);return status;
}
int f_check_group_detach(const char *drive,const ff_check_group *group,
    const ff_check_entry *entries,uint32_t count,int resume,ff_check_result *out,
    ff_check_commit commit,void *context)
{
    FATFS *fs=0;
    CK_CTX c;
    CK_GROUP_VISIT visit;
    DWORD i,j;
    int status,present;
    FF_DIR dir;
    if(!group || !(group->flags&FF_CHECK_GROUP_ACTIVE) || !entries || !out || !commit || !count || count>CK_GROUP_MAX)
        return FF_CHECK_UNSUPPORTED;
    memset(out,0,sizeof(*out));status=ck_open(drive,&fs);
    if(status){if(fs)ck_unlock(fs);return status;}
    status=ck_setup(&c,fs,out,0);
    if(!status && (ff_check_write_allowed(fs->pdrv)||ck_any_open(fs)))status=FF_CHECK_BUSY;
    if(!status)status=ck_geometry(&c);
    if(!status && memcmp(&group->geometry,&out->geometry,sizeof(group->geometry)))status=FF_CHECK_STRUCTURAL;
    if(!status)status=ck_group_proof(&c);
    memset(&visit,0,sizeof(visit));visit.c=&c;visit.group=group;visit.plan=entries;visit.plan_count=count;
    if(!status) {
        memset(c.duplicate,0,c.map_bytes);
        if(!(group->flags&FF_CHECK_GROUP_IDENTITY))ck_group_chain(&c,group->seed_cluster,1);
        if(c.error)status=c.error;
        c.mode=CK_GROUP;c.visitor=ck_group_visit;c.visitor_context=&visit;
        out->affected_files=0;
        if(!status)status=ck_tree(&c);
    }
    for(i=0;!status && i<count;i++) {
        for(j=0;j<i;j++)if(entries[j].parent_cluster==entries[i].parent_cluster && entries[j].entry_offset==entries[i].entry_offset)status=FF_CHECK_STRUCTURAL;
        if(!status && !visit.parents[i])status=FF_CHECK_STRUCTURAL;
        if(!status)status=ck_plan_entry(&c,&entries[i],&dir,&present);
        if(!status && (present?!visit.seen[i]:!resume))status=FF_CHECK_STRUCTURAL;
    }
    if(!status && !commit(FF_CHECK_GROUP_PENDING,context))status=FF_CHECK_IO;
    for(i=0;!status && i<count;i++) {
        status=ck_plan_entry(&c,&entries[i],&dir,&present);
        if(!status)status=ck_detach(&c,&entries[i]);
    }
    if(!status) {
        FRESULT res=sync_fs(fs);
        if(res!=FR_OK)status=ck_io(&c,res);
    }
    fs->winsect=(DWORD)0-1U;
    for(i=0;!status && i<count;i++) {
        status=ck_plan_entry(&c,&entries[i],&dir,&present);
        if(!status && present)status=ck_io(&c,FR_DISK_ERR);
        DWORD offset=entries[i].lfn_offset;
        for(;!status && offset!=CK_NONE && offset<entries[i].entry_offset;offset+=32) {
            FF_DIR part;FRESULT res;
            memset(&part,0,sizeof(part));part.obj.fs=fs;part.obj.id=fs->id;part.obj.sclust=entries[i].parent_cluster;
            res=dir_sdi(&part,offset);if(res==FR_OK)res=move_window(fs,part.sect);
            if(res!=FR_OK)status=ck_io(&c,res);
            else if(fs->win[part.dptr%SS(fs)]!=DDEM)status=ck_io(&c,FR_DISK_ERR);
        }
    }
    if(!status){out->complete=1;out->batch_files=count;}
    ck_release(&c);ck_unlock(fs);return status;
}


static void ck_repair_reset(CK_CTX *c)
{
    ff_check_geometry geometry=c->out->geometry;
    memset(c->out,0,sizeof(*c->out));c->out->geometry=geometry;
    memset(c->owned,0,c->map_bytes*3U);
    c->error=c->unsafe=c->foreign=c->directory_uncertain=0;
    c->cache_sector=CK_NONE;c->mode=CK_SCAN;
    c->fs->winsect=(DWORD)0-1U;
}
static BYTE ck_mirror_mask(CK_CTX *c,DWORD offset)
{
    DWORD skip=c->fs->fs_type==FS_FAT12?3U:c->fs->fs_type==FS_FAT16?4U:8U;
    if(offset<skip || offset>=c->used_fat_bytes)return 0;
    if(c->fs->fs_type==FS_FAT32 && (offset&3U)==3U)return 0x0f;
    if(c->fs->fs_type==FS_FAT12 && (c->fs->n_fatent&1U) && offset+1U==c->used_fat_bytes)return 0x0f;
    return 0xff;
}
/* Reuse one workspace sequentially. Directory traversal itself must use the
 * candidate FAT too; changing only the checker's cache would not prove it. */
static int ck_mirror_candidate(CK_CTX *c,UINT copy,int *valid)
{
    DWORD base=c->fs->fatbase;
    int status;
    *valid=0;ck_repair_reset(c);c->fs->fatbase=base+copy*c->fs->fsize;
    status=ck_tree(c);
    /* Reference topology must be entirely sound. Only after that proof can
     * all_fat's unowned orphan/invalid entries be deferred to reclamation;
     * they do not make a usable reference FAT ambiguous by themselves. */
    int references_ok=!status && !c->out->damage_flags && !c->directory_uncertain;
    if(references_ok)status=ck_all_fat(c,0);
    if(!status)*valid=references_ok &&
        !(c->out->damage_flags&~(FF_CHECK_ORPHAN|FF_CHECK_INVALID_LINK));
    DWORD fs_error=c->out->fs_error;
    c->fs->fatbase=base;ck_repair_reset(c);
    if(status){c->error=status;c->out->fs_error=fs_error;}
    return status;
}
static int ck_mirror_trust(CK_CTX *c,int *trusted)
{
    int valid[2]={0,0},status=0;
    *trusted=-1;
    if(!c->mirror_enabled)return 0;
    for(UINT copy=0;copy<2 && !status;copy++)status=ck_mirror_candidate(c,copy,&valid[copy]);
    if(!status && valid[0]!=valid[1])*trusted=valid[0]?0:1;
    return status;
}
static int ck_mirror_propose(CK_CTX *c,ff_check_patch *patch,int trusted)
{
    int status=0,valid=0;
    if(trusted<0)status=ck_mirror_trust(c,&trusted);
    else {
        status=ck_mirror_candidate(c,(UINT)trusted,&valid);
        if(!status && !valid)trusted=-1;
    }
    if(status || trusted<0)return status;
    for(DWORD sector=0;sector<c->used_fat_sectors;sector++) {
        int changed=0;
        if(disk_read(c->fs->pdrv,c->cache,c->fs->fatbase+trusted*c->fs->fsize+sector,1)!=RES_OK ||
                disk_read(c->fs->pdrv,c->scratch,c->fs->fatbase+(1-trusted)*c->fs->fsize+sector,1)!=RES_OK)
            return ck_io(c,FR_DISK_ERR);
        for(UINT i=0;i<SS(c->fs);i++) {
            BYTE mask=ck_mirror_mask(c,sector*SS(c->fs)+i);
            if((c->cache[i]&mask)!=(c->scratch[i]&mask)){changed=1;break;}
        }
        if(!changed)continue;
        memset(patch,0,sizeof(*patch));patch->geometry=c->out->geometry;
        patch->kind=FF_CHECK_PATCH_MIRROR;patch->source_fat=(uint32_t)trusted;patch->fat_sector=sector;
        memcpy(patch->before,c->scratch,SS(c->fs));
        for(UINT i=0;i<SS(c->fs);i++) {
            BYTE mask=ck_mirror_mask(c,sector*SS(c->fs)+i);
            patch->after[i]=(BYTE)((patch->before[i]&~mask)|(c->cache[i]&mask));
        }
        return 0;
    }
    return 0;
}
static int ck_patch_directory_trusted(CK_CTX *c,const ff_check_patch *patch)
{
    DWORD parent=patch->parent_cluster;
    if(!parent && c->fs->fs_type==FS_FAT32)parent=c->fs->dirbase;
    if(ck_group_chain(c,parent,0))return 0;
    if((patch->identity[DIR_Attr]&AM_DIR) && patch->after[patch->entry_offset%SS(c->fs)]!=DDEM &&
            !(patch->identity[0]=='.' && (patch->entry_offset==0 || patch->entry_offset==32))) {
        if(ck_group_chain(c,ld_clust(c->fs,patch->identity),0))return 0;
    }
    return !c->error;
}
int f_check_repair_next(const char *drive,ff_check_result *out,ff_check_patch *patch,
    ff_check_progress progress,void *context)
{
    FATFS *fs=0;CK_CTX c;int status;
    if(!out || !patch)return FF_CHECK_UNSUPPORTED;
    memset(out,0,sizeof(*out));status=ck_open(drive,&fs);
    if(status){if(fs)ck_unlock(fs);return status;}
    status=ck_setup(&c,fs,out,0);
    if(!status && (ff_check_write_allowed(fs->pdrv)||ck_any_open(fs)))status=FF_CHECK_BUSY;
    if(!status && SS(fs)>FF_CHECK_PATCH_MAX)status=FF_CHECK_UNSUPPORTED;
    if(!status)status=ck_geometry(&c);
    int trusted=-1;
    if(!status && patch->kind==FF_CHECK_PATCH_MIRROR) {
        if(memcmp(&out->geometry,&patch->geometry,sizeof(patch->geometry)) ||
                !c.mirror_enabled || patch->source_fat>1 || patch->fat_sector>=c.used_fat_sectors)
            status=FF_CHECK_STRUCTURAL;
        else {
            DWORD base=fs->fatbase+patch->fat_sector;
            if(disk_read(fs->pdrv,c.scratch,base+(1U-patch->source_fat)*fs->fsize,1)!=RES_OK ||
                    disk_read(fs->pdrv,c.cache,base+patch->source_fat*fs->fsize,1)!=RES_OK)status=ck_io(&c,FR_DISK_ERR);
            if(!status && memcmp(c.scratch,patch->after,SS(fs)))status=FF_CHECK_STRUCTURAL;
            for(UINT i=0;!status && i<SS(fs);i++) {
                BYTE mask=ck_mirror_mask(&c,patch->fat_sector*SS(fs)+i);
                if((c.cache[i]&mask)!=(patch->after[i]&mask))status=FF_CHECK_STRUCTURAL;
            }
            if(!status)trusted=(int)patch->source_fat;
            c.cache_sector=CK_NONE;
        }
    }
    memset(patch,0,sizeof(*patch));
    c.progress=progress;c.context=context;
    if(!status)status=ck_mirrors(&c);
    if(!status && (out->damage_flags&FF_CHECK_MIRROR))status=ck_mirror_propose(&c,patch,trusted);
    else if(!status) {
        c.mode=CK_SCAN;c.repair_scan=1;c.patch=patch;
        status=ck_tree(&c);
        if(!status && patch->kind && !ck_patch_directory_trusted(&c,patch)) {
            if(c.error)status=c.error;
            patch->kind=0;
        }
        if(c.directory_uncertain)patch->kind=0;
    }
    if(!status){out->complete=1;status=patch->kind?FF_CHECK_STRUCTURAL:FF_CHECK_CLEAN;}
    ck_release(&c);ck_unlock(fs);return status;
}
static int ck_patch_address(CK_CTX *c,const ff_check_patch *patch,DWORD *address)
{
    if(patch->kind==FF_CHECK_PATCH_MIRROR) {
        if(!c->mirror_enabled || patch->source_fat>1 || patch->fat_sector>=c->used_fat_sectors)return FF_CHECK_STRUCTURAL;
        *address=c->fs->fatbase+(1U-patch->source_fat)*c->fs->fsize+patch->fat_sector;
        return 0;
    }
    if(patch->kind==FF_CHECK_PATCH_DIRECTORY) {
        FF_DIR dir;FRESULT res;
        if(patch->entry_offset&31U)return FF_CHECK_STRUCTURAL;
        memset(&dir,0,sizeof(dir));dir.obj.fs=c->fs;dir.obj.id=c->fs->id;dir.obj.sclust=patch->parent_cluster;
        res=dir_sdi(&dir,patch->entry_offset);if(res!=FR_OK)return ck_io(c,res);
        *address=dir.sect;return 0;
    }
    return FF_CHECK_STRUCTURAL;
}
int f_check_repair_apply(const char *drive,const ff_check_patch *patch,int resume,
    ff_check_result *out,ff_check_commit commit,void *context)
{
    FATFS *fs=0;CK_CTX c;DWORD address=0;int status,already=0;
    if(!patch || !out || !commit)return FF_CHECK_UNSUPPORTED;
    memset(out,0,sizeof(*out));status=ck_open(drive,&fs);
    if(status){if(fs)ck_unlock(fs);return status;}
    status=ck_setup(&c,fs,out,0);
    if(!status && (ff_check_write_allowed(fs->pdrv)||ck_any_open(fs)))status=FF_CHECK_BUSY;
    if(!status && SS(fs)>FF_CHECK_PATCH_MAX)status=FF_CHECK_UNSUPPORTED;
    if(!status)status=ck_geometry(&c);
    if(!status && memcmp(&out->geometry,&patch->geometry,sizeof(patch->geometry)))status=FF_CHECK_STRUCTURAL;
    if(!status)status=ck_patch_address(&c,patch,&address);
    if(!status && disk_read(fs->pdrv,c.scratch,address,1)!=RES_OK)status=ck_io(&c,FR_DISK_ERR);
    if(!status) {
        already=!memcmp(c.scratch,patch->after,SS(fs));
        if(already && !resume)status=FF_CHECK_STRUCTURAL;
        for(UINT i=0;!status && i<SS(fs);i++)
            if(c.scratch[i]!=patch->before[i] && (!resume || c.scratch[i]!=patch->after[i]))status=FF_CHECK_STRUCTURAL;
    }
    if(!status && !already && patch->kind==FF_CHECK_PATCH_DIRECTORY) {
        UINT position=patch->entry_offset%SS(fs);
        if(memcmp(patch->before+position,patch->identity,32))status=FF_CHECK_STRUCTURAL;
        for(UINT i=0;!status && i<SS(fs);i++)
            if((i<position || i>=position+32U) && patch->before[i]!=patch->after[i])status=FF_CHECK_STRUCTURAL;
        if(!status) {
            c.repair_scan=1;c.verify_patch=patch;c.mode=CK_SCAN;
            status=ck_mirrors(&c);
            if(!status && (out->damage_flags&FF_CHECK_MIRROR))status=FF_CHECK_STRUCTURAL;
            if(!status)status=ck_tree(&c);
            if(!status && (!c.patch_verified || c.directory_uncertain || !ck_patch_directory_trusted(&c,patch)))
                status=c.error?c.error:FF_CHECK_STRUCTURAL;
        }
    } else if(!status && !already && patch->kind==FF_CHECK_PATCH_MIRROR) {
        int valid;
        /* Source authority was established by repair_next and persisted with
         * this patch. It must remain valid, but the partially repaired peer
         * may now be viable too; never choose a different source mid-replay. */
        status=ck_mirror_candidate(&c,patch->source_fat,&valid);
        if(!status && !valid)status=FF_CHECK_STRUCTURAL;
        if(!status && disk_read(fs->pdrv,c.cache,fs->fatbase+patch->source_fat*fs->fsize+patch->fat_sector,1)!=RES_OK)
            status=ck_io(&c,FR_DISK_ERR);
        for(UINT i=0;!status && i<SS(fs);i++) {
            BYTE mask=ck_mirror_mask(&c,patch->fat_sector*SS(fs)+i);
            if(patch->after[i]!=(BYTE)((patch->before[i]&~mask)|(c.cache[i]&mask)))status=FF_CHECK_STRUCTURAL;
        }
    }
    if(!status && !commit(FF_CHECK_PATCH_PENDING,context))status=FF_CHECK_IO;
    if(!status && !already) {
        memcpy(c.scratch,patch->after,SS(fs));
        if(disk_write(fs->pdrv,c.scratch,address,1)!=RES_OK)status=ck_io(&c,FR_DISK_ERR);
        fs->winsect=(DWORD)0-1U;
        if(patch->kind==FF_CHECK_PATCH_MIRROR){fs->free_clst=fs->last_clst=0xffffffffU;}
    }
    if(!status && disk_ioctl(fs->pdrv,CTRL_SYNC,0)!=RES_OK)status=ck_io(&c,FR_DISK_ERR);
    if(!status && (disk_read(fs->pdrv,c.scratch,address,1)!=RES_OK ||
            memcmp(c.scratch,patch->after,SS(fs))))status=ck_io(&c,FR_DISK_ERR);
    if(!status)out->complete=1;
    ck_release(&c);ck_unlock(fs);return status;
}
