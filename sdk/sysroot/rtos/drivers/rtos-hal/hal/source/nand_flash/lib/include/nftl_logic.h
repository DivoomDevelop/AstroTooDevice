#ifndef __NFTL_LOGIC_H
#define __NFTL_LOGIC_H

#include "nftl_osal.h"
#include "nftl_physic.h"
#include "nftl_common.h"

typedef struct
{
	/* 指向当前使用block */
	_phy_block_info *block_info; 
	/* 指向当前使用block物理到逻辑的映射表 */
	_user_info user_info;
	uint16 page_used;
}_current_block;

int nand_write_logic_page_no_gc(struct _nftl_zone *zone, uint32 page_no,
		uchar *buf);
int get_new_current_block(struct _nftl_zone *zone,
		_current_block *current_block);
int write_phy_page_map_to_current_block(struct _nftl_zone *zone,
		_current_block *current_block);
int write_logic_data_to_current_block(struct _nftl_zone *zone,
		_current_block *current_block, uint32 data_type,
		uint32 page_no, uchar *buf);
int new_block_init_for_write(struct _nftl_zone *zone, _phy_block_info *block,
		uint32 block_used_count);
int write_phy_data_to_current_block(struct _nftl_zone *zone, _current_block
		*current_block, uchar*buf, uint32 page_no, uint32 type);
int check_invalid_page_right(struct _nftl_zone *zone, _user_info *user_info,
		_phy_block_info *block, uint16 page_num);
void nftl_ops_init(struct _nftl_zone *zone);
int current_block_at_last_page(struct _nftl_zone *zone,
		_current_block *current_block);
int current_block_at_first_page(struct _nftl_zone *zone,
		_current_block *current_block);
int do_before_write(struct _nftl_zone *zone);
int do_after_write(struct _nftl_zone *zone);
int write_data_to_page(struct _nftl_zone *zone, uint32 data_type,
		uint32 page_no, uchar *buf);
int fill_no_use_data_to_page(struct _nftl_zone *zone);
int fill_no_use_data_to_current_block(struct _nftl_zone *zone);
int fill_no_use_page_to_current_block(struct _nftl_zone *zone, int page_num);
int nand_before_shutdown(struct _nftl_zone *zone);

#endif
