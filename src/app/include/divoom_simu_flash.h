#ifndef _divoom_simu_flash_h_
#define _divoom_simu_flash_h_
void  divoom_simu_flash_init(uint8 format_flag);
void  divoom_simu_flash_flush(void);
T_BOOL divoom_simu_flash_erase_spi_block(T_U32 block_id);
T_BOOL  divoom_simu_flash_erase_spi_page(T_U32 sector);
T_BOOL  divoom_simu_flash_read_spi_pages(T_U32 page, T_U8 *buf, T_U32 page_cnt);
T_BOOL  divoom_simu_flash_write_spi_pages(T_U32 page, T_U8 *buf, T_U32 page_cnt);

#endif



