#include "divoom_port_fs.h"
#include "divoom_port_hw.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void skip_space(const char **p)
{
	while (**p && isspace((unsigned char)**p))
		(*p)++;
}

static int parse_cp_rf(const char *cmd)
{
	const char *p = cmd;
	char src[512];
	char dst[512];
	int n;

	if (strncmp(p, "cp -rf ", 7) != 0 && strncmp(p, "cp -r ", 6) != 0)
		return -1;
	p += (p[5] == 'f') ? 7 : 6;
	skip_space(&p);
	n = (int)strcspn(p, " \t");
	if (n <= 0 || n >= (int)sizeof(src))
		return -1;
	memcpy(src, p, (size_t)n);
	src[n] = '\0';
	p += n;
	skip_space(&p);
	if (!p[0])
		return -1;
	strncpy(dst, p, sizeof(dst) - 1);
	dst[sizeof(dst) - 1] = '\0';
	{
		char *nl = strchr(dst, '\n');
		if (nl)
			*nl = '\0';
	}
	return divoom_port_copy_tree(src, dst);
}

static int parse_rm_paths(const char *p, int tree)
{
	char path[512];
	int n;
	int ret = 0;
	int any = 0;

	while (*p) {
		skip_space(&p);
		if (!*p || *p == '\n')
			break;
		n = (int)strcspn(p, " \t\n");
		if (n <= 0 || n >= (int)sizeof(path))
			return -1;
		memcpy(path, p, (size_t)n);
		path[n] = '\0';
		p += n;
		any = 1;
		if (tree) {
			if (divoom_port_remove_tree(path) != 0)
				ret = -1;
		} else if (divoom_port_remove_file(path) != 0) {
			ret = -1;
		}
	}
	return any ? ret : -1;
}

static int parse_rm_rf(const char *cmd)
{
	if (strncmp(cmd, "rm -rf ", 7) != 0)
		return -1;
	return parse_rm_paths(cmd + 7, 1);
}

static int parse_rm_f(const char *cmd)
{
	if (strncmp(cmd, "rm -f ", 6) != 0)
		return -1;
	return parse_rm_paths(cmd + 6, 0);
}

static int parse_mv(const char *cmd)
{
	const char *p = cmd;
	char src[512];
	char dst[512];
	int n;

	if (strncmp(p, "mv ", 3) != 0)
		return -1;
	p += 3;
	skip_space(&p);
	n = (int)strcspn(p, " \t");
	if (n <= 0 || n >= (int)sizeof(src))
		return -1;
	memcpy(src, p, (size_t)n);
	src[n] = '\0';
	p += n;
	skip_space(&p);
	if (!p[0])
		return -1;
	strncpy(dst, p, sizeof(dst) - 1);
	dst[sizeof(dst) - 1] = '\0';
	{
		char *nl = strchr(dst, '\n');
		if (nl)
			*nl = '\0';
	}
	if (divoom_port_rename_file(src, dst) == 0)
		return 0;
	if (divoom_port_copy_tree(src, dst) == 0) {
		divoom_port_remove_tree(src);
		return 0;
	}
	return -1;
}

static int parse_echo_redirect(const char *cmd)
{
	const char *p = cmd;
	char value[128];
	char path[512];
	int n;

	if (strncmp(p, "echo ", 5) != 0)
		return -1;
	p += 5;
	n = (int)strcspn(p, " \t");
	if (n <= 0 || n >= (int)sizeof(value))
		return -1;
	memcpy(value, p, (size_t)n);
	value[n] = '\0';
	p += n;
	skip_space(&p);
	if (*p != '>')
		return -1;
	p++;
	skip_space(&p);
	if (!p[0])
		return -1;
	strncpy(path, p, sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';
	{
		char *nl = strchr(path, '\n');
		if (nl)
			*nl = '\0';
	}
	return divoom_port_hw_sysfs_write(path, value);
}

int divoom_port_exec(const char *cmd)
{
	const char *p;

	if (!cmd || !cmd[0])
		return -1;
	p = cmd;
	skip_space(&p);
	if (strcmp(p, "sync") == 0)
		return divoom_port_sync();
	if (strcmp(p, "reboot") == 0)
		return divoom_port_hw_reboot();
	if (strncmp(p, "rm -rf ", 7) == 0)
		return parse_rm_rf(p);
	if (strncmp(p, "rm -f ", 6) == 0)
		return parse_rm_f(p);
	if (strncmp(p, "cp -rf ", 7) == 0 || strncmp(p, "cp -r ", 6) == 0)
		return parse_cp_rf(p);
	if (strncmp(p, "mv ", 3) == 0)
		return parse_mv(p);
	if (strncmp(p, "echo ", 5) == 0)
		return parse_echo_redirect(p);
	/* chmod/amixer/killall etc. — not implemented on RTOS */
	return 0;
}
