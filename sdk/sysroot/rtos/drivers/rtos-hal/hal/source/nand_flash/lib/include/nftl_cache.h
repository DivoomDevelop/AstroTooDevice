#ifndef __NFTL_CACHE_H
#define __NFTL_CACHE_H

#include "nftl_osal.h"

#define MAX_CACHE_NUM 30
#define MAX_CACHE_WRITE_NUM 3
#define MIN_CACHE_WRITE_NUM 3
#define MAX_CACHE_WRITE_END_NUM 10

#define CACHE_READ              0
#define CACHE_WRITE             1
#define CACHE_WRITE_END         2
#define CACHE_EMPTY             0xff

typedef struct __cache_node
{
	uint16 cache_no;
	uint16 cache_info; /* [CACHE_EMPTY,CACHE_READ,CACHE_WRITE] */
	uint16 start_sector;
	uint16 sector_len;
	uint32 page_no;
	uchar* buf;
	struct __cache_node*   cache_read_next;
	struct __cache_node*   cache_read_prev;
	struct __cache_node*   cache_write_next;
	struct __cache_node*   cache_write_prev;
}_cache_node;

typedef struct
{
	uint16          cache_totals;
	uint16          cache_read_nums;
	uint16          cache_write_nums;
	uint16          cache_write_end_nums;
	_cache_node     cache_node[MAX_CACHE_NUM];
	_cache_node     cache_read_head;
	_cache_node     cache_write_head;
	uchar*          cache_page_buf;
}_cache;

uint32 nand_cache_read(struct _nftl_zone *zone, uint32 page_no, uint16 bitmap,
		uchar *buf);
uint32 nand_cache_write(struct _nftl_zone *zone, uint32 page_no, uint16 bitmap,
		uchar *buf);
uint32 nand_op(uchar flag, struct _nftl_zone *zone, uint32 start_sector,
		uint32 len, uchar *buf);
uint32 __nand_read(struct _nftl_zone *zone, uint32 start_sector, uint32 len,
		uchar *buf);
uint32 __nand_write(struct _nftl_zone *zone, uint32 start_sector, uint32 len,
		uchar *buf);
uint32 __nand_flush_write_cache(struct _nftl_zone *zone, uint32 num);
uint32 flush_write_cache_to_nand(struct _nftl_zone *zone);
uint32 __shutdown_op(struct _nftl_zone *zone);
_cache_node *search_from_cache_read_list(_cache *cache, uint32 page);
_cache_node *search_from_cache_write_list(_cache *cache, uint32 page);
_cache_node *get_empty_cahce_node(_cache *cache);
int add_to_cache_read_list_tail(_cache *cache, _cache_node *cache_node,
		uint32 flag);
int add_to_cache_write_list_tail(_cache *cache, _cache_node *cache_node);
_cache_node *del_from_cache_read_list(_cache *cache, _cache_node *cache_node);
_cache_node *del_from_cache_write_list(_cache *cache, _cache_node *cache_node);
int nftl_cache_init(struct _nftl_zone *zone);
int nftl_cache_exit(struct _nftl_zone *zone);
uint32 nand_discard(struct _nftl_zone *zone, uint32 start_sector, uint32 len);
uint32 __nand_discard(struct _nftl_zone *zone, uint32 start_sector, uint32 len);
uint32 print_logic_page_map(struct _nftl_zone *zone, uint32 start_page);
uint32 get_cold_logic_page(struct _nftl_zone *zone, uint32 start_page,
		uint32 total_pages);
int read_reclaim(struct _nftl_blk *start_blk, struct _nftl_blk *nftl_blk,
		uchar*buf);
struct _nftl_blk *get_nftl_need_read_claim(struct _nftl_blk *start_blk);
uint32 set_nftl_read_claim_complete(struct _nftl_blk *start_blk,
		struct _nftl_blk *nftl_blk);

#endif
