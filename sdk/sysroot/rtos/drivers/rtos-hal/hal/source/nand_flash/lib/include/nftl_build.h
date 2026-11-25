#ifndef _NFTL_BUILD_H
#define _NFTL_BUILD_H

#include "nftl_osal.h"
#include "nftl_physic.h"

_phy_block_info *get_phy_block_addr(struct _nftl_zone *zone, uint16 block);
uint32 is_phy_block_valid(_phy_block_info *p_phy_block_info,
		struct _nftl_zone *zone);
uint32 is_last_phy_block(_phy_block_info *p_phy_block_info,
		struct _nftl_zone *zone);
int mark_free_block(struct _nftl_zone *zone, _phy_block_info *block);
_phy_block_info *current_block_ecc_error(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr);
uint32 recover_phy_page_mapping(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr, uint32 *data);
uint32 recover_block_phy_page_mapping(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr, uint32 *data);
uint32 do_write_error_in_build_list(struct _nftl_zone *zone,
		_phy_block_info *block1, _phy_block_info *block2,
		uint16 page_num);
int erase_block(struct _nftl_zone *zone, _phy_block_info *block,
		uint16 erase_num);
uint32 get_used_block_count(struct _nftl_zone *zone,
		_phy_block_info *phy_block_ptr);
void print_nftl_zone(struct _nftl_zone *zone);
int zone_param_exit(struct _nftl_zone *zone);
uint32 get_vaild_blocks(struct _nftl_zone *zone, uint32 start_block,
		uint32 blocks);
_phy_block_info *cross_talk_rebuild_current_block(struct _nftl_zone *zone,
		_phy_block_info *block1, _phy_block_info *block2);
_phy_block_info *no_cross_talk_rebuild_current_block(struct _nftl_zone *zone,
		_phy_block_info *block1, _phy_block_info *block2);
uint32 first_scan_all_blocks(struct _nftl_zone *zone);
uint32 init_zone_after_first_scan(struct _nftl_zone *zone, uint32 block_nums);
uint32 second_scan_all_blocks(struct _nftl_zone *zone);
uint32 get_current_used_block(struct _nftl_zone *zone,
		_phy_block_info *p_phy_block_info, uint32 block_used_count);
uint32 recover_logic_page_mapping(struct _nftl_zone *zone,
		_phy_block_info *p_phy_block_info, uint32 *data,
		uint32 phy_page_nums);
int zone_param_init(struct _nftl_zone *zone, uint16 start_block,
		uint32 logic_sects, uint32 backup_cap_in_sects);
uint32 is_no_use_device(struct _nftl_zone *zone, uint32 size);
uint32 build_zone_list_first(struct _nftl_zone *zone, uint32 size);
uint32 build_zone_list(struct _nftl_zone *zone);
uint32 init_smart_info(struct _nftl_zone *zone, _phy_block_info *phy_block_ptr);
int adjust_free_list(struct _nftl_zone *zone);

#endif
