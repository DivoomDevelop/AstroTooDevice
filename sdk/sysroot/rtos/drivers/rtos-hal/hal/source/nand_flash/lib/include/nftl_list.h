#ifndef _NFTL_LIST_H
#define _NFTL_LIST_H

#include "nftl_osal.h"
#include "nftl_common.h"

int adjust_invaild_list(struct _nftl_zone *zone);
int put_phy_block_to_invalid_page_list(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr);
_phy_block_info *out_phy_block_from_invalid_page_list_by_block(
		struct _nftl_zone *zone, _phy_block_info *phy_block_ptr);
_phy_block_info *out_phy_block_from_free_list_by_block(
		struct _nftl_zone *zone, _phy_block_info *phy_block_ptr);
_phy_block_info *out_phy_block_from_free_list_by_erase_time(
		struct _nftl_zone *zone, uint16 erase_time);
uint16 get_max_erase_time_from_free_list(struct _nftl_zone *zone,
		uint16 erase_time);
_phy_block_info *get_block_max_erase_time_from_free_list(
		struct _nftl_zone *zone, uint16 erase_time);
_phy_block_info *get_block_max_erase_time_from_invalid_list(
		struct _nftl_zone *zone, uint16 erase_time);
_phy_block_info *get_block_min_erase_time_from_invalid_list(
		struct _nftl_zone *zone, uint16 erase_time, uint32 max_count);
_phy_block_info *get_block_max_count_from_invalid_list(struct _nftl_zone *zone);
_phy_block_info *get_block_min_count_from_invalid_list(struct _nftl_zone *zone);
_phy_block_info *out_phy_block_from_free_list(struct _nftl_zone *zone);
int phy_block_from_invalid_page_incr(struct _nftl_zone *zone,
		_phy_block_info *block);
_phy_block_info *out_phy_block_from_invalid_page_list(struct _nftl_zone *zone);
void print_block_invalid_list(struct _nftl_zone *zone);
uint32 is_block_in_invalid_list(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr);
uint32 is_block_in_free_list(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr);
int put_phy_block_to_free_list(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr);
int add_block_count_list(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr);
void print_free_list(struct _nftl_zone *zone);
_phy_block_info *del_block_count_by_block(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr);

#endif
