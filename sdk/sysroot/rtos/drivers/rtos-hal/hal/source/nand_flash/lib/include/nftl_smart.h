#ifndef __NFTL_SMART_H
#define __NFTL_SMART_H

#include "nftl_osal.h"
#include "nftl_physic.h"

unsigned int nftl_smart_get_boot_cnt(struct _nftl_zone *zone);
void nftl_smart_print(struct _nftl_zone *_zone);
uint32 nftl_smart_init(struct _nftl_zone *zone, _phy_block_info *phy_block_ptr);

#define  SMART_VERSION     0xaaaaaa01   //在后面增加成员不用修改版本号
typedef struct __smart{

    uint32 gap[16];

    uint32 version;

    uint64 total_recv_read_sectors;
    uint64 total_recv_write_sectors;

    uint64 total_real_read_pages;
    uint64 total_real_write_pages;

    uint64 total_recv_discard_sectors;
    uint64 total_real_discard_sectors;

    uint32 total_recv_read_claim_pages;

    uint32 total_gc_times;
    uint32 total_gc_pages;
    uint32 total_wl_times;

    uint32 total_real_read_error_pages;
    uint32 total_real_write_error_pages;

    uint32 total_normal_power_cycles;
    uint32 total_unusual_power_cycles;

    uint32 max_block_erase_times;
    uint32 min_block_erase_times;

    uint32 max_block_used_counter;
    uint32 min_block_used_counter;

    uint32 read_reclaim_utc;
    uint32 cross_talk_times;        //43

    uint32 read_reclaim_zone_no;    //44
    uint32 read_reclaim_page_no;    //45
    uint32 total_wl_gc_times;     //46

    uint32  reserve[80-46];       //补齐320字节

}_smart;

#endif
