#ifndef __NFTL_INIT_H
#define __NFTL_INIT_H

#include "nftl_common.h"

void *nftl_malloc(uint32 size);
void nftl_free(const void *ptr);
int nftl_initialize(struct _nftl_blk *nftl_blk, int no);
int nftl_exit(struct _nftl_blk *nftl_blk);
int _nand_read(struct _nftl_blk *nftl_blk, uint32 start_sector, uint32 len,
		unsigned char *buf);
int _nand_write(struct _nftl_blk *nftl_blk, uint32 start_sector, uint32 len,
		unsigned char *buf);
int _nand_discard(struct _nftl_blk *nftl_blk, uint32 start_sector, uint32 len);
int _nand_flush_write_cache(struct _nftl_blk *nftl_blk, uint32 num);
int _shutdown_op(struct _nftl_blk *nftl_blk);
struct _nftl_blk *del_last_nftl_blk(struct _nftl_blk *head);
void add_nftl_blk_list(struct _nftl_blk *head, struct _nftl_blk *nftl_blk);
int _nand_sector_read(struct _nftl_blk *nftl_blk, uint32 start_sector,
		uint32 len, unsigned char *buf);
int _nand_sector_write(struct _nftl_blk *nftl_blk, uint32 start_sector,
		uint32 len, unsigned char *buf);
int _nand_flush_sector_write_cache(struct _nftl_blk *nftl_blk, uint32 num);

#endif
