#ifndef __NFTL_HW_INTERFACE_H
#define __NFTL_HW_INTERFACE_H

#include "nftl_osal.h"

typedef struct{
	uint16 Block_NO;
}_nand_block;

/* phy block数组 */
typedef struct __phy_block{
	_nand_block         phy_block;
	uint16              info; /* 0xaa:表示有page ECC error */
	uint16              invalid_page_count; /* [0,pages_per_blk-1] */
	uint16              erase_count; /* [0,10000] */
	sint32              block_used_count;

	/*
	 * block内无效page数链表，以vaild_page_count大小为顺序的单向链表，
	 * 用于garbage collect 常驻内存，启动时恢复。
	 */
	struct __phy_block*    invalid_page_next;
	struct __phy_block*    invalid_page_prev;

	//free block链表,以erase_count大小为顺序的单向链表
	struct __phy_block*    free_next;
	struct __phy_block*    free_prev;

	//block使用先后链表
	struct __phy_block*    block_used_next;
	struct __phy_block*    block_used_prev;
}_phy_block_info;

typedef struct{
	uint16  Page_NO;
	uint16  Block_NO;
}_nand_page;

typedef struct{
	_nand_page phy_page;
	uint16 page_bitmap;
	uchar *main_data_addr;
	uchar *spare_data_addr;
}_physic_op_par;

int _nftl_nand_read_page(struct _nftl_zone *zone, _physic_op_par *p);
int _nftl_nand_write_page(struct _nftl_zone *zone, _physic_op_par *p);
int _nftl_nand_erase_superblk(struct _nftl_zone *zone, _physic_op_par *p);
int _nftl_nand_mark_bad_blk(struct _nftl_zone *zone, _physic_op_par *p);
int _nftl_nand_is_blk_good(struct _nftl_zone *zone, _physic_op_par *p);

#endif
