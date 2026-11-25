#ifndef __NFTL_START__
#define __NFTL_START__

#include "nftl_common.h"

int nftl_start(struct _nftl_blk *nftl_blk, struct _nftl_cfg *cfg,
		struct _nftl_zone **pzone, uint64 logic_size,
		uint64 total_size, unsigned erasesize, unsigned writesize,
		unsigned oobavail, char *name, int no);
void nftl_zone_release(struct _nftl_zone *zone);

#endif
