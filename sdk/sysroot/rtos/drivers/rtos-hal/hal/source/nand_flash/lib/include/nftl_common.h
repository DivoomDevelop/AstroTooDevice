#ifndef __NFTL_COMMON_H
#define __NFTL_COMMON_H

#include "nftl_osal.h"
#include "nftl_errno.h"
#include "nftl_type.h"
#include "nftl_physic.h"
#include "nftl_smart.h"

#define	FACTORY_BAD_BLOCK_ERROR 159
#define BYTES_PER_SECTOR 512
#define SHIFT_PER_SECTOR 9
#define BYTES_OF_USER_PER_PAGE 16
#define MIN_BYTES_OF_USER_PER_PAGE 16

#define MAX_PAGE_PER_BLOCK 1024

#define MAX_PRIO_GC_NUM 10

#define MAX_ERASE_NUM 3000

#define PHY_MAPPING_PAGE 0xaaaaffff
#define PHY_FILL_PAGE 0x55555555

#define PHY_FUNCTION_INFO_PAGE 0xaa110000
#define PHY_DISCARD_INFO_PAGE 0xaa111111
#define PHY_ERROR_INFO_PAGE 0xaa112222
#define PHY_NORMAL_POWER_DOWN 0xaa113333
#define PHY_XXX_1 0xaa114444
#define PHY_XXX_2 0xaa115555
#define PHY_XXX_3 0xaa116666
#define PHY_XXX_4 0xaa117777
#define PHY_XXX_5 0xaa118888

#define SPARE_SPECIAL_DATA 0xaa
#define SPARE_NOUSE_DATA 0xbb
#define BLOCK_FOR_FTL_END 0xcc
#define BLOCK_FOR_FTL_START 0xee

#define FLAG_FOR_LOGIC_PAGE 0xC0

#define SPARE_OFFSET_BAD_FLAG 0
#define SPARE_OFFSET_LOGIC_PAGE 1
#define SPARE_OFFSET_SPECIAL_FLAG 1
#define SPARE_OFFSET_SPECIAL_TYPE (SPARE_OFFSET_SPECIAL_FLAG+1)
#define SPARE_OFFSET_ERASE_TIMES (SPARE_OFFSET_LOGIC_PAGE+4)
#define SPARE_OFFSET_COUNTER (SPARE_OFFSET_ERASE_TIMES+2)
#define SPARE_OFFSET_NFTL_SIZE (SPARE_OFFSET_SPECIAL_TYPE+1)

#define BLOCK_NO_USED 0
#define BLOCK_HAVE_USED 1
#define BLOCK_ALL_INFO_PAGE 2
#define BLOCK_IS_BAD 0xff

//常驻内存，当这个block写到最后一页时将此数据写入。
typedef struct __user_info{
    uchar *buf;
    uint32 buf_size;

    uint32 *map_data;
    uint32 map_size;

    _smart *smart;
    uint32 smart_size;
}_user_info;

typedef struct{
    uint16  Page_NO  : 12;
    uint16  Read_flag : 1;
    uint16  Nouse :     3;
    uint16  Block_NO;
}_mapping_page;

int _nftl_nand_copy_page(struct _nftl_zone *zone, _phy_block_info *a,
		_phy_block_info *b, uchar *buf, uint16 page);
void set_oob_special_page(struct _nftl_zone *zone, uchar *buf,
		uint32 special_data, uint32 block_used_count,
		uint16 erase_times);
uint32 get_logic_page_from_oob(uchar *buf);
uint16 get_erase_count_from_oob(uchar *buf);
void set_oob_logic_page(struct _nftl_zone *zone, uchar *buf,
		uint32 logic_page, uint32 block_used_count, uint16 erase_times);
uint32 get_special_data_from_oob(uchar *buf);
_mapping_page *get_logic_page_map(struct _nftl_zone *zone, uint32 logic_page);
int malloc_logic_page_map(struct _nftl_zone *zone, uint32 total_pages);
int is_ftl_start_flag_page(uchar *buf);
void set_start_block_flag(struct _nftl_zone *zone, uchar *buf, uint32 size);
int is_ftl_end_flag_page(uchar *buf);
void set_end_block_flag(struct _nftl_zone *zone, uchar *buf);
int is_ftl_special_data(uchar *buf);
int is_function_info_page(uchar*buf);
int is_ftl_logic_page_data(uchar *buf);
void set_no_use_page(struct _nftl_zone *zone, uchar *buf);
int is_nouse_page(uchar *buf);
int is_fill_page(uchar *buf);
int is_phy_mapping_page(uchar *buf);
int is_power_down_info_page(uchar *buf);
void set_physic_op_par(_physic_op_par *phy_op_par, uint16 blkNO,
		uint16 Page_NO, uint16 page_bitmap, uchar *main_buf,
		uchar *spare_buf);
int free_logic_page_map(struct _nftl_zone *zone);
uint32 get_spare_data(uchar *buf, uchar offset, uchar num);
uint32 get_block_used_count_from_oob(uchar *buf);
int nftl_get_bad_block_cnt(struct _nftl_zone  *nftl_zone);

#endif
