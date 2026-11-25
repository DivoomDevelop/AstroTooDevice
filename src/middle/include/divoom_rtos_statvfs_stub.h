#ifndef _DIVOOM_RTOS_STATVFS_STUB_H_
#define _DIVOOM_RTOS_STATVFS_STUB_H_

#include "divoom_build_config.h"

#if DIVOOM_BUILD_SIMULATOR
#error "Simulator uses system statvfs; do not include divoom_rtos_statvfs_stub.h"
#endif

struct statvfs {
	unsigned long f_bsize;
	unsigned long f_blocks;
	unsigned long f_bfree;
	unsigned long f_bavail;
};

static inline int statvfs(const char *path, struct statvfs *buf)
{
	(void)path;
	if (buf) {
		buf->f_bsize = 512;
		buf->f_blocks = 0;
		buf->f_bfree = 0;
		buf->f_bavail = 0;
	}
	return -1;
}

#endif /* _DIVOOM_RTOS_STATVFS_STUB_H_ */
