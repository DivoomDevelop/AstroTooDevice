#include "divoom_port_fs.h"
#include "divoom_build_config.h"
#include "divoom_malloc.h"
#ifdef _WIN32
#include "sim_compat.h"
#endif
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef DIVOOM_PORT_PATH_MAX
#define DIVOOM_PORT_PATH_MAX 512
#endif

#ifndef DIVOOM_PORT_COPY_CHUNK
#define DIVOOM_PORT_COPY_CHUNK DIVOOM_PORT_WRITE_CHUNK
#endif

static int port_is_vfs_mount_root(const char *dir)
{
	return dir && (!strcmp(dir, "/userdata") || !strcmp(dir, "/usr"));
}

static int port_mkdir_one(const char *dir)
{
	if (!dir || !dir[0])
		return -1;
	/*
	 * VFS mount roots already exist once elmfat_mount() succeeds.
	 * FatFs f_stat("0:/") -> FR_INVALID_NAME so access() always fails here;
	 * do not let that abort mkdir_p before subdirs (system/tmp/wifi...) are created.
	 */
	if (port_is_vfs_mount_root(dir))
		return 0;
	if (access(dir, F_OK) == 0)
		return 0;
	if (mkdir(dir, 0777) == 0)
		return 0;
	if (errno == EEXIST)
		return 0;
	return -1;
}

int divoom_port_mkdir_p(const char *path)
{
	const char *resolved;
	char work[DIVOOM_PORT_PATH_MAX];
	char *p;
	size_t len;

	if (!path || !path[0])
		return -1;
	resolved = divoom_port_path_resolve(path);
	if (!resolved)
		return -1;
	len = strlen(resolved);
	if (len >= sizeof(work))
		return -1;
	memcpy(work, resolved, len + 1);
#ifdef _WIN32
	/* 映射后是 Windows 盘符绝对路径，必须逐级创建，不能只 mkdir 最深层。 */
	if (len > 3 && work[1] == ':') {
		for (p = work + 3; *p; p++) {
			if (*p == '/' || *p == '\\') {
				char separator = *p;
				*p = '\0';
				if (port_mkdir_one(work) != 0 && access(work, F_OK) != 0) {
					return -1;
				}
				*p = separator;
			}
		}
	}
#endif
	if (work[0] == '/' && work[1] != '\0') {
		for (p = work + 1; *p; p++) {
			if (*p == '/') {
				*p = '\0';
				if (port_mkdir_one(work) != 0 && access(work, F_OK) != 0)
					return -1;
				*p = '/';
			}
		}
	}
	return port_mkdir_one(work);
}

int divoom_port_stat(const char *path, divoom_port_stat_t *out)
{
	const char *resolved;
	struct stat st;

	if (!path || !out)
		return -1;
	resolved = divoom_port_path_resolve(path);
	if (!resolved)
		return -1;
	if (stat(resolved, &st) != 0)
		return -1;
	out->size = (long)st.st_size;
	out->mtime = (long)st.st_mtime;
	out->mode = (unsigned int)st.st_mode;
	out->is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
	return 0;
}

const divoom_port_dirent_t *divoom_port_dir_read(void *dir)
{
#ifdef _MSC_VER
	/* 多个文件 worker 可同时遍历不同目录，返回槽位不能跨线程共享。 */
	static __declspec(thread) divoom_port_dirent_t ent;
#else
	static divoom_port_dirent_t ent;
#endif
	struct dirent *de;

	if (!dir)
		return NULL;
	de = readdir((DIR *)dir);
	if (!de)
		return NULL;
	ent.name = de->d_name;
#ifdef DT_DIR
	ent.is_dir = (de->d_type == DT_DIR) ? 1 : 0;
#else
	ent.is_dir = 0;
#endif
	return &ent;
}

static void *port_copy_alloc(size_t n)
{
    void *buffer = divoom_malloc_raw((uint32)n, 1, __LINE__, __FILE__, 0);
    if (!buffer) errno = ENOMEM;
    return buffer;
}

static void port_copy_free(void *p)
{
	if (p)
		divoom_system_free(p);
}

static int port_copy_file_resolved(const char *src, const char *dst)
{
	void *in = NULL;
	void *out = NULL;
	char *buf = NULL;
	size_t n;
	int rc = -1, saved_errno;

	in = divoom_port_file_open(src, "rb");
	if (!in)
		goto out;
	out = divoom_port_file_open(dst, "wb");
	if (!out)
		goto out;

	buf = (char *)port_copy_alloc(DIVOOM_PORT_COPY_CHUNK);
	if (!buf)
		goto out;

	while ((n = divoom_port_file_read(buf, 1, DIVOOM_PORT_COPY_CHUNK, in)) > 0) {
		if (divoom_port_file_write(buf, 1, n, out) != n)
			goto out;
	}
	rc = 0;

out:
    saved_errno = errno;
	if (in)
		divoom_port_file_close(in);
	if (out)
		divoom_port_file_close(out);
	port_copy_free(buf);
    if (rc != 0) errno = saved_errno;
	return rc;
}

int divoom_port_copy_file(const char *src, const char *dst)
{
	if (!src || !dst)
		return -1;
	return port_copy_file_resolved(src, dst);
}

static int port_copy_tree_resolved(const char *src, const char *dst)
{
	divoom_port_stat_t st;
	void *dir;
	const divoom_port_dirent_t *ent;
	char src_child[DIVOOM_PORT_PATH_MAX];
	char dst_child[DIVOOM_PORT_PATH_MAX];
	int ret = 0;

	if (divoom_port_stat(src, &st) != 0)
		return -1;
	if (!st.is_dir)
		return divoom_port_copy_file(src, dst);
	if (divoom_port_mkdir_p(dst) != 0)
		return -1;
	dir = divoom_port_dir_open(src);
	if (!dir)
		return -1;
	while ((ent = divoom_port_dir_read(dir)) != NULL) {
		if (ent->name[0] == '.' &&
		    (ent->name[1] == '\0' || (ent->name[1] == '.' && ent->name[2] == '\0')))
			continue;
		snprintf(src_child, sizeof(src_child), "%s/%s", src, ent->name);
		snprintf(dst_child, sizeof(dst_child), "%s/%s", dst, ent->name);
		if (port_copy_tree_resolved(src_child, dst_child) != 0)
			ret = -1;
	}
	divoom_port_dir_close(dir);
	return ret;
}

int divoom_port_copy_tree(const char *src, const char *dst)
{
	if (!src || !dst)
		return -1;
	return port_copy_tree_resolved(src, dst);
}

static int port_remove_tree_resolved(const char *path)
{
	divoom_port_stat_t st;
	void *dir;
	const divoom_port_dirent_t *ent;
	char child[DIVOOM_PORT_PATH_MAX];

	if (divoom_port_stat(path, &st) != 0)
		return divoom_port_remove_file(path);
	if (!st.is_dir)
		return divoom_port_remove_file(path);
	dir = divoom_port_dir_open(path);
	if (!dir)
		return -1;
	while ((ent = divoom_port_dir_read(dir)) != NULL) {
		divoom_port_stat_t cst;

		if (ent->name[0] == '.' &&
		    (ent->name[1] == '\0' || (ent->name[1] == '.' && ent->name[2] == '\0')))
			continue;
		snprintf(child, sizeof(child), "%s/%s", path, ent->name);
		if (divoom_port_stat(child, &cst) == 0 && cst.is_dir)
			port_remove_tree_resolved(child);
		else
			divoom_port_remove_file(child);
	}
	divoom_port_dir_close(dir);
	return divoom_port_remove_file(path);
}

int divoom_port_remove_tree(const char *path)
{
	if (!path)
		return -1;
	return port_remove_tree_resolved(path);
}

int divoom_port_sync(void)
{
#ifdef _WIN32
	return 0;
#elif !DIVOOM_BUILD_RTOS_DEVICE
	sync();
#endif
	return 0;
}
