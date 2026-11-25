#include "divoom_build_config.h"
#if !DIVOOM_BUILD_SIMULATOR
#error "divoom_port_fs_sim.c is simulator-only; exclude from RTOS Kbuild."
#endif

#include "divoom_port_fs.h"
#include "divoom_sim_paths.h"
#include <direct.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <windows.h>
#include <stdlib.h>

#define DIVOOM_PORT_SIM_PATH_BUF_COUNT 4
#define DIVOOM_PORT_SIM_PATH_BUF_SIZE 1024

#if defined(_MSC_VER)
#define DIVOOM_PORT_TLS __declspec(thread)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define DIVOOM_PORT_TLS _Thread_local
#else
#define DIVOOM_PORT_TLS __thread
#endif

static DIVOOM_PORT_TLS char divoom_port_path_buf[DIVOOM_PORT_SIM_PATH_BUF_COUNT][DIVOOM_PORT_SIM_PATH_BUF_SIZE];
static DIVOOM_PORT_TLS int divoom_port_path_next;

const char *divoom_port_path_resolve(const char *path)
{
	char *buf;
	const char *mapped;

	if (!path)
		return NULL;
	buf = divoom_port_path_buf[divoom_port_path_next++ % DIVOOM_PORT_SIM_PATH_BUF_COUNT];
	mapped = sim_fs_map_path(path, buf, DIVOOM_PORT_SIM_PATH_BUF_SIZE);
	return mapped ? mapped : path;
}

int divoom_port_file_exists(const char *path)
{
	const char *resolved;

	if (!path || !path[0])
		return 0;
	resolved = divoom_port_path_resolve(path);
	return _access(resolved, 0) == 0;
}

int divoom_port_remove_file(const char *path)
{
	const char *resolved;

	if (!path)
		return -1;
	resolved = divoom_port_path_resolve(path);
	return remove(resolved);
}

int divoom_port_rename_file(const char *old_path, const char *new_path)
{
	const char *resolved_old;
	const char *resolved_new;

	if (!old_path || !new_path)
		return -1;
	resolved_old = divoom_port_path_resolve(old_path);
	resolved_new = divoom_port_path_resolve(new_path);
	return rename(resolved_old, resolved_new);
}

static int divoom_port_fs_sim_remove_directory(const char *path)
{
	void *dir;
	const divoom_port_dirent_t *entry;
	char child[DIVOOM_PORT_SIM_PATH_BUF_SIZE];
	DWORD attributes = GetFileAttributesA(path);
	int rc = 0;
	/* 拒绝 junction/symlink，递归不能跳出仿真分区。 */
	if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
		return -1;
	}
	dir = divoom_port_dir_open(path);
	if (!dir) {
		return -1;
	}
	while ((entry = divoom_port_dir_read(dir)) != NULL) {
		if (!strcmp(entry->name, ".") || !strcmp(entry->name, "..")) {
			continue;
		}
		if (snprintf(child, sizeof(child), "%s/%s", path, entry->name) >= sizeof(child)) {
			rc = -1;
			break;
		}
		attributes = GetFileAttributesA(child);
		if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
		    ((attributes & FILE_ATTRIBUTE_DIRECTORY) ? divoom_port_fs_sim_remove_directory(child) : remove(child)) != 0) {
			rc = -1;
			break;
		}
	}
	divoom_port_dir_close(dir);
	return rc == 0 ? _rmdir(path) : rc;
}

int divoom_port_fs_format_volume(const char *devpath, const char *mount_path)
{
	void *dir;
	const divoom_port_dirent_t *ent;
	char child[DIVOOM_PORT_SIM_PATH_BUF_SIZE];
	char name[256];
	int is_dir;
	int guard = 0;
	char root[DIVOOM_PORT_SIM_PATH_BUF_SIZE];
	char expected[DIVOOM_PORT_SIM_PATH_BUF_SIZE];
	char resolved[DIVOOM_PORT_SIM_PATH_BUF_SIZE];

	/* 仅允许两个逻辑分区，拒绝宿主盘符和任意目录。 */
	if (!devpath || !mount_path ||
	    !((strcmp(devpath, "/dev/usr") == 0 && strcmp(mount_path, "/usr") == 0) ||
	      (strcmp(devpath, "/dev/UDISK") == 0 && strcmp(mount_path, "/userdata") == 0))) {
		return -1;
	}

	/* Windows 无法对映射目录 mkfs：反复取第一个非 . 项并删除，直到清空 */
	if (getenv("DIVOOM_SIM_FORMAT_FAULT") && strcmp(getenv("DIVOOM_SIM_FORMAT_FAULT"), "volume") == 0) {
		return -1;
	}
	if (!GetFullPathNameA(divoom_port_path_resolve(mount_path), sizeof(root), root, NULL) ||
	    snprintf(expected, sizeof(expected), "%s%s", sim_fs_resource_root(), mount_path) >= sizeof(expected) ||
	    !GetFullPathNameA(expected, sizeof(resolved), resolved, NULL) || _stricmp(root, resolved) != 0 ||
	    (GetFileAttributesA(root) & FILE_ATTRIBUTE_REPARSE_POINT)) {
		return -1;
	}
	while (guard++ < 10000) {
		dir = divoom_port_dir_open(mount_path);
		if (!dir)
			return -1;
		name[0] = '\0';
		is_dir = 0;
		while ((ent = divoom_port_dir_read(dir)) != NULL) {
			if (!ent->name[0] || strcmp(ent->name, ".") == 0 || strcmp(ent->name, "..") == 0)
				continue;
			strncpy(name, ent->name, sizeof(name) - 1);
			name[sizeof(name) - 1] = '\0';
			is_dir = ent->is_dir;
			break;
		}
		divoom_port_dir_close(dir);
		if (!name[0])
			return 0;
		if (snprintf(child, sizeof(child), "%s/%s", mount_path, name) >= (int)sizeof(child))
			return -1;
		if (is_dir) {
			char host_child[DIVOOM_PORT_SIM_PATH_BUF_SIZE];
			sim_fs_map_path(child, host_child, sizeof(host_child));
			if (divoom_port_fs_sim_remove_directory(host_child) != 0)
				return -1;
		} else if (divoom_port_remove_file(child) != 0) {
			return -1;
		}
	}
	return -1;
}

int divoom_port_fs_format_userdata(void)
{
	return divoom_port_fs_format_volume("/dev/UDISK", "/userdata");
}
