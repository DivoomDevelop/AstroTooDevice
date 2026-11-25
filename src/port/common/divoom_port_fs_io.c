#include "divoom_port_fs.h"
#include "divoom_build_config.h"
#include "divoom_malloc.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#if DIVOOM_BUILD_SIMULATOR
#include <string.h>
#include <windows.h>

/* 仿真 userdata 分区逻辑容量，单位 KB，用于复现真机低空间清理流程。 */
#define DIVOOM_PORT_SIM_USERDATA_CAPACITY_KB (200U * 1024U)
#define DIVOOM_PORT_SIM_SCAN_PATH_SIZE        1024
#else
#include <sys/statfs.h>
#endif

#if DIVOOM_BUILD_SIMULATOR
static int divoom_port_fs_sim_path_has_prefix(const char *path, const char *prefix)
{
	size_t prefix_len;

	prefix_len = strlen(prefix);
	return strncmp(path, prefix, prefix_len) == 0 &&
		(path[prefix_len] == '\0' || path[prefix_len] == '/' || path[prefix_len] == '\\');
}

static int divoom_port_fs_sim_is_userdata_path(const char *path)
{
	if((path[0] == 'A' || path[0] == 'a') && path[1] == ':')
	{
		path += 2;
	}
	return divoom_port_fs_sim_path_has_prefix(path, "/userdata") ||
		divoom_port_fs_sim_path_has_prefix(path, "/data") ||
		divoom_port_fs_sim_path_has_prefix(path, "/sdcard") ||
		divoom_port_fs_sim_path_has_prefix(path, "/mnt/sdcard");
}

/* 统计目录内文件逻辑大小；达到容量上限后提前结束，避免无意义地继续扫描。 */
static int divoom_port_fs_sim_scan_used_bytes(const char *dir_path,
	uint64_t stop_bytes, uint64_t *used_bytes)
{
	WIN32_FIND_DATAA file_info;
	HANDLE find_handle;
	char search_path[DIVOOM_PORT_SIM_SCAN_PATH_SIZE];
	char child_path[DIVOOM_PORT_SIM_SCAN_PATH_SIZE];
	int path_len;

	path_len = snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);
	if(path_len <= 0 || path_len >= (int)sizeof(search_path))
	{
		return 0;
	}
	find_handle = FindFirstFileA(search_path, &file_info);
	if(find_handle == INVALID_HANDLE_VALUE)
	{
		return GetLastError() == ERROR_FILE_NOT_FOUND;
	}
	do
	{
		if(strcmp(file_info.cFileName, ".") == 0 || strcmp(file_info.cFileName, "..") == 0)
		{
			continue;
		}
		path_len = snprintf(child_path, sizeof(child_path), "%s\\%s",
			dir_path, file_info.cFileName);
		if(path_len <= 0 || path_len >= (int)sizeof(child_path))
		{
			FindClose(find_handle);
			return 0;
		}
		if((file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			/* 跳过重解析目录，防止宿主机目录链接形成递归环。 */
			if((file_info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 &&
				divoom_port_fs_sim_scan_used_bytes(child_path, stop_bytes, used_bytes) == 0)
			{
				FindClose(find_handle);
				return 0;
			}
		}
		else
		{
			*used_bytes += ((uint64_t)file_info.nFileSizeHigh << 32) |
				(uint64_t)file_info.nFileSizeLow;
		}
		if(*used_bytes >= stop_bytes)
		{
			break;
		}
	} while(FindNextFileA(find_handle, &file_info));
	FindClose(find_handle);
	return 1;
}

static int divoom_port_fs_sim_userdata_free_space(uint32_t *free_space_kb)
{
	const char *userdata_root;
	uint64_t capacity_bytes;
	uint64_t used_bytes = 0;
	uint64_t used_space_kb;

	userdata_root = divoom_port_path_resolve("/userdata");
	if(userdata_root == NULL)
	{
		return 0;
	}
	capacity_bytes = (uint64_t)DIVOOM_PORT_SIM_USERDATA_CAPACITY_KB * 1024U;
	if(divoom_port_fs_sim_scan_used_bytes(userdata_root, capacity_bytes, &used_bytes) == 0)
	{
		return 0;
	}
	used_space_kb = (used_bytes + 1023U) / 1024U;
	if(used_space_kb >= DIVOOM_PORT_SIM_USERDATA_CAPACITY_KB)
	{
		*free_space_kb = 0;
	}
	else
	{
		*free_space_kb = DIVOOM_PORT_SIM_USERDATA_CAPACITY_KB - (uint32_t)used_space_kb;
	}
	return 1;
}
#endif

void *divoom_port_file_open(const char *path, const char *mode)
{
	const char *resolved;

	if (!path || !mode)
		return NULL;
	resolved = divoom_port_path_resolve(path);
	if (!resolved)
		return NULL;
	return fopen(resolved, mode);
}

int divoom_port_file_stat(void *file, divoom_port_stat_t *out)
{
    struct stat st;
    if (!file || !out) { errno = EINVAL; return -1; }
#ifdef _WIN32
    if (fstat(_fileno((FILE *)file), &st) != 0) return -1;
#else
    if (fstat(fileno((FILE *)file), &st) != 0) return -1;
#endif
    out->size = (long)st.st_size;
    out->mtime = (long)st.st_mtime;
    out->mode = (unsigned int)st.st_mode;
#ifdef _WIN32
    out->is_dir = (st.st_mode & _S_IFMT) == _S_IFDIR;
#else
    out->is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
#endif
    return 0;
}

int divoom_port_file_close(void *file)
{
	if (!file)
		return -1;
	return fclose((FILE *)file);
}

int divoom_port_file_sync(void *file)
{
	if (!file || fflush((FILE *)file) != 0)
		return -1;
#ifdef _WIN32
	return _commit(_fileno((FILE *)file));
#else
	return fsync(fileno((FILE *)file));
#endif
}

size_t divoom_port_file_read(void *buf, size_t elem_size, size_t count, void *file)
{
	if (!file || !buf)
		return 0;
	return fread(buf, elem_size, count, (FILE *)file);
}

size_t divoom_port_file_write(const void *buf, size_t elem_size, size_t count, void *file)
{
	if (!file || !buf)
		return 0;
	return fwrite(buf, elem_size, count, (FILE *)file);
}

int divoom_port_file_seek(void *file, long offset, int whence)
{
	int std_whence;

	if (!file)
		return -1;
	switch (whence) {
	case DIVOOM_PORT_SEEK_SET:
		std_whence = SEEK_SET;
		break;
	case DIVOOM_PORT_SEEK_CUR:
		std_whence = SEEK_CUR;
		break;
	case DIVOOM_PORT_SEEK_END:
		std_whence = SEEK_END;
		break;
	default:
		return -1;
	}
	return fseek((FILE *)file, offset, std_whence);
}

long divoom_port_file_tell(void *file)
{
	if (!file)
		return -1;
	return ftell((FILE *)file);
}

char *divoom_port_file_gets(char *buf, int size, void *file)
{
	if (!file || !buf || size <= 0)
		return NULL;
	return fgets(buf, size, (FILE *)file);
}

int divoom_port_file_puts(const char *s, void *file)
{
	if (!file || !s)
		return -1;
	return fputs(s, (FILE *)file) == EOF ? -1 : 0;
}

long divoom_port_file_size(const char *path)
{
	void *file;
	long size;

	file = divoom_port_file_open(path, "rb");
	if (!file)
		return 0;
	if (divoom_port_file_seek(file, 0, DIVOOM_PORT_SEEK_END) != 0) {
		divoom_port_file_close(file);
		return 0;
	}
	size = divoom_port_file_tell(file);
	divoom_port_file_close(file);
	return size > 0 ? size : 0;
}

uint32_t divoom_port_fs_free_space(const char *path)
{
	const char *resolved;
#if DIVOOM_BUILD_SIMULATOR
	return 200*1024;
#else
	struct statfs fs;
#endif

	if (!path || !path[0])
	{
		return 0;
	}
	resolved = divoom_port_path_resolve(path);
	if (!resolved)
	{
		return 0;
	}
#if DIVOOM_BUILD_SIMULATOR
	return 200*1024;
#else
	if (statfs(resolved, &fs) != 0)
	{
		return 0;
	}
    if(fs.f_bsize >= 1024)
    {
	    return (uint64_t)fs.f_bavail * (fs.f_bsize/1024);
    }
    else
    {
	    return (uint64_t)fs.f_bavail / (1024/fs.f_bsize);
    }
#endif
}

/* Only trace the factory manifest; optional config reads stay quiet. */
static void divoom_port_manifest_read_trace(const char *path, int fd, const char *stage, long value)
{
	int saved_errno = errno;
	if (path && strcmp(path, "/usr/file_crc.bin") == 0)
		printf("[PT_FS] path=%s fd=%d stage=%s value=%ld errno=%d\n",
			path, fd, stage, value, saved_errno);
	errno = saved_errno;
}

char *divoom_port_file_read_all_bounded(const char *path, int *out_len, int max_len)
{
    char *buf = NULL;
    long file_len = 0;
    int saved_errno = 0;

    if (out_len)
        *out_len = 0;
    void *file = divoom_port_file_open(path, "rb");
    int trace_fd = file && path && strcmp(path, "/usr/file_crc.bin") == 0 ? fileno((FILE *)file) : -1;
    divoom_port_manifest_read_trace(path, trace_fd, "open_fd", trace_fd);
    if (!file)
        return NULL;

    errno = 0;
    int rc = divoom_port_file_seek(file, 0, DIVOOM_PORT_SEEK_END);
    divoom_port_manifest_read_trace(path, trace_fd, "seek_end", rc);
    if (rc != 0) {
        saved_errno = errno ? errno : EIO;
        goto close_file;
    }
    file_len = divoom_port_file_tell(file);
    divoom_port_manifest_read_trace(path, trace_fd, "tell_size", file_len);
    if (file_len < 0) {
        saved_errno = errno ? errno : EIO;
        goto close_file;
    }
    if (file_len > max_len) {
        saved_errno = max_len == INT_MAX ? EOVERFLOW : EFBIG;
        goto close_file;
    }

    buf = divoom_malloc_raw((uint32)((size_t)file_len + 1U), 1, __LINE__, __FILE__, 0);
    if (!buf) {
        saved_errno = ENOMEM; /* The RTOS allocator does not set errno. */
        divoom_port_manifest_read_trace(path, trace_fd, "alloc_failed_bytes", file_len + 1);
        goto close_file;
    }
    errno = 0;
    rc = divoom_port_file_seek(file, 0, DIVOOM_PORT_SEEK_SET);
    divoom_port_manifest_read_trace(path, trace_fd, "seek_start", rc);
    if (rc != 0) {
        saved_errno = errno ? errno : EIO;
        goto close_file;
    }

    errno = 0;
    size_t read_len = divoom_port_file_read(buf, 1, (size_t)file_len, file);
    divoom_port_manifest_read_trace(path, trace_fd, "read_bytes", (long)read_len);
    if (read_len != (size_t)file_len || ferror((FILE *)file)) {
        saved_errno = errno ? errno : EIO;
        goto close_file;
    }
    buf[read_len] = '\0';

close_file:
    errno = 0;
    rc = divoom_port_file_close(file);
    divoom_port_manifest_read_trace(path, trace_fd, "close", rc);
    if (rc != 0 && !saved_errno)
        saved_errno = errno ? errno : EIO;
    if (saved_errno) {
        divoom_system_free(buf);
        errno = saved_errno;
        return NULL;
    }
    if (out_len)
        *out_len = (int)file_len;
    return buf;
}

char *divoom_port_file_read_all(const char *path, int *out_len)
{
    return divoom_port_file_read_all_bounded(path, out_len, INT_MAX);
}

void *divoom_port_dir_open(const char *path)
{
	const char *resolved;

	if (!path)
		return NULL;
	resolved = divoom_port_path_resolve(path);
	if (!resolved)
		return NULL;
	return opendir(resolved);
}

int divoom_port_dir_close(void *dir)
{
	if (!dir)
		return -1;
	return closedir((DIR *)dir);
}
