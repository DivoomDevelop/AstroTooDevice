#ifndef __NFTL_INC_H_
#define __NFTL_INC_H_

#include "nftl_osal.h"
#include "nftl_type.h"
#include "nftl_common.h"
#include "nftl_list.h"
#include "nftl_logic.h"
#include "nftl_gc.h"
#include "nftl_cache.h"
#include "nftl_cfg.h"
#include "nftl_start.h"
#include "nftl_build.h"
#include "nftl_crc.h"
#include "nftl_smart.h"
#include "nftl_hexdump.h"

#define NFTL_VERSION_MAIN 0
#define NFTL_VERSION_MIDDLE 0
#define NFTL_VERSION_SUB 1

#define min_t(x, y, t) (t)(x) < (t)(y) ? x : y
#define max_t(x, y, t) (t)(x) > (t)(y) ? x : y
#define min(x, y) min_t(x, y, uint32)
#define max(x, y) max_t(x, y, uint32)

#define byte_to_sector(sz) (sz >> SHIFT_PER_SECTOR)
#define sector_to_byte(sz) (sz << SHIFT_PER_SECTOR)

#ifndef NFTL_ERR
#define NFTL_ERR(fmt, ...) \
	do { \
		PRINT("[NE]: %s(%d): "fmt, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)
#endif

#ifdef CONFIG_NAND_FLASH_DEBUG
#ifndef NFTL_INFO
#define NFTL_INFO(fmt, ...) \
	do { \
		PRINT("[NI]: "fmt, ##__VA_ARGS__); \
	} while (0)
#endif
#ifndef NFTL_DBG
#define NFTL_DBG(fmt, ...) \
	do { \
		PRINT_DBG("[ND]: "fmt, ##__VA_ARGS__); \
	} while (0)
#endif
#else /* !CONFIG_NAND_FLASH_DEBUG */
#ifndef NFTL_INFO
#define NFTL_INFO(fmt, ...)
#endif

#ifndef NFTL_DBG
#define NFTL_DBG(fmt, ...)
#endif
#endif /* CONFIG_NAND_FLASH_DEBUG */

#ifndef NULL
#define NULL ((void *)0)
#endif

struct  nand_chip_t {
	uchar           sector_per_page;
	uchar           frequence_par;
	uchar           bytes_user_date;
	uchar           bytes_ecc_date;
	uchar           ecc_bits;            //8,24,32
	uchar           ecc_for_bytes;       //512 or 1024
	uchar           two_plane_support;
	uint16          max_erase_times;
	uint16          two_plane_offset;
	uint16          pages_per_blk;
	uint16          page_offset_in_nextblk;
	uint16          blk_per_chip;
	uint16          bytes_per_page;
	uint16          bitmap_per_page;
	uint16          support_read_reclaim;
	_phy_block_info *nand_block_info;
};

//错误记录
typedef struct{
	uint16   phy_read_ecc_error_times;
	uint16   phy_write_error_times;
	uint16   logic_error_times;
	uint16   logic_error_type[16];
	uint16   phy_read_ecc_error_block[16];
	uint16   phy_write_ecc_error_block[16];
}_error_notes;

//trim记录
typedef struct{
	uint32   logic_start;
	uint32   len;
}_discard_item;

typedef struct{
	_discard_item   discard[100];
	uint16          item_num;
	uint16          item_ptr;
}_discard_notes;

#define  MAX_MALLOC_BYTES    0x100000
#define  MAX_LOGIC_MAP_NUM   25
#define  MALLOC_FLAG         1

#define   SUPPORT_COSS_TALK         1

#define zone_to_blk(zone) (((struct _nftl_zone *)zone)->priv)
struct _nftl_zone {
	/* nftl_blk */
	void *priv;
	/* 这个物理分区所属的物理nand chip */
	struct  nand_chip_t *nand_chip;
	char version[8];
	/* 这个分区的编号 */
	uchar zone_no;
	/* 这个分区的属性 */
	uchar zone_attr;
	/* 这个物理分区一共有多少个可用的物理block */
	uint16 blocks;
	uint16 bad_block;
	/* 这个分区free block数目 */
	uint16 free_block_num;
	/* 这个物理分区的逻辑容量 */
	uint32 logic_cap_in_sects;
	uint32 logic_cap_in_page;
	/* 这个分区留给算法的备份空间 */
	uint32 backup_cap_in_sects;
	/* 首个物理block的编号 */
	_nand_block zone_start_blk_NO;
	/* 指向物理block数组的首个元素 */
	_phy_block_info *zone_start_phy_block;
	/* 指向物理block数组的首个元素 */
	_phy_block_info *zone_end_phy_block;
	_phy_block_info *recv_free_block;
	/* 指向逻辑page映射表首个元素 */
#if  MALLOC_FLAG
	_mapping_page *zone_logic_page_map[MAX_LOGIC_MAP_NUM];
#else
	_mapping_page *zone_logic_page_map;
#endif
	/* 指向free block排列链表 */
	_phy_block_info free_head;
	/* 指向block内无效page数排列链表 */
	_phy_block_info invalid_page_head;
	/* 指向used block顺序排列链表 */
	_phy_block_info block_used_head;
	/* garbage_collect时使用 */
	_user_info zone_phy_page_map_for_gc;
	uchar *temp_page_buf;
	uchar *logic_page_buf;

	_current_block current_block;
	_current_block assist_block;

	uint32 test;
	uint32 read_reclaim_complete;
	uint32 already_read_flag;
	uint32 wl_gc_count;
	uint32 wl_gc_page_num;

	uint32 max_erase_num;
	_gc_strategy gc_strategy;
	_prio_gc prio_gc;
	_s_wl s_wl;
	_cache cache;
	_smart *smart;
	struct _nftl_cfg *cfg;

	int (*nftl_nand_erase_superblk)(struct _nftl_zone *zone,
			_physic_op_par *p);
	int (*nftl_nand_read_page)(struct _nftl_zone *zone,
			_physic_op_par *p);
	int (*nftl_nand_write_page)(struct _nftl_zone *zone,
			_physic_op_par *p);
	int (*nftl_nand_copy_page)(struct _nftl_zone *zone, _phy_block_info *a,
			_phy_block_info *b, uchar *buf, uint16 page);
	int (*nftl_nand_is_blk_good)(struct _nftl_zone *zone,
			_physic_op_par *p);
	int (*nftl_nand_mark_bad_blk)(struct _nftl_zone *zone,
			_physic_op_par *p);
	int (*nftl_nand_write_logic_page)(struct _nftl_zone *zone,
			uint32 page_no, uchar *buf);
	int (*nftl_nand_read_logic_page)(struct _nftl_zone *zone,
			uint32 page_no, uchar *buf);
	int (*nftl_nand_discard_logic_page)(struct _nftl_zone *zone,
			uint32 page_no);
};

extern void *nftl_malloc(uint32 size);
extern void nftl_free(const void *ptr);

#endif /* __NFTL_H_ */
