#ifndef _NFTL_GC_H
#define _NFTL_GC_H

#include "nftl_osal.h"
#include "nftl_list.h"

#define  GC_ON             0
#define  GC_STOP           1

#define  WL_ON             1
#define  WL_STOP           0

//Schmidt oscillator
typedef struct{
    uint16   start_gc_free_blocks;  //free 块小于此值开始gc
    uint16   stop_gc_free_blocks;   //free 块大于此值停止gc
    uint16   process;
    uint16   flag_gc_block;
    uint16   gc_page;
}_gc_strategy;


#define   PRIO_NONE              0
#define   GC_READ_RECLAIM        1
#define   GC_WEAR_LEVELING       2
#define   GC_CHANGE              3
#define   GC_READ_ERROR          4
#define   GC_NEW_WEAR_LEVELING   5

#define WL_GC_BLOCK_COUNT 20
#define WL_GC_PAGE_NUM 4

typedef struct __prio_gc_node
{
    uint16                   gc_no;
    uint16                   prio_type;
    _phy_block_info*         phy_block_info;
    struct __prio_gc_node*   prio_gc_next;
    struct __prio_gc_node*   prio_gc_prev;
//  _phy_page_mapping *      zone_phy_page_map_for_prio;
}_prio_gc_node;

typedef struct
{
    uint16                   gc_num;
    uint16                   prio_type_now;
    _prio_gc_node            prio_gc_node[MAX_PRIO_GC_NUM];
    _prio_gc_node            prio_gc_head;
}_prio_gc;

typedef struct
{
    uint16              erase_threshold;
    uint16              erase_span;
    uint16              s_wl_status;
    uint16              s_wl_start;
    _phy_block_info*    block_for_s_wl;
}_s_wl;
uint32 garbage_collect_first(struct _nftl_zone *zone, _phy_block_info *block,
		uint16 page_num);
uint32 add_prio_gc(struct _nftl_zone *zone, _phy_block_info *block, uint16 type);
uint32 prio_gc_all(struct _nftl_zone *zone);
uint32 do_prio_gc(struct _nftl_zone *zone);
uint32 gc_all(struct _nftl_zone *zone, uint16 invalid_page_count);
uint32 gc_one(struct _nftl_zone *zone, uint16 invalid_page_count);
uint32 garbage_collect(struct _nftl_zone *zone);
uint32 do_static_wear_leveling(struct _nftl_zone *zone);
uint32 done_static_wear_leveling(struct _nftl_zone *zone);
uint32 prio_gc_one(struct _nftl_zone *zone, uint16 block, uint32 flag);
uint32 gc_win_block(struct _nftl_zone *zone, uint16 block);
uint32 is_current_blcok(struct _nftl_zone *zone, _phy_block_info *block);
uint32 static_wear_leveling(struct _nftl_zone *zone);
uint32 power_on_gc(struct _nftl_zone *zone);
uint32 all_time_gc(struct _nftl_zone *zone);
uint32 shutdown_on_gc(struct _nftl_zone *zone);
uint32 gc_all_enhance(struct _nftl_zone *zone);
uint32 gc_all_base_on_free_blks(struct _nftl_zone *zone);
int nftl_get_gc_info(struct _nftl_zone *zone, char *buf, int size);

#endif
