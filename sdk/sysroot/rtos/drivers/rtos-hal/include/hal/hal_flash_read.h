/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _HAL_FLASH_READ_H_
#define _HAL_FLASH_READ_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the flash memory.
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int hal_flash_read_flash_init(void);

/**
 * @brief Deinitializes the flash memory.
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int hal_flash_read_flash_deinit(void);

/**
 * @brief Reads data from the flash memory.
 *
 * @param sector The sector number to read from.
 * @param sector_num The number of sectors to read.
 * @param buff The buffer to store the read data.
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int hal_flash_read_flash_read(uint32_t sector, uint32_t sector_num, void *buff);

/**
 * @brief Reads the flash configuration.
 *
 * This function reads the configuration from the flash memory.
 *
 * @param pirv A pointer to a structure or data where the flash configuration will be stored.
 *
 * @return int Returns 0 if successful, non-zero if there is an error.
 */
int hal_flash_read_flash_config(void *pirv);

#ifdef CONFIG_DRIVERS_AW_FLASH_WRITE
/**
 * @brief Erases a specified range of flash memory sectors.
 *
 * This function erases the flash memory for the specified number of sectors starting from a given sector.
 *
 * @param sector The starting sector to erase.
 * @param sector_num The number of sectors to erase.
 *
 * @return int Returns 0 if successful, non-zero if there is an error.
 */
int hal_flash_read_flash_erase(uint32_t sector, uint32_t sector_num);

/**
 * @brief Writes data to a specified range of flash memory sectors.
 *
 * This function writes data to the flash memory for the specified number of sectors starting from a given sector.
 *
 * @param sector The starting sector to write to.
 * @param sector_num The number of sectors to write to.
 * @param buff A pointer to the buffer that contains the data to be written.
 *
 * @return int Returns 0 if successful, non-zero if there is an error.
 */
int hal_flash_read_flash_write(uint32_t sector, uint32_t sector_num, void *buff);
#endif /* CONFIG_DRIVERS_AW_FLASH_WRITE */

/**
 * @brief Sets the start sector for the GPT (GUID Partition Table).
 *
 * @param sector The starting sector number to set.
 */
void hal_flash_read_gpt_set_start_sector(uint32_t sector);

/**
 * @brief Initializes the GPT (GUID Partition Table).
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int hal_flash_read_gpt_init(void);

/**
 * @brief Deinitializes the GPT (GUID Partition Table).
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int hal_flash_read_gpt_deinit(void);

/**
 * @brief Gets the sector and sector number of a partition by its name.
 *
 * @param name The name of the partition.
 * @param sector Output parameter that will hold the starting sector number.
 * @param sector_num Output parameter that will hold the number of sectors.
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int hal_flash_read_get_partition_by_name(char *name, uint32_t *sector, uint32_t *sector_num);

#ifdef __cplusplus
}
#endif

#endif /* _HAL_FLASH_READ_H_ */
