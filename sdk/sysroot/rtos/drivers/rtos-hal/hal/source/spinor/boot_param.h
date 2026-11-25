#ifndef __BOOT_PARAM_H__
#define __BOOT_PARAM_H__

#include "inter.h"

typedef struct {
        uint8_t magic[8];
        int32_t readcmd;
        int32_t read_mode;
        int32_t write_mode;
        int32_t flash_size;
        int32_t addr4b_opcodes;
        int32_t erase_size;
        int32_t delay_cycle; //When the frequency is greater than 60MHZ configured as 1;when the frequency is less than 24MHZ configured as 2;and other 3
        int32_t lock_flag;
        int32_t frequency;
        unsigned int sample_delay;
        unsigned int sample_mode;
        enum spi_nor_protocol read_proto;
        enum spi_nor_protocol write_proto;
        uint8_t read_dummy;
	unsigned int bootparam_addr;
} spinor_info_t;

extern spinor_info_t *get_spinor_info(void);
#ifdef CONFIG_DRIVERS_SPIF
extern int update_boot_param(struct nor_flash *nor, struct sunxi_spif *sspi);
#else
extern int update_boot_param(struct nor_flash *nor, struct nor_spi_master *spim);
#endif
extern void dump_spinor_info(spinor_info_t *spinor_info);
extern void *malloc_align(unsigned int len, unsigned int align, void **buf);

#endif /* __BOOT_PARAM_H__ */
