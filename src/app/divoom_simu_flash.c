#include "divoom.h"

//负责sd卡的初始化,查询,mount,unmount等
//中途插入,中途拔出,开机有,开机没有
//对sd卡的detect脚进行检测,插入卡了,上报到上层-->做不到
//外部监测,检测到了就调用初始化并且mount,检测到detect脚变高了,就是拔出sd卡了,进行unmount
#if 0

#ifdef DIVOOM_USE_SPI_SD_MODE
#define DIVOOM_SPI_SD_CS		DIVOOM_SD_D3_IO
#define DIVOOM_SPI_SD_CLK		DIVOOM_SD_CLK_IO
#define DIVOOM_SPI_SD_MOSI		DIVOOM_SD_CMD_IO
#define DIVOOM_SPI_SD_MISO		DIVOOM_SD_D0_IO
#endif
#define DIVOOM_SD_FILE_PREFIX					DIVOOM_LEGACY_SD_ROOT

typedef struct _divoom_sd_
{
	//可能是挂载失败,啥的,拿给外面用,切换到sd卡就显示错误之类的
	int				sd_state;
    int             lastState;
    int             format_flag;
	sdmmc_host_t sd_host;
#ifndef DIVOOM_USE_SPI_SD_MODE
	sdmmc_slot_config_t slot_config;
#else
	sdspi_slot_config_t slot_config;
#endif
    sdmmc_card_t* sd_card;
	//尝试初始化失败,多次尝试初始化
	void			*sd_time_handle;
	void			*sd_lock;
	xQueueHandle sdcard_evt_queue;
}divoom_sd_ctrl, *Pdivoom_sd_ctrl;

Pdivoom_sd_ctrl pdivoom_sd_handle = NULL;

#define SDCARD_EVT_QUEUE_LEN                    1
#define SDCARD_DETECTION_TASK_PRIORITY          3
#define SDCARD_DETECTION_TASK_STACK_SIZE        (5<<10)

/* SD card related */
#define SD_CARD_INTR_SEL            GPIO_SEL_27

#define SD_CHECK_FILE	DIVOOM_LEGACY_SD_ROOT "/divoom_check_file"


void divoom_sd_card_mount(void)
{
    int i;
    esp_err_t ret;
    pdivoom_sd_handle = divoom_system_malloc(sizeof(divoom_sd_ctrl));
#ifndef DIVOOM_USE_SPI_SD_MODE
	pdivoom_sd_handle->sd_host = (sdmmc_host_t)SDMMC_HOST_DEFAULT();
	pdivoom_sd_handle->sd_host.flags = SDMMC_HOST_FLAG_4BIT;
	pdivoom_sd_handle->slot_config = (sdmmc_slot_config_t)SDMMC_SLOT_CONFIG_DEFAULT();
	//只有检测脚,没有写保护,检测脚传下去没意义啊,还是算了
	//pdivoom_sd_handle->slot_config.gpio_cd = DIVOOM_SD_DETECT_IO;

    gpio_set_pull_mode(DIVOOM_SD_CMD_IO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(DIVOOM_SD_D0_IO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(DIVOOM_SD_D1_IO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(DIVOOM_SD_D2_IO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(DIVOOM_SD_D3_IO, GPIO_PULLUP_ONLY);
#else
	pdivoom_sd_handle->sd_host = (sdmmc_host_t)SDSPI_HOST_DEFAULT();
	pdivoom_sd_handle->slot_config = (sdspi_slot_config_t)SDSPI_SLOT_CONFIG_DEFAULT();
	pdivoom_sd_handle->slot_config.gpio_cs = DIVOOM_SPI_SD_CS;
	pdivoom_sd_handle->slot_config.gpio_sck = DIVOOM_SD_CLK_IO;
	pdivoom_sd_handle->slot_config.gpio_mosi = DIVOOM_SD_CMD_IO;
	pdivoom_sd_handle->slot_config.gpio_miso = DIVOOM_SD_D0_IO;
#endif
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		//不要format,但是可能需要显示点什么
        .format_if_mount_failed = 0,
        .max_files = 16,
		//仅仅是在.format_if_mount_failed设置为true的时候才有效?
        .allocation_unit_size = 16 * 1024
    };

    for(i = 0 ; i < 3; i++)
    {
        ret = esp_vfs_fat_sdmmc_mount(DIVOOM_SD_FILE_PREFIX, &pdivoom_sd_handle->sd_host, &pdivoom_sd_handle->slot_config, &mount_config, &pdivoom_sd_handle->sd_card);
        if(ret == ESP_OK)
        {
            divoom_print("sd has mounted!\n");
            break;
        }
        divoom_print("we will remount sd!: %d\n", i);
    }

	switch (ret) 
	{
        case ESP_OK:
            // Card has been initialized, print its properties
            sdmmc_card_print_info(stdout, pdivoom_sd_handle->sd_card);
            divoom_print("CID name %s!\n", pdivoom_sd_handle->sd_card->cid.name);
            break;
			
        case ESP_ERR_INVALID_STATE:
            divoom_print("File system already mounted");
            break;
		
        case ESP_FAIL:
            divoom_print("Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
            break;
		
        default:
            divoom_print("Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
            break;
    }

}

#define DIVOOM_SIMU_FLASH_CACHE_SIZE (1024*1024)
#define DIVOOM_SIMU_FLASH_WRITE_SIZE (4*1024)
#define DIVOOM_SIMU_FLASH_FILE_SIZE (8*1024*1024)
typedef struct _divoom_simu_flash_ctrl
{
    uint8 wirte_flag;
    void *lock;
    uint32 cache_off;
    uint32 write_cache_off;
    uint8 cache_buf[DIVOOM_SIMU_FLASH_CACHE_SIZE];
    uint8 write_cache_buf[DIVOOM_SIMU_FLASH_CACHE_SIZE];
}divoom_simu_flash_ctrl, *Pdivoom_simu_flash_ctrl;

Pdivoom_simu_flash_ctrl pdivoom_simu_flash_handle = NULL;

void  divoom_simu_flash_read_data(uint8 *cache_buf, uint32 cache_off)
{
    int i, file_size;
    char file_name[64];
    FILE *simu_file_handle = NULL;

    sprintf(file_name, DIVOOM_LEGACY_SD_ROOT "/divom_simu_flash%d.bin", cache_off/DIVOOM_SIMU_FLASH_CACHE_SIZE);
    divoom_print("read simu file: %x:%s\n", cache_off, file_name);
    simu_file_handle = divoom_port_file_open(file_name, "rb");
    if(simu_file_handle == NULL)
    {
        memset(cache_buf, 0xFF, DIVOOM_SIMU_FLASH_CACHE_SIZE);
        divoom_print("read simu file: %s will use default file\n", file_name);
        return;
    }
    divoom_port_file_seek(simu_file_handle, 0, SEEK_END);
    file_size = divoom_port_file_tell(simu_file_handle);
    if(file_size != DIVOOM_SIMU_FLASH_CACHE_SIZE)
    {
        memset(cache_buf, 0xFF, DIVOOM_SIMU_FLASH_CACHE_SIZE);        
        divoom_port_file_close(simu_file_handle);
        divoom_print("read simu file: %s will use default file\n", file_name);
        return;
    }
    divoom_port_file_seek(simu_file_handle, 0, SEEK_SET);
    for(i = 0; i < DIVOOM_SIMU_FLASH_CACHE_SIZE/DIVOOM_SIMU_FLASH_WRITE_SIZE; i ++)
    {
        divoom_port_file_read(&cache_buf[i * DIVOOM_SIMU_FLASH_WRITE_SIZE], 1, DIVOOM_SIMU_FLASH_WRITE_SIZE, simu_file_handle);
        divoom_system_delay_ms(2);
    }
    divoom_port_file_close(simu_file_handle);
}

void  divoom_simu_flash_write_data(void)
{
    int i;
    char file_name[128];
    FILE *simu_file_handle = NULL;

    sprintf(file_name, DIVOOM_LEGACY_SD_ROOT "/divom_simu_flash%d.bin", pdivoom_simu_flash_handle->write_cache_off/DIVOOM_SIMU_FLASH_CACHE_SIZE);
    divoom_print("write simu file: %s\n", file_name);
    simu_file_handle = divoom_port_file_open(file_name, "wb");
    if(simu_file_handle == NULL)
    {
        return;
    }
    for(i = 0; i < DIVOOM_SIMU_FLASH_CACHE_SIZE/DIVOOM_SIMU_FLASH_WRITE_SIZE; i ++)
    {
        divoom_port_file_write(&pdivoom_simu_flash_handle->write_cache_buf[i * DIVOOM_SIMU_FLASH_WRITE_SIZE], 1, DIVOOM_SIMU_FLASH_WRITE_SIZE, simu_file_handle);
        divoom_system_delay_ms(2);
    }
    divoom_port_file_close(simu_file_handle);
}

void  divoom_simu_flash_init(uint8 format_flag)
{
    FILE *font_handle;
    int i;
    char file_name[128];
    pdivoom_simu_flash_handle = divoom_system_malloc(sizeof(divoom_simu_flash_ctrl));

    pdivoom_simu_flash_handle->lock = divoom_system_create_lock();
    divoom_sd_card_mount();
    if(format_flag == 1)
    {
        for(i = 0; i < DIVOOM_SIMU_FLASH_FILE_SIZE/DIVOOM_SIMU_FLASH_CACHE_SIZE; i ++)
        {
            sprintf(file_name, DIVOOM_LEGACY_SD_ROOT "/divom_simu_flash%d.bin", i);
            divoom_app_com_del_file(file_name);
        }
    }
    pdivoom_simu_flash_handle->cache_off = 0;
    pdivoom_simu_flash_handle->write_cache_off = 0;
    divoom_simu_flash_read_data(pdivoom_simu_flash_handle->write_cache_buf, pdivoom_simu_flash_handle->write_cache_off);
    //divoom_simu_flash_read_data(pdivoom_simu_flash_handle->cache_buf, pdivoom_simu_flash_handle->cache_off);
    memcpy(pdivoom_simu_flash_handle->cache_buf, pdivoom_simu_flash_handle->write_cache_buf, DIVOOM_SIMU_FLASH_CACHE_SIZE);
    font_handle = divoom_port_file_open(DIVOOM_LEGACY_SD_ROOT "/divoom_out_fond16.bin", "rb");
    if(font_handle)
    {
        uint8 *file_buf;
        uint32 start_page, read_len, tol_len = 0;
        file_buf = divoom_system_malloc(4096);
        divoom_port_file_read(file_buf, 1, DIVOOM_PAGE_SIZE, font_handle);
        divoom_flash_get_data_addr_info(DIVOOM_CONFIG_UNLOCK_FONT_DATA, &start_page);
        divoom_system_read_spi_pages(start_page, &file_buf[DIVOOM_PAGE_SIZE], 1);
        if(memcmp(file_buf, &file_buf[DIVOOM_PAGE_SIZE], DIVOOM_PAGE_SIZE))
        {
            divoom_com_print_buf(file_buf, 512);
            divoom_port_file_seek(font_handle, 0, SEEK_SET);
            divoom_print("start copy font!\n");
            while(1)
            {
                read_len = divoom_port_file_read(file_buf, 1, DIVOOM_PAGE_SIZE * 16, font_handle);
                divoom_system_erase_spi_page(start_page/16);
                divoom_system_write_spi_pages(start_page, file_buf, 16);
                divoom_system_delay_ms(5);
                tol_len += read_len;
                if(read_len < DIVOOM_PAGE_SIZE * 16)
                {
                    break;
                }
                start_page += 16;
            }
            divoom_print("end copy font!: %d\n",tol_len);
            divoom_simu_flash_flush();
        }
        divoom_system_free(file_buf);
        divoom_port_file_close(font_handle);
    }
}

void  divoom_simu_flash_flush(void)
{
    divoom_system_get_lock(pdivoom_simu_flash_handle->lock);
    if(pdivoom_simu_flash_handle->wirte_flag)
    {
        divoom_print("write data!\n");
        divoom_simu_flash_write_data();
        pdivoom_simu_flash_handle->wirte_flag = 0;
    }
    divoom_system_free_lock(pdivoom_simu_flash_handle->lock);
}

void  divoom_simu_flash_check_read_offset(uint32 offset)
{
    if(offset/DIVOOM_SIMU_FLASH_CACHE_SIZE == pdivoom_simu_flash_handle->cache_off/DIVOOM_SIMU_FLASH_CACHE_SIZE)
    {
        return;
    }
    if(offset/DIVOOM_SIMU_FLASH_CACHE_SIZE == pdivoom_simu_flash_handle->write_cache_off/DIVOOM_SIMU_FLASH_CACHE_SIZE)
    {
        return;
    }

    pdivoom_simu_flash_handle->cache_off = offset / DIVOOM_SIMU_FLASH_CACHE_SIZE * DIVOOM_SIMU_FLASH_CACHE_SIZE;
    if(pdivoom_simu_flash_handle->write_cache_off == pdivoom_simu_flash_handle->cache_off)
    {
        memcpy(pdivoom_simu_flash_handle->cache_buf, pdivoom_simu_flash_handle->write_cache_buf, DIVOOM_SIMU_FLASH_CACHE_SIZE);
        return;
    }
    divoom_simu_flash_read_data(pdivoom_simu_flash_handle->cache_buf, pdivoom_simu_flash_handle->cache_off);
}

void  divoom_simu_flash_check_write_offset(uint32 offset)
{
    if(offset/DIVOOM_SIMU_FLASH_CACHE_SIZE == pdivoom_simu_flash_handle->write_cache_off/DIVOOM_SIMU_FLASH_CACHE_SIZE)
    {
        return;
    }
    if(pdivoom_simu_flash_handle->write_cache_off == pdivoom_simu_flash_handle->cache_off)
    {
        memcpy(pdivoom_simu_flash_handle->cache_buf, pdivoom_simu_flash_handle->write_cache_buf, DIVOOM_SIMU_FLASH_CACHE_SIZE);
    }
    if(pdivoom_simu_flash_handle->wirte_flag)
    {        
        divoom_simu_flash_write_data();
        pdivoom_simu_flash_handle->wirte_flag = 0;
    }
    pdivoom_simu_flash_handle->write_cache_off = offset / DIVOOM_SIMU_FLASH_CACHE_SIZE * DIVOOM_SIMU_FLASH_CACHE_SIZE;
    if(pdivoom_simu_flash_handle->write_cache_off == pdivoom_simu_flash_handle->cache_off)
    {
        memcpy(pdivoom_simu_flash_handle->write_cache_buf, pdivoom_simu_flash_handle->cache_buf, DIVOOM_SIMU_FLASH_CACHE_SIZE);
        return;
    }
    divoom_simu_flash_read_data(pdivoom_simu_flash_handle->write_cache_buf, pdivoom_simu_flash_handle->write_cache_off);
}

void  divoom_simu_flash_clear_data(uint32 offset, uint32 size)
{
    divoom_simu_flash_check_write_offset(offset);
    if(pdivoom_simu_flash_handle->wirte_flag == 0)
    {
        divoom_print("write flag: %x\n", pdivoom_simu_flash_handle->cache_off);
    }
    offset = offset % DIVOOM_SIMU_FLASH_CACHE_SIZE;
    memset(&pdivoom_simu_flash_handle->cache_buf[offset], 0xFF, size);
    pdivoom_simu_flash_handle->wirte_flag = 1;
    
}

/**
 * NAME               divoom_simu_flash_erase_spi_block
 * *  @brief           擦除ＳＰＩ,以64k为单位的块擦除
 * *  @param[in]   ?block_id   
                                
 * *  @return      none
 * */

T_BOOL divoom_simu_flash_erase_spi_block(T_U32 block_id)
{
    divoom_system_get_lock(pdivoom_simu_flash_handle->lock);
    divoom_simu_flash_clear_data(block_id * (64*1024), 64*1024);
    divoom_system_free_lock(pdivoom_simu_flash_handle->lock);
	return 1;
}




/**
 * NAME               divoom_simu_flash_erase_spi_page
 * *  @brief           擦除ＳＰＩ,以4K为单位的 页擦除
 * *  @param[in]   ?page   
                                
 * *  @return      none
 * */

T_BOOL  divoom_simu_flash_erase_spi_page(T_U32 sector)
{
    divoom_system_get_lock(pdivoom_simu_flash_handle->lock);
    divoom_simu_flash_clear_data(sector * 4 * 1024, 4 * 1024);
    divoom_system_free_lock(pdivoom_simu_flash_handle->lock);
	return 1;
}



/**
 * NAME               divoom_simu_flash_read_spi_pages
 * *  @brief           读ＳＰＩ的数据信息
 * *  @param[in]   ?page
                                buf
                                page_cnt
                                
 * *  @return      none
 * */

T_BOOL  divoom_simu_flash_read_spi_pages(T_U32 page, T_U8 *buf, T_U32 page_cnt)
{
    uint32 offset;
    int i;
    
    divoom_system_get_lock(pdivoom_simu_flash_handle->lock);

    for(i = 0; i < page_cnt; i ++)
    {        
        offset = page * 256;
        divoom_simu_flash_check_read_offset(offset);
        if(offset/DIVOOM_SIMU_FLASH_CACHE_SIZE == pdivoom_simu_flash_handle->write_cache_off/DIVOOM_SIMU_FLASH_CACHE_SIZE)
        {
            offset = offset % DIVOOM_SIMU_FLASH_CACHE_SIZE;
            memcpy(&buf[i * 256], &pdivoom_simu_flash_handle->write_cache_buf[offset], 256);
        }
        else
        {
            offset = offset % DIVOOM_SIMU_FLASH_CACHE_SIZE;
            memcpy(&buf[i * 256], &pdivoom_simu_flash_handle->cache_buf[offset], 256);
        }
        page ++;
    }
    divoom_system_free_lock(pdivoom_simu_flash_handle->lock);
    return 1;
}




/**
 * NAME               divoom_simu_flash_write_spi_pages
 * *  @brief           写ＳＰＩ的数据信息
 * *  @param[in]   ?page
                                buf
                                page_cnt
                                
 * *  @return      none
 * */

T_BOOL  divoom_simu_flash_write_spi_pages(T_U32 page, T_U8 *buf, T_U32 page_cnt)
{
    uint32 offset;
    int i;

    divoom_system_get_lock(pdivoom_simu_flash_handle->lock);
    for(i = 0; i < page_cnt; i ++)
    {        
        offset = page * 256;
        divoom_simu_flash_check_write_offset(offset);
        offset = offset % DIVOOM_SIMU_FLASH_CACHE_SIZE;
        memcpy(&pdivoom_simu_flash_handle->write_cache_buf[offset], &buf[i * 256], 256);
        pdivoom_simu_flash_handle->wirte_flag = 1;
        page ++;
    }
    divoom_system_free_lock(pdivoom_simu_flash_handle->lock);
    return 1;
}
#endif

