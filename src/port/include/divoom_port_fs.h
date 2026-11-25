#ifndef DIVOOM_PORT_FS_H
#define DIVOOM_PORT_FS_H

#include <stddef.h>
#include <stdint.h>

#define DIVOOM_PORT_SEEK_SET  0
#define DIVOOM_PORT_SEEK_CUR  1
#define DIVOOM_PORT_SEEK_END  2

/*
 * Sequential file copy/write chunk. Sized for FatFs CMD25 (multi-sector)
 * on SDNAND; keep close/f_sync unchanged. Must stay a multiple of 512.
 */
#ifndef DIVOOM_PORT_WRITE_CHUNK
#define DIVOOM_PORT_WRITE_CHUNK (16 * 1024)
#endif

typedef struct divoom_port_stat {
	long size;
	long mtime;
	unsigned int mode;
	int is_dir;
} divoom_port_stat_t;

typedef struct divoom_port_dirent {
	const char *name;
	int is_dir;
} divoom_port_dirent_t;

typedef enum divoom_port_dir_change_type {
	DIVOOM_PORT_DIR_CHANGE_ADDED = 1,
	DIVOOM_PORT_DIR_CHANGE_REMOVED = 2,
	DIVOOM_PORT_DIR_CHANGE_MODIFIED = 3,
} divoom_port_dir_change_type_t;

typedef struct divoom_port_dir_change {
	divoom_port_dir_change_type_t type;
	/* Root-relative UTF-8 path. Valid only for the duration of the callback. */
	const char *relative_path;
	uint64_t old_size;
	uint64_t new_size;
	uint32_t old_crc32;
	uint32_t new_crc32;
} divoom_port_dir_change_t;

typedef int (*divoom_port_dir_change_cb)(
	const divoom_port_dir_change_t *change, void *user_data);

typedef struct divoom_port_dir_compare_result {
	uint32_t manifest_file_count;
	uint32_t current_file_count;
	uint32_t manifest_crc32;
	uint32_t current_crc32;
	uint32_t change_count;
} divoom_port_dir_compare_result_t;

enum {
	DIVOOM_PORT_DIR_COMPARE_MATCH = 0,
	DIVOOM_PORT_DIR_COMPARE_DIFFERENT = 1,
	DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT = -1,
	DIVOOM_PORT_DIR_COMPARE_ERR_IO = -2,
	DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT = -3,
	DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM = -4,
	DIVOOM_PORT_DIR_COMPARE_ERR_PATH_TOO_LONG = -5,
	DIVOOM_PORT_DIR_COMPARE_ERR_CALLBACK = -6,
	DIVOOM_PORT_DIR_COMPARE_ERR_CHANGED = -7,
    DIVOOM_PORT_DIR_COMPARE_ERR_NOT_FOUND = -8,
};

/*
 * Map device logical path to host/RTOS path.
 * Returned pointer may be static storage; valid until next call.
 */
const char *divoom_port_path_resolve(const char *path);

/* Directory / path */
int divoom_port_mkdir_p(const char *path);
int divoom_port_file_exists(const char *path);
int divoom_port_remove_file(const char *path);
int divoom_port_rename_file(const char *old_path, const char *new_path);
int divoom_port_stat(const char *path, divoom_port_stat_t *out);
int divoom_port_copy_file(const char *src, const char *dst);
int divoom_port_copy_tree(const char *src, const char *dst);
int divoom_port_remove_tree(const char *path);
int divoom_port_sync(void);
int divoom_port_exec(const char *cmd);

/*
 * Unmount mount_path, FAT-format the block device, remount.
 * RTOS: /dev/UDISK + /userdata → elmfat f_mkfs(FM_FAT) 整分区，再挂回。
 * Simulator: 清空映射目录内容（无法 mkfs），保留挂载根目录。
 * 调用方须先关掉该卷上的打开文件；本接口不 seed、不 reboot。
 * 成功返回 0，失败返回 -1。
 */
int divoom_port_fs_format_volume(const char *devpath, const char *mount_path);
int divoom_port_fs_format_userdata(void);

/* File handle I/O (handle is stdio FILE* on current backends) */
void *divoom_port_file_open(const char *path, const char *mode);
int divoom_port_file_close(void *file);
/* Metadata from an already open handle, without a path lookup. FatFs reports
 * mtime=0 (unknown); callers must not compare it to a path stat timestamp. */
int divoom_port_file_stat(void *file, divoom_port_stat_t *out);
/* Flush stdio and persist file data/length while the handle remains open. */
int divoom_port_file_sync(void *file);
size_t divoom_port_file_read(void *buf, size_t elem_size, size_t count, void *file);
size_t divoom_port_file_write(const void *buf, size_t elem_size, size_t count, void *file);
int divoom_port_file_seek(void *file, long offset, int whence);
long divoom_port_file_tell(void *file);
char *divoom_port_file_gets(char *buf, int size, void *file);
int divoom_port_file_puts(const char *s, void *file);

/* Convenience on logical path. Free read_all buffer with divoom_system_free(). */
long divoom_port_file_size(const char *path);
char *divoom_port_file_read_all(const char *path, int *out_len);
/* Same ownership as read_all. Rejects oversized input with EFBIG before allocating. */
char *divoom_port_file_read_all_bounded(const char *path, int *out_len, int max_len);
int divoom_port_file_crc32(const char *path, uint64_t *size_out,
	uint32_t *crc32_out);
//返回有多少剩余空间，以K为单位
uint32_t divoom_port_fs_free_space(const char *path);

/* Directory (handle is DIR* on current backends) */
void *divoom_port_dir_open(const char *path);
int divoom_port_dir_close(void *dir);
const divoom_port_dirent_t *divoom_port_dir_read(void *dir);

/*
 * Compare a Python-generated DCRC manifest with root_path.
 *
 * Files are read only once to build the current directory summaries. Matching
 * directory file_count/CRC32 pairs are then pruned without walking their
 * children. cb may be NULL for a summary-only comparison. A non-zero callback
 * return aborts the comparison with DIVOOM_PORT_DIR_COMPARE_ERR_CALLBACK.
 *
 * Return DIVOOM_PORT_DIR_COMPARE_MATCH, DIVOOM_PORT_DIR_COMPARE_DIFFERENT, or
 * one of the negative DIVOOM_PORT_DIR_COMPARE_ERR_* values above.
 */
int divoom_port_dir_checksum_compare(
	const char *manifest_path,
	const char *root_path,
	divoom_port_dir_change_cb cb,
	void *user_data,
	divoom_port_dir_compare_result_t *result);

/*
 * Compare two DCRC manifests without scanning a directory. Changes describe
 * the transition from old_manifest_path to new_manifest_path:
 * ADDED/MODIFIED entries come from the new manifest, REMOVED entries from the
 * old manifest. Both manifests must have the same root name.
 */
int divoom_port_dir_manifest_compare(
	const char *old_manifest_path,
	const char *new_manifest_path,
	divoom_port_dir_change_cb cb,
	void *user_data,
	divoom_port_dir_compare_result_t *result);

/* 完整解析清单并检查根名/路径；create 非零时创建全部目录（包含空目录）。 */
int divoom_port_dir_manifest_prepare_dirs(const char *manifest, const char *root, int create);

/* Validate the entire DCRC manifest (including its checksum and root name),
 * then read one root-relative file's expected length/CRC. No live tree scan.
 * Returns 0 only when a matching file entry is present and valid. */
int divoom_port_dir_manifest_file_info(const char *manifest, const char *root,
	const char *relative_path, uint64_t *size_out, uint32_t *crc32_out);

/* Visit every validated manifest file. No live scan; old_size/old_crc32
 * contain expectations. Callback nonzero stops the traversal. */
int divoom_port_dir_manifest_visit(const char *manifest, const char *root,
    divoom_port_dir_change_cb callback, void *user_data);
/* Also visit directories (excluding the root), reported as ADDED; file
 * entries remain REMOVED with expected old_size/old_crc32. No live writes. */
int divoom_port_dir_manifest_visit_paths(const char *manifest, const char *root,
    divoom_port_dir_change_cb callback, void *user_data);
/* One caller-owned verifier per serial scan. Reuses CRC table/read buffer;
 * prefers 256KB total, halves to 32KB if needed; splits input/stdio buffers
 * equally to avoid nano libc 1KB refills. No static storage/shared file handles. */
void *divoom_port_file_verifier_create(void);
void divoom_port_file_verifier_destroy(void *verifier);
/* Opens once and obtains size from the handle; no per-file path stat.
 * allow_changed_size: differently sized files pass without reading content.
 * Matching sizes require a complete, error-free read, byte count and CRC. */
int divoom_port_file_verifier_verify(void *verifier, const char *path,
    uint64_t expected_size, uint32_t expected_crc, int allow_changed_size);

/* relaxed: differently sized files need only a complete, error-free read. */
int divoom_port_file_verify(const char *path, uint64_t expected_size,
    uint32_t expected_crc, int relaxed);
#endif /* DIVOOM_PORT_FS_H */
